"""SCB/1 数据层：96B 日志记录、256B 日志头、TELEMETRY 负载、灰度归一化、57 列 CSV。

布局与 SmartCar 固件 datalog v2（SmartCar/App/datalog.h、datalog.c、
gray_sensor.c）逐字节对应；只读固件头文件获取布局，不修改固件。
所有多字节整数小端，浮点 IEEE754 binary32。
"""

from __future__ import annotations

import struct
from dataclasses import dataclass, field


class RecordError(ValueError):
    """记录/日志头数据校验失败。"""


# ---------------------------------------------------------------------------
# 96B 日志记录（datalog v2 固定记录）
# ---------------------------------------------------------------------------

RECORD_FORMAT = "<I4h2h2h3h2H2B16H2I2hH4B14B"
RECORD_SIZE = struct.calcsize(RECORD_FORMAT)
assert RECORD_SIZE == 96, f"记录长度必须为 96B，实际 {RECORD_SIZE}B"

# 记录内各字段偏移（与固件 datalog.h 一致，供测试与文档核对）
OFFSET_TICK = 0            # u32 tick，1ms
OFFSET_ADC = 4             # i16×4 adc（横左/竖左/竖右/横右）
OFFSET_SPEED = 12          # i16 speed_l、speed_r（脉冲/1ms）
OFFSET_TRACK = 16          # i16 track_error、track_out
OFFSET_GYRO = 20           # i16×3 gyro_x/y/z（度/秒）
OFFSET_DL1B = 26           # u16 dl1b_mm（8192 无效）、u16 battery
OFFSET_KERNEL_STATE = 30   # u8 kernel_state、roundabout_state
OFFSET_GRAY_RAW = 32       # u16×16 gray_raw
OFFSET_GRAY_SEQ = 64       # u32 gray_sequence、control_sequence
OFFSET_EM_ERROR = 72       # i16 em_error、optical_error
OFFSET_PROTECTION = 76     # u16 protection_flags
OFFSET_MODE = 78           # u8 mode、optical_state、gray_valid、stop_reason
OFFSET_RESERVED = 82       # 14B reserved，保持 0

RESERVED_SIZE = 14
DL1B_INVALID_MM = 8192     # dl1b_mm 无效哨兵值


@dataclass(frozen=True)
class LogRecord:
    """96B 日志记录。"""

    tick: int                          # u32，1ms
    adc: tuple[int, int, int, int]     # i16×4
    speed_l: int                       # i16，脉冲/1ms
    speed_r: int                       # i16
    track_error: int                   # i16
    track_out: int                     # i16
    gyro_x: int                        # i16，度/秒
    gyro_y: int                        # i16
    gyro_z: int                        # i16
    dl1b_mm: int                       # u16，8192 无效
    battery: int                       # u16
    kernel_state: int                  # u8
    roundabout_state: int              # u8
    gray_raw: tuple[int, ...]          # u16×16
    gray_sequence: int                 # u32
    control_sequence: int              # u32
    em_error: int                      # i16
    optical_error: int                 # i16
    protection_flags: int              # u16
    mode: int                          # u8
    optical_state: int                 # u8
    gray_valid: int                    # u8
    stop_reason: int                   # u8
    reserved: bytes = b"\x00" * RESERVED_SIZE  # 14B，保持 0

    def __post_init__(self):
        if len(self.adc) != 4:
            raise RecordError(f"adc 必须为 4 通道，实际 {len(self.adc)}")
        if len(self.gray_raw) != 16:
            raise RecordError(f"gray_raw 必须为 16 通道，实际 {len(self.gray_raw)}")
        if len(self.reserved) != RESERVED_SIZE:
            raise RecordError(f"reserved 必须为 {RESERVED_SIZE}B")


def pack_record(record: LogRecord) -> bytes:
    """把日志记录打包为 96B 字节串。"""
    data = struct.pack(
        RECORD_FORMAT,
        record.tick,
        *record.adc,
        record.speed_l,
        record.speed_r,
        record.track_error,
        record.track_out,
        record.gyro_x,
        record.gyro_y,
        record.gyro_z,
        record.dl1b_mm,
        record.battery,
        record.kernel_state,
        record.roundabout_state,
        *record.gray_raw,
        record.gray_sequence,
        record.control_sequence,
        record.em_error,
        record.optical_error,
        record.protection_flags,
        record.mode,
        record.optical_state,
        record.gray_valid,
        record.stop_reason,
        *record.reserved,
    )
    assert len(data) == RECORD_SIZE
    return data


def unpack_record(data: bytes) -> LogRecord:
    """从 96B 字节串解出日志记录；长度不符即拒绝，不解析半条记录。"""
    if len(data) != RECORD_SIZE:
        raise RecordError(f"记录长度必须为 {RECORD_SIZE}B，实际 {len(data)}B")
    fields = struct.unpack(RECORD_FORMAT, data)
    return LogRecord(
        tick=fields[0],
        adc=tuple(fields[1:5]),
        speed_l=fields[5],
        speed_r=fields[6],
        track_error=fields[7],
        track_out=fields[8],
        gyro_x=fields[9],
        gyro_y=fields[10],
        gyro_z=fields[11],
        dl1b_mm=fields[12],
        battery=fields[13],
        kernel_state=fields[14],
        roundabout_state=fields[15],
        gray_raw=tuple(fields[16:32]),
        gray_sequence=fields[32],
        control_sequence=fields[33],
        em_error=fields[34],
        optical_error=fields[35],
        protection_flags=fields[36],
        mode=fields[37],
        optical_state=fields[38],
        gray_valid=fields[39],
        stop_reason=fields[40],
        reserved=bytes(fields[41:55]),
    )


# ---------------------------------------------------------------------------
# TELEMETRY 负载：24B 元数据头 + 1~10 条 96B 记录
# ---------------------------------------------------------------------------

TELEMETRY_HEADER_FORMAT = "<6I"
TELEMETRY_HEADER_SIZE = struct.calcsize(TELEMETRY_HEADER_FORMAT)
assert TELEMETRY_HEADER_SIZE == 24


@dataclass(frozen=True)
class TelemetryPacket:
    """TELEMETRY 负载解码结果。"""

    first_index: int            # u32 首条记录索引
    revision: int               # u32 参数版本
    stm32_dropped_records: int  # u32 STM32 累计上传丢弃记录数
    esp32_dropped_packets: int  # u32 ESP32 累计上传丢弃包数
    first_tick: int             # u32 首条记录 tick
    run_id: int                 # u32 当前运行编号
    records: tuple[LogRecord, ...]


def pack_telemetry(
    first_index: int,
    revision: int,
    stm32_dropped_records: int,
    esp32_dropped_packets: int,
    first_tick: int,
    run_id: int,
    records: list[LogRecord] | tuple[LogRecord, ...],
) -> bytes:
    """打包 TELEMETRY 负载；记录数必须为 1~10。"""
    if not (1 <= len(records) <= 10):
        raise RecordError(f"TELEMETRY 记录数必须为 1~10，实际 {len(records)}")
    header = struct.pack(
        TELEMETRY_HEADER_FORMAT,
        first_index,
        revision,
        stm32_dropped_records,
        esp32_dropped_packets,
        first_tick,
        run_id,
    )
    return header + b"".join(pack_record(r) for r in records)


def unpack_telemetry(payload: bytes) -> TelemetryPacket:
    """解码 TELEMETRY 负载，校验 24 + n*96 长度关系及记录数范围。"""
    if len(payload) < TELEMETRY_HEADER_SIZE:
        raise RecordError(f"TELEMETRY 负载不足 {TELEMETRY_HEADER_SIZE}B 头")
    body = len(payload) - TELEMETRY_HEADER_SIZE
    if body % RECORD_SIZE != 0:
        raise RecordError(
            f"TELEMETRY 负载长度 {len(payload)} 不满足 24+n*96"
        )
    count = body // RECORD_SIZE
    if not (1 <= count <= 10):
        raise RecordError(f"TELEMETRY 记录数必须为 1~10，实际 {count}")
    values = struct.unpack_from(TELEMETRY_HEADER_FORMAT, payload, 0)
    records = tuple(
        unpack_record(payload[TELEMETRY_HEADER_SIZE + i * RECORD_SIZE:
                              TELEMETRY_HEADER_SIZE + (i + 1) * RECORD_SIZE])
        for i in range(count)
    )
    return TelemetryPacket(
        first_index=values[0],
        revision=values[1],
        stm32_dropped_records=values[2],
        esp32_dropped_packets=values[3],
        first_tick=values[4],
        run_id=values[5],
        records=records,
    )


# ---------------------------------------------------------------------------
# 256B 日志头（datalog v2 会话头）
# ---------------------------------------------------------------------------

LOG_HEADER_SIZE = 256
LOG_MAGIC = 0x324C4353       # "SCL2" 小端
LOG_VERSION = 2
LOG_RECORD_SIZE = 96
LOG_SAMPLE_PERIOD_US = 1000
LOG_CONTROL_PERIOD_US = 2000
LOG_CHANNEL_COUNT = 16
LOG_NOISE = 50
LOG_LOST = 200
LOG_MIN_CONTRAST = 200
LOG_CAPACITY = 87336
LOG_DATALOG_DIV = 1

# CRC16-CCITT：初值 0xFFFF、多项式 0x1021，覆盖字节 8~255 共 248B
LOG_CRC16_OFFSET = 6
LOG_CRC16_RANGE_START = 8

LOG_HEADER_FORMAT = "<I6H2B5H16h16h16bfffhHfffhHffhh5I84x"
assert struct.calcsize(LOG_HEADER_FORMAT) == LOG_HEADER_SIZE


def crc16_ccitt(data: bytes, init: int = 0xFFFF) -> int:
    """CRC16-CCITT（初值 0xFFFF、多项式 0x1021，不反射、不异或输出）。"""
    crc = init
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


@dataclass(frozen=True)
class LogHeader:
    """256B 日志头字段。black/white/weights 均为 16 通道。"""

    magic: int = LOG_MAGIC
    version: int = LOG_VERSION
    crc16: int = 0
    header_size: int = LOG_HEADER_SIZE
    record_size: int = LOG_RECORD_SIZE
    sample_period_us: int = LOG_SAMPLE_PERIOD_US
    control_period_us: int = LOG_CONTROL_PERIOD_US
    mode: int = 0                       # u8，0 电磁 / 1 光电
    gear: int = 0                       # u8，0 低档 / 1 高档
    channel_count: int = LOG_CHANNEL_COUNT
    noise: int = LOG_NOISE
    lost: int = LOG_LOST
    min_contrast: int = LOG_MIN_CONTRAST
    reserved0: int = 0
    black: tuple[int, ...] = field(default_factory=lambda: (0,) * 16)   # i16×16
    white: tuple[int, ...] = field(default_factory=lambda: (0,) * 16)   # i16×16
    weights: tuple[int, ...] = field(
        default_factory=lambda: tuple(15 - 2 * i for i in range(16)))   # i8×16
    em_kp: float = 0.0
    em_k2p: float = 0.0
    em_kd: float = 0.0
    em_base_speed: int = 0              # i16
    em_reserved: int = 0                # u16
    opt_kp: float = 0.0
    opt_k2p: float = 0.0
    opt_kd: float = 0.0
    opt_base_speed: int = 0             # i16
    opt_reserved: int = 0               # u16
    kp_v: float = 0.0
    ki_v: float = 0.0
    fan_run: int = 0                    # i16
    fan_idle: int = 0                   # i16
    gray_budget_us: int = 0             # u32
    control_budget_us: int = 0          # u32
    run_delay_count: int = 0            # u32
    capacity: int = LOG_CAPACITY        # u32
    datalog_div: int = LOG_DATALOG_DIV  # u32

    def __post_init__(self):
        if len(self.black) != 16 or len(self.white) != 16 or len(self.weights) != 16:
            raise RecordError("black/white/weights 必须为 16 通道")


def pack_log_header(header: LogHeader, fix_crc: bool = True) -> bytes:
    """打包 256B 日志头。

    fix_crc=True 时忽略 header.crc16，按字节 8~255 重新计算并填入；
    传 False 可原样打包（用于构造 CRC 错误的测试数据）。
    """
    crc_value = header.crc16
    data = struct.pack(
        LOG_HEADER_FORMAT,
        header.magic,
        header.version,
        crc_value,
        header.header_size,
        header.record_size,
        header.sample_period_us,
        header.control_period_us,
        header.mode,
        header.gear,
        header.channel_count,
        header.noise,
        header.lost,
        header.min_contrast,
        header.reserved0,
        *header.black,
        *header.white,
        *header.weights,
        header.em_kp,
        header.em_k2p,
        header.em_kd,
        header.em_base_speed,
        header.em_reserved,
        header.opt_kp,
        header.opt_k2p,
        header.opt_kd,
        header.opt_base_speed,
        header.opt_reserved,
        header.kp_v,
        header.ki_v,
        header.fan_run,
        header.fan_idle,
        header.gray_budget_us,
        header.control_budget_us,
        header.run_delay_count,
        header.capacity,
        header.datalog_div,
    )
    assert len(data) == LOG_HEADER_SIZE
    if fix_crc:
        crc_value = crc16_ccitt(data[LOG_CRC16_RANGE_START:])
        data = data[:LOG_CRC16_OFFSET] + struct.pack("<H", crc_value) + data[8:]
    return data


def unpack_log_header(data: bytes) -> LogHeader:
    """解包 256B 日志头（只解析，不做有效性校验；校验用 validate_log_header）。"""
    if len(data) != LOG_HEADER_SIZE:
        raise RecordError(f"日志头长度必须为 {LOG_HEADER_SIZE}B，实际 {len(data)}B")
    f = struct.unpack(LOG_HEADER_FORMAT, data)
    return LogHeader(
        magic=f[0],
        version=f[1],
        crc16=f[2],
        header_size=f[3],
        record_size=f[4],
        sample_period_us=f[5],
        control_period_us=f[6],
        mode=f[7],
        gear=f[8],
        channel_count=f[9],
        noise=f[10],
        lost=f[11],
        min_contrast=f[12],
        reserved0=f[13],
        black=tuple(f[14:30]),
        white=tuple(f[30:46]),
        weights=tuple(f[46:62]),
        em_kp=f[62],
        em_k2p=f[63],
        em_kd=f[64],
        em_base_speed=f[65],
        em_reserved=f[66],
        opt_kp=f[67],
        opt_k2p=f[68],
        opt_kd=f[69],
        opt_base_speed=f[70],
        opt_reserved=f[71],
        kp_v=f[72],
        ki_v=f[73],
        fan_run=f[74],
        fan_idle=f[75],
        gray_budget_us=f[76],
        control_budget_us=f[77],
        run_delay_count=f[78],
        capacity=f[79],
        datalog_div=f[80],
    )


def validate_log_header(data: bytes) -> LogHeader:
    """复现 Datalog_IsHeaderValid 的全部条件，全部通过返回 LogHeader，否则抛 RecordError。

    检查：长度、magic、version、各固定字段（header_size/record_size/采样与控制周期/
    通道数/noise/lost/min_contrast）、mode<2、gear<2、capacity==87336、
    datalog_div==1、CRC16 匹配、标定有效（逐通道 black/white 越界或 |delta|<200 置掩码位，
    掩码非 0 即无效）。
    """
    header = unpack_log_header(data)  # 长度检查在解包内完成
    if header.magic != LOG_MAGIC:
        raise RecordError(f"日志头 magic 错误：0x{header.magic:08X}")
    if header.version != LOG_VERSION:
        raise RecordError(f"日志头 version 错误：{header.version}")
    fixed = (
        ("header_size", header.header_size, LOG_HEADER_SIZE),
        ("record_size", header.record_size, LOG_RECORD_SIZE),
        ("sample_period_us", header.sample_period_us, LOG_SAMPLE_PERIOD_US),
        ("control_period_us", header.control_period_us, LOG_CONTROL_PERIOD_US),
        ("channel_count", header.channel_count, LOG_CHANNEL_COUNT),
        ("noise", header.noise, LOG_NOISE),
        ("lost", header.lost, LOG_LOST),
        ("min_contrast", header.min_contrast, LOG_MIN_CONTRAST),
        ("capacity", header.capacity, LOG_CAPACITY),
        ("datalog_div", header.datalog_div, LOG_DATALOG_DIV),
    )
    for name, actual, expected in fixed:
        if actual != expected:
            raise RecordError(f"日志头固定字段 {name}={actual}，期望 {expected}")
    if not (0 <= header.mode < 2):
        raise RecordError(f"日志头 mode={header.mode} 越界（必须 <2）")
    if not (0 <= header.gear < 2):
        raise RecordError(f"日志头 gear={header.gear} 越界（必须 <2）")
    actual_crc = crc16_ccitt(data[LOG_CRC16_RANGE_START:])
    if actual_crc != header.crc16:
        raise RecordError(
            f"日志头 CRC16 不匹配：头内 0x{header.crc16:04X}，实算 0x{actual_crc:04X}"
        )
    mask = gray_calibration_mask(header.black, header.white)
    if mask != 0:
        raise RecordError(f"日志头标定无效，通道掩码 0x{mask:04X}")
    return header


# ---------------------------------------------------------------------------
# 灰度归一化（严格复现 Gray_Normalize / Gray_ValidateCalibration 的 C 语义）
# ---------------------------------------------------------------------------

GRAY_MIN_VALID_DELTA = 200   # |white-black| 小于该值判定标定无效
GRAY_Q_MIN = 50              # q < 50 输出 0
GRAY_Q_MAX = 1000            # q > 1000 输出 1000


def _trunc_div(numerator: int, denominator: int) -> int:
    """C 整数除法语义：向零截断（Python // 是向下取整，必须另行处理符号）。"""
    quotient = abs(numerator) // abs(denominator)
    if (numerator < 0) != (denominator < 0):
        return -quotient
    return quotient


def gray_normalize(raw: int, black: int, white: int) -> int:
    """单通道灰度归一化，复现 Gray_Normalize：

    delta = white - black；black/white 不在 [0,4095] 或 -200 < delta < 200 时返回 0；
    否则 q = trunc(1000*(raw-black)/delta)（向零截断）；
    q < 50 输出 0，q > 1000 输出 1000，其余输出 q。
    """
    delta = white - black
    if not (0 <= black <= 4095) or not (0 <= white <= 4095):
        return 0
    if -GRAY_MIN_VALID_DELTA < delta < GRAY_MIN_VALID_DELTA:
        return 0
    q = _trunc_div(1000 * (raw - black), delta)
    if q < GRAY_Q_MIN:
        return 0
    if q > GRAY_Q_MAX:
        return 1000
    return q


def gray_calibration_mask(black: tuple[int, ...], white: tuple[int, ...]) -> int:
    """复现 Gray_ValidateCalibration：逐通道 black/white 越界 [0,4095]
    或 |delta|<200 时置对应掩码位；返回 u16 掩码，非 0 即标定无效。
    """
    if len(black) != 16 or len(white) != 16:
        raise RecordError("标定必须为 16 通道")
    mask = 0
    for i in range(16):
        b, w = black[i], white[i]
        delta = w - b
        if not (0 <= b <= 4095) or not (0 <= w <= 4095) or abs(delta) < GRAY_MIN_VALID_DELTA:
            mask |= 1 << i
    return mask


# ---------------------------------------------------------------------------
# 57 列 CSV（严格对应固件 Datalog_Dump）
# ---------------------------------------------------------------------------

CSV_COLUMNS = (
    ["tick"]
    + [f"adc{i}" for i in range(4)]
    + ["spdL", "spdR", "err", "tout"]
    + ["gx", "gy", "gz"]
    + ["dl1b", "bat", "kstate", "rstate"]
    + [f"gray{i}" for i in range(16)]
    + [f"q{i}" for i in range(16)]
    + ["em_err", "opt_err", "mode", "opt_state", "gray_valid",
       "protect", "reason", "gray_seq", "control_seq"]
)
CSV_COLUMN_COUNT = len(CSV_COLUMNS)
assert CSV_COLUMN_COUNT == 57, f"CSV 必须为 57 列，实际 {CSV_COLUMN_COUNT}"


def csv_header_line() -> str:
    """57 列 CSV 列头行。"""
    return ",".join(CSV_COLUMNS)


def csv_preamble_lines(header: LogHeader) -> list[str]:
    """导出 CSV 的前导行（实时 CSV 没有日志头时缺省并记录事件）。"""
    return [
        f"#log_version={header.version},record_size={header.record_size},"
        f"mode={header.mode},gear={header.gear}",
        "#black," + ",".join(str(v) for v in header.black),
        "#white," + ",".join(str(v) for v in header.white),
    ]


def record_to_csv_row(
    record: LogRecord,
    black: tuple[int, ...] | None = None,
    white: tuple[int, ...] | None = None,
) -> list[str]:
    """把 96B 记录转成 57 列 CSV 行（字符串列表，列序与 CSV_COLUMNS 一致）。

    提供 16 通道 black/white 标定时按 Gray_Normalize 计算 q0~q15；
    没有有效头时 q 字段留空（调用方应保留原始数据并记录缺元数据事件）。
    """
    if (black is None) != (white is None):
        raise RecordError("black 与 white 必须同时提供或同时缺省")
    if black is not None and (len(black) != 16 or len(white) != 16):
        raise RecordError("标定必须为 16 通道")
    if black is not None:
        q_values: list[str] = [
            str(gray_normalize(record.gray_raw[i], black[i], white[i]))
            for i in range(16)
        ]
    else:
        q_values = [""] * 16
    row = (
        [str(record.tick)]
        + [str(v) for v in record.adc]
        + [str(record.speed_l), str(record.speed_r),
           str(record.track_error), str(record.track_out)]
        + [str(record.gyro_x), str(record.gyro_y), str(record.gyro_z)]
        + [str(record.dl1b_mm), str(record.battery),
           str(record.kernel_state), str(record.roundabout_state)]
        + [str(v) for v in record.gray_raw]
        + q_values
        + [str(record.em_error), str(record.optical_error),
           str(record.mode), str(record.optical_state), str(record.gray_valid),
           str(record.protection_flags), str(record.stop_reason),
           str(record.gray_sequence), str(record.control_sequence)]
    )
    assert len(row) == CSV_COLUMN_COUNT
    return row


# ---------------------------------------------------------------------------
# LOG_DATA 负载：u32 offset、u16 length、u16 reserved=0、length 字节数据
# ---------------------------------------------------------------------------

LOG_DATA_HEADER_FORMAT = "<IHH"
LOG_DATA_HEADER_SIZE = struct.calcsize(LOG_DATA_HEADER_FORMAT)
assert LOG_DATA_HEADER_SIZE == 8
LOG_DATA_MAX_LENGTH = 960    # LOG_READ 的 length 上限


@dataclass(frozen=True)
class LogDataBlock:
    """LOG_DATA 负载解码结果。"""

    offset: int   # u32
    data: bytes   # length 字节原始数据


def pack_log_data(offset: int, data: bytes, reserved: int = 0) -> bytes:
    """打包 LOG_DATA 负载。"""
    if not (0 <= len(data) <= 0xFFFF):
        raise RecordError(f"LOG_DATA 数据长度 {len(data)} 超出 u16 范围")
    if not (0 <= reserved <= 0xFFFF):
        raise RecordError(f"LOG_DATA reserved {reserved} 超出 u16 范围")
    return struct.pack(LOG_DATA_HEADER_FORMAT, offset, len(data), reserved) + bytes(data)


def unpack_log_data(payload: bytes) -> LogDataBlock:
    """解码 LOG_DATA 负载：校验长度关系与 reserved==0。"""
    if len(payload) < LOG_DATA_HEADER_SIZE:
        raise RecordError(f"LOG_DATA 负载不足 {LOG_DATA_HEADER_SIZE}B 头")
    offset, length, reserved = struct.unpack_from(LOG_DATA_HEADER_FORMAT, payload, 0)
    if reserved != 0:
        raise RecordError(f"LOG_DATA reserved 必须为 0，实际 {reserved}")
    if len(payload) != LOG_DATA_HEADER_SIZE + length:
        raise RecordError(
            f"LOG_DATA 负载长度 {len(payload)} 与 8+length={LOG_DATA_HEADER_SIZE + length} 不一致"
        )
    return LogDataBlock(offset=offset, data=bytes(payload[LOG_DATA_HEADER_SIZE:]))
