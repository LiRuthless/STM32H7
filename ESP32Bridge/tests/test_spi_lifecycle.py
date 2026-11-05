"""Inject independent SCB bytes into the actual production SPI task loop.

The Python side schedules virtual hardware transactions; all application
state/handshakes/reliability/ownership are executed by production C.
"""
import ctypes as C
import json
import struct
import unittest
import zlib
from spi_host import Snapshot, load
from test_protocol import WIRE, frame, mutate, telemetry

SESSION = 0x12345678
HELLO = frame(1, b'{"role":"stm32","protocol":1}', session=0, seq=0, ack=0)
CONFIRM = frame(session=SESSION, seq=1, ack=0)
RESULT_PAYLOAD = b'{"op":"GET_STATUS","status":"OK"}'
RESULT = frame(0x11, RESULT_PAYLOAD, flags=1, session=SESSION, seq=2, ack=0, request=77)


def idle(seq, ack=0, session=SESSION):
    return frame(session=session, seq=seq, ack=ack)


class SpiLifecycleTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.lib = load()

    def trace(self, frames=(), *, mode=0, bits=None, waits=None, reserve_after=0, rng=SESSION, failure=0):
        count = len(frames)
        blob = b''.join(b + bytes(1024 - len(b)) for b in frames)
        bit_array = (C.c_uint32 * count)(*(bits if bits is not None else [8192] * count))
        timeouts = (C.c_uint32 * (count + 1))(*(waits if waits is not None else [0] * (count + 1)))
        result = self.lib.bridge_spi_run_trace(mode, C.create_string_buffer(blob), count, bit_array, timeouts,
                                              reserve_after, rng, failure)
        self.assertEqual(result, 0, 'C task fixture assertion at line ' + str(result))
        self.health = Snapshot()
        self.assertEqual(self.lib.bridge_spi_snapshot(C.byref(self.health)), 0)
        captures = []
        for index in range(self.metric(9)):
            out = C.create_string_buffer(1024)
            self.assertEqual(self.lib.bridge_spi_trace_capture(index, out, 1024), 1024)
            captures.append(bytes(out))
        return captures

    def metric(self, index):
        return self.lib.bridge_spi_trace_metric(index)

    def decode(self, blob):
        fields = WIRE.unpack(blob[:32])
        payload = blob[32:32 + fields[4]]
        self.assertEqual(fields[0:2], (b'SCB1', 1))
        self.assertEqual(fields[10], zlib.crc32(payload, zlib.crc32(blob[:28])))
        self.assertEqual(blob[32 + fields[4]:], bytes(1024 - 32 - fields[4]))
        return dict(type=fields[2], session=fields[6], seq=fields[7], ack=fields[8], request=fields[9], payload=payload)

    def test_real_task_handshake_responds_in_subsequent_transaction(self):
        tx = self.trace([HELLO, CONFIRM])
        self.assertEqual(self.decode(tx[0])['type'], 3)
        self.assertEqual(self.decode(tx[0])['session'], 0)
        ack = self.decode(tx[1])
        self.assertEqual(ack['type'], 2)
        self.assertEqual(json.loads(ack['payload']), dict(protocol=1, session_id=SESSION, link_ready=False))
        self.assertEqual(self.decode(tx[2])['seq'], 1)
        self.assertTrue(self.health.link_ready)
        self.assertEqual(self.health.session_id, SESSION)

    def test_ready_order_and_one_queued_descriptor(self):
        self.trace([HELLO, CONFIRM, idle(2)])
        events = [self.lib.bridge_spi_trace_ready(i) for i in range(self.metric(10))]
        self.assertEqual(events, [0, 1, 0, 1, 0, 1, 0])
        self.assertEqual(self.health.queued, self.health.completed + 1)
        self.assertTrue(self.health.driver_owned)

    def test_no_clock_wait_keeps_dma_and_descriptor_immutable(self):
        self.trace([], waits=[600])
        self.assertEqual(self.health.wait_timeouts, 600)
        self.assertEqual(self.health.queued, 1)
        self.assertEqual(self.health.completed, 0)
        self.assertEqual(self.metric(4), 0)  # READY low: real queued transaction.
        self.assertTrue(self.health.driver_owned)

    def test_repeated_hello_reuses_random_session(self):
        tx = self.trace([HELLO, HELLO, CONFIRM])
        self.assertEqual(tx[1], tx[2])
        self.assertEqual(self.metric(6), 1)
        self.assertTrue(self.health.link_ready)

    def test_rng_zero_does_not_fabricate_session(self):
        tx = self.trace([HELLO], rng=0)
        self.assertEqual(self.metric(6), 4)
        self.assertEqual(self.health.protocol.rng_failures, 1)
        self.assertFalse(self.health.link_ready)
        self.assertEqual(self.decode(tx[-1])['session'], 0)

    def test_wrong_role_and_old_session_never_delivered(self):
        pc = frame(1, b'{"role":"pc","protocol":1}', session=0, seq=0, ack=0)
        self.trace([pc, RESULT])
        self.assertEqual(self.metric(0), 0)
        self.assertEqual(self.metric(6), 0)
        self.assertFalse(self.health.link_ready)
        self.assertEqual(self.health.protocol.invalid, 2)

    def test_new_hello_revokes_old_session_and_rejects_old_frames(self):
        tx = self.trace([HELLO, CONFIRM, HELLO, RESULT], mode=9)
        self.assertEqual(self.health.session_id, SESSION + 1)
        self.assertFalse(self.health.link_ready)
        self.assertEqual(self.metric(0), 0)
        self.assertEqual(self.metric(3), 3)
        self.assertEqual(self.decode(tx[3])['session'], SESSION + 1)

    def test_strict_lengths_and_requeue_recovery(self):
        tx = self.trace([HELLO, CONFIRM, RESULT, RESULT, RESULT,
            frame(0x20, telemetry(10), count=10, session=SESSION, seq=10, ack=0)],
            mode=1, bits=[8192, 8192, 0, 8191, 8193, 8192])
        self.assertEqual(self.health.truncated, 2)
        self.assertEqual(self.health.overlength, 1)
        self.assertEqual(self.metric(0), 0)
        self.assertEqual(self.health.protocol.telemetry_records, 10)
        self.assertEqual(self.health.protocol.records_dropped, 10)
        self.assertEqual(self.decode(tx[-1])['ack'], 0)

    def test_crc_bad_json_and_wrong_direction_do_not_ack(self):
        bad_crc = mutate(RESULT, 28, 0, 'I', recalc=False)
        bad_json = frame(0x11, b'{', flags=1, seq=2, session=SESSION, ack=0, request=77)
        command = frame(0x10, b'{}', flags=1, seq=2, session=SESSION, ack=0, request=77)
        tx = self.trace([HELLO, CONFIRM, bad_crc, bad_json, command], mode=1)
        self.assertEqual(self.health.protocol.crc_errors, 1)
        self.assertEqual(self.health.protocol.invalid, 2)
        self.assertEqual(self.metric(0), 0)
        self.assertEqual(self.decode(tx[-1])['ack'], 0)

    def test_capacity_reserved_before_ack_then_retry_delivered(self):
        tx = self.trace([HELLO, CONFIRM, RESULT, RESULT], mode=1, reserve_after=4)
        self.assertEqual(self.decode(tx[3])['ack'], 0)
        self.assertEqual(self.decode(tx[4])['ack'], 2)
        self.assertEqual(self.health.protocol.backpressure, 1)
        self.assertEqual(self.metric(0), 1)

    def test_duplicate_identity_not_hidden_by_new_telemetry(self):
        self.trace([HELLO, CONFIRM, RESULT,
            frame(0x20, telemetry(1), count=1, session=SESSION, seq=10, ack=0), RESULT], mode=1)
        self.assertEqual(self.metric(0), 1)
        self.assertEqual(self.metric(1), 1)
        self.assertEqual(self.health.protocol.duplicates, 1)
        out, request = C.create_string_buffer(992), C.c_uint32()
        length = self.lib.bridge_spi_trace_commit(0, out, 992, C.byref(request))
        self.assertEqual(bytes(out[:length]), RESULT_PAYLOAD)
        self.assertEqual(request.value, 77)

    def test_offline_is_real_backpressure_and_attributed_telemetry_drop(self):
        self.trace([HELLO, CONFIRM, RESULT, frame(0x20, telemetry(1), count=1, session=SESSION, seq=3, ack=0)])
        self.assertEqual(self.health.protocol.backpressure, 1)
        self.assertEqual(self.health.protocol.telemetry_dropped, 1)
        self.assertEqual(self.health.protocol.records_dropped, 1)
        self.assertEqual(self.metric(7), 0)

    def test_bound_telemetry_consumer_takes_copy_without_drop(self):
        self.trace([HELLO, CONFIRM, frame(0x20, telemetry(1), count=1, session=SESSION, seq=3, ack=0)], mode=3)
        self.assertEqual(self.health.protocol.telemetry_packets, 1)
        self.assertEqual(self.health.protocol.telemetry_dropped, 0)

    def test_outbound_command_preserved_and_idle_advances_while_waiting_ack(self):
        tx = self.trace([HELLO, CONFIRM, idle(2)], mode=5)
        command = self.decode(tx[2])
        self.assertEqual(command['type'], 0x10)
        self.assertEqual(command['request'], 77)
        self.assertEqual(command['payload'], b'{"op":"GET_STATUS","args":{}}')
        self.assertEqual(self.decode(tx[3])['type'], 3)
        self.assertEqual(self.metric(2), 1)
        self.assertTrue(self.metric(8))

    def test_three_exact_retry_images_and_rebuild_without_clock_mutation(self):
        frames = [HELLO, CONFIRM] + [idle(i) for i in range(2, 10)]
        waits = [0] * 11
        for index in (3, 5, 7, 9):
            waits[index] = 20
        tx = self.trace(frames, mode=5, waits=waits)
        for index in (4, 6, 8):
            self.assertEqual(tx[index], tx[2])
        self.assertEqual(self.health.protocol.retries, 3)
        self.assertEqual(self.health.protocol.rebuilds, 1)
        self.assertFalse(self.health.link_ready)
        self.assertEqual(self.decode(tx[-1])['session'], 0)

    def test_exact_ack_cancels_pending_resend_after_driver_returns(self):
        tx = self.trace([HELLO, CONFIRM, idle(2), idle(3, ack=1)], mode=5, waits=[0, 0, 0, 20, 0])
        self.assertEqual(self.decode(tx[-1])['type'], 3)
        self.assertEqual(self.health.protocol.retries, 0)
        self.assertFalse(self.metric(8))

    def test_short_reliable_tx_is_retried_without_advancing_send_timer(self):
        tx = self.trace([HELLO, CONFIRM, idle(2), idle(3)], mode=5, bits=[8192, 8192, 8191, 8192])
        self.assertEqual(tx[2], tx[3])
        self.assertEqual(self.health.truncated, 1)
        self.assertEqual(self.health.protocol.retries, 0)
        self.assertEqual(self.metric(2), 1)

    def test_queue_failure_retracts_only_proven_unaccepted_ownership(self):
        self.trace(failure=1)
        self.assertEqual(self.health.service_error, 0x602)
        self.assertFalse(self.health.driver_owned)
        self.assertEqual(self.metric(5), 1)  # APPLICATION
        self.assertEqual(self.metric(4), 1)
        self.assertEqual(self.health.queued, 0)

    def test_get_error_or_alien_descriptor_never_reclaims_dma(self):
        for failure, error in ((2, 0x604), (3, 0x103)):
            self.trace(failure=failure)
            self.assertEqual(self.health.service_error, error)
            self.assertTrue(self.health.driver_owned)
            self.assertEqual(self.metric(5), 2)  # DRIVER
            self.assertEqual(self.metric(4), 1)

    def test_port_binding_requires_complete_reservation_contract(self):
        self.assertEqual(self.lib.bridge_spi_port_check_test(), 0)

    def test_service_failure_revokes_ready_without_touching_owned_dma(self):
        self.trace([HELLO, CONFIRM], mode=5, failure=5)
        self.assertEqual(self.health.service_error, 0x604)
        self.assertTrue(self.health.driver_owned)
        self.assertFalse(self.health.link_ready)
        self.assertEqual(self.health.session_id, 0)
        self.assertEqual(self.metric(3), 3)

    def test_event_and_log_block_contents_survive_actual_takeover_interface(self):
        block = struct.pack('<IHH', 256, 16, 0) + bytes(range(16))
        event = b'{"event":"RUN_STARTED","run_id":9,"tick":100,"control_sequence":50,"revision":3}'
        tx = self.trace([HELLO, CONFIRM,
            frame(0x30, block, flags=1, session=SESSION, seq=2, ack=0, request=88),
            frame(0x12, event, flags=1, session=SESSION, seq=3, ack=0, request=0)], mode=1)
        self.assertEqual(self.metric(0), 2)
        for index, expected, expected_request in ((0, block, 88), (1, event, 0)):
            out, request = C.create_string_buffer(992), C.c_uint32()
            length = self.lib.bridge_spi_trace_commit(index, out, 992, C.byref(request))
            self.assertEqual(bytes(out[:length]), expected)
            self.assertEqual(request.value, expected_request)
        self.assertEqual(self.decode(tx[-1])['ack'], 3)

    def test_async_and_log_request_identity_checked_before_reservation(self):
        block = struct.pack('<IHH', 256, 1, 0) + b'X'
        tx = self.trace([HELLO, CONFIRM,
            frame(0x30, block, flags=1, session=SESSION, seq=2, ack=0, request=0),
            frame(0x12, b'{}', flags=1, session=SESSION, seq=3, ack=0, request=88)], mode=1)
        self.assertEqual(self.metric(0), 0)
        self.assertEqual(self.health.protocol.invalid, 2)
        self.assertEqual(self.decode(tx[-1])['ack'], 0)

    def test_bad_rx_crc_does_not_erase_actual_full_tx_completion(self):
        bad = mutate(RESULT, 28, 0, 'I', recalc=False)
        tx = self.trace([HELLO, CONFIRM, bad, idle(3)], mode=5, waits=[0, 0, 0, 20, 0])
        self.assertEqual(self.health.protocol.crc_errors, 1)
        self.assertEqual(tx[4], tx[2])  # Reliable retry due after real TX completion.
