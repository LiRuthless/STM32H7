"""Compile exact protocol sources; no SDK mocks or replacement JSON logic."""
import ctypes
import json
import os
from pathlib import Path
import secrets
import subprocess

ROOT = Path(__file__).resolve().parents[1]


class Header(ctypes.Structure):
    _fields_ = [("type", ctypes.c_uint8), ("flags", ctypes.c_uint16),
                ("payload_len", ctypes.c_uint16), ("record_count", ctypes.c_uint16),
                *[(name, ctypes.c_uint32) for name in ("session_id", "seq", "ack_seq", "request_id")]]


class Frame(ctypes.Structure):
    _fields_ = [("header", Header), ("payload", ctypes.c_void_p)]


class View(ctypes.Structure):
    _fields_ = [("bytes", ctypes.c_void_p), ("length", ctypes.c_size_t)]


class Workspace(ctypes.Structure):
    _fields_ = [("object_start", ctypes.c_uint16 * 497), ("state", ctypes.c_uint8 * 497)]


_library = None


def load():
    global _library
    if _library is not None:
        return _library
    compiler = Path(os.environ.get("BRIDGE_HOST_CLANG", "C:/Program Files/MATLAB/R2025a/toolbox/sldrt/clang/win64/clang.exe"))
    linker = Path(os.environ.get("BRIDGE_HOST_LLD", "C:/Espressif/tools/esp-clang/esp-20.1.1_20250829/esp-clang/bin/ld.lld.exe"))
    if os.name != "nt" or not compiler.is_file() or not linker.is_file():
        raise RuntimeError("Existing Windows native compiler/linker required; nothing will be installed")
    output = ROOT / ".local/p2-host" / ("run-" + secrets.token_hex(8))
    output.mkdir(parents=True)
    commands = []

    def run(args):
        commands.append(list(map(str, args)))
        result = subprocess.run(args, capture_output=True, text=True, timeout=60)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)

    support = output / "support.c"
    support.write_text(
        '#include <stddef.h>\n'
        'void *memset(void *p,int c,size_t n){unsigned char *d=p;for(size_t i=0;i<n;++i)d[i]=(unsigned char)c;return p;}\n'
        'void *memcpy(void *p,const void *s,size_t n){unsigned char *d=p;const unsigned char *b=s;'
        'for(size_t i=0;i<n;++i)d[i]=b[i];return p;}\n', encoding="ascii")
    objects = []
    for source in (ROOT / "main/protocol.c", ROOT / "main/json_guard.c", support):
        obj = output / (source.stem + ".obj")
        run([str(compiler), "--target=x86_64-pc-windows-msvc", "-std=c11", "-O1", "-Wall", "-Wextra", "-Werror",
             "-ffreestanding", "-fno-builtin", "-fno-stack-protector", "-I", str(ROOT / "main"),
             "-c", str(source), "-o", str(obj)])
        objects.append(str(obj))
    dll = output / "protocol.dll"
    run([str(linker), "-flavor", "link", "/dll", "/noentry", "/nodefaultlib", "/machine:x64", "/out:" + str(dll),
         "/export:bridge_json_validate", "/export:bridge_json_object_get", "/export:scb_encode", "/export:scb_decode",
         *objects])
    lib = ctypes.CDLL(str(dll))
    lib.bridge_json_validate.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(Workspace)]
    lib.bridge_json_validate.restype = ctypes.c_int
    lib.bridge_json_object_get.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_char_p,
                                          ctypes.POINTER(Workspace), ctypes.POINTER(View)]
    lib.bridge_json_object_get.restype = ctypes.c_bool
    lib.scb_decode.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_int,
                              ctypes.POINTER(Workspace), ctypes.POINTER(Frame)]
    lib.scb_decode.restype = ctypes.c_int
    lib.scb_encode.argtypes = [ctypes.POINTER(Header), ctypes.c_void_p, ctypes.c_int, ctypes.c_void_p,
                              ctypes.c_size_t, ctypes.POINTER(Workspace), ctypes.POINTER(ctypes.c_size_t)]
    lib.scb_encode.restype = ctypes.c_int
    (output / "compile-evidence.json").write_text(json.dumps({
        "commands": commands, "compiler_version": subprocess.check_output([str(compiler), "--version"], text=True),
        "linker_version": subprocess.check_output([str(linker), "--version"], text=True),
        "sources": ["main/protocol.c", "main/json_guard.c"], "number_validation": "integer-only exact binary64 overflow boundary",
        "limitations": "native Windows host execution; not ESP32 runtime or device memory evidence",
    }, indent=2) + "\n", encoding="utf-8")
    (ROOT / ".local/p2-host-latest.txt").write_text(str(output), encoding="utf-8")
    _library = lib
    return lib
