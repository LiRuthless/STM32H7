from pathlib import Path
import subprocess,json,hashlib,re,time
R=Path(__file__).resolve().parents[2]
config=R/'App/app_config.h'
paths=[config]+[R/'MDK-GCC'/('SmartCar.'+ext) for ext in ('elf','hex','bin')]
original={p:(p.read_bytes() if p.exists() else None) for p in paths}
backup=R/'MDK-GCC/build/phase17-g5-original-bytes';backup.mkdir(exist_ok=True)
for path,value in original.items():
    if value is not None:(backup/path.name).write_bytes(value)
def digest(value):return hashlib.sha256(value).hexdigest() if value is not None else None
record={'before':{str(p.relative_to(R)):digest(v) for p,v in original.items()},'builds':{}}
try:
    for lcd in (0,1):
        changed,count=re.subn(rb'(#define LCD_TARGET_EXTERNAL )0\b',lambda m:m[1]+str(lcd).encode(),original[config]);assert count==1
        config.write_bytes(changed)
        log=R/f'MDK-GCC/build/phase17-g5-lcd{lcd}.log'
        with log.open('wb') as stream:
            result=subprocess.run(['C:/Program Files/Git/bin/bash.exe',str(R/'MDK-GCC/build.sh')],stdout=stream,stderr=subprocess.STDOUT)
        output=log.read_text(encoding='utf-8',errors='replace')
        assert result.returncode==0 and not re.search(r'warning:|error:',output),(lcd,output[-4000:])
        size=subprocess.check_output(['C:/Users/Li/.eide/tools/gcc_arm/bin/arm-none-eabi-size.exe',str(R/'MDK-GCC/SmartCar.elf')]).decode()
        text,data,bss=map(int,size.splitlines()[1].split()[:3])
        record['builds'][str(lcd)]={'text':text,'data':data,'bss':bss,'warnings':0,'errors':0}
        for ext in ('elf','hex','bin'):
            path=R/'MDK-GCC'/('SmartCar.'+ext)
            if path.exists():(R/f'MDK-GCC/build/phase17-g5-lcd{lcd}.{ext}').write_bytes(path.read_bytes())
        print('PASS LCD',lcd,text,data,bss,flush=True)
finally:
    for path,value in original.items():
        for attempt in range(5):
            try:
                if value is None:
                    if path.exists():path.unlink()
                elif not path.exists() or path.read_bytes()!=value:path.write_bytes(value)
                assert (path.read_bytes() if path.exists() else None)==value,path
                break
            except OSError:
                if attempt==4:raise
                time.sleep(.4)
    record['restored']={str(p.relative_to(R)):digest(p.read_bytes() if p.exists() else None) for p in paths}
    assert record['before']==record['restored']
    (R/'MDK-GCC/build/phase17-g5-build-results.json').write_text(json.dumps(record,indent=2),encoding='utf-8')
    print('RESTORED byte-exact config/ELF/HEX/BIN',flush=True)
