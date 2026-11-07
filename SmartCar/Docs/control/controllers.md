# 方向、角度与速度控制器

现行说明核对基线：`8d5ab2faf68432ae03b2c1f91bc4593c7dc7cc3e`（2026-11-07）。本次仅源码对照，不新增固件或实机验收。

<a id="f08"></a>
## F08 显式输入与状态隔离

控制器把业务观测转换为控制量，不读传感器或硬件。正常 TIM6=2ms 拍调用；方向历史分别由两个 FSM 持有，角度历史归 angle_controller，左右速度积分归 speed_controller。pid.c 持有活动增益和可见输出。getter 返回静态只读对象，不提供前台跨 ISR 一致快照。上电先恢复参数再经应用协调写活动值，默认值不等于任意运行时值。

<a id="f08-direction"></a>
## 方向控制的计算与转换

输入 input 为 int16 算法偏差，accel 为 float 现有加速度标度，gains/history 均须非空。error 提升为 int32，依次计算 P=kp·e、P2=k2p·e·abs(e)、D=kd·(e−history)、D2=k2d·accel；先写 history=e，再将 P+P2+D−D2 赋给 int16 输出，最后两次比较限到 ±1000。D 是拍间差分，没有除以0.002s；ki_x 不参与该算法。活动 IMU accel[0] 当前为0，阻尼项不产生作用；保留项不表示已实现加速度闭环。

先窄化后限幅存在越界浮点转换风险 R01；不能描述成所有输入均安全饱和。方向历史不在函数内共享，电磁/光电观察互不覆盖。调用者 reset 各自历史，PID_ResetAll 不代替 FSM reset。

<a id="f08-angle"></a>
## 角度控制的写入顺序

目标 int16 角度与当前 float 角度单位为度，gyro 为度/秒。先写 owner error=target−angle，再写 output=kp·error+kd·(error−s_angle_err_last)−kg·gyro，最后更新本模块 s_angle_err_last。无输出限幅、无电机动作，也没有按秒归一化差分。AngleController_Reset 只清历史；PID_ResetAll 还清 owner 的 angle_err/angle_out。旧包装的取负、窄化和电机副作用属于 [休眠边界](elements-and-dormant-code.md#f13)，不能套到纯算法接口上。

<a id="f08-speed"></a>
## 双轮位置式 PI

目标/反馈均 int16，单位脉冲/2ms；左右分别存 integral(float) 与 last_target(int16)。同一算法按下列严格顺序执行：

1. 仅 target>0且last_target<0，或 target<0且last_target>0 时清积分。经过0再反向不满足该条件，不能泛称跨零总清零。
2. 保存 last_target=target。
3. int32 error=target−measured；P=kp·float(error)，integral+=ki·float(error)。积分没有乘控制周期。
4. 两次比较将 integral 限到 ±(7500−dead_zone)，当前两轮 dead_zone=500，因此为±7000。
5. owner 输出=P+integral+d_out，其中 d_out 初值0且无更新；kd_v 不参与活动 PI。
6. target>0加500，target<0减500，target=0不补死区；零目标仍可有误差及积分输出。
7. owner float 输出限±7500，再 float→int32→int16 返回，整数化发生在限幅后。

Motor 先左后右计算，再先左后右发布方向与绝对值占空比，见 [执行链](motor-execution.md#f08-motor)。PID_ResetSpeed 先清左右积分、左右目标历史，再清 out_l/out_r；不改增益。紧急停车必须显式清 PWM，不能靠零目标自然衰减。

<a id="parameters"></a>
## 控制器参数与覆盖关系

| 参数/类型 | 默认或固定值、单位/范围 | 来源、覆盖、生效与持久化 |
|---|---|---|
| PID_MAX_DIR_OUT / int宏 | 1000，输出算法标度，±1000 | pid.h 编译固定；每次方向计算末尾比较；无Flash项 |
| PID_MAX_SPD_OUT / int宏 | 7500，带符号PWM万分比，±7500 | pid.h 编译固定；积分/输出限幅当拍生效；不落盘 |
| PID_MOTOR_DEAD_ZONE_LEFT/RIGHT | 各500，PWM万分比 | 编译固定；非零目标补偿；派生积分范围±7000；不落盘 |
| kp_v / ki_v / float | 默认20 / 0.75，每拍标度 | parameter_store默认→有效Flash恢复→ParameterApply活动值；PID_SetGain可改且不钳位/不清历史；下次PI生效。活动值与查询镜像可能不同，不证明一致；保存关系见参数服务 |
| kp/k2p/kd/k2d / float | 当前配置值见 [状态矩阵](profiles.md#parameters)，普通电磁为2/0.008/15/1 | 电磁按状态部分覆盖，光电来自所选bank；临时事务可覆盖本拍方向值；未证明物理稳定范围，不虚构合法调参区间 |
| kp_a/kd_a/kg_a / float | 右环主态2/1.2/0，每度/每拍/度秒标度 | 状态配置只写置位组；PID_SetGain不钳位；下次角度计算使用；不把表常量视为Flash运行值 |

增益镜像、档位/模式、菜单/UART/SCB与保存入口见 [持久参数](../services/parameters-and-tuning.md#f15)，当前状态覆盖见 [F12](profiles.md#f12)。未知稳定区间和物理标定仍待实测。

## 实现与阶段依据

[direction_controller.c](../../App/control/direction_controller.c) 的 PID_DirectionStep；[angle_controller.c](../../App/control/angle_controller.c) 的 AngleController_Update/Reset；[speed_controller.c](../../App/control/speed_controller.c) 的 UpdateLeft/Right/Reset；[pid.c](../../App/control/pid.c) 的增益setter、CalculateAngle、UpdateSpeedLeft/Right、ResetSpeed/All；[pid.h](../../App/control/pid.h) 定义参数布局和限值。

来源：[Phase03](../../specs/features/03-track-following/requirements.md) FR-04/05/06/08/10、[Phase12](../../specs/features/12-control-safety-closure/requirements.md) FR-01、[Phase13](../../specs/features/13-control-structure/requirements.md) FR-02/04/05/08、[Phase17](../../specs/features/17-app-architecture/requirements.md) FR-06。本次核对公式、窄化、零目标、换向和各owner；既有control_equivalence等历史证据不消除R01或证明实车通过。
