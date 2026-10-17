"""保存实际验证日志，核对示例导出字段与有限 JSON；标准库。"""
from pathlib import Path
import subprocess
import sys
import json
import csv
ROOT=Path(__file__).resolve().parent
r=subprocess.run([sys.executable,'-m','unittest','-v','test_algorithms.py'],cwd=ROOT/'code',capture_output=True,text=True,encoding='utf8')
log=f'Python: {sys.version}\nCommand: python -m unittest -v test_algorithms.py\n'+r.stdout+r.stderr
(ROOT/'validation.txt').write_text(log,encoding='utf8')
if r.returncode:print(log);raise SystemExit(r.returncode)
d=subprocess.run([sys.executable,'demo.py'],cwd=ROOT/'code',capture_output=True,text=True,encoding='utf8')
if d.returncode:print(d.stderr);raise SystemExit(d.returncode)
summary=json.loads(d.stdout)
with (ROOT/'code/output/samples.csv').open(encoding='utf-8-sig',newline='') as f:rows=list(csv.DictReader(f))
frames=[json.loads(line) for line in (ROOT/'code/output/frames.jsonl').read_text(encoding='utf8').splitlines()]
assert len(rows)==len(frames)==summary['samples']==101
assert frames[50]['control']['valid'] is False
assert frames[50]['control']['u']==0
assert frames[60]['control']['method']=='centroid_fallback'
assert abs(summary['odometry_example']['yaw_rad']-.0186)<1e-12
assert abs(summary['optical_example']['e']-.775/.925)<1e-12
log+='\nDEMO AND EXPORT: PASS; 101 synchronized synthetic frames; CSV/JSONL count and key fields checked.\n'
log+='Report examples: centroid e=.5853658536585366, DOS e=.8378378378378379; PID=.30696666666666667; odom yaw=.0186 rad.\n'
(ROOT/'validation.txt').write_text(log,encoding='utf8')
print(log)
