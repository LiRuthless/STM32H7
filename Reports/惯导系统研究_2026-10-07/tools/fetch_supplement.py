"""Fetch a few additional public code files at previously recorded commits."""
from pathlib import Path
import urllib.request, json, hashlib
ROOT=Path(__file__).resolve().parents[1]
m=json.loads((ROOT/'source_manifest.json').read_text(encoding='utf-8'))
cache=Path(m['cache'])
extra={'qtr':['QTRSensors.cpp','library.properties'],'zumo':['library.properties'],'nav2':['nav2_regulated_pure_pursuit_controller/src/regulated_pure_pursuit_controller.cpp']}
for r in m['repositories']:
 for f in extra.get(r['id'],[]):
  url=f"https://raw.githubusercontent.com/{r['repo']}/{r['commit']}/{f}"
  try:
   data=urllib.request.urlopen(urllib.request.Request(url,headers={'User-Agent':'Research-report/1.0'}),timeout=30).read()
   (cache/(r['id']+'_'+f.replace('/','_'))).write_bytes(data)
   record={'file':f,'url':url,'sha256':hashlib.sha256(data).hexdigest()}
   r['files_readable']=[i for i in r['files_readable'] if i['file']!=f]+[record]
   print(r['id'],f,'downloaded')
  except Exception as e:print(r['id'],f,str(e))
m['reading_access_notes']={'mahony':'本地下载403，但web全文阅读工具成功读取16页作者稿（hal-00488376 v1/稿件v3）。','coulter':'本地下载证书校验失败，但web全文阅读工具成功读取15页CMU官方PDF。','wheelins':'arXiv当前下载确认为v4、12页；早期v2网页仅作版本核对，性能引用采用v4。'}
(ROOT/'source_manifest.json').write_text(json.dumps(m,ensure_ascii=False,indent=2),encoding='utf-8')
