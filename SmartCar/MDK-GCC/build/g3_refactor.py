from pathlib import Path
import re
R=Path(__file__).resolve().parents[2]; A=R/'App'
def rd(p): return p.read_bytes().decode('utf-8').replace('\r\n','\n')
def wr(p,s): p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(s.replace('\r\n','\n').replace('\n','\r\n').encode())
def take(s,name):
    m=re.search(r'(?m)^(?:static )?[\w *]+\b'+name+r'\([^;]*?\)\n\{',s);assert m,name
    start=s.rfind('/**',0,m.start());end=m.end();depth=1
    while depth:
        if s[end]=='{':depth+=1
        if s[end]=='}':depth-=1
        end+=1
    return s[start:end]+'\n'
def remove(s,name): return s.replace(take(s,name),'')
def file(name,brief,details,inc,body,source=''):
    provenance=''
    if source:
        provenance=' * @note 移植来源：Sirius20260718/project/code/'+source+'，STC AI8051U 省赛快照（2026-07-18）。\n * @note 源文件未单列作者及独立许可证；保留出处，关联 Phase 13/15。\n'
    return f'/**\n * @file {name}\n * @brief {brief}\n * @details {details}\n{provenance} */\n'+inc+'\n'+body
def header(path,brief,body):
    guard='SMARTCAR_APP_'+path.replace('/','_').replace('.','_').upper()
    wr(A/path,file(Path(path).name,brief,'声明对应实现的模块接口，私有历史不通过本头公开。','',f'#ifndef {guard}\n#define {guard}\n\n{body}\n#endif\n'))
def decls(s):
    out=''
    for m in re.finditer(r'(?m)^((?!static)[\w *]+\b\w+\([^;]*?\))\n\{',s):
        name=re.search(r'(\w+)\(',m.group(1)).group(1)
        out+=f'/**\n * @brief {name} 模块接口。\n * @note 完整契约见对应 C 文件::{name}。\n */\n'+m.group(1)+';\n\n'
    return out
C=rd(A/'control/control.c');H=rd(A/'control/control.h');P=rd(A/'control/pid.c');PH=rd(A/'control/pid.h');Q=rd(A/'control/roundabout.c');QH=rd(A/'control/roundabout.h')
inc=C[C.index('#include'):C.index('/* 共享监视')]
profile_names=['Control_ApplyProfile','Control_GetKernelProfile','Control_MergeProfile']
profile=C[C.index('/* 只覆盖'):C.index('/* 前台选择')]+''.join(take(C,n) for n in profile_names)
profile+=Q[Q.index('enum\n'):Q.index('/* App 每')]+Q[Q.index('/* 电磁影子'):Q.index('/**\n * @brief 读取原四类')]+take(Q,'Roundabout_GetProfile')
profile+='\n/* 配置距离为滤波编码器累计脉冲，只读访问保持原比较时点。 */\nstatic uint16_t s_enter_distance = 8000;\nstatic uint16_t s_out_distance = 10000;\n'
for name,obj in [('Enter','enter'),('Out','out')]:
    profile+=f'\n/**\n * @brief 读取环岛{name}距离配置。\n * @return 滤波编码器累计脉冲阈值。\n */\nuint16_t Control_GetRoundabout{name}Distance(void)\n{{\n    return s_{obj}_distance;\n}}\n'
wr(A/'control/control_profile.c',file('control_profile.c','控制部分覆写配置与环岛距离。','静态表供观察器只读；执行应用按权重、基础速度、方向、角度依次覆写。', '#include "control/control_profile.h"\n#include "control/motor.h"',profile,'control.c / roundabout.c'))
ph=H[H.index('#define CONTROL_KERNEL'):H.index('typedef enum\n')]
ph+=QH[QH.index('#define ROUNDABOUT_STATE'):QH.index('/* 当前赛道')]
header('control/control_profile.h','控制配置与环岛状态常量。','#include <stdint.h>\n#include "sensing/track_sensor.h"\n#include "control/pid.h"\n\n'+ph+decls(profile))
tracking=C
for n in profile_names+['Control_UpdateLegacy']:tracking=remove(tracking,n)
tracking=tracking[tracking.index('/* 前台选择'):]
tracking='/* 监视量由原发布点更新，与完整拍 track_out 生命周期不同。 */\nstatic int16_t s_track_output;\nstatic uint8_t s_kernel_state = CONTROL_KERNEL_TRACKING;\nstatic uint8_t s_cask_flag;\nstatic uint8_t s_roundabout_state = ROUNDABOUT_STATE_NORMAL;\n\n'+tracking
access=[('TrackOutput','int16_t','s_track_output'),('KernelState','uint8_t','s_kernel_state'),('CaskFlag','uint8_t','s_cask_flag'),('RoundaboutState','uint8_t','s_roundabout_state')]
for name,t,obj in access:
    tracking+=f'\n/**\n * @brief 读取{name}监视值。\n * @return 当前标量，不保证与其他字段组成原子快照。\n */\n{t} Tracking_Get{name}(void)\n{{\n    return {obj};\n}}\n\n/**\n * @brief 写入{name}监视值。\n * @param[in] value 原标量，不作范围归一化。\n * @note 只写监视字段，不修改完整拍快照或 FSM 历史。\n */\nvoid Tracking_Set{name}({t} value)\n{{\n    {obj} = value;\n}}\n'
wr(A/'control/tracking.c',file('tracking.c','双模式观察与唯一执行协调。','前台选择模式档位，TIM6 推进观察、选择输出并发布完整拍；保护累加使用原临界区。',inc,tracking,'control.c'))
th=H[H.index('typedef enum\n'):H.index('/* 方向控制输出 */')]
header('control/tracking.h','双模式状态与监视量访问。','#include "control/control_profile.h"\n\n'+th+decls(tracking))
legacy=take(C,'Control_UpdateLegacy')
legacy+='''\n/**
 * @brief 保留角度控制的电机执行包装。
 * @details 先完成误差、输出和历史写入，再将输出取负并转换为差速接口整数参数。
 * @param[in] target_angle 目标 X 角度，度。
 * @param[in] angle 当前 X 角度，度。
 * @param[in] gyro 当前 X 角速度，度/秒。
 * @note 当前活动链不调用，不新增角度环接入。
 */
void PID_UpdateAngle(int16_t target_angle, float angle, float gyro)
{
    Motor_DistributeSpeed(-PID_CalculateAngle(target_angle, angle, gyro));
}
'''
wr(A/'control/legacy_control.c',file('legacy_control.c','旧控制执行包装。','保留双传感器检查和角度差速副作用，活动调度不新增调用。',inc,legacy,'control.c / pid.c'))
header('control/legacy_control.h','旧执行包装。','#include <stdint.h>\n'+decls(legacy))
# Old element and roundabout retain formulas; only owners and configuration access change.
e=rd(A/'control/element.c').replace('int16_t g_element_enter_tick','static int16_t s_enter_tick').replace('g_element_enter_tick','s_enter_tick')
e=e.replace('extern int16_t s_enter_tick;','')
wr(A/'control/legacy_element.c',e.replace('@file element.c','@file legacy_element.c'))
eh=rd(A/'control/element.h');eh=re.sub(r'extern int16_t g_element_enter_tick;','',eh);wr(A/'control/legacy_element.h',eh.replace('element.c','legacy_element.c').replace('@file element.h','@file legacy_element.h'))
q=Q[:Q.index('enum\n')]+Q[Q.index('/**\n * @brief 执行旧距离'):]
state='''typedef struct
{
    uint8_t left_flag; /**< 旧左环标志。 */
    uint8_t right_flag; /**< 旧右环标志。 */
    int8_t sign_round; /**< 旧方向标记，初值及 reset 为 1。 */
    int16_t enter_angle1; /**< 原入环角度 30 度。 */
    int16_t enter_angle2; /**< 原入环角度 60 度。 */
    int16_t out_angle1; /**< 原出环角度 200 度。 */
} legacy_roundabout_state_t;
/* 旧判断独占，起跑复位只清标志，不清保留角度配置。 */
static legacy_roundabout_state_t s_roundabout = { 0, 0, 1, 30, 60, 200 };
'''
absfn=take(rd(A/'app.c'),'App_AbsFloat').replace('float App_AbsFloat','static float Roundabout_AbsFloat')
q=q[:q.index('/**\n * @brief 执行旧距离')]+state+absfn+q[q.index('/**\n * @brief 执行旧距离'):]
q=re.sub(r'&s_round_profiles\[ROUNDABOUT_ROUND_PROFILE_\w+\]', 'Roundabout_GetProfile(Tracking_GetRoundaboutState())',q)
for n in ('left_flag','right_flag','sign_round'):q=q.replace('g_roundabout_'+n,'s_roundabout.'+n)
q=q.replace('App_AbsFloat','Roundabout_AbsFloat')
wr(A/'control/legacy_roundabout.c',q.replace('@file roundabout.c','@file legacy_roundabout.c'))
qh=QH[:QH.index('/* 当前赛道')]+QH[QH.index('/**\n * @brief 执行旧距离'):];wr(A/'control/legacy_roundabout.h',qh.replace('roundabout.c','legacy_roundabout.c').replace('@file roundabout.h','@file legacy_roundabout.h'))
app=rd(A/'app.c');wr(A/'app.c',remove(app,'App_AbsFloat'))
apph=rd(A/'app.h');apph=re.sub(r'/\*\*\n \* @brief[^/]*?float App_AbsFloat\(float a\);\n','',apph);wr(A/'app.h',apph)
# Extract the pure PID algorithms, keeping the state facade as the only gain/output owner.
direction=take(P,'PID_DirectionStep')
wr(A/'control/direction_controller.c',file('direction_controller.c','无静态历史的方向计算。','调用方拥有历史；原四项求和先窄化，再限幅。','#include <stdlib.h>\n#include "control/direction_controller.h"',direction,'pid.c'))
header('control/direction_controller.h','显式输入方向计算。','#include "control/pid.h"\n'+decls(direction))
angle=take(P,'PID_UpdateAngle');angle=angle[:angle.index('    Motor_DistributeSpeed')]+ '}\n'
angle=angle.replace('void PID_UpdateAngle(int16_t target_angle)','void AngleController_Update(int16_t target_angle, float angle, float gyro,\n    const pid_angle_gains_t *gains, float *error, float *output)')
angle=angle.replace('s_pid.angle_err','*error').replace('s_pid.angle_out','*output').replace('IMU_GetState()->angle[0]','angle').replace('IMU_GetState()->gyro[0]','gyro')
for field,new in [('kp_a','kp'),('kd_a','kd'),('kg_a','kg')]:angle=angle.replace('s_pid.'+field,'gains->'+new)
angle=angle.replace('计算原角度 PD 并分配差速','计算角度 PD 并更新 owner 输出').replace('把浮点输出取负传给 Motor_DistributeSpeed；本函数没有方向限幅','仅写输出指针；本函数没有方向限幅')
angle=angle.replace(' * @note 保留的角度控制入口；浮点输出向差速接口整数形参转换存在范围约束。',' * @param[in] angle 当前角度，度。\n * @param[in] gyro 当前角速度，度/秒。\n * @param[in] gains 非空三项增益。\n * @param[out] error 非空 owner 误差字段。\n * @param[out] output 非空 owner 输出字段。\n * @note 本模块独占角度前拍历史，不执行电机动作。')
angle='/* 角度拍间差分历史，由计算更新、Reset 清零。 */\nstatic float s_angle_err_last = 0.0f;\n'+angle+'\n/**\n * @brief 只清角度前拍历史。\n */\nvoid AngleController_Reset(void)\n{\n    s_angle_err_last = 0.0f;\n}\n'
wr(A/'control/angle_controller.c',file('angle_controller.c','纯角度控制计算。','接收角度和角速度，按原顺序写 owner 误差输出并保存私有历史。','#include "control/angle_controller.h"',angle,'pid.c'));header('control/angle_controller.h','角度算法及历史复位。','#include "control/pid.h"\n'+decls(angle))
speed=P[P.index('typedef struct'):P.index('/* TIM6 更新')]+ '/* 左右积分与目标历史各自独立，只有原 reset 路径清零。 */\nstatic pid_speed_pi_state_t s_speed_left;\nstatic pid_speed_pi_state_t s_speed_right;\n'+take(P,'PID_UpdateSpeed')
speed=speed.replace('PID_UpdateSpeed(','SpeedController_Update(').replace('int16_t dead_zone, float *output)','int16_t dead_zone, float kp, float ki, float *output)').replace('s_pid.kp_v','kp').replace('s_pid.ki_v','ki')
speed=speed.replace(' * @param[out] output',' * @param[in] kp 速度比例增益。\n * @param[in] ki 速度积分增益。\n * @param[out] output')
for side in ['Left','Right']:
    speed+=f'\n/**\n * @brief 更新{side}轮独立 PI 历史与 owner 输出。\n * @param[in] target 目标脉冲/2 ms。\n * @param[in] measured 反馈脉冲/2 ms。\n * @param[in] kp 比例增益。\n * @param[in] ki 积分增益。\n * @param[out] output 非空 owner 输出，写入限幅结果。\n * @return 输出先转 int32_t 再转 int16_t，范围 ±7500。\n */\nint16_t SpeedController_Update{side}(int16_t target, int16_t measured,\n    float kp, float ki, float *output)\n{{\n    return SpeedController_Update(&s_speed_{side.lower()}, target, measured,\n        PID_MOTOR_DEAD_ZONE_{side.upper()}, kp, ki, output);\n}}\n'
speed+='\n/**\n * @brief 按左积分、右积分、左目标、右目标顺序清活动 PI 历史。\n */\nvoid SpeedController_Reset(void)\n{\n    s_speed_left.integral = 0.0f;\n    s_speed_right.integral = 0.0f;\n    s_speed_left.last_target = 0;\n    s_speed_right.last_target = 0;\n}\n'
wr(A/'control/speed_controller.c',file('speed_controller.c','独立双轮位置式 PI。','显式目标反馈与增益输入；换向清积分、积分限幅、死区补偿、输出限幅依次执行。','#include "control/speed_controller.h"',speed,'pid.c'));header('control/speed_controller.h','双轮 PI 与复位。','#include "control/pid.h"\n'+decls(speed))
lp='/* 旧共享方向历史，前台预览与旧环岛共用，ResetAll 单独清零。 */\nstatic int32_t s_track_error_last;\n'
lp+=take(P,'PID_UpdateTrack').replace('PID_UpdateTrack(void)','PID_UpdateTrack(int16_t error, float accel)').replace('    const pid_track_gains_t gains','    const pid_state_t *pid = PID_GetState();\n    const pid_track_gains_t gains').replace('s_pid.','pid->').replace('Track_GetState()->error, IMU_GetState()->accel[0]','error, accel')
lp=lp.replace('读取电磁偏差和 IMU X 轴加速度','使用显式电磁偏差和 IMU X 轴加速度').replace(' * @return 转换有效时',' * @param[in] error 当前电磁偏差。\n * @param[in] accel 当前 IMU X 加速度原标度。\n * @return 转换有效时')
for side in ['Left','Right']:
    fn=take(P,'PID_UpdateIncremental'+side).replace('PID_UpdateIncremental'+side+'(void)','PID_UpdateIncremental'+side+'(int16_t target, int16_t measured)')
    fn=fn.replace('    static float s_p_out','    const pid_state_t *pid = PID_GetState();\n    static float s_p_out',1).replace('s_pid.','pid->')
    fn=re.sub(r'Motor_GetState\(\)->'+side.lower()+r'.target_speed\s*- Motor_GetState\(\)->'+side.lower()+r'.real_speed','target - measured',fn)
    fn=fn.replace(' * @return 原浮点累计',' * @param[in] target 目标脉冲/2 ms。\n * @param[in] measured 反馈脉冲/2 ms。\n * @return 原浮点累计')
    lp+=fn
lp+='\n/**\n * @brief 只清旧共享方向历史，不清休眠增量式局部状态。\n */\nvoid LegacyPID_ResetTrackHistory(void)\n{\n    s_track_error_last = 0;\n}\n'
wr(A/'control/legacy_pid.c',file('legacy_pid.c','旧共享方向与休眠增量式算法。','显式输入避免硬件依赖；增量式历史保留且不参与活动 reset。','#include "control/legacy_pid.h"\n#include "control/direction_controller.h"',lp,'pid.c'));header('control/legacy_pid.h','旧算法显式输入接口。','#include "control/pid.h"\n'+decls(lp))
facade='/* 增益与可见输出唯一 owner，前台设置、控制拍更新。 */\nstatic pid_state_t s_pid;\n'+''.join(take(P,n) for n in ['PID_GetState','PID_SetGain','PID_SetTrackGains','PID_SetAngleGains'])
facade+='''\n/**
 * @brief 用模块增益计算纯角度输出。
 * @param[in] target_angle 目标 X 角度，度。
 * @param[in] angle 当前 X 角度，度。
 * @param[in] gyro 当前 X 角速度，度/秒。
 * @return 原角度输出，未取负或限幅。
 * @note 只更新角度历史及 s_pid 误差输出，无电机动作。
 */
float PID_CalculateAngle(int16_t target_angle, float angle, float gyro)
{
    const pid_angle_gains_t gains = { s_pid.kp_a, s_pid.kd_a, s_pid.kg_a };
    AngleController_Update(target_angle, angle, gyro, &gains, &s_pid.angle_err, &s_pid.angle_out);
    return s_pid.angle_out;
}
'''
for side,field in [('Left','l'),('Right','r')]:
    facade+=f'\n/**\n * @brief 以模块速度增益计算{side}轮 PI。\n * @param[in] target 目标脉冲/2 ms。\n * @param[in] measured 反馈脉冲/2 ms。\n * @return 限幅的整数 PI 输出，范围 ±7500。\n * @note 双轮历史由 speed_controller 独立持有。\n */\nint16_t PID_UpdateSpeed{side}(int16_t target, int16_t measured)\n{{\n    return SpeedController_Update{side}(target, measured, s_pid.kp_v, s_pid.ki_v, &s_pid.out_{field});\n}}\n'
reset=take(P,'PID_ResetSpeed');reset=re.sub(r'    s_speed_left.integral.*?s_speed_right.last_target = 0;', '    SpeedController_Reset();',reset,flags=re.S)
allreset=take(P,'PID_ResetAll').replace('s_track_error_last = 0;','LegacyPID_ResetTrackHistory();').replace('s_angle_err_last = 0.0f;','AngleController_Reset();')
facade+=reset+allreset
wr(A/'control/pid.c',file('pid.c','控制参数和输出协调接口。','唯一持有增益和可见输出，算法及历史委托专属模块，不读取传感器或硬件。','#include "control/pid.h"\n#include "control/angle_controller.h"\n#include "control/speed_controller.h"\n#include "control/legacy_pid.h"',facade,'pid.c'))
header('control/pid.h','控制参数与只读输出。',PH[PH.index('#include <stdint.h>'):PH.index('/**\n * @brief 读取模块')]+decls(facade))
# Legacy IMU operates on the owner's scalar addresses; no second state instance.
imu=rd(A/'sensing/imu_proc.c');limu=''
for name in ['IMU_UpdateAccelVelocity','IMU_CalibrateAccel']:
    fn=take(imu,name);body=fn[fn.index('{'):]
    body=body.replace('s_imu.','state->')
    for i,axis in enumerate('xyz'):
        body=body.replace('s_accel_offset_'+axis,('offsets['+str(i)+']') if name.endswith('Velocity') else ('*offsets['+str(i)+']'))
        body=body.replace('s_accel_last_'+axis,'*history['+str(i)+']')
    if name.endswith('Velocity'):
        sig='void LegacyIMU_UpdateAccelVelocity(imu_state_t *state, const float offsets[3],\n    float *const history[3])'
        wrapper='    const float offsets[3] = { s_accel_offset_x, s_accel_offset_y, s_accel_offset_z };\n    float *const history[3] = { &s_accel_last_x, &s_accel_last_y, &s_accel_last_z };\n    LegacyIMU_UpdateAccelVelocity(&s_imu, offsets, history);\n'
        params=' * @param[in] offsets 三轴偏置本次副本。\n * @param[in,out] history 三个非空指针，分别指向 owner 独立历史。\n'
    else:
        sig='void LegacyIMU_CalibrateAccel(imu_state_t *state, float *const offsets[3])'
        wrapper='    float *const offsets[3] = { &s_accel_offset_x, &s_accel_offset_y, &s_accel_offset_z };\n    LegacyIMU_CalibrateAccel(&s_imu, offsets);\n'
        params=' * @param[out] offsets 三个非空指针，依次写 owner 零偏。\n'
    limu+='/**\n * @brief 执行旧加速度处理。\n * @details 保留原读数来源、公式和写入顺序，不修正休眠算法。\n * @param[in,out] state 非空 owner 状态，更新原加速度或速度字段。\n'+params+' * @note 仅由 imu_proc 同名入口组参转发，不新增活动调用。\n */\n'+sig+'\n'+body+'\n'
    imu=imu.replace(fn,fn[:fn.index('{')+1]+'\n'+wrapper+'}\n')
imu=imu.replace('#include "sensing/filter.h"','#include "sensing/filter.h"\n#include "sensing/legacy_imu.h"');wr(A/'sensing/imu_proc.c',imu)
wr(A/'sensing/legacy_imu.c',file('legacy_imu.c','旧加速度算法，无持久状态。','owner 提供状态和独立偏置/历史地址，读取均经 BSP。','#include "bsp.h"\n#include "app_config.h"\n#include "sensing/legacy_imu.h"',limu,'gyroscope.c'));header('sensing/legacy_imu.h','仅供 IMU owner 转发的旧算法。','#include "sensing/imu_proc.h"\n'+decls(limu))
# Caller migration: assignments become scalar setters; reads become getters.
mapping={'g_control_track_output':'TrackOutput','g_control_kernel_state':'KernelState','g_control_cask_flag':'CaskFlag','g_roundabout_state':'RoundaboutState'}
for p in A.rglob('*'):
    if p.suffix not in ('.c','.h') or p.name in ('control.c','control.h','roundabout.c','roundabout.h','element.c','element.h'):continue
    s=rd(p).replace('control/control.h','control/tracking.h').replace('control/roundabout.h','control/legacy_roundabout.h').replace('control/element.h','control/legacy_element.h')
    if p.name in ['tracking.c','legacy_control.c']:
        s=s.replace('#include "control/legacy_element.h"\n','').replace('#include "control/legacy_roundabout.h"\n','')
    if p.name in ['electromagnetic_fsm.c','electromagnetic_fsm.h','optical_fsm.c','optical_fsm.h']:
        s=s.replace('control/legacy_roundabout.h','control/control_profile.h').replace('control/legacy_element.h','control/control_profile.h')
    for old,name in mapping.items():
        s=re.sub(r'\b'+old+r'\s*=\s*([^;]+);',lambda m:'Tracking_Set'+name+'('+m.group(1)+');',s)
        s=re.sub(r'\b'+old+r'\b','Tracking_Get'+name+'()',s)
    s=s.replace('g_roundabout_enter_distance1','Control_GetRoundaboutEnterDistance()').replace('g_roundabout_out_distance1','Control_GetRoundaboutOutDistance()')
    s=s.replace('PID_UpdateTrack()','PID_UpdateTrack(Track_GetState()->error, IMU_GetState()->accel[0])')
    for side in ['Left','Right']:
        s=s.replace('PID_UpdateSpeed'+side+'()',f'PID_UpdateSpeed{side}(s_motor.{side.lower()}.target_speed, s_motor.{side.lower()}.real_speed)')
    if 'PID_UpdateTrack(' in s and p.name not in ('legacy_pid.c','legacy_pid.h'):s=s.replace('#include "control/pid.h"','#include "control/pid.h"\n#include "control/legacy_pid.h"')
    if 'PID_DirectionStep(' in s and p.name not in ('direction_controller.c','direction_controller.h'):s=s.replace('#include "control/pid.h"','#include "control/pid.h"\n#include "control/direction_controller.h"')
    wr(p,s)
for name in ['control','element','roundabout']:
    for ext in ['c','h']:(A/f'control/{name}.{ext}').unlink()
# Explicit Keil source paths retain existing XML formatting.
kp=R/'MDK-ARM/SmartCar.uvprojx';k=rd(kp)
for old,new in [('control','tracking'),('element','legacy_element'),('roundabout','legacy_roundabout')]:
    k=k.replace(f'control\\{old}.c',f'control\\{new}.c').replace(f'<FileName>{old}.c</FileName>',f'<FileName>{new}.c</FileName>')
newfiles=['control_profile','direction_controller','angle_controller','speed_controller','legacy_control','legacy_pid']
entry=re.search(r'\s*<File>\s*<FileName>pid.c</FileName>.*?</File>',k,re.S).group()
for n in newfiles:k=k.replace(entry,entry+entry.replace('pid.c',n+'.c'),1)
entry=re.search(r'\s*<File>\s*<FileName>imu_proc.c</FileName>.*?</File>',k,re.S).group();k=k.replace(entry,entry+entry.replace('imu_proc.c','legacy_imu.c'));wr(kp,k)
