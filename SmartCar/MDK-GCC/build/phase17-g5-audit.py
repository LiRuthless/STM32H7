from pathlib import Path
import importlib.util,json,re,sys
R=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('prep',R/'MDK-GCC/build/phase17-g5-prepare.py')
g=importlib.util.module_from_spec(spec);spec.loader.exec_module(g)
paths=json.loads((R/'MDK-GCC/build/phase17-g5-paths.json').read_text(encoding='utf-8'))
issues=[];inventory=[]
for relative in paths:
    path=R/relative
    if path.suffix not in ('.c','.h') or not path.exists():continue
    raw=path.read_bytes();text=path.read_text(encoding='utf-8')
    assert not raw.startswith(b'\xef\xbb\xbf'),relative
    if not text.endswith('\n') or text.endswith('\n\n'):issues.append((relative,'EOF'))
    for n,line in enumerate(text.splitlines(),1):
        if '\t' in line or line.rstrip()!=line:issues.append((relative,n,'whitespace'))
        if len(line)>100 and '"' not in line:issues.append((relative,n,'long',len(line),line))
    if path.suffix=='.h':continue
    for name,value in g.functions(text).items():
        assert value.startswith('/**'),(relative,name)
        end=value.index('*/')+2;doc=value[:end];signature=g.signature(value)
        args=signature[signature.index('(')+1:signature.rindex(')')]
        params=[] if args.strip()=='void' else [re.search(r'(\w+)(?:\[[^]]*\])?$',p.strip())[1] for p in args.split(',')]
        actual=re.findall(r'@param\[(?:in|out|in,out)\] (\w+)',doc)
        if actual!=params:issues.append((relative,name,'params',params,actual))
        result=signature.split(name+'(')[0].strip()
        if result not in ('void','static void') and '@return ' not in doc:issues.append((relative,name,'return'))
        if '@brief ' not in doc:issues.append((relative,name,'brief'))
        if re.search(r'按现有调用顺序|函数体既定|按原逻辑|同上|待确认',doc):issues.append((relative,name,'placeholder'))
        inventory.append(dict(path=relative,symbol=name,parameters=params,details='@details ' in doc))
(R/'MDK-GCC/build/phase17-g5-comment-inventory.json').write_text(json.dumps(inventory,ensure_ascii=False,indent=2),encoding='utf-8')
print('G5 inspected C definitions',len(inventory))
print(json.dumps(issues,ensure_ascii=False,indent=2));sys.exit(bool(issues))
