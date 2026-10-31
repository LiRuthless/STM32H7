exec(open(__file__.replace('g3_polish.py','g3_refactor.py'),encoding='utf-8').read().split('C=rd(')[0])
for n in ['electromagnetic_fsm','optical_fsm']:
    p=A/f'control/{n}.c';s=rd(p);s=s.replace('#include "control/'+n+'.h"','#include "control/'+n+'.h"\n#include "control/direction_controller.h"');wr(p,s)
p=R/'MDK-ARM/SmartCar.uvprojx';s=rd(p)
for a,b in [('control','tracking'),('element','legacy_element'),('roundabout','legacy_roundabout')]:s=s.replace('App/control/'+a+'.c','App/control/'+b+'.c')
wr(p,s)
p=A/'app.h';s=rd(p);start=s.index('/**\n * @brief 按原分支求');end=s.index('float App_AbsFloat(float a);',start)+len('float App_AbsFloat(float a);');wr(p,s[:start]+s[end:])
p=A/'control/angle_controller.c';s=rd(p).replace('保存误差历史后，仅写输出指针；本函数没有方向限幅。','依次写误差、写输出、保存本拍误差历史；本函数没有方向限幅。');wr(p,s)
p=A/'sensing/legacy_imu.c';s=rd(p).replace(' * @brief 执行旧加速度处理。\n * @details 保留原读数来源、公式和写入顺序，不修正休眠算法。',' * @brief 用旧公式更新加速度与速度积分。\n * @details 先读 Gyro 到 raw_gyro，仍用 raw_accel 减偏置除 14.3 得到 accel；\n *     速度累加 (gyro+前拍 accel)*0.001，最后以本拍 accel 更新历史。\n *     读数来源和积分输入不一致属于保留的旧算法疑点，本次不修正。',1)
s=s.replace(' * @brief 执行旧加速度处理。\n * @details 保留原读数来源、公式和写入顺序，不修正休眠算法。',' * @brief 采一百次加速度，写三轴偏置并清速度。\n * @details 以 int8_t 计数逐次 GetAcc 累加到 int32_t，再按 X/Y/Z 顺序求均值；\n *     最后清三轴 velocity，不清积分历史。当前入口不参与活动校准流程。',1);wr(p,s)
p=A/'control/legacy_roundabout.c';s=rd(p).replace('App 环岛配置与旧入口；配置供电磁影子机使用，旧判断函数当前不接入。','旧环岛判断、执行及复位入口，活动双模式调度不接入。').replace('依据电感、角度与里程推进左右环岛阶段，并按阶段部分覆写控制剖面。','按旧电感、角度及里程条件推进状态，通过只读 profile 接口取配置；\n *     私有旧标志由本模块维护，活动配置和距离门限由 control_profile 唯一持有。');wr(p,s)
p=A/'control/legacy_pid.c';s=rd(p).replace('保留旧入口和调用边，当前观察器使用各自独立历史。','停车前台预览仍调用本入口，旧环岛保留原调用；活动双模式观察器使用各自独立历史。');wr(p,s)
for p in A.rglob('*'):
    if p.suffix in ['.c','.h']:
        s=rd(p);s=re.sub(r'\n{3,}','\n\n',s).replace('}\n/**','}\n\n/**');wr(p,s)
