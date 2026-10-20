"""导出与离线转换测试（验收第 5 条）：重复块、错偏移、整体 CRC 失败、
.partial 保留及原因、CSV 57 列、灰度归一化对照、convert 无事件注明不完整。"""

import json
import os
import shutil
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import scb_client as client_mod
import scb_export as export_mod
import scb_records as records
from mock_esp32 import MockESP32, make_record, make_header


class ExportTestBase(unittest.TestCase):
    RECORD_COUNT = 25

    def setUp(self):
        self.tmp = tempfile.mkdtemp(prefix="scbexp_")
        self.output_base = os.path.join(self.tmp, "run01-full")
        self.header = make_header(mode=1, gear=0)
        self.records = [make_record(1000 + i, i)
                        for i in range(self.RECORD_COUNT)]
        self.mock = MockESP32()
        self.mock.set_log_image(self.header, self.records)
        self.port = self.mock.start()
        self.client = client_mod.DeviceClient(host="127.0.0.1", port=self.port,
                                              timeout=3.0)
        self.client.connect()

    def tearDown(self):
        self.client.close()
        self.mock.stop()
        shutil.rmtree(self.tmp, ignore_errors=True)

    def exporter(self):
        return export_mod.LogExporter(self.client)


class TestExportHappyPath(ExportTestBase):
    def test_full_export(self):
        result = self.exporter().export(self.output_base)
        self.assertTrue(result.ok, result.reason)
        self.assertEqual(result.record_count, self.RECORD_COUNT)
        self.assertEqual(result.total_bytes, 256 + 96 * self.RECORD_COUNT)
        self.assertTrue(os.path.exists(result.final_path))
        self.assertFalse(os.path.exists(self.output_base + ".partial"))
        # 二进制内容与设备镜像一致
        with open(result.final_path, "rb") as f:
            data = f.read()
        self.assertEqual(data, self.mock._log_image)
        # CSV：前导 3 行 + 列头 + 记录行，57 列
        with open(result.csv_path, encoding="utf-8") as f:
            lines = [line.rstrip("\n") for line in f]
        self.assertEqual(lines[0], "#log_version=2,record_size=96,mode=1,gear=0")
        self.assertTrue(lines[1].startswith("#black,"))
        self.assertTrue(lines[2].startswith("#white,"))
        rows = [line.split(",") for line in lines[3:]]
        self.assertEqual(len(rows), 1 + self.RECORD_COUNT)
        for row in rows:
            self.assertEqual(len(row), 57)

    def test_gray_normalization_matches_firmware_semantics(self):
        # 对照 Gray_Normalize 语义逐通道核对 CSV 的 q 列
        result = self.exporter().export(self.output_base)
        self.assertTrue(result.ok, result.reason)
        with open(result.csv_path, encoding="utf-8") as f:
            lines = [line.rstrip("\n") for line in f]
        rows = [line.split(",") for line in lines[4:]]  # 跳过前导+列头
        for i, row in enumerate(rows):
            record = self.records[i]
            for ch in range(16):
                expected = records.gray_normalize(
                    record.gray_raw[ch],
                    self.header.black[ch], self.header.white[ch])
                self.assertEqual(row[32 + ch], str(expected),
                                 f"记录{i} 通道{ch}")

    def test_duplicate_block_not_double_counted(self):
        # 设备重复发送相同 LOG_DATA 块（可靠重发场景）：不重写、不重复累计 CRC
        self.mock.log_read_duplicate = True
        result = self.exporter().export(self.output_base)
        self.assertTrue(result.ok, result.reason)
        self.assertEqual(result.total_bytes, 256 + 96 * self.RECORD_COUNT)


class TestExportFailures(ExportTestBase):
    def test_wrong_offset_fails_and_partial_kept(self):
        def hook(offset, data):
            if offset == 0:
                return (offset + 96, data)  # 错偏移
            return None
        self.mock.log_read_hook = hook
        result = self.exporter().export(self.output_base)
        self.assertFalse(result.ok)
        self.assertIn("错偏移", result.reason)
        partial = self.output_base + ".partial"
        self.assertTrue(os.path.exists(partial))
        self.assertTrue(os.path.exists(partial + ".reason.txt"))
        with open(partial + ".reason.txt", encoding="utf-8") as f:
            self.assertIn("错偏移", f.read())
        self.assertFalse(os.path.exists(self.output_base + ".bin"))

    def test_transport_level_duplicate_block_skipped(self):
        # 设备对后续 LOG_READ 回应一个旧 offset 的完整重复块（可靠重发场景）：
        # 应跳过重发、不重写、不重复累计整体 CRC，导出照常成功
        calls = {"n": 0}

        def hook(offset, data):
            calls["n"] += 1
            if calls["n"] == 2:
                # 第二次读（期望 offset=960）先收到 0 偏移的旧块
                return (0, self.mock._log_image[0:960])
            return None
        self.mock.log_read_hook = hook
        result = self.exporter().export(self.output_base)
        self.assertTrue(result.ok, result.reason)
        self.assertEqual(result.total_bytes, 256 + 96 * self.RECORD_COUNT)

    def test_overall_crc_mismatch(self):
        self.mock.end_crc32_override = 0xDEADBEEF
        result = self.exporter().export(self.output_base)
        self.assertFalse(result.ok)
        self.assertIn("CRC32", result.reason)
        self.assertTrue(os.path.exists(self.output_base + ".partial"))
        self.assertFalse(os.path.exists(self.output_base + ".bin"))

    def test_busy_when_not_stopped(self):
        self.mock.allow_begin = False
        result = self.exporter().export(self.output_base)
        self.assertFalse(result.ok)
        self.assertIn("BUSY", result.reason)
        self.assertFalse(os.path.exists(self.output_base + ".partial"))

    def test_no_valid_log(self):
        mock2 = MockESP32()  # 无日志
        port2 = mock2.start()
        client2 = client_mod.DeviceClient(host="127.0.0.1", port=port2,
                                          timeout=3.0)
        client2.connect()
        try:
            result = export_mod.LogExporter(client2).export(
                os.path.join(self.tmp, "empty"))
            self.assertFalse(result.ok)
            self.assertIn("log_valid", result.reason)
        finally:
            client2.close()
            mock2.stop()


class TestConvertOffline(ExportTestBase):
    def _make_bin(self) -> str:
        result = self.exporter().export(self.output_base)
        self.assertTrue(result.ok, result.reason)
        return result.final_path

    def test_convert_without_events_notes_incomplete(self):
        bin_path = self._make_bin()
        csv_path = os.path.join(self.tmp, "conv.csv")
        info = export_mod.convert_log(bin_path, csv_path)
        self.assertTrue(info["notes"], "无事件文件必须注明参数历史不完整")
        with open(csv_path, encoding="utf-8") as f:
            content = f.read()
        self.assertIn("#note,", content)
        self.assertIn("参数历史不完整", content)

    def test_convert_with_events_no_note(self):
        bin_path = self._make_bin()
        events_path = os.path.join(self.tmp, "events.jsonl")
        with open(events_path, "w", encoding="utf-8") as f:
            f.write(json.dumps({"event": "tune_set_result"}) + "\n")
        csv_path = os.path.join(self.tmp, "conv2.csv")
        info = export_mod.convert_log(bin_path, csv_path,
                                      events_path=events_path)
        self.assertEqual(info["notes"], [])
        with open(csv_path, encoding="utf-8") as f:
            self.assertNotIn("#note,", f.read())

    def test_convert_rejects_bad_file(self):
        bad = os.path.join(self.tmp, "bad.bin")
        with open(bad, "wb") as f:
            f.write(b"\x00" * 100)
        with self.assertRaises(export_mod.ExportError):
            export_mod.convert_log(bad, os.path.join(self.tmp, "x.csv"))


if __name__ == "__main__":
    unittest.main()
