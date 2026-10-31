from pathlib import Path
p=Path(__file__).with_name('phase17-g4-tuning.py')
__file__=str(p)
exec(p.read_text(encoding='utf-8').split('# Original runtime call sites')[0])
# Verify all extracted baseline functions by lexical matching, not just serializers.
for module in ['param.c','menu.c','tuning.c']:
    src=read(module)
    names=re.findall(r'(?m)^(?:static )?[\w *]+\b(\w+)\([^;]*?\)\n\{',src)
    for name in names:
        f=extract(src,name)
        assert f.rstrip().endswith('}'),name
    print('lexical extraction verified',module,len(names))
k=ROOT/'MDK-ARM/SmartCar.uvprojx';xml=k.read_text(encoding='utf-8')
xml=re.sub(r'\s*<File>\s*<FileName>param.c</FileName>.*?</File>','',xml,flags=re.S)
for cat in ['system','services','control','hmi']:
    g=re.search(r'(<GroupName>App/'+cat+r'</GroupName>\s*<Files>)(.*?)(</Files>)',xml,re.S)
    assert g,cat
    entries=g[2]
    for f in sorted((A/cat).glob('*.c')):
        if '<FileName>'+f.name+'</FileName>' not in entries:
            entries+='\n          <File>\n            <FileName>'+f.name+'</FileName>\n            <FileType>1</FileType>\n            <FilePath>..\\App\\'+cat+'\\'+f.name+'</FilePath>\n          </File>\n'
    xml=xml[:g.start(2)]+entries+xml[g.end(2):]
k.write_bytes(xml.replace('\n','\r\n').encode('utf-8'))
