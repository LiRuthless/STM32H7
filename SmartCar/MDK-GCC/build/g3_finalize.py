exec(open(__file__.replace('g3_finalize.py','g3_refactor.py'),encoding='utf-8').read().split('C=rd(')[0])
for rel in ['control/tracking','control/control_profile','control/pid','control/direction_controller','control/angle_controller','control/speed_controller','control/legacy_control','control/legacy_pid','sensing/legacy_imu','control/legacy_element','control/legacy_roundabout']:
    hp=A/(rel+'.h');s=rd(hp).replace('\n 完整契约见','\n *     完整契约见')
    if '/legacy_' in rel:
        for old in ['ELEMENT','ROUNDABOUT']:
            s=s.replace('SMARTCAR_APP_'+old+'_H','SMARTCAR_APP_CONTROL_LEGACY_'+old+'_H')
    lines=[]
    for line in s.splitlines():
        if len(line)>100 and line.startswith(' * '):
            while len(line)>100:
                i=line.rfind(' ',5,95)
                if i<10:i=90
                lines.append(line[:i]);line=' *     '+line[i:].lstrip()
        lines.append(line)
    wr(hp,'\n'.join(lines)+'\n')
p=A/'app.c';wr(p,rd(p).rstrip()+'\n')
import shutil
b=R/'MDK-GCC/build'
for p in b.glob('phase17-g2-*.log'):
    # Only outputs from the G3 executions enumerated here are promoted.
    if p.name.removeprefix('phase17-g2-') in [n+'.log' for n in ['lcd0','lcd1','control_equivalence','runtime_tuning','app_layout_check','g2_system','g3_control','datalog_v2','scb_protocol','scb_link','spi_wifi_state','spi_log_export']]:
        shutil.copyfile(p,b/p.name.replace('phase17-g2-','phase17-g3-'))
