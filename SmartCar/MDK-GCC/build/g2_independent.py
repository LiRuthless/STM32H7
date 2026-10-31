import os,sys,tempfile
from pathlib import Path
r=Path(__file__).resolve().parents[2]
os.environ['PATH']='E:/APP/mingw64/bin;'+os.environ['PATH']
os.environ['TEMP']=os.environ['TMP']=str(r/'MDK-GCC/build/host-temp')
sys.path.insert(0,str(r/'tests'))
import dual_mode_tracking as dual
with tempfile.TemporaryDirectory(prefix='g2_independent_') as directory:
    work=Path(directory)
    dual.fsm_tests(work)
    dual.param_tests(work)
    dual.app_tests(work)
