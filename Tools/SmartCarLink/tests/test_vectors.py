"""共同测试向量交叉核对：独立解码固定十六进制字节，逐项断言字段。

不是同一编解码器的自我往返：字节固定，期望字段由独立公式/硬编码给出。
"""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import scb_protocol as proto
import scb_records as records
import scb_vectors as vectors


def decode(hex_text: str):
    return proto.decode_frame(vectors.from_hex(hex_text))


class TestNormalVectors(unittest.TestCase):
    """正常向量：解码固定字节并逐项核对期望字段。"""

    def test_header_fields_all_vectors(self):
        for name, expect in vectors.EXPECTED_HEADERS.items():
            with self.subTest(vector=name):
                header, payload = decode(getattr(vectors, name + "_HEX"))
                for field_name, expected in expect.items():
                    self.assertEqual(getattr(header, field_name), expected,
                                     f"{name}.{field_name}")
                self.assertEqual(header.session_id, vectors.VECTOR_SESSION_ID)
                self.assertEqual(len(payload), header.payload_len)

    def test_idle(self):
        header, payload = decode(vectors.IDLE_HEX)
        self.assertEqual(header.msg_type, proto.MSG_IDLE)
        self.assertEqual(payload, b"")
        self.assertFalse(header.reliable)
        self.assertEqual(header.crc32, 0x8940ACA1)  # 帧内固定 CRC 值

    def test_get_status(self):
        header, payload = decode(vectors.GET_STATUS_HEX)
        self.assertTrue(header.reliable)
        self.assertEqual(header.request_id, 1)
        self.assertEqual(proto.decode_json_payload(payload),
                         {"op": "GET_STATUS", "args": {}})

    def test_set_params_full_values(self):
        header, payload = decode(vectors.SET_PARAMS_HEX)
        self.assertTrue(header.reliable)
        self.assertEqual(header.request_id, 2)
        body = proto.decode_json_payload(payload)
        self.assertEqual(body["op"], "SET_PARAMS")
        self.assertEqual(body["args"]["mode"], 1)
        self.assertEqual(body["args"]["gear"], 0)
        # 完整 8 项 values，逐项核对
        self.assertEqual(body["args"]["values"],
                         vectors.SET_PARAMS_EXPECTED_VALUES)
        self.assertEqual(len(body["args"]["values"]), 8)

    def test_telemetry_1(self):
        header, payload = decode(vectors.TELEMETRY_1_HEX)
        self.assertEqual(header.record_count, 1)
        packet = records.unpack_telemetry(payload)
        for key, expected in vectors.TELEMETRY_1_META.items():
            self.assertEqual(getattr(packet, key), expected, key)
        self.assertEqual(len(packet.records), 1)
        self.assertEqual(packet.records[0],
                         records.LogRecord(**vectors.expected_record_fields(0)))

    def test_telemetry_10(self):
        header, payload = decode(vectors.TELEMETRY_10_HEX)
        self.assertEqual(header.record_count, 10)
        self.assertEqual(header.payload_len, 24 + 10 * 96)
        packet = records.unpack_telemetry(payload)
        for key, expected in vectors.TELEMETRY_10_META.items():
            self.assertEqual(getattr(packet, key), expected, key)
        self.assertEqual(len(packet.records), 10)
        for i, record in enumerate(packet.records):
            self.assertEqual(
                record,
                records.LogRecord(**vectors.expected_record_fields(i, base_tick=1010)),
                f"记录 {i}")
        # 同包记录索引/tick 连续
        ticks = [r.tick for r in packet.records]
        self.assertEqual(ticks, list(range(1010, 1020)))

    def test_log_data(self):
        header, payload = decode(vectors.LOG_DATA_HEX)
        self.assertTrue(header.reliable)
        self.assertEqual(header.request_id, 6)
        block = records.unpack_log_data(payload)
        self.assertEqual(block.offset, 256)
        self.assertEqual(block.data, bytes(range(32)))


class TestErrorVectors(unittest.TestCase):
    """错误 CRC / 错误长度向量必须被拒绝。"""

    def test_bad_crc_rejected(self):
        with self.assertRaises(proto.ProtocolError):
            decode(vectors.BAD_CRC_HEX)

    def test_bad_length_rejected(self):
        with self.assertRaises(proto.ProtocolError):
            decode(vectors.BAD_LENGTH_HEX)


class TestDuplicateVectors(unittest.TestCase):
    """重复可靠帧与重复 request_id 向量。"""

    def test_duplicate_reliable_frames_identical(self):
        first, second = vectors.DUP_RELIABLE_HEX
        self.assertEqual(first, second)  # 重发保留原 seq 和原内容
        h1, p1 = decode(first)
        h2, p2 = decode(second)
        self.assertEqual(h1, h2)
        self.assertEqual(p1, p2)
        self.assertTrue(h1.reliable)
        self.assertEqual(h1.seq, 7)
        self.assertEqual(h1.request_id, 9)

    def test_duplicate_request_id_same_content(self):
        first, second, _ = vectors.DUP_REQUEST_ID_HEX
        h1, p1 = decode(first)
        h2, p2 = decode(second)
        # 同编号同内容：应返回缓存结果，不重复执行
        self.assertEqual(h1.request_id, 11)
        self.assertEqual(h2.request_id, 11)
        self.assertEqual(p1, p2)
        self.assertEqual(proto.decode_json_payload(p1)["op"], "TUNE_ENABLE")

    def test_duplicate_request_id_different_content(self):
        first, _, third = vectors.DUP_REQUEST_ID_HEX
        h1, p1 = decode(first)
        h3, p3 = decode(third)
        # 同编号不同内容：设备应返回 INVALID_ARG
        self.assertEqual(h1.request_id, h3.request_id)
        self.assertNotEqual(p1, p3)
        self.assertEqual(proto.decode_json_payload(p3)["op"], "TUNE_DISABLE")


class TestVectorsThroughParser(unittest.TestCase):
    """全部正常向量拼接后经 TCP 增量解析器解出。"""

    def test_stream_parse(self):
        stream = b"".join(vectors.from_hex(h) for h in (
            vectors.IDLE_HEX, vectors.GET_STATUS_HEX, vectors.SET_PARAMS_HEX,
            vectors.TELEMETRY_1_HEX, vectors.TELEMETRY_10_HEX,
            vectors.LOG_DATA_HEX,
        ))
        frames = proto.FrameParser().feed(stream)
        self.assertEqual(
            [h.msg_type for h, _ in frames],
            [proto.MSG_IDLE, proto.MSG_COMMAND, proto.MSG_COMMAND,
             proto.MSG_TELEMETRY, proto.MSG_TELEMETRY, proto.MSG_LOG_DATA])


if __name__ == "__main__":
    unittest.main()
