"""Cross-check the real C codec against immutable vectors and Python zlib/JSON."""
from pathlib import Path
import json
import struct
import subprocess
import tempfile
import zlib
import random
from host_support import ROOT, build

HARNESS = r'''
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "scb_protocol.h"
static unsigned char input[2048], output[1024];
static scb_json_t document;
int main(int argc, char **argv)
{
    scb_frame_t frame;
    scb_command_t command;
    scb_status_t status;
    FILE *file;
    size_t length;
    if (argc != 3 || !(file = fopen(argv[2], "rb"))) return 20;
    length = fread(input, 1, sizeof input, file);
    fclose(file);
    if (argv[1][0] == 'F')
    {
        float value;
        scb_writer_t writer;
        unsigned char guard[4] = {1,2,3,4};
        SCB_JsonBegin(&writer, guard, 4);
        SCB_JsonAppend(&writer, "12345");
        assert(writer.failed && !writer.length && guard[0] == 1 && guard[3] == 4);
        if (length != 4) return 22;
        memcpy(&value, input, 4);
        SCB_JsonBegin(&writer, output, 992);
        SCB_JsonFloat(&writer, value);
        if (writer.failed) return 1;
        fwrite(output, 1, writer.length, stdout);
        return 0;
    }
    if (argv[1][0] == 'J')
    {
        if (!SCB_ParseJson(input, (uint16_t)length, &document)) return 1;
        return 0;
    }
    if (argv[1][0] == 'C')
    {
        if (!SCB_ParseCommand(input, (uint16_t)length, &command, &status)) return 1;
        printf("%s %s %u %u %.9g %ld", command.op, SCB_StatusName(status),
            command.mode, command.gear, (double)command.values.kp, (long)command.values.base_speed);
        return 0;
    }
    if (!SCB_DecodeFrame(input, length, &frame)) return 1;
    printf("%u %u %u %u %lu %lu %lu %lu", frame.type, frame.flags,
        frame.payload_len, frame.record_count, (unsigned long)frame.session_id,
        (unsigned long)frame.seq, (unsigned long)frame.ack_seq, (unsigned long)frame.request_id);
    if (!SCB_EncodeFrame(output, &frame)) return 2;
    file = fopen(argv[1], "wb");
    if (!file) return 21;
    fwrite(output, 1, sizeof output, file);
    fclose(file);
    return 0;
}
'''


def main():
    vectors = json.loads((Path(__file__).parent / "scb_vectors.json").read_text())
    checks = 0
    with tempfile.TemporaryDirectory(prefix="scb_protocol_") as temp:
        work = Path(temp)
        (work / "test.c").write_text(HARNESS)
        binary = build(work, [work / "test.c", ROOT / "App/scb_protocol.c"])
        source = work / "input.bin"
        output = work / "output.bin"

        def run(mode, data, valid=True):
            nonlocal checks
            source.write_bytes(data)
            result = subprocess.run([str(binary), mode, str(source)], capture_output=True)
            assert (result.returncode == 0) == valid, (mode, data[:120], result)
            checks += 1
            return result.stdout

        assert struct.calcsize("<4sBBHHHIIIII") == 32
        for name, entries in vectors.items():
            for entry in entries if isinstance(entries, list) else [entries]:
                data = bytes.fromhex(entry)
                valid = not name.startswith("BAD_")
                decoded = run(str(output), data, valid)
                if valid:
                    assert output.read_bytes() == data.ljust(1024, b"\0")
                    header = struct.unpack("<4sBBHHHIIIII", data[:32])
                    assert list(map(int, decoded.split())) == list(header[2:10])
                    assert zlib.crc32(data[32:], zlib.crc32(data[:28])) == header[-1]
                    if header[2] == 0x10:
                        parsed = json.loads(data[32:])
                        assert parsed["op"].encode() in run("C", data[32:])
        base = bytearray(bytes.fromhex(vectors["GET_STATUS_HEX"]))

        def crc(frame):
            frame[28:32] = struct.pack("<I", zlib.crc32(frame[32:], zlib.crc32(frame[:28])))
            return bytes(frame)

        for offset, value in [(0, 0), (4, 2), (5, 0xff), (6, 3), (10, 1)]:
            frame = base.copy()
            frame[offset] = value
            run(str(output), crc(frame), False)
        for length in [0, 991, 992, 993]:
            frame = bytearray(base[:32])
            frame[5] = 0x11
            frame[8:10] = struct.pack("<H", length)
            frame.extend(b"x" * length)
            run(str(output), crc(frame), length <= 992)
        valid_json = [b'{}', b'{"op":"GET_STATUS","args":{}}',
                      b'{"a":"\\uD83D\\uDE00"}', '{"中文":"值"}'.encode(),
                      b'{"a":[true,false,null,-1.2e+3]}', b'"' + b'x' * 990 + b'"',
                      b'[' * 8 + b'0' + b']' * 8, b'[' + b'0,' * 126 + b'0]',
                      b'1' + b'0' * 62]
        for data in valid_json:
            json.loads(data)
            run("J", data)
        bad_json = [b'{"a":1,"a":2}', b'{"a":1,"\\u0061":2}', b'{"x":NaN}',
                    b'{"x":Infinity}', b'{"x":1e999}', b'{"x":01}', b'{"x":1.}',
                    b'{"x":1e}', b'{"x":+1}', b'{"x":"\\uD800"}', b'{"x":"\\uDC00"}',
                    b'{"x":"\\u0000"}', b'{"x":"\xc0\x80"}', b'{"x":"\xed\xa0\x80"}',
                    b'{"x":"\xf4\x90\x80\x80"}', b'{"x":1,}', b'{}{}', b'{"x":',
                    b'[[[[[[[[[[0]]]]]]]]]]', b'[' + b'0,' * 130 + b'0]',
                    b'[' * 9 + b'0' + b']' * 9, b'[' + b'0,' * 127 + b'0]',
                    b'1' + b'0' * 63,
                    b'"' + b'x' * 991 + b'"', b'{"x":"\0"}']
        for data in bad_json:
            run("J", data, False)
        values = dict(kp=2, k2p=.008, kd=15, kp_v=20, ki_v=.75,
                      base_speed=180, fan_run=1600, fan_idle=1100)
        for field in values:
            args = dict(mode=1, gear=0, values={k: v for k, v in values.items() if k != field})
            assert b"INVALID_ARG" in run("C", json.dumps(dict(op="SET_PARAMS", args=args)).encode())
        for field, value, expected in [("kp", 101, b"OUT_OF_RANGE"),
                                       ("base_speed", 1.5, b"INVALID_ARG"),
                                       ("base_speed", 301, b"OUT_OF_RANGE"),
                                       ("fan_run", -1, b"OUT_OF_RANGE")]:
            args = dict(mode=0, gear=1, values={**values, field: value})
            assert expected in run("C", json.dumps(dict(op="SET_PARAMS", args=args)).encode())
        randomizer = random.Random(16)
        samples = [0, 1, 0x80000000, 0x80000001, 0x7f7fffff, 0xff7fffff, 0x3c03126f]
        samples += [randomizer.getrandbits(32) for _ in range(512)]
        for bits in samples:
            data = struct.pack("<I", bits)
            finite = bits & 0x7f800000 != 0x7f800000
            rendered = run("F", data, finite)
            if finite:
                value = json.loads(rendered)
                assert struct.pack("<f", value) == data or bits == 0x80000000, (hex(bits), rendered)
    print(f"PASS V-04/05 real C codec/JSON: {checks} cases; immutable vectors, independent zlib")


if __name__ == "__main__":
    main()
