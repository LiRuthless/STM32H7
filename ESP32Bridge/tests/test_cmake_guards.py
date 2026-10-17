"""Run the real CMake entry point on invalid input before IDF can start."""

import os
from pathlib import Path
import secrets
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


@unittest.skipUnless(shutil.which("cmake"), "Run in the verified IDF tool environment")
class CMakeGuardTests(unittest.TestCase):
    def setUp(self):
        parent = ROOT / ".local/test-runs"
        parent.mkdir(parents=True, exist_ok=True)
        self.directory = tempfile.TemporaryDirectory(dir=parent)
        self.addCleanup(self.directory.cleanup)
        self.source = Path(self.directory.name) / "source"
        (self.source / "scripts").mkdir(parents=True)
        (self.source / ".local").mkdir()
        shutil.copyfile(ROOT / "CMakeLists.txt", self.source / "CMakeLists.txt")
        shutil.copyfile(ROOT / "scripts/check_config.py", self.source / "scripts/check_config.py")

    def run_rejected(self, expected, environment=None, extra=(), forbidden=None):
        result = subprocess.run(
            [shutil.which("cmake"), "-S", str(self.source), "-B", str(self.source.parent / "build"), *extra],
            env=environment, capture_output=True, timeout=45,
        )
        self.assertNotEqual(result.returncode, 0)
        output = result.stdout + result.stderr
        self.assertIn(expected.encode(), b" ".join(output.split()))
        if forbidden:
            self.assertNotIn(forbidden.encode(), output)

    def test_missing_environment(self):
        environment = os.environ.copy()
        environment.pop("IDF_PATH", None)
        self.run_rejected("Activate the specified", environment)

    def test_wrong_idf_path(self):
        environment = os.environ.copy()
        environment["IDF_PATH"] = str(self.source)
        self.run_rejected("IDF_PATH must select", environment)

    def test_other_installed_sdk(self):
        environment = os.environ.copy()
        environment["IDF_PATH"] = "D:/esp/v5.5.5/esp-idf"
        self.run_rejected("IDF_PATH must select", environment)

    def test_wrong_target(self):
        self.run_rejected("only supports IDF_TARGET=esp32s3", extra=("-DIDF_TARGET=esp32",))

    def test_missing_secret(self):
        self.run_rejected("configuration is missing")

    def test_invalid_secret_not_echoed(self):
        secret = secrets.token_hex(32)
        (self.source / ".local/sdkconfig.local").write_text(
            'CONFIG_BRIDGE_AP_PASSWORD="' + secret + '"\n', encoding="ascii")
        self.run_rejected("8 to 63 ASCII", forbidden=secret)

    def test_stale_cached_credentials_rejected_before_kconfig(self):
        secret = secrets.token_hex(12)
        (self.source / ".local/sdkconfig.local").write_text(
            'CONFIG_BRIDGE_AP_PASSWORD="' + secrets.token_hex(12) + '"\n', encoding="ascii")
        (self.source / "sdkconfig").write_text(
            'CONFIG_BRIDGE_AP_PASSWORD="' + secret + '"\n', encoding="ascii")
        self.run_rejected("Generated credentials are stale", forbidden=secret)


if __name__ == "__main__":
    unittest.main()
