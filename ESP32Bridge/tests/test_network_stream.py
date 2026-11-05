"""Bounded TCP stream behavior of the production network task.

All stream assembly, handshake, reliability and closing decisions execute in
production C (net_bridge + protocol/json_guard/reliable_link/link_session).
Python only scripts socket-fixture events and independently re-parses the
captured bytes with struct/zlib/json.
"""
import json
import unittest
import zlib

from test_protocol import WIRE, frame, mutate
from net_host import (Script, run, captured, load, RECV_EOF, RECV_EAGAIN, RECV_ERROR,
                      SEND_EAGAIN, SEND_ZERO, SEND_ERROR, BUSY_SEND, BUSY_SEND_EAGAIN)

SESSION = 0x12345678
NEW_SESSION = 0x2468ACE0
PEER_IP = 0xC0A80464


def pc_hello():
    return frame(1, b'{"role":"pc","protocol":1}', session=0, seq=0, ack=0)


def subscribe(request=7, seq=1, session=SESSION):
    return frame(0x10, b'{"op":"SUBSCRIBE","args":{"udp_port":5001}}', 1,
                 session=session, seq=seq, ack=0, request=request)


def parse_stream(blob):
    frames = []
    while blob:
        fields = WIRE.unpack(blob[:32])
        length = 32 + fields[4]
        assert len(blob) >= length, 'truncated captured frame'
        frames.append(blob[:length])
        blob = blob[length:]
    return frames


def decode(blob):
    fields = WIRE.unpack(blob[:32])
    payload = blob[32:32 + fields[4]]
    assert len(blob) == 32 + fields[4], 'TCP frame must carry no SPI padding'
    assert fields[0:2] == (b'SCB1', 1)
    assert fields[10] == zlib.crc32(payload, zlib.crc32(blob[:28])), 'independent CRC failed'
    return dict(type=fields[2], flags=fields[3], count=fields[5], session=fields[6],
                seq=fields[7], ack=fields[8], request=fields[9], payload=payload)


def hello_script():
    return Script().accept(PEER_IP).recv(pc_hello()).send()


def expected_hello_ack(session=SESSION, ready=True):
    body = b'{"protocol":1,"session_id":%d,"link_ready":%s}' % (session, b'true' if ready else b'false')
    return frame(2, body, session=session, seq=0, ack=0)


def expected_busy(session=SESSION, ready=True):
    body = b'{"protocol":1,"session_id":%d,"link_ready":%s,"status":"BUSY"}' % (
        session, b'true' if ready else b'false')
    return frame(2, body, session=session, seq=0, ack=0)


class NetworkStreamTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.lib = load()

    def test_listen_config_idle_snapshot_and_bounded_poll(self):
        snapshot = run(self.lib, Script())
        self.assertTrue(snapshot.listening)
        self.assertFalse(snapshot.control_open)
        self.assertEqual(snapshot.init_error, 0)
        self.assertEqual(self.lib.bridge_net_metric(2), 1)      # one bounded poll
        self.assertLessEqual(self.lib.bridge_net_metric(9), 10)
        self.assertEqual(self.lib.bridge_net_metric(3), 0)      # no accepts

    def test_hello_ack_reports_real_session_and_link_ready(self):
        snapshot = run(self.lib, hello_script())
        self.assertEqual((snapshot.connected, snapshot.received), (1, 1))
        blob = captured(self.lib)
        self.assertEqual(blob, expected_hello_ack())  # no padding, exact bytes
        ack = decode(blob)
        self.assertEqual((ack['type'], ack['flags'], ack['count']), (2, 0, 0))
        self.assertEqual((ack['seq'], ack['ack'], ack['request']), (0, 0, 0))
        self.assertEqual(json.loads(ack['payload']),
                         dict(protocol=1, session_id=SESSION, link_ready=True))
        self.assertEqual(snapshot.sent_bytes, len(blob))

    def test_hello_ack_reports_zero_session_and_not_ready(self):
        # Idle iterations after the send prove the status-only connection is
        # still open once the loop polls the dead link again (real hardware
        # runs forever; the script must not stop right after the send).
        snapshot = run(self.lib, hello_script().idle(5), spi_session=0, spi_ready=False)
        ack = decode(parse_stream(captured(self.lib))[0])
        self.assertEqual(json.loads(ack['payload']), dict(protocol=1, session_id=0, link_ready=False))
        self.assertTrue(snapshot.control_open)  # status-only connection stays open
        self.assertEqual(snapshot.link_failures, 0)  # a session-less dead link is not a failure

    def test_business_frame_without_session_is_refused(self):
        # No current session: a reliable COMMAND is not taken over; the dead
        # link refusal closes the connection without a fabricated RESULT.
        script = hello_script().recv(subscribe(session=0)).idle(3)
        snapshot = run(self.lib, script, spi_session=0, spi_ready=False)
        self.assertFalse(snapshot.control_open)
        self.assertEqual(snapshot.invalid_closes, 1)
        self.assertEqual(snapshot.link_failures, 0)
        self.assertEqual(snapshot.subscriptions, 0)
        self.assertEqual([decode(f)['type'] for f in parse_stream(captured(self.lib))], [2])

    def test_zero_session_hold_busy_disconnect_reconnect_and_business_refusal(self):
        # One continuous trace of the hardware failure and recovery sequence.
        # A two-second quiet connection must retain ownership of the control
        # slot, and BUSY must not depend on either client sending another frame.
        script = hello_script().idle(201)
        script.accept(PEER_IP + 1).push(BUSY_SEND, 1024).idle(3)
        script.push(RECV_EOF).accept(PEER_IP).recv(pc_hello()).send().idle(5)
        script.recv(subscribe(session=0)).idle(3)
        script.accept(PEER_IP).recv(pc_hello()).send().idle(5)
        snapshot = run(self.lib, script, spi_session=0, spi_ready=False)
        self.assertGreaterEqual(self.lib.bridge_net_metric(8), 2000)
        self.assertEqual(snapshot.connected, 3)
        self.assertEqual(snapshot.rejected, 1)
        self.assertEqual(snapshot.invalid_closes, 1)
        self.assertEqual(snapshot.link_failures, 0)
        self.assertEqual(snapshot.subscriptions, 0)
        self.assertTrue(snapshot.control_open)
        self.assertEqual(captured(self.lib, busy=True), expected_busy(0, False))
        self.assertEqual(captured(self.lib), expected_hello_ack(0, False) * 3)
        # Only the second client, voluntarily closed first client, and refused
        # no-session business client close. The final HELLO stays connected.
        self.assertEqual([self.lib.bridge_net_close_event(i) for i in range(3)], [201, 200, 202])

    def test_byte_by_byte_split_reassembles_exactly_once(self):
        script = Script().accept(PEER_IP)
        for index in range(len(pc_hello())):
            script.recv(pc_hello()[index:index + 1])
        script.send()
        snapshot = run(self.lib, script)
        self.assertEqual(snapshot.received, 1)
        self.assertEqual(decode(parse_stream(captured(self.lib))[0])['type'], 2)

    def test_sticky_multiple_frames_dispatch_in_order(self):
        hello = pc_hello()
        idle = frame(session=SESSION, seq=1, ack=0)
        sub = subscribe(seq=2)
        script = Script().accept(PEER_IP).recv(hello + idle + sub).send().send()
        snapshot = run(self.lib, script)
        self.assertEqual(snapshot.received, 3)
        self.assertEqual(snapshot.subscriptions, 1)
        frames = parse_stream(captured(self.lib))
        self.assertEqual([decode(f)['type'] for f in frames], [2, 0x11])
        result = decode(frames[1])
        self.assertEqual(json.loads(result['payload']), dict(op='SUBSCRIBE', status='OK'))
        self.assertEqual((result['request'], result['ack']), (7, 2))

    def test_bad_magic_and_bad_type_close_without_dispatch(self):
        bad_magic = mutate(pc_hello(), 0, ord('X'))
        bad_type = mutate(pc_hello(), 5, 0x13)
        for blob in (bad_magic, bad_type):
            snapshot = run(self.lib, Script().accept(PEER_IP).recv(blob))
            self.assertEqual(snapshot.invalid_closes, 1)
            self.assertEqual(snapshot.received, 0)
            self.assertFalse(snapshot.control_open)
            self.assertEqual(captured(self.lib), b'')

    def test_bad_crc_and_invalid_json_close_connection(self):
        bad_crc = mutate(subscribe(), 28, 0, 'I', recalc=False)
        bad_json = frame(0x10, b'{', 1, session=SESSION, seq=1, request=7)
        for blob, crc_count in ((bad_crc, 1), (bad_json, 0)):
            snapshot = run(self.lib, hello_script().recv(blob))
            self.assertEqual(snapshot.crc_errors, crc_count)
            self.assertEqual(snapshot.invalid_closes, 1 - crc_count)
            self.assertFalse(snapshot.control_open)

    def test_overlength_claim_closes_at_header_without_payload(self):
        header = bytearray(subscribe()[:32])
        header[8] = 993 & 0xFF
        header[9] = 993 >> 8
        snapshot = run(self.lib, Script().accept(PEER_IP).recv(bytes(header)))
        self.assertEqual(snapshot.invalid_closes, 1)
        self.assertFalse(snapshot.control_open)
        self.assertEqual(snapshot.received, 0)

    def test_truncated_frame_then_peer_close_is_clean(self):
        snapshot = run(self.lib, Script().accept(PEER_IP).recv(pc_hello()[:20]).push(RECV_EOF))
        self.assertEqual(snapshot.received, 0)
        self.assertEqual(snapshot.invalid_closes, 0)
        self.assertFalse(snapshot.control_open)

    def test_frame_before_hello_closes(self):
        snapshot = run(self.lib, Script().accept(PEER_IP).recv(subscribe()))
        self.assertEqual(snapshot.invalid_closes, 1)
        self.assertEqual(snapshot.subscriptions, 0)
        self.assertEqual(captured(self.lib), b'')

    def test_busy_refusal_frame_and_first_connection_untouched(self):
        script = hello_script().accept(PEER_IP + 1).push(BUSY_SEND, 1024)
        script.recv(frame(session=SESSION, seq=3, ack=0))
        snapshot = run(self.lib, script)
        self.assertEqual(snapshot.rejected, 1)
        self.assertTrue(snapshot.control_open)   # first connection still served
        self.assertEqual(snapshot.received, 2)   # its later IDLE still consumed
        busy = captured(self.lib, busy=True)
        self.assertEqual(busy, expected_busy())
        refusal = decode(busy)
        self.assertEqual((refusal['type'], refusal['flags'], refusal['count']), (2, 0, 0))
        self.assertEqual((refusal['seq'], refusal['ack'], refusal['request']), (0, 0, 0))
        self.assertEqual(refusal['session'], SESSION)
        self.assertEqual(json.loads(refusal['payload']),
                         dict(protocol=1, session_id=SESSION, link_ready=True, status='BUSY'))
        self.assertEqual(self.lib.bridge_net_close_event(0), 201)  # only the second fd closed

    def test_busy_short_writes_complete_then_close(self):
        script = hello_script().accept(PEER_IP + 1)
        for _ in range((len(expected_busy()) + 4) // 5):
            script.push(BUSY_SEND, 5)
        snapshot = run(self.lib, script)
        self.assertEqual(snapshot.rejected, 1)
        self.assertGreater(snapshot.short_writes, 0)
        self.assertEqual(captured(self.lib, busy=True), expected_busy())
        self.assertTrue(snapshot.control_open)

    def test_busy_deadline_closes_partial_refusal(self):
        script = hello_script().accept(PEER_IP + 1).push(BUSY_SEND_EAGAIN).idle(51)
        snapshot = run(self.lib, script)
        self.assertEqual(snapshot.rejected, 1)
        self.assertEqual(captured(self.lib, busy=True), b'')
        self.assertGreaterEqual(self.lib.bridge_net_metric(8), 500)
        self.assertTrue(snapshot.control_open)

    def test_third_connection_closed_immediately_while_busy_pending(self):
        script = hello_script().accept(PEER_IP + 1).accept(PEER_IP + 2)
        snapshot = run(self.lib, script)
        self.assertEqual(snapshot.rejected, 2)
        self.assertEqual(self.lib.bridge_net_close_event(0), 202)
        self.assertEqual(captured(self.lib, busy=True), b'')  # busy not yet sent
        self.assertTrue(snapshot.control_open)

    def test_short_write_cursor_never_repeats_or_drops_bytes(self):
        expected = expected_hello_ack()
        for cap in (1, 3, 7):
            script = Script().accept(PEER_IP).recv(pc_hello())
            for _ in range((len(expected) + cap - 1) // cap):
                script.send(cap)
            snapshot = run(self.lib, script)
            self.assertEqual(captured(self.lib), expected)
            self.assertGreater(snapshot.short_writes, 0)

    def test_send_zero_and_eagain_do_not_lose_progress(self):
        script = hello_script()
        script.entries[2:3] = []  # drop the plain send
        script.push(SEND_ZERO).push(SEND_EAGAIN).send()
        snapshot = run(self.lib, script)
        self.assertEqual(snapshot.short_writes, 1)
        self.assertEqual(decode(parse_stream(captured(self.lib))[0])['type'], 2)

    def test_send_error_and_recv_error_close_connection(self):
        for failing in (SEND_ERROR,):
            snapshot = run(self.lib, Script().accept(PEER_IP).recv(pc_hello()).push(failing))
            self.assertFalse(snapshot.control_open)
        snapshot = run(self.lib, hello_script().push(RECV_ERROR))
        self.assertFalse(snapshot.control_open)

    def test_stalled_frame_closes_connection_at_two_seconds(self):
        snapshot = run(self.lib, Script().accept(PEER_IP).recv(pc_hello()).idle(201))
        self.assertEqual(snapshot.stalls, 1)
        self.assertFalse(snapshot.control_open)
        self.assertGreaterEqual(self.lib.bridge_net_metric(8), 2000)

    def test_reliable_result_resends_exact_image_then_closes(self):
        script = hello_script().recv(subscribe()).send()
        for _ in range(3):
            script.idle(21).send()
        script.idle(21)
        snapshot = run(self.lib, script)
        self.assertEqual(snapshot.link_failures, 1)
        self.assertFalse(snapshot.control_open)
        frames = parse_stream(captured(self.lib))
        self.assertEqual(len(frames), 5)
        self.assertEqual(decode(frames[0])['type'], 2)
        for resend in frames[2:]:
            self.assertEqual(resend, frames[1])  # byte-identical reliable image
        self.assertLessEqual(self.lib.bridge_net_metric(9), 10)

    def test_exact_ack_releases_pending_without_resend(self):
        script = hello_script().recv(subscribe()).send()
        script.recv(frame(session=SESSION, seq=2, ack=1)).idle(30)
        snapshot = run(self.lib, script)
        frames = parse_stream(captured(self.lib))
        self.assertEqual([decode(f)['type'] for f in frames], [2, 0x11])
        self.assertEqual(snapshot.link_failures, 0)
        self.assertTrue(snapshot.control_open)

    def test_duplicate_reliable_confirmed_by_idle_without_redelivery(self):
        script = hello_script().recv(subscribe()).send().recv(subscribe()).send().idle()
        snapshot = run(self.lib, script)
        self.assertEqual(snapshot.subscriptions, 1)
        frames = parse_stream(captured(self.lib))
        self.assertEqual([decode(f)['type'] for f in frames], [2, 0x11, 3])
        self.assertEqual(decode(frames[2])['ack'], 1)

    def test_other_commands_are_real_backpressure_never_acked(self):
        command = frame(0x10, b'{"op":"GET_STATUS","args":{}}', 1, session=SESSION, seq=1, request=9)
        snapshot = run(self.lib, hello_script().recv(command).recv(command).idle())
        self.assertEqual(snapshot.backpressure, 2)
        self.assertEqual(snapshot.received, 3)
        frames = parse_stream(captured(self.lib))
        self.assertEqual(len(frames), 1)  # HELLO_ACK only; no fabricated RESULT
        self.assertEqual(decode(frames[0])['ack'], 0)
        self.assertTrue(snapshot.control_open)

    def test_session_change_closes_tcp_and_reconnect_needs_hello(self):
        script = hello_script()
        script.set_spi(NEW_SESSION, True).idle()
        script.accept(PEER_IP + 1).recv(subscribe(session=SESSION))  # stale, pre-HELLO
        script.accept(PEER_IP + 2).recv(pc_hello()).send()
        snapshot = run(self.lib, script)
        self.assertEqual(snapshot.session_closes, 1)
        self.assertEqual(snapshot.invalid_closes, 1)
        frames = parse_stream(captured(self.lib))
        self.assertEqual(len(frames), 2)
        second = decode(frames[1])
        self.assertEqual(second['session'], NEW_SESSION)
        self.assertEqual(json.loads(second['payload'])['session_id'], NEW_SESSION)
        self.assertTrue(snapshot.control_open)

    def test_ap_parameters_dhcp_and_no_open_fallback(self):
        self.assertEqual(self.lib.bridge_net_ap_test(0), 0)

    def test_ap_step_failures_return_error_and_rollback(self):
        for scenario in range(1, 10):
            with self.subTest(scenario=scenario):
                self.assertEqual(self.lib.bridge_net_ap_test(scenario), 0,
                                 'AP failure scenario %d fixture line' % scenario)

    def test_listen_setup_failures_stop_only_network_service(self):
        for scenario in range(1, 6):
            with self.subTest(scenario=scenario):
                self.assertEqual(self.lib.bridge_net_start_fail_test(scenario), 0)

    def test_generation_credential_rejects_stale_udp_work(self):
        self.assertEqual(self.lib.bridge_net_generation_test(), 0)


if __name__ == '__main__':
    unittest.main()
