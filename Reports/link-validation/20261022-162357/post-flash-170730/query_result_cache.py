"""Read-only business queries followed by passive STM32 result-cache capture."""
from pathlib import Path
import json
import struct
import subprocess
import sys
import zlib

out = Path(__file__).resolve().parent / "business-caches"
out.mkdir(exist_ok=True)
root = out.parents[4]
helper = root / "Reports/link-validation/20261021-093438/parked_recovery_probe.py"
source = helper.read_text(encoding="utf-8").split("\npeer = None\n", 1)[0]
sys.argv = [str(helper), str(out)]
exec(compile(source, str(helper), "exec"))
header = (out.parent / "command-capture/read-cache.cfg").read_text().split("echo [stm32h7x.cpu0 curstate]")[0]
executable = root / "Tools/STM32Flash/openocd/bin/openocd.exe"
scripts = root / "Tools/STM32Flash/openocd/share/openocd/scripts"
summaries = []
for operation, args in [("GET_PARAMS", {"mode": 1, "gear": 0}), ("GET_LOG_META", {})]:
    peer = None
    rid = NEXT_REQUEST
    entry = dict(operation=operation, request_id=rid)
    try:
        peer = Peer()
        entry["network_result"] = peer.command(operation, args)
    except Exception as error:
        entry["network_error"] = repr(error)
        note("cache_query_error", **entry)
    finally:
        if peer is not None:
            peer.close()
    binary = out / (operation + ".bin")
    config = out / (operation + ".cfg")
    config.write_text(header + 'echo [stm32h7x.cpu0 curstate]\n'
                      + 'dump_image {' + binary.as_posix() + '} 0x24018940 1024\n'
                      + 'echo [stm32h7x.cpu0 curstate]\nshutdown\n')
    result = subprocess.run([str(executable), "-s", str(scripts), "-f", str(config)],
                            capture_output=True)
    (out / (operation + "-openocd.log")).write_bytes(result.stdout + result.stderr)
    assert result.returncode == 0
    raw = binary.read_bytes()
    h = struct.unpack("<4sBBHHHIIIII", raw[:32])
    crc_valid = zlib.crc32(raw[:28] + raw[32:32+h[4]]) & 0xffffffff == h[10]
    matches = h[0] == b"SCB1" and h[2] == 17 and h[9] == rid and crc_valid
    entry.update(cache_request=h[9], cache_session=h[6], cache_sequence=h[7],
                 cache_ack=h[8], crc_valid=crc_valid, matches_request=matches)
    if matches:
        entry["stm32_cached_result"] = json.loads(raw[32:32+h[4]])
    summaries.append(entry)
    (out / "summary.json").write_text(json.dumps(summaries, indent=2), encoding="utf-8")
TRACE.close()
print(json.dumps(summaries, indent=2))
