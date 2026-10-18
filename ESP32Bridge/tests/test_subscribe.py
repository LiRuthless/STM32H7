"""SUBSCRIBE validation and UDP telemetry egress of the production network task.

All decoding, validation, subscription state, generation credentials and the
UDP outlet decision execute in production C. Python scripts fixture events and
independently re-parses captured streams/datagrams; it never reimplements the
network behavior.
"""
import json
import unittest

from test_protocol import frame, telemetry
from test_network_stream import SESSION, PEER_IP, hello_script, parse_stream, decode
from net_host import Script, run, captured, datagrams, load, RECV_EOF, UDP_FAIL, UNBIND

ESP_ERR_INVALID_ARG = 0x102
ESP_ERR_INVALID_STATE = 0x103


def subscribe(payload, request=7, seq=1):
    return frame(0x10, payload, 1, session=SESSION, seq=seq, ack=0, request=request)


def sub_port(value, request=7):
    return subscribe(b'{"op":"SUBSCRIBE","args":{"udp_port":%s}}' % value, request)


def result_of(blob):
    frames = parse_stream(blob)
    assert len(frames) == 2, 'expected HELLO_ACK plus one RESULT'
    result = decode(frames[1])
    assert result['type'] == 0x11 and result['flags'] == 1
    return result


class SubscribeValidationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.lib = load()

    def check(self, payload, status, request=7):
        script = hello_script().recv(subscribe(payload, request)).send()
        snapshot = run(self.lib, script)
        result = result_of(captured(self.lib))
        self.assertEqual(json.loads(result['payload']), dict(op='SUBSCRIBE', status=status))
        self.assertEqual(result['request'], request)  # request_id echoed verbatim
        self.assertEqual(snapshot.subscriptions, int(status == 'OK'))
        self.assertTrue(snapshot.control_open)  # an invalid argument never closes
        return snapshot

    def test_udp_port_boundaries(self):
        self.check(b'{"op":"SUBSCRIBE","args":{"udp_port":0}}', 'INVALID_ARG')
        snapshot = self.check(b'{"op":"SUBSCRIBE","args":{"udp_port":1}}', 'OK')
        self.assertEqual(snapshot.udp_port, 1)
        snapshot = self.check(b'{"op":"SUBSCRIBE","args":{"udp_port":65535}}', 'OK')
        self.assertEqual(snapshot.udp_port, 65535)
        self.assertTrue(snapshot.subscribed)
        self.check(b'{"op":"SUBSCRIBE","args":{"udp_port":65536}}', 'INVALID_ARG')

    def test_udp_port_must_be_an_unsigned_integer(self):
        for literal in (b'-1', b'1.5', b'true', b'"5001"', b'null', b'[5001]', b'{}'):
            with self.subTest(literal=literal):
                self.check(b'{"op":"SUBSCRIBE","args":{"udp_port":%s}}' % literal, 'INVALID_ARG')

    def test_missing_or_nonobject_args_rejected(self):
        for payload in (b'{"op":"SUBSCRIBE"}',
                        b'{"op":"SUBSCRIBE","args":5}',
                        b'{"op":"SUBSCRIBE","args":{}}',
                        b'{"op":"SUBSCRIBE","args":{"port":5001}}'):
            with self.subTest(payload=payload):
                self.check(payload, 'INVALID_ARG')

    def test_zero_request_id_rejected(self):
        self.check(b'{"op":"SUBSCRIBE","args":{"udp_port":5001}}', 'INVALID_ARG', request=0)

    def test_case_and_whitespace_of_op_are_exact(self):
        for op in (b'"subscribe"', b'"SUBSCRIBE "', b'" SUBSCRIBE"'):
            with self.subTest(op=op):
                payload = b'{"op":%s,"args":{"udp_port":5001}}' % op
                # Not a SUBSCRIBE at all: an unknown reliable COMMAND is real
                # backpressure, never an ACK and never a fabricated RESULT.
                snapshot = run(self.lib, hello_script().recv(subscribe(payload)))
                self.assertEqual(snapshot.subscriptions, 0)
                self.assertEqual(snapshot.backpressure, 1)


class TelemetryEgressTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.lib = load()

    def subscribed(self, script=None, port=5001):
        script = (script or hello_script()).recv(sub_port(b'%d' % port)).send()
        return script

    def test_offer_after_subscribe_sends_exact_frame_to_tcp_peer(self):
        image = frame(0x20, telemetry(1), count=1)
        snapshot = run(self.lib, self.subscribed().offer(image))
        self.assertEqual((snapshot.subscriptions, snapshot.udp_sent), (1, 1))
        self.assertEqual((snapshot.udp_failed, snapshot.udp_dropped), (0, 0))
        self.assertEqual(self.lib.bridge_net_offer_result(0), 0)
        self.assertEqual(datagrams(self.lib), [(PEER_IP, 5001, image)])
        self.assertEqual(snapshot.peer_ipv4, PEER_IP)
        self.assertTrue(snapshot.subscribed)

    def test_maximum_1016_byte_frame_and_one_datagram_per_frame(self):
        first = frame(0x20, telemetry(10), count=10)
        second = frame(0x20, telemetry(1), count=1)
        self.assertEqual(len(first), 1016)
        script = self.subscribed().offer(first).offer(second)
        snapshot = run(self.lib, script)
        self.assertEqual(snapshot.udp_sent, 2)
        self.assertEqual(datagrams(self.lib), [(PEER_IP, 5001, first), (PEER_IP, 5001, second)])

    def test_frame_internal_ip_field_is_never_honored(self):
        payload = b'{"op":"SUBSCRIBE","args":{"udp_port":5001,"ip":"10.9.9.9","port":1}}'
        image = frame(0x20, telemetry(1), count=1)
        script = hello_script().recv(subscribe(payload)).send().offer(image)
        snapshot = run(self.lib, script)
        self.assertEqual(snapshot.udp_sent, 1)
        # Destination is always the TCP peer recorded at accept.
        self.assertEqual(datagrams(self.lib), [(PEER_IP, 5001, image)])

    def test_offer_without_subscription_is_dropped_and_counted(self):
        image = frame(0x20, telemetry(1), count=1)
        snapshot = run(self.lib, hello_script().offer(image))
        self.assertEqual(self.lib.bridge_net_offer_result(0), 0)  # offer accepted
        self.assertEqual((snapshot.udp_sent, snapshot.udp_dropped), (0, 1))
        self.assertEqual(datagrams(self.lib), [])

    def test_offer_after_disconnect_is_dropped(self):
        image = frame(0x20, telemetry(1), count=1)
        script = self.subscribed().push(RECV_EOF).offer(image)
        snapshot = run(self.lib, script)
        self.assertEqual((snapshot.udp_sent, snapshot.udp_dropped), (0, 1))
        self.assertEqual(datagrams(self.lib), [])
        self.assertFalse(snapshot.subscribed)
        self.assertEqual(snapshot.udp_port, 0)

    def test_reconnect_without_resubscribe_drops_stale_work(self):
        image = frame(0x20, telemetry(1), count=1)
        script = self.subscribed().push(RECV_EOF)
        script.accept(PEER_IP + 1).recv(frame(1, b'{"role":"pc","protocol":1}', session=0, seq=0, ack=0))
        script.send().offer(image)
        snapshot = run(self.lib, script)
        self.assertEqual((snapshot.udp_sent, snapshot.udp_dropped), (0, 1))
        self.assertEqual(datagrams(self.lib), [])

    def test_resubscribe_after_reconnect_resumes_delivery(self):
        image = frame(0x20, telemetry(1), count=1)
        script = self.subscribed().push(RECV_EOF)
        script.accept(PEER_IP + 1).recv(frame(1, b'{"role":"pc","protocol":1}', session=0, seq=0, ack=0))
        script.send().recv(sub_port(b'5002')).send().offer(image)
        snapshot = run(self.lib, script)
        self.assertEqual((snapshot.udp_sent, snapshot.udp_dropped), (1, 0))
        self.assertEqual(datagrams(self.lib), [(PEER_IP + 1, 5002, image)])

    def test_unbound_outlet_drops_and_counts(self):
        image = frame(0x20, telemetry(1), count=1)
        script = self.subscribed().push(UNBIND).offer(image)
        snapshot = run(self.lib, script)
        self.assertEqual((snapshot.udp_sent, snapshot.udp_dropped), (0, 1))
        self.assertEqual(datagrams(self.lib), [])

    def test_socket_send_failure_is_counted_never_blocking(self):
        image = frame(0x20, telemetry(1), count=1)
        script = self.subscribed().push(UDP_FAIL, 1).offer(image)
        snapshot = run(self.lib, script)
        self.assertEqual((snapshot.udp_sent, snapshot.udp_failed), (0, 1))
        self.assertEqual(datagrams(self.lib), [])
        self.assertTrue(snapshot.control_open)  # the control path is unaffected

    def test_invalid_offers_rejected_before_any_state_change(self):
        lib = self.lib
        lib.fixture_reset()
        lib.fixture_spi(SESSION, 1)
        good = frame(0x20, telemetry(1), count=1)
        # Wrong transport/type, truncated and oversized frames: INVALID_ARG.
        self.assertEqual(lib.bridge_net_telemetry_offer(frame(0x10, b'{}', 1), 34), ESP_ERR_INVALID_ARG)
        self.assertEqual(lib.bridge_net_telemetry_offer(good[:20], 20), ESP_ERR_INVALID_ARG)
        self.assertEqual(lib.bridge_net_telemetry_offer(good + b'x' * (1017 - len(good)), 1017),
                         ESP_ERR_INVALID_ARG)
        self.assertEqual(lib.bridge_net_telemetry_offer(good, len(good)), 0)
        # The static image is occupied until the network task consumes it.
        self.assertEqual(lib.bridge_net_telemetry_offer(good, len(good)), ESP_ERR_INVALID_STATE)


if __name__ == '__main__':
    unittest.main()
