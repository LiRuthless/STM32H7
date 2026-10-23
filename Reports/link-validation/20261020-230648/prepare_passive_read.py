import pathlib,json,sys
out=pathlib.Path(sys.argv[1]);src=pathlib.Path('SmartCar/MDK-ARM/SmartCar/SmartCar.hex');mem={};base=0
for line in src.read_text().splitlines():
    if not line.startswith(':'):continue
    b=bytes.fromhex(line[1:]);assert sum(b)%256==0
    n=b[0];a=int.from_bytes(b[1:3],'big');t=b[3];p=b[4:4+n]
    if t==4:base=int.from_bytes(p,'big')<<16
    elif t==0:
        for i,v in enumerate(p):mem[base+a+i]=v
start=min(mem);end=max(mem)+1
assert start==0x08000000 and end<=0x08200000
(out/'hex-metadata.json').write_text(json.dumps({'source':str(src),'start':start,'end':end,'length':end-start,'defined_bytes':len(mem)},indent=2))
config='''# Passive attach: no halt/reset/write to application/peripheral memory.
source [find interface/stlink.cfg]
transport select hla_swd
source [find target/stm32h7x.cfg]
# Override stock examine hook, which would modify DBGMCU and watchdog freeze.
stm32h7x.cpu0 configure -event examine-end {}
reset_config none
adapter speed 1000
gdb_port disabled
tcl_port disabled
telnet_port disabled
init
targets
'''
config+=f'dump_image {{{out.as_posix()}/stm32-flash-readback.bin}} 0x{start:08x} {end-start}\n'
config+='echo SNAPSHOT_A\nmdw 0x24034924 6\nmdw 0x40003800 6\nmdw 0x40020000 24\nmdw 0x58020000 10\nmdw 0x58020400 10\nmdw 0x58020c00 10\nmdw 0x58024400 32\nmdw 0x240003f4 94\n'
config+='echo SNAPSHOT_B\nmdw 0x24034924 6\nmdw 0x40003800 6\nmdw 0x40020000 24\nmdw 0x2407f000 8\nmdw 0x2407f400 8\ntargets\nshutdown\n'
(out/'stm32-passive-read.cfg').write_text(config)
print(json.dumps({'start':hex(start),'end':hex(end),'bytes':end-start,'config':str(out/'stm32-passive-read.cfg')}))
