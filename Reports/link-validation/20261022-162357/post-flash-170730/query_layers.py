"""Bounded parked read-only probes; each failure is recorded independently."""
from pathlib import Path
import json
import sys
import time

out = Path(__file__).resolve().parent / "individual-queries"
out.mkdir(exist_ok=True)
helper = Path(__file__).resolve().parents[2] / "20261021-093438/parked_recovery_probe.py"
source = helper.read_text(encoding="utf-8").split("\npeer = None\n", 1)[0]
sys.argv = [str(helper), str(out)]
exec(compile(source, str(helper), "exec"))
results = []
for operation, arguments in [("GET_STATUS", {}), ("GET_PARAMS", {"mode": 1, "gear": 0}),
                             ("GET_LOG_META", {}), ("SUBSCRIBE", {"udp_port": 5001})]:
    peer = None
    started = time.monotonic()
    try:
        peer = Peer()
        if operation == "GET_STATUS":
            # Verify BUSY before a failing business query can invalidate the session.
            second = Peer.__new__(Peer)
            second.sock = socket.create_connection((HOST, 5000), timeout=3)
            second.buffer = b""
            try:
                h, body = second.read(2)
                assert h[2] == 2 and body["status"] == "BUSY"
                second.sock.settimeout(1)
                assert second.sock.recv(1) == b""
                note("second_connection_busy", result=body, eof=True)
            finally:
                second.sock.close()
        result = peer.command(operation, arguments)
        entry = dict(op=operation, result=result, session=peer.sid)
        if operation == "SUBSCRIBE" and result["status"] == "OK":
            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as udp:
                udp.bind(("0.0.0.0", 5001))
                entry["observation"] = peer.observe(udp, 10)
                # Explicit close and new HELLO require a fresh subscription.
                peer.close()
                peer = Peer()
                entry["reconnected_session"] = peer.sid
                entry["resubscribe"] = peer.command(operation, arguments)
                entry["reobservation"] = peer.observe(udp, 10)
    except Exception as error:
        entry = dict(op=operation, error=repr(error))
        note("individual_query_failed", **entry)
    finally:
        if peer is not None:
            peer.close()
    entry["elapsed_seconds"] = time.monotonic() - started
    results.append(entry)
    (out / "summary.json").write_text(json.dumps(results, indent=2), encoding="utf-8")
    time.sleep(0.4)
TRACE.close()
print(json.dumps(results, indent=2))
