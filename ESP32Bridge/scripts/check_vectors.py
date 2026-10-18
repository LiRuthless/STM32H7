"""Independent fixed-vector audit: byte offsets + bit CRC; no struct/zlib/production import."""
import argparse
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
FIELDS = dict(version=(4, 1), type=(5, 1), flags=(6, 2), payload_len=(8, 2), record_count=(10, 2),
              session_id=(12, 4), seq=(16, 4), ack_seq=(20, 4), request_id=(24, 4))


def crc32(parts):
    state = 0xffffffff
    for part in parts:
        for byte in part:
            state ^= byte
            for _ in range(8):
                state = (state >> 1) ^ (0xedb88320 if state & 1 else 0)
    return state ^ 0xffffffff


def audit(path):
    document = json.loads(path.read_text(encoding='ascii'))
    if document['status'] != 'LOCAL_CANDIDATE_NOT_THREE_END_VERIFIED':
        raise ValueError('Candidate status changed without three-end evidence')
    results = []
    for item in document['vectors']:
        b = bytes.fromhex(item['hex'])
        if len(b) < 32:
            raise ValueError('Truncated candidate header')
        fields = {key: int.from_bytes(b[p:p + n], 'little') for key, (p, n) in FIELDS.items()}
        fields['magic'] = b[:4].decode('ascii')
        computed = crc32((b[:28], b[32:]))
        encoded = int.from_bytes(b[28:32], 'little')
        error = 'LENGTH' if len(b) != 32 + fields['payload_len'] else 'CRC' if encoded != computed else 'OK'
        expected = item['expected']
        if (fields != expected['fields'] or hashlib.sha256(b).hexdigest() != item['sha256'] or
            f'{computed:08x}' != expected['computed_crc32'] or f'{encoded:08x}' != expected['crc32'] or
            b[32:].hex() != expected['payload_hex'] or error != expected['codec_error']):
            raise ValueError('Independent field/CRC/SHA/error mismatch: ' + item['id'])
        results.append(dict(id=item['id'], bytes=len(b), result=error, sha256=item['sha256']))
    if len(results) != document['vector_count'] or len({r['id'] for r in results}) != len(results):
        raise ValueError('Invalid vector count/duplicate IDs')
    return dict(status='LOCAL_CANDIDATE_AUDIT_PASS', three_end_verified=False,
                baseline_sha256=hashlib.sha256(path.read_bytes()).hexdigest(), vectors=results)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, default=ROOT / 'tests/vectors/scb-v1.json')
    parser.add_argument('--report', type=Path, help='optional audit evidence JSON output')
    args = parser.parse_args()
    try:
        report = audit(args.input)
        if args.report:
            args.report.parent.mkdir(parents=True, exist_ok=True)
            args.report.write_text(json.dumps(report, indent=2) + '\n', encoding='ascii')
        print(f"Independent audit: {len(report['vectors'])} vectors; SHA256={report['baseline_sha256']}; three-end pending")
        return 0
    except (ValueError, KeyError, OSError, TypeError):
        print('Candidate audit failed; inspect fixed bytes/expectations. Nothing was replaced.', file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
