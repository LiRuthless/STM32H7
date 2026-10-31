exec(open(__file__.replace('g3_tests_adapt.py','g3_refactor.py'),encoding='utf-8').read().split('C=rd(')[0])
tests=R/'tests'
# Production filenames are expanded only on the current side; historical git paths remain literal.
def expand(seq):
    result=[]
    mp={'control':['tracking','control_profile','legacy_control'],'element':['legacy_element'],'roundabout':['legacy_roundabout'],'pid':['pid','direction_controller','angle_controller','speed_controller','legacy_pid'],'imu_proc':['imu_proc','legacy_imu']}
    for n in seq:result.extend(mp.get(n,[n]))
    return result
for p in tests.glob('*.py'):
    if p.name in ['app_fixture.py','control_equivalence.py','dual_mode_tracking.py','app_layout_check.py']:continue
    s=rd(p)
    # Only lists of source module basenames, never git show/historical source paths.
    def lists(m):
        raw=m.group();names=re.findall(r"['\"]([a-z_]+)['\"]",raw)
        if 'pid' not in names:return raw
        return '('+', '.join(repr(n) for n in expand(names))+')' if raw[0]=='(' else '['+', '.join(repr(n) for n in expand(names))+']'
    s=re.sub(r"[\[(](?:['\"][a-z_]+['\"]\s*,?\s*){4,}[\])]",lists,s)
    s=s.replace('#include "control.h"','#include "tracking.h"').replace('#include "roundabout.h"','#include "legacy_roundabout.h"').replace('#include "element.h"','#include "legacy_element.h"')
    wr(p,s)
p=tests/'control_equivalence.py';s=rd(p)
s=s.replace('    return C_TOKENS.sub(replace, source)','''    source = C_TOKENS.sub(replace, source)
    source = source.replace('#include "control.h"', '#include "tracking.h"').replace('#include "roundabout.h"', '#include "legacy_roundabout.h"').replace('#include "element.h"', '#include "legacy_element.h"')
    source = source.replace('#include "pid.h"', '#include "pid.h"\\n#include "legacy_pid.h"\\n#include "legacy_control.h"')
    source = source.replace('PID_UpdateTrack()', 'PID_UpdateTrack(Track_GetState()->error, IMU_GetState()->accel[0])')
    for side in ('Left', 'Right'):
        source = source.replace('PID_UpdateSpeed'+side+'()', 'PID_UpdateSpeed'+side+'(Motor_GetState()->'+side.lower()+'.target_speed, Motor_GetState()->'+side.lower()+'.real_speed)')
    for symbol, name in [('g_control_kernel_state','KernelState'),('g_control_track_output','TrackOutput'),('g_control_cask_flag','CaskFlag'),('g_roundabout_state','RoundaboutState')]:
        source = re.sub(r'\\b'+symbol+r'\\s*=\\s*([^;]+);', lambda m:'Tracking_Set'+name+'('+m.group(1)+');', source)
        source = re.sub(r'\\b'+symbol+r'\\b', 'Tracking_Get'+name+'()', source)
    return source''')
s=s.replace('"control.h", "element.h",','"tracking.h", "legacy_element.h",').replace('"roundabout.h", "motor.h"','"legacy_roundabout.h", "motor.h"')
s=s.replace('new = legacy_names((APP / filename).read_text(encoding="utf-8"))','new = legacy_names((APP / "control_profile.c").read_text(encoding="utf-8"))\n        if filename == "roundabout.c":\n            new += legacy_names((APP / "legacy_roundabout.c").read_text(encoding="utf-8"))')
s=s.replace('str(source_dir / "motor.c"),','str(source_dir / "motor.c"),\n                   *([str(APP / (name+".c")) for name in ("direction_controller","angle_controller","speed_controller","legacy_pid","legacy_imu")] if label=="current" else []),')
wr(p,s)
p=tests/'dual_mode_tracking.py';s=rd(p)
for old,new in [('control','tracking'),('roundabout','legacy_roundabout'),('element','legacy_element')]:
    # Includes inside current_names() are already converted, but static preinclude uses current headers.
    pass
s=s.replace("sources += [(APP if current else old)/name for name in ('control.c','element.c','roundabout.c')]", "sources += [APP/(name+'.c') for name in ('direction_controller','angle_controller','speed_controller','legacy_pid','legacy_imu')]\n        sources += [APP/(name+'.c') for name in ('tracking','control_profile','legacy_control','legacy_element','legacy_roundabout')] if current else [old/name for name in ('control.c','element.c','roundabout.c')]")
for start in ["files=['pid','track_sensor'","files=('lifecycle'"]:
    pos=s.index(start);end=s.index('\n',pos);raw=s[pos:end];seq=re.findall("'([a-z_]+)'",raw);new=expand(seq)
    s=s[:pos]+('files='+repr(new) if '[' in raw else 'files='+repr(tuple(new)))+s[end:]
wr(p,s)
p=tests/'app_layout_check.py';s=rd(p).replace('    assert historical_names <= {path.name for path in paths}, "original module names preserved through G2"','    removed={"control.c","control.h","element.c","element.h","roundabout.c","roundabout.h"}\n    assert historical_names - removed <= {path.name for path in paths}, "all non-split modules retained"').replace('    assert set(mapping.values()) <= {path.relative_to(APP_ROOT).as_posix() for path in paths}','    assert {v for k,v in mapping.items() if k not in removed} <= {path.relative_to(APP_ROOT).as_posix() for path in paths}')
wr(p,s)
