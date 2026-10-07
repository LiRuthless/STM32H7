"""Retrieve public primary sources into temporary reading cache; record exact versions.
No paper text is bundled in the deliverables. Python standard library + pypdf.
"""
import urllib.request, json, hashlib, tempfile, concurrent.futures
from pathlib import Path
from pypdf import PdfReader
ROOT=Path(__file__).resolve().parents[1]
CACHE=Path(tempfile.gettempdir())/'ins_research_20261007'
CACHE.mkdir(exist_ok=True)
SOURCES={
 'woodman':'https://www.cl.cam.ac.uk/techreports/UCAM-CL-TR-696.pdf',
 'mahony':'https://www.mikrocontroller.net/attachment/197058/2007_Mahony.etal_TAC-06-396_v3.pdf',
 'madgwick':'https://x-io.co.uk/downloads/madgwick_internal_report.pdf',
 'sola':'https://arxiv.org/pdf/1711.02508',
 'wheelins':'https://arxiv.org/pdf/1912.07805',
 'aiimu':'https://arxiv.org/pdf/1904.06064',
 'rpp':'https://arxiv.org/pdf/2305.20026',
 'stanley':'https://robots.stanford.edu/papers/thrun.stanley05.pdf',
 'coulter':'https://publications.ri.cmu.edu/storage/publications/pub_files/pub3/coulter_r_craig_1992_1/coulter_r_craig_1992_1.pdf',
 'tedaldi':'https://albertopretto.altervista.org/papers/tpm_icra2014.pdf',
 'gyrodometry':'https://www.cs.cmu.edu/~motionplanning/papers/sbp_papers/integrated2/borenstien_gyro.pdf',
 'wheelgins':'https://arxiv.org/pdf/2501.03079',
}
REPOS={'wheelins':'i2Nav-WHU/Wheel-INS','aiimu':'mbrossar/ai-imu-dr','fusion':'xioTechnologies/Fusion','zumo':'pololu/zumo-32u4-arduino-library','qtr':'pololu/qtr-sensors-arduino','nav2':'ros-navigation/navigation2','robot_localization':'cra-ros-pkg/robot_localization'}
def get(url):
 return urllib.request.urlopen(urllib.request.Request(url,headers={'User-Agent':'Research-report/1.0'}),timeout=45).read()
def paper(item):
 name,url=item
 try:
  data=get(url); target=CACHE/(name+'.pdf'); target.write_bytes(data)
  pdf=PdfReader(target)
  (CACHE/(name+'.txt')).write_text('\n'.join(f'\n=== PDF PAGE {i+1} ===\n'+(p.extract_text() or '') for i,p in enumerate(pdf.pages)),encoding='utf-8')
  return {'id':name,'url':url,'status':'downloaded_fulltext','pdf_pages':len(pdf.pages),'sha256':hashlib.sha256(data).hexdigest()}
 except Exception as e:return {'id':name,'url':url,'status':'failed','error':str(e)}
def repo(item):
 name,path=item
 try:
  info=json.loads(get('https://api.github.com/repos/'+path)); branch=info['default_branch']
  commit=json.loads(get('https://api.github.com/repos/'+path+'/commits/'+branch)); sha=commit['sha']
  files={'zumo':['examples/MazeSolver/GridMovement.h','examples/MazeSolver/TurnSensor.h','examples/MazeSolver/MazeSolver.ino','examples/LineFollower/LineFollower.ino','examples/RotationResist/RotationResist.ino'], 'fusion':['README.md','Fusion/FusionAhrs.c'], 'aiimu':['README.md'], 'wheelins':['README.md','config/robot.yaml','dataset/ReadMe.pdf'], 'robot_localization':['doc/configuring_robot_localization.rst']} .get(name,['README.md'])
  fetched=[]
  for file in files:
   try:
    data=get(f'https://raw.githubusercontent.com/{path}/{sha}/{file}')
    dest=CACHE/(name+'_'+file.replace('/','_'));dest.write_bytes(data)
    if file.endswith('.pdf'):
     pdf=PdfReader(dest);dest.with_suffix('.txt').write_text('\n'.join(p.extract_text() or '' for p in pdf.pages),encoding='utf-8')
    fetched.append({'file':file,'sha256':hashlib.sha256(data).hexdigest()})
   except Exception as e:fetched.append({'file':file,'error':str(e)})
  return {'id':name,'repo':path,'branch':branch,'commit':sha,'commit_date':commit['commit']['committer']['date'],'files_readable':fetched}
 except Exception as e:return {'id':name,'repo':path,'error':str(e)}
if __name__=='__main__':
 with concurrent.futures.ThreadPoolExecutor(max_workers=6) as pool:
  papers=list(pool.map(paper,SOURCES.items())); repos=list(pool.map(repo,REPOS.items()))
 manifest={'retrieved_date':'2026-10-07','cache':str(CACHE),'papers':papers,'repositories':repos}
 (ROOT/'source_manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
 print(json.dumps(manifest,ensure_ascii=False,indent=2))
