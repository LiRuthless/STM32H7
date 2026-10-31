from pathlib import Path
import re
R=Path(__file__).resolve().parents[2];A=R/'App'
modules=['control/tuning','control/tuning_values','system/parameter_application','system/tuning_application',
         'services/parameter_bank','services/parameter_image','services/parameter_store',
         'services/command_dispatch','hmi/menu','hmi/menu_keys','hmi/menu_render']
def tokens(s):return re.findall(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|\w+|[^\s]',re.sub(r'/\*.*?\*/','',s,flags=re.S))
for m in modules:
    for ext in ['.c','.h']:
        p=A/(m+ext)
        if not p.exists():continue
        original=p.read_text(encoding='utf-8');s=original
        # Multiline aggregate braces are formatted without changing executable tokens.
        s=re.sub(r'(\([^()]*_t\))\{\n',r'\1\n    {\n',s)
        s=re.sub(r'(\([^()]*_t\))\{(?=[^\n]*\n)',r'\1\n    {\n        ',s)
        # A multiline initializer ends on its own line. Single-line compounds remain compact.
        lines=s.splitlines();out=[];compound=False
        for line in lines:
            if re.match(r'\s*\{\s*$',line) and out and re.search(r'\([^()]*_t\)\s*$',out[-1]):compound=True
            if compound and re.search(r'\};\s*$',line) and line.strip()!='};':
                line=line[:line.rfind('}')].rstrip();out.append(line)
                out.append('    };');compound=False;continue
            if compound and line.strip()=='};':compound=False
            out.append(line)
        s='\n'.join(out)
        s=re.sub(r'[ ]+/\*\*<',' /**<',s)
        if m.startswith('hmi/menu') and ext=='.c':
            s=s.replace(' */\n#include',' */\n/*\n * 移植来源：Sirius20260718/project/code/menu.c，STC AI8051U省赛快照（2026-07-18）。\n * 原算法未单列作者或独立许可证；导航、采集和绘制职责由Phase17拆分。\n */\n#include',1)
        s=s.replace('HAL 毫秒 tick','BSP毫秒tick')
        lines=[]
        for line in s.splitlines():
            if len(line)>100 and line.lstrip().startswith(('/*','*')) and 'http' not in line:
                indent=line[:len(line)-len(line.lstrip())]
                if line.lstrip().startswith('/*') and line.rstrip().endswith('*/'):
                    content=line.strip()[2:-2].strip();line=indent+'/*\n'+indent+' * '+content+'\n'+indent+' */'
                    for v in line.splitlines():
                        while len(v)>100:
                            cut=v.rfind(' ',10,98)
                            if cut<10:cut=96
                            lines.append(v[:cut]);v=indent+' *     '+v[cut:].lstrip()
                        lines.append(v)
                    continue
            lines.append(line.rstrip())
        s='\n'.join(lines)+'\n'
        assert tokens(original)==tokens(s),p
        p.write_bytes(s.replace('\n','\r\n').encode('utf-8'))
        for i,line in enumerate(s.splitlines(),1):
            if len(line)>100 and '"' not in line:print('LONG',p.relative_to(R),i,len(line),line)
print('G4 formatting executable tokens unchanged')
