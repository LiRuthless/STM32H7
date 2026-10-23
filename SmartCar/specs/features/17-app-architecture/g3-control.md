# G3 控制器、控制协调及旧入口隔离实施契约

日期：2026-10-23。对应 FR-06/10/11/12、V-03/06/07/08。适用基线为 G1/G2
完成后的同一算法；历史数值对照仍用现有测试指定的提交，不把本轮实现改作历史基线。
本文件须在 G3 实现前作纯规约提交。用户已授权完整五大类架构与休眠入口隔离；隔离不构成启用、
删除、修正或重新标定旧算法的授权。本组无未决设计项。

## G3-01 文件及逐函数归属

所有下列文件直接位于 App 的大类目录，不建更深目录。拆分后的接口头是实际声明，旧文件头不以
宏维持旧路径。调用方同步 include；历史文档保留原名并追加本表定位。

| 原实现及符号 | 目标 C/H | 决定 |
|---|---|---|
| control.c::Tracking_GetState/TrySetMode/SetGear/CompleteControl/Stopped/AddProtection/Observe/Execute/AccumulateMotion | control/tracking.c/h | 签名、返回和副作用顺序不变 |
| control.c::Control_ResetRunState | control/tracking.c/h | 保留函数名及 void(void) 契约 |
| control.c::Control_ApplyProfile/GetKernelProfile/MergeProfile | control/control_profile.c/h | 保留函数名及签名 |
| roundabout.c::Roundabout_GetProfile | control/control_profile.c/h | 保留函数名及签名；活动 FSM 不再包含旧环岛头 |
| control.c::Control_UpdateLegacy | control/legacy_control.c/h | 保留 void(void)，不接入活动调度 |
| pid.c::PID_GetState/SetGain/SetTrackGains/SetAngleGains/ResetSpeed/ResetAll | control/pid.c/h | 保留小型参数、输出及复位协调 façade；不保留算法公式或硬件依赖 |
| pid.c::PID_DirectionStep | control/direction_controller.c/h | 保留原函数名、四个形参及返回；唯一公式实现，无模块静态历史 |
| pid.c::PID_UpdateAngle | control/legacy_control.c/h | 保留 void 执行包装和原 Motor_DistributeSpeed 调用边；显式输入及纯计算接口见下文 |
| pid.c::PID_UpdateSpeed/UpdateSpeedLeft/UpdateSpeedRight | control/speed_controller.c/h + pid.c/h | 单轮公式及左右历史归 speed_controller；显式输入 façade 保留公共函数名前缀 |
| pid.c::PID_UpdateTrack/UpdateIncrementalLeft/UpdateIncrementalRight | control/legacy_pid.c/h | 显式输入；旧共享方向历史和两轮增量式局部历史随真实算法移动 |
| element.c 全部 8 个 Element_* 函数 | control/legacy_element.c/h | 名称、签名、内部调用边与空壳保留 |
| roundabout.c 除 GetProfile 外全部函数 | control/legacy_roundabout.c/h | 名称、签名、内部调用边与空壳保留 |
| app.c/app.h::App_AbsFloat | legacy_roundabout.c::Roundabout_AbsFloat | 改为 static float(float)，删除 App 公共声明及跨模块 extern |
| imu_proc.c::IMU_UpdateAccelVelocity/IMU_CalibrateAccel 算法体 | sensing/legacy_imu.c/h | 同名公共入口仍由 imu_proc.c 提供，转发同一 owner 对象，详见 G3-05 |
| imu_proc.c 其余函数 | sensing/imu_proc.c/h | 所有权和签名不变，活动积分和陀螺校准不移动 |

删除已空的 control/control.c/h、element.c/h、roundabout.c/h；保留 pid.c/h 的原因是已有参数、
HMI、日志调用依赖统一的 pid_state_t 只读视图。该 façade 只协调增益、输出和 reset，不能成为
把旧传感器或电机依赖藏起来的适配层。GCC 自动源扫描和 Keil 源清单同步，每源只编译一次。

## G3-02 配置、协调和监视状态

control_profile.h 接管 control_profile_t、CONTROL_PROFILE_*、CONTROL_KERNEL_* 以及
ROUNDABOUT_* 状态常量；值、位宽和布局保持。pid.h 继续声明原 pid_* 类型、增益枚举及限幅宏；
direction_controller.h 引用该类型，不引用 tracking/FSM。tracking.h 接管 tracking_* 类型与保护位。
互相依赖只通过需要的类型头，不让 FSM 包含旧入口头。

control_profile.c 独占原 s_kernel_profiles[7]、s_round_profiles[4] 和环岛配置参数。
Control_GetKernelProfile 对 state>=7 返回 TRACKING 项；Roundabout_GetProfile 仍只对
LPREENTER、TURN_LEFT、IN、OUT 选择原四项，其余返回 TURN 的 fields=0 项。
不添加 RPREENTER 或 TURN_RIGHT 配置。全部表项数值逐 token 保持，包括内环不写 weights、
左环主配置不写 base、TURN 空配置。ApplyProfile 的四个独立 if 顺序为 weights/base/track/angle；
MergeProfile 同序逐组拷贝后 fields 按位或；未置位组维持上一拍，不改成整表赋值。

原 g_roundabout_enter_distance1/out_distance1 移成 profile 私有 uint16_t，初值 8000/10000。
接口 `uint16_t Control_GetRoundaboutEnterDistance(void)`、`Control_GetRoundaboutOutDistance(void)`
仅返回同一对象值，电磁 FSM 和旧环岛在原比较表达式位置调用；不增加运行态 setter。
原 enter_angle1/enter_angle2/out_angle1 的 int16_t 30/60/200 继续作为 legacy_roundabout
私有保留状态，不因未使用删除或更改类型。原 sign_round 为该文件 private int8_t，初值及 reset=1。
这些角度、sign_round 与 left_flag/right_flag 同属一个 private static
`legacy_roundabout_state_t s_roundabout` 对象的字段；不定义无读取的独立 static 标量造成新警告。
该结构不是协议或持久化格式，字段类型及原赋值点保持，不向其他模块暴露可写地址。

tracking.c 独占 s_tracking（上电 mode=OPTICAL，其余零）及原控制监视量：

| 原对象 | 唯一私有 owner / 入口 | 更新时机 |
|---|---|---|
| g_control_track_output | tracking.c::s_track_output；Tracking_GetTrackOutput / Tracking_SetTrackOutput | 前台预览、选中控制执行、旧入口写；类型 int16_t |
| g_control_kernel_state | tracking.c::s_kernel_state；Tracking_GetKernelState / Tracking_SetKernelState | Observe 发布电磁主状态；旧入口原赋值；reset=TRACKING |
| g_control_cask_flag | tracking.c::s_cask_flag；Tracking_GetCaskFlag / Tracking_SetCaskFlag | 旧判断原赋值；reset=0 |
| g_roundabout_state | tracking.c::s_roundabout_state；Tracking_GetRoundaboutState / Tracking_SetRoundaboutState | Observe 发布电磁阶段；旧判断及 Roundabout_ResetRunState 原赋值 |
| g_roundabout_left_flag/right_flag | legacy_roundabout.c 私有 uint8_t | 原旧判断及 reset，初值零 |
| g_element_enter_tick | legacy_element.c 私有 int16_t | 原跷跷板进入时赋值，不增加 reset，保持截断缺陷 |

上述 getter/setter 均为标量类型（后 3 对 uint8_t），只读/只写对应字段，不归一化、不检查范围、
不复位别的状态、不增加临界区。SetTrackOutput 只改原监视量，不隐式同步 s_tracking.track_out：
后者原本仅在执行口或停车/起跑发布，前台预览不能改变完整控制拍快照。此处两个量是原有不同
生命周期的字段，不能合并。所有原赋值点替换成 setter；所有读点（包括 dashboard）替换 getter。
活动 FSM 内部 s_em 与 s_optical 状态仍是各自唯一 owner，不借这些监视量推进判断。

Control_ResetRunState 依次清 kernel/cask/output、s_tracking.active_error/track_out/sequence/
protection_flags，随后电磁 reset、光电 reset；不改变 mode/gear，不额外清 roundabout 旧标志。
Roundabout_ResetRunState 的 state/left/right/sign 写入顺序独立保留。App 原起跑复位仍按原顺序
调用两者，禁止合并。Tracking_AddProtection 的原 PRIMASK 临界区完整保留。

Tracking_Observe 保持 Select EM→Select optical→Tuning_Select/Get→Calculate EM→Calculate
optical→Track_PublishObservation→发布监视量的顺序；不能为拆文件多计算一次方向。
Tracking_Execute 光电非 TRACKING 提前返回；电磁 ApplyProfile→分配 steering→按请求清里程→
按请求清 IMU X→发布 output 的顺序不变，steering 与最终 output 不能替换为一个值。
故障、模式校验、参数保存失败打印、成功 reset 顺序、uint32_t 拍序号回绕保持现状。
Control_UpdateLegacy 通过这些接口更新状态，并用 G2 Lifecycle_MarkRunning 替代原 run_flag=1；
采样、累计全保护位、首因优先级和提前返回仍同原函数。

## G3-03 控制算法显式输入和唯一历史

下列签名为本组唯一允许的迁移；单位沿用当前算法，非空指针由内部调用方保证。不新增空指针
容错、NaN 处理、参数限幅、状态机分支或失败码。无硬件动作的控制器算法不依赖 bsp.h、传感器、
motor、App 调度、服务或 HMI；只能依赖标准 C 与控制类型。

| 接口 | 输入、输出和调用迁移 |
|---|---|
| `int16_t PID_DirectionStep(int16_t error, float accel, const pid_track_gains_t *gains, int32_t *history)` | 原样保留；EM、optical 分别传自己的历史；legacy_pid 传旧共享历史 |
| `int16_t PID_UpdateTrack(int16_t error, float accel)` | legacy_pid 从 PID_GetState 读取四增益，调用 DirectionStep；前台预览及旧 Roundabout_Update 在原位置传 Track_GetState()->error 和 IMU_GetState()->accel[0] |
| `float PID_CalculateAngle(int16_t target_angle, float angle, float gyro)` | 新增纯计算 façade，传 angle 增益及 &s_pid.angle_err/out 到 AngleController_Update，返回原始正负语义的 angle_out，不执行电机动作 |
| `void PID_UpdateAngle(int16_t target_angle, float angle, float gyro)` | legacy_control 中的旧执行包装，函数体调用一次 `Motor_DistributeSpeed(-PID_CalculateAngle(target_angle, angle, gyro))`；计算的原状态更新全部完成后才发生电机调用，保留负号及形参 int16_t 的原隐式转换；没有活动调用，不新增接入 |
| `int16_t PID_UpdateSpeedLeft(int16_t target, int16_t measured)` / `Right` | motor.c 在原左右调用顺序传对应 target_speed、real_speed；不在 PID 内读取 Motor；输出仍 s_pid.out_l/r |
| `int16_t PID_UpdateIncrementalLeft(int16_t target, int16_t measured)` / `Right` | legacy_pid 从 PID_GetState 取速度增益，原增量式公式只把 Motor 读取替换为形参；当前无活动调用，不接入 Motor_UpdateControl |

PID_UpdateTrack 的名字属于旧共享历史契约，但前台停车预览仍活动调用，不能把移动到 legacy_pid
解释为禁用它；这是原有前台→legacy_pid 的明确活动调用例外，不能把“无新增 legacy 调用边”
误写为“所有 legacy 模块无活动调用”。G2 前台原赋值替换为
`Tracking_SetTrackOutput(PID_UpdateTrack(Track_GetState()->error, IMU_GetState()->accel[0]))`，
执行位置保持在 Track_UpdateError 后、电池采样前。旧环岛同样在原位置更新监视量再分配速度。

PID_DirectionStep 保留原 p/p2/d/d2 四项、abs 的整数输入、history 保存点及浮点求和顺序；
总和先转换 int16_t 后才限幅 ±1000。不得顺手修复大浮点转窄整数的问题或改成先限幅。

angle_controller.c 独占原 s_angle_err_last。接口确定为
`void AngleController_Update(int16_t target, float angle, float gyro,
const pid_angle_gains_t *gains, float *error, float *output)`，按原顺序写 error、计算
kp*error + kd*(error-last) - kg*gyro、保存 last。error/output 指向 pid.c 私有 s_pid 对应字段，
不另存一份输出；负号与向 Motor 整数参数转换仍由 legacy_control 中 PID_UpdateAngle 执行，
不能只返回数值而删除旧电机副作用。新增
`void AngleController_Reset(void)` 只清该历史；PID_ResetAll 继续随后清 angle_err/angle_out。

speed_controller.c 独占 s_speed_left/right（integral float、last_target int16_t）；单轮私有函数
`SpeedController_Update` 是原 PID_UpdateSpeed 的公式，增加显式 float kp、float ki 输入。
两个公共内部入口为 `int16_t SpeedController_UpdateLeft(int16_t target, int16_t measured,
float kp, float ki, float *output)` 和 Right；死区分别使用原宏 500。PID façade 传自身增益和
&s_pid.out_l/r，不复制或缓存输出。单轮过程仍为跨零换向清积分→保存目标→误差/比例/积分→
积分 ±(7500-dead_zone) 限幅→p+integral+d(0)→按目标正负加减死区→±7500 限幅→
float 转 int32_t 再返回 int16_t。target=0 不新增积分清零或死区，kd_v 仍不用于活动 PI。

`void SpeedController_Reset(void)` 按原顺序清左/右 integral，再清左/右 last_target。
PID_ResetSpeed 调用它后清 s_pid.out_l/out_r。PID_ResetAll 顺序为
`LegacyPID_ResetTrackHistory()`（只清旧共享 int32_t 方向历史）、AngleController_Reset、
清 s_pid.angle_err/out、PID_ResetSpeed。EM/光电影子历史仍由各自 reset 清除。

legacy_pid.c 两个增量式函数继续各自持有原全部 static p/i/d/sum/current_error/last_error；
微分占位为零，不恢复注释公式；ResetSpeed/All 不清这些量。共享方向历史 s_track_error_last
从 pid.c 移到本文件，初值零且仅 ResetAll 中上述内部接口清零。没有历史复制。
pid.c::s_pid 唯一持有原所有增益、angle_err/out、out_l/r，GetState 返回此静态只读指针；
SetGain 未知 id 无操作，所有增益设置不清历史，初值和设置次序保持。

## G3-04 旧元素/环岛隔离

legacy_element.c 保留 Element_Judge、JudgeStraight、JudgeCrossroads、JudgeCrossroadsExit、
JudgeTeeterboard、JudgeTeeterboardExit、JudgeCask、JudgeCaskExit。原共享控制字段改为
tracking setter，控制拍读取用 G2 Lifecycle_GetControlTicks，进入时仍隐式窄化到 int16_t。
跷跷板退出两个 if 独立保留，信号 >1000 与计时差 >300 的先后顺序不变。
所有阈值、条件、空壳和汇总四条调用边保持，不接入任何当前 FSM。

legacy_roundabout.c 保留 Roundabout_Update、JudgePreentryLeft/Right/Exit、JudgeEntryLeft/
Right、JudgeAhead、JudgeEntered、JudgeEnteredAgain、JudgePreExit、JudgeExit、JudgeExited、
Judge、ResetRunState。原共享 kernel/cask/roundabout/output 用 tracking 标量访问器，原测距读取
用 G2 Lifecycle_GetDistanceMm，原距离阈值用 profile getter。原 Motor/IMU 执行动作继续存在于
这些旧入口；活动 FSM 不因此调用它们。Roundabout_Update 通过 Roundabout_GetProfile 取同一张
profile 表，不另建旧参数副本；普通未知 state 的 switch 仍无动作。

Roundabout_AbsFloat 原分支 `if (a < 0) a = -a; return a;` 不改为 fabsf，保留负零/NaN 的原结果。
只由 JudgeEnteredAgain 调用。所有空壳继续空操作；不新增左环、跷跷板、路障及角度环调度。

## G3-05 休眠 IMU 使用同一 owner

imu_proc.c 保留 s_imu、s_angle_delta_x、两个 HPF、全部 accel/gyro offsets、全部 accel/gyro
last 历史，初值、声明类型和活动 reset 顺序不变。禁止 legacy_imu 另建 s_imu 或返回可写 owner。
为把三个独立 scalar 的地址传递到算法，使用栈上指针数组，不能把它们强制当成连续数组。

新增内部接口：

- `void LegacyIMU_UpdateAccelVelocity(imu_state_t *state, const float offsets[3],
  float *const history[3])`：state 指向 s_imu；offsets 是三偏置值的本次栈副本；history 的每项
  分别指向 s_accel_last_x/y/z，非空且互不重叠。原 IMU_UpdateAccelVelocity 入口只组参转发。
- `void LegacyIMU_CalibrateAccel(imu_state_t *state, float *const offsets[3])`：偏置指针分别指向
  owner 的 s_accel_offset_x/y/z；原 IMU_CalibrateAccel 入口只组参转发。

legacy_imu.c 只含这两个真实算法体，无模块 static 状态。直接读取均仍经 bsp.h：保留读取 Gyro
而非 Acc 的旧写法，raw_accel 减偏置除 IMU_GYRO_RAW_TO_DPS、gyro 加 accel_last 积分、
最后以 accel 保存历史的全部顺序。CalibrateAccel 保留 int8_t samples=100、int32_t temp[3]、
100 次 GetAcc、三偏置顺序及 velocity 的原逗号清零表达式。不修正已知公式疑点。
只有原两个 owner 入口可传入这些可写指针；公共 IMU_GetState 仍 const。全部 reset 从 owner
直接清同一历史，校准结果跨起跑保留。IMU_UpdateGyroAngle/CalibrateGyro/Init 的处理、SPI
读数来源、Y 轴高通、0.001f 角度积分系数和滤波初始化不变；休眠入口不新增调用。

## G3-06 依赖、工程与验证门禁

本组调用更新范围包括 system/scheduler/lifecycle（前台预览与原 reset）、control/motor/
electromagnetic_fsm/optical_fsm/tuning、services/param/datalog/wireless、hmi/menu/dashboard
及相关宿主夹具的声明/源清单/接口调用。未改变行为的纯 include 迁移不改变这些模块职责。
控制核心及 FSM 不依赖 legacy_element/legacy_roundabout；活动环岛常量、profile、距离门限全部
来自 control_profile。legacy_* 的存在不增加 ISR 调用边。
legacy_control 是旧执行编排，允许保留对 Motor、传感器和生命周期接口的原依赖；纯算法边界
只约束 direction/angle/speed/legacy_pid/pid，不把执行包装挪入纯算法模块。

| 验证编号 | 对应需求 | 唯一通过标准 |
|---|---|---|
| G3-V01 | G3-01/02/04 | 每个上表函数和对象可定位；无 writable extern control/roundabout/element 状态；旧入口在活动链的调用数不增加 |
| G3-V02 | G3-03 | direction/angle/speed/legacy_pid/pid 不包含或调用 sensor/motor/BSP/App；每个历史仅一处定义，s_pid 输出无第二份存储 |
| G3-V03 | G3-02/03/04 | control_equivalence.py 保留历史基线、全部输入与输出比较，逐拍输出/状态及 %a 浮点比较无差异；既有 legacy 路径也比较 |
| G3-V04 | G3-02/03 | dual_mode_tracking.py 原断言、runtime_tuning.py 原完整拍生效/历史断言保持通过；外部历史样本缺失单独记录阻塞 |
| G3-V05 | G3-05 | active IMU 原轨迹逐拍一致；对同一 owner 人工调用旧加速度更新/校准后 reset，偏置保留、全部原积分历史清零，未新增活动调用 |
| G3-V06 | G3-01～05 | LCD=0/1 构建均 0 警告 0 错误；Keil 源清单无缺失/重复；R08 逐函数证据和依赖摘要更新通过 |

现有测试白盒依赖平铺 control.c/pid.c/element.c/roundabout.c、静态符号及无参 PID 调用。
当前侧夹具必须更新真实拆分源清单；当前调参与状态读写分别映射到 owner 接口；历史侧 git show
路径、历史源码和期望输出不改。当前侧显式参数适配只传原传感器/目标反馈值，不重写公式。
夹具原监视量读写映射到 tracking getter/setter，方向历史/reset 用公共 reset 触发；不得为测试在
产品头导出可写对象，不得通过复制当前算法生成历史期望，不删比较或改容差以适配失败。
angle 原电机副作用由当前测试直接调用 legacy_control 的 PID_UpdateAngle（传原 target/angle/gyro）
触发，记录 Motor_DistributeSpeed 调用次数、参数及调用时 angle_err/out/history 的更新结果；与历史
void 入口调用对比，不在测试中手动模拟被删除的产品副作用。PID_CalculateAngle 单独验证只更新
原状态、不执行电机动作，不能用它替代旧执行入口的行为等价验收。
新增接口边界测试只覆盖左右独立、模式独立、reset 不清休眠增量历史、换向/零目标和 profile
未置位字段保留等真实契约，不为可逆路径移动添加镜像实现测试。

同步 Phase 03/04/05/12/13/14 现行引用、休眠清单、README 和注释证据；不改写历史验收。
实际执行结果回填本阶段 validation.md；AC5、700/900μs 实测和实车未执行则保持待验。
任一算法等价失败即停止 G3 实现提交，不改算法来迎合拆分；回退必须覆盖本组全部源、工程、
调用方和夹具，不触碰 Flash 布局、控制周期、优先级或其他目录。
