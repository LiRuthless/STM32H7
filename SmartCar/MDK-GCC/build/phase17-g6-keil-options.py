from pathlib import Path
import re,xml.etree.ElementTree as E
p=Path('SmartCar/MDK-ARM/SmartCar.uvoptx');original=p.read_bytes();t=original.decode();nl='\r\n' if b'\r\n' in original else '\n';t=t.replace('\r\n','\n')
project=E.parse('SmartCar/MDK-ARM/SmartCar.uvprojx');groups=project.findall('.//Groups/Group')
blocks=re.findall(r'(?ms)^  <Group>\n.*?^  </Group>',t);byname={E.fromstring(b).findtext('GroupName'):b for b in blocks};old=byname['App'];app_blocks=[b for n,b in byname.items() if n=='App' or n.startswith('App/')];files=[f for b in app_blocks for f in re.findall(r'(?ms)^    <File>\n.*?^    </File>',b)]
for b in app_blocks:
 if b!=old:t=t.replace(b+'\n\n','');byfile={E.fromstring(f).findtext('FilenameWithoutPath'):f for f in files};template=files[0]
newgroups=[]
for g in groups:
 name=g.findtext('GroupName')
 if not (name=='App' or name.startswith('App/')):continue
 prefix=old[:old.index('    <File>')];prefix=prefix.replace('<GroupName>App</GroupName>',f'<GroupName>{name}</GroupName>');parts=[]
 for f in g.findall('Files/File'):
  fname=f.findtext('FileName');node=byfile.get(fname,template)
  node=re.sub(r'(?<=<PathWithFileName>).*?(?=</PathWithFileName>)',lambda m:f.findtext('FilePath').replace('\\','/'),node)
  node=re.sub(r'(?<=<FilenameWithoutPath>).*?(?=</FilenameWithoutPath>)',lambda m:fname,node);parts.append(node)
 newgroups.append(prefix+'\n'.join(parts)+'\n  </Group>')
t=t.replace(old,'\n\n'.join(newgroups))
number=0;indexes={}
for gi,g in enumerate(groups,1):
 for f in g.findall('Files/File'):
  number+=1;indexes[(p.parent/f.findtext('FilePath')).resolve()]=(gi,number)
for b in re.findall(r'(?ms)^  <Group>\n.*?^  </Group>',t):
 replacement=b
 for f in re.findall(r'(?ms)^    <File>\n.*?^    </File>',b):
  path=(p.parent/E.fromstring(f).findtext('PathWithFileName')).resolve()
  if path not in indexes:raise RuntimeError('stale non-App option file '+str(path))
  groupnum,filenum=indexes[path];changed=re.sub(r'<GroupNumber>\d+</GroupNumber>',f'<GroupNumber>{groupnum}</GroupNumber>',f)
  changed=re.sub(r'<FileNumber>\d+</FileNumber>',f'<FileNumber>{filenum}</FileNumber>',changed);replacement=replacement.replace(f,changed)
 t=t.replace(b,replacement)
p.write_bytes(t.replace('\n',nl).encode());print('App option groups synchronized;',len(newgroups),'groups; non-App settings preserved')
