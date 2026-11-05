"""Exercise PowerShell activation/entry failures with the actual scripts."""

import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class BuildEntrypointTests(unittest.TestCase):
    def setUp(self):
        self.shell = shutil.which("pwsh") or shutil.which("powershell.exe")
        self.assertTrue(self.shell, "Existing PowerShell is required")
        parent = ROOT / ".local/test-runs"
        parent.mkdir(parents=True, exist_ok=True)
        self.directory = tempfile.TemporaryDirectory(dir=parent)
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)

    def run_script(self, script, environment):
        return subprocess.run([self.shell, "-NoProfile", "-NonInteractive", "-File", str(script)],
                              env=environment, capture_output=True, timeout=45)

    def test_build_requires_selected_python_environment(self):
        environment = os.environ.copy()
        environment.pop("IDF_PYTHON_ENV_PATH", None)
        result = self.run_script(ROOT / "scripts/build.ps1", environment)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn(b"Activate the specified", result.stdout + result.stderr)

    def test_build_rejects_missing_interpreter(self):
        environment = os.environ.copy()
        environment["IDF_PYTHON_ENV_PATH"] = str(self.root / "missing")
        result = self.run_script(ROOT / "scripts/build.ps1", environment)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn(b"Python environment is unavailable", result.stdout + result.stderr)

    def test_activate_refuses_other_sdk_without_switching(self):
        environment = os.environ.copy()
        environment["IDF_PATH"] = "D:/esp/v5.5.5/esp-idf"
        result = self.run_script(ROOT / "scripts/activate.ps1", environment)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn(b"Another IDF_PATH", result.stdout + result.stderr)

    def test_failed_environment_check_restores_previous_variables(self):
        helper = self.root / "rollback.ps1"
        # Paths are data passed through an environment variable, never shell text.
        helper.write_text('''$names = @('PATH','IDF_PATH','IDF_TOOLS_PATH','IDF_PYTHON_ENV_PATH','IDF_COMPONENT_MANAGER','IDF_CCACHE_ENABLE')
$before = @{}
foreach ($name in $names) { $before[$name] = [Environment]::GetEnvironmentVariable($name,'Process') }
try { . $env:BRIDGE_ACTIVATION_SCRIPT; exit 91 } catch {
    foreach ($name in $names) {
        if ([Environment]::GetEnvironmentVariable($name,'Process') -cne $before[$name]) {
            Write-Output ('rollback-mismatch:' + $name)
            exit 92
        }
    }
    Write-Output 'rollback-preserved'
    exit 1
}
''', encoding="utf-8")
        environment = os.environ.copy()
        environment.pop("IDF_PATH", None)
        environment.pop("IDF_PYTHON_ENV_PATH", None)
        environment["IDF_TARGET"] = "esp32"
        environment["BRIDGE_ACTIVATION_SCRIPT"] = str(ROOT / "scripts/activate.ps1")
        result = self.run_script(helper, environment)
        self.assertEqual(result.returncode, 1, result.stdout.decode("utf-8", errors="replace"))
        self.assertIn(b"rollback-preserved", result.stdout)


if __name__ == "__main__":
    unittest.main()
