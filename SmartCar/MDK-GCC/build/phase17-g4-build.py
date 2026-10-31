from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[2]
paths=[root/'MDK-GCC'/('SmartCar.'+ext) for ext in ('elf','hex','bin')]
original={p:p.read_bytes() for p in paths if p.exists()}
config=root/'App/app_config.h';before=config.read_bytes()
try:
    for lcd in (0,1):
        config.write_bytes(before.replace(b'#define LCD_TARGET_EXTERNAL 0',f'#define LCD_TARGET_EXTERNAL {lcd}'.encode()))
        log=root/f'MDK-GCC/build/phase17-g4-lcd{lcd}.log'
        with log.open('wb') as stream:
            result=subprocess.run(['C:/Program Files/Git/bin/bash.exe',str(root/'MDK-GCC/build.sh')],stdout=stream,stderr=subprocess.STDOUT)
        print(lcd,result.returncode,log)
        print(log.read_text(encoding='utf-8',errors='replace')[-3500:])
        if result.returncode:break
finally:
    config.write_bytes(before)
    for path,data in original.items():path.write_bytes(data)
    assert config.read_bytes() == before
    assert all(path.read_bytes() == data for path, data in original.items())
    print('RESTORED exact app_config and original elf/hex/bin bytes')

