"""SCB/1 设备客户端：TCP 控制连接 + UDP 实时接收。

- TCP 读线程独立运行，只负责收帧、可靠层维护与分发，绝不被 CSV/终端输出阻塞
  （业务数据一律经队列/回调解耦）。
- 全部COMMAND共享一个未完成请求槽；可靠结果先独立ACK再分派。
- 业务超时不释放请求槽；断线后的桥接墓碑可能阻塞全部查询到SPI新会话。
- 发送使用锁 + 循环处理部分发送；拆包粘包由 protocol.FrameParser 处理。
- UDP 接收队列默认 1024 包，溢出计数并写本地事件；坏包（CRC/长度/版本/
  session/record_count/类型不合法）计数丢弃，不解析半条记录。
- SUBSCRIBE 的 udp_port 限定 1~65535；UDP 只接受来自 TCP 对端 IP 的数据报。
- link_ready=false 时只查询连接状态，不下发控制请求。
- 断线后未决请求不自动重放；重连可尝试 GET_REQUEST_RESULT 查询；桥接墓碑可能阻塞到SPI会话改变。
"""

from __future__ import annotations

import json
import queue
import socket
import threading
import time

import scb_protocol as protocol
import scb_records as records
import scb_session as session_mod
from scb_session import (
    DeliverFrame, DuplicateReliable, LinkDead, ReliableReleased, SendBytes,
    RequestEntry, REQ_UNKNOWN, STATUS_UNKNOWN,
)

DEFAULT_HOST = "192.168.4.1"
DEFAULT_TCP_PORT = 5000
DEFAULT_UDP_PORT = 5001
DEFAULT_TIMEOUT_S = 5.0
UDP_QUEUE_SIZE = 1024
UDP_MAX_DATAGRAM = 1016        # UDP 每个数据报为一帧，不超过 1016B
TIMER_INTERVAL_S = 0.05        # 可靠层重发检查周期
_EVENT_THROTTLE_S = 1.0        # 本地事件（坏包/溢出）写盘节流

# link_ready=false 时允许的查询操作
_LINK_NOT_READY_ALLOWED_OPS = frozenset({"GET_STATUS"})


class ClientError(Exception):
    """客户端通用错误。"""


class ConnectError(ClientError):
    """连接/握手失败。"""


class RequestTimeout(ClientError):
    """请求超时（不代表设备未执行，需要时按 UNKNOWN 处理并查询）。"""


class LinkLostError(ClientError):
    """链路中断（TCP 断开或可靠层判死）。"""


class RequestRejected(ClientError):
    """设备返回错误状态。"""

    def __init__(self, status: str, result: dict | None = None):
        super().__init__(f"设备返回 {status}")
        self.status = status
        self.result = result or {}


class _Waiter:
    """单请求的等待句柄：cond 通知；RESULT/LOG_DATA 都按 request_id 匹配。"""

    def __init__(self) -> None:
        self.cond = threading.Condition()
        self.result: dict | None = None       # RESULT 的 JSON
        self.log_block: records.LogDataBlock | None = None
        self.error: Exception | None = None
        self.done = False

    def finish(self, *, result=None, log_block=None, error=None,
               done: bool = True) -> None:
        with self.cond:
            if result is not None:
                self.result = result
            if log_block is not None:
                self.log_block = log_block
            if error is not None:
                self.error = error
            if done:
                self.done = True
            self.cond.notify_all()


class DeviceClient:
    """SCB/1 电脑端客户端。

    event_handler(dict)：设备 EVENT（request_id=0）回调，在读取线程外分发
    （内部经事件队列 + 分发线程，回调慢不阻塞收帧）。
    local_event_handler(dict)：本地事件（坏包/溢出/断线等）回调。
    clock 可注入单调秒时钟（单测用）。
    """

    def __init__(
        self,
        host: str = DEFAULT_HOST,
        port: int = DEFAULT_TCP_PORT,
        udp_port: int = DEFAULT_UDP_PORT,
        timeout: float = DEFAULT_TIMEOUT_S,
        udp_queue_size: int = UDP_QUEUE_SIZE,
        event_handler=None,
        local_event_handler=None,
        clock=time.monotonic,
    ):
        self.host = host
        self.port = port
        self.udp_port = udp_port
        self.timeout = timeout
        self._clock = clock
        self.session = session_mod.Session(clock=clock)
        self._event_handler = event_handler
        self._local_event_handler = local_event_handler

        self._sock: socket.socket | None = None
        self._send_lock = threading.Lock()
        self._session_lock = threading.Lock()
        self._parser = protocol.FrameParser()
        self._waiters: dict[int, _Waiter] = {}
        self._waiters_lock = threading.Lock()
        self._pending_request: int | None = None  # 全部COMMAND共享一个待最终结果请求槽

        self._stop = threading.Event()
        self._read_thread: threading.Thread | None = None
        self._timer_thread: threading.Thread | None = None
        self._event_thread: threading.Thread | None = None
        self._event_queue: queue.Queue = queue.Queue()

        # UDP 接收
        self.telemetry_queue: queue.Queue = queue.Queue(maxsize=udp_queue_size)
        self.udp_stats = {
            "received": 0, "bad_packets": 0, "wrong_source": 0,
            "wrong_session": 0, "queue_overflow": 0,
        }
        self._udp_sock: socket.socket | None = None
        self._udp_thread: threading.Thread | None = None
        self._device_ip: str | None = None
        self._last_local_event: dict[str, float] = {}
        self._request_lock = threading.Lock()

    # ------------------------------------------------------------------
    # 连接与握手
    # ------------------------------------------------------------------

    @property
    def connected(self) -> bool:
        return self._sock is not None and self.session.alive

    def connect(self) -> dict:
        """建立 TCP 并完成 HELLO 握手，返回 HELLO_ACK 的 JSON。"""
        self.close()
        self._stop.clear()
        old_session = self.session
        self.session = session_mod.Session(clock=self._clock)
        # 保留此前请求记录：同会话重连后供 GET_REQUEST_RESULT 查询，绝不自动重放
        self.session.requests = old_session.requests
        # request_id 会话内递增非零，重连后接着编号，避免与缓存冲突
        self.session._next_request_id = old_session._next_request_id
        # 沿用旧 session_id 供 adopt_session 判断会话是否改变
        self.session.session_id = old_session.session_id
        self._parser = protocol.FrameParser()
        try:
            sock = socket.create_connection((self.host, self.port), timeout=self.timeout)
        except OSError as exc:
            raise ConnectError(f"无法连接 {self.host}:{self.port}：{exc}") from exc
        self._sock = sock
        self._device_ip = sock.getpeername()[0]
        # HELLO：session_id=0、seq=0，负载 {"role":"pc","protocol":1}
        hello = protocol.build_frame(
            protocol.MSG_HELLO,
            protocol.encode_json_payload({"role": "pc", "protocol": 1}),
            session_id=0, seq=0)
        try:
            self._send_all(hello)
            ack_payload = self._wait_hello_ack()
        except Exception:
            self.close()
            raise
        try:
            body = protocol.decode_json_payload(ack_payload)
        except protocol.ProtocolError as exc:
            self.close()
            raise ConnectError(f"HELLO_ACK 负载非法：{exc}") from exc
        if body.get("protocol") != 1:
            self.close()
            raise ConnectError(f"对端协议版本不支持：{body!r}")
        session_id = body.get("session_id")
        if not isinstance(session_id, int) or session_id == 0:
            self.close()
            raise ConnectError(f"HELLO_ACK 缺少有效 session_id：{body!r}")
        link_ready = bool(body.get("link_ready", False))
        changed = self.session.adopt_session(session_id, link_ready)
        if changed:
            self._emit_local("session", session_id=session_id, link_ready=link_ready)
        # 握手完成后启动读线程、可靠层定时器、事件分发线程
        self._read_thread = threading.Thread(
            target=self._read_loop, name="scb-tcp-read", daemon=True)
        self._timer_thread = threading.Thread(
            target=self._timer_loop, name="scb-timer", daemon=True)
        self._event_thread = threading.Thread(
            target=self._event_loop, name="scb-event", daemon=True)
        self._read_thread.start()
        self._timer_thread.start()
        self._event_thread.start()
        return body

    def _wait_hello_ack(self) -> bytes:
        """握手期间同步等待 HELLO_ACK（读线程尚未启动）。"""
        deadline = time.monotonic() + self.timeout
        assert self._sock is not None
        self._sock.settimeout(0.2)
        try:
            while time.monotonic() < deadline:
                try:
                    chunk = self._sock.recv(4096)
                except socket.timeout:
                    continue
                if not chunk:
                    raise ConnectError("握手期间连接被关闭")
                try:
                    frames = self._parser.feed(chunk)
                except protocol.ProtocolError as exc:
                    raise ConnectError(f"握手期间收到非法帧：{exc}") from exc
                for header, payload in frames:
                    if header.msg_type == protocol.MSG_HELLO_ACK:
                        return payload
                    if header.msg_type == protocol.MSG_HELLO:
                        continue  # 重复 HELLO 回显忽略
                    raise ConnectError(f"握手期间收到意外帧 {header.type_name}")
        finally:
            self._sock.settimeout(None)
        raise ConnectError("等待 HELLO_ACK 超时")

    def close(self) -> None:
        """关闭连接与线程；不清除请求记录（供重连后查询）。"""
        self._stop.set()
        sock, self._sock = self._sock, None
        if sock is not None:
            try:
                sock.shutdown(socket.SHUT_RDWR)
            except OSError:
                pass
            try:
                sock.close()
            except OSError:
                pass
        for thread in (self._read_thread, self._timer_thread):
            if thread is not None and thread.is_alive():
                thread.join(timeout=2.0)
        self._read_thread = self._timer_thread = None
        with self._session_lock:
            self.session.on_disconnect()
        self._wake_all_waiters(LinkLostError("连接已关闭"))
        self.stop_udp()
        # 事件线程最后停，保证队列里的事件被处理
        if self._event_thread is not None and self._event_thread.is_alive():
            self._event_queue.put(None)
            self._event_thread.join(timeout=2.0)
        self._event_thread = None

    # ------------------------------------------------------------------
    # 底层收发
    # ------------------------------------------------------------------

    def _send_all(self, data: bytes) -> None:
        """循环发送直到全部写出（处理部分发送）。"""
        view = memoryview(data)
        with self._send_lock:
            sock = self._sock
            if sock is None:
                raise LinkLostError("连接未建立")
            while view:
                try:
                    sent = sock.send(view)
                except OSError as exc:
                    raise LinkLostError(f"发送失败：{exc}") from exc
                if sent <= 0:
                    raise LinkLostError("发送返回 0 字节")
                view = view[sent:]

    def _read_loop(self) -> None:
        """TCP 读线程：只收帧、维护可靠层、分发；不做任何慢操作。"""
        try:
            while not self._stop.is_set():
                try:
                    chunk = self._sock.recv(4096)
                except OSError:
                    break
                if not chunk:
                    break
                try:
                    frames = self._parser.feed(chunk)
                except protocol.ProtocolError as exc:
                    # 非法 TCP 帧：按协议关闭连接
                    self._emit_local("bad_tcp_frame", error=str(exc))
                    break
                for header, payload in frames:
                    with self._session_lock:
                        events = self.session.feed_frame(header, payload)
                    self._handle_session_events(events)
                    if not self.connected:
                        break
                if not self.connected:
                    break
        finally:
            self._handle_disconnect()

    def _timer_loop(self) -> None:
        """可靠层定时器：检查重发；判死则断开。"""
        while not self._stop.wait(TIMER_INTERVAL_S):
            if not self.session.alive:
                continue
            with self._session_lock:
                events = self.session.poll()
            self._handle_session_events(events)

    def _event_loop(self) -> None:
        """设备事件分发线程：回调慢不阻塞 TCP 读线程。"""
        while True:
            item = self._event_queue.get()
            if item is None:
                break
            if self._event_handler is not None:
                try:
                    self._event_handler(item)
                except Exception as exc:  # 回调异常不拖垮客户端
                    self._emit_local("event_handler_error", error=str(exc))

    def _handle_session_events(self, events) -> None:
        for event in events:
            if isinstance(event, SendBytes):
                try:
                    self._send_all(event.data)
                except LinkLostError:
                    return
            elif isinstance(event, DeliverFrame):
                self._dispatch(event.header, event.payload)
            elif isinstance(event, DuplicateReliable):
                if not self._ack_received():
                    return
            elif isinstance(event, ReliableReleased):
                pass
            elif isinstance(event, LinkDead):
                self._emit_local("link_dead", reason=event.reason)
                self._close_socket_only()
                return

    def _ack_received(self) -> bool:
        """独立确认可靠入帧；失败关闭本连接，不把结果当作已完成。"""
        try:
            with self._session_lock:
                events = self.session.send_frame(protocol.MSG_IDLE, b"")
            for event in events:
                if isinstance(event, SendBytes):
                    self._send_all(event.data)
        except (LinkLostError, session_mod.SessionError):
            self._handle_disconnect()
            return False
        return self.connected

    def _dispatch(self, header: protocol.FrameHeader, payload: bytes) -> None:
        """分发新帧（在 TCP 读线程内执行，必须快）。"""
        if header.msg_type == protocol.MSG_RESULT:
            try:
                body = protocol.decode_json_payload(payload)
                if not isinstance(body, dict):
                    raise protocol.ProtocolError("RESULT根值必须为对象")
            except protocol.ProtocolError as exc:
                self._emit_local("bad_result", error=str(exc))
                self._handle_disconnect()
                return
            if header.reliable and not self._ack_received():
                return
            request_id = header.request_id
            status = str(body.get("status", ""))
            body["request_id"] = request_id  # RESULT 沿用帧头 request_id，并入结果便于业务记录
            self.session.requests.update_result(request_id, status, body)
            final = session_mod.is_final_status(status)
            waiter = self._take_waiter(request_id, final=final)
            if final:
                self._release_request_slot(request_id)
            if waiter is not None:
                # RECEIVED只更新结果；最终ACK已发出，才解除单请求等待。
                waiter.finish(result=body, done=final)
        elif header.msg_type == protocol.MSG_LOG_DATA:
            try:
                block = records.unpack_log_data(payload)
            except records.RecordError as exc:
                self._emit_local("bad_log_data", error=str(exc))
                self._handle_disconnect()
                return
            if header.reliable and not self._ack_received():
                return
            waiter = self._take_waiter(header.request_id, final=True)
            self._release_request_slot(header.request_id)
            if waiter is not None:
                waiter.finish(log_block=block)
        elif header.msg_type == protocol.MSG_EVENT:
            try:
                body = protocol.decode_json_payload(payload)
                if not isinstance(body, dict):
                    raise protocol.ProtocolError("EVENT根值必须为对象")
            except protocol.ProtocolError as exc:
                self._emit_local("bad_event", error=str(exc))
                self._handle_disconnect()
                return
            if header.reliable and not self._ack_received():
                return
            self._event_queue.put(dict(body))
        else:
            self._emit_local("unexpected_frame", type=header.type_name)

    def _handle_disconnect(self) -> None:
        was_connected = self._sock is not None
        self._close_socket_only()
        with self._session_lock:
            self.session.on_disconnect()
        self._wake_all_waiters(LinkLostError("TCP 连接断开"))
        if was_connected:
            self._emit_local("disconnected")

    def _wake_all_waiters(self, error: Exception) -> None:
        with self._waiters_lock:
            waiters = list(self._waiters.values())
            self._waiters.clear()
        with self._request_lock:
            self._pending_request = None
        for waiter in waiters:
            waiter.finish(error=error)

    def _close_socket_only(self) -> None:
        sock, self._sock = self._sock, None
        if sock is not None:
            try:
                sock.shutdown(socket.SHUT_RDWR)
            except OSError:
                pass
            try:
                sock.close()
            except OSError:
                pass

    # ------------------------------------------------------------------
    # 请求 API
    # ------------------------------------------------------------------

    def request(self, op: str, args: dict | None = None,
                timeout: float | None = None) -> dict:
        """发送 COMMAND 并等待首个 RESULT（RECEIVED 也算返回）。

        返回设备 RESULT 的 JSON（至少含 op/status）。超时抛 RequestTimeout。
        """
        waiter, request_id = self._send_command(op, args or {})
        return self._wait(waiter, request_id, timeout, wait_final=False)

    def request_final(self, op: str, args: dict | None = None,
                      timeout: float | None = None) -> dict:
        """发送 COMMAND 并等待最终状态：RECEIVED 保持待生效，APPLIED/错误才结束。"""
        waiter, request_id = self._send_command(op, args or {})
        return self._wait(waiter, request_id, timeout, wait_final=True)

    def request_log_data(self, op: str, args: dict,
                         timeout: float | None = None) -> records.LogDataBlock:
        """发送 LOG_READ 类请求，等待 LOG_DATA（设备出错时抛 RequestRejected）。"""
        waiter, request_id = self._send_command(op, args)
        deadline = time.monotonic() + (timeout or self.timeout)
        with waiter.cond:
            while not waiter.done:
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    self._take_waiter(request_id, final=True)
                    raise RequestTimeout(f"{op} 等待 LOG_DATA 超时；结果未知，请求槽仍保留")
                waiter.cond.wait(remaining)
        if waiter.error is not None:
            raise waiter.error
        if waiter.log_block is not None:
            return waiter.log_block
        status = str((waiter.result or {}).get("status", ""))
        raise RequestRejected(status, waiter.result)

    def _send_command(self, op: str, args: dict) -> tuple[_Waiter, int]:
        if not self.connected:
            raise LinkLostError("连接未建立或已断开")
        if not self.session.link_ready and op not in _LINK_NOT_READY_ALLOWED_OPS:
            raise ClientError("link_ready=false：只允许查询连接状态，不下发控制请求")
        payload = protocol.encode_json_payload({"op": op, "args": args})
        request_id = self._claim_request_slot()
        try:
            entry = RequestEntry(request_id=request_id, op=op, content=payload,
                                 local_time_ns=time.time_ns())
            waiter = _Waiter()
            self.session.requests.add(entry)
            with self._waiters_lock:
                self._waiters[request_id] = waiter
            with self._session_lock:
                events = self.session.send_frame(protocol.MSG_COMMAND, payload,
                                                 request_id=request_id)
            self._handle_session_events(events)
        except Exception:
            with self._waiters_lock:
                self._waiters.pop(request_id, None)
            self._release_request_slot(request_id)
            raise
        return waiter, request_id

    def _wait(self, waiter: _Waiter, request_id: int,
              timeout: float | None, wait_final: bool) -> dict:
        deadline = time.monotonic() + (timeout or self.timeout)
        with waiter.cond:
            while True:
                if waiter.done:
                    break
                if waiter.result is not None and not wait_final:
                    break
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    self._take_waiter(request_id, final=True)
                    raise RequestTimeout(
                        f"请求 {request_id} 超时且结果未知；请求槽保留到迟到最终响应或断线，"
                        "重连查询可能被桥接墓碑阻塞到SPI会话改变")
                waiter.cond.wait(min(remaining, 0.2))
        if waiter.error is not None:
            raise waiter.error
        return waiter.result

    def _take_waiter(self, request_id: int, final: bool) -> _Waiter | None:
        with self._waiters_lock:
            waiter = self._waiters.get(request_id)
            if waiter is None:
                return None
            if final:
                self._waiters.pop(request_id, None)
            return waiter

    def _claim_request_slot(self) -> int:
        """在同一临界区取得唯一请求编号；分配失败不占槽。"""
        with self._request_lock:
            if not self.connected:
                raise LinkLostError("连接未建立或已断开")
            if self._pending_request is not None:
                raise ClientError("已有命令等待最终结果；所有命令必须串行，超时不表示槽已释放")
            request_id = self.session.allocate_request_id()
            self._pending_request = request_id
            return request_id

    def _release_request_slot(self, request_id: int) -> None:
        with self._request_lock:
            if self._pending_request == request_id:
                self._pending_request = None

    # ------------------------------------------------------------------
    # 断线恢复：查询未决请求，绝不自动重放
    # ------------------------------------------------------------------

    def recover_pending(self) -> list[RequestEntry]:
        """重连后处理未决请求：逐条 GET_REQUEST_RESULT；NOT_FOUND 标 UNKNOWN。

        不自动重放 SET_PARAMS。桥接墓碑存在时查询可能背压到SPI会话改变。
        返回处理后的未决请求条目列表。
        """
        unresolved = self.session.requests.unresolved()
        for entry in unresolved:
            try:
                result = self.request_final(
                    "GET_REQUEST_RESULT", {"target_request_id": entry.request_id})
            except ClientError:
                self.session.requests.mark_unknown(entry.request_id)
                continue
            status = str(result.get("status", ""))
            if status == "NOT_FOUND":
                self.session.requests.mark_unknown(entry.request_id)
                self._emit_local("request_unknown", request_id=entry.request_id,
                                 op=entry.op)
            else:
                self.session.requests.update_result(
                    entry.request_id, status, result.get("result", result))
        return unresolved

    # ------------------------------------------------------------------
    # UDP 实时接收
    # ------------------------------------------------------------------

    def subscribe(self, udp_port: int | None = None) -> dict:
        """SUBSCRIBE 注册 UDP 端口（1~65535）。目的 IP 由 ESP32 取 TCP 对端。"""
        port = self.udp_port if udp_port is None else udp_port
        if not (1 <= port <= 65535):
            raise ClientError(f"udp_port {port} 超出 1~65535 范围")
        result = self.request_final("SUBSCRIBE", {"udp_port": port})
        if result.get("status") == "OK":
            self.session.subscribed_udp_port = port
        return result

    def start_udp(self, udp_port: int | None = None) -> None:
        """启动 UDP 接收线程；验证后的包放入 telemetry_queue。

        队列满：溢出计数并写本地事件（节流）；坏包计数丢弃，不解析半条记录。
        """
        if self._udp_thread is not None:
            return
        port = self.udp_port if udp_port is None else udp_port
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.bind(("0.0.0.0", port))
        sock.settimeout(0.2)
        self._udp_sock = sock
        self._udp_thread = threading.Thread(
            target=self._udp_loop, name="scb-udp-recv", daemon=True)
        self._udp_thread.start()

    def stop_udp(self) -> None:
        sock, self._udp_sock = self._udp_sock, None
        if sock is not None:
            try:
                sock.close()
            except OSError:
                pass
        thread, self._udp_thread = self._udp_thread, None
        if thread is not None and thread.is_alive():
            thread.join(timeout=2.0)

    def _udp_loop(self) -> None:
        sock = self._udp_sock
        while sock is not None and not self._stop.is_set():
            try:
                data, addr = sock.recvfrom(2048)
            except socket.timeout:
                continue
            except OSError:
                break
            recv_time_ns = time.time_ns()
            # 只接受 TCP 对端 IP 的数据报
            if self._device_ip is not None and addr[0] != self._device_ip:
                self.udp_stats["wrong_source"] += 1
                self._emit_local_throttled("udp_wrong_source", source=addr[0])
                continue
            if len(data) > UDP_MAX_DATAGRAM:
                self.udp_stats["bad_packets"] += 1
                self._emit_local_throttled("udp_bad_packet", reason="超过 1016B")
                continue
            try:
                header, payload = protocol.decode_frame(data)
            except protocol.ProtocolError as exc:
                self.udp_stats["bad_packets"] += 1
                self._emit_local_throttled("udp_bad_packet", reason=str(exc))
                continue
            if header.msg_type != protocol.MSG_TELEMETRY:
                self.udp_stats["bad_packets"] += 1
                self._emit_local_throttled("udp_bad_packet",
                                           reason=f"UDP 非 TELEMETRY：{header.type_name}")
                continue
            if header.session_id != self.session.session_id:
                self.udp_stats["wrong_session"] += 1
                self._emit_local_throttled("udp_wrong_session",
                                           session_id=header.session_id)
                continue
            try:
                packet = records.unpack_telemetry(payload)
            except records.RecordError as exc:
                self.udp_stats["bad_packets"] += 1
                self._emit_local_throttled("udp_bad_packet", reason=str(exc))
                continue
            try:
                self.telemetry_queue.put_nowait((recv_time_ns, data, packet))
                self.udp_stats["received"] += 1
            except queue.Full:
                self.udp_stats["queue_overflow"] += 1
                self._emit_local_throttled(
                    "udp_queue_overflow", count=self.udp_stats["queue_overflow"])

    # ------------------------------------------------------------------
    # 本地事件
    # ------------------------------------------------------------------

    def _emit_local(self, kind: str, **fields) -> None:
        if self._local_event_handler is not None:
            event = {"event": kind, "local_time_ns": time.time_ns()}
            event.update(fields)
            try:
                self._local_event_handler(event)
            except Exception:
                pass

    def _emit_local_throttled(self, kind: str, **fields) -> None:
        """坏包/溢出等高频本地事件：计数始终累加，写事件按 1 秒节流。"""
        now = time.monotonic()
        last = self._last_local_event.get(kind, 0.0)
        if now - last >= _EVENT_THROTTLE_S:
            self._last_local_event[kind] = now
            self._emit_local(kind, **fields)

    # ------------------------------------------------------------------
    # 上下文管理
    # ------------------------------------------------------------------

    def __enter__(self) -> "DeviceClient":
        self.connect()
        return self

    def __exit__(self, *exc) -> None:
        self.close()
