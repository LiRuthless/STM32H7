"""Vendor a pinned official KaTeX distribution for completely offline typesetting."""
from pathlib import Path
import urllib.request, json, tarfile, io, hashlib
ROOT=Path(__file__).resolve().parents[1]
VERSION='0.19.0'
URL=f'https://registry.npmjs.org/katex/-/katex-{VERSION}.tgz'
data=urllib.request.urlopen(URL,timeout=45).read()
dest=ROOT/'assets'/'katex';dest.mkdir(parents=True,exist_ok=True)
selected=[]
with tarfile.open(fileobj=io.BytesIO(data),mode='r:gz') as archive:
 for member in archive.getmembers():
  rel=member.name.removeprefix('package/')
  if rel in ['dist/katex.js','dist/katex.min.css','LICENSE','package.json'] or (rel.startswith('dist/fonts/') and rel.endswith('.woff2')):
   if not member.isfile():continue
   target=dest/rel
   if not target.resolve().is_relative_to(dest.resolve()):raise ValueError('Unsafe archive path')
   target.parent.mkdir(parents=True,exist_ok=True)
   raw=archive.extractfile(member).read();target.write_bytes(raw)
   selected.append({'path':rel,'sha256':hashlib.sha256(raw).hexdigest()})
manifest={'version':VERSION,'source':URL,'retrieved_date':'2026-10-08','archive_sha256':hashlib.sha256(data).hexdigest(),'files':selected,'usage':'Node构建时预渲染；HTML内嵌CSS和WOFF2字体，不需要客户端KaTeX脚本或网络'}
(dest/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
print('Vendored KaTeX',VERSION,len(selected),'files')
