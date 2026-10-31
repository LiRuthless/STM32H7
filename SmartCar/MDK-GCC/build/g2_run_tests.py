from pathlib import Path
import os,subprocess,sys
r=Path(__file__).resolve().parents[2]
os.environ['PYTHONUTF8']='1';os.environ['PATH']='E:/APP/mingw64/bin;'+os.environ['PATH']
os.environ['TEMP']=os.environ['TMP']=str(r/'MDK-GCC/build/host-temp')
tests=sys.argv[1:] or ['control_equivalence','runtime_tuning','datalog_v2','scb_protocol','scb_link','spi_wifi_state','spi_log_export','app_layout_check']
for name in tests:
    log=r/f'MDK-GCC/build/phase17-g2-{name}.log'
    with log.open('wb') as out:
        result=subprocess.run([sys.executable,str(r/'tests'/f'{name}.py')],stdout=out,stderr=subprocess.STDOUT)
    print(name,result.returncode,flush=True)
    print(log.read_text(encoding='utf-8',errors='replace')[-6500:],flush=True)
