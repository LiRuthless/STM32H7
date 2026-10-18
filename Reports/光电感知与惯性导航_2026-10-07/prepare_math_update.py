from pathlib import Path
import json
import urllib.request
import tarfile
import io
import hashlib

root=Path(__file__).resolve().parent
parts=root/'math_parts'
parts.mkdir(exist_ok=True)
text=(root/'report.md').read_text(encoding='utf8')
(root/'qa/report_before_math_update.md').write_text(text,encoding='utf8')
sections={i:text.index(f'## {i} ') for i in range(1,9)}
(parts/'optical.md').write_text(text[:sections[3]],encoding='utf8')
(parts/'pipeline.md').write_text(text[sections[3]:sections[4]],encoding='utf8')
(parts/'inertial.md').write_text(text[sections[4]:sections[5]],encoding='utf8')
(parts/'implementation.md').write_text(text[sections[5]:sections[7]],encoding='utf8')
(parts/'references.md').write_text(text[sections[7]:],encoding='utf8')
vendor=root/'vendor/katex'
vendor.mkdir(parents=True,exist_ok=True)
version='0.16.22'
url=f'https://registry.npmjs.org/katex/-/katex-{version}.tgz'
payload=urllib.request.urlopen(url,timeout=30).read()
with tarfile.open(fileobj=io.BytesIO(payload),mode='r:gz') as archive:
    for member in archive.getmembers():
        relative=Path(member.name).relative_to('package')
        if str(relative) in ('LICENSE','package.json') or str(relative).replace('\\','/').startswith('dist/'):
            target=vendor/relative
            if member.isfile():
                target.parent.mkdir(parents=True,exist_ok=True)
                target.write_bytes(archive.extractfile(member).read())
(vendor/'provenance.json').write_text(json.dumps({'version':version,'source':url,'sha256':hashlib.sha256(payload).hexdigest()},indent=2),encoding='utf8')
print('Split report and fetched pinned KaTeX distribution',version)
