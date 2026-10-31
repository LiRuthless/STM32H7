from pathlib import Path
import subprocess,sys,os,json,time
R=Path(__file__).resolve().parents[2]
T=R/'MDK-GCC/build/host-temp';T.mkdir(exist_ok=True)
os.environ.update(TEMP=str(T),TMP=str(T),PYTHONUTF8='1')
os.environ['PATH']='E:/APP/mingw64/bin;'+os.environ['PATH']
names=['scb_protocol','scb_link','spi_wifi_state','control_equivalence','datalog_v2',
       'runtime_tuning','spi_log_export','g2_system','g3_control',
       'g4_boundaries','g4_transactions','g4_hmi','app_layout_check','dual_mode_tracking']
results={}
for name in names:
    start=time.time();log=R/f'MDK-GCC/build/phase17-g5-{name}.log'
    with log.open('wb') as stream:
        run=subprocess.run([sys.executable,str(R/'tests'/f'{name}.py')],stdout=stream,stderr=subprocess.STDOUT)
    text=log.read_text(encoding='utf-8',errors='replace')
    results[name]={'exit_code':run.returncode,'seconds':round(time.time()-start,2),'tail':text[-1400:]}
    print(name,run.returncode,results[name]['seconds'],text[-1000:] if run.returncode else text.strip().splitlines()[-1],flush=True)
    (R/'MDK-GCC/build/phase17-g5-host-results.json').write_text(json.dumps(results,ensure_ascii=False,indent=2),encoding='utf-8')
with (R/'MDK-GCC/build/phase17-g5-dual-independent.log').open('wb') as stream:
    code="import sys,tempfile;from pathlib import Path;sys.path.insert(0,"+repr(str(R/'tests'))+");import dual_mode_tracking as d;\nwith tempfile.TemporaryDirectory(prefix='g5_independent_') as p:\n w=Path(p);d.fsm_tests(w);d.param_tests(w);d.app_tests(w)"
    run=subprocess.run([sys.executable,'-c',code],stdout=stream,stderr=subprocess.STDOUT)
print('independent_dual',run.returncode,flush=True)
