from pathlib import Path
import re
import json
ROOT=Path(__file__).resolve().parents[2]
DRAFT=ROOT/'MDK-GCC/build/phase17-g5-draft'
changed=[]
def write(target,data):
    original=target.read_bytes() if target.exists() else b''
    if original and b'\r\n' in original:
        data=data.replace(b'\r\n',b'\n').replace(b'\n',b'\r\n')
    target.write_bytes(data)
    changed.append(target.relative_to(ROOT).as_posix())
for source in (DRAFT/'services').iterdir():
    write(ROOT/'App/services'/source.name,source.read_bytes())
for name in ('datalog.c','scb_protocol.c','scb_protocol.h'):
    (ROOT/'App/services'/name).unlink()
    changed.append('App/services/'+name)
for source in (DRAFT/'tests').iterdir():
    write(ROOT/'tests'/source.name,source.read_bytes())
project=ROOT/'MDK-ARM/SmartCar.uvprojx'
text=project.read_text(encoding='utf-8')
for old,new in [('datalog',['run_record','logger','log_store','log_export']),
                ('scb_protocol',['checksum','scb_frame','scb_json','scb_command_codec'])]:
    pattern=re.compile(r'(?m)^([ \t]*)<File>\s*<FileName>'+old+r'\.c</FileName>.*?</File>',re.S)
    def replacement(match):
        indent=match[1]
        return '\n'.join(indent+'<File>\n'+indent+'  <FileName>'+name+'.c</FileName>\n'
                         +indent+'  <FileType>1</FileType>\n'+indent+'  <FilePath>..\\App\\services\\'
                         +name+'.c</FilePath>\n'+indent+'</File>' for name in new)
    text,count=pattern.subn(replacement,text)
    assert count==1,(old,count)
write(project,text.encode())
(ROOT/'MDK-GCC/build/phase17-g5-paths.json').write_text(json.dumps(changed,indent=2),encoding='utf-8')
print('Applied',len(changed),'explicit G5 paths')
