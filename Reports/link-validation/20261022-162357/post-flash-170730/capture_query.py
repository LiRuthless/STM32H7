from pathlib import Path
import sys
import time

out = Path(__file__).resolve().parent / "command-capture"
helper = Path(__file__).resolve().parents[2] / "20261021-093438/parked_recovery_probe.py"
source = helper.read_text(encoding="utf-8").split("\npeer = None\n", 1)[0]
sys.argv = [str(helper), str(out)]
exec(compile(source, str(helper), "exec"))
peer = None
try:
    time.sleep(0.5)
    peer = Peer()
    peer.command("GET_STATUS")
except Exception as error:
    note("capture_query_error", error=repr(error))
finally:
    if peer is not None:
        peer.close()
    TRACE.close()
