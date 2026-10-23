"""Run the unchanged production BSP and repository C harness with existing Clang/LLD.

Alternate host compiler evidence only: this is neither GCC nor hardware validation.
No SDK installation, product toolchain changes, or imported CRT required.
"""
from pathlib import Path
import ctypes
import hashlib
import importlib.util
import json
import os
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
CLANG = Path(r"C:\Program Files\MATLAB\R2025a\toolbox\sldrt\clang\win64\clang.exe")
LLD = Path(r"C:\Espressif\tools\esp-clang\esp-20.1.1_20250829\esp-clang\bin\ld.lld.exe")


def child(dll):
    # Assertions report the exact C expression, then terminate this child process.
    # No exception or longjmp crosses the Python callback boundary.
    callback_type = ctypes.CFUNCTYPE(None, ctypes.c_char_p, ctypes.c_int)

    @callback_type
    def report(message, failed):
        print(message.decode("utf-8"), file=sys.stderr if failed else sys.stdout, flush=True)
        if failed:
            os._exit(1)

    library = ctypes.CDLL(str(dll))
    library.test_set_reporter.argtypes = [callback_type]
    library.test_set_reporter(report)
    return library.main()


def main():
    sys.path.insert(0, str(ROOT / "SmartCar/tests"))
    spec = importlib.util.spec_from_file_location("spi_test", ROOT / "SmartCar/tests/spi_wifi_state.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    source = (ROOT / "SmartCar/BSP/bsp_wifi.c").read_text(encoding="utf-8")
    startup = "    if (s_status.permits)\n    {\n        __HAL_GPIO_EXTI_CLEAR_IT(WIFI_INT_Pin);\n    }\n"
    dma = "    tx_stopped = BSP_WiFi_AbortDma(hspi2.hdmatx);\n    rx_stopped = BSP_WiFi_AbortDma(hspi2.hdmarx);\n"
    assert source.count(startup) == source.count(dma) == 1
    old_dma = source.replace(dma, "    tx_stopped = 1;\n    rx_stopped = 1;\n", 1)
    start = old_dma.index("static uint8_t BSP_WiFi_AbortDma(")
    end = old_dma.index("/**", start)
    old_dma = old_dma[:start] + old_dma[end:]
    cases = [("current", source, None),
             ("without_startup_dedup", source.replace(startup, "", 1), "s_status.permits==expected"),
             ("without_dma_reclaim", old_dma, "tx_dma.State")]
    summary = {"kind": "alternate Windows Clang host, not GCC or board",
               "source_sha256": hashlib.sha256(source.encode()).hexdigest(), "cases": []}
    for label, compiler in [("clang", CLANG), ("lld", LLD)]:
        result = subprocess.run([str(compiler), "--version"], capture_output=True, text=True)
        summary[label] = result.stdout + result.stderr
    for name, candidate, expected in cases:
        work = HERE / name
        work.mkdir(exist_ok=True)
        files = {"bsp_wifi.c": candidate, "bsp.h": module.BSP, "test.c": module.HARNESS,
                 "assert.h": '#pragma once\nvoid test_fail(const char *);\n#define assert(x) ((x)?(void)0:test_fail(#x))\n',
                 "stdio.h": '#pragma once\nint puts(const char *);\n',
                 "string.h": '#pragma once\n#include <stddef.h>\nvoid *memset(void *, int, size_t);\n',
                 "shim.c": '''#include <stddef.h>
typedef void (*reporter_t)(const char *, int);
static reporter_t reporter;
void test_set_reporter(reporter_t value) { reporter=value; }
void test_fail(const char *expr) { reporter(expr,1); for (;;) {} }
int puts(const char *message) { reporter(message,0); return 0; }
void *memset(void *dest,int value,size_t size)
{ unsigned char *p=dest; for(size_t i=0;i<size;++i)p[i]=(unsigned char)value; return dest; }
'''}
        for filename, content in files.items():
            (work / filename).write_text(content, encoding="utf-8")
        commands = []
        for unit in ["test", "shim"]:
            commands.append([str(CLANG), "--target=x86_64-pc-windows-msvc", "-std=c99", "-O2",
                             "-Wall", "-Wextra", "-Werror", "-ffreestanding", "-fno-builtin",
                             "-fno-stack-protector", "-I", str(work), "-I", str(ROOT / "SmartCar/BSP"),
                             "-c", str(work / (unit + ".c")), "-o", str(work / (unit + ".obj"))])
        dll = work / "test.dll"
        commands.append([str(LLD), "-flavor", "link", "/dll", "/noentry", "/nodefaultlib",
                         "/machine:x64", "/export:main", "/export:test_set_reporter",
                         "/out:" + str(dll), str(work / "test.obj"), str(work / "shim.obj")])
        logs = []
        for command in commands:
            result = subprocess.run(command, capture_output=True, text=True)
            logs.append({"command": command, "returncode": result.returncode,
                         "stdout": result.stdout, "stderr": result.stderr})
            (work / "build.json").write_text(json.dumps(logs, indent=2), encoding="utf-8")
            assert result.returncode == 0, logs[-1]
        result = subprocess.run([sys.executable, str(Path(__file__).resolve()), "--child", str(dll)],
                                capture_output=True, text=True)
        passed = (result.returncode == 0 if expected is None
                  else result.returncode == 1 and expected in result.stderr)
        entry = {"name": name, "returncode": result.returncode, "expected_assertion": expected,
                 "stdout": result.stdout, "stderr": result.stderr, "passed": passed}
        summary["cases"].append(entry)
        (HERE / "clang-host-summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
        print(json.dumps(entry), flush=True)
        assert passed, entry


if __name__ == "__main__":
    if len(sys.argv) == 3 and sys.argv[1] == "--child":
        raise SystemExit(child(Path(sys.argv[2])))
    main()
