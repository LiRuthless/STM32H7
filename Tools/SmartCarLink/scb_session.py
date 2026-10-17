"""SCB/1 可靠层与会话状态机（纯逻辑，不依赖 socket，可注入时钟便于单测）。

实现任务书第 3 节的传输规则：
- 每方向最多一个可靠帧在途；ack_seq 精确匹配才释放；释放后才发队列中下一可靠帧。
- 200ms 未确认重发，最多三次；仍失败标记链路死亡（TCP 由上层关闭重连）。
- 重发保留原 seq 和原内容；正常新帧 seq 单调递增，0 保留。
- 去重以可靠帧身份为准：重复可靠帧只确认、不重复分发；
  不因后来 IDLE/TELEMETRY 的 seq 更大而丢弃尚未确认的可靠重发。
- 会话改变清除传输缓存、未应用请求和订阅状态。
- request_id 会话内递增且非零；最多缓存最近 32 个请求及结果：
  同编号同内容返回缓存结果，同编号不同内容判 INVALID_ARG；
  断线/会话改变后未决请求标 UNKNOWN，绝不自动重放 SET_PARAMS。
- 业务等待与传输分离：RECEIVED 只是非最终状态，APPLIED/错误状态才结束调参等待。
"""

from __future__ import annotations

import time
from collections import deque
from dataclasses import dataclass, field

import scb_protocol as protocol

# ---------------------------------------------------------------------------
# 常量
# ---------------------------------------------------------------------------

RELIABLE_TIMEOUT_S = 0.2      # 200ms 未确认重发
MAX_RETRANSMIT = 3            # 最多重发三次
REQUEST_CACHE_CAPACITY = 32   # 缓存最近 32 个请求及结果

# 最终状态集合：RESULT 的 status 属于这些值时结束业务等待；RECEIVED 不是最终状态
FINAL_STATUSES = frozenset({
    "OK", "APPLIED", "SAVED", "INVALID_ARG", "OUT_OF_RANGE",
    "BUSY", "SPECIAL_ELEMENT", "NOT_FOUND", "IO_ERROR",
})
STATUS_RECEIVED = "RECEIVED"   # 已接收待生效，非最终
STATUS_UNKNOWN = "UNKNOWN"     # 断线/会话改变后无法确认的本地标记（非协议状态）


class SessionError(ValueError):
    """会话/可靠层逻辑错误。"""


def is_final_status(status: str) -> bool:
    """status 是否为最终状态（RECEIVED 返回 False）。"""
    return status in FINAL_STATUSES


# ---------------------------------------------------------------------------
# 会话层事件（feed_frame/poll 的返回项）
# ---------------------------------------------------------------------------

@dataclass(frozen=True)
class SendBytes:
    """需要写到 socket 的字节（新帧或重发帧）。"""
    data: bytes


@dataclass(frozen=True)
class DeliverFrame:
    """通过校验与去重的新帧，需要分发给业务层。"""
    header: protocol.FrameHeader
    payload: bytes


@dataclass(frozen=True)
class DuplicateReliable:
    """重复可靠帧：只确认（ack_seq 已含），不重复分发。"""
    header: protocol.FrameHeader


@dataclass(frozen=True)
class ReliableReleased:
    """在途可靠帧被 ack_seq 精确匹配释放。"""
    seq: int


@dataclass(frozen=True)
class LinkDead:
    """可靠帧重发三次仍未确认，链路判定死亡。"""
    reason: str


# ---------------------------------------------------------------------------
# 请求缓存（最近 32 个请求及结果）
# ---------------------------------------------------------------------------

# 请求状态
REQ_PENDING = "pending"    # 已发送，未得任何业务结果
REQ_RECEIVED = "received"  # 设备返回 RECEIVED，等待 APPLIED/错误
REQ_FINAL = "final"        # 已得最终状态
REQ_UNKNOWN = "unknown"    # 断线/会话改变后无法确认（NOT_FOUND 查询后标记）


@dataclass
class RequestEntry:
    """一条业务请求的记录。"""

    request_id: int
    op: str
    content: bytes          # 请求 JSON 负载原文，用于同编号同/异内容判定
    state: str = REQ_PENDING
    status: str | None = None      # 最终/最近状态码
    result: dict | None = None     # RESULT 的完整 JSON
    local_time_ns: int = 0         # 本机时间
    applied_values: dict | None = None  # APPLIED 时设备返回的实际完整 values

    @property
    def is_final(self) -> bool:
        return self.state == REQ_FINAL


class RequestCache:
    """会话内请求缓存：容量 32，同编号同内容返回缓存，同编号不同内容 INVALID_ARG。"""

    def __init__(self, capacity: int = REQUEST_CACHE_CAPACITY):
        self.capacity = capacity
        self._entries: dict[int, RequestEntry] = {}
        self._order: deque[int] = deque()

    def __len__(self) -> int:
        return len(self._entries)

    def check(self, request_id: int, content: bytes) -> str:
        """判定请求编号/内容关系："new" / "same"（返回缓存结果）/ "conflict"（INVALID_ARG）。"""
        entry = self._entries.get(request_id)
        if entry is None:
            return "new"
        return "same" if entry.content == content else "conflict"

    def add(self, entry: RequestEntry) -> None:
        if entry.request_id in self._entries:
            self._order.remove(entry.request_id)
        self._entries[entry.request_id] = entry
        self._order.append(entry.request_id)
        while len(self._order) > self.capacity:
            oldest = self._order.popleft()
            self._entries.pop(oldest, None)

    def get(self, request_id: int) -> RequestEntry | None:
        return self._entries.get(request_id)

    def update_result(self, request_id: int, status: str, result: dict) -> RequestEntry | None:
        """登记 RESULT：RECEIVED 置 received，最终状态置 final。未知编号返回 None。"""
        entry = self._entries.get(request_id)
        if entry is None:
            return None
        entry.status = status
        entry.result = result
        entry.state = REQ_FINAL if is_final_status(status) else REQ_RECEIVED
        if status == "APPLIED" and isinstance(result.get("values"), dict):
            entry.applied_values = result["values"]
        return entry

    def unresolved(self) -> list[RequestEntry]:
        """所有未得最终结果的请求。"""
        return [e for e in self._entries.values() if e.state in (REQ_PENDING, REQ_RECEIVED)]

    def mark_unknown(self, request_id: int) -> None:
        entry = self._entries.get(request_id)
        if entry is not None and entry.state in (REQ_PENDING, REQ_RECEIVED):
            entry.state = REQ_UNKNOWN
            entry.status = STATUS_UNKNOWN

    def clear(self, mark_unresolved_unknown: bool = True) -> None:
        """清空缓存；未决请求按需标 UNKNOWN（不静默丢弃、不自动重放）。"""
        if mark_unresolved_unknown:
            for entry in self._entries.values():
                if entry.state in (REQ_PENDING, REQ_RECEIVED):
                    entry.state = REQ_UNKNOWN
                    entry.status = STATUS_UNKNOWN


# ---------------------------------------------------------------------------
# 会话状态机
# ---------------------------------------------------------------------------

@dataclass
class _InFlight:
    seq: int
    frame: bytes
    last_send: float
    retries: int = 0


@dataclass
class _QueuedReliable:
    msg_type: int
    payload: bytes
    request_id: int


class Session:
    """单条 TCP 控制连接的可靠层与会话状态。

    所有方法返回事件列表；事件中的 SendBytes 由上层写入 socket。
    时钟可注入（clock 返回单调秒），便于单测重发超时。
    """

    def __init__(self, clock=time.monotonic):
        self._clock = clock
        self.session_id = 0
        self.link_ready = False
        self.alive = True
        self.requests = RequestCache()
        self.subscribed_udp_port: int | None = None
        self._reset_transport()

    def _reset_transport(self) -> None:
        self._tx_seq = 0                      # 本方向已分配的最后一个 seq（0 保留）
        self._in_flight: _InFlight | None = None
        self._queue: deque[_QueuedReliable] = deque()
        self._rx_reliable_seqs: set[int] = set()   # 已接受的对端可靠帧 seq（去重身份）
        self._rx_reliable_order: deque[int] = deque()
        self._ack_seq = 0                     # 最后接受的对端可靠消息序号
        self._next_request_id = 1             # request_id 会话内递增非零

    # ------------------------------------------------------------------
    # 会话管理
    # ------------------------------------------------------------------

    def adopt_session(self, session_id: int, link_ready: bool) -> bool:
        """采用握手得到的会话。会话改变时清除传输缓存、未应用请求和订阅状态。

        返回是否发生了会话改变。
        """
        changed = session_id != self.session_id
        if changed:
            self.requests.clear(mark_unresolved_unknown=True)
            self._reset_transport()
            self.subscribed_udp_port = None
        self.session_id = session_id
        self.link_ready = link_ready
        self.alive = True
        return changed

    def on_disconnect(self) -> None:
        """TCP 断开：传输缓存清除；未决请求保留记录待重连后 GET_REQUEST_RESULT 查询。

        绝不自动重放 SET_PARAMS。
        """
        self.alive = False
        self._in_flight = None
        self._queue.clear()
        self.subscribed_udp_port = None

    # ------------------------------------------------------------------
    # 请求编号与缓存
    # ------------------------------------------------------------------

    def allocate_request_id(self) -> int:
        """分配会话内递增且非零的 request_id。"""
        request_id = self._next_request_id
        self._next_request_id += 1
        if self._next_request_id > 0xFFFFFFFF:  # 接近回绕：重建连接（上层负责）
            self._next_request_id = 1
        return request_id

    # ------------------------------------------------------------------
    # 发送
    # ------------------------------------------------------------------

    def send_frame(
        self,
        msg_type: int,
        payload: bytes,
        request_id: int = 0,
        record_count: int = 0,
        now: float | None = None,
    ) -> list:
        """构造一帧。可靠帧受"每方向最多一个在途"限制，超出时排队。

        返回事件列表（通常为一个 SendBytes；排队时为空）。
        """
        if not self.alive:
            raise SessionError("链路已死亡，不能发送")
        if protocol.default_flags(msg_type) & protocol.FLAG_RELIABLE:
            if self._in_flight is not None:
                self._queue.append(_QueuedReliable(msg_type, payload, request_id))
                return []
            return [self._start_reliable(msg_type, payload, request_id, now)]
        return [SendBytes(self._build(msg_type, payload, request_id, record_count, 0))]

    def _build(self, msg_type, payload, request_id, record_count, flags) -> bytes:
        self._tx_seq += 1
        return protocol.build_frame(
            msg_type, payload,
            session_id=self.session_id,
            seq=self._tx_seq,
            ack_seq=self._ack_seq,
            request_id=request_id,
            record_count=record_count,
            flags=flags if flags else None,
        )

    def _start_reliable(self, msg_type, payload, request_id, now) -> SendBytes:
        frame = self._build(msg_type, payload, request_id, 0, protocol.FLAG_RELIABLE)
        # seq 是帧内第 16~20 字节，取回登记在途
        seq = int.from_bytes(frame[16:20], "little")
        self._in_flight = _InFlight(
            seq=seq, frame=frame,
            last_send=self._clock() if now is None else now,
        )
        return SendBytes(frame)

    # ------------------------------------------------------------------
    # 接收
    # ------------------------------------------------------------------

    def feed_frame(self, header: protocol.FrameHeader, payload: bytes,
                   now: float | None = None) -> list:
        """喂入一个已通过协议层校验的帧，返回事件列表。

        处理：会话匹配、ack 释放在途、可靠帧去重、ack_seq 更新。
        """
        events: list = []
        if header.session_id != self.session_id:
            return events  # 旧会话帧丢弃，不确认、不分发
        # ack 处理：精确匹配才释放在途可靠帧
        if self._in_flight is not None and header.ack_seq == self._in_flight.seq:
            released = self._in_flight.seq
            self._in_flight = None
            events.append(ReliableReleased(released))
            # 释放后立即发出队列中的下一个可靠帧
            if self._queue:
                nxt = self._queue.popleft()
                events.append(self._start_reliable(
                    nxt.msg_type, nxt.payload, nxt.request_id, now))
        # 可靠帧去重：以帧身份（seq）为准，与后来更大的不可靠 seq 无关
        if header.reliable:
            if header.seq in self._rx_reliable_seqs:
                events.append(DuplicateReliable(header))
                return events
            self._rx_reliable_seqs.add(header.seq)
            self._rx_reliable_order.append(header.seq)
            while len(self._rx_reliable_order) > 64:
                self._rx_reliable_seqs.discard(self._rx_reliable_order.popleft())
            self._ack_seq = header.seq  # 最后接受的可靠消息序号
        events.append(DeliverFrame(header, payload))
        return events

    # ------------------------------------------------------------------
    # 定时：重发与链路死亡
    # ------------------------------------------------------------------

    def poll(self, now: float | None = None) -> list:
        """检查在途可靠帧是否超时需重发；重发三次仍未确认判链路死亡。"""
        events: list = []
        if self._in_flight is None:
            return events
        now = self._clock() if now is None else now
        if now - self._in_flight.last_send < RELIABLE_TIMEOUT_S:
            return events
        if self._in_flight.retries >= MAX_RETRANSMIT:
            self._in_flight = None
            self._queue.clear()
            self.alive = False
            events.append(LinkDead("可靠帧重发 %d 次仍未确认" % MAX_RETRANSMIT))
            return events
        self._in_flight.retries += 1
        self._in_flight.last_send = now
        events.append(SendBytes(self._in_flight.frame))  # 重发保留原 seq 和原内容
        return events

    # ------------------------------------------------------------------
    # 查询
    # ------------------------------------------------------------------

    @property
    def in_flight_seq(self) -> int | None:
        return None if self._in_flight is None else self._in_flight.seq

    @property
    def queued_count(self) -> int:
        return len(self._queue)

    @property
    def ack_seq(self) -> int:
        return self._ack_seq

    @property
    def tx_seq(self) -> int:
        return self._tx_seq
