"""Check staged comment-review evidence, never infer semantic correctness.

Run from the repository: python SmartCar/tests/comment_review_check.py --base COMMIT
The records and every source are read from the Git index (not the working tree).
Schema and workflow: specs/features/15-code-style-consistency/r08-review-contract.md.
"""
import argparse
import datetime
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import subprocess
import sys

DEFAULT_RECORDS = 'SmartCar/specs/features/15-code-style-consistency/comment-evidence.json'
PLACEHOLDERS = re.compile(r'TODO|TBD|待确认|待处理|同上|保持原逻辑|按原逻辑', re.I)


def digest(data):
    return hashlib.sha256(data).hexdigest() if data is not None else None


def safe_path(path):
    if not isinstance(path, str) or not path or '\\' in path or ':' in path:
        raise ValueError(f'invalid repository path: {path!r}')
    p = PurePosixPath(path)
    if p.is_absolute() or any(x in ('', '.', '..') for x in path.split('/')):
        raise ValueError(f'invalid repository path: {path!r}')
    return path


def meaningful(value):
    return isinstance(value, str) and len(value.strip()) >= 4 and not PLACEHOLDERS.search(value)


def scoped(path):
    p = PurePosixPath(path)
    if p.suffix.lower() not in ('.c', '.h'):
        return False
    return (path.startswith(('SmartCar/App/', 'SmartCar/Core/'))
            or (p.parent.as_posix() == 'SmartCar/BSP' and p.name.startswith('bsp'))
            or path in ('SmartCar/BSP/lcd/lcd.c', 'SmartCar/BSP/lcd/lcd.h'))


def unique_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f'duplicate JSON key: {key}')
        result[key] = value
    return result


def parse_records(data):
    value = json.loads(data.decode('utf-8'), object_pairs_hook=unique_object)
    if value.get('version') != 1 or not isinstance(value.get('files'), dict):
        raise ValueError('records must contain version=1 and files object')
    return value['files']


class Repository:
    def __init__(self, root):
        self.root = root
        self.cache = {}

    def git(self, *args, optional=False):
        proc = subprocess.run(['git', '-C', str(self.root), *args],
                              capture_output=True)
        if proc.returncode and not optional:
            raise ValueError(proc.stderr.decode('utf-8', errors='replace').strip())
        return None if proc.returncode else proc.stdout

    def blob(self, revision, path):
        safe_path(path)
        key = (revision, path)
        if key not in self.cache:
            spec = ':' + path if revision == 'index' else revision + ':' + path
            self.cache[key] = self.git('show', spec, optional=True)
        return self.cache[key]


def user_regions(data):
    """Reject malformed markers; preserve exact contents and marker identities."""
    text = (data or b'').decode('utf-8')
    markers = list(re.finditer(r'/\* USER CODE (BEGIN|END) ([^*]+?) \*/', text))
    regions = []
    if len(markers) % 2:
        raise ValueError('unpaired USER CODE markers')
    for begin, end in zip(markers[::2], markers[1::2]):
        if (begin[1] != 'BEGIN' or end[1] != 'END' or begin[2] != end[2]):
            raise ValueError('mismatched USER CODE markers')
        regions.append((begin[2], text[begin.end():end.start()]))
    return regions


def check(repo, base, records_path):
    base = repo.git('rev-parse', '--verify', base + '^{commit}').decode().strip()
    if repo.git('ls-files', '-u'):
        raise ValueError('unmerged index: resolve conflicts before review')
    raw = repo.blob('index', records_path)
    if raw is None:
        raise ValueError('review records are absent from the index: ' + records_path)
    records = parse_records(raw)
    changed = {p.decode('utf-8') for p in repo.git(
        'diff', '--cached', '--name-only', '--no-renames', '-z', base, '--').split(b'\0') if p}
    errors = []
    required = {p for p in changed if scoped(p)}
    old_raw = repo.blob(base, records_path)
    old = parse_records(old_raw) if old_raw is not None else {}
    for path in old.keys() - records.keys():
        errors.append(f'{path}: historical review removed; retain record and document deletion')
    for path in sorted(required - records.keys()):
        errors.append(f'{path}: missing review record')
    for path, row in records.items():
        try:
            safe_path(path)
            if not scoped(path) or not isinstance(row, dict):
                raise ValueError('record must describe a scoped C/H file')
            current = repo.blob('index', path)
            before = repo.blob(base, path)
            if current is None and before is None and path not in old:
                raise ValueError('record refers to nonexistent source')
            if 'sha256' not in row or row['sha256'] != digest(current):
                raise ValueError('source digest stale (including comment-only changes)')
            reviewed_base = row.get('reviewed_base', '')
            if not re.fullmatch(r'[0-9a-f]{40,64}', reviewed_base):
                raise ValueError('reviewed_base must be a full commit ID')
            repo.git('cat-file', '-e', reviewed_base + '^{commit}')
            if not meaningful(row.get('reviewer')):
                raise ValueError('missing reviewer')
            datetime.date.fromisoformat(row.get('reviewed_at', ''))
            if row.get('unresolved') != []:
                raise ValueError('unresolved decisions must be an empty array')
            deps = row.get('dependencies')
            base_deps = row.get('base_dependencies', {})
            if not isinstance(deps, dict) or not isinstance(base_deps, dict):
                raise ValueError('dependency maps required')
            affected = path in required
            for dep, expected in deps.items():
                data = repo.blob('index', dep)
                if data is None or digest(data) != expected:
                    raise ValueError(f'dependency digest stale/missing: {dep}')
                affected |= dep in changed
            # Removing a previous dependency also requires a new review.
            previous_deps = old.get(path, {}).get('dependencies', {})
            affected |= any(dep in changed for dep in previous_deps)
            affected |= set(previous_deps) != set(deps) and path in old
            if affected and reviewed_base != base:
                raise ValueError('changed source/dependency requires review against this base')
            for dep, expected in base_deps.items():
                data = repo.blob(reviewed_base, dep)
                if data is None or digest(data) != expected:
                    raise ValueError(f'baseline evidence digest stale/missing: {dep}')
            for category in ('coverage', 'format', 'content', 'behavior'):
                verdict = row.get('checks', {}).get(category, {})
                if verdict.get('status') != 'pass' or not meaningful(verdict.get('reason')):
                    raise ValueError(f'missing/non-specific {category} verdict')
            items = row.get('items')
            if not isinstance(items, list) or not items:
                raise ValueError('nonempty object/fact/evidence items required')
            for item in items:
                if not meaningful(item.get('object')) or not meaningful(item.get('fact')):
                    raise ValueError('object/fact missing or placeholder')
                evidence = item.get('evidence')
                if not isinstance(evidence, list) or not evidence:
                    raise ValueError('fact has no evidence')
                for ev in evidence:
                    dep, rev, anchor = ev.get('path'), ev.get('revision'), ev.get('anchor')
                    if rev not in ('index', 'base') or not meaningful(anchor):
                        raise ValueError('evidence revision/anchor invalid')
                    if rev == 'index' and dep != path and dep not in deps:
                        raise ValueError('index evidence lacks dependency digest')
                    if rev == 'base' and dep not in base_deps:
                        raise ValueError('base evidence lacks digest')
                    data = repo.blob('index' if rev == 'index' else reviewed_base, dep)
                    if data is None or anchor not in data.decode('utf-8'):
                        raise ValueError(f'evidence anchor absent: {dep}::{anchor}')
            # All additions and removals require a reference-impact statement; this
            # covers renames without relying on heuristic rename detection.
            if path in required and (before is None or current is None):
                if not meaningful(row.get('cleanup')):
                    raise ValueError('addition/deletion/rename needs reference cleanup explanation')
            exception = row.get('exception')
            if exception is not None:
                if (not path.startswith('SmartCar/Core/')
                        or exception.get('scope') != 'generated-only'
                        or not meaningful(exception.get('reason'))
                        or not meaningful(exception.get('evidence'))):
                    raise ValueError('invalid generated-only exception')
                if user_regions(before) != user_regions(current):
                    raise ValueError('generated-only exception changes USER CODE regions')
        except (ValueError, TypeError, KeyError, AttributeError) as exc:
            errors.append(f'{path}: {exc}')
    return errors, len(required), len(records)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--base', required=True)
    parser.add_argument('--records', default=DEFAULT_RECORDS)
    args = parser.parse_args()
    try:
        errors, count, registered = check(Repository(args.repo), args.base, args.records)
        if errors:
            for error in errors:
                print('FAIL:', error)
            return 1
        print(f'PASS: {count} changed scoped files; {registered} registered reviews current.')
        print('Evidence consistency only; semantic review remains a human responsibility.')
        return 0
    except (ValueError, OSError, TypeError, AttributeError) as exc:
        print('FAIL:', exc)
        return 1


if __name__ == '__main__':
    sys.exit(main())
