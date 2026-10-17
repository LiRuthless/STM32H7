from pathlib import Path
import re
import json
import hashlib
from html.parser import HTMLParser
import xml.etree.ElementTree as ET
root=Path(__file__).resolve().parent
audit=json.loads((root/'qa/math_render_audit.json').read_text(encoding='utf8'))
html=(root/'光电感知与惯性导航_技术报告.html').read_text(encoding='utf8')
md=(root/'report.md').read_text(encoding='utf8')
old=json.loads((root/'manifest.json').read_text(encoding='utf8'))
for f in ['optical.py','inertial.py','demo.py','test_algorithms.py']:
    data=(root/'code'/f).read_bytes()
    assert hashlib.sha256(data).hexdigest()==old['code/'+f]['sha256'],f+' changed'
assert audit['display']>=20 and audit['inline']>=150,audit['total']
assert 'katex-error' not in html
assert 'data-math-slot=' not in html
assert 'src="http' not in html and '<script src=' not in html
assert 'url(fonts/' not in html
assert html.count('data:font/woff2;base64,')==20
mathml=re.findall(r'(<math\b.*?</math>)',html,re.S)
assert len(mathml)==audit['total']
for math in mathml:ET.fromstring(math)
# Every legacy mathematical text block must have been replaced, except the API flow.
textblocks=re.findall(r'```text\n(.*?)```',md,re.S)
assert len(textblocks)==1 and 'Navigator.update' in textblocks[0]
for value in ['0.585365854','0.837837838','0.024','0.0186','0.306967']:
    assert value in md,value
summary={'engine':audit['engine'],'display_formulas':audit['display'],'inline_formulas':audit['inline'],'total_mathml_checked':len(mathml),'embedded_math_fonts':20,'code_hashes_unchanged':True,'external_font_or_script_requests':0,'legacy_formula_blocks':0}
(root/'qa/math_validation.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2),encoding='utf8')
print(json.dumps(summary,ensure_ascii=False))
