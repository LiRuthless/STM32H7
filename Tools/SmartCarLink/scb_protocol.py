"""SCB/1 协议 v1 帧头编解码与校验（纯协议层，不依赖 socket）。

本模块实现任务书 03-python-agent.md 第 2、3 节规定的 32B 帧头、
CRC32 校验、JSON 负载编解码以及 TCP 增量帧解析器。
所有多字节整数为小端；帧头 struct 格式为 ``<4sBBHHHIIIII``，长度必须等于 32。

CRC32 计算方式：先对帧头字节 0~27（不含 CRC 字段本身）计算 zlib.crc32，
再继续计算 payload_len 字节负载，结果截断为 u32。
"""

from __future__ import annotations

import json
import struct
import zlib
from dataclasses import dataclass

# ---------------------------------------------------------------------------
# 常量定义
# ---------------------------------------------------------------------------

MAGIC = b"SCB1"                  # 帧头魔数
PROTOCOL_VERSION = 1             # 协议版本，固定 1
HEADER_FORMAT = "<4sBBHHHIIIII"  # 帧头 struct 格式
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)
assert HEADER_SIZE == 32, f"帧头长度必须为 32B，实际 {HEADER_SIZE}B"

MAX_PAYLOAD_SIZE = 992           # 负载上限（字节）
FLAG_RELIABLE = 0x0001           # flags bit0：可靠帧，其余位必须为 0

# 消息类型
MSG_HELLO = 0x01
MSG_HELLO_ACK = 0x02
MSG_IDLE = 0x03
MSG_COMMAND = 0x10
MSG_RESULT = 0x11
MSG_EVENT = 0x12
MSG_TELEMETRY = 0x20
MSG_LOG_DATA = 0x30

MESSAGE_TYPE_NAMES = {
    MSG_HELLO: "HELLO",
    MSG_HELLO_ACK: "HELLO_ACK",
    MSG_IDLE: "IDLE",
    MSG_COMMAND: "COMMAND",
    MSG_RESULT: "RESULT",
    MSG_EVENT: "EVENT",
    MSG_TELEMETRY: "TELEMETRY",
    MSG_LOG_DATA: "LOG_DATA",
}

# 不设 RELIABLE 的类型：HELLO、HELLO_ACK、IDLE、TELEMETRY；其余类型默认置 RELIABLE
_UNRELIABLE_TYPES = frozenset({MSG_HELLO, MSG_HELLO_ACK, MSG_IDLE, MSG_TELEMETRY})

# TELEMETRY 负载结构：24B 元数据头 + record_count 条 96B 记录
TELEMETRY_HEADER_SIZE = 24
TELEMETRY_RECORD_SIZE = 96
TELEMETRY_MIN_RECORDS = 1
TELEMETRY_MAX_RECORDS = 10


class ProtocolError(ValueError):
    """协议帧校验失败。TCP 侧收到此类错误应关闭连接，不能从坏帧执行部分内容。"""


@dataclass(frozen=True)
class FrameHeader:
    """解码后的 32B 帧头字段。"""

    msg_type: int
    flags: int
    payload_len: int
    record_count: int
    session_id: int
    seq: int
    ack_seq: int
    request_id: int
    crc32: int

    @property
    def reliable(self) -> bool:
        """是否可靠帧（flags bit0）。"""
        return bool(self.flags & FLAG_RELIABLE)

    @property
    def type_name(self) -> str:
        return MESSAGE_TYPE_NAMES.get(self.msg_type, f"0x{self.msg_type:02X}")


# ---------------------------------------------------------------------------
# CRC32
# ---------------------------------------------------------------------------

def compute_crc32(header_first28: bytes, payload: bytes) -> int:
    """计算帧 CRC32：头字节 0~27 起算，再继续 payload，截断 u32。

    ``header_first28`` 必须恰好是帧头前 28 字节（不含 CRC 字段）。
    """
    if len(header_first28) != HEADER_SIZE - 4:
        raise ValueError(f"CRC 输入头长度必须为 28B，实际 {len(header_first28)}B")
    crc = zlib.crc32(header_first28)
    crc = zlib.crc32(payload, crc)
    return crc & 0xFFFFFFFF


# ---------------------------------------------------------------------------
# 帧编码
# ---------------------------------------------------------------------------

def default_flags(msg_type: int) -> int:
    """按协议规则给出默认 flags：除 HELLO/HELLO_ACK/IDLE/TELEMETRY 外置 RELIABLE。"""
    return 0 if msg_type in _UNRELIABLE_TYPES else FLAG_RELIABLE


def build_frame(
    msg_type: int,
    payload: bytes = b"",
    session_id: int = 0,
    seq: int = 0,
    ack_seq: int = 0,
    request_id: int = 0,
    record_count: int = 0,
    flags: int | None = None,
) -> bytes:
    """构造一帧完整字节（32B 头 + 负载），自动计算 CRC32。

    flags 缺省时按消息类型自动设置 RELIABLE。
    """
    if msg_type not in MESSAGE_TYPE_NAMES:
        raise ProtocolError(f"非法消息类型 0x{msg_type:02X}")
    if not (0 <= len(payload) <= MAX_PAYLOAD_SIZE):
        raise ProtocolError(f"负载长度 {len(payload)} 超出 0~{MAX_PAYLOAD_SIZE} 范围")
    if flags is None:
        flags = default_flags(msg_type)
    if flags & ~FLAG_RELIABLE:
        raise ProtocolError(f"flags 只允许 bit0(RELIABLE)，实际 0x{flags:04X}")
    header28 = struct.pack(
        "<4sBBHHHIIII",
        MAGIC,
        PROTOCOL_VERSION,
        msg_type,
        flags,
        len(payload),
        record_count,
        session_id & 0xFFFFFFFF,
        seq & 0xFFFFFFFF,
        ack_seq & 0xFFFFFFFF,
        request_id & 0xFFFFFFFF,
    )
    crc = compute_crc32(header28, payload)
    return header28 + struct.pack("<I", crc) + payload


# ---------------------------------------------------------------------------
# 帧解码与校验
# ---------------------------------------------------------------------------

def _validate_header_fields(header: FrameHeader) -> None:
    """对帧头字段做语义校验（长度与 CRC 之外的检查）。"""
    if header.msg_type not in MESSAGE_TYPE_NAMES:
        raise ProtocolError(f"非法消息类型 0x{header.msg_type:02X}")
    if header.flags & ~FLAG_RELIABLE:
        raise ProtocolError(f"flags 含保留位：0x{header.flags:04X}")
    if header.msg_type == MSG_TELEMETRY:
        # TELEMETRY：record_count 1~10，且 payload_len == 24 + record_count*96
        if not (TELEMETRY_MIN_RECORDS <= header.record_count <= TELEMETRY_MAX_RECORDS):
            raise ProtocolError(
                f"TELEMETRY record_count 必须为 1~10，实际 {header.record_count}"
            )
        expected = TELEMETRY_HEADER_SIZE + header.record_count * TELEMETRY_RECORD_SIZE
        if header.payload_len != expected:
            raise ProtocolError(
                f"TELEMETRY payload_len 应为 {expected}（24+{header.record_count}*96），"
                f"实际 {header.payload_len}"
            )
    else:
        if header.record_count != 0:
            raise ProtocolError(
                f"非 TELEMETRY 帧 record_count 必须为 0，实际 {header.record_count}"
            )


def parse_header(header_bytes: bytes) -> FrameHeader:
    """解析 32B 帧头并做字段级校验（不含 CRC、不依赖负载）。

    仅检查 magic、version、payload_len 上限、flags、type 与 record_count 语义；
    供增量解析器在收齐负载前预判本帧需要的总长度。
    """
    if len(header_bytes) != HEADER_SIZE:
        raise ProtocolError(f"帧头长度必须为 32B，实际 {len(header_bytes)}B")
    (magic, version, msg_type, flags, payload_len, record_count,
     session_id, seq, ack_seq, request_id, crc32) = struct.unpack(
        HEADER_FORMAT, header_bytes
    )
    if magic != MAGIC:
        raise ProtocolError(f"魔数错误：{magic!r}")
    if version != PROTOCOL_VERSION:
        raise ProtocolError(f"协议版本错误：{version}，期望 {PROTOCOL_VERSION}")
    if payload_len > MAX_PAYLOAD_SIZE:
        raise ProtocolError(
            f"payload_len {payload_len} 超过上限 {MAX_PAYLOAD_SIZE}"
        )
    header = FrameHeader(
        msg_type=msg_type,
        flags=flags,
        payload_len=payload_len,
        record_count=record_count,
        session_id=session_id,
        seq=seq,
        ack_seq=ack_seq,
        request_id=request_id,
        crc32=crc32,
    )
    _validate_header_fields(header)
    return header


def decode_frame(data: bytes) -> tuple[FrameHeader, bytes]:
    """校验并解码一整帧（UDP 数据报或 TCP 收齐的完整帧）。

    校验：magic、version、payload_len 上限、payload_len 与实际字节一致、
    CRC32、type 合法、flags 保留位、record_count 与负载结构关系。
    返回 (帧头, 负载)。任何一项不通过抛出 ProtocolError。
    """
    if len(data) < HEADER_SIZE:
        raise ProtocolError(f"帧长度 {len(data)} 不足帧头 32B")
    header = parse_header(bytes(data[:HEADER_SIZE]))
    if len(data) != HEADER_SIZE + header.payload_len:
        raise ProtocolError(
            f"帧总长度 {len(data)} 与 32+payload_len={HEADER_SIZE + header.payload_len} 不一致"
        )
    payload = bytes(data[HEADER_SIZE:])
    actual_crc = compute_crc32(bytes(data[: HEADER_SIZE - 4]), payload)
    if actual_crc != header.crc32:
        raise ProtocolError(
            f"CRC32 不匹配：帧内 0x{header.crc32:08X}，实算 0x{actual_crc:08X}"
        )
    return header, payload


# ---------------------------------------------------------------------------
# JSON 负载编解码
# ---------------------------------------------------------------------------

def _reject_constant(value: str):
    """json parse_constant 回调：拒绝 NaN/Infinity/-Infinity 等非有限数。"""
    raise ProtocolError(f"JSON 含非有限数：{value}")


def _no_duplicate_keys(pairs: list) -> dict:
    """json object_pairs_hook 回调：检测并拒绝重复键。"""
    obj = {}
    for key, value in pairs:
        if key in obj:
            raise ProtocolError(f"JSON 含重复键：{key!r}")
        obj[key] = value
    return obj


def encode_json_payload(obj) -> bytes:
    """把对象编码为 JSON 负载：UTF-8、紧凑分隔符、不带结尾 NUL、拒绝非有限数。"""
    try:
        text = json.dumps(
            obj, ensure_ascii=False, allow_nan=False, separators=(",", ":")
        )
    except (TypeError, ValueError) as exc:
        raise ProtocolError(f"JSON 编码失败：{exc}") from exc
    payload = text.encode("utf-8")
    if len(payload) > MAX_PAYLOAD_SIZE:
        raise ProtocolError(
            f"JSON 负载 {len(payload)}B 超过上限 {MAX_PAYLOAD_SIZE}B"
        )
    return payload


def decode_json_payload(payload: bytes) -> object:
    """解码 JSON 负载：拒绝超长、非法 UTF-8、错误 JSON、重复键、非有限数。

    不允许多余的结尾 NUL 或截断执行。
    """
    if len(payload) > MAX_PAYLOAD_SIZE:
        raise ProtocolError(
            f"JSON 负载 {len(payload)}B 超过上限 {MAX_PAYLOAD_SIZE}B"
        )
    try:
        text = payload.decode("utf-8", errors="strict")
    except UnicodeDecodeError as exc:
        raise ProtocolError(f"JSON 负载不是合法 UTF-8：{exc}") from exc
    try:
        return json.loads(
            text,
            object_pairs_hook=_no_duplicate_keys,
            parse_constant=_reject_constant,
        )
    except json.JSONDecodeError as exc:
        raise ProtocolError(f"JSON 解析失败：{exc}") from exc


# ---------------------------------------------------------------------------
# TCP 增量帧解析器（处理拆包、粘包）
# ---------------------------------------------------------------------------

class FrameParser:
    """TCP 字节流增量解析器。

    用 :meth:`feed` 喂入任意长度的字节流（可拆包、可粘包），
    返回本次解析出的完整帧列表 ``[(FrameHeader, payload), ...]``。
    遇到非法帧立即抛出 ProtocolError（按协议，非法 TCP 帧应关闭连接），
    解析器内部缓冲随之失效，调用方不应继续使用。
    """

    def __init__(self) -> None:
        self._buf = bytearray()

    @property
    def buffered_size(self) -> int:
        """当前缓冲中尚未成帧的字节数。"""
        return len(self._buf)

    def feed(self, data: bytes) -> list[tuple[FrameHeader, bytes]]:
        frames: list[tuple[FrameHeader, bytes]] = []
        self._buf.extend(data)
        while True:
            if len(self._buf) < HEADER_SIZE:
                break
            # 头校验失败（魔数/版本/长度/type 等）直接抛错，不做重同步
            header = parse_header(bytes(self._buf[:HEADER_SIZE]))
            frame_size = HEADER_SIZE + header.payload_len
            if len(self._buf) < frame_size:
                break  # 负载未收齐，等待更多字节
            raw = bytes(self._buf[:frame_size])
            del self._buf[:frame_size]
            frames.append(decode_frame(raw))  # 含 CRC 校验
        return frames
