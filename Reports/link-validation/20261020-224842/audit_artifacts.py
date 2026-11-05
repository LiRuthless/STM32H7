import pathlib,json,hashlib,sys,struct,zlib
root=pathlib.Path.cwd(); out=pathlib.Path(sys.argv[1]); result={}
for run in ['bf24b81807e842f8b66fded4d9186e17','e86b5d91a3e84e3e91b91e17a049b510']:
    base=root/'ESP32Bridge/artifacts'/run
    if not (base/'manifest.json').exists():continue
    m=json.loads((base/'manifest.json').read_text(encoding='utf-8-sig'))
    r={'manifest_keys':list(m),'status':m.get('status'),'sources':[],'app_sha256':hashlib.sha256((base/'esp32bridge.bin').read_bytes()).hexdigest()}
    for f in m.get('source_files',[]):
        p=root/'ESP32Bridge'/f['path']
        # hash only; never copy source/config values into evidence
        r['sources'].append({'path':f['path'],'expected':f['sha256'],'current':hashlib.sha256(p.read_bytes()).hexdigest() if p.is_file() else None})
    r['source_mismatches']=[x['path'] for x in r['sources'] if x['expected']!=x['current']]
    data=(base/'esp32bridge.bin').read_bytes();magic=struct.pack('<I',0xABCD5432);i=data.find(magic)
    if i>=0:
        app=data[i:i+256]; cstr=lambda b:b.split(b'\0')[0].decode('utf-8',errors='replace')
        r['app_description']={'version':cstr(app[16:48]),'project':cstr(app[48:80]),'build_time':cstr(app[80:96]),'build_date':cstr(app[96:112]),'idf':cstr(app[112:144]),'elf_sha256':app[144:176].hex()}
    result[run]=r
result['stm32_artifacts']=[{'path':str(p.relative_to(root)),'size':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in [root/'SmartCar/MDK-ARM/SmartCar/SmartCar.hex',root/'SmartCar/MDK-ARM/SmartCar/SmartCar.axf',root/'SmartCar/MDK-ARM/flash-preflight-build.log',root/'SmartCar/MDK-ARM/flash-stlink.log',root/'SmartCar/MDK-ARM/flash-stlink-interactive.log'] if p.exists()]
(out/'firmware-audit.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
for k,v in result.items():print(k, json.dumps({a:b for a,b in v.items() if a!='sources'},ensure_ascii=True) if isinstance(v,dict) else json.dumps(v,ensure_ascii=True))
