import sys, os, json, tempfile
from pathlib import Path
root=Path(__file__).parent/'SmartCar'
sys.path.insert(0,str(root/'tests'))
import phase16_check as p
import dual_mode_tracking as d
os.environ['PYTHONUTF8']='1'
original=(root/'App/app_config.h').read_bytes()
record={}
for name in ('datalog_v2','runtime_tuning','spi_log_export'):
    output=p.command([sys.executable,root/'tests'/f'{name}.py'],p.BUILD/f'audit-{name}.log')
    print(name,output.strip().splitlines()[-1],flush=True)
    record[name]='PASS'
for fn in (d.fsm_tests,d.param_tests,d.app_tests):
    with tempfile.TemporaryDirectory(prefix='audit_') as tmp: fn(Path(tmp))
    print(fn.__name__,'PASS',flush=True)
try:
    for lcd in (0,1):
        p.configuration(lcd,original)
        output=p.command([p.BASH,root/'MDK-GCC/build.sh'],p.BUILD/f'audit-lcd{lcd}.log')
        record[f'lcd{lcd}']=output.strip().splitlines()[-3:]
        print(record[f'lcd{lcd}'],flush=True)
    record['symbols']=p.symbol_evidence()
    record['stack']=p.stack_evidence(original)
finally: (root/'App/app_config.h').write_bytes(original)
(Path(__file__).parent/'audit-results.json').write_text(json.dumps(record,indent=2),encoding='utf-8')
print('Remaining verification complete',flush=True)
