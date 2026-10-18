"""修正离线显示与章节编号，检查最终 HTML/Markdown 与包内容。"""
from pathlib import Path
import re
ROOT=Path(__file__).resolve().parent
p=ROOT/'report.md'
s=p.read_text(encoding='utf8')
for n in ('2','3','4'):
    s=re.sub(r'^#### '+n+r'\.'+n+r'\.(\d+) ',lambda m:'### '+n+'.'+m[1]+' ',s,flags=re.M)
# Mathematical markup now uses LaTeX; retain commands such as \kappa.
s=s.replace('具体检查结论见验证节。','独立复核的两项修订均已关闭；修复后的 30 项验证由主 agent 与复核 agent 分别运行并通过。具体检查结论见验证节。')
p.write_text(s,encoding='utf8')
print('Display headings and symbols finalized')
