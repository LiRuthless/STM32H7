from pathlib import Path
import hashlib
import json
import re

out = Path(__file__).resolve().parent
root = out.parents[3]
hexpath = root / "SmartCar/MDK-ARM/SmartCar/SmartCar.hex"
flash = (out / "stm32-flash-readback.bin").read_bytes()
base = 0
defined = {}
for line in hexpath.read_text().splitlines():
    data = bytes.fromhex(line[1:])
    assert sum(data) % 256 == 0
    size, offset, kind = data[0], int.from_bytes(data[1:3], "big"), data[3]
    payload = data[4:4+size]
    if kind == 4:
        base = int.from_bytes(payload, "big") << 16
    elif kind == 0:
        for index, value in enumerate(payload):
            defined[base+offset+index] = value
mismatch = [address for address, value in defined.items()
            if flash[address-0x08000000] != value]
comparison = dict(hex_sha256=hashlib.sha256(hexpath.read_bytes()).hexdigest(),
                  readback_sha256=hashlib.sha256(flash).hexdigest(),
                  compared_defined_bytes=len(defined), mismatch_count=len(mismatch),
                  first_address=hex(min(defined)), last_address=hex(max(defined)))
(out / "stm32-firmware-compare.json").write_text(json.dumps(comparison, indent=2))
assert not mismatch
print(comparison)
text = (out / "stm32-diagnosis.log").read_text(errors="replace")
pattern = re.compile(r"SAMPLE (\d+) WIFI (.*?) SPI (.*?) DMA (.*?) HSTATE (.*?) DMAERR (.*?) LINK (.*)")
samples = []
for match in pattern.finditer(text):
    row = {"index": int(match[1])}
    row.update({key: [int(value, 0) for value in match[index].split()]
                for index, key in enumerate(["wifi", "spi", "dma", "hstate", "dmaerr", "link"], 2)})
    samples.append(row)
first, last = samples[0], samples[-1]
summary = dict(samples=len(samples), first=first, last=last,
               completed_delta=last["wifi"][2]-first["wifi"][2],
               error_delta=last["wifi"][3]-first["wifi"][3],
               timeout_delta=last["wifi"][4]-first["wifi"][4],
               tx_remaining_values=sorted({s["dma"][1] for s in samples}),
               rx_remaining_values=sorted({s["dma"][7] for s in samples}),
               states=sorted({s["wifi"][5] for s in samples}),
               sessions=sorted({s["link"][0] for s in samples}),
               limitation="Non-atomic running snapshots, not waveform or business-response proof")
(out / "stm32-diagnosis-summary.json").write_text(json.dumps(summary, indent=2))
print({k: v for k, v in summary.items() if k not in
       ["first", "last", "tx_remaining_values", "rx_remaining_values"]})
