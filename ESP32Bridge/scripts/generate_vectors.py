"""Deterministic SCB/1 LOCAL CANDIDATE vectors. No credentials or hardware I/O."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
import zlib

ROOT = Path(__file__).resolve().parents[1]
WIRE = struct.Struct('<4sBBHHHIIIII')
assert WIRE.size == 32
SESSION = 0x12345678


def generate():
    vectors = []

    def add(name, kind, payload=b'', *, reliable=False, count=0, seq=1, request=0,
            session=SESSION, declared=None, corrupt=False, error='OK', note=''):
        length = len(payload) if declared is None else declared
        h = WIRE.pack(b'SCB1', 1, kind, int(reliable), length, count, session, seq, 0, request, 0)
        crc = zlib.crc32(payload, zlib.crc32(h[:28]))
        wire_crc = crc ^ 1 if corrupt else crc
        encoded = h[:28] + struct.pack('<I', wire_crc) + payload
        vectors.append(dict(id=name, transport='TCP', hex=encoded.hex(),
            sha256=hashlib.sha256(encoded).hexdigest(), note=note,
            expected=dict(codec_error=error, fields=dict(magic='SCB1', version=1, type=kind,
                flags=int(reliable), payload_len=length, record_count=count, session_id=session,
                seq=seq, ack_seq=0, request_id=request), crc32=f'{wire_crc:08x}',
                computed_crc32=f'{crc:08x}', payload_hex=payload.hex())))
        return payload

    def text(value):
        return json.dumps(value, ensure_ascii=True, separators=(',', ':'), allow_nan=False).encode('ascii')

    add('hello-stm32', 1, text(dict(role='stm32', protocol=1)), session=0, seq=0)
    add('hello-pc', 1, text(dict(role='pc', protocol=1)), session=0, seq=0)
    add('hello-ack', 2, text(dict(protocol=1, session_id=SESSION, link_ready=False)), seq=0)
    add('idle', 3)
    query = add('get-status', 0x10, text(dict(op='GET_STATUS', args={})), reliable=True, seq=2, request=1)
    values = dict(kp=2.0, k2p=0.008, kd=15.0, kp_v=20.0, ki_v=0.75, base_speed=180, fan_run=1600, fan_idle=1100)
    settings = text(dict(op='SET_PARAMS', args=dict(mode=0, gear=0, values=values)))
    add('set-params', 0x10, settings, reliable=True, seq=3, request=2)
    for count in (1, 10):
        records = []
        for i in range(count):
            r = bytearray(96)
            struct.pack_into('<I4h2h2h3h2H2B', r, 0, 1000 + i, 101, 202, 303, 404,
                             -5, 6, -7, 8, -9, 10, -11, 1234, 12000, 2, 3)
            struct.pack_into('<16HII2hH4B', r, 32, *range(100, 116), 200 + i, 300 + i, -12, 13, 0, 0, 1, 1, 0)
            records.append(r)
        payload = struct.pack('<6I', 0, 7, 0, 0, 1000, 1) + b''.join(records)
        add(f'telemetry-{count}', 0x20, payload, count=count, seq=10 + count)
    add('log-data', 0x30, struct.pack('<IHH', 256, 16, 0) + bytes(range(16)), reliable=True, seq=21, request=3)
    add('bad-crc', 0x10, query, reliable=True, seq=2, request=1, corrupt=True, error='CRC', note='Only CRC bit 0 changes.')
    add('bad-length', 0x10, query, reliable=True, seq=2, request=1, declared=len(query) + 1, error='LENGTH',
        note='Header declares one byte more than present; no partial execution.')
    add('reliable-repeat', 0x10, query, reliable=True, seq=2, request=1, note='Byte-identical to get-status; deliver once, ACK again.')
    add('request-id-repeat', 0x10, settings, reliable=True, seq=22, request=2,
        note='Same business ID/content but new link seq; ESP32 delivers to STM32 request cache.')
    values['base_speed'] = 181
    add('request-id-conflict', 0x10, text(dict(op='SET_PARAMS', args=dict(mode=0, gear=0, values=values))),
        reliable=True, seq=23, request=2, note='Same business ID/different content: ESP32 forwards; STM32 decides INVALID_ARG.')
    return dict(status='LOCAL_CANDIDATE_NOT_THREE_END_VERIFIED', protocol='SCB/1',
                session_id=SESSION, vector_count=len(vectors), vectors=vectors)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='compare only; never replace reviewed bytes')
    args = parser.parse_args()
    path = ROOT / 'tests/vectors/scb-v1.json'
    data = (json.dumps(generate(), ensure_ascii=True, indent=2) + '\n').encode('ascii')
    if args.check:
        if not path.is_file() or path.read_bytes() != data:
            print('Candidate baseline differs; inspect changes and review before regenerating.', file=sys.stderr)
            return 1
        print('14 fixed LOCAL CANDIDATE vectors unchanged; SHA256=' + hashlib.sha256(data).hexdigest())
    else:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
        print('Wrote 14 LOCAL CANDIDATE vectors; three-end comparison remains pending.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
