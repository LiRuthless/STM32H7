"""Verify complete old/final symbol indexes and every registered migration target."""
import hashlib
import json
from pathlib import Path
import subprocess

from app_inventory import inventory

ROOT = Path(__file__).resolve().parents[1]
REPO = ROOT.parent
DOCUMENT = ROOT/'specs/features/17-app-architecture/final-symbol-map.json'


def key(row):
    return f"{row['path']}::{row['kind']}::{row.get('scope', '')}::{row['symbol']}@{row['line']}"


def main():
    data = json.loads(DOCUMENT.read_text(encoding='utf-8'))
    base = data['baseline']
    paths = subprocess.check_output(['git', 'ls-tree', '-r', '--name-only', base, 'SmartCar/App'],
                                    cwd=REPO, text=True).splitlines()
    original = []
    for name in paths:
        if name.endswith(('.c', '.h')):
            source = subprocess.check_output(['git', 'show', base+':'+name], cwd=REPO).decode('utf-8')
            original.extend(inventory(name, source))
    assert data['original'] == original, 'old definitions/conditions missing or altered'
    current = []
    for name, digest in data['current_sha256'].items():
        source = (REPO/name).read_bytes()
        assert hashlib.sha256(source.replace(b'\r\n', b'\n')).hexdigest() == digest, ('stale final inventory', name)
        current.extend(inventory(name, source.decode('utf-8')))
    assert data['current'] == current, 'final definitions/conditions incomplete'
    actual_paths = {p.relative_to(REPO).as_posix() for p in (ROOT/'App').rglob('*')
                    if p.suffix in ('.c', '.h')}
    assert actual_paths <= set(data['current_sha256']), 'current App file missing'
    originals = {key(row) for row in original}
    targets = {key(row) for row in current}
    assert {row['original'] for row in data['migrations']} == originals
    assert len(data['migrations']) == len(originals), 'duplicate origin binding'
    for row in data['migrations']:
        assert row['targets'] and set(row['targets']) <= targets, row
        assert row['reason'].strip() and row['validation'], row
    assert {row['item'] for row in data['owners']} == targets, 'unregistered final definition'
    for row in data['owners']:
        assert row['owner'] in data['current_sha256'] and row['context'] and row['validation'], row
    print(f'PASS migration index: {len(original)} old items, {len(current)} final items; '
          'all functions/objects/types/fields/macros/branches bound to existing source')


if __name__ == '__main__':
    main()
