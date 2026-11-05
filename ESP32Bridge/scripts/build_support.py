"""Private build orchestration for build.ps1; no installation or flashing."""

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import stat
import subprocess
import sys
import uuid

from check_config import ConfigError, EXPECTED_IDF, check_environment, check_generated, read_password

COPY_FILES = ("CMakeLists.txt", "sdkconfig.defaults")
COPY_TREES = ("main", "scripts")
PRODUCTS = ("esp32bridge.bin", "esp32bridge.elf", "esp32bridge.map", "bootloader/bootloader.bin",
            "partition_table/partition-table.bin", "flasher_args.json", "flash_args")
FLASH_FILES = {"esp32bridge.bin", "bootloader/bootloader.bin", "partition_table/partition-table.bin"}
OWNER_FILE = ".esp32bridge-build-owner.json"


class BuildError(Exception):
    """Messages are fixed diagnostics; never include raw configuration/tool text."""


def check_no_reparse(path):
    """Check unresolved ancestors too: resolve() alone hides a junction escape."""
    absolute = Path(os.path.abspath(path))
    for part in (absolute, *absolute.parents):
        try:
            info = part.lstat()
        except FileNotFoundError:
            continue
        if stat.S_ISLNK(info.st_mode) or getattr(info, "st_file_attributes", 0) & stat.FILE_ATTRIBUTE_REPARSE_POINT:
            raise BuildError("Reparse points and symbolic links are not allowed in build paths")
    return absolute.resolve()


def ensure_directory(path):
    checked = check_no_reparse(path)
    checked.mkdir(parents=True, exist_ok=True)
    return check_no_reparse(checked)


def within(path, root):
    return path != root and path.is_relative_to(root)


def require_file(path, root):
    checked = check_no_reparse(path)
    if not within(checked, root) or not checked.is_file():
        raise BuildError("Required project/build file is missing or outside its root")
    return checked


def redact(text, password):
    if not password:
        return text
    variants = {password, json.dumps(password, ensure_ascii=True)[1:-1], repr(password)[1:-1],
                password.replace("\\", "\\\\").replace('"', '\\"')}
    for value in sorted(variants, key=len, reverse=True):
        text = text.replace(value, "<redacted>")
    return text


class Diagnostics:
    def __init__(self, path):
        self.path = path
        self.password = None

    def write(self, text):
        with self.path.open("a", encoding="utf-8", newline="\n") as stream:
            stream.write(redact(text, self.password))


def run_command(arguments, cwd, diagnostics):
    diagnostics.write("COMMAND " + json.dumps([str(x) for x in arguments]) + "\n")
    with subprocess.Popen([str(x) for x in arguments], cwd=cwd, stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT, text=True, encoding="utf-8", errors="replace") as process:
        for line in process.stdout:
            diagnostics.write(line)
        return process.wait()


def sdk_metadata():
    def git(*args):
        result = subprocess.run(["git", "-C", str(EXPECTED_IDF), *args], capture_output=True, text=True, timeout=60)
        if result.returncode:
            raise BuildError("Cannot record SDK Git state; check repository access without changing global configuration")
        return result.stdout.strip()
    return {"path": str(EXPECTED_IDF), "version": "6.0.0", "commit": git("rev-parse", "HEAD"),
            "tag": git("describe", "--tags", "--always"), "dirty": bool(git("status", "--short"))}


def copy_project(source, destination):
    """Explicit source whitelist; never recurse through the repository root."""
    for name in COPY_FILES:
        shutil.copy2(require_file(source / name, source), destination / name)
    for tree in COPY_TREES:
        directory = check_no_reparse(source / tree)
        if not within(directory, source) or not directory.is_dir():
            raise BuildError("Required project source directory is missing")
        for parent, directories, names in os.walk(directory, followlinks=False):
            directories[:] = [name for name in directories if name != "__pycache__"]
            for name in directories:
                check_no_reparse(Path(parent) / name)
            for name in names:
                if name.endswith((".pyc", ".pyo")):
                    continue
                file = require_file(Path(parent) / name, source)
                target = destination / file.relative_to(source)
                ensure_directory(target.parent)
                shutil.copy2(file, target)
    local = ensure_directory(destination / ".local")
    shutil.copy2(require_file(source / ".local/sdkconfig.local", source), local / "sdkconfig.local")


def prepare_work(source, root, run_id):
    if not re.fullmatch(r"[0-9a-f]{32}", run_id):
        raise BuildError("Invalid workspace identity")
    root = check_no_reparse(root)
    if re.search(r"\s", str(root)):
        raise BuildError("WorkRoot must have no whitespace; specify an independent absolute path")
    work = root / ("run-" + run_id)
    if work == source or source.is_relative_to(work):
        raise BuildError("Build workspace cannot contain the source project")
    ensure_directory(root)
    work.mkdir(exist_ok=False)
    owner = {"run_id": run_id, "root": str(root), "source": str(source)}
    (work / OWNER_FILE).write_text(json.dumps(owner), encoding="utf-8")
    return work


def safe_cleanup(work, root, source, run_id):
    if not re.fullmatch(r"[0-9a-f]{32}", run_id):
        raise BuildError("Cleanup refused: invalid workspace identity")
    root, work, source = check_no_reparse(root), check_no_reparse(work), check_no_reparse(source)
    if work != root / ("run-" + run_id) or not within(work, root) or work == source or source.is_relative_to(work):
        raise BuildError("Cleanup refused: workspace identity or source boundary mismatch")
    owner_path = require_file(work / OWNER_FILE, work)
    try:
        owner = json.loads(owner_path.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        raise BuildError("Cleanup refused: invalid owner marker") from None
    if owner != {"run_id": run_id, "root": str(root), "source": str(source)}:
        raise BuildError("Cleanup refused: owner marker mismatch")
    for parent, directories, names in os.walk(work, followlinks=False):
        for name in (*directories, *names):
            check_no_reparse(Path(parent) / name)
    # Only this freshly-created, verified run directory is removed. The source,
    # root directory and other runs are never passed to recursive removal.
    shutil.rmtree(work)


def validate_flash_metadata(path):
    metadata = json.loads(path.read_text(encoding="utf-8"))
    if set(metadata.get("flash_files", {}).values()) != FLASH_FILES:
        raise BuildError("Unexpected flash file list; refusing incomplete or external artifacts")
    if metadata.get("extra_esptool_args", {}).get("chip") != "esp32s3":
        raise BuildError("Unexpected flash target")
    for key in ("bootloader", "partition-table", "app"):
        if metadata.get(key, {}).get("file") not in FLASH_FILES:
            raise BuildError("Unexpected flash metadata path")


def file_record(path, base):
    return {"path": path.relative_to(base).as_posix(), "bytes": path.stat().st_size,
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}


def build_project(project, work_root=None, keep_work=False, *, runner=run_command,
                  environment_checker=check_environment, sdk_reader=sdk_metadata):
    project = check_no_reparse(project)
    run_id = uuid.uuid4().hex
    diagnostic_dir = ensure_directory(project / ".local/build-diagnostics" / run_id)
    diagnostics = Diagnostics(diagnostic_dir / "build.log")
    root = Path(work_root) if work_root else project / ".local/build-work"
    work = None
    output = None
    stage = "preflight"
    manifest = {"run_id": run_id, "status": "failed", "stage": stage, "source": str(project),
                "started_at": datetime.now(timezone.utc).isoformat(), "target": "esp32s3", "credentials": "omitted"}
    try:
        diagnostics.password = read_password(project / ".local/sdkconfig.local")
        versions = environment_checker()
        sdk = sdk_reader()
        manifest["environment"] = {"tools": versions, "sdk": sdk,
                                   "executables": {name: shutil.which(name) for name in ("cmake", "ninja", "xtensa-esp-elf-gcc")},
                                   "python": sys.executable}
        stage = "prepare"
        work = prepare_work(project, root, run_id)
        root = check_no_reparse(root)
        manifest["workspace"] = str(work)
        stage = "copy"
        copy_project(project, work)
        manifest["source_files"] = [file_record(path, work) for path in sorted(work.rglob("*"))
                                    if path.is_file() and ".local" not in path.relative_to(work).parts and path.name != OWNER_FILE]
        stage = "build"
        idf = EXPECTED_IDF / "tools/idf.py"
        code = runner([sys.executable, idf, "-B", "build", "build"], work, diagnostics)
        if code:
            manifest["tool_exit_code"] = code
            raise BuildError("ESP32 target build failed; inspect the retained redacted diagnostic")
        stage = "configuration"
        check_generated(diagnostics.password, require_file(work / "sdkconfig", work))
        description = json.loads(require_file(work / "build/project_description.json", work).read_text(encoding="utf-8"))
        if description.get("target") != "esp32s3":
            raise BuildError("Built project target mismatch")
        main_component = (work / "main").resolve()
        components = [Path(value).resolve() for value in description.get("build_component_paths", []) if value]
        if main_component not in components or any(not path.is_relative_to(EXPECTED_IDF.resolve()) and
                                                   path != main_component for path in components):
            raise BuildError("Built project references sources outside this project and the SDK")
        stage = "size"
        size_diagnostics = Diagnostics(diagnostic_dir / "size-report.txt")
        size_diagnostics.password = diagnostics.password
        code = runner([sys.executable, idf, "-B", "build", "size"], work, size_diagnostics)
        if code:
            manifest["tool_exit_code"] = code
            raise BuildError("Size report failed; retained workspace is available")
        stage = "artifacts"
        build = work / "build"
        validate_flash_metadata(require_file(build / "flasher_args.json", work))
        output = ensure_directory(project / "artifacts" / run_id)
        for name in PRODUCTS:
            source = require_file(build / name, work)
            # BIN/ELF necessarily contain local firmware configuration. They stay
            # ignored and private; textual deliverables may not disclose it.
            if source.suffix not in (".bin", ".elf") and redact(source.read_text(encoding="utf-8"), diagnostics.password) != source.read_text(encoding="utf-8"):
                raise BuildError("Credential found in a textual artifact; publication refused")
            destination = output / name
            ensure_directory(destination.parent)
            shutil.copy2(source, destination)
        shutil.copy2(size_diagnostics.path, output / "size-report.txt")
        (output / "environment.json").write_text(json.dumps(manifest["environment"], indent=2) + "\n", encoding="utf-8")
        stage = "cleanup"
        if not keep_work:
            safe_cleanup(work, root, project, run_id)
        manifest.update(status="success", stage="complete", workspace_retained=keep_work,
                        artifact_directory=str(output), build_exit_code=0, size_exit_code=0)
        diagnostics.write("Build complete; credentials omitted.\n")
        shutil.copy2(diagnostics.path, output / "build.log")
        manifest["artifacts"] = [file_record(path, output) for path in sorted(output.rglob("*")) if path.is_file()]
    except ConfigError as error:
        manifest.update(stage=stage, error=str(error))
    except BuildError as error:
        manifest.update(stage=stage, error=str(error))
    except (OSError, ValueError, TypeError, subprocess.SubprocessError):
        manifest.update(stage=stage, error="Build operation failed; check selected paths, tools and file permissions")
    manifest["finished_at"] = datetime.now(timezone.utc).isoformat()
    manifest["diagnostic_directory"] = str(diagnostic_dir)
    if manifest["status"] != "success":
        manifest["workspace_retained"] = work is not None and work.exists()
        diagnostics.write("FAILED stage=" + stage + ": " + manifest["error"] + "\n")
    serialized = json.dumps(manifest, ensure_ascii=True, indent=2) + "\n"
    (diagnostic_dir / "manifest.json").write_text(serialized, encoding="utf-8")
    if output is not None:
        (output / "manifest.json").write_text(serialized, encoding="utf-8")
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("environment", "build"))
    parser.add_argument("--work-root", type=Path)
    parser.add_argument("--keep-work", action="store_true")
    args = parser.parse_args()
    if args.action == "environment":
        try:
            print(json.dumps(check_environment(), sort_keys=True))
            return 0
        except (ConfigError, OSError, ValueError, subprocess.SubprocessError) as error:
            print(str(error) if isinstance(error, ConfigError) else "Environment check failed", file=sys.stderr)
            return 1
    try:
        result = build_project(Path(__file__).resolve().parents[1], args.work_root, args.keep_work)
        print("ESP32Bridge build " + result["status"] + "; stage=" + result["stage"])
        print("Diagnostics: " + result["diagnostic_directory"])
        if result["status"] == "success":
            print("Artifacts: " + result["artifact_directory"])
        else:
            print(result["error"], file=sys.stderr)
        return 0 if result["status"] == "success" else 1
    except (BuildError, OSError):
        print("Build path cannot be initialized safely; no workspace was removed", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
