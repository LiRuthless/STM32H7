from pathlib import Path
import re,json,sys
R=Path(__file__).resolve().parents[2];A=R/'App'
mods=['control/tuning','control/tuning_values','system/parameter_application','system/tuning_application',
      'services/parameter_bank','services/parameter_image','services/parameter_store',
      'services/command_dispatch','hmi/menu','hmi/menu_keys','hmi/menu_render']
report=[];issues=[]
for m in mods:
    p=A/(m+'.c')
    if not p.exists():continue
    s=p.read_text(encoding='utf-8')
    for f in re.finditer(r'(?m)^(?:static )?([\w *]+)\b(\w+)\(([^;]*?)\)\n\{',s):
        n=f[2];doc=s[s.rfind('/**',0,f.start()):f.start()]
        assert doc.rstrip().endswith('*/'),(p,n)
        pars=[] if f[3].strip()=='void' else [re.search(r'(\w+)\s*$',v.strip())[1] for v in f[3].split(',')]
        tags=re.findall(r'@param\[(?:in|out|in,out)\]\s+(\w+)',doc)
        if pars!=tags:issues.append((str(p.relative_to(R)),n,'params',pars,tags))
        if '@brief' not in doc:issues.append((str(p),n,'brief'))
        if f[1].strip()!='void' and '@return' not in doc:issues.append((str(p.relative_to(R)),n,'return'))
        report.append(dict(path=str(p.relative_to(R)).replace('\\','/'),symbol=n,parameters=pars,
                           details='@details' in doc,returns='@return' in doc))
for m in mods:
    for ext in ['.c','.h']:
        p=A/(m+ext)
        if not p.exists():continue
        data=p.read_bytes();s=data.decode('utf-8')
        assert data.endswith(b'\r\n') and not data.endswith(b'\r\n\r\n'),p
        assert not data.startswith(b'\xef\xbb\xbf'),p
        for i,line in enumerate(s.splitlines(),1):
            if '\t' in line or line.rstrip()!=line:issues.append((str(p.relative_to(R)),i,'whitespace'))
            if len(line)>100 and '"' not in line:issues.append((str(p.relative_to(R)),i,'long',len(line)))
(R/'MDK-GCC/build/phase17-g4-comment-inventory.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print('G4 function declarations reviewed',len(report))
print(json.dumps(issues,ensure_ascii=False,indent=2));sys.exit(bool(issues))
