import os,sys,re,json,subprocess,shutil
from pathlib import Path
root=Path(__file__).resolve().parents[3]
repo=root.parent
out=Path(__file__).parent
base='e1771e5'
files=[f'BSP/bsp_{m}{e}' for m in ('adc','dl1b','key','pwm') for e in ('.c','.h')]+['App/pid.c','App/pid.h']
# Translation phase 2 precedes comment recognition. Preserve string/character literals.
lex=re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|/\*.*?\*/|//[^\n]*',re.S)
def stripped(s):
    s=s.replace('\r\n','\n').replace('\\\n','')
    return lex.sub(lambda m:' ' + '\n'*m[0].count('\n') if m[0].startswith(('/*','//')) else m[0],s)
def tokens(s):
    s=stripped(s)
    pattern=r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|(?:\d|\.\d)(?:[\w.]|[eEpP][+-])*|[a-zA-Z_]\w*|>>=|<<=|\.\.\.|->|\+\+|--|<<|>>|<=|>=|==|!=|&&|\|\||[+*/%&|^!-]=|##|[^\s]'
    directives=[re.findall(pattern,x) for x in s.splitlines() if x.lstrip().startswith('#')]
    return re.findall(pattern,s),directives
assert tokens('char*s="/* hi */"; /* x */\n#define X \\\n1')[0]==tokens('char*s="/* hi */"; /* y */\n#define X 1')[0]
results={}
for f in files:
    before=subprocess.check_output(['git','show',base+':SmartCar/'+f],cwd=repo).decode('utf-8')
    after=(root/f).read_text(encoding='utf-8')
    assert tokens(before)==tokens(after),f
results['noncomment_tokens_and_directives']='10/10 identical'
if '--tokens-only' in sys.argv:
    assert tokens('x++;')!=tokens('x+ +;')
    assert tokens('"// text";')!=tokens('"/* text */";')
    print(results)
    sys.exit(0)
copy=out/'isolated'/'SmartCar'
copy.mkdir(parents=True,exist_ok=True)
for folder in ('App','BSP','Core','Drivers','tests','MDK-GCC','MDK-ARM'):
    shutil.copytree(root/folder,copy/folder,dirs_exist_ok=True,
                    ignore=shutil.ignore_patterns('build','__pycache__','*.elf','*.bin','*.hex'))
sys.path.insert(0,str(copy/'tests'))
import phase16_check as p
import dual_mode_tracking as d
os.environ['PATH']='E:/APP/mingw64/bin;'+os.environ['PATH']
os.environ['PYTHONUTF8']='1'
for name in ('control_equivalence','dual_mode_tracking','datalog_v2'):
    run=subprocess.run([sys.executable,str(copy/'tests'/f'{name}.py')],cwd=repo,capture_output=True)
    (out/f'{name}.log').write_bytes(run.stdout+run.stderr)
    results[name]={'exit':run.returncode,'tail':(run.stdout+run.stderr).decode('utf-8',errors='replace')[-1200:]}
    print(name,run.returncode,flush=True)
for fn in (d.fsm_tests,d.param_tests,d.app_tests):
    import tempfile
    with tempfile.TemporaryDirectory(prefix='r08_') as tmp: fn(Path(tmp))
    results[fn.__name__]='PASS'
original=(copy/'App/app_config.h').read_bytes()
for lcd in (0,1):
    p.configuration(lcd,original)
    run=subprocess.run([str(p.BASH),str(copy/'MDK-GCC/build.sh')],cwd=repo,capture_output=True)
    (out/f'lcd{lcd}.log').write_bytes(run.stdout+run.stderr)
    results[f'lcd{lcd}']={'exit':run.returncode,'tail':run.stdout.decode(errors='replace')[-600:]}
    print('LCD',lcd,run.returncode,flush=True)
(copy/'App/app_config.h').write_bytes(original)
(out/'results.json').write_text(json.dumps(results,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(results,ensure_ascii=False),flush=True)
