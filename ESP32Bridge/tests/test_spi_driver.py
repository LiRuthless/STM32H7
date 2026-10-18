"""Actual production SPI adapter and pool vs explicit host SDK boundary fixtures."""
import ctypes
import json
import os
from pathlib import Path
import secrets
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[1]


class SpiDriverTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = Path(os.environ.get('BRIDGE_HOST_CLANG', 'C:/Program Files/MATLAB/R2025a/toolbox/sldrt/clang/win64/clang.exe'))
        linker = Path(os.environ.get('BRIDGE_HOST_LLD', 'C:/Espressif/tools/esp-clang/esp-20.1.1_20250829/esp-clang/bin/ld.lld.exe'))
        output = ROOT / '.local/p3-host' / ('run-' + secrets.token_hex(8))
        output.mkdir(parents=True)
        obj, dll = output / 'spi.obj', output / 'spi.dll'
        commands = [[str(compiler), '--target=x86_64-pc-windows-msvc', '-std=c11', '-O1', '-Wall', '-Wextra', '-Werror',
            '-ffreestanding', '-fno-builtin', '-fno-stack-protector', '-I', str(ROOT / 'tests/host/include'),
            '-I', str(ROOT / 'main'), '-c', str(ROOT / 'tests/host/spi_driver_harness.c'), '-o', str(obj)],
            [str(linker), '-flavor', 'link', '/dll', '/noentry', '/nodefaultlib', '/machine:x64',
             '/out:' + str(dll), '/export:bridge_spi_driver_test', str(obj)]]
        for args in commands:
            result = subprocess.run(args, capture_output=True, text=True, timeout=60)
            if result.returncode:
                raise RuntimeError(result.stdout + result.stderr)
        (output / 'compile-evidence.json').write_text(json.dumps(dict(commands=commands,
            production_sources=['main/spi_bridge.c', 'main/buffer_pool.c'],
            limitations='Windows host task/driver boundary simulation; not real RTOS, DMA or electrical measurements'), indent=2)+'\n')
        (ROOT / '.local/p3-host-latest.txt').write_text(str(output))
        cls.lib = ctypes.CDLL(str(dll))
        cls.lib.bridge_spi_driver_test.argtypes = [ctypes.c_int]
        cls.lib.bridge_spi_driver_test.restype = ctypes.c_int


def scenario(number):
    def run(self):
        result = self.lib.bridge_spi_driver_test(number)
        self.assertEqual(result, 0, 'production SPI scenario failed at C harness line ' + str(result))
    return run


for number, name in enumerate(('driver_pins_mode_affinity_and_dma', 'task_creation_failure', 'semaphore_creation_failure',
                              'wrong_core', 'missing_pool', 'dma_already_owned', 'driver_failure_releases_dma',
                              'duplicate_start_rejected')):
    setattr(SpiDriverTests, 'test_' + name, scenario(number))
