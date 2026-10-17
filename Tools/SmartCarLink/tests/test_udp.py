"""UDP 实时接收与输出文件组测试（验收第 3 条与第 5 条 CSV 部分）。

缺口、乱序、重复、PC 队列满溢出计数、新运行索引归零不归并、tick 回绕。
"""

import json
import os
import queue
import shutil
import sys
import tempfile
import time
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import scb_client as client_mod
import scb_output as output_mod
from mock_esp32 import MockESP32, make_record

SESSION_ID = 0x1234
RUN_ID = 7


class UdpTestBase(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.mkdtemp(prefix="scbtest_")
        self.base = os.path.join(self.tmp, "run01")
        self.mock = MockESP32(session_id=SESSION_ID)  # 只用其 UDP 发送
        # 找空闲 UDP 端口
        import socket
        probe = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        probe.bind(("127.0.0.1", 0))
        self.udp_port = probe.getsockname()[1]
        probe.close()
        self.client = client_mod.DeviceClient(
            host="127.0.0.1", udp_port=self.udp_port,
            udp_queue_size=self.queue_size())
        self.client.session.adopt_session(SESSION_ID, link_ready=True)
        self.client._device_ip = "127.0.0.1"
        self.recorder = output_mod.RunRecorder(self.base)
        self.recorder.start()
        self.client._local_event_handler = self.recorder.log_event
        self.client.start_udp(self.udp_port)

    def queue_size(self):
        return 1024

    def tearDown(self):
        self.client.close()
        self.recorder.close()
        self.mock.stop()
        shutil.rmtree(self.tmp, ignore_errors=True)

    def send_records(self, first_index, ticks, run_id=RUN_ID):
        recs = [make_record(t, i) for i, t in enumerate(ticks)]
        self.mock.send_telemetry(self.udp_port, recs, first_index,
                                 run_id=run_id,
                                 first_tick=ticks[0])

    def drain(self, count, timeout=3.0):
        """消费 count 个遥测包。"""
        deadline = time.time() + timeout
        got = 0
        while got < count and time.time() < deadline:
            try:
                recv_ns, raw, packet = self.client.telemetry_queue.get(
                    timeout=0.2)
            except queue.Empty:
                continue
            self.recorder.handle_packet(recv_ns, raw, SESSION_ID, packet)
            got += 1
        self.assertEqual(got, count, "消费遥测包超时")
        self.recorder.flush()

    def csv_rows(self, path=None):
        path = path or (self.base + ".csv")
        with open(path, encoding="utf-8") as f:
            return [line.strip().split(",") for line in f if line.strip()]


class TestGapDuplicateOutOfOrder(UdpTestBase):
    def test_gap_counted(self):
        self.send_records(0, [1000, 1001, 1002])
        self.send_records(6, [1006, 1007])   # 缺 3,4,5
        self.drain(2)
        self.assertEqual(self.recorder.stats["records_gap_missing"], 3)
        self.assertEqual(self.recorder.stats["gap_count"], 1)
        self.assertEqual(self.recorder.stats["records_written"], 5)
        # 缺口事件已写
        with open(self.base + ".events.jsonl", encoding="utf-8") as f:
            events = [json.loads(line) for line in f]
        gaps = [e for e in events if e.get("event") == "record_gap"]
        self.assertEqual(len(gaps), 1)
        self.assertEqual(gaps[0]["missing"], 3)

    def test_duplicate_not_rewritten(self):
        self.send_records(0, [1000, 1001, 1002])
        self.send_records(0, [1000, 1001, 1002])   # 完全重复
        self.drain(2)
        self.assertEqual(self.recorder.stats["records_duplicate"], 3)
        self.assertEqual(self.recorder.stats["records_written"], 3)
        rows = self.csv_rows()
        self.assertEqual(len(rows), 1 + 3)   # 列头 + 3 条

    def test_out_of_order_marked_not_interpolated(self):
        self.send_records(0, [1000, 1001, 1002])
        self.send_records(5, [1005])           # 先跳到 5（缺口 3,4）
        self.send_records(3, [1003, 1004])     # 迟到的 3,4：乱序
        self.drain(3)
        s = self.recorder.stats
        self.assertEqual(s["records_out_of_order"], 2)
        self.assertEqual(s["records_written"], 6)
        self.assertEqual(s["records_gap_missing"], 2)  # 3,4 曾计为缺口
        # CSV 共 7 行（列头+6），不插值补假样
        rows = self.csv_rows()
        self.assertEqual(len(rows), 7)
        ticks = [int(r[0]) for r in rows[1:]]
        self.assertEqual(sorted(ticks), [1000, 1001, 1002, 1003, 1004, 1005])

    def test_csv_column_count_and_raw_file(self):
        self.send_records(0, [1000, 1001])
        self.drain(1)
        rows = self.csv_rows()
        for row in rows:
            self.assertEqual(len(row), 57)
        # raw 文件：每条 u64 时间ns + u32 长度 + 帧字节
        with open(self.base + ".raw", "rb") as f:
            data = f.read()
        import struct as st
        recv_ns, length = st.unpack_from("<QI", data, 0)
        self.assertGreater(recv_ns, 0)
        self.assertEqual(length, 32 + 24 + 2 * 96)
        self.assertEqual(len(data), 12 + length)


class TestNewRunAndTickWrap(UdpTestBase):
    def test_new_run_new_segment_not_merged(self):
        self.send_records(0, [1000, 1001], run_id=7)
        self.send_records(0, [5000, 5001], run_id=8)   # 新运行索引归零
        self.drain(2)
        s = self.recorder.stats
        self.assertEqual(s["run_count"], 2)
        self.assertEqual(s["records_written"], 4)   # 新运行数据未被按索引误删
        self.assertEqual(s["records_duplicate"], 0)
        self.assertTrue(os.path.exists(self.base + ".seg2.csv"))
        rows = self.csv_rows(self.base + ".seg2.csv")
        self.assertEqual(len(rows), 1 + 2)
        self.assertEqual([r[0] for r in rows[1:]], ["5000", "5001"])

    def test_tick_wrap_classified(self):
        self.send_records(0, [0xFFFFFFFE])
        self.send_records(1, [0xFFFFFFFF])
        self.send_records(2, [0, 1])   # tick 自然回绕
        self.drain(3)
        self.assertEqual(self.recorder.stats["tick_wraps"], 1)
        self.assertEqual(self.recorder.stats["records_out_of_order"], 0)


class TestQueueOverflow(unittest.TestCase):
    def test_overflow_counted_and_event(self):
        tmp = tempfile.mkdtemp(prefix="scbtest_")
        try:
            import socket
            probe = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            probe.bind(("127.0.0.1", 0))
            udp_port = probe.getsockname()[1]
            probe.close()
            local_events = []
            client = client_mod.DeviceClient(
                host="127.0.0.1", udp_port=udp_port, udp_queue_size=4)
            client.session.adopt_session(SESSION_ID, link_ready=True)
            client._device_ip = "127.0.0.1"
            client._local_event_handler = local_events.append
            client.start_udp(udp_port)
            mock = MockESP32(session_id=SESSION_ID)
            try:
                for i in range(20):   # 不消费，队列只有 4
                    mock.send_telemetry(udp_port, [make_record(1000 + i)], i)
                deadline = time.time() + 3.0
                while client.udp_stats["received"] + client.udp_stats["queue_overflow"] < 20 \
                        and time.time() < deadline:
                    time.sleep(0.02)
                self.assertGreater(client.udp_stats["queue_overflow"], 0)
                self.assertEqual(
                    client.udp_stats["received"] + client.udp_stats["queue_overflow"]
                    + client.udp_stats["bad_packets"], 20)
                self.assertTrue(any(e.get("event") == "udp_queue_overflow"
                                    for e in local_events))
            finally:
                client.close()
                mock.stop()
        finally:
            shutil.rmtree(tmp, ignore_errors=True)


class TestBadPacket(UdpTestBase):
    def test_bad_crc_dropped_not_parsed(self):
        self.send_records(0, [1000])
        frame = bytearray(self.mock.send_telemetry(
            self.udp_port, [make_record(1001)], 1))
        frame[-1] ^= 0xFF   # 破坏负载 → CRC 错
        import socket
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.sendto(bytes(frame), ("127.0.0.1", self.udp_port))
        sock.close()
        self.drain(1)   # 只有第一个好包被消费
        deadline = time.time() + 2.0
        while self.client.udp_stats["bad_packets"] < 1 and time.time() < deadline:
            time.sleep(0.02)
        self.assertEqual(self.client.udp_stats["bad_packets"], 1)
        self.assertEqual(self.recorder.stats["records_written"], 1)


if __name__ == "__main__":
    unittest.main()
