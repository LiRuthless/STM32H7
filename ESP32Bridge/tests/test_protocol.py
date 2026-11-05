"""Production C codec vs independent Python struct/zlib and strict JSON oracle.

These generated inputs are test cases, not the reviewed common-vector baseline.
No transport action, session handshake or hardware outcome is claimed here.
"""
import ctypes as C
import json
import math
import random
import struct
import unittest
import zlib

from protocol_host import Frame, Header, View, Workspace, load

WIRE = struct.Struct("<4sBBHHHIIIII")
assert WIRE.size == 32
SPI, TCP, UDP = 0, 1, 2
OK, ARGUMENT, LENGTH, MAGIC, VERSION, TYPE, FLAGS, COUNT, CRC, PAYLOAD, JSON, TRANSPORT, ALIAS = range(13)


def frame(kind=3, payload=b"", flags=0, count=0, session=0x12345678, seq=19, ack=11, request=0):
    header = WIRE.pack(b"SCB1", 1, kind, flags, len(payload), count, session, seq, ack, request, 0)
    crc = zlib.crc32(payload, zlib.crc32(header[:28]))
    return header[:28] + struct.pack("<I", crc) + payload


def mutate(blob, offset, value, fmt="B", recalc=True):
    out = bytearray(blob)
    struct.pack_into("<" + fmt, out, offset, value)
    if recalc:
        struct.pack_into("<I", out, 28, zlib.crc32(out[32:], zlib.crc32(out[:28])))
    return bytes(out)


def telemetry(count):
    records = []
    for i in range(count):
        record = bytearray(96)
        struct.pack_into("<I", record, 0, 1000 + i)
        struct.pack_into("<4h", record, 4, -32768, -1, 0, 32767)
        struct.pack_into("<II", record, 64, 7, 30 + i)
        records.append(record)
    return struct.pack("<6I", 19, 2, 3, 4, 1000, 1) + b"".join(records)


class ProtocolTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.lib = load()

    def setUp(self):
        self.ws = Workspace()

    def decode(self, blob, transport=TCP, expected=OK):
        source = C.create_string_buffer(blob)
        view = Frame()
        C.memset(C.byref(view), 0xa5, C.sizeof(view))
        error = self.lib.scb_decode(source, len(blob), transport, C.byref(self.ws), C.byref(view))
        self.assertEqual(error, expected)
        if error:
            self.assertEqual(bytes(view), bytes(C.sizeof(view)), "failure leaked partial view")
            return None
        return view.header, C.string_at(view.payload, view.header.payload_len)

    def encode(self, kind=3, payload=b"", flags=0, count=0, transport=TCP, capacity=1024, expected=OK):
        header = Header(kind, flags, len(payload), count, 0x12345678, 19, 11, 0)
        output = C.create_string_buffer(b"\xa5" * 1032, 1032)
        source = C.create_string_buffer(payload)
        used = C.c_size_t(999)
        error = self.lib.scb_encode(C.byref(header), source, transport, output, capacity,
                                    C.byref(self.ws), C.byref(used))
        self.assertEqual(error, expected)
        self.assertEqual(output.raw[capacity:], b"\xa5" * (1032 - capacity))
        if error:
            self.assertEqual(used.value, 0)
            self.assertEqual(output.raw, b"\xa5" * 1032, "failed encode modified destination")
        return output.raw[:used.value]

    def test_independent_header_offsets_and_crc(self):
        payload = b'{"op":"GET_STATUS","args":{}}'
        expected = frame(0x10, payload, 1)
        actual = self.encode(0x10, payload, 1)
        self.assertEqual(actual, expected)
        header, decoded = self.decode(expected)
        self.assertEqual((header.session_id, header.seq, header.ack_seq), (0x12345678, 19, 11))
        self.assertEqual(decoded, payload)
        self.assertEqual(WIRE.unpack(actual[:32])[-1], zlib.crc32(actual[32:], zlib.crc32(actual[:28])))

    def test_zero_and_992_byte_boundaries(self):
        self.assertEqual(self.encode(), frame())
        maximum = b'"' + b"x" * 990 + b'"'
        self.assertEqual(len(self.encode(0x10, maximum, 1)), 1024)
        self.decode(frame(0x10, maximum, 1))
        self.decode(mutate(frame(), 8, 993, "H"), expected=LENGTH)

    def test_spi_exact_transfer_and_zero_tail(self):
        out = self.encode(transport=SPI)
        self.assertEqual(len(out), 1024)
        self.assertEqual(out[:32], frame())
        self.assertEqual(out[32:], bytes(992))
        self.decode(out, SPI)
        self.decode(out[:32] + b"\xff" * 992, SPI)  # padding excluded from CRC
        for size in (0, 31, 32, 1023, 1025):
            with self.subTest(size=size): self.decode(out[:size] if size < 1024 else out + b"x", SPI, LENGTH)

    def test_tcp_exact_size_without_padding(self):
        self.decode(frame() + b"x", expected=LENGTH)
        self.decode(frame(0x10, b'{}', 1)[:-1], expected=LENGTH)

    def test_all_type_and_flag_combinations(self):
        types = {1: (b'{}', 0), 2: (b'{}', 0), 3: (b'', 0), 0x10: (b'{}', 1),
                 0x11: (b'{}', 1), 0x12: (b'{}', 1), 0x20: (telemetry(1), 0),
                 0x30: (struct.pack('<IHH', 0, 1, 0) + b'x', 1)}
        for kind, (payload, flag) in types.items():
            for actual in (0, 1, 2, 0xffff):
                with self.subTest(kind=kind, flags=actual):
                    self.decode(frame(kind, payload, actual, int(kind == 0x20)),
                                expected=OK if actual == flag else FLAGS)
        for kind in (0, 4, 0x13, 0xff):
            self.decode(frame(kind), expected=TYPE)

    def test_magic_version_and_crc_rejected_without_view(self):
        self.decode(mutate(frame(), 0, ord('X')), expected=MAGIC)
        self.decode(mutate(frame(), 4, 2), expected=VERSION)
        self.decode(mutate(frame(), 28, 0, recalc=False), expected=CRC)
        source = frame(0x10, b'{"x":1}', 1)
        self.decode(source[:-2] + b'2}', expected=CRC)

    def test_every_header_and_payload_bit_is_covered(self):
        original = frame(0x10, b'{"op":"GET_STATUS","args":{}}', 1)
        for index in range(len(original)):
            corrupted = bytearray(original)
            corrupted[index] ^= 1
            view = Frame()
            source = C.create_string_buffer(bytes(corrupted))
            error = self.lib.scb_decode(source, len(corrupted), TCP, C.byref(self.ws), C.byref(view))
            self.assertNotEqual(error, 0, "unprotected byte " + str(index))
            self.assertFalse(view.payload)

    def test_idle_payload_and_nontelemetry_counts(self):
        self.decode(frame(3, b'x'), expected=PAYLOAD)
        for kind in (1, 2, 3, 0x10, 0x11, 0x12, 0x30):
            self.decode(frame(kind, flags=int(kind >= 0x10), count=1), expected=COUNT)

    def test_telemetry_1_and_10_independently_encoded(self):
        for count, length in ((1, 152), (10, 1016)):
            payload = telemetry(count)
            encoded = self.encode(0x20, payload, count=count, transport=UDP)
            self.assertEqual(encoded, frame(0x20, payload, count=count))
            self.assertEqual(len(encoded), length)
            self.decode(encoded, UDP)
        for count in (0, 11, 0xffff):
            self.decode(frame(0x20, telemetry(1), count=count), UDP, COUNT)
        self.decode(frame(0x20, telemetry(1)[:-1], count=1), UDP, LENGTH)

    def test_all_telemetry_reserved_bytes(self):
        valid = frame(0x20, telemetry(10), count=10)
        for record in range(10):
            for reserved in range(82, 96):
                self.decode(mutate(valid, 32 + 24 + 96 * record + reserved, 1), expected=PAYLOAD)

    def test_telemetry_metadata_without_vehicle_interpretation(self):
        valid = frame(0x20, telemetry(1), count=1)
        self.decode(mutate(valid, 32 + 20, 0, 'I'), expected=PAYLOAD)
        self.decode(mutate(valid, 32 + 16, 999, 'I'), expected=PAYLOAD)
        # Control/sensor values are opaque, not range-checked by ESP32.
        self.decode(mutate(valid, 32 + 24 + 78, 255))

    def test_log_lengths_offsets_reserved_and_opaque_bytes(self):
        for count in (1, 960):
            payload = struct.pack('<IHH', 0x12340000, count, 0) + b'\xff' * count
            expected = frame(0x30, payload, 1)
            self.assertEqual(self.encode(0x30, payload, 1), expected)
            self.decode(expected)
        for count in (0, 961):
            self.decode(frame(0x30, struct.pack('<IHH', 0, count, 0) + b'x' * count, 1), expected=LENGTH)
        self.decode(frame(0x30, bytes(7), 1), expected=LENGTH)
        valid = frame(0x30, struct.pack('<IHH', 0, 2, 0) + b'xy', 1)
        self.decode(mutate(valid, 32 + 6, 1), expected=PAYLOAD)
        self.decode(mutate(valid, 32 + 4, 1, 'H'), expected=LENGTH)
        self.decode(mutate(valid, 32, 0xffffffff, 'I'), expected=PAYLOAD)

    def test_udp_type_and_transport_restrictions(self):
        self.decode(frame(), UDP, TRANSPORT)
        self.decode(frame(), 3, TRANSPORT)
        self.encode(transport=3, expected=TRANSPORT)

    def test_encode_capacity_alias_and_failures_are_transactional(self):
        self.encode(capacity=31, expected=LENGTH)
        self.encode(0x10, b'bad', 1, expected=JSON)
        self.encode(0x10, b'{}', 0, expected=FLAGS)
        header = Header(0x10, 1, 2, 0, 1, 1, 0, 1)
        output = C.create_string_buffer(b'{}' + bytes(1022), 1024)
        before = output.raw
        used = C.c_size_t(99)
        self.assertEqual(self.lib.scb_encode(C.byref(header), output, TCP, output, 1024,
                                            C.byref(self.ws), C.byref(used)), ALIAS)
        self.assertEqual(used.value, 0)
        self.assertEqual(output.raw, before)

    def test_null_argument_and_missing_json_workspace(self):
        view = Frame()
        self.assertEqual(self.lib.scb_decode(None, 32, TCP, None, C.byref(view)), ARGUMENT)
        self.assertEqual(self.lib.scb_decode(b'x' * 32, 32, TCP, None, None), ARGUMENT)
        valid = frame(0x10, b'{}', 1)
        self.assertEqual(self.lib.scb_decode(valid, len(valid), TCP, None, C.byref(view)), JSON)
        self.assertFalse(view.payload)
        # No JSON scratch is needed for binary/IDLE frames.
        self.assertEqual(self.lib.scb_decode(frame(), 32, TCP, None, C.byref(view)), OK)
        used = C.c_size_t(999)
        self.assertEqual(self.lib.scb_encode(None, None, TCP, None, 0, None, C.byref(used)), ARGUMENT)
        self.assertEqual(used.value, 0)

    def test_frame_ack_and_request_id_have_no_business_side_effect(self):
        for body in (b'{"status":"RECEIVED"}', b'{"status":"APPLIED"}', b'{"status":"SAVED"}'):
            original = frame(0x11, body, 1, request=0x87654321)
            header, payload = self.decode(original)
            self.assertEqual((header.request_id, header.ack_seq, payload), (0x87654321, 11, body))

    def test_valid_crc_invalid_json_never_yields_partial_frame(self):
        for kind in (1, 2, 0x10, 0x11, 0x12):
            for body in (b'bad', b'{"op":"GET_STATUS"}junk', b'{"a":1,"a":2}',
                         b'{"x":1e999}', b'{"x":"\xc0\xaf"}', b'{"x":1', b''):
                with self.subTest(kind=kind, body=body):
                    self.decode(frame(kind, body, int(kind >= 0x10)), expected=JSON)


class JsonTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls): cls.lib = load()

    def setUp(self): self.ws = Workspace()

    def check(self, payload, expected=True):
        if isinstance(payload, str): payload = payload.encode('utf-8')
        error = self.lib.bridge_json_validate(payload, len(payload), C.byref(self.ws))
        self.assertEqual(error == 0, expected, repr(payload[:150]) + ': error=' + str(error))
        return error

    def test_valid_syntax_and_complete_values(self):
        for body in ('{}', '[]', 'null', 'true', 'false', '0', '-0', '1.2e-3',
                     ' \r\n{"x":[true,false,null,0,-1.5e3],"b":{}} \t',
                     '{"é":"中😀","emoji":"\\ud83d\\ude00","slash":"\\/","nul":"\\u0000"}'):
            with self.subTest(body=body): self.check(body)

    def test_escaped_duplicate_keys_and_separate_objects(self):
        for body in ('{"a":1,"a":2}', '{"a":1,"\\u0061":2}',
                     '{"a":{"x":1,"x":2}}', '{"😀":1,"\\ud83d\\ude00":2}',
                     '{"a\\u0000":1,"a\\u0000":2}', '{"":1,"":2}'):
            self.assertEqual(self.check(body, False), 4)
        for body in ('{"a":{"a":1},"b":{"a":2}}', '[{"x":1},{"x":2}]',
                     '{"a":[{"x":1}],"b":"a","é":1,"e\\u0301":2}'):
            self.check(body)

    def test_invalid_json_and_full_consumption(self):
        bodies = ('', ' ', '{', '[', '{"a":}', '{"a" 1}', '{"a":1,}', '[1,]',
                  '[,1]', '[1 2]', 'true false', '{}x', '01', '-01', '+1', '.1',
                  '1.', '1e', '--1', '1e+', 'NaN', 'Infinity', '-Infinity',
                  '"abc', '"\\x20"', '"\\u00zz"', '{1:2}', '"line\nbreak"', '{}\x00')
        for body in bodies:
            with self.subTest(body=body): self.check(body, False)

    def test_utf8_and_surrogate_errors(self):
        for raw in (b'"\x80"', b'"\xc0\xaf"', b'"\xc1\xbf"', b'"\xe0\x80\x80"',
                    b'"\xed\xa0\x80"', b'"\xf4\x90\x80\x80"', b'"\xf5\x80\x80\x80"',
                    b'"\xe2\x82"', b'"\xe2x\xac"', b'"\x00"', b'"\\ud800"', b'"\\udfff"',
                    b'"\\ud800\\u0041"', b'"\\ud800x"'):
            with self.subTest(raw=raw): self.check(raw, False)

    def test_numbers_finite_limits_underflow_and_exact_grammar(self):
        for body in ('1.7976931348623157e308', '-1.7976931348623157e308',
                     '1.7976931348623158e308', '1e-999', '-0e999', '0e999', '9' * 300):
            self.check(body)
        for body in ('1e309', '-1e999', '1.7976931348623159e308', '9' * 400, '{"x":1e999}'):
            self.assertEqual(self.check(body, False), 5)

    def test_exact_overflow_boundary_and_decimal_differential(self):
        threshold = 2**1024 - 2**970
        for integer, accepted in ((threshold - 1, True), (threshold, False), (threshold + 1, False)):
            for sign in ('', '-'):
                self.check(sign + str(integer), accepted)
        rng = random.Random(0x53434231)
        for _ in range(1000):
            digits = ''.join(str(rng.randrange(10)) for _ in range(rng.randrange(1, 480)))
            split = rng.randrange(len(digits))
            text = ('-' if rng.randrange(2) else '') + '0.' + digits[:split] + digits[split:] + 'e' + str(rng.randrange(-1200, 1200))
            self.check(text, math.isfinite(float(text)))

    def test_deepest_fitting_input_and_length_limits(self):
        self.check('[' * 496 + ']' * 496)
        self.check('[' * 495 + '0' + ']' * 495)
        self.check('"' + 'x' * 990 + '"')
        self.check('"' + 'x' * 991 + '"', False)
        self.check('[' * 496 + '0' + ']' * 496, False)
        for length in range(1, 70): self.check(('[' * 70 + ']' * 70)[:length], False)

    def test_object_lookup_validates_everything_before_returning(self):
        for name, expected in ((b'op', b'"GET_STATUS"'), (b'args', b'{"x":[1,{"y":2}]}'),
                               (b'bool', b'true'), (b'n', b'null')):
            body = C.create_string_buffer(b'{"\\u006fp":"GET_STATUS","args":{"x":[1,{"y":2}]},"bool":true,"n":null}')
            view = View()
            self.assertTrue(self.lib.bridge_json_object_get(body, len(body.value), name, C.byref(self.ws), C.byref(view)))
            self.assertEqual(C.string_at(view.bytes, view.length), expected)
        for body, key in ((b'{"op":1,"op":2}', b'op'), (b'{"op":1}x', b'op'),
                          (b'{"op":1,"other":1e999}', b'op'), (b'[]', b'op'), (b'{}', b'absent')):
            view = View(0x1234, 100)
            self.assertFalse(self.lib.bridge_json_object_get(body, len(body), key, C.byref(self.ws), C.byref(view)))
            self.assertEqual((view.bytes, view.length), (None, 0))

    def test_invalid_workspace_and_view_arguments(self):
        self.assertNotEqual(self.lib.bridge_json_validate(b'{}', 2, None), 0)
        self.assertNotEqual(self.lib.bridge_json_validate(None, 2, C.byref(self.ws)), 0)
        self.assertFalse(self.lib.bridge_json_object_get(b'{}', 2, b'x', C.byref(self.ws), None))

    def test_differential_json_mutations(self):
        rng = random.Random(20261018)

        def pairs(items):
            result = {}
            for key, value in items:
                if key in result: raise ValueError('duplicate')
                result[key] = value
            return result

        def finite(value):
            if isinstance(value, float) and not math.isfinite(value): raise ValueError('nonfinite')
            if isinstance(value, str) and any(0xd800 <= ord(c) <= 0xdfff for c in value): raise ValueError('surrogate')
            if isinstance(value, dict):
                for k, v in value.items(): finite(k); finite(v)
            elif isinstance(value, list):
                for v in value: finite(v)

        corpus = [b'{"op":"GET_STATUS","args":{}}', b'[null,true,false,-1.25e+3,"\\u00e9"]',
                  '{"é":{"x":1},"emoji":"😀"}'.encode(), b'{"x":0,"y":[1,2,3]}']
        for i in range(1200):
            raw = bytearray(rng.choice(corpus))
            if i < 1000:
                position = rng.randrange(len(raw))
                action = rng.randrange(3)
                if action == 0: raw[position] = rng.randrange(256)
                elif action == 1: del raw[position]
                else: raw.insert(position, rng.randrange(256))
            else:
                raw = bytearray(json.dumps({str(i): [rng.randint(-99999, 99999), rng.random(), '中😀']},
                                           ensure_ascii=bool(i % 2)).encode())
            try:
                parsed = json.loads(raw.decode('utf-8'), object_pairs_hook=pairs,
                                    parse_constant=lambda _: (_ for _ in ()).throw(ValueError()))
                finite(parsed)
                accepted = True
            except (ValueError, UnicodeError): accepted = False
            self.check(bytes(raw), accepted)


if __name__ == '__main__': unittest.main()
