"""Ensure the typesetting migration preserves all original equation references."""
from pathlib import Path
import re, subprocess, json
ROOT=Path(__file__).resolve().parents[1]
REPO=ROOT.parents[1]
relative=(ROOT/'report.md').relative_to(REPO).as_posix()
old=subprocess.check_output(['git','show',f'aa93de8:{relative}'],cwd=REPO).decode('utf-8')
new=(ROOT/'report.md').read_text(encoding='utf-8')
old_tags=set(re.findall(r'^.*\((\d+-\d+)\)\s*$',old,re.M))
new_tags=set(re.findall(r'\\tag\{(\d+-\d+)\}',new))
result={'original_equation_numbers':sorted(old_tags),'new_equation_numbers':sorted(new_tags),'preserved':old_tags==new_tags,'display_blocks':len(re.findall(r'^\$\$$',new,re.M))//2,'old_formula_code_blocks_remaining':new.count('```text')}
(ROOT/'qa'/'math_2026-10-08'/'migration_check.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
print(json.dumps(result,indent=2))
if not result['preserved'] or result['old_formula_code_blocks_remaining']:raise SystemExit(1)
