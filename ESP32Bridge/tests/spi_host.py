"""Bindings/compiler only. Task, codec, session, reliability and pool execute C."""
import ctypes as C
import json
import os
from pathlib import Path
import secrets
import subprocess

ROOT = Path(__file__).resolve().parents[1]
_library = None


class ProtocolStats(C.Structure):
    _fields_ = [(name, C.c_uint32) for name in ('received', 'crc_errors', 'invalid', 'duplicates', 'backpressure',
        'delivered', 'telemetry_packets', 'telemetry_records', 'telemetry_dropped', 'records_dropped',
        'retries', 'rebuilds', 'rng_failures', 'invalid_outbound')]


class Snapshot(C.Structure):
    _fields_ = [('init_error', C.c_int), ('service_error', C.c_int),
        *[(name, C.c_uint32) for name in ('queued', 'completed', 'truncated', 'overlength', 'wait_timeouts', 'session_id', 'stack_free_min')],
        ('link_ready', C.c_bool), ('driver_owned', C.c_bool), ('protocol', ProtocolStats)]


def load():
    global _library
    if _library is not None:
        return _library
    compiler = Path(os.environ.get('BRIDGE_HOST_CLANG', 'C:/Program Files/MATLAB/R2025a/toolbox/sldrt/clang/win64/clang.exe'))
    linker = Path(os.environ.get('BRIDGE_HOST_LLD', 'C:/Espressif/tools/esp-clang/esp-20.1.1_20250829/esp-clang/bin/ld.lld.exe'))
    output = ROOT / '.local/p3-host' / ('run-' + secrets.token_hex(8))
    output.mkdir(parents=True)
    sources = [ROOT / 'tests/host/spi_driver_harness.c'] + [ROOT / 'main' / (name + '.c') for name in
        ('spi_protocol', 'protocol', 'json_guard', 'reliable_link', 'link_session')]
    objects, commands = [], []
    for source in sources:
        obj = output / (source.stem + '.obj')
        commands.append([str(compiler), '--target=x86_64-pc-windows-msvc', '-std=c11', '-O1', '-Wall', '-Wextra', '-Werror',
            '-ffreestanding', '-fno-builtin', '-fno-stack-protector', '-I', str(ROOT / 'tests/host/include'),
            '-I', str(ROOT / 'main'), '-c', str(source), '-o', str(obj)])
        objects.append(str(obj))
    dll = output / 'spi.dll'
    exports = ('bridge_spi_driver_test', 'bridge_spi_run_trace', 'bridge_spi_trace_size', 'bridge_spi_trace_metric',
        'bridge_spi_trace_ready', 'bridge_spi_trace_capture', 'bridge_spi_trace_commit', 'bridge_spi_snapshot', 'bridge_spi_port_check_test')
    commands.append([str(linker), '-flavor', 'link', '/dll', '/noentry', '/nodefaultlib', '/machine:x64',
                    '/out:' + str(dll), *['/export:' + name for name in exports], *objects])
    for args in commands:
        result = subprocess.run(args, capture_output=True, text=True, timeout=60)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
    (output / 'compile-evidence.json').write_text(json.dumps(dict(commands=commands,
        production_sources=['main/spi_bridge.c', 'main/buffer_pool.c', *['main/' + p.name for p in sources[1:]]],
        limitations='Native Windows mocked task/driver boundary; exact production task loop/codec/session/reliable/pool; not RTOS or electrical evidence'), indent=2)+'\n')
    (ROOT / '.local/p3-host-latest.txt').write_text(str(output))
    lib = C.CDLL(str(dll))
    lib.bridge_spi_driver_test.argtypes = [C.c_int]
    lib.bridge_spi_run_trace.argtypes = [C.c_uint, C.c_void_p, C.c_uint, C.POINTER(C.c_uint32), C.POINTER(C.c_uint32), C.c_uint, C.c_uint32, C.c_int]
    lib.bridge_spi_trace_size.restype = C.c_size_t
    assert lib.bridge_spi_trace_size() == C.sizeof(Snapshot)
    lib.bridge_spi_trace_metric.argtypes = [C.c_uint]
    lib.bridge_spi_trace_metric.restype = C.c_uint
    lib.bridge_spi_trace_ready.argtypes = [C.c_uint]
    lib.bridge_spi_trace_ready.restype = C.c_uint
    lib.bridge_spi_trace_capture.argtypes = [C.c_uint, C.c_void_p, C.c_size_t]
    lib.bridge_spi_trace_capture.restype = C.c_size_t
    lib.bridge_spi_trace_commit.argtypes = [C.c_uint, C.c_void_p, C.c_size_t, C.POINTER(C.c_uint32)]
    lib.bridge_spi_trace_commit.restype = C.c_size_t
    lib.bridge_spi_snapshot.argtypes = [C.POINTER(Snapshot)]
    _library = lib
    return lib
