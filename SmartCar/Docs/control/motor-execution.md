# 差速、反馈与电机执行

现行说明核对基线：`8d5ab2faf68432ae03b2c1f91bc4593c7dc7cc3e`（2026-11-07）。本次仅源码对照，不新增固件或实机验收。

<a id="f08-motor"></a>
## F08 执行协调

Motor 拥有 s_motor、左右独立低通，控制拍更新；前台参数应用与停车只能走原门禁。Motor_Init 初始化PWM为0并将左右方向初值均置高，不运行PI。Motor_InitEncoder 初始化编码器并清两低通历史，系数取 s_motor.alpha。

每2ms Motor_ReadEncoder 先取清采样器左右累计计数，再分别算 y=0.88·input+0.12·last_output，先存float历史，再窄化 int16 为 real_speed。分别累加整数反馈至左右距离，平均距离=(left_distance+right_distance)/2，整数除法。里程因此是滤波后脉冲累计，不能冒充原始编码器累计或毫米。

Motor_DistributeSpeed 将基础速度 b 与方向 d 合成：d≥0时 L=b−(3d)/2、R=b+d；d<0时 L=b−d、R=b+(3d)/2。3d先乘再整数除2，目标再写int16，无额外目标饱和。方向控制限值不保证任意基础速度下的目标转换安全。

Motor_UpdateControl 先左PI后右PI，再依次写左方向和占空比、右方向和占空比；左非负置高、右非负置低，负值取反后转uint32写PWM。物理通道、频率与CCR由 [BSP父入口](../platform/board-and-bsp.md#f26) 进入具体执行主题，不把业务符号直接当硬件极性验证。

<a id="parameters"></a>
## 参数、复位与错误边界

| 字段/参数 | 数值、单位/范围 | 来源、覆盖、生效与持久化 |
|---|---|---|
| alpha / float | 0.88，算法系数 | motor.c静态初始值；初始化/重跑清历史时复制到filter；无公开运行修改/Flash恢复入口 |
| fan_duty_idle / int16 | 初始1100，PWM万分比，约定0～10000 | Motor_SetFanDutyIdle只保存不钳位；生命周期原调用点才写硬件；不由该setter落盘 |
| fan_duty / int16 | 静态零初值，运行按参数应用覆盖，约定0～10000 | setter只保存；生命周期/控制原调用点写PWM；持久化归参数服务 |
| base_speed / int16 | 脉冲/2ms，静态零后由参数/状态/临时覆盖 | setter只改差速基础值，下次分配才改目标；[矩阵](profiles.md#parameters)及bank共同决定运行值 |
| target_speed/real_speed | int16，脉冲/2ms | 目标来自差速或显式setter；反馈来自2ms累计低通；每轮独立，不落盘 |
| left/right.distance、distance | 滤波脉冲累计 | 控制拍累加，ResetDistance清零；非毫米，无Flash项；底层原码读清见平台 |

Motor_SetTargets/BaseSpeed/FanDuty/FanDutyIdle 均不执行PI、不自动写PWM或保存Flash；调用顺序由生命周期/控制协调器承担。Motor_ResetRunState 清目标、反馈、距离、低通历史，不重新初始化编码器、不清PI；相邻生命周期调用另清PI。Motor_EmergencyStop 顺序为清两目标→PID_ResetSpeed→左PWM0→右PWM0，不清方向脚、反馈、距离或风扇，风扇归安全协调器。不能把任意getter解释成事务快照。

## 定位、验证与限制

主实现：[motor.c](../../App/control/motor.c) / [motor.h](../../App/control/motor.h)，协作 [控制器](controllers.md#f08)、[唯一执行](tracking-modes.md#f11)、[起停](../system/runtime-and-safety.md#f02-stop)。来源：[Phase03](../../specs/features/03-track-following/requirements.md)、[Phase12](../../specs/features/12-control-safety-closure/requirements.md)、[Phase13](../../specs/features/13-control-structure/requirements.md)、[Phase17](../../specs/features/17-app-architecture/requirements.md)。本次按源码核对差速整数顺序、反馈滤波及清零副作用；物理方向、脉冲到距离系数、实际控制效果仍待硬件验证。
