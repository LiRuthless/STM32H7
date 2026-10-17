"""Exercise the actual credential/preflight checker, including non-disclosure."""

import importlib.util
import os
from pathlib import Path
import secrets
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

CHECKER = Path(__file__).resolve().parents[1] / "scripts/check_config.py"
spec = importlib.util.spec_from_file_location("check_config", CHECKER)
config = importlib.util.module_from_spec(spec)
spec.loader.exec_module(config)


class ConfigurationTests(unittest.TestCase):
    def setUp(self):
        # The test values are disposable, never credentials for a real AP.
        local_root = CHECKER.parents[1] / ".local/test-runs"
        local_root.mkdir(parents=True, exist_ok=True)
        self.directory = tempfile.TemporaryDirectory(dir=local_root)
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.local = self.root / "sdkconfig.local"
        self.password = secrets.token_hex(8)

    def write_local(self, value):
        escaped = value.replace("\\", "\\\\").replace('"', '\\"')
        self.local.write_bytes((config.PASSWORD_KEY + '="' + escaped + '"\n').encode("utf-8"))

    def generated(self, value):
        self.write_local(value)
        path = self.root / "sdkconfig"
        path.write_bytes(self.local.read_bytes() + b'CONFIG_IDF_TARGET="esp32s3"\nCONFIG_IDF_TARGET_ESP32S3=y\n')
        return path

    def test_length_boundaries(self):
        for length in (0, 7, 8, 63, 64):
            with self.subTest(length=length):
                self.write_local(secrets.token_hex(32)[:length])
                if length in (8, 63):
                    self.assertEqual(len(config.read_password(self.local)), length)
                else:
                    with self.assertRaises(config.ConfigError):
                        config.read_password(self.local)

    def test_missing_nonascii_and_nul(self):
        with self.assertRaises(config.ConfigError):
            config.read_password(self.local)
        for suffix in ("中", "\0"):
            self.write_local(self.password + suffix)
            with self.assertRaises(config.ConfigError):
                config.read_password(self.local)

    def test_escaped_quotes_backslashes_and_ascii(self):
        self.password += '"\\ #$;\t'
        self.write_local(self.password)
        self.assertEqual(config.read_password(self.local), self.password)

    def test_duplicate_unknown_and_malformed_assignments(self):
        self.write_local(self.password)
        original = self.local.read_bytes()
        for data in (original * 2, original + b"CONFIG_SPIRAM=y\n", original[:-2], b"# no password\n"):
            self.local.write_bytes(data)
            with self.assertRaises(config.ConfigError):
                config.read_password(self.local)

    def test_generated_target_memory_and_stale_password(self):
        path = self.generated(self.password)
        config.check_generated(self.password, path)
        original = path.read_bytes()
        for data in (original.replace(b"esp32s3", b"esp32"), original + b"CONFIG_SPIRAM=y\n",
                     original + b"CONFIG_FREERTOS_UNICORE=y\n"):
            path.write_bytes(data)
            with self.assertRaises(config.ConfigError):
                config.check_generated(self.password, path)
        path.write_bytes(original)
        with self.assertRaises(config.ConfigError):
            config.check_generated(secrets.token_hex(12), path)

    def test_reject_wrong_idf_path_and_sdk_declaration(self):
        with patch.dict(os.environ, {"IDF_PATH": str(self.root)}):
            with self.assertRaises(config.ConfigError):
                config.check_environment()
        version_file = self.root / "tools/cmake/version.cmake"
        version_file.parent.mkdir(parents=True)
        version_file.write_text("set(IDF_VERSION_MAJOR 5)\nset(IDF_VERSION_MINOR 5)\nset(IDF_VERSION_PATCH 5)\n")
        with patch.object(config, "EXPECTED_IDF", self.root), patch.dict(os.environ, {"IDF_PATH": str(self.root)}):
            with self.assertRaisesRegex(config.ConfigError, "exactly v6.0.0"):
                config.check_environment()

    def test_wrong_python_target_and_missing_tools(self):
        with patch.dict(os.environ, {"IDF_PATH": str(config.EXPECTED_IDF), "IDF_TARGET": "esp32"}):
            with self.assertRaisesRegex(config.ConfigError, "IDF_TARGET"):
                config.check_environment()
        with patch.dict(os.environ, {"IDF_PATH": str(config.EXPECTED_IDF), "IDF_TARGET": "esp32s3",
                                     "IDF_PYTHON_ENV_PATH": str(self.root)}):
            with self.assertRaisesRegex(config.ConfigError, "interpreter"):
                config.check_environment()
        with patch.dict(os.environ, {"IDF_PATH": str(config.EXPECTED_IDF), "IDF_TARGET": "esp32s3",
                                     "IDF_PYTHON_ENV_PATH": str(Path(sys.executable).parent.parent)}), \
                patch.object(config.shutil, "which", return_value=None):
            with self.assertRaisesRegex(config.ConfigError, "tool missing"):
                config.check_environment()

    def test_cli_never_echoes_invalid_input(self):
        self.write_local(self.password + "中")
        result = subprocess.run([sys.executable, str(CHECKER), "--local", str(self.local)], capture_output=True)
        self.assertEqual(result.returncode, 1)
        self.assertNotIn(self.password.encode(), result.stdout + result.stderr)
        self.write_local(self.password)
        result = subprocess.run([sys.executable, str(CHECKER), "--local", str(self.local)], capture_output=True)
        self.assertEqual(result.returncode, 0)
        self.assertNotIn(self.password.encode(), result.stdout + result.stderr)

    def test_actual_missing_tool_environment(self):
        self.write_local(self.password)
        environment = os.environ.copy()
        environment.update(IDF_PATH=str(config.EXPECTED_IDF), IDF_TARGET="esp32s3",
                           IDF_PYTHON_ENV_PATH=str(Path(sys.executable).parent.parent), PATH="")
        result = subprocess.run([sys.executable, str(CHECKER), "--local", str(self.local), "--environment"],
                                env=environment, capture_output=True)
        self.assertEqual(result.returncode, 1)
        self.assertIn(b"Required build tool missing", result.stderr)
        self.assertNotIn(self.password.encode(), result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
