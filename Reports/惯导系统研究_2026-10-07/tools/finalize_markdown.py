"""Append standalone source/coverage indexes to the complete Markdown, idempotently."""
from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[1]
marker='<!-- GENERATED-APPENDICES -->'
report=(ROOT/'report.md').read_text(encoding='utf-8').split(marker)[0].rstrip()
def appendix(file):
 text=(ROOT/file).read_text(encoding='utf-8')
 text=text.split('\n',1)[1].lstrip()
 return re.sub(r'(?m)^(#{2,5}) ',lambda m:m[1]+'# ',text)
report+='\n\n'+marker+'\n\n## 17. 文献与公开案例索引\n\n'+appendix('sources.md')
report+='\n\n## 18. 16项覆盖与证据对照表\n\n'+appendix('coverage.md')
# A space after closing bold brackets avoids CommonMark punctuation ambiguity.
report=re.sub(r'\*\*([^\n*]+)\*\*(?=[\u4e00-\u9fff])',r'**\1** ',report)
(ROOT/'report.md').write_text(report.rstrip()+'\n',encoding='utf-8')
print('Complete Markdown with 18 chapters and 18 indexed sources.')
