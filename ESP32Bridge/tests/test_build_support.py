"""Real filesystem/error-path tests; SDK command fixtures are explicitly simulated."""

import importlib.util
import json
import os
from pathlib import Path
import secrets
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
import build_support as build


class BuildSupportTests(unittest.TestCase):
    def setUp(self):
        parent = ROOT / ".local/test-runs"
        parent.mkdir(parents=True, exist_ok=True)
        self.directory = tempfile.TemporaryDirectory(dir=parent)
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name).resolve()
        self.source = self.root / "source with spaces"
        for name in ("main", "scripts", ".local", ".git", "tests"):
            (self.source / name).mkdir(parents=True, exist_ok=True)
        for name in ("CMakeLists.txt", "sdkconfig.defaults", "main/app_main.c", "scripts/check_config.py"):
            (self.source / name).write_text("host filesystem fixture\n", encoding="utf-8")
        (self.source / ".git/not-a-source").write_text("do not copy", encoding="utf-8")
        (self.source / ".local/unrelated-private-file").write_text("do not copy", encoding="utf-8")
        (self.source / "tests/not-needed-for-target").write_text("do not copy", encoding="utf-8")
        self.password = secrets.token_hex(12)
        (self.source / ".local/sdkconfig.local").write_text('CONFIG_BRIDGE_AP_PASSWORD="' + self.password + '"\n', encoding="ascii")
        self.work_root = self.root / "work"
        self.calls = []

    def fake_runner(self, arguments, cwd, diagnostics):
        self.calls.append(arguments[-1])
        diagnostics.write("host fixture output " + self.password + "\n")
        if arguments[-1] == "size":
            diagnostics.write("HOST FIXTURE SIZE; NOT TARGET EVIDENCE\n")
            return 0
        output = cwd / "build"
        output.mkdir()
        for name in build.PRODUCTS:
            path = output / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(b"HOST FIXTURE PRODUCT\n")
        metadata = {"flash_files": {"0x0": "bootloader/bootloader.bin", "0x8000": "partition_table/partition-table.bin",
                                    "0x10000": "esp32bridge.bin"}, "extra_esptool_args": {"chip": "esp32s3"},
                    "bootloader": {"file": "bootloader/bootloader.bin"}, "partition-table": {"file": "partition_table/partition-table.bin"},
                    "app": {"file": "esp32bridge.bin"}}
        (output / "flasher_args.json").write_text(json.dumps(metadata), encoding="utf-8")
        (cwd / "sdkconfig").write_bytes((cwd / ".local/sdkconfig.local").read_bytes() +
                                       b'CONFIG_IDF_TARGET="esp32s3"\nCONFIG_IDF_TARGET_ESP32S3=y\n')
        (output / "project_description.json").write_text(json.dumps({"target": "esp32s3",
            "build_component_paths": [str(cwd / "main"), str(build.EXPECTED_IDF / "components/log")]}), encoding="utf-8")
        return 0

    def run_build(self, runner=None, **kwargs):
        return build.build_project(self.source, self.work_root, runner=runner or self.fake_runner,
                                   environment_checker=lambda: {"host_fixture": True},
                                   sdk_reader=lambda: {"host_fixture": True}, **kwargs)

    def assert_private_diagnostics(self, result):
        for path in Path(result["diagnostic_directory"]).rglob("*"):
            if path.is_file():
                self.assertTrue(self.password.encode() not in path.read_bytes(), "Credential disclosed by diagnostics")

    def test_success_cleanup_artifacts_and_unchanged_source(self):
        before = (self.source / "main/app_main.c").read_bytes()
        result = self.run_build()
        self.assertEqual(result["status"], "success")
        self.assertEqual(self.calls, ["build", "size"])
        self.assertFalse(Path(result["workspace"]).exists())
        self.assertEqual((self.source / "main/app_main.c").read_bytes(), before)
        output = Path(result["artifact_directory"])
        self.assertTrue(all((output / name).is_file() for name in build.PRODUCTS))
        self.assertFalse((output / "sdkconfig").exists())
        self.assertEqual(json.loads((output / "manifest.json").read_text())["status"], "success")
        self.assert_private_diagnostics(result)

    def test_keep_workspace_and_source_whitelist(self):
        result = self.run_build(keep_work=True)
        work = Path(result["workspace"])
        self.assertEqual(result["status"], "success")
        self.assertTrue(work.is_dir())
        self.assertTrue((work / ".local/sdkconfig.local").is_file())
        self.assertFalse((work / ".local/unrelated-private-file").exists())
        self.assertFalse((work / ".git").exists())
        self.assertFalse((work / "tests").exists())
        self.assertNotIn(".local/sdkconfig.local", [entry["path"] for entry in result["source_files"]])

    def test_copy_failure_retains_owned_workspace(self):
        with patch.object(build.shutil, "copy2", side_effect=OSError("injected copy failure")):
            result = self.run_build()
        self.assertEqual((result["status"], result["stage"]), ("failed", "copy"))
        self.assertTrue(Path(result["workspace"]).is_dir())
        self.assertEqual(self.calls, [])
        self.assert_private_diagnostics(result)

    def test_compile_failure_retains_diagnostics_and_source(self):
        def fail(arguments, cwd, diagnostics):
            diagnostics.write("failure " + self.password + "\n")
            return 7
        result = self.run_build(fail)
        self.assertEqual((result["status"], result["stage"], result["tool_exit_code"]), ("failed", "build", 7))
        self.assertTrue(Path(result["workspace"]).exists())
        self.assertTrue((self.source / "main/app_main.c").exists())
        self.assert_private_diagnostics(result)

    def test_size_failure(self):
        def runner(args, cwd, diagnostics):
            return self.fake_runner(args, cwd, diagnostics) if args[-1] == "build" else 9
        result = self.run_build(runner)
        self.assertEqual((result["status"], result["stage"], result["tool_exit_code"]), ("failed", "size", 9))
        self.assertTrue(result["workspace_retained"])

    def test_missing_product_never_publishes_success(self):
        def runner(args, cwd, diagnostics):
            code = self.fake_runner(args, cwd, diagnostics)
            if args[-1] == "build": (cwd / "build/esp32bridge.map").unlink()
            return code
        result = self.run_build(runner)
        self.assertEqual((result["status"], result["stage"]), ("failed", "artifacts"))
        for path in (self.source / "artifacts").glob("*/manifest.json"):
            self.assertEqual(json.loads(path.read_text())["status"], "failed")

    def test_stale_generated_credentials(self):
        def runner(args, cwd, diagnostics):
            code = self.fake_runner(args, cwd, diagnostics)
            (cwd / "sdkconfig").write_text('CONFIG_BRIDGE_AP_PASSWORD="' + secrets.token_hex(12) + '"\n', encoding="ascii")
            return code
        result = self.run_build(runner)
        self.assertEqual((result["status"], result["stage"]), ("failed", "configuration"))
        self.assert_private_diagnostics(result)

    def test_external_flash_paths_refused(self):
        def runner(args, cwd, diagnostics):
            code = self.fake_runner(args, cwd, diagnostics)
            if args[-1] == "build":
                path = cwd / "build/flasher_args.json"
                data = json.loads(path.read_text())
                data["flash_files"]["0x0"] = "../../external.bin"
                path.write_text(json.dumps(data))
            return code
        result = self.run_build(runner)
        self.assertEqual((result["status"], result["stage"]), ("failed", "artifacts"))

    def test_component_source_escape_refused(self):
        def runner(args, cwd, diagnostics):
            code = self.fake_runner(args, cwd, diagnostics)
            path = cwd / "build/project_description.json"
            path.write_text(json.dumps({"target": "esp32s3", "build_component_paths": [str(cwd / "main"), str(self.root / "external")]}))
            return code
        result = self.run_build(runner)
        self.assertEqual((result["status"], result["stage"]), ("failed", "configuration"))

    def test_missing_password_preflight(self):
        (self.source / ".local/sdkconfig.local").unlink()
        result = self.run_build()
        self.assertEqual((result["status"], result["stage"]), ("failed", "preflight"))
        self.assertFalse(self.work_root.exists())

    def test_whitespace_workspace_rejected(self):
        self.work_root = self.root / "work with spaces"
        result = self.run_build()
        self.assertEqual((result["status"], result["stage"]), ("failed", "prepare"))
        self.assertFalse(self.work_root.exists())

    def test_cleanup_refuses_source_root_and_wrong_owner(self):
        run_id = "a" * 32
        work = build.prepare_work(self.source, self.work_root, run_id)
        sentinel = self.source / "preserve.txt"
        sentinel.write_text("preserve")
        for target in (self.source, self.work_root):
            with self.assertRaises(build.BuildError):
                build.safe_cleanup(target, self.work_root, self.source, run_id)
        (work / build.OWNER_FILE).write_text('{}')
        with self.assertRaises(build.BuildError):
            build.safe_cleanup(work, self.work_root, self.source, run_id)
        self.assertTrue(sentinel.is_file() and work.is_dir())

    def test_redaction_escaped_forms(self):
        password = self.password + '"\\'
        raw = password + " " + json.dumps(password)[1:-1]
        sanitized = build.redact(raw, password)
        self.assertTrue(password not in sanitized)
        self.assertEqual(sanitized.count("<redacted>"), 2)

    def test_real_windows_junction_rejected_without_traversal(self):
        shell = shutil.which("pwsh") or shutil.which("powershell.exe")
        self.assertTrue(shell, "Existing PowerShell is required for the Windows junction check")
        helper = self.root / "make-junction.ps1"
        helper.write_text('param([string]$Link,[string]$Target)\n$ErrorActionPreference="Stop"\nNew-Item -ItemType Junction -Path $Link -Target $Target | Out-Null\n')
        target = self.root / "preserve-target"
        target.mkdir()
        (target / "preserve.txt").write_text("preserve")
        work = build.prepare_work(self.source, self.work_root, "b" * 32)
        for link in (self.source / "main/junction", work / "junction"):
            result = subprocess.run([shell, "-NoProfile", "-NonInteractive", "-File", str(helper), "-Link", str(link), "-Target", str(target)], capture_output=True)
            self.assertEqual(result.returncode, 0, "Cannot create the test-only junction")
            try:
                if link.parent == work:
                    with self.assertRaises(build.BuildError):
                        build.safe_cleanup(work, self.work_root, self.source, "b" * 32)
                    self.assertTrue(work.exists())
                else:
                    result = self.run_build()
                    self.assertEqual((result["status"], result["stage"]), ("failed", "copy"))
                self.assertTrue((target / "preserve.txt").is_file())
            finally:
                link.rmdir()  # Remove only the verified junction, never its target.


if __name__ == "__main__":
    unittest.main()
