# 双模式循迹与控制器

现行功能说明，核对基线 `68cb508`；2026-10-31迁移。需求经[功能索引](../traceability.md)进入本页，再由实现表定位源码。
当前行为不等于全部已批准/实机验收，冲突见[已知问题](../known-issues.md)。

<a id="f08"></a>
## F08 方向角度速度与执行
### 显式输入与算法历史

方向公式`kp*e+k2p*e*abs(e)+kd*(e-last_e)-k2d*accel_x`；两模式各传自己的方向历史。当前先转int16再限±1000，越界转换有R01风险，不能承诺所有合法调参都获得数学饱和。活动accel_x为0，因此该项当前不产生作用。
左右速度共用一个位置式PI算法但各持独立积分/上次目标，默认KP=20、KI=0.75；目标换向清积分、积分限±7000，非零目标加带符号500死区，输出限±7500；零目标不加死区。
Motor差速把基础速度与方向输出组合：内侧变化量为3/2、外侧为1，左右符号及int16截断按代码；2ms反馈取采样器累计、α=0.88低通。方向脚左正高、右正低，PWM对外0～10000，BSP才换CCR。
角度误差`e=target_angle-angle`（度），输出`kp*e+kd*(e-last_e)-kg*gyro`，gyro为度/秒；写误差→写输出→存历史，不加方向限幅。角度控制纯算法的历史由angle_controller持有，旧包装的Motor副作用只在原包装；当前方向/角度/速度算法不反读传感器或生命周期。

### 复位、边界与验证

`Motor_EmergencyStop`直接清目标/活动PI/PWM，不能等待速度环自然衰减。ResetAll不擅自清休眠增量PI历史；两模式方向历史不合并。PID左右速度薄入口实际有(target,measured)参数，旧P13“无参”不适用于当前签名。
核对输入/输出类型、转换顺序、换向、零值、饱和、左右隔离和差速；control_equivalence/g3_control为既有按需证据。宿主保持错误顺序也可能通过，不据此关闭R01。实车调参/编码器方向待验。

**需求来源：** [P03 FR-04, FR-05, FR-06, FR-08, FR-10](../../specs/features/03-track-following/requirements.md)；[P12 FR-01](../../specs/features/12-control-safety-closure/requirements.md)；[P13 FR-02, FR-04, FR-05, FR-08](../../specs/features/13-control-structure/requirements.md)；[P17 FR-06](../../specs/features/17-app-architecture/requirements.md)。实现入口：[App/control/direction_controller.c](../../App/control/direction_controller.c)、[App/control/angle_controller.c](../../App/control/angle_controller.c)、[App/control/speed_controller.c](../../App/control/speed_controller.c)、[App/control/pid.c](../../App/control/pid.c)、[App/control/motor.c](../../App/control/motor.c)。

<a id="f10"></a>
## F10 光电状态机
### 状态与输入输出

`OPTICAL_WAIT=0/TRACKING=1/LOST=2/STOPPED=3`。复位WAIT，控制计数>1000且双信号有效才TRACKING；运行灰度失线LOST并请求停车，LOST锁存到复位，其他停车转STOPPED。
仅普通方向PID，不识别光电十字/环岛。输入是归一化灰度误差与当前光电bank；输出控制意图及私有方向历史，选中模式由tracking执行。电磁处于特殊元素并不授权它控制光电输出。

### 验证

核对1000/1001、有效/失线/复位、未选中观察及输出隔离；dual独立FSM子项已有历史记录。当前标定-1下不能以正式固件直接跑该链；硬件循迹待验。

**需求来源：** [P14 FR-09](../../specs/features/14-dual-mode-tracking/requirements.md)。实现入口：[App/control/optical_fsm.c](../../App/control/optical_fsm.c)。

<a id="f11"></a>
## F11 双FSM与唯一执行
### 完整拍协调

tracking拥有完整控制拍与独立监视量；监视track_out不等于执行快照字段，不能合并。先完成双传感及保护，再同帧推进光电/电磁观察，分别用显式输入与独立方向历史计算，只选定模式提交Motor/PID及共享复位意图。
未选中电磁只能累加私有左右距离及angle_delta；选择后才可应用共享里程/角度复位。模式切换、跨模块reset和保存属于lifecycle，不属于纯setter。控制完成序号、调参提交/事件、完整日志发布按scheduler原时点顺序。

### 并发与验证

TIM6更新控制快照，TIM15同优先级不抢占后复制；const getter没有跨ISR事务保证。Tuning前台快照使用原短临界区，不扩大锁复制大缓冲。
审查唯一执行分支、观察路径无Motor/PID/IMU/PWM副作用、错误出口与记录一致性；既有真实C/人工审查组合证据不能把只比较Motor/PID内存扩大成“所有硬件调用均探测为零”。物理时序待验。

**需求来源：** [P14 FR-07, FR-11](../../specs/features/14-dual-mode-tracking/requirements.md)；[P17 FR-11, FR-12](../../specs/features/17-app-architecture/requirements.md)。实现入口：[App/control/tracking.c](../../App/control/tracking.c)、[App/system/scheduler.c](../../App/system/scheduler.c)。

<a id="f12"></a>
## F12 部分配置与全局调参规划
### 现行部分配置

control_profile有7主态、4环岛阶段静态表及8000/10000距离门限。每条有fields位，未置位字段沿用上一值，不补零、不把所有状态变成完整覆盖；先主态后子态再计算。非法主态回普通配置，未知环岛阶段回空TURN配置。数值表以源码常量为唯一实际值，本文实现表定位原表，不复制另一套待维护矩阵。

### 未实施目标

P13完成结构建表，P16完成显式普通赛道临时覆盖；不等于P10的“基准+用户调参”全局合成语义已完成。P10 FR1/2仍待用户确定，枚举化/不可达状态处置也不能借结构完成自动关闭。P09新元素必须复用已存在结构，不再以“等待P10才建表”作为现行前提。
验证按字段存在性、应用顺序及历史等价；全局策略未定，不宣称菜单任何增益运行都不会被覆写。

**需求来源：** [P10 FR-01, FR-02, FR-03, FR-04, FR-05](../../specs/features/10-control-consistency/requirements.md)；[P13 FR-06](../../specs/features/13-control-structure/requirements.md)。实现入口：[App/control/control_profile.c](../../App/control/control_profile.c)。

## 本页实现与模块反查

下表是当前源码定位；完整符号表仅在模块主功能页维护，协作行转向主条目。公共/私有辅助函数归所属功能，不把声明视为另一实现。Core生成部分只作边界，用户区按原标记维护。

| 文件 | 主功能 | 定义/配置定位 |
|---|---|---|
| [App/control/angle_controller.c](../../App/control/angle_controller.c) | F08 | `AngleController_Update`、`AngleController_Reset` |
| [App/control/angle_controller.h](../../App/control/angle_controller.h) | F08 | 类型/布局/配置；关联同模块C定义 |
| [App/control/control_profile.c](../../App/control/control_profile.c) | F12 | `Control_ApplyProfile`、`Control_GetKernelProfile`、`Control_MergeProfile`、`Roundabout_GetProfile`、`Control_GetRoundaboutEnterDistance`、`Control_GetRoundaboutOutDistance` |
| [App/control/control_profile.h](../../App/control/control_profile.h) | F12 | 类型/布局/配置；关联同模块C定义 |
| [App/control/direction_controller.c](../../App/control/direction_controller.c) | F08 | `PID_DirectionStep` |
| [App/control/direction_controller.h](../../App/control/direction_controller.h) | F08 | 类型/布局/配置；关联同模块C定义 |
| [App/control/motor.c](../../App/control/motor.c) | F08 | `Motor_GetState`、`Motor_SetTargets`、`Motor_SetBaseSpeed`、`Motor_SetFanDuty`、`Motor_SetFanDutyIdle`、`Motor_ResetDistance`、`Motor_DistributeSpeed`、`Motor_UpdateControl`、`Motor_ReadEncoder`、`Motor_Init`、`Motor_InitEncoder`、`Motor_ResetRunState`、`Motor_EmergencyStop` |
| [App/control/motor.h](../../App/control/motor.h) | F08 | 类型/布局/配置；关联同模块C定义 |
| [App/control/optical_fsm.c](../../App/control/optical_fsm.c) | F10 | `OpticalFSM_GetState`、`OpticalFSM_Reset`、`OpticalFSM_Select`、`OpticalFSM_Calculate`、`OpticalFSM_Update` |
| [App/control/optical_fsm.h](../../App/control/optical_fsm.h) | F10 | 类型/布局/配置；关联同模块C定义 |
| [App/control/pid.c](../../App/control/pid.c) | F08 | `PID_GetState`、`PID_SetGain`、`PID_SetTrackGains`、`PID_SetAngleGains`、`PID_CalculateAngle`、`PID_UpdateSpeedLeft`、`PID_UpdateSpeedRight`、`PID_ResetSpeed`、`PID_ResetAll` |
| [App/control/pid.h](../../App/control/pid.h) | F08 | 类型/布局/配置；关联同模块C定义 |
| [App/control/speed_controller.c](../../App/control/speed_controller.c) | F08 | `SpeedController_Update`、`SpeedController_UpdateLeft`、`SpeedController_UpdateRight`、`SpeedController_Reset` |
| [App/control/speed_controller.h](../../App/control/speed_controller.h) | F08 | 类型/布局/配置；关联同模块C定义 |
| [App/control/tracking.c](../../App/control/tracking.c) | F11 | `Tracking_GetState`、`Tracking_SetGear`、`Tracking_CompleteControl`、`Tracking_Stopped`、`Tracking_AddProtection`、`Tracking_Observe`、`Tracking_Execute`、`Tracking_AccumulateMotion`、`Control_ResetRunState`、`Tracking_GetTrackOutput`、`Tracking_SetTrackOutput`、`Tracking_GetKernelState`、`Tracking_SetKernelState`、`Tracking_GetCaskFlag`、`Tracking_SetCaskFlag`、`Tracking_GetRoundaboutState`、`Tracking_SetRoundaboutState`、`Tracking_SetMode` |
| [App/control/tracking.h](../../App/control/tracking.h) | F11 | 类型/布局/配置；关联同模块C定义 |
| [App/system/scheduler.c](../../App/system/scheduler.c) | F01 | 协作模块；完整符号表见[F01主条目](../system/runtime-and-safety.md#f01) |
