from pathlib import Path
import subprocess,time
root=Path(__file__).resolve().parents[2]
paths=[root/'MDK-GCC'/('SmartCar.'+ext) for ext in ('elf','hex','bin')]
original={p:p.read_bytes() for p in paths if p.exists()}
config=root/'App/app_config.h';before=config.read_bytes()
for path,data in original.items():
    (root/'MDK-GCC/build'/('phase17-g4-post-approval-original-'+path.suffix[1:])).write_bytes(data)
(root/'MDK-GCC/build/phase17-g4-post-approval-original-config').write_bytes(before)
try:
    for lcd in (0,1):
        config.write_bytes(before.replace(b'#define LCD_TARGET_EXTERNAL 0',f'#define LCD_TARGET_EXTERNAL {lcd}'.encode()))
        log=root/f'MDK-GCC/build/phase17-g4-post-approval-lcd{lcd}.log'
        with log.open('wb') as stream:
            result=subprocess.run(['C:/Program Files/Git/bin/bash.exe',str(root/'MDK-GCC/build.sh')],stdout=stream,stderr=subprocess.STDOUT)
        print('LCD',lcd,'exit',result.returncode,flush=True)
        print(log.read_text(encoding='utf-8',errors='replace')[-500:],flush=True)
        assert result.returncode==0,log
finally:
    config.write_bytes(before)
    for path,data in original.items():
        for attempt in range(6):
            try:
                path.write_bytes(data)
                assert path.read_bytes()==data
                break
            except OSError:
                if attempt==5:raise
                time.sleep(.5)
    assert config.read_bytes()==before
    print('RESTORED exact original configuration and ELF/HEX/BIN bytes',flush=True)
