from pathlib import Path
import re
root=Path(__file__).resolve().parent
names=['optical','pipeline','inertial','implementation','references']
text=''.join((root/'math_parts'/f'{n}.md').read_text(encoding='utf8').rstrip()+'\n\n' for n in names)
# Formula rendering is a revision; the research source date remains unchanged.
text=text.replace('中文技术报告 · 2026 年 10 月 7 日','中文技术报告 · 2026 年 10 月 7 日 · 数学排版修订 2026 年 10 月 9 日')
if text.count('$$')%2:raise ValueError('Unbalanced display delimiters')
if len(re.findall(r'^## [1-8] ',text,re.M))!=8:raise ValueError('Missing sections')
(root/'report.md').write_text(text,encoding='utf8')
print('Merged mathematical markup:',len(text),'characters')
