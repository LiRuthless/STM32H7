"""scb_client + mock_esp32 集成测试（验收第 2 条）。

模拟 TCP 任意拆包粘包、部分发送、可靠重发、业务结果延迟、会话变化、
GET_REQUEST_RESULT 缺失 → NOT_FOUND → UNKNOWN 且不重放。
"""

import os
import sys
import time
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))  # tests 目录

import scb_client as client_mod
import scb_session as sess
from mock_esp32 import MockESP32


class ClientTestBase(unittest.TestCase):
    def setUp(self):
        self.mock = MockESP32()
        self.port = self.mock.start()
        self.client = client_mod.DeviceClient(
            host="127.0.0.1", port=self.port, timeout=3.0)
        self.client.connect()

    def tearDown(self):
        self.client.close()
        self.mock.stop()


class TestHandshakeAndBasic(ClientTestBase):
    def test_handshake_adopts_session(self):
        self.assertEqual(self.client.session.session_id, 0x1234)
        self.assertTrue(self.client.session.link_ready)
        self.assertTrue(self.client.connected)

    def test_get_status(self):
        result = self.client.request("GET_STATUS", {})
        self.assertEqual(result["status"], "OK")
        self.assertEqual(result["mode"], 1)
        self.assertIn("request_id", result)

    def test_arbitrary_chunking(self):
        # 对端按 7 字节分片发送（任意拆包粘包），5 次请求全部正常
        self.mock.chunk_size = 7
        for _ in range(5):
            result = self.client.request("GET_STATUS", {})
            self.assertEqual(result["status"], "OK")

    def test_link_not_ready_blocks_control(self):
        self.client.session.link_ready = False
        with self.assertRaises(client_mod.ClientError):
            self.client.request("SET_PARAMS", {"values": {}})
        # GET_STATUS 等连接状态查询仍允许
        self.client.session.link_ready = True
        result = self.client.request("GET_STATUS", {})
        self.assertEqual(result["status"], "OK")


class TestReliableRetransmit(unittest.TestCase):
    def test_retransmit_three_times_then_link_dead(self):
        mock = MockESP32()
        port = mock.start()
        client = client_mod.DeviceClient(host="127.0.0.1", port=port,
                                         timeout=3.0)
        client.connect()
        try:
            mock.withhold_acks = True  # 吞确认
            with self.assertRaises(client_mod.ClientError):
                client.request("GET_STATUS", {}, timeout=3.0)
            # 客户端应重发 3 次（同 seq），共 4 个同 seq 可靠帧，且只执行一次
            time.sleep(1.0)
            seqs = mock.received_seqs
            self.assertGreaterEqual(len(seqs), 3)
            self.assertEqual(len(set(seqs)), 1, "重发必须保留原 seq")
            self.assertEqual(len(mock.executed), 1, "重复可靠帧不重复执行")
            deadline = time.time() + 3.0
            while client.session.alive and time.time() < deadline:
                time.sleep(0.05)
            self.assertFalse(client.session.alive, "重发 3 次未确认应判链路死亡")
        finally:
            client.close()
            mock.stop()

    def test_partial_send_handled(self):
        # 模拟 socket.send 每次只发一部分，客户端必须循环补发
        client = client_mod.DeviceClient(host="127.0.0.1", port=1)
        sent = bytearray()

        class FakeSock:
            def send(self, view):
                n = max(1, len(view) // 3)  # 每次只发 1/3
                sent.extend(view[:n])
                return n

        client._sock = FakeSock()
        data = bytes(range(200))
        client._send_all(data)
        self.assertEqual(bytes(sent), data)


class TestDelayedBusinessResult(ClientTestBase):
    SCRIPT = [
        {"status": "RECEIVED"},
        {"status": "APPLIED", "values": {"kp": 2.0}, "revision": 4,
         "tick": 999, "control_sequence": 88},
    ]

    def test_received_is_not_applied(self):
        # 先 RECEIVED 后 APPLIED：request() 返回 RECEIVED（不是生效）
        self.mock.result_scripts["SET_PARAMS"] = list(self.SCRIPT)
        result = self.client.request(
            "SET_PARAMS", {"mode": 1, "gear": 0, "values": {"kp": 2.0}})
        self.assertEqual(result["status"], "RECEIVED")
        entry = self.client.session.requests.get(result["request_id"])
        self.assertEqual(entry.state, sess.REQ_RECEIVED)
        # 等 APPLIED 到达后条目转 final，调参槽位释放
        deadline = time.time() + 3.0
        while entry.state != sess.REQ_FINAL and time.time() < deadline:
            time.sleep(0.05)
        self.assertEqual(entry.state, sess.REQ_FINAL)
        self.assertEqual(entry.status, "APPLIED")
        self.assertEqual(entry.applied_values, {"kp": 2.0})

    def test_request_final_waits_for_applied(self):
        # request_final 跳过 RECEIVED 直接等到 APPLIED
        self.mock.result_scripts["SET_PARAMS"] = list(self.SCRIPT)
        result = self.client.request_final(
            "SET_PARAMS", {"mode": 1, "gear": 0, "values": {"kp": 2.0}},
            timeout=3.0)
        self.assertEqual(result["status"], "APPLIED")
        self.assertEqual(result["revision"], 4)
        self.assertEqual(result["control_sequence"], 88)


class TestSessionChangeAndRecovery(unittest.TestCase):
    def test_session_change_marks_unknown_no_replay(self):
        mock = MockESP32(session_id=0x1000)
        port = mock.start()
        client = client_mod.DeviceClient(host="127.0.0.1", port=port,
                                         timeout=3.0)
        client.connect()
        try:
            # 发起 SET_PARAMS 但设备不回确认（模拟断线前未得最终结果）
            mock.withhold_acks = True
            try:
                client.request("SET_PARAMS",
                               {"mode": 1, "gear": 0, "values": {}},
                               timeout=0.5)
            except client_mod.ClientError:
                pass
            self.assertEqual(len(mock.executed), 1)
            self.assertEqual(len(client.session.requests.unresolved()), 1)
            rid = client.session.requests.unresolved()[0].request_id
            # 会话变化后重连
            mock.drop_connection()
            mock.withhold_acks = False
            mock.new_session(0x2000)
            time.sleep(0.2)
            client.connect()
            self.assertEqual(client.session.session_id, 0x2000)
            # 会话改变：未决请求标 UNKNOWN，绝不自动重放
            client.recover_pending()
            entry = client.session.requests.get(rid)
            self.assertEqual(entry.status, sess.STATUS_UNKNOWN)
            self.assertEqual(len(mock.executed), 1, "会话变化后不得重放 SET_PARAMS")
        finally:
            client.close()
            mock.stop()

    def test_reconnect_same_session_not_found_marks_unknown(self):
        mock = MockESP32(session_id=0x1000)
        port = mock.start()
        client = client_mod.DeviceClient(host="127.0.0.1", port=port,
                                         timeout=3.0)
        client.connect()
        try:
            mock.withhold_acks = True
            try:
                client.request("SET_PARAMS",
                               {"mode": 1, "gear": 0, "values": {}},
                               timeout=0.5)
            except client_mod.ClientError:
                pass
            rid = client.session.requests.unresolved()[0].request_id
            mock.drop_connection()
            mock.withhold_acks = False
            mock.results_cache.clear()  # 设备侧结果丢失 → NOT_FOUND
            time.sleep(0.2)
            client.connect()  # 同 session_id 重连
            self.assertEqual(client.session.session_id, 0x1000)
            client.recover_pending()
            entry = client.session.requests.get(rid)
            # NOT_FOUND → UNKNOWN，不自动重放
            self.assertEqual(entry.status, sess.STATUS_UNKNOWN)
            set_params_executed = [e for e in mock.executed
                                   if e["op"] == "SET_PARAMS"]
            self.assertEqual(len(set_params_executed), 1,
                             "NOT_FOUND 后不得自动重放 SET_PARAMS")
            ops = [c["op"] for c in mock.received_commands]
            self.assertIn("GET_REQUEST_RESULT", ops)
        finally:
            client.close()
            mock.stop()


if __name__ == "__main__":
    unittest.main()
