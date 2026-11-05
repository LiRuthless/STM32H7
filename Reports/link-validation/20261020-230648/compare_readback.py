import pathlib,sys,json,hashlib
out=pathlib.Path(sys.argv[1]);p=out/'stm32-flash-readback.bin';m={};base=0
for line in pathlib.Path('SmartCar/MDK-ARM/SmartCar/SmartCar.hex').read_text().splitlines():
 b=bytes.fromhex(line[1:]);n=b[0];addr=int.from_bytes(b[1:3],'big');t=b[3]
 if t==4:base=int.from_bytes(b[4:6],'big')<<16
 elif t==0:m.update({base+addr+i:v for i,v in enumerate(b[4:4+n])})
if p.exists():
 data=p.read_bytes();bad=[a for a,v in m.items() if a-0x08000000>=len(data) or data[a-0x08000000]!=v]
 r={'defined_hex_bytes':len(m),'readback_bytes':len(data),'mismatches':len(bad),'first_mismatches':[hex(a) for a in bad[:16]],'readback_sha256':hashlib.sha256(data).hexdigest()}
 (out/'stm32-firmware-compare.json').write_text(json.dumps(r,indent=2));print(json.dumps(r))
