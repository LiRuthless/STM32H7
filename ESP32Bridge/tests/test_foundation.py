"""Compile production C and exercise it in a native Windows DLL, without CRT.

The SDK/allocator/GPIO boundary is replaced explicitly by host fixtures.
No host result is a device startup, DMA capability or heap measurement.
"""

import ctypes
import json
import os
from pathlib import Path
import secrets
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[1]
CLANG = Path(os.environ.get("BRIDGE_HOST_CLANG", "C:/Program Files/MATLAB/R2025a/toolbox/sldrt/clang/win64/clang.exe"))
LINKER = Path(os.environ.get("BRIDGE_HOST_LLD", "C:/Espressif/tools/esp-clang/esp-20.1.1_20250829/esp-clang/bin/ld.lld.exe"))
CASES = {
    0: "pool_capacity_and_initialization", 1: "allocation_failure_1", 2: "allocation_failure_2",
    3: "allocation_failure_3", 4: "allocation_failure_4", 5: "misalignment_each_block",
    6: "alias_rollback_no_double_free", 7: "all_slots_backpressure_peaks_and_forged_handles",
    8: "dma_ownership_and_refused_destroy", 9: "stale_handles_after_reinitialization",
    10: "pool_invalid_arguments", 11: "two_thousand_cycles_no_allocations", 12: "password_all_byte_values",
    20: "startup_normal_order", 21: "startup_nvs_failure", 22: "startup_netif_failure",
    23: "startup_event_failure", 24: "startup_ap_failure", 25: "startup_socket_failure",
    26: "startup_spi_failure", 27: "startup_missing_ap", 28: "startup_missing_nvs",
    29: "invalid_password_does_not_block_spi", 30: "ready_guard_does_not_block_network",
    31: "pool_failure_blocks_all_services", 32: "startup_invalid_arguments",
    33: "independent_nvs_and_spi_failures", 34: "real_spi_error_not_masked_by_missing_ap",
    35: "first_precondition_error_preserved_when_pool_also_fails",
    40: "idf_adapter_real_order_and_spi_callback", 41: "idf_adapter_nvs_failure",
    42: "idf_adapter_netif_failure", 43: "idf_adapter_event_failure", 44: "idf_adapter_latch_failure",
    45: "idf_adapter_pad_failure", 46: "idf_adapter_internal_endpoint_failure",
    47: "idf_adapter_dma_endpoint_failure", 48: "idf_adapter_non_dma_internal_endpoint_failure",
    49: "idf_adapter_fourth_allocation_failure", 50: "actual_app_main_diagnostics",
    51: "actual_app_main_failure_diagnostics", 60: "actual_invalid_compiled_password",
    99: "generation_exhaustion_never_wraps",
}


class FoundationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if os.name != "nt" or not CLANG.is_file() or not LINKER.is_file():
            raise RuntimeError("Existing Windows host-capable Clang is required; no tool will be installed")
        cls.output = ROOT / ".local/g2-host" / ("run-" + secrets.token_hex(4))
        cls.output.mkdir(parents=True)
        cls.commands = []
        cls.credentials = {"valid": secrets.token_hex(12), "invalid": secrets.token_hex(4)[:7]}
        cls.libraries = {}
        for profile, credential in cls.credentials.items():
            directory = cls.output / profile
            directory.mkdir()
            (directory / "sdkconfig.h").write_text(
                '#define CONFIG_IDF_TARGET_ESP32S3 1\n#define CONFIG_BRIDGE_AP_PASSWORD "' + credential + '"\n',
                encoding="ascii",
            )
            sources = [ROOT / "tests/host/foundation_harness.c"] + [ROOT / "main" / name for name in
                       ("startup.c", "startup_idf.c", "app_main.c")]
            objects = []
            for source in sources:
                obj = directory / (source.stem + ".obj")
                cls.run_tool([str(CLANG), "--target=x86_64-pc-windows-msvc", "-std=c11", "-O1",
                              "-Wall", "-Wextra", "-Werror", "-ffreestanding", "-fno-builtin", "-fno-stack-protector",
                              "-I", str(directory), "-I", str(ROOT / "tests/host/include"), "-I", str(ROOT / "main"),
                              "-c", str(source), "-o", str(obj)])
                objects.append(str(obj))
            dll = directory / "foundation.dll"
            cls.run_tool([str(LINKER), "-flavor", "link", "/dll", "/noentry", "/nodefaultlib",
                          "/machine:x64", "/out:" + str(dll), "/export:bridge_test_run", "/export:bridge_test_log", *objects])
            library = ctypes.CDLL(str(dll))
            library.bridge_test_run.argtypes = [ctypes.c_int]
            library.bridge_test_run.restype = ctypes.c_int
            library.bridge_test_log.restype = ctypes.c_char_p
            cls.libraries[profile] = library
        version = subprocess.run([str(CLANG), "--version"], capture_output=True, text=True, check=True).stdout
        (cls.output / "compile-evidence.json").write_text(json.dumps({
            "compiler": str(CLANG), "version": version, "target": "x86_64-pc-windows-msvc",
            "commands": cls.commands, "production_sources": ["buffer_pool.c", "startup.c", "startup_idf.c", "app_main.c"],
            "scenario_count": len(CASES), "credentials": "omitted; disposable host fixtures only",
        }, indent=2) + "\n", encoding="utf-8")
        (ROOT / ".local/g2-host-latest.txt").write_text(str(cls.output), encoding="utf-8")

    @classmethod
    def run_tool(cls, args):
        cls.commands.append(args)
        result = subprocess.run(args, capture_output=True, text=True, timeout=60)
        if result.returncode:
            diagnostic = result.stdout + result.stderr
            for credential in cls.credentials.values():
                diagnostic = diagnostic.replace(credential, "<redacted>")
            raise RuntimeError("Native C build failed:\n" + diagnostic)


def make_test(scenario):
    def check(self):
        profile = "invalid" if scenario == 60 else "valid"
        library = self.libraries[profile]
        line = library.bridge_test_run(scenario)
        self.assertEqual(line, 0, "C scenario failed at foundation_harness.c line " + str(line))
        diagnostics = library.bridge_test_log() or b""
        for value in self.credentials.values():
            self.assertTrue(value.encode("ascii") not in diagnostics, "Credential disclosed by production diagnostics")
        if scenario == 50:
            self.assertIn(b"Stage=NVS state=SUCCEEDED", diagnostics)
            self.assertIn(b"Stage=AP state=UNIMPLEMENTED", diagnostics)
            self.assertIn(b"Stage=SOCKETS state=BLOCKED", diagnostics)
            self.assertIn(b"Stage=SPI state=SUCCEEDED", diagnostics)
        if scenario == 51:
            self.assertIn(b"Stage=NVS state=FAILED", diagnostics)
    return check


for number, label in CASES.items():
    setattr(FoundationTests, "test_" + str(number).zfill(2) + "_" + label, make_test(number))

if __name__ == "__main__":
    unittest.main()
