"""Bindings/compiler only. Network task, codec, session and reliability run in C.

The socket/Wi-Fi/netif boundary is an explicit fixture driven by a script of
poll iterations; Python supplies bytes/timing inputs and checks results. No
network behavior is reimplemented in Python.
"""
import ctypes as C
import json
import os
from pathlib import Path
import secrets
import subprocess

ROOT = Path(__file__).resolve().parents[1]
_library = None


class Snapshot(C.Structure):
    _fields_ = [('init_error', C.c_int), ('service_error', C.c_int),
        *[(name, C.c_uint32) for name in (
            'connected', 'rejected', 'received', 'crc_errors', 'invalid_closes',
            'sent_bytes', 'short_writes', 'stalls', 'link_failures',
            'subscriptions', 'udp_sent', 'udp_failed', 'udp_dropped',
            'backpressure', 'session_closes', 'stack_free_min', 'peer_ipv4')],
        ('udp_port', C.c_uint16),
        ('listening', C.c_bool), ('control_open', C.c_bool), ('subscribed', C.c_bool)]


# Script entry kinds, mirroring net_harness.c.
IDLE, ACCEPT, ACCEPT_EAGAIN = 0, 1, 2
RECV, RECV_EOF, RECV_EAGAIN, RECV_ERROR = 3, 4, 5, 6
SEND, SEND_EAGAIN, SEND_ZERO, SEND_ERROR = 7, 8, 9, 10
BUSY_SEND, BUSY_SEND_EAGAIN, BUSY_SEND_ZERO, BUSY_SEND_ERROR = 11, 12, 13, 14
OFFER, SET_SPI, UDP_FAIL = 15, 16, 17
UNBIND = 18


class Script:
    """One entry per network-task poll iteration; empty entry advances the
    injected clock by the bounded timeout the production loop requested."""

    def __init__(self):
        self.entries = []

    def push(self, kind, arg=0, data=b''):
        self.entries.append((kind, arg, bytes(data)))
        return self

    def idle(self, count=1):
        for _ in range(count):
            self.push(IDLE)
        return self

    def accept(self, ip=0xC0A80464):
        return self.push(ACCEPT, ip)

    def recv(self, data):
        return self.push(RECV, 0, data)

    def send(self, cap=1024):
        return self.push(SEND, cap)

    def offer(self, frame):
        return self.push(OFFER, 0, frame)

    def set_spi(self, session, ready):
        return self.push(SET_SPI, session, b'\x01' if ready else b'\x00')


def load():
    global _library
    if _library is not None:
        return _library
    compiler = Path(os.environ.get('BRIDGE_HOST_CLANG', 'C:/Program Files/MATLAB/R2025a/toolbox/sldrt/clang/win64/clang.exe'))
    linker = Path(os.environ.get('BRIDGE_HOST_LLD', 'C:/Espressif/tools/esp-clang/esp-20.1.1_20250829/esp-clang/bin/ld.lld.exe'))
    if os.name != 'nt' or not compiler.is_file() or not linker.is_file():
        raise RuntimeError('Existing Windows native compiler/linker required; nothing will be installed')
    output = ROOT / '.local/p4-host' / ('run-' + secrets.token_hex(8))
    output.mkdir(parents=True)
    (output / 'sdkconfig.h').write_text('#define CONFIG_BRIDGE_AP_PASSWORD "hostFixturePw1"\n', encoding='ascii')
    sources = [ROOT / 'tests/host/net_harness.c'] + [ROOT / 'main' / (name + '.c') for name in
        ('protocol', 'json_guard', 'reliable_link', 'link_session')]
    objects, commands = [], []
    for source in sources:
        obj = output / (source.stem + '.obj')
        commands.append([str(compiler), '--target=x86_64-pc-windows-msvc', '-std=c11', '-O1', '-Wall', '-Wextra', '-Werror',
            '-ffreestanding', '-fno-builtin', '-fno-stack-protector', '-I', str(output),
            '-I', str(ROOT / 'tests/host/include'), '-I', str(ROOT / 'main'), '-c', str(source), '-o', str(obj)])
        objects.append(str(obj))
    dll = output / 'net.dll'
    exports = ('bridge_net_run', 'bridge_net_ap_test', 'bridge_net_start_fail_test', 'bridge_net_generation_test',
        'bridge_net_snapshot', 'bridge_net_snapshot_size', 'bridge_net_metric', 'bridge_net_capture',
        'bridge_net_busy_capture', 'bridge_net_udp_get', 'bridge_net_offer_result', 'bridge_net_close_event',
        'bridge_net_log', 'bridge_net_telemetry_offer', 'fixture_reset', 'fixture_spi', 'fixture_push')
    commands.append([str(linker), '-flavor', 'link', '/dll', '/noentry', '/nodefaultlib', '/machine:x64',
                    '/out:' + str(dll), *['/export:' + name for name in exports], *objects])
    for args in commands:
        result = subprocess.run(args, capture_output=True, text=True, timeout=60)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
    (output / 'compile-evidence.json').write_text(json.dumps(dict(commands=commands,
        production_sources=['main/net_bridge.c (included by harness)',
                            *['main/' + p.name for p in sources[1:]]],
        test_password='hostFixturePw1 (disposable host fixture, not a deployment credential)',
        limitations='Native Windows scripted socket/Wi-Fi fixtures; exact production task loop/codec/session/reliable; not RF, DHCP, RTOS or network evidence'), indent=2) + '\n')
    (ROOT / '.local/p4-host-latest.txt').write_text(str(output))
    lib = C.CDLL(str(dll))
    lib.bridge_net_snapshot_size.restype = C.c_size_t
    assert lib.bridge_net_snapshot_size() == C.sizeof(Snapshot)
    lib.bridge_net_run.restype = C.c_int
    lib.bridge_net_ap_test.argtypes = [C.c_int]
    lib.bridge_net_ap_test.restype = C.c_int
    lib.bridge_net_start_fail_test.argtypes = [C.c_int]
    lib.bridge_net_start_fail_test.restype = C.c_int
    lib.bridge_net_generation_test.restype = C.c_int
    lib.bridge_net_snapshot.argtypes = [C.POINTER(Snapshot)]
    lib.bridge_net_metric.argtypes = [C.c_uint]
    lib.bridge_net_metric.restype = C.c_uint
    lib.bridge_net_capture.argtypes = [C.c_void_p, C.c_size_t]
    lib.bridge_net_capture.restype = C.c_size_t
    lib.bridge_net_busy_capture.argtypes = [C.c_void_p, C.c_size_t]
    lib.bridge_net_busy_capture.restype = C.c_size_t
    lib.bridge_net_udp_get.argtypes = [C.c_uint, C.c_void_p, C.c_size_t, C.POINTER(C.c_uint32), C.POINTER(C.c_uint16)]
    lib.bridge_net_udp_get.restype = C.c_size_t
    lib.bridge_net_offer_result.argtypes = [C.c_uint]
    lib.bridge_net_offer_result.restype = C.c_int
    lib.bridge_net_close_event.argtypes = [C.c_uint]
    lib.bridge_net_close_event.restype = C.c_int
    lib.bridge_net_log.restype = C.c_char_p
    lib.bridge_net_telemetry_offer.argtypes = [C.c_void_p, C.c_size_t]
    lib.bridge_net_telemetry_offer.restype = C.c_int
    lib.fixture_spi.argtypes = [C.c_uint32, C.c_int]
    lib.fixture_push.argtypes = [C.c_uint32, C.c_uint32, C.c_void_p, C.c_uint32]
    _library = lib
    return lib


def run(lib, script, spi_session=0x12345678, spi_ready=True):
    """Drive the production network task through one scripted session."""
    lib.fixture_reset()
    lib.fixture_spi(spi_session, int(spi_ready))
    for kind, arg, data in script.entries:
        lib.fixture_push(kind, arg, data, len(data))
    line = lib.bridge_net_run()
    assert line == 0, 'C network fixture assertion at net_harness.c line ' + str(line)
    assert b'hostFixturePw1' not in (lib.bridge_net_log() or b''), 'AP password leaked into network log'
    snapshot = Snapshot()
    assert lib.bridge_net_snapshot(C.byref(snapshot)) == 0
    return snapshot


def captured(lib, busy=False):
    out = C.create_string_buffer(65536)
    size = (lib.bridge_net_busy_capture if busy else lib.bridge_net_capture)(out, 65536)
    return bytes(out[:size])


def datagrams(lib):
    result = []
    for index in range(lib.bridge_net_metric(5)):
        out = C.create_string_buffer(1100)
        ip, port = C.c_uint32(), C.c_uint16()
        size = lib.bridge_net_udp_get(index, out, 1100, C.byref(ip), C.byref(port))
        assert size
        result.append((ip.value, port.value, bytes(out[:size])))
    return result
