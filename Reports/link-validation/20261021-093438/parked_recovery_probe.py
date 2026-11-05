"""本次实机取证脚本；仅查询/订阅，不是 Phase06 产品实现。"""
import datetime
import json
import pathlib
import select
import socket
import struct
import sys
import time
import zlib

OUT = pathlib.Path(sys.argv[1])
TRACE = (OUT / "recovery-wire.jsonl").open("w", encoding="utf-8")
HOST = "192.168.4.1"
ALLOWED = {"GET_STATUS", "GET_PARAMS", "GET_LOG_META", "SUBSCRIBE"}
NEXT_REQUEST = int(time.time()) & 0x7fffffff


def note(event, **fields):
    row = dict(event=event,
               time=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat(),
               monotonic_ns=time.monotonic_ns(), **fields)
    TRACE.write(json.dumps(row, ensure_ascii=False) + "\n")
    TRACE.flush()
    if event not in ("tx", "rx", "udp"):
        print(json.dumps(row, ensure_ascii=True), flush=True)


def encode(kind, body=None, sid=0, seq=0, ack=0, rid=0):
    payload = json.dumps(body, separators=(",", ":")).encode() if body is not None else b""
    head = struct.pack("<4sBBHHHIIII", b"SCB1", 1, kind,
                       1 if kind == 0x10 else 0, len(payload), 0, sid, seq, ack, rid)
    return head + struct.pack("<I", zlib.crc32(head + payload) & 0xffffffff) + payload


def decode(raw):
    h = struct.unpack("<4sBBHHHIIIII", raw[:32])
    assert h[0] == b"SCB1" and h[1] == 1 and h[3] & ~1 == 0
    assert len(raw) == 32 + h[4] and h[4] <= 992
    assert zlib.crc32(raw[:28] + raw[32:]) & 0xffffffff == h[-1]
    return h, json.loads(raw[32:]) if h[4] and h[2] != 0x20 else None


class Peer:
    def __init__(self):
        self.sock = socket.create_connection((HOST, 5000), timeout=3)
        self.buffer = b""
        self.sid = self.seq = self.ack = 0
        self.send(encode(1, {"role": "pc", "protocol": 1}))
        h, body = self.read(3)
        assert h[2] == 2 and body.get("status") != "BUSY", "existing control connection BUSY"
        assert body["protocol"] == 1 and body["session_id"] == h[6]
        self.sid = h[6]
        note("hello_ack", body=body)
        if not self.sid or not body["link_ready"]:
            self.sock.close()
            raise RuntimeError("SPI link not ready; no business command sent")

    def send(self, raw):
        self.sock.sendall(raw)
        note("tx", raw_hex=raw.hex())

    def read(self, timeout):
        deadline = time.monotonic() + timeout
        while True:
            if len(self.buffer) >= 32:
                n = 32 + struct.unpack_from("<H", self.buffer, 8)[0]
                assert n <= 1024
                if len(self.buffer) >= n:
                    raw, self.buffer = self.buffer[:n], self.buffer[n:]
                    h, body = decode(raw)
                    note("rx", raw_hex=raw.hex(), crc_valid=True, body=body)
                    return h, body
            left = deadline - time.monotonic()
            if left <= 0:
                raise socket.timeout()
            self.sock.settimeout(left)
            part = self.sock.recv(4096)
            if not part:
                raise EOFError("device closed TCP")
            self.buffer += part

    def accept(self, h, body):
        assert h[6] == self.sid, "session changed"
        if h[3] & 1:
            self.ack = h[7]
            self.seq += 1
            self.send(encode(3, sid=self.sid, seq=self.seq, ack=self.ack))
        if h[2] == 0x12:
            note("device_event", body=body)

    def command(self, op, args=None):
        global NEXT_REQUEST
        assert op in ALLOWED
        rid = NEXT_REQUEST
        NEXT_REQUEST += 1
        self.seq += 1
        sent_seq = self.seq
        raw = encode(0x10, {"op": op, "args": args or {}}, self.sid,
                     sent_seq, self.ack, rid)
        started = last_send = time.monotonic()
        self.send(raw)
        acked = False
        retries = 0
        while time.monotonic() - started < 5:
            try:
                h, body = self.read(0.05)
            except socket.timeout:
                if not acked and time.monotonic() - last_send >= 0.2:
                    if retries == 3:
                        raise TimeoutError(f"{op}: transport not acknowledged")
                    self.send(raw)
                    last_send = time.monotonic()
                    retries += 1
                continue
            acked |= h[8] == sent_seq
            self.accept(h, body)
            if h[2] == 0x11 and h[9] == rid:
                assert body["op"] == op
                if body["status"] == "RECEIVED":
                    continue
                note("business_result", op=op, request_id=rid,
                     elapsed_ms=(time.monotonic() - started) * 1000,
                     retries=retries, result=body)
                return body
        raise TimeoutError(f"{op}: no final business result")

    def busy_check(self):
        second = None
        try:
            second = Peer.__new__(Peer)
            second.sock = socket.create_connection((HOST, 5000), timeout=3)
            second.buffer = b""
            h, body = second.read(2)
            assert h[2] == 2 and body["status"] == "BUSY"
            second.sock.settimeout(1)
            assert second.sock.recv(1) == b""
            note("second_connection_busy", result=body, eof=True)
        finally:
            if second is not None:
                second.sock.close()
        return self.command("GET_STATUS")

    def observe(self, udp, seconds):
        started = time.monotonic()
        stats = dict(datagrams=0, valid_packets=0, records=0, invalid_packets=0)
        while time.monotonic() - started < seconds:
            if self.buffer or select.select([self.sock], [], [], 0.1)[0]:
                try:
                    h, body = self.read(0.1)
                    self.accept(h, body)
                except socket.timeout:
                    pass
            while select.select([udp], [], [], 0)[0]:
                raw, source = udp.recvfrom(2048)
                stats["datagrams"] += 1
                try:
                    h, _ = decode(raw)
                    assert source[0] == HOST and h[2] == 0x20 and h[6] == self.sid
                    assert 1 <= h[5] <= 10 and h[4] == 24 + 96 * h[5]
                    stats["valid_packets"] += 1
                    stats["records"] += h[5]
                    note("udp", raw_hex=raw.hex(), source=source)
                except Exception as exc:
                    stats["invalid_packets"] += 1
                    note("udp_error", error=repr(exc))
        note("parked_subscription_observation", seconds=time.monotonic() - started,
             session_id=self.sid, **stats)
        return stats

    def close(self):
        self.sock.close()
        note("client_active_close", session_id=self.sid)


peer = None
summary = {}
try:
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as udp:
        udp.bind(("0.0.0.0", 5001))
        peer = Peer()
        before = peer.command("GET_STATUS")
        assert before["status"] == "OK" and not before["running"], "parking required"
        target = {"mode": before["mode"], "gear": before["gear"]}
        params_before = peer.command("GET_PARAMS", target)
        meta = peer.command("GET_LOG_META")
        status_after_busy = peer.busy_check()
        assert not status_after_busy["running"]
        assert peer.command("SUBSCRIBE", {"udp_port": 5001})["status"] == "OK"
        first_observation = peer.observe(udp, 10)
        assert peer.command("GET_STATUS")["status"] == "OK"
        session_before = peer.sid
        peer.close()
        peer = None
        time.sleep(0.3)
        peer = Peer()
        after = peer.command("GET_STATUS")
        assert not after["running"]
        params_after = peer.command("GET_PARAMS", target)
        assert peer.command("SUBSCRIBE", {"udp_port": 5001})["status"] == "OK"
        second_observation = peer.observe(udp, 10)
        final = peer.command("GET_STATUS")
        assert not final["running"]
        summary = dict(session_before=session_before, session_after=peer.sid,
                       before=before, after=after, final=final, log_meta=meta,
                       parameters_unchanged=params_before.get("values") == params_after.get("values"),
                       effective_unchanged=params_before.get("effective") == params_after.get("effective"),
                       observations=[first_observation, second_observation],
                       result="parked_queries_busy_subscribe_hold_reconnect_completed",
                       note="Parking observation is not a 60-second running telemetry throughput test.")
        note("completed", **summary)
except Exception as exc:
    summary["error"] = repr(exc)
    note("failed", error=repr(exc))
finally:
    if peer is not None:
        peer.close()
    (OUT / "recovery-summary.json").write_text(json.dumps(summary, ensure_ascii=False, indent=2), encoding="utf-8")
    TRACE.close()
