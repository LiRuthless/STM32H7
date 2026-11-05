"""Production handshake machine; outputs parsed independently with struct/JSON."""
import ctypes as C
import json
import random
import unittest
from decimal import Decimal
from link_host import Session, View, Workspace, library
from test_protocol import frame, mutate, WIRE

REJECT, REPLY, ESTABLISHED, PASS, NEED_RANDOM = range(5)


def hello(role='stm32', **kwargs):
    return frame(1, json.dumps({'role': role, 'protocol': 1}, separators=(',', ':')).encode(),
                 session=0, seq=0, ack=0, **kwargs)


class SessionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.lib = library()

    def setUp(self):
        self.session, self.ws = Session(), Workspace()
        self.lib.scb_session_init(C.byref(self.session))

    def spi(self, blob=None, candidate=51, expected=REPLY, capacity=1024):
        blob = hello() if blob is None else blob
        blob += bytes(1024 - len(blob))
        out, used = C.create_string_buffer(1024), C.c_size_t(999)
        self.assertEqual(self.lib.scb_session_spi(C.byref(self.session), C.create_string_buffer(blob), len(blob),
                         candidate, C.byref(self.ws), out, capacity, C.byref(used)), expected)
        if expected != REPLY:
            self.assertEqual(used.value, 0)
            return None
        self.assertEqual(used.value, 1024)
        return self.ack(bytes(out))

    def ack(self, blob):
        fields = WIRE.unpack(blob[:32])
        self.assertEqual(fields[:5], (b'SCB1', 1, 2, 0, fields[4]))
        self.assertEqual(fields[5], 0)
        self.assertEqual(fields[7:10], (0, 0, 0))
        # Independent CRC, not a production encoder/decoder roundtrip.
        import zlib
        self.assertEqual(fields[10], zlib.crc32(blob[32:32 + fields[4]], zlib.crc32(blob[:28])))
        payload = json.loads(blob[32:32 + fields[4]])
        self.assertEqual(payload['session_id'], fields[6])
        self.assertEqual(payload['protocol'], 1)
        self.assertEqual(blob[32 + fields[4]:], bytes(len(blob) - 32 - fields[4]))
        return payload

    def tcp(self, blob=None, expected=REPLY, capacity=1024):
        blob = hello('pc') if blob is None else blob
        out, used = C.create_string_buffer(1024), C.c_size_t(999)
        self.assertEqual(self.lib.scb_session_tcp_hello(C.byref(self.session), C.create_string_buffer(blob), len(blob),
                         C.byref(self.ws), out, capacity, C.byref(used)), expected)
        if expected == REPLY:
            return self.ack(bytes(out[:used.value]))
        self.assertEqual(used.value, 0)

    def open(self):
        self.assertTrue(self.lib.scb_session_tcp_open(C.byref(self.session)))

    def ready(self):
        self.spi()
        self.spi(frame(session=51, seq=1, ack=0), expected=ESTABLISHED)

    def permitted(self, id=51):
        return self.lib.scb_session_tcp_permitted(C.byref(self.session), id)

    def test_spi_confirmation_and_subsequent_transaction_reply(self):
        result = self.spi()
        self.assertEqual(result, {'protocol': 1, 'session_id': 51, 'link_ready': False})
        self.assertEqual(self.session.phase, 1)
        self.spi(frame(session=51, seq=1, ack=0), expected=ESTABLISHED)
        self.assertEqual(self.session.phase, 2)
        self.spi(frame(0x10, b'{}', flags=1, session=51, seq=2, ack=0), expected=PASS)

    def test_repeated_hello_while_waiting_uses_same_session(self):
        first = self.spi()
        for candidate in (0, 52, 0xffffffff):
            self.assertEqual(self.spi(candidate=candidate), first)

    def test_zero_or_reused_random_requires_retry_without_mutation(self):
        before = bytes(self.session)
        self.spi(candidate=0, expected=NEED_RANDOM)
        self.assertEqual(bytes(self.session), before)
        self.ready()
        before = bytes(self.session)
        self.spi(candidate=51, expected=NEED_RANDOM)
        self.assertEqual(bytes(self.session), before)

    def test_new_hello_after_ready_revokes_tcp_before_owner_closes(self):
        self.ready()
        self.open()
        self.tcp()
        self.assertTrue(self.permitted())
        self.spi(candidate=52)
        self.assertTrue(self.session.close_tcp)
        self.assertFalse(self.permitted(51))
        self.assertFalse(self.permitted(52))
        self.assertFalse(self.lib.scb_session_tcp_open(C.byref(self.session)))
        self.tcp(expected=REJECT)
        self.lib.scb_session_tcp_close(C.byref(self.session))
        self.open()
        result = self.tcp()
        self.assertEqual(result, {'protocol': 1, 'session_id': 52, 'link_ready': False})
        self.spi(frame(session=52, seq=1, ack=0), expected=ESTABLISHED)
        self.assertTrue(self.permitted(52))

    def test_tcp_hello_never_changes_spi(self):
        for initialize in (False, True):
            if initialize:
                self.ready()
            self.open()
            before = (self.session.phase, self.session.session_id, self.session.previous_session)
            result = self.tcp()
            self.assertEqual(result['link_ready'], initialize)
            self.assertEqual(before, (self.session.phase, self.session.session_id, self.session.previous_session))
            self.lib.scb_session_tcp_close(C.byref(self.session))

    def test_not_ready_rejects_control_and_snapshot_reports_zero(self):
        self.open()
        self.assertFalse(self.permitted())
        self.assertEqual(self.tcp(), {'protocol': 1, 'session_id': 0, 'link_ready': False})
        self.assertFalse(self.permitted(0))

    def test_old_session_before_or_after_restart_is_rejected(self):
        self.spi(frame(session=51, seq=1, ack=0), expected=REJECT)
        self.ready()
        self.lib.scb_session_spi_lost(C.byref(self.session))
        self.spi(frame(session=51, seq=2, ack=0), expected=REJECT)
        self.spi(candidate=51, expected=NEED_RANDOM)
        self.spi(candidate=52)
        self.spi(frame(session=51, seq=1, ack=0), expected=REJECT)

    def test_timeout_revokes_ready_and_requests_tcp_close(self):
        self.ready()
        self.open()
        self.tcp()
        self.lib.scb_session_spi_lost(C.byref(self.session))
        self.assertEqual(self.session.phase, 0)
        self.assertEqual(self.session.session_id, 0)
        self.assertTrue(self.session.close_tcp)
        self.assertFalse(self.permitted())

    def test_tcp_disconnect_preserves_spi_and_reconnect_does_not_replay(self):
        self.ready()
        self.open()
        self.tcp()
        self.lib.scb_session_tcp_close(C.byref(self.session))
        self.assertEqual((self.session.phase, self.session.session_id), (2, 51))
        self.open()
        self.assertFalse(self.permitted())
        self.assertTrue(self.tcp()['link_ready'])
        self.assertTrue(self.permitted())
        self.assertFalse(self.permitted(50))

    def test_wrong_role_protocol_and_handshake_fields_rejected(self):
        for blob in (hello('pc'), hello(request=1), mutate(hello(), 12, 1, 'I'),
                     mutate(hello(), 16, 1, 'I'), mutate(hello(), 20, 1, 'I'),
                     frame(1, b'{"role":"stm32","protocol":2}', session=0, seq=0, ack=0),
                     frame(1, b'{"role":"stm32","protocol":true}', session=0, seq=0, ack=0),
                     frame(1, b'{"role":"stm32","protocol":"1"}', session=0, seq=0, ack=0)):
            with self.subTest(blob=blob):
                before = bytes(self.session)
                self.spi(blob, expected=REJECT)
                self.assertEqual(bytes(self.session), before)

    def test_confirmation_must_be_idle_current_session_and_normal_seq(self):
        self.spi()
        for blob in (frame(session=50, seq=1, ack=0), frame(session=51, seq=0, ack=0),
                     frame(session=51, seq=0xffffffff, ack=0), frame(session=51, seq=1, ack=1),
                     frame(session=51, seq=1, ack=0, request=1),
                     frame(0x10, b'{}', flags=1, session=51, seq=1, ack=0)):
            self.spi(blob, expected=REJECT)
            self.assertEqual(self.session.phase, 1)

    def test_bad_crc_and_output_capacity_failure_are_transactional(self):
        self.spi(mutate(hello(), 28, 0, 'I', recalc=False), expected=REJECT)
        self.assertEqual(self.session.session_id, 0)
        self.spi(capacity=100, expected=REJECT)
        self.assertEqual(self.session.session_id, 0)
        self.open()
        self.tcp(capacity=10, expected=REJECT)
        self.assertFalse(self.session.tcp_hello)

    def test_tcp_requires_open_and_one_hello_per_connection(self):
        self.tcp(expected=REJECT)
        self.open()
        self.tcp(hello('stm32'), expected=REJECT)
        self.tcp()
        self.tcp(expected=REJECT)
        self.assertFalse(self.lib.scb_session_tcp_open(C.byref(self.session)))

    def test_json_escaped_strings_keys_and_equivalent_protocol_one(self):
        for number in (b'1', b'1.0', b'1e0', b'0.1E+1', b'10000e-4'):
            self.lib.scb_session_init(C.byref(self.session))
            payload = b'{"ro\\u006ce":"st\\u006d32","pro\\u0074ocol":' + number + b'}'
            self.spi(frame(1, payload, session=0, seq=0, ack=0))


class JsonFieldTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.lib = library()

    def integer(self, text):
        source = C.create_string_buffer(text.encode())
        value = C.c_uint32(999)
        valid = self.lib.bridge_json_u32(View(C.cast(source, C.c_void_p), len(text)), C.byref(value))
        return valid, value.value

    def test_exact_integer_boundaries_fractional_and_overflow(self):
        for text, expected in (('0', 0), ('1.000', 1), ('4294967295', 4294967295),
                               ('42949672950e-1', 4294967295), ('1e9', 1000000000),
                               ('0e9999', 0), ('0.0000', 0)):
            self.assertEqual(self.integer(text), (True, expected), text)
        for text in ('-1', '1.5', '4294967296', '1e10', '1e-1', '1e999', 'true', '"1"', '01', '1x', ''):
            self.assertEqual(self.integer(text), (False, 0), text)

    def test_random_decimal_differential(self):
        rng = random.Random(20261018)
        for _ in range(2000):
            text = f'{rng.randrange(10**12)}.{rng.randrange(10000):04}e{rng.randrange(-12, 12):+}'
            value = Decimal(text)
            valid = value == value.to_integral_value() and 0 <= value <= 0xffffffff
            self.assertEqual(self.integer(text), (valid, int(value) if valid else 0), text)
