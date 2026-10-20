"""monitor 与 CLI 测试：本地逐行 JSON 接口、转发、loopback 绑定、convert 子命令。"""

import json
import os
import shutil
import socket
import sys
import tempfile
import threading
import time
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import scb_cli
import scb_monitor as monitor_mod
import scb_records as records
from mock_esp32 import MockESP32, make_record, make_header


def free_tcp_port() -> int:
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind(("127.0.0.1", 0))
    port = s.getsockname()[1]
    s.close()
    return port


def free_udp_port() -> int:
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.bind(("127.0.0.1", 0))
    port = s.getsockname()[1]
    s.close()
    return port


class TestMonitor(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.mkdtemp(prefix="scbmon_")
        self.mock = MockESP32()
        self.mock_port = self.mock.start()
        self.local_port = free_tcp_port()
        self.udp_port = free_udp_port()
        self.monitor = monitor_mod.Monitor(
            host="127.0.0.1", port=self.mock_port, udp_port=self.udp_port,
            output_base=os.path.join(self.tmp, "run01"),
            local_port=self.local_port)
        self.thread = threading.Thread(target=self.monitor.run, daemon=True)
        self.thread.start()
        # 等本地接口就绪
        deadline = time.time() + 5.0
        while time.time() < deadline:
            try:
                with socket.create_connection(("127.0.0.1", self.local_port),
                                              timeout=0.2):
                    break
            except OSError:
                time.sleep(0.05)
        else:
            self.fail("monitor 本地接口未就绪")

    def tearDown(self):
        self.monitor._stop.set()
        self.thread.join(timeout=5.0)
        self.mock.stop()
        shutil.rmtree(self.tmp, ignore_errors=True)

    def test_status_and_forward(self):
        response = monitor_mod.forward_to_monitor("STATUS", {},
                                                  local_port=self.local_port)
        self.assertEqual(response["status"], "OK")
        self.assertTrue(response["result"]["connected"])
        self.assertEqual(response["result"]["session_id"],
                         self.mock.session_id)
        # 转发设备操作
        response = monitor_mod.forward_to_monitor("GET_STATUS", {},
                                                  local_port=self.local_port)
        self.assertEqual(response["status"], "OK")
        self.assertEqual(response["result"]["mode"], 1)
        ops = [c["op"] for c in self.mock.received_commands]
        self.assertIn("GET_STATUS", ops)

    def test_invalid_op_rejected(self):
        response = monitor_mod.forward_to_monitor("REMOTE_START", {},
                                                  local_port=self.local_port)
        self.assertEqual(response["status"], "INVALID_ARG")

    def test_shutdown_via_local(self):
        response = monitor_mod.forward_to_monitor("SHUTDOWN", {},
                                                  local_port=self.local_port)
        self.assertEqual(response["status"], "OK")
        self.thread.join(timeout=5.0)
        self.assertFalse(self.thread.is_alive())
        # 有序退出：统计与事件文件保留
        base = os.path.join(self.tmp, "run01")
        self.assertTrue(os.path.exists(base + ".events.jsonl"))
        self.assertTrue(os.path.exists(base + ".link_stats.json"))

    def test_local_binds_loopback_only(self):
        # 外部地址不可达：只绑 127.0.0.1
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(0.5)
        with self.assertRaises(OSError):
            s.connect(("0.0.0.0", self.local_port))  # 非 loopback 形式不可连
        s.close()


class TestCliConvert(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.mkdtemp(prefix="scbcli_")

    def tearDown(self):
        shutil.rmtree(self.tmp, ignore_errors=True)

    def test_convert_command(self):
        header = make_header()
        recs = [make_record(1000 + i, i) for i in range(5)]
        bin_path = os.path.join(self.tmp, "log.bin")
        with open(bin_path, "wb") as f:
            f.write(records.pack_log_header(header))
            for r in recs:
                f.write(records.pack_record(r))
        csv_path = os.path.join(self.tmp, "out.csv")
        rc = scb_cli.main(["convert", bin_path, "--output", csv_path])
        self.assertEqual(rc, 0)
        with open(csv_path, encoding="utf-8") as f:
            lines = f.read().splitlines()
        self.assertTrue(lines[0].startswith("#log_version=2"))
        self.assertTrue(any(line.startswith("#note,") for line in lines))

    def test_convert_bad_file_returns_1(self):
        bad = os.path.join(self.tmp, "bad.bin")
        with open(bad, "wb") as f:
            f.write(b"\x00" * 10)
        rc = scb_cli.main(["convert", bad])
        self.assertEqual(rc, 1)

    def test_parser_all_subcommands(self):
        parser = scb_cli.build_parser()
        args = parser.parse_args(["monitor", "--output", "run01"])
        self.assertEqual(args.output, "run01")
        args = parser.parse_args(["tune", "set", "--kp", "2", "--speed", "180"])
        self.assertEqual(args.kp, 2.0)
        self.assertEqual(args.speed, 180)
        args = parser.parse_args(["params", "get", "--mode", "1", "--gear", "0"])
        self.assertEqual((args.mode, args.gear), (1, 0))
        args = parser.parse_args(["dump", "--output", "run01-full"])
        self.assertEqual(args.output, "run01-full")
        # 默认值
        args = parser.parse_args(["params", "get"])
        self.assertEqual(args.host, "192.168.4.1")
        self.assertEqual(args.port, 5000)
        self.assertEqual(args.udp_port, 5001)


if __name__ == "__main__":
    unittest.main()
