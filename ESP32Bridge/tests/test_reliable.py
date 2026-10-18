"""Independent wire inputs and injected monotonic time against production C."""
import ctypes as C
import unittest
from link_host import Link, Frame, Header, Workspace, library
from test_protocol import frame, mutate, telemetry

OK, DELIVER, DUPLICATE, BACKPRESSURE, STALE, INVALID, BUSY, RESEND, REBUILD = range(9)


class ReliableTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.lib = library()

    def setUp(self):
        self.ws, self.link = Workspace(), Link()
        self.lib.scb_link_reset(C.byref(self.link), 1, 51)

    def receive(self, blob=None, reserved=True, expected=DELIVER, link=None):
        link = link if link is not None else self.link
        blob = blob if blob is not None else frame(0x10, b'{"op":"GET_STATUS","args":{}}', flags=1, session=51, seq=1, ack=0, request=10)
        source, view = C.create_string_buffer(blob), Frame()
        result = self.lib.scb_link_receive(C.byref(link), source, len(blob), reserved, C.byref(self.ws), C.byref(view))
        self.assertEqual(result, expected)
        if result != DELIVER:
            self.assertEqual(bytes(view), bytes(C.sizeof(view)))
        else:
            self.assertEqual(C.string_at(view.payload, view.header.payload_len), blob[32:32 + view.header.payload_len])
        return blob

    def prepare(self, reliable=True, expected=OK, link=None, capacity=1024):
        link = link if link is not None else self.link
        payload = b'{"op":"GET_STATUS","args":{}}' if reliable else b''
        h = Header(0x10 if reliable else 3, int(reliable), len(payload), 0, 999, 999, 999, 10 if reliable else 0)
        out, used = C.create_string_buffer(1024), C.c_size_t(999)
        result = self.lib.scb_link_prepare(C.byref(link), C.byref(h), C.create_string_buffer(payload),
                                         out, capacity, C.byref(self.ws), C.byref(used))
        self.assertEqual(result, expected)
        if result != OK:
            self.assertEqual(used.value, 0)
        return bytes(out[:used.value])

    def sent(self, now, link=None):
        link = link if link is not None else self.link
        self.assertTrue(self.lib.scb_link_sent(C.byref(link), link.tx_seq, now))

    def poll(self, now, expected=OK, link=None):
        link = link if link is not None else self.link
        pointer, length = C.c_void_p(), C.c_size_t(999)
        self.assertEqual(self.lib.scb_link_poll(C.byref(link), now, C.byref(pointer), C.byref(length)), expected)
        if expected == RESEND:
            return C.string_at(pointer, length.value)
        self.assertFalse(pointer.value)
        self.assertEqual(length.value, 0)

    def test_reservation_before_ack_and_retry_after_capacity_available(self):
        blob = self.receive(reserved=False, expected=BACKPRESSURE)
        self.assertEqual(self.link.ack_seq, 0)
        self.assertEqual(self.link.rx_length, 0)
        self.receive(blob)
        self.assertEqual(self.link.ack_seq, 1)
        self.receive(blob, reserved=False, expected=DUPLICATE)

    def test_exact_ack_only_and_never_before_first_send(self):
        self.prepare()
        self.receive(frame(session=51, seq=1, ack=1))
        self.assertTrue(self.link.pending)
        self.sent(100)
        for seq, ack in ((2, 0), (3, 2), (4, 0xffffffff)):
            self.receive(frame(session=51, seq=seq, ack=ack))
            self.assertTrue(self.link.pending)
        self.receive(frame(session=51, seq=5, ack=1))
        self.assertFalse(self.link.pending)
        self.prepare()
        self.assertEqual(self.link.tx_seq, 2)

    def test_exact_199_200_and_three_retries_immutable_bytes(self):
        original = self.prepare()
        self.sent(1000)
        for retry, now in enumerate((1200, 1400, 1600), 1):
            self.poll(now - 1)
            self.assertEqual(self.poll(now, RESEND), original)
            self.assertEqual(self.link.retries, retry - 1)
            self.sent(now)
            self.assertEqual(self.link.retries, retry)
        self.poll(1799)
        self.poll(1800, REBUILD)
        self.prepare(expected=REBUILD)

    def test_partial_send_has_no_timeout_until_completion(self):
        self.prepare()
        self.poll(1000000)
        self.assertFalse(self.link.sent)
        self.sent(1000000)
        original = self.poll(1000200, RESEND)
        self.poll(2000000)
        self.assertEqual(self.link.retries, 0)
        self.assertEqual(bytes(self.link.tx[:self.link.tx_length]), original)
        self.sent(2000000)
        self.poll(2000199)
        self.poll(2000200, RESEND)

    def test_idle_passes_occupied_window_and_new_ack_does_not_edit_retry(self):
        original = self.prepare()
        self.sent(0)
        self.prepare(expected=BUSY)
        self.receive()
        idle = self.prepare(False)
        self.assertEqual(int.from_bytes(idle[20:24], 'little'), 1)
        self.assertEqual(self.poll(200, RESEND), original)

    def test_nonreliable_interleaving_does_not_hide_first_or_duplicate_reliable(self):
        self.receive(frame(0x20, telemetry(1), count=1, session=51, seq=20, ack=0))
        blob = self.receive()
        self.receive(frame(session=51, seq=21, ack=0))
        self.receive(blob, expected=DUPLICATE)
        self.assertEqual(self.link.rx_high, 21)
        self.assertEqual(self.link.ack_seq, 1)

    def test_old_reliable_cannot_repeat_after_a_new_reliable(self):
        old = self.receive()
        self.receive(mutate(old, 16, 3, 'I'))
        self.receive(old, expected=STALE)
        self.assertEqual(self.link.ack_seq, 3)

    def test_duplicate_identity_includes_header_and_all_payload_bytes(self):
        original = self.receive()
        for offset, value, fmt in ((20, 99, 'I'), (24, 99, 'I'), (6, 0, 'H')):
            with self.subTest(offset=offset):
                self.receive(mutate(original, offset, value, fmt), expected=INVALID)
        changed = original[32:].replace(b'GET_STATUS', b'GET_PARAMS')
        self.receive(frame(0x10, changed, flags=1, session=51, seq=1, ack=0, request=10), expected=INVALID)
        self.assertEqual(self.link.ack_seq, 1)

    def test_invalid_crc_json_session_seq_cannot_release_tx_or_ack_rx(self):
        self.prepare()
        self.sent(0)
        valid = frame(0x10, b'{}', flags=1, session=51, seq=1, ack=1, request=10)
        bad_crc = mutate(valid, 28, 0, 'I', recalc=False)
        for blob in (bad_crc, mutate(valid, 12, 50, 'I'), mutate(valid, 16, 0, 'I'),
                     mutate(valid, 16, 0xffffffff, 'I'), frame(0x10, b'{', flags=1, session=51, seq=1, ack=1)):
            self.receive(blob, expected=INVALID)
            self.assertTrue(self.link.pending)
            self.assertEqual(self.link.ack_seq, 0)

    def test_full_rx_slot_still_allows_opposite_direction_exact_ack(self):
        self.prepare()
        self.sent(0)
        blob = frame(0x10, b'{}', flags=1, session=51, seq=1, ack=1)
        self.receive(blob, reserved=False, expected=BACKPRESSURE)
        self.assertFalse(self.link.pending)
        self.assertEqual(self.link.ack_seq, 0)

    def test_spi_tcp_windows_and_timers_are_independent(self):
        other = Link()
        self.lib.scb_link_reset(C.byref(other), 0, 51)
        tcp_image, spi_image = self.prepare(), self.prepare(link=other)
        self.assertEqual(len(spi_image), 1024)
        self.assertEqual(spi_image[:len(tcp_image)], tcp_image)
        self.sent(0)
        self.sent(100, other)
        self.poll(200, RESEND)
        self.poll(200, link=other)
        self.receive(frame(session=51, seq=1, ack=1))
        self.assertTrue(other.pending)
        self.poll(300, RESEND, other)

    def test_sequence_wrap_rebuild_and_reset_clears_all_identity(self):
        self.link.next_seq = 0xfffffffe
        self.prepare(False)
        self.prepare(False, expected=REBUILD)
        self.lib.scb_link_reset(C.byref(self.link), 1, 52)
        self.assertFalse(self.link.dead)
        self.assertEqual(self.link.next_seq, 1)
        self.assertEqual(self.link.ack_seq, 0)
        self.receive(frame(session=51, seq=1), expected=INVALID)

    def test_failed_encoding_does_not_consume_seq_or_window(self):
        self.prepare(capacity=10, expected=INVALID)
        self.assertEqual(self.link.next_seq, 1)
        self.assertFalse(self.link.pending)
        self.prepare()

    def test_duplicate_unreliable_is_not_dispatched_again(self):
        blob = self.receive(frame(session=51, seq=3, ack=0))
        self.receive(blob, expected=STALE)

    def test_backward_clock_does_not_trigger_false_timeout(self):
        self.prepare()
        self.sent(1000)
        self.poll(999)
        self.poll(1199)
        self.poll(1200, RESEND)

    def test_wrong_send_completions_and_late_completion_are_harmless(self):
        self.prepare()
        self.assertFalse(self.lib.scb_link_sent(C.byref(self.link), 2, 0))
        self.sent(100)
        self.assertFalse(self.lib.scb_link_sent(C.byref(self.link), 1, 200))
        self.assertEqual(self.link.last_send_ms, 100)
        self.receive(frame(session=51, seq=1, ack=1))
        self.assertFalse(self.lib.scb_link_sent(C.byref(self.link), 1, 300))

    def test_zero_session_or_udp_is_not_a_reliable_link(self):
        for transport, session in ((1, 0), (2, 51), (99, 51)):
            self.lib.scb_link_reset(C.byref(self.link), transport, session)
            self.prepare(expected=REBUILD)

    def test_same_business_request_id_new_transport_identity_is_delivered(self):
        blob = self.receive()
        self.receive(mutate(blob, 16, 2, 'I'))
        self.assertEqual(self.link.ack_seq, 2)
