"""只打包本次明确交付路径，校验归档内容与源码一致。"""
from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED
from html.parser import HTMLParser
import hashlib
import json
ROOT=Path(__file__).resolve().parent
class Structure(HTMLParser):
    def __init__(self):super().__init__();self.counts={};self.images=[]
    def handle_starttag(self,tag,attrs):
        self.counts[tag]=self.counts.get(tag,0)+1
        if tag=='img':self.images.append(dict(attrs).get('src'))
html=(ROOT/'光电感知与惯性导航_技术报告.html').read_text(encoding='utf8')
parser=Structure();parser.feed(html)
assert html.count('<figure class="diagram">')==2
assert parser.counts.get('table',0)>=8
assert parser.counts.get('h1')==1
assert '<!-- TEST_RESULTS -->' not in html
assert not parser.images, 'HTML diagrams must be embedded, offline'
assert 'src="http' not in html and '<script src=' not in html
text=(ROOT/'report_full.md').read_text(encoding='utf8')
assert '\\kappa' in text, 'Greek symbols must use mathematical markup'
assert 'class="katex-error"' not in html
assert 'data-math-slot=' not in html
assert 'data:font/woff2;base64,' in html
for f in ['optical.py','inertial.py','demo.py','test_algorithms.py']:
    assert (ROOT/'code'/f).read_text(encoding='utf8') in text
files=[ROOT/f for f in ['光电感知与惯性导航_技术报告.html','report_full.md','README.md','review.md','validation.txt','流程图.svg','思维导图.svg']]
files.append(ROOT/'vendor/katex/LICENSE')
files.append(ROOT/'qa/math_render_audit.json')
files += sorted((ROOT/'code').glob('*.py'))
files += sorted((ROOT/'code/output').glob('*'))
manifest={str(f.relative_to(ROOT)).replace('\\','/'):{'bytes':f.stat().st_size,'sha256':hashlib.sha256(f.read_bytes()).hexdigest()} for f in files}
(ROOT/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf8')
files.append(ROOT/'manifest.json')
with ZipFile(ROOT/'交付包.zip','w',ZIP_DEFLATED) as z:
    for f in files:z.write(f,f.relative_to(ROOT))
with ZipFile(ROOT/'交付包.zip') as z:
    assert z.testzip() is None
    for f in files:assert z.read(str(f.relative_to(ROOT)).replace('\\','/'))==f.read_bytes()
print(json.dumps({'package_files':len(files),'zip_bytes':(ROOT/'交付包.zip').stat().st_size,'html_tables':parser.counts.get('table'),'svg':parser.counts.get('svg'),'code_appendix_matches':True},ensure_ascii=False))
