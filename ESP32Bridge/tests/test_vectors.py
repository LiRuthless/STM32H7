"""Frozen LOCAL CANDIDATE bytes tested against production C and independent audit."""
import ctypes as C
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import unittest
from link_host import Frame, Link, Workspace, library

ROOT = Path(__file__).resolve().parents[1]
BASELINE = ROOT / 'tests/vectors/scb-v1.json'


class CandidateVectorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.lib = library()
        cls.document = json.loads(BASELINE.read_text())
        cls.vectors = {v['id']: v for v in cls.document['vectors']}

    def test_frozen_baseline_regeneration_check(self):
        result = subprocess.run([sys.executable, str(ROOT / 'scripts/generate_vectors.py'), '--check'], capture_output=True)
        self.assertEqual(result.returncode, 0, result.stderr.decode())

    def test_git_checkout_preserves_frozen_file_bytes(self):
        # --filters applies the real repository checkout rules to its indexed
        # blob without replacing the working file or creating another checkout.
        result = subprocess.run(['git', 'check-attr', 'text', 'eol', '--', 'ESP32Bridge/tests/vectors/scb-v1.json'],
                                cwd=ROOT.parent, capture_output=True, check=True)
        self.assertIn(b'text: set', result.stdout)
        self.assertIn(b'eol: lf', result.stdout)
        result = subprocess.run(['git', 'cat-file', '--filters', 'HEAD:ESP32Bridge/tests/vectors/scb-v1.json'],
                                cwd=ROOT.parent, capture_output=True, check=True)
        self.assertNotIn(b'\r', result.stdout)
        self.assertEqual(result.stdout, BASELINE.read_bytes())

    def test_independent_offset_bit_crc_sha256_audit(self):
        spec = importlib.util.spec_from_file_location('independent_vectors', ROOT / 'scripts/check_vectors.py')
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        result = module.audit(BASELINE)
        self.assertFalse(result['three_end_verified'])
        self.assertEqual(len(result['vectors']), 14)
        self.assertEqual([v['result'] for v in result['vectors']].count('OK'), 12)

    def test_all_fixed_vectors_against_production_c_codec_and_fields(self):
        errors = {'OK': 0, 'LENGTH': 2, 'CRC': 8}
        for item in self.vectors.values():
            for transport in (0, 1):
                with self.subTest(vector=item['id'], transport=transport):
                    b = bytes.fromhex(item['hex'])
                    # Length-negative vector must remain truncated for SPI too.
                    if transport == 0 and item['expected']['codec_error'] != 'LENGTH':
                        b += bytes(1024 - len(b))
                    view, ws = Frame(), Workspace()
                    source = C.create_string_buffer(b)
                    error = self.lib.scb_decode(source, len(b), transport, C.byref(ws), C.byref(view))
                    self.assertEqual(error, errors[item['expected']['codec_error']])
                    if error:
                        self.assertEqual(bytes(view), bytes(C.sizeof(view)))
                    else:
                        for field, expected in item['expected']['fields'].items():
                            if field not in ('magic', 'version'):
                                self.assertEqual(getattr(view.header, field), expected, field)
                        self.assertEqual(C.string_at(view.payload, view.header.payload_len).hex(), item['expected']['payload_hex'])

    def receive(self, link, name):
        b = bytes.fromhex(self.vectors[name]['hex'])
        view, ws = Frame(), Workspace()
        return self.lib.scb_link_receive(C.byref(link), C.create_string_buffer(b), len(b), True, C.byref(ws), C.byref(view))

    def test_fixed_reliable_repeat_delivers_once(self):
        link = Link()
        self.lib.scb_link_reset(C.byref(link), 1, self.document['session_id'])
        self.assertEqual(self.receive(link, 'get-status'), 1)
        self.assertEqual(self.receive(link, 'reliable-repeat'), 2)
        self.assertEqual(link.ack_seq, 2)

    def test_fixed_same_request_id_is_transparent_to_stm32_cache(self):
        link = Link()
        self.lib.scb_link_reset(C.byref(link), 1, self.document['session_id'])
        for name in ('set-params', 'request-id-repeat', 'request-id-conflict'):
            self.assertEqual(self.receive(link, name), 1)
        self.assertEqual(link.ack_seq, 23)

    def test_required_fixed_semantics_and_record_boundaries(self):
        self.assertEqual(self.vectors['get-status']['hex'], self.vectors['reliable-repeat']['hex'])
        for count, length in ((1, 152), (10, 1016)):
            item = self.vectors[f'telemetry-{count}']
            self.assertEqual(len(bytes.fromhex(item['hex'])), length)
            payload = bytes.fromhex(item['expected']['payload_hex'])
            self.assertEqual(int.from_bytes(payload[20:24], 'little'), 1)
            for index in range(count):
                record = payload[24 + index * 96:24 + (index + 1) * 96]
                self.assertEqual(int.from_bytes(record[:4], 'little'), 1000 + index)
                self.assertEqual(record[82:], bytes(14))
