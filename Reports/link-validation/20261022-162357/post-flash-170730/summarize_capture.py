from collections import Counter
from pathlib import Path
import json
import re
import struct
import zlib

out = Path(__file__).resolve().parent / "command-capture"
text = (out / "capture.log").read_text(errors="replace")
pattern = re.compile(r"CAP (\d+) TICK (.*?) BEFORE (.*?) RX (.*?) AFTER (.*?) LINK (.*)")
rows = []
for match in pattern.finditer(text):
    before = [int(value, 0) for value in match[3].split()]
    after = [int(value, 0) for value in match[5].split()]
    link = [int(value, 0) for value in match[6].split()]
    raw = b"".join(struct.pack("<I", int(value, 0)) for value in match[4].split())
    row = dict(index=int(match[1]), tick=int(match[2], 0), before=before, after=after,
               link=link, raw_hex=raw.hex(), magic=raw[:4].hex(), crc_valid=False,
               stable_finished_buffer=before[5] == after[5] == 2 and before[2] == after[2])
    if raw[:4] == b"SCB1":
        header = struct.unpack("<4sBBHHHIIIII", raw[:32])
        length = header[4]
        row.update(type=header[2], payload_length=length, session=header[6],
                   sequence=header[7], ack=header[8], request_id=header[9])
        if length <= len(raw)-32:
            payload = raw[32:32+length]
            row["crc_valid"] = zlib.crc32(raw[:28]+payload) & 0xffffffff == header[10]
            row["payload_hex"] = payload.hex()
            if row["crc_valid"] and payload:
                try:
                    row["body"] = json.loads(payload)
                except Exception as error:
                    row["json_error"] = repr(error)
    rows.append(row)
summary = dict(samples=len(rows), magic_counts=dict(Counter(r["magic"] for r in rows)),
               crc_valid_types=dict(Counter(str(r.get("type")) for r in rows if r["crc_valid"])),
               commands=[r for r in rows if r.get("type") == 16],
               link_ack_values=sorted({r["link"][2] for r in rows}),
               limitation="RAM read snapshots; valid CRC supports packet integrity, not physical waveform")
(out / "decoded-snapshots.json").write_text(json.dumps(rows, indent=2), encoding="utf-8")
(out / "capture-summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
print(json.dumps(summary, indent=2))
