exec(open(__file__.replace('g3_headers.py','g3_refactor.py'),encoding='utf-8').read().split('C=rd(')[0])
for rel in ['hmi/menu.h','services/scb_protocol.h']:
    import subprocess
    (A/rel).write_bytes(subprocess.run(['git','show','HEAD:SmartCar/App/'+rel],capture_output=True,check=True).stdout)
p=A/'control/speed_controller.c';s=rd(p).replace('int16_t measured, int16_t dead_zone, float kp, float ki, float *output)','int16_t measured, int16_t dead_zone,\n    float kp, float ki, float *output)');wr(p,s)
p=A/'control/control_profile.c';s=rd(p).replace('返回电磁状态机持有的控制剖面地址','返回本模块持有的控制剖面地址').replace(' * @details 按函数体既定条件和写入顺序更新模块状态；提前返回分支不执行其后的状态发布。\n * @param[in] state ROUNDABOUT',' * @details 仅左预进、左打角、岛中及出岛四个状态选择表项；其他值返回 TURN 空配置。\n * @param[in] state ROUNDABOUT').replace('检查阶段索引后返回对应静态部分剖面；越界阶段回退到第一项','检查阶段后返回对应静态部分剖面；未知阶段返回 TURN 空配置').replace('环岛Enter','环岛入环').replace('环岛Out','环岛出环');wr(p,s)
p=A/'control/tracking.c';s=rd(p).replace('把非零输入归一为高档、零归一为低档，更新当前档位后立即重新应用该档参数；不改变当前循迹模式。','仅在 gear 小于 2 时写所选档位，其他值无操作；不应用参数或改变模式。').replace('更新 s_tracking.gear，并由参数模块重载方向与基础速度','只更新 s_tracking.gear，参数应用由原调用方协调')
for en,zh in [('TrackOutput','方向输出'),('KernelState','电磁主状态'),('CaskFlag','旧路障标志'),('RoundaboutState','环岛阶段')]:s=s.replace(en+'监视值',zh+'监视值')
wr(p,s)
# H contains the actual C summary and precise implementation location, not a template name.
for rel in ['control/tracking','control/control_profile','control/pid','control/direction_controller','control/angle_controller','control/speed_controller','control/legacy_control','control/legacy_pid','sensing/legacy_imu']:
    cp=A/(rel+'.c');hp=A/(rel+'.h');c=rd(cp);h=rd(hp)
    for m in list(re.finditer(r'/\*\*\n \* @brief (\w+) 模块接口。\n \* @note 完整契约见对应 C 文件::\w+。\n \*/',h)):
        name=m.group(1);fn=take(c,name);comment=fn[:fn.index('*/')]
        brief=re.search(r'@brief (.*)',comment).group(1)
        note=re.search(r'@note (.*?)(?=\n \* @|\n \*/|$)',comment,re.S)
        constraint=note.group(1).replace('\n *     ','') if note else ''
        h=h.replace(m.group(),f'/**\n * @brief {brief}\n * @note {constraint}完整契约见 {cp.name}::{name}。\n */')
    # Summary comments are wrapped without changing declarations.
    lines=[]
    for line in h.splitlines():
        if len(line)>100 and line.startswith(' * '):
            while len(line)>100:
                split=line.rfind(' ',5,95)
                if split<10:split=90
                lines.append(line[:split]);line=' *     '+line[split:].lstrip()
        lines.append(line)
    wr(hp,'\n'.join(lines)+'\n')
