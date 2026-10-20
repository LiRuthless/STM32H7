"""scb_protocol 单元测试：帧头、CRC32、JSON、增量解析、各类非法帧。"""

import os
import struct
import sys
import unittest
import zlib

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import scb_protocol as proto
import scb_vectors as vectors


class TestHeaderBasics(unittest.TestCase):
    """帧头长度与基本编解码。"""

    def test_header_size_is_32(self):
        self.assertEqual(struct.calcsize(proto.HEADER_FORMAT), 32)
        self.assertEqual(proto.HEADER_SIZE, 32)

    def test_crc32_algorithm_anchor(self):
        # 独立锚点：zlib.crc32("123456789") 的公开已知值
        self.assertEqual(zlib.crc32(b"123456789") & 0xFFFFFFFF, 0xCBF43926)

    def test_crc32_independent_vector(self):
        # 硬编码期望值（来自固定 IDLE 向量的 CRC 字段，非现算后回填）
        # IDLE 帧头前 28 字节 + 空负载的 CRC32 必须等于 0x8940ACA1
        header28 = bytes.fromhex(
            "5343423101030000000000000100c3a5090000000400000000000000"
        )
        self.assertEqual(len(header28), 28)
        self.assertEqual(proto.compute_crc32(header28, b""), 0x8940ACA1)

    def test_crc32_input_length_checked(self):
        with self.assertRaises(ValueError):
            proto.compute_crc32(b"\x00" * 27, b"")

    def test_empty_payload_roundtrip(self):
        frame = proto.build_frame(proto.MSG_IDLE, b"", session_id=1, seq=9, ack_seq=4)
        self.assertEqual(len(frame), 32)
        header, payload = proto.decode_frame(frame)
        self.assertEqual(header.msg_type, proto.MSG_IDLE)
        self.assertEqual(header.payload_len, 0)
        self.assertEqual(payload, b"")
        self.assertFalse(header.reliable)

    def test_max_payload_992(self):
        payload = bytes(i & 0xFF for i in range(992))
        frame = proto.build_frame(proto.MSG_EVENT, payload, session_id=2, seq=3)
        self.assertEqual(len(frame), 32 + 992)
        header, decoded = proto.decode_frame(frame)
        self.assertEqual(header.payload_len, 992)
        self.assertEqual(decoded, payload)
        self.assertTrue(header.reliable)  # EVENT 默认 RELIABLE

    def test_payload_over_limit_rejected(self):
        with self.assertRaises(proto.ProtocolError):
            proto.build_frame(proto.MSG_EVENT, b"\x00" * 993)

    def test_default_reliable_flags(self):
        for t in (proto.MSG_COMMAND, proto.MSG_RESULT, proto.MSG_EVENT,
                  proto.MSG_LOG_DATA):
            header, _ = proto.decode_frame(proto.build_frame(t, b""))
            self.assertTrue(header.reliable, f"type 0x{t:02X} 应默认 RELIABLE")
        for t in (proto.MSG_HELLO, proto.MSG_HELLO_ACK, proto.MSG_IDLE,
                  proto.MSG_TELEMETRY):
            if t == proto.MSG_TELEMETRY:
                import scb_records as records
                payload = records.pack_telemetry(
                    0, 1, 0, 0, 0, 1,
                    [records.LogRecord(
                        tick=0, adc=(0, 0, 0, 0), speed_l=0, speed_r=0,
                        track_error=0, track_out=0, gyro_x=0, gyro_y=0,
                        gyro_z=0, dl1b_mm=0, battery=0, kernel_state=0,
                        roundabout_state=0, gray_raw=(0,) * 16,
                        gray_sequence=0, control_sequence=0, em_error=0,
                        optical_error=0, protection_flags=0, mode=0,
                        optical_state=0, gray_valid=0, stop_reason=0)])
                frame = proto.build_frame(t, payload, record_count=1)
            else:
                frame = proto.build_frame(t, b"")
            header, _ = proto.decode_frame(frame)
            self.assertFalse(header.reliable, f"type 0x{t:02X} 不应默认 RELIABLE")

    def test_header_field_roundtrip(self):
        frame = proto.build_frame(
            proto.MSG_COMMAND, b"{}", session_id=0xDEADBEEF, seq=0x12345678,
            ack_seq=7, request_id=42)
        header, payload = proto.decode_frame(frame)
        self.assertEqual(header.session_id, 0xDEADBEEF)
        self.assertEqual(header.seq, 0x12345678)
        self.assertEqual(header.ack_seq, 7)
        self.assertEqual(header.request_id, 42)
        self.assertEqual(payload, b"{}")


class TestDecodeErrors(unittest.TestCase):
    """错长度/版本/CRC/魔数/类型/flags/record_count 一律拒绝。"""

    def test_short_frame(self):
        with self.assertRaises(proto.ProtocolError):
            proto.decode_frame(b"\x00" * 31)

    def test_bad_magic(self):
        frame = bytearray(vectors.from_hex(vectors.IDLE_HEX))
        frame[0:4] = b"XXXX"
        with self.assertRaises(proto.ProtocolError):
            proto.decode_frame(bytes(frame))

    def test_bad_version(self):
        frame = bytearray(vectors.from_hex(vectors.IDLE_HEX))
        frame[4] = 2
        with self.assertRaises(proto.ProtocolError):
            proto.decode_frame(bytes(frame))

    def test_bad_crc(self):
        with self.assertRaises(proto.ProtocolError):
            proto.decode_frame(vectors.from_hex(vectors.BAD_CRC_HEX))

    def test_bad_length(self):
        # 帧头声称 payload_len=100，实际没有负载
        with self.assertRaises(proto.ProtocolError):
            proto.decode_frame(vectors.from_hex(vectors.BAD_LENGTH_HEX))

    def test_trailing_bytes_rejected(self):
        frame = vectors.from_hex(vectors.IDLE_HEX) + b"\x00"
        with self.assertRaises(proto.ProtocolError):
            proto.decode_frame(frame)

    def test_payload_len_over_992(self):
        frame = bytearray(vectors.from_hex(vectors.IDLE_HEX))
        frame[8:10] = struct.pack("<H", 993)
        with self.assertRaises(proto.ProtocolError):
            proto.decode_frame(bytes(frame))

    def test_bad_type(self):
        frame = bytearray(vectors.from_hex(vectors.IDLE_HEX))
        frame[5] = 0x7F
        with self.assertRaises(proto.ProtocolError):
            proto.decode_frame(bytes(frame))

    def test_reserved_flags_rejected(self):
        frame = bytearray(vectors.from_hex(vectors.IDLE_HEX))
        frame[6:8] = struct.pack("<H", 0x0002)
        with self.assertRaises(proto.ProtocolError):
            proto.decode_frame(bytes(frame))

    def test_telemetry_record_count_zero(self):
        frame = bytearray(vectors.from_hex(vectors.TELEMETRY_1_HEX))
        frame[10:12] = struct.pack("<H", 0)
        with self.assertRaises(proto.ProtocolError):
            proto.decode_frame(bytes(frame))

    def test_telemetry_record_count_over_10(self):
        frame = bytearray(vectors.from_hex(vectors.TELEMETRY_1_HEX))
        frame[10:12] = struct.pack("<H", 11)
        with self.assertRaises(proto.ProtocolError):
            proto.decode_frame(bytes(frame))

    def test_telemetry_count_length_mismatch(self):
        # record_count=2 但 payload_len 仍是 24+1*96
        frame = bytearray(vectors.from_hex(vectors.TELEMETRY_1_HEX))
        frame[10:12] = struct.pack("<H", 2)
        with self.assertRaises(proto.ProtocolError):
            proto.decode_frame(bytes(frame))

    def test_non_telemetry_record_count_nonzero(self):
        frame = bytearray(vectors.from_hex(vectors.IDLE_HEX))
        frame[10:12] = struct.pack("<H", 1)
        with self.assertRaises(proto.ProtocolError):
            proto.decode_frame(bytes(frame))


class TestJsonPayload(unittest.TestCase):
    """JSON 负载：合法往返 + 重复键/非有限数/非法编码/错误 JSON 拒绝。"""

    def test_roundtrip(self):
        obj = {"op": "GET_STATUS", "args": {}, "n": 1, "f": 2.5, "中文": "值"}
        payload = proto.encode_json_payload(obj)
        self.assertFalse(payload.endswith(b"\x00"))  # 不带结尾 NUL
        self.assertEqual(proto.decode_json_payload(payload), obj)

    def test_duplicate_keys_rejected(self):
        with self.assertRaises(proto.ProtocolError):
            proto.decode_json_payload(b'{"a":1,"a":2}')

    def test_nested_duplicate_keys_rejected(self):
        with self.assertRaises(proto.ProtocolError):
            proto.decode_json_payload(b'{"args":{"x":1,"x":2}}')

    def test_nan_rejected(self):
        with self.assertRaises(proto.ProtocolError):
            proto.decode_json_payload(b'{"v":NaN}')

    def test_infinity_rejected(self):
        with self.assertRaises(proto.ProtocolError):
            proto.decode_json_payload(b'{"v":Infinity}')
        with self.assertRaises(proto.ProtocolError):
            proto.decode_json_payload(b'{"v":-Infinity}')

    def test_encode_non_finite_rejected(self):
        with self.assertRaises(proto.ProtocolError):
            proto.encode_json_payload({"v": float("nan")})
        with self.assertRaises(proto.ProtocolError):
            proto.encode_json_payload({"v": float("inf")})

    def test_invalid_utf8_rejected(self):
        with self.assertRaises(proto.ProtocolError):
            proto.decode_json_payload(b'{"v":"\xff\xfe"}')

    def test_broken_json_rejected(self):
        with self.assertRaises(proto.ProtocolError):
            proto.decode_json_payload(b'{"op":"GET_STATUS"')  # 截断，不允许截断执行

    def test_trailing_nul_rejected(self):
        with self.assertRaises(proto.ProtocolError):
            proto.decode_json_payload(b'{"op":"GET_STATUS","args":{}}\x00')

    def test_overlong_payload_rejected(self):
        with self.assertRaises(proto.ProtocolError):
            proto.decode_json_payload(b"\x00" * 993)
        with self.assertRaises(proto.ProtocolError):
            proto.encode_json_payload({"pad": "x" * 2000})


class TestFrameParser(unittest.TestCase):
    """TCP 增量解析：拆包、粘包、部分头、非法帧。"""

    def test_byte_by_byte(self):
        data = vectors.from_hex(vectors.GET_STATUS_HEX)
        parser = proto.FrameParser()
        frames = []
        for i in range(len(data)):
            frames.extend(parser.feed(data[i:i + 1]))
        self.assertEqual(len(frames), 1)
        header, payload = frames[0]
        self.assertEqual(header.msg_type, proto.MSG_COMMAND)
        self.assertEqual(proto.decode_json_payload(payload),
                         {"op": "GET_STATUS", "args": {}})

    def test_coalesced_frames(self):
        data = (vectors.from_hex(vectors.IDLE_HEX)
                + vectors.from_hex(vectors.GET_STATUS_HEX)
                + vectors.from_hex(vectors.LOG_DATA_HEX))
        parser = proto.FrameParser()
        frames = parser.feed(data)
        self.assertEqual([h.msg_type for h, _ in frames],
                         [proto.MSG_IDLE, proto.MSG_COMMAND, proto.MSG_LOG_DATA])

    def test_partial_then_rest(self):
        data = vectors.from_hex(vectors.GET_STATUS_HEX)
        parser = proto.FrameParser()
        self.assertEqual(parser.feed(data[:10]), [])   # 不足 32B 头
        self.assertEqual(parser.feed(data[10:40]), [])  # 头齐、负载未齐
        frames = parser.feed(data[40:])
        self.assertEqual(len(frames), 1)

    def test_invalid_frame_raises(self):
        parser = proto.FrameParser()
        with self.assertRaises(proto.ProtocolError):
            parser.feed(b"XXXX" + b"\x00" * 28)

    def test_bad_crc_frame_raises(self):
        parser = proto.FrameParser()
        with self.assertRaises(proto.ProtocolError):
            parser.feed(vectors.from_hex(vectors.BAD_CRC_HEX))


if __name__ == "__main__":
    unittest.main()
