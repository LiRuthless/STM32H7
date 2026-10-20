"""scb_records 单元测试：96B 记录、256B 日志头、灰度归一化、57 列 CSV、LOG_DATA。"""

import os
import struct
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import scb_records as records


def make_record() -> records.LogRecord:
    """字段全部取可区分值（含负数与边界）的测试记录。"""
    return records.LogRecord(
        tick=0x01020304,
        adc=(-1, 2000, -3000, 4095),
        speed_l=-100,
        speed_r=200,
        track_error=-32768,
        track_out=32767,
        gyro_x=-90,
        gyro_y=45,
        gyro_z=180,
        dl1b_mm=records.DL1B_INVALID_MM,
        battery=740,
        kernel_state=3,
        roundabout_state=2,
        gray_raw=tuple(range(100, 100 + 16)),
        gray_sequence=0xAABBCCDD,
        control_sequence=0x11223344,
        em_error=-500,
        optical_error=600,
        protection_flags=0x00F0,
        mode=1,
        optical_state=4,
        gray_valid=1,
        stop_reason=9,
    )


def make_valid_header() -> records.LogHeader:
    return records.LogHeader(
        mode=1,
        gear=0,
        black=tuple(100 + i for i in range(16)),
        white=tuple(3000 - i for i in range(16)),
        em_kp=2.0, em_k2p=0.008, em_kd=15.0, em_base_speed=180,
        opt_kp=1.5, opt_k2p=0.004, opt_kd=12.0, opt_base_speed=150,
        kp_v=20.0, ki_v=0.75,
        fan_run=1600, fan_idle=1100,
        gray_budget_us=800, control_budget_us=1500, run_delay_count=100,
    )


class TestRecord96(unittest.TestCase):
    """96B 记录：长度断言与逐字段偏移核对。"""

    def test_record_size_is_96(self):
        self.assertEqual(struct.calcsize(records.RECORD_FORMAT), 96)
        self.assertEqual(records.RECORD_SIZE, 96)

    def test_field_offsets(self):
        raw = records.pack_record(make_record())
        self.assertEqual(len(raw), 96)
        self.assertEqual(struct.unpack_from("<I", raw, 0)[0], 0x01020304)      # tick
        self.assertEqual(struct.unpack_from("<4h", raw, 4), (-1, 2000, -3000, 4095))
        self.assertEqual(struct.unpack_from("<2h", raw, 12), (-100, 200))      # speed
        self.assertEqual(struct.unpack_from("<2h", raw, 16), (-32768, 32767))  # track
        self.assertEqual(struct.unpack_from("<3h", raw, 20), (-90, 45, 180))   # gyro
        self.assertEqual(struct.unpack_from("<2H", raw, 26), (8192, 740))      # dl1b/bat
        self.assertEqual(struct.unpack_from("<2B", raw, 30), (3, 2))           # states
        self.assertEqual(struct.unpack_from("<16H", raw, 32),
                         tuple(range(100, 116)))                               # gray_raw
        self.assertEqual(struct.unpack_from("<2I", raw, 64),
                         (0xAABBCCDD, 0x11223344))                             # seq
        self.assertEqual(struct.unpack_from("<2h", raw, 72), (-500, 600))      # errors
        self.assertEqual(struct.unpack_from("<H", raw, 76)[0], 0x00F0)         # protect
        self.assertEqual(struct.unpack_from("<4B", raw, 78), (1, 4, 1, 9))     # mode 等
        self.assertEqual(raw[82:96], b"\x00" * 14)                             # reserved

    def test_roundtrip(self):
        record = make_record()
        self.assertEqual(records.unpack_record(records.pack_record(record)), record)

    def test_wrong_length_rejected(self):
        with self.assertRaises(records.RecordError):
            records.unpack_record(b"\x00" * 95)
        with self.assertRaises(records.RecordError):
            records.unpack_record(b"\x00" * 97)

    def test_bad_channel_count_rejected(self):
        with self.assertRaises(records.RecordError):
            records.LogRecord(
                tick=0, adc=(0, 0, 0), speed_l=0, speed_r=0, track_error=0,
                track_out=0, gyro_x=0, gyro_y=0, gyro_z=0, dl1b_mm=0,
                battery=0, kernel_state=0, roundabout_state=0,
                gray_raw=(0,) * 16, gray_sequence=0, control_sequence=0,
                em_error=0, optical_error=0, protection_flags=0, mode=0,
                optical_state=0, gray_valid=0, stop_reason=0)


class TestTelemetryPayload(unittest.TestCase):
    """TELEMETRY 负载：24B 头偏移与 24+n*96 长度关系。"""

    def test_header_offsets(self):
        rec = make_record()
        payload = records.pack_telemetry(11, 22, 33, 44, 55, 66, [rec])
        self.assertEqual(len(payload), 24 + 96)
        self.assertEqual(struct.unpack_from("<6I", payload, 0),
                         (11, 22, 33, 44, 55, 66))
        self.assertEqual(payload[24:], records.pack_record(rec))

    def test_roundtrip_10_records(self):
        recs = [make_record() for _ in range(10)]
        packet = records.unpack_telemetry(
            records.pack_telemetry(1, 2, 3, 4, 5, 6, recs))
        self.assertEqual(len(packet.records), 10)
        self.assertEqual(packet.records, tuple(recs))
        self.assertEqual((packet.first_index, packet.revision,
                          packet.stm32_dropped_records, packet.esp32_dropped_packets,
                          packet.first_tick, packet.run_id), (1, 2, 3, 4, 5, 6))

    def test_bad_lengths_rejected(self):
        rec = records.pack_record(make_record())
        with self.assertRaises(records.RecordError):
            records.unpack_telemetry(b"\x00" * 23)          # 不足 24B 头
        with self.assertRaises(records.RecordError):
            records.unpack_telemetry(b"\x00" * 24)          # 0 条记录
        with self.assertRaises(records.RecordError):
            records.unpack_telemetry(b"\x00" * (24 + 95))   # 非 96 整数倍
        with self.assertRaises(records.RecordError):
            records.unpack_telemetry(b"\x00" * (24 + 11 * 96))  # 超过 10 条
        with self.assertRaises(records.RecordError):
            records.pack_telemetry(0, 0, 0, 0, 0, 0, [])    # 打包 0 条


class TestLogHeader256(unittest.TestCase):
    """256B 日志头：偏移、CRC16、固定字段与标定校验。"""

    def test_header_size_and_crc16_vector(self):
        self.assertEqual(struct.calcsize(records.LOG_HEADER_FORMAT), 256)
        # CRC-16/CCITT-FALSE 对 "123456789" 的公开已知值
        self.assertEqual(records.crc16_ccitt(b"123456789"), 0x29B1)

    def test_field_offsets(self):
        header = make_valid_header()
        raw = records.pack_log_header(header)
        self.assertEqual(len(raw), 256)
        self.assertEqual(struct.unpack_from("<I", raw, 0)[0], 0x324C4353)   # magic
        self.assertEqual(struct.unpack_from("<H", raw, 4)[0], 2)            # version
        crc16 = struct.unpack_from("<H", raw, 6)[0]
        self.assertEqual(crc16, records.crc16_ccitt(raw[8:]))               # crc16 覆盖 8~255
        self.assertEqual(struct.unpack_from("<4H", raw, 8),
                         (256, 96, 1000, 2000))
        self.assertEqual(struct.unpack_from("<2B", raw, 16), (1, 0))        # mode/gear
        self.assertEqual(struct.unpack_from("<5H", raw, 18),
                         (16, 50, 200, 200, 0))
        self.assertEqual(struct.unpack_from("<16h", raw, 28),
                         tuple(100 + i for i in range(16)))                 # black
        self.assertEqual(struct.unpack_from("<16h", raw, 60),
                         tuple(3000 - i for i in range(16)))                # white
        self.assertEqual(struct.unpack_from("<16b", raw, 92),
                         tuple(15 - 2 * i for i in range(16)))              # weights
        em = struct.unpack_from("<fffhH", raw, 108)
        self.assertAlmostEqual(em[0], 2.0)
        self.assertAlmostEqual(em[1], 0.008)
        self.assertAlmostEqual(em[2], 15.0)
        self.assertEqual(em[3:], (180, 0))
        opt = struct.unpack_from("<fffhH", raw, 124)
        self.assertAlmostEqual(opt[0], 1.5)
        self.assertEqual(opt[3:], (150, 0))
        vf = struct.unpack_from("<2f", raw, 140)
        self.assertAlmostEqual(vf[0], 20.0)
        self.assertAlmostEqual(vf[1], 0.75)
        self.assertEqual(struct.unpack_from("<2h", raw, 148), (1600, 1100))
        self.assertEqual(struct.unpack_from("<5I", raw, 152),
                         (800, 1500, 100, 87336, 1))
        self.assertEqual(raw[172:], b"\x00" * 84)                           # reserved_tail

    def test_roundtrip(self):
        header = make_valid_header()
        decoded = records.validate_log_header(records.pack_log_header(header))
        self.assertEqual(decoded.mode, 1)
        self.assertEqual(decoded.black, header.black)
        self.assertEqual(decoded.white, header.white)
        self.assertAlmostEqual(decoded.em_kp, 2.0)
        self.assertEqual(decoded.capacity, 87336)

    def _corrupt(self, offset: int, fmt: str, value) -> bytes:
        raw = bytearray(records.pack_log_header(make_valid_header()))
        struct.pack_into(fmt, raw, offset, value)
        # 重新补齐 CRC16，保证失败原因是被改字段本身而非 CRC
        struct.pack_into("<H", raw, 6, records.crc16_ccitt(bytes(raw[8:])))
        return bytes(raw)

    def test_validate_rejects_bad_fields(self):
        cases = [
            (0, "<I", 0x12345678),   # magic
            (4, "<H", 1),            # version
            (8, "<H", 128),          # header_size
            (10, "<H", 95),          # record_size
            (12, "<H", 999),         # sample_period_us
            (14, "<H", 1000),        # control_period_us
            (16, "<B", 2),           # mode >= 2
            (17, "<B", 2),           # gear >= 2
            (18, "<H", 15),          # channel_count
            (20, "<H", 49),          # noise
            (22, "<H", 199),         # lost
            (24, "<H", 199),         # min_contrast
            (164, "<I", 87335),      # capacity
            (168, "<I", 2),          # datalog_div
        ]
        for offset, fmt, value in cases:
            with self.subTest(offset=offset):
                with self.assertRaises(records.RecordError):
                    records.validate_log_header(self._corrupt(offset, fmt, value))

    def test_validate_rejects_bad_crc16(self):
        raw = bytearray(records.pack_log_header(make_valid_header()))
        raw[200] ^= 0xFF   # 改动覆盖区但不修 CRC
        with self.assertRaises(records.RecordError):
            records.validate_log_header(bytes(raw))

    def test_validate_rejects_bad_calibration(self):
        header = make_valid_header()
        bad_white = list(header.white)
        bad_white[5] = header.black[5] + 100   # |delta|=100 < 200
        header = records.LogHeader(**{**header.__dict__, "white": tuple(bad_white)})
        with self.assertRaises(records.RecordError):
            records.validate_log_header(records.pack_log_header(header))

    def test_wrong_length_rejected(self):
        with self.assertRaises(records.RecordError):
            records.validate_log_header(b"\x00" * 255)


class TestGrayNormalize(unittest.TestCase):
    """灰度归一化：delta 正负、向零截断、q<50、q>1000、无效标定。"""

    def test_trunc_div_c_semantics(self):
        # C 整数除法向零截断，Python // 向下取整，负数必须另行处理
        self.assertEqual(records._trunc_div(7, 2), 3)
        self.assertEqual(records._trunc_div(-7, 2), -3)   # Python // 得 -4
        self.assertEqual(records._trunc_div(7, -2), -3)
        self.assertEqual(records._trunc_div(-7, -2), 3)

    def test_normal_positive_delta(self):
        # q = trunc(1000*999/2000) = trunc(499.5) = 499
        self.assertEqual(records.gray_normalize(1999, 1000, 3000), 499)

    def test_negative_delta(self):
        # black=3000 white=1000，delta=-2000；raw=2500 → q=250
        self.assertEqual(records.gray_normalize(2500, 3000, 1000), 250)

    def test_q_below_50_clamped_to_0(self):
        # q = 1000*10/2000 = 5 < 50
        self.assertEqual(records.gray_normalize(1010, 1000, 3000), 0)
        # q = 1000*99/2000 = 49 < 50
        self.assertEqual(records.gray_normalize(1099, 1000, 3000), 0)
        # q = 1000*100/2000 = 50，不小于 50，保留
        self.assertEqual(records.gray_normalize(1100, 1000, 3000), 50)

    def test_q_above_1000_clamped(self):
        # q = 1000*2900/2000 = 1450 > 1000
        self.assertEqual(records.gray_normalize(3900, 1000, 3000), 1000)
        # raw 超 white 也钳到 1000
        self.assertEqual(records.gray_normalize(4095, 1000, 3000), 1000)

    def test_raw_below_black_negative_q(self):
        # q = trunc(1000*(-100)/2000) = -50 < 50 → 0
        self.assertEqual(records.gray_normalize(900, 1000, 3000), 0)

    def test_invalid_calibration_returns_0(self):
        self.assertEqual(records.gray_normalize(2000, -1, 3000), 0)     # black 越界
        self.assertEqual(records.gray_normalize(2000, 1000, 5000), 0)   # white 越界
        self.assertEqual(records.gray_normalize(2000, 1000, 1199), 0)   # delta=199
        self.assertEqual(records.gray_normalize(2000, 1199, 1000), 0)   # delta=-199
        # 边界：delta=±200 有效
        self.assertEqual(records.gray_normalize(4095, 1000, 1200), 1000)
        self.assertEqual(records.gray_normalize(1100, 1000, 1200), 500)

    def test_calibration_mask(self):
        black = tuple([100] * 16)
        white = tuple([3000] * 16)
        self.assertEqual(records.gray_calibration_mask(black, white), 0)
        # 通道 3：delta=199 → 置位；通道 7：black 越界 → 置位
        w = list(white)
        w[3] = 100 + 199
        b = list(black)
        b[7] = 5000
        mask = records.gray_calibration_mask(tuple(b), tuple(w))
        self.assertEqual(mask, (1 << 3) | (1 << 7))


class TestCsv57(unittest.TestCase):
    """57 列 CSV：列数、列头、示例行内容、前导行。"""

    def test_column_count(self):
        self.assertEqual(len(records.CSV_COLUMNS), 57)
        self.assertEqual(records.CSV_COLUMN_COUNT, 57)
        self.assertEqual(len(records.csv_header_line().split(",")), 57)
        self.assertEqual(records.CSV_COLUMNS[0], "tick")
        self.assertEqual(records.CSV_COLUMNS[16], "gray0")
        self.assertEqual(records.CSV_COLUMNS[31], "gray15")
        self.assertEqual(records.CSV_COLUMNS[32], "q0")
        self.assertEqual(records.CSV_COLUMNS[47], "q15")
        self.assertEqual(records.CSV_COLUMNS[56], "control_seq")

    def test_row_content(self):
        record = records.LogRecord(
            tick=123456, adc=(1, 2, 3, 4), speed_l=10, speed_r=-20,
            track_error=-30, track_out=40, gyro_x=5, gyro_y=-6, gyro_z=7,
            dl1b_mm=8192, battery=740, kernel_state=2, roundabout_state=1,
            gray_raw=tuple(range(100, 116)), gray_sequence=111,
            control_sequence=222, em_error=-300, optical_error=400,
            protection_flags=5, mode=1, optical_state=3, gray_valid=1,
            stop_reason=0)
        black = tuple([100] * 16)
        white = tuple([3000] * 16)
        row = records.record_to_csv_row(record, black, white)
        self.assertEqual(len(row), 57)
        self.assertEqual(row[0], "123456")
        self.assertEqual(row[1:5], ["1", "2", "3", "4"])
        self.assertEqual(row[5:7], ["10", "-20"])
        self.assertEqual(row[7:9], ["-30", "40"])
        self.assertEqual(row[9:12], ["5", "-6", "7"])
        self.assertEqual(row[12:16], ["8192", "740", "2", "1"])
        self.assertEqual(row[16:32], [str(v) for v in range(100, 116)])
        # 通道 i：q = 1000*(gray-100)/2900；通道 0 raw=100=black → q=0
        self.assertEqual(row[32], "0")
        # 通道 15：q = trunc(1000*15/2900) = 5 < 50 → 0
        self.assertEqual(row[47], "0")
        self.assertEqual(row[48:57],
                         ["-300", "400", "1", "3", "1", "5", "0", "111", "222"])

    def test_row_without_calibration_leaves_q_empty(self):
        record = make_record()
        row = records.record_to_csv_row(record)
        self.assertEqual(len(row), 57)
        self.assertEqual(row[32:48], [""] * 16)

    def test_preamble_lines(self):
        header = make_valid_header()
        lines = records.csv_preamble_lines(header)
        self.assertEqual(lines[0], "#log_version=2,record_size=96,mode=1,gear=0")
        self.assertEqual(lines[1],
                         "#black," + ",".join(str(100 + i) for i in range(16)))
        self.assertEqual(lines[2],
                         "#white," + ",".join(str(3000 - i) for i in range(16)))


class TestLogData(unittest.TestCase):
    """LOG_DATA 负载编解码。"""

    def test_roundtrip(self):
        data = bytes(range(64))
        payload = records.pack_log_data(256 + 96 * 3, data)
        self.assertEqual(struct.unpack_from("<IHH", payload, 0), (256 + 96 * 3, 64, 0))
        block = records.unpack_log_data(payload)
        self.assertEqual(block.offset, 256 + 96 * 3)
        self.assertEqual(block.data, data)

    def test_max_length_960(self):
        block = records.unpack_log_data(records.pack_log_data(0, b"\xAA" * 960))
        self.assertEqual(len(block.data), 960)

    def test_reserved_nonzero_rejected(self):
        payload = records.pack_log_data(0, b"\x00" * 4, reserved=1)
        with self.assertRaises(records.RecordError):
            records.unpack_log_data(payload)

    def test_length_mismatch_rejected(self):
        payload = records.pack_log_data(0, b"\x00" * 4) + b"\x00"
        with self.assertRaises(records.RecordError):
            records.unpack_log_data(payload)
        with self.assertRaises(records.RecordError):
            records.unpack_log_data(b"\x00" * 7)  # 不足 8B 头


if __name__ == "__main__":
    unittest.main()
