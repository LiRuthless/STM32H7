"""Regression tests for staged evidence validation; all writes use temporary repos."""
import copy
import json
from pathlib import Path
import tempfile
import unittest

from comment_review_check import Repository, check, digest, DEFAULT_RECORDS

SOURCE = 'SmartCar/App/demo.c'
DEPENDENCY = 'SmartCar/Core/Inc/config.h'
OLD = b'/* initial */\nint demo(void) { return 1; }\n'
NEW = b'/* revised evidence */\nint demo(void) { return 1; }\n'


class EvidenceTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix='r08_gate_')
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.repo = Repository(self.root)
        self.repo.git('init', '-q')
        self.repo.git('config', 'user.name', 'R08 Test')
        self.repo.git('config', 'user.email', 'test@example.invalid')
        self.repo.git('config', 'core.autocrlf', 'false')
        self.write(SOURCE, OLD)
        self.write(DEPENDENCY, b'#define VALUE 1\n')
        self.repo.git('add', '--', SOURCE, DEPENDENCY)
        self.repo.git('commit', '-qm', 'test baseline')
        self.base = self.repo.git('rev-parse', 'HEAD').decode().strip()
        self.write(SOURCE, NEW)
        self.repo.git('add', '--', SOURCE)
        self.records = {SOURCE: self.row(SOURCE)}

    def write(self, path, data):
        target = self.root / path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)

    def row(self, path, deleted=False):
        data = None if deleted else (self.root/path).read_bytes()
        return dict(sha256=digest(data), reviewed_base=self.base,
                    reviewer='test reviewer', reviewed_at='2026-10-22', unresolved=[],
                    dependencies={}, base_dependencies=({path: digest(OLD)} if deleted else {}),
                    checks={k: {'status': 'pass', 'reason': 'Verified against function body.'}
                            for k in ('coverage', 'format', 'content', 'behavior')},
                    items=[{'object': 'demo module', 'fact': 'The function returns integer one.',
                            'evidence': [{'path': path, 'anchor': 'int demo(void)',
                                          'revision': 'base' if deleted else 'index'}]}])

    def run_gate(self, passes):
        self.write(DEFAULT_RECORDS, json.dumps({'version': 1, 'files': self.records}).encode())
        self.repo.git('add', '--', DEFAULT_RECORDS)
        # Invalidate only this test helper's blob cache after mutations.
        self.repo = Repository(self.root)
        snapshot = {str(p.relative_to(self.root)): p.read_bytes()
                    for p in self.root.rglob('*') if p.is_file()}
        errors, _, _ = check(self.repo, self.base, DEFAULT_RECORDS)
        after = {str(p.relative_to(self.root)): p.read_bytes()
                 for p in self.root.rglob('*') if p.is_file()}
        self.assertEqual(snapshot, after, 'checker must not write files, index or git config')
        self.assertEqual(not errors, passes, errors)
        return errors

    def test_valid_and_read_only(self):
        self.run_gate(True)

    def test_missing_record(self):
        self.records = {}
        self.assertIn('missing', self.run_gate(False)[0])

    def test_comment_only_stale(self):
        self.records[SOURCE]['sha256'] = digest(OLD)
        self.run_gate(False)

    def test_source_changed_again(self):
        self.write(SOURCE, NEW.replace(b'return 1', b'return 2'))
        self.repo.git('add', '--', SOURCE)
        self.run_gate(False)

    def test_partial_staging_uses_index(self):
        self.write(SOURCE, b'working tree intentionally differs')
        self.run_gate(True)

    def test_unstaged_records_not_used(self):
        self.run_gate(True)
        self.write(DEFAULT_RECORDS, b'invalid unstaged json')
        errors, _, _ = check(Repository(self.root), self.base, DEFAULT_RECORDS)
        self.assertEqual(errors, [])

    def test_unresolved(self):
        self.records[SOURCE]['unresolved'] = ['confirm timer']
        self.run_gate(False)

    def test_placeholder(self):
        self.records[SOURCE]['checks']['content']['reason'] = 'TODO verify later'
        self.run_gate(False)

    def test_missing_anchor(self):
        self.records[SOURCE]['items'][0]['evidence'][0]['anchor'] = 'nonexistent_function'
        self.run_gate(False)

    def test_evidence_requires_registered_dependency(self):
        self.records[SOURCE]['items'][0]['evidence'][0] = {
            'path': DEPENDENCY, 'anchor': '#define VALUE', 'revision': 'index'}
        self.run_gate(False)

    def test_dependency_stale(self):
        self.records[SOURCE]['dependencies'] = {DEPENDENCY: digest(b'#define VALUE 0\n')}
        self.run_gate(False)

    def test_dependency_only_change_invalidates_prior_review(self):
        self.records[SOURCE]['dependencies'] = {DEPENDENCY: digest(b'#define VALUE 1\n')}
        self.run_gate(True)
        self.repo.git('commit', '-qm', 'record previous review')
        self.base = self.repo.git('rev-parse', 'HEAD').decode().strip()
        self.write(DEPENDENCY, b'#define VALUE 2\n')
        self.repo.git('add', '--', DEPENDENCY)
        # Even replacing the hash cannot reuse an old review base.
        self.records[SOURCE]['dependencies'][DEPENDENCY] = digest(b'#define VALUE 2\n')
        errors = self.run_gate(False)
        self.assertTrue(any('changed source/dependency' in e for e in errors))

    def test_removed_dependency_still_invalidates(self):
        self.records[SOURCE]['dependencies'] = {DEPENDENCY: digest(b'#define VALUE 1\n')}
        self.run_gate(True)
        self.repo.git('commit', '-qm', 'record prior dependency')
        self.base = self.repo.git('rev-parse', 'HEAD').decode().strip()
        self.records[SOURCE]['dependencies'] = {}
        self.run_gate(False)

    def test_registered_record_cannot_disappear(self):
        self.run_gate(True)
        self.repo.git('commit', '-qm', 'record review')
        self.base = self.repo.git('rev-parse', 'HEAD').decode().strip()
        self.records = {}
        self.run_gate(False)

    def test_addition_requires_reference_analysis(self):
        path = 'SmartCar/App/new.c'
        self.write(path, NEW)
        self.repo.git('add', '--', path)
        self.records[path] = self.row(path)
        self.run_gate(False)
        self.records[path]['cleanup'] = 'New helper has no previous declarations or callers.'
        self.run_gate(True)

    def test_delete_requires_cleanup_and_base_evidence(self):
        self.repo.git('rm', '-f', '--', SOURCE)
        self.records[SOURCE] = self.row(SOURCE, deleted=True)
        self.run_gate(False)
        self.records[SOURCE]['cleanup'] = 'Removed private function; no callers remain.'
        self.run_gate(True)

    def test_rename_checked_as_delete_and_add(self):
        target = 'SmartCar/App/renamed.c'
        self.repo.git('mv', '--', SOURCE, target)
        self.repo.git('add', '--', target)
        self.records = {SOURCE: self.row(SOURCE, deleted=True), target: self.row(target)}
        for row in self.records.values():
            row['cleanup'] = 'Renamed private source; build references updated and reviewed.'
        self.run_gate(True)

    def core_case(self, change_user=False, reason='Generated macro change; user block unchanged.'):
        path = 'SmartCar/Core/Src/demo.c'
        old = b'#define X 1\n/* USER CODE BEGIN 0 */\nint demo(void);\n/* USER CODE END 0 */\n'
        self.write(path, old)
        self.repo.git('add', '--', path)
        self.repo.git('commit', '-qm', 'add generated baseline')
        self.base = self.repo.git('rev-parse', 'HEAD').decode().strip()
        new = old.replace(b'X 1', b'X 2')
        if change_user:
            new = new.replace(b'demo(void)', b'demo(int x)')
        self.write(path, new)
        self.repo.git('add', '--', path)
        row = self.row(path)
        row['items'][0]['evidence'][0]['anchor'] = 'int demo'
        row['exception'] = dict(scope='generated-only', reason=reason,
                                evidence='USER CODE block bytes compared with baseline.')
        self.records = {path: row}

    def test_generated_only_exception(self):
        self.core_case()
        self.run_gate(True)

    def test_generated_exception_cannot_hide_user_change(self):
        self.core_case(change_user=True)
        self.run_gate(False)

    def test_exception_needs_reason(self):
        self.core_case(reason='')
        self.run_gate(False)

    def test_third_party_excluded(self):
        path = 'SmartCar/Drivers/vendor.c'
        self.write(path, OLD)
        self.repo.git('add', '--', path)
        self.run_gate(True)

    def test_bad_path(self):
        row = copy.deepcopy(self.records[SOURCE])
        self.records['../outside.c'] = row
        self.run_gate(False)


if __name__ == '__main__':
    unittest.main(verbosity=2)
