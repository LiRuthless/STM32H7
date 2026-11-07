# 双模式循迹与控制功能总览

现行功能按独立机制拆分，F08/F10/F11/F12身份和本页旧锚点保留。核对基线8d5ab2f（2026-11-07）。具体算法和值表只在下列主说明维护，历史Phase仍保存当时完整设计与结果。

<a id="f08"></a>
## F08 方向角度速度与执行

[控制器总说明](controllers.md#f08)：[方向](controllers.md#f08-direction)、[角度](controllers.md#f08-angle)、[速度PI](controllers.md#f08-speed)；[差速/反馈/电机执行](motor-execution.md#f08-motor)。

<a id="f10"></a>
## F10 光电状态机

[光电状态、等待、失线锁存](tracking-modes.md#f10)。

<a id="f11"></a>
## F11 双FSM与唯一执行

[观察、计算和唯一执行完整拍](tracking-modes.md#f11)。

<a id="f12"></a>
## F12 部分配置与全局调参规划

[完整fields主态/子态矩阵与覆盖顺序](profiles.md#f12)。Phase10未实施目标保留在该阶段，不将静态表存在解释为全局调参完成。

## 本页实现与模块反查

下表是当前源码导航；语义与配置在上述主题主说明维护，协作行转向主条目。公共/私有辅助函数归所属功能，不把声明视为另一实现。Core生成部分只作边界，用户区按原标记维护。

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
