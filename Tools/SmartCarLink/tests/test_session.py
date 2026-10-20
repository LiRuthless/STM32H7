"""scb_session 单元测试：可靠层、重发、去重、会话变化、请求缓存（注入假时钟）。"""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import scb_protocol as protocol
import scb_session as sess


class FakeClock:
    def __init__(self):
        self.now = 1000.0

    def __call__(self):
        return self.now

    def advance(self, dt):
        self.now += dt


def make_session():
    clock = FakeClock()
    s = sess.Session(clock=clock)
    s.adopt_session(0x1234, link_ready=True)
    return s, clock


def reliable_command(s, payload=b'{"op":"GET_STATUS","args":{}}', rid=1):
    return s.send_frame(protocol.MSG_COMMAND, payload, request_id=rid)


def ack_frame(seq, ack_seq, session_id=0x1234, reliable=True, msg_type=protocol.MSG_RESULT):
    """构造一个来自对端的帧头（feed_frame 只关心头字段）。"""
    flags = protocol.FLAG_RELIABLE if reliable else 0
    return protocol.FrameHeader(
        msg_type=msg_type, flags=flags, payload_len=2,
        record_count=0, session_id=session_id, seq=seq,
        ack_seq=ack_seq, request_id=1, crc32=0)


class TestReliableWindow(unittest.TestCase):
    def test_single_in_flight(self):
        s, _ = make_session()
        events = reliable_command(s, rid=1)
        self.assertEqual(len(events), 1)
        self.assertIsInstance(events[0], sess.SendBytes)
        self.assertEqual(s.in_flight_seq, 1)
        # 第二个可靠帧：在途未释放 → 排队，不立即发送
        events2 = reliable_command(s, rid=2)
        self.assertEqual(events2, [])
        self.assertEqual(s.queued_count, 1)

    def test_ack_exact_match_releases_and_sends_queued(self):
        s, _ = make_session()
        reliable_command(s, rid=1)
        reliable_command(s, rid=2)
        # ack_seq 不匹配（0 或其他）→ 不释放
        events = s.feed_frame(ack_frame(seq=1, ack_seq=9), b"{}")
        self.assertTrue(any(isinstance(e, sess.DeliverFrame) for e in events))
        self.assertEqual(s.in_flight_seq, 1)
        self.assertEqual(s.queued_count, 1)
        # ack_seq 精确匹配 → 释放并立即发出队列中的下一帧
        events = s.feed_frame(ack_frame(seq=2, ack_seq=1), b"{}")
        self.assertTrue(any(isinstance(e, sess.ReliableReleased) and e.seq == 1
                            for e in events))
        send_events = [e for e in events if isinstance(e, sess.SendBytes)]
        self.assertEqual(len(send_events), 1)
        self.assertEqual(s.in_flight_seq, 2)
        self.assertEqual(s.queued_count, 0)

    def test_seq_monotonic_and_retransmit_keeps_seq(self):
        s, clock = make_session()
        events = reliable_command(s, rid=1)
        frame1 = events[0].data
        seq1 = int.from_bytes(frame1[16:20], "little")
        clock.advance(sess.RELIABLE_TIMEOUT_S)
        events = s.poll()
        self.assertEqual(len(events), 1)
        frame2 = events[0].data
        self.assertEqual(frame1, frame2)  # 重发保留原 seq 和原内容
        self.assertEqual(int.from_bytes(frame2[16:20], "little"), seq1)

    def test_retransmit_three_times_then_dead(self):
        s, clock = make_session()
        reliable_command(s, rid=1)
        sends = 0
        for i in range(sess.MAX_RETRANSMIT):
            clock.advance(sess.RELIABLE_TIMEOUT_S)
            events = s.poll()
            self.assertTrue(any(isinstance(e, sess.SendBytes) for e in events))
            sends += 1
            self.assertTrue(s.alive)
        clock.advance(sess.RELIABLE_TIMEOUT_S)
        events = s.poll()
        self.assertTrue(any(isinstance(e, sess.LinkDead) for e in events))
        self.assertFalse(s.alive)
        self.assertIsNone(s.in_flight_seq)
        # 链路死亡后不能发送
        with self.assertRaises(sess.SessionError):
            reliable_command(s, rid=9)

    def test_no_retransmit_before_timeout(self):
        s, clock = make_session()
        reliable_command(s, rid=1)
        clock.advance(sess.RELIABLE_TIMEOUT_S - 0.01)
        self.assertEqual(s.poll(), [])


class TestInboundDedup(unittest.TestCase):
    def test_duplicate_reliable_only_acked(self):
        s, _ = make_session()
        header = ack_frame(seq=5, ack_seq=0)
        events = s.feed_frame(header, b"{}")
        self.assertTrue(any(isinstance(e, sess.DeliverFrame) for e in events))
        self.assertEqual(s.ack_seq, 5)
        # 同 seq 重复可靠帧：只确认，不重复分发
        events = s.feed_frame(header, b"{}")
        self.assertTrue(any(isinstance(e, sess.DuplicateReliable) for e in events))
        self.assertFalse(any(isinstance(e, sess.DeliverFrame) for e in events))

    def test_later_unreliable_seq_does_not_invalidate_reliable_retry(self):
        s, _ = make_session()
        # 接受可靠帧 seq=5
        s.feed_frame(ack_frame(seq=5, ack_seq=0), b"{}")
        # 之后收到更大的不可靠帧（IDLE/TELEMETRY）seq=6,7
        s.feed_frame(ack_frame(seq=6, ack_seq=0, reliable=False,
                               msg_type=protocol.MSG_IDLE), b"")
        s.feed_frame(ack_frame(seq=7, ack_seq=0, reliable=False,
                               msg_type=protocol.MSG_IDLE), b"")
        # 可靠帧 seq=5 的重发仍判重复，不被"后来的大 seq"冲掉
        events = s.feed_frame(ack_frame(seq=5, ack_seq=0), b"{}")
        self.assertTrue(any(isinstance(e, sess.DuplicateReliable) for e in events))

    def test_old_session_frame_dropped(self):
        s, _ = make_session()
        events = s.feed_frame(ack_frame(seq=1, ack_seq=0, session_id=0x9999), b"{}")
        self.assertEqual(events, [])


class TestSessionChange(unittest.TestCase):
    def test_session_change_clears_state(self):
        s, _ = make_session()
        reliable_command(s, rid=1)
        s.requests.add(sess.RequestEntry(request_id=1, op="SET_PARAMS",
                                         content=b"x"))
        s.subscribed_udp_port = 5001
        changed = s.adopt_session(0x2000, link_ready=True)
        self.assertTrue(changed)
        self.assertIsNone(s.in_flight_seq)
        self.assertEqual(s.queued_count, 0)
        self.assertIsNone(s.subscribed_udp_port)
        # 未决请求标 UNKNOWN，不静默丢弃
        entry = s.requests.get(1)
        self.assertEqual(entry.state, sess.REQ_UNKNOWN)
        self.assertEqual(entry.status, sess.STATUS_UNKNOWN)
        # request_id 重新从 1 开始（新会话）
        self.assertEqual(s.allocate_request_id(), 1)

    def test_same_session_keeps_state_flag(self):
        s, _ = make_session()
        changed = s.adopt_session(0x1234, link_ready=True)
        self.assertFalse(changed)

    def test_request_id_nonzero_increment(self):
        s, _ = make_session()
        ids = [s.allocate_request_id() for _ in range(5)]
        self.assertEqual(ids, [1, 2, 3, 4, 5])


class TestRequestCache(unittest.TestCase):
    def test_capacity_32_eviction(self):
        cache = sess.RequestCache(capacity=32)
        for i in range(1, 40):
            cache.add(sess.RequestEntry(request_id=i, op="GET_STATUS",
                                        content=b"c%d" % i))
        self.assertEqual(len(cache), 32)
        self.assertIsNone(cache.get(1))       # 最旧的被淘汰
        self.assertIsNotNone(cache.get(39))

    def test_same_id_same_content(self):
        cache = sess.RequestCache()
        cache.add(sess.RequestEntry(request_id=7, op="TUNE_ENABLE", content=b"abc"))
        self.assertEqual(cache.check(7, b"abc"), "same")

    def test_same_id_different_content_conflict(self):
        cache = sess.RequestCache()
        cache.add(sess.RequestEntry(request_id=7, op="TUNE_ENABLE", content=b"abc"))
        self.assertEqual(cache.check(7, b"abd"), "conflict")

    def test_received_is_not_final(self):
        self.assertFalse(sess.is_final_status("RECEIVED"))
        self.assertTrue(sess.is_final_status("APPLIED"))
        cache = sess.RequestCache()
        cache.add(sess.RequestEntry(request_id=1, op="SET_PARAMS", content=b"x"))
        entry = cache.update_result(1, "RECEIVED", {"status": "RECEIVED"})
        self.assertEqual(entry.state, sess.REQ_RECEIVED)
        entry = cache.update_result(1, "APPLIED", {"status": "APPLIED",
                                                   "values": {"kp": 2.0}})
        self.assertEqual(entry.state, sess.REQ_FINAL)
        self.assertEqual(entry.applied_values, {"kp": 2.0})

    def test_unresolved(self):
        cache = sess.RequestCache()
        cache.add(sess.RequestEntry(request_id=1, op="A", content=b"a"))
        cache.add(sess.RequestEntry(request_id=2, op="B", content=b"b"))
        cache.update_result(2, "OK", {"status": "OK"})
        unresolved = cache.unresolved()
        self.assertEqual([e.request_id for e in unresolved], [1])


if __name__ == "__main__":
    unittest.main()
