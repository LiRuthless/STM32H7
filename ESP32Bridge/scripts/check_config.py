"""Fail-closed build input checks. Never print credential input or parser errors."""

import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

EXPECTED_IDF = Path("D:/esp/v6.0/esp-idf")
PASSWORD_KEY = "CONFIG_BRIDGE_AP_PASSWORD"


class ConfigError(Exception):
    """Only fixed, non-secret diagnostic messages may be raised."""


def read_password(path, local=True):
    try:
        # Decode explicitly; reject embedded NUL before handing anything to IDF.
        data = Path(path).read_bytes().decode("ascii")
    except (OSError, UnicodeError):
        raise ConfigError("Local/generated configuration is missing or is not ASCII") from None
    if "\0" in data:
        raise ConfigError("NUL is forbidden in configuration")
    values = []
    for line in data.splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        if not line.startswith(PASSWORD_KEY + "="):
            if local:
                raise ConfigError("Local configuration may contain only the AP password assignment")
            continue
        match = re.fullmatch(PASSWORD_KEY + r'="((?:[^"\\]|\\.)*)"', line)
        if match is None:
            raise ConfigError("Malformed AP password assignment")
        # Kconfig removes backslash escaping (it does not decode C/JSON escapes).
        values.append(re.sub(r"\\(.)", r"\1", match[1]))
    if len(values) != 1:
        raise ConfigError("Exactly one local AP password assignment is required")
    password = values[0]
    if not 8 <= len(password) <= 63 or any(ord(c) == 0 or ord(c) > 127 for c in password):
        raise ConfigError("AP password must be 8 to 63 ASCII bytes without NUL")
    return password


def read_idf_version(path):
    try:
        text = (Path(path) / "tools/cmake/version.cmake").read_text(encoding="utf-8")
        return tuple(int(re.search(r"set\(IDF_VERSION_" + part + r"\s+(\d+)\)", text)[1])
                     for part in ("MAJOR", "MINOR", "PATCH"))
    except (OSError, TypeError, ValueError):
        raise ConfigError("Cannot read SDK version declaration") from None


def check_environment():
    if Path(os.environ.get("IDF_PATH", "")).resolve() != EXPECTED_IDF.resolve():
        raise ConfigError("IDF_PATH must select D:/esp/v6.0/esp-idf")
    if read_idf_version(EXPECTED_IDF) != (6, 0, 0):
        raise ConfigError("SDK must declare exactly v6.0.0")
    if os.environ.get("IDF_TARGET", "esp32s3") != "esp32s3":
        raise ConfigError("IDF_TARGET must be esp32s3")
    venv = os.environ.get("IDF_PYTHON_ENV_PATH")
    if not venv or Path(sys.executable).resolve() != (Path(venv) / "Scripts/python.exe").resolve():
        raise ConfigError("Use the activated IDF Python interpreter")
    # Validate actual executables against this SDK's own recommended versions;
    # do not accept arbitrary tools merely because they appear on PATH.
    metadata = json.loads((EXPECTED_IDF / "tools/tools.json").read_text(encoding="utf-8"))
    versions = {"python": sys.version.split()[0], "idf": "6.0.0", "target": "esp32s3"}
    for name in ("xtensa-esp-elf", "cmake", "ninja"):
        tool = next(t for t in metadata["tools"] if t["name"] == name)
        executable = shutil.which(tool["version_cmd"][0])
        if not executable:
            raise ConfigError("Required build tool missing: " + name)
        try:
            result = subprocess.run([executable, *tool["version_cmd"][1:]], capture_output=True, text=True, timeout=30)
        except OSError as error:
            # Never echo argv, environment or localized exception text. Windows
            # sandbox process failures otherwise become an unhelpful generic error.
            code = getattr(error, "winerror", None) or error.errno or 0
            hint = "; verify process-launch restrictions with the same SDK; do not bypass version checks" if code == 623 else ""
            raise ConfigError("Cannot start build tool: " + name + "; OS error=" + str(code) + hint) from error
        match = re.search(tool["version_regex"], result.stdout + result.stderr)
        if result.returncode or not match or not any(
            v["name"] == match[1] and v["status"] == "recommended" for v in tool["versions"]
        ):
            raise ConfigError("Build tool does not match SDK tools.json: " + name)
        versions[name] = match[1]
    result = subprocess.run(
        [sys.executable, str(EXPECTED_IDF / "tools/idf_tools.py"), "check-python-dependencies"],
        capture_output=True, timeout=60,
    )
    if result.returncode:
        raise ConfigError("IDF Python dependency check failed; inspect SDK environment without installing automatically")
    return versions


def check_generated(local_password, path):
    if read_password(path, local=False) != local_password:
        raise ConfigError("Generated credentials are stale; remove ignored sdkconfig and reconfigure")
    data = Path(path).read_text(encoding="ascii")
    required = ('CONFIG_IDF_TARGET="esp32s3"', "CONFIG_IDF_TARGET_ESP32S3=y")
    if any(line not in data.splitlines() for line in required):
        raise ConfigError("Generated target must be esp32s3")
    if re.search(r"^CONFIG_(SPIRAM|FREERTOS_UNICORE)=y$", data, re.MULTILINE):
        raise ConfigError("PSRAM and single-core configurations are forbidden")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--local", required=True, type=Path)
    parser.add_argument("--environment", action="store_true")
    parser.add_argument("--sdkconfig", type=Path)
    args = parser.parse_args()
    try:
        password = read_password(args.local)
        if args.environment:
            print(json.dumps(check_environment(), sort_keys=True))
        if args.sdkconfig:
            check_generated(password, args.sdkconfig)
    except ConfigError as exc:
        print("ESP32Bridge configuration rejected: " + str(exc), file=sys.stderr)
        return 1
    except (OSError, ValueError, subprocess.SubprocessError):
        print("ESP32Bridge configuration check could not execute; inspect selected tools", file=sys.stderr)
        return 1
    print("ESP32Bridge configuration checks passed (credentials omitted)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
