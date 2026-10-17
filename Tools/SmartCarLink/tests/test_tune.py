"""调参测试（验收第 4 条）：整组拼装、非有限/越界/非整数拒绝、
RECEIVED≠APPLIED、SAVED 与生效分离、断线不重放（断线部分见 test_client）。"""

import os
import sys
import time
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import scb_client as client_mod
import scb_tune as tune
from mock_esp32 import MockESP32, DEFAULT_VALUES, DEFAULT_LIMITS


class TestBuildSetValues(unittest.TestCase):
    """整组拼装：只改指定项，其余保持设备当前值。"""

    def test_replace_only_specified(self):
        values = tune.build_set_values(
            dict(DEFAULT_VALUES), {"kp": 3.5, "base_speed": 200}, DEFAULT_LIMITS)
        self.assertEqual(values["kp"], 3.5)
        self.assertEqual(values["base_speed"], 200)
        for key in ("k2p", "kd", "kp_v", "ki_v", "fan_run", "fan_idle"):
            self.assertEqual(values[key], DEFAULT_VALUES[key])
        self.assertEqual(len(values), 8)

    def test_unknown_key_rejected(self):
        with self.assertRaises(tune.TuneError):
            tune.build_set_values(dict(DEFAULT_VALUES), {"xyz": 1},
                                  DEFAULT_LIMITS)

    def test_nan_inf_rejected(self):
        for bad in (float("nan"), float("inf"), float("-inf")):
            with self.assertRaises(tune.TuneError):
                tune.build_set_values(dict(DEFAULT_VALUES), {"kp": bad},
                                      DEFAULT_LIMITS)

    def test_non_integer_speed_rejected(self):
        with self.assertRaises(tune.TuneError):
            tune.build_set_values(dict(DEFAULT_VALUES), {"base_speed": 2.5},
                                  DEFAULT_LIMITS)
        with self.assertRaises(tune.TuneError):
            tune.build_set_values(dict(DEFAULT_VALUES), {"fan_run": True},
                                  DEFAULT_LIMITS)

    def test_out_of_range_rejected_no_clamp(self):
        with self.assertRaises(tune.TuneError):
            tune.build_set_values(dict(DEFAULT_VALUES), {"base_speed": 301},
                                  DEFAULT_LIMITS)
        with self.assertRaises(tune.TuneError):
            tune.build_set_values(dict(DEFAULT_VALUES), {"kp": 100.1},
                                  DEFAULT_LIMITS)
        # 边界值允许
        values = tune.build_set_values(dict(DEFAULT_VALUES),
                                       {"kp": 100, "base_speed": 300},
                                       DEFAULT_LIMITS)
        self.assertEqual(values["kp"], 100.0)
        self.assertEqual(values["base_speed"], 300)

    def test_missing_limits_rejected(self):
        with self.assertRaises(tune.TuneError):
            tune.build_set_values(dict(DEFAULT_VALUES), {"kp": 1.0}, None)

    def test_incomplete_current_values_rejected(self):
        broken = dict(DEFAULT_VALUES)
        del broken["kd"]
        with self.assertRaises(tune.TuneError):
            tune.build_set_values(broken, {"kp": 1.0}, DEFAULT_LIMITS)


class TuneFlowBase(unittest.TestCase):
    def setUp(self):
        self.mock = MockESP32()
        self.port = self.mock.start()
        self.events = []
        self.client = client_mod.DeviceClient(
            host="127.0.0.1", port=self.port, timeout=3.0,
            event_handler=self.events.append)
        self.client.connect()

    def tearDown(self):
        self.client.close()
        self.mock.stop()


class TestTuneSetFlow(TuneFlowBase):
    def test_full_flow_applied(self):
        outcome = tune.tune_set(self.client, {"kp": 3.0},
                                event_logger=lambda e, immediate=False:
                                self.events.append(e))
        self.assertTrue(outcome["ok"])
        self.assertEqual(outcome["status"], "APPLIED")
        # 设备收到完整 8 项：kp 替换，其余默认
        sent = [c for c in self.mock.received_commands if c["op"] == "SET_PARAMS"]
        self.assertEqual(len(sent), 1)
        values = sent[0]["args"]["values"]
        self.assertEqual(len(values), 8)
        self.assertEqual(values["kp"], 3.0)
        self.assertEqual(values["base_speed"], DEFAULT_VALUES["base_speed"])
        # 事件包含请求编号/session/tick/控制拍/revision/实际完整值/确认状态
        tune_events = [e for e in self.events
                       if e.get("event") == "tune_set_result"]
        self.assertEqual(len(tune_events), 1)
        ev = tune_events[0]
        self.assertEqual(ev["confirm_status"], "APPLIED")
        self.assertEqual(ev["revision"], 4)
        self.assertIsNotNone(ev["request_id"])
        self.assertEqual(ev["session_id"], self.client.session.session_id)
        self.assertEqual(ev["actual_values"]["kp"], 3.0)
        self.assertFalse(ev["persisted"])  # 未收到 PARAM_SAVED，不得称已持久化

    def test_received_then_applied_then_saved_event(self):
        # RECEIVED ≠ APPLIED；PARAM_SAVED 与生效分离
        self.mock.result_scripts["SET_PARAMS"] = [
            {"status": "RECEIVED"},
            {"status": "APPLIED", "values": dict(DEFAULT_VALUES),
             "revision": 5, "tick": 500, "control_sequence": 60},
        ]
        outcome = tune.tune_set(self.client, {"kd": 20.0})
        self.assertTrue(outcome["ok"])
        self.assertEqual(outcome["status"], "APPLIED")
        self.assertEqual(outcome["revision"], 5)
        # 设备随后异步发 PARAM_SAVED 事件：独立到达
        self.mock.send_event({"event": "PARAM_SAVED", "run_id": 7,
                              "tick": 600, "control_sequence": 66,
                              "revision": 5})
        deadline = time.time() + 2.0
        while not any(e.get("event") == "PARAM_SAVED" for e in self.events) \
                and time.time() < deadline:
            time.sleep(0.02)
        self.assertTrue(any(e.get("event") == "PARAM_SAVED"
                            for e in self.events))

    def test_special_element_rejected(self):
        self.mock.command_handler = lambda op, args: (
            {"status": "SPECIAL_ELEMENT"} if op == "SET_PARAMS" else None)
        outcome = tune.tune_set(self.client, {"kp": 1.0})
        self.assertFalse(outcome["ok"])
        self.assertEqual(outcome["status"], "SPECIAL_ELEMENT")

    def test_running_fan_idle_must_equal_current(self):
        self.mock.status_body = {"running": True}
        with self.assertRaises(tune.TuneError):
            tune.tune_set(self.client, {"fan_idle": 1200})
        # 不指定 fan_idle（保持当前值）则允许
        outcome = tune.tune_set(self.client, {"kp": 2.5})
        self.assertTrue(outcome["ok"])


class TestTuneEnableDisable(TuneFlowBase):
    def test_enable_precheck_and_result(self):
        outcome = tune.tune_enable(self.client)
        self.assertTrue(outcome["ok"])
        self.assertEqual(outcome["result"]["tick"], 123600)
        self.assertIn("running", outcome["precheck"])
        ops = [c["op"] for c in self.mock.received_commands]
        self.assertEqual(ops[0], "GET_STATUS")   # 先查起停/元素状态
        self.assertIn("TUNE_ENABLE", ops)

    def test_disable(self):
        outcome = tune.tune_disable(self.client)
        self.assertTrue(outcome["ok"])


if __name__ == "__main__":
    unittest.main()
