"""Document integrity checks; these do not validate real vehicle performance."""
from pathlib import Path
import re, json, hashlib, math, xml.etree.ElementTree as ET
ROOT=Path(__file__).resolve().parents[1]
md=(ROOT/'report.md').read_text(encoding='utf-8')
sources=(ROOT/'sources.md').read_text(encoding='utf-8')
coverage=(ROOT/'coverage.md').read_text(encoding='utf-8')
checks={}
checks['chapters_1_to_18']=all(re.search(r'^## '+str(i)+r'\.',md,re.M) for i in range(1,19))
checks['source_ids_1_to_18']=all(f'## S{i:02d}：' in sources for i in range(1,19))
checks['coverage_rows_1_to_16']=all(re.search(r'^\| '+str(i)+r' \|',coverage,re.M) for i in range(1,17))
checks['paired_code_fences']=md.count('```')%2==0
checks['paired_display_math']=len(re.findall(r'^\$\$$',md,re.M))%2==0
checks['no_old_formula_fences']='```text' not in md
table_errors=[]; expected=None
for i,line in enumerate(md.splitlines(),1):
 if line.startswith('|'):
  columns=len(re.split(r'(?<!\\)\|',line))-1
  if expected is None:expected=columns
  elif columns!=expected:table_errors.append({'line':i,'actual':columns,'expected':expected})
 else:expected=None
checks['markdown_table_columns']=not table_errors
figures=re.findall(r'!\[[^\]]*\]\((figures/[^)]+)\)',md)
checks['six_named_figures']=len(figures)==6 and len(set(figures))==6
checks['all_svg_parse']=all((ROOT/f).exists() for f in figures)
if checks['all_svg_parse']:
 for f in figures:ET.parse(ROOT/f)
checks['no_unresolved_placeholders']=not re.search(r'TODO|待补|待填写|turn\d+(?:view|search)|cite',md)
r=json.loads((ROOT/'data/results.json').read_text(encoding='utf-8'))
checks['numerical_examples_match']=abs(r['false_arrival']['shortfall_m']-.4761904761904763)<1e-12 and abs(r['tilt_1deg_10s']['dp_m']-8.557482079409318)<1e-12
html_path=ROOT/'报告.html'
if html_path.exists():
 html=html_path.read_text(encoding='utf-8')
 checks['html_six_inline_svg']=len(re.findall(r'<figure><svg',html))==6
 checks['math_typeset']=len(re.findall(r'class="math-display"',html))==35 and len(re.findall(r'class="math-inline"',html))>=500
 checks['embedded_math_fonts']='data:font/woff2;base64,' in html
 checks['html_no_external_required_assets']=not re.search(r'<(?:script|link|img)[^>]+(?:src|href)=[\"\']https?://',html)
 checks['html_utf8']=bool(re.search(r'<meta[^>]+charset=[\"\']?utf-8',html,re.I))
else:checks['html_exists']=False
result={'scope':'文档完整性与数值一致性，非实车性能验证','checks':checks,'table_errors':table_errors,'passed':all(checks.values()),'markdown_characters':len(md),'report_sha256':hashlib.sha256((ROOT/'report.md').read_bytes()).hexdigest()}
(ROOT/'qa').mkdir(exist_ok=True)
(ROOT/'qa'/'integrity.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(result,ensure_ascii=False,indent=2))
if not result['passed']:raise SystemExit(1)
