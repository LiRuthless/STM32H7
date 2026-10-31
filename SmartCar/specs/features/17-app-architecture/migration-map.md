> 2026-10-31：本清单是Phase17迁移基线的符号/历史导航，JSON不在本轮改写。现行功能及模块归属先见[docs架构](../../../Docs/architecture.md)，需求从[Phase→docs](../../../Docs/traceability.md)进入；本表不再维护一套现行功能契约。

# G1 逐文件、逐函数迁移清单

基线：`4745aa2339209d5df9a739449685fc7f1cb66429`。G1符号不改名、作用域不变；目标是物理归类，后续组另行登记接口。

| 原文件 | G1目标 |
|---|---|
| App/control.c | App/control/control.c |
| App/control.h | App/control/control.h |
| App/dashboard.c | App/hmi/dashboard.c |
| App/dashboard.h | App/hmi/dashboard.h |
| App/datalog.c | App/services/datalog.c |
| App/datalog.h | App/services/datalog.h |
| App/electromagnetic_fsm.c | App/control/electromagnetic_fsm.c |
| App/electromagnetic_fsm.h | App/control/electromagnetic_fsm.h |
| App/element.c | App/control/element.c |
| App/element.h | App/control/element.h |
| App/filter.c | App/sensing/filter.c |
| App/filter.h | App/sensing/filter.h |
| App/gray_calibration.h | App/sensing/gray_calibration.h |
| App/gray_sensor.c | App/sensing/gray_sensor.c |
| App/gray_sensor.h | App/sensing/gray_sensor.h |
| App/imu_proc.c | App/sensing/imu_proc.c |
| App/imu_proc.h | App/sensing/imu_proc.h |
| App/menu.c | App/hmi/menu.c |
| App/menu.h | App/hmi/menu.h |
| App/motor.c | App/control/motor.c |
| App/motor.h | App/control/motor.h |
| App/optical_fsm.c | App/control/optical_fsm.c |
| App/optical_fsm.h | App/control/optical_fsm.h |
| App/param.c | App/services/param.c |
| App/param.h | App/services/param.h |
| App/pid.c | App/control/pid.c |
| App/pid.h | App/control/pid.h |
| App/roundabout.c | App/control/roundabout.c |
| App/roundabout.h | App/control/roundabout.h |
| App/scb_link.c | App/services/scb_link.c |
| App/scb_link.h | App/services/scb_link.h |
| App/scb_protocol.c | App/services/scb_protocol.c |
| App/scb_protocol.h | App/services/scb_protocol.h |
| App/telemetry.c | App/services/telemetry.c |
| App/telemetry.h | App/services/telemetry.h |
| App/track_sensor.c | App/sensing/track_sensor.c |
| App/track_sensor.h | App/sensing/track_sensor.h |
| App/tuning.c | App/control/tuning.c |
| App/tuning.h | App/control/tuning.h |
| App/wireless.c | App/services/wireless.c |
| App/wireless.h | App/services/wireless.h |

## app.c → app.c

函数：`App_IsStopped`、`App_Init`、`App_Loop`、`App_TryStart`、`App_ControlISR`、`App_TaskISR`、`HAL_TIM_PeriodElapsedCallback`、`App_SampleISR`、`App_RequestStop`、`App_FinalizeStop`、`App_ResetRunState`、`App_AbsFloat`。

对象/类型/宏和条件编译分支随文件整体迁移，原始完整声明以基线文件为准；G1不改变所有权。

## control.c → control/control.c

函数：`Control_ApplyProfile`、`Tracking_GetState`、`Tracking_TrySetMode`、`Tracking_SetGear`、`Tracking_CompleteControl`、`Tracking_Stopped`、`Tracking_AddProtection`、`Control_GetKernelProfile`、`Control_MergeProfile`、`Tracking_Observe`、`Tracking_Execute`、`Tracking_AccumulateMotion`、`Control_UpdateLegacy`、`Control_ResetRunState`。

对象/类型/宏和条件编译分支随文件整体迁移，原始完整声明以基线文件为准；G1不改变所有权。

## dashboard.c → hmi/dashboard.c

函数：`Dashboard_Init`、`Dashboard_Update`。

对象/类型/宏和条件编译分支随文件整体迁移，原始完整声明以基线文件为准；G1不改变所有权。

## datalog.c → services/datalog.c

函数：`Datalog_CalculateCrc16`、`Datalog_LatchFault`、`Datalog_RequestFaultStop`、`Datalog_SetStopSummary`、`Datalog_GetStatus`、`Datalog_IsActive`、`Datalog_GetCount`、`Datalog_Init`、`Datalog_SetProfile`、`Datalog_Start`、`Datalog_Clear`、`Datalog_GetMeta`、`Datalog_CancelExport`、`Datalog_ServiceExport`、`Datalog_IsExporting`、`Datalog_GetExportStatus`、`Datalog_BeginExport`、`Datalog_ReadExport`、`Datalog_EndExport`、`Datalog_PublishControl`、`Datalog_Push`、`Datalog_Service`、`Datalog_Flush`、`Datalog_Stop`、`Datalog_IsHeaderValid`、`Datalog_Dump`。

对象/类型/宏和条件编译分支随文件整体迁移，原始完整声明以基线文件为准；G1不改变所有权。

## electromagnetic_fsm.c → control/electromagnetic_fsm.c

函数：`ElectromagneticFSM_GetState`、`ElectromagneticFSM_Reset`、`ElectromagneticFSM_Accumulate`、`ElectromagneticFSM_GetDistance`、`ElectromagneticFSM_ClearDistance`、`ElectromagneticFSM_ClearAngle`、`ElectromagneticFSM_Measure`、`ElectromagneticFSM_Select`、`ElectromagneticFSM_Calculate`、`ElectromagneticFSM_Update`。

对象/类型/宏和条件编译分支随文件整体迁移，原始完整声明以基线文件为准；G1不改变所有权。

## element.c → control/element.c

函数：`Element_Judge`、`Element_JudgeStraight`、`Element_JudgeCrossroads`、`Element_JudgeCrossroadsExit`、`Element_JudgeTeeterboard`、`Element_JudgeTeeterboardExit`、`Element_JudgeCask`、`Element_JudgeCaskExit`。

对象/类型/宏和条件编译分支随文件整体迁移，原始完整声明以基线文件为准；G1不改变所有权。

## filter.c → sensing/filter.c

函数：`Filter_InitLowpass`、`Filter_UpdateLowpass`、`Filter_InitHighpass`、`Filter_UpdateHighpass`。

对象/类型/宏和条件编译分支随文件整体迁移，原始完整声明以基线文件为准；G1不改变所有权。

## gray_sensor.c → sensing/gray_sensor.c

函数：`Gray_GetState`、`Gray_GetBlack`、`Gray_GetWhite`、`Gray_ValidateCalibration`、`Gray_CheckCalibration`、`Gray_Normalize`、`Gray_ComputeError`、`Gray_ResetSequence`、`Gray_Init`、`Gray_Update`。

对象/类型/宏和条件编译分支随文件整体迁移，原始完整声明以基线文件为准；G1不改变所有权。

## imu_proc.c → sensing/imu_proc.c

函数：`IMU_GetAngleDeltaX`、`IMU_GetState`、`IMU_ZeroAngleX`、`IMU_Init`、`IMU_UpdateAccelVelocity`、`IMU_UpdateGyroAngle`、`IMU_CalibrateAccel`、`IMU_CalibrateGyro`、`IMU_ResetRunState`。

对象/类型/宏和条件编译分支随文件整体迁移，原始完整声明以基线文件为准；G1不改变所有权。

## menu.c → hmi/menu.c

函数：`Menu_ReadKeyAverage`、`Menu_ScanKey`、`Menu_HandleKey`、`Menu_DrawContent`、`Menu_DrawCursor`、`Menu_Update`。

对象/类型/宏和条件编译分支随文件整体迁移，原始完整声明以基线文件为准；G1不改变所有权。

## motor.c → control/motor.c

函数：`Motor_GetState`、`Motor_SetTargets`、`Motor_SetBaseSpeed`、`Motor_SetFanDuty`、`Motor_SetFanDutyIdle`、`Motor_ResetDistance`、`Motor_DistributeSpeed`、`Motor_UpdateControl`、`Motor_ReadEncoder`、`Motor_Init`、`Motor_InitEncoder`、`Motor_ResetRunState`、`Motor_EmergencyStop`。

对象/类型/宏和条件编译分支随文件整体迁移，原始完整声明以基线文件为准；G1不改变所有权。

## optical_fsm.c → control/optical_fsm.c

函数：`OpticalFSM_GetState`、`OpticalFSM_Reset`、`OpticalFSM_Select`、`OpticalFSM_Calculate`、`OpticalFSM_Update`。

对象/类型/宏和条件编译分支随文件整体迁移，原始完整声明以基线文件为准；G1不改变所有权。

## param.c → services/param.c

函数：`Param_CalculateCrc16`、`Param_CalculateCrcV1`、`Param_CalculateCrcV2`、`Param_CalculateCrcV3`、`Param_ClampFanDuty`、`Param_SetOpticalDefaults`、`Param_SetDefaults`、`Param_LoadBanks`、`Param_StoreBanks`、`Param_GetDirection`、`Param_ApplyCurrentDirection`、`Param_SetDirection`、`Param_GetFan`、`Param_SetFan`、`Param_SelectGear`、`Param_WriteMirror`、`Param_Load`、`Param_MarkDirty`、`Param_TryFlushPending`、`Param_Save`、`Param_ServiceSave`、`Param_FlushPending`、`Param_GetSpeedGains`、`Param_SetSpeedGains`。

对象/类型/宏和条件编译分支随文件整体迁移，原始完整声明以基线文件为准；G1不改变所有权。

## pid.c → control/pid.c

函数：`PID_GetState`、`PID_SetGain`、`PID_SetTrackGains`、`PID_SetAngleGains`、`PID_DirectionStep`、`PID_UpdateTrack`、`PID_UpdateAngle`、`PID_UpdateIncrementalLeft`、`PID_UpdateIncrementalRight`、`PID_UpdateSpeed`、`PID_UpdateSpeedLeft`、`PID_UpdateSpeedRight`、`PID_ResetSpeed`、`PID_ResetAll`。

对象/类型/宏和条件编译分支随文件整体迁移，原始完整声明以基线文件为准；G1不改变所有权。

## roundabout.c → control/roundabout.c

函数：`Roundabout_GetProfile`、`Roundabout_Update`、`Roundabout_JudgePreentryLeft`、`Roundabout_JudgePreentryRight`、`Roundabout_JudgePreentryExit`、`Roundabout_JudgeEntryLeft`、`Roundabout_JudgeEntryRight`、`Roundabout_JudgeAhead`、`Roundabout_JudgeEntered`、`Roundabout_JudgeEnteredAgain`、`Roundabout_JudgePreExit`、`Roundabout_JudgeExit`、`Roundabout_JudgeExited`、`Roundabout_Judge`、`Roundabout_ResetRunState`。

对象/类型/宏和条件编译分支随文件整体迁移，原始完整声明以基线文件为准；G1不改变所有权。

## scb_link.c → services/scb_link.c

函数：`SCB_FindRequest`、`SCB_AllocateRequest`、`SCB_SetHandlers`、`SCB_ResetSession`、`SCB_Init`、`SCB_QueueEvent`、`SCB_TakeEvent`、`SCB_FormatEvent`、`SCB_QueueResult`、`SCB_GetRequestResult`、`SCB_AcceptRequest`、`SCB_HandleAck`、`SCB_ReceiveFrame`、`SCB_StartReliable`、`SCB_PrepareFrame`、`SCB_Service`、`SCB_GetStatus`。

对象/类型/宏和条件编译分支随文件整体迁移，原始完整声明以基线文件为准；G1不改变所有权。

## scb_protocol.c → services/scb_protocol.c

函数：`SCB_ReadU16`、`SCB_ReadU32`、`SCB_WriteU16`、`SCB_WriteU32`、`SCB_CalculateCrc`、`SCB_BuildFrame`、`SCB_EncodeFrame`、`SCB_DecodeFrame`、`SCB_ValidateUtf8`、`SCB_SkipSpace`、`SCB_ReadHex`、`SCB_AppendUtf8`、`SCB_ParseString`、`SCB_ParseNumber`、`SCB_ParseValue`、`SCB_ParseJson`、`SCB_FindMember`、`SCB_GetString`、`SCB_GetNumber`、`SCB_StatusName`、`SCB_JsonBegin`、`SCB_JsonAppend`、`SCB_JsonU32`、`SCB_JsonI32`、`SCB_JsonFloat`、`SCB_JsonValues`、`SCB_ParseCommand`、`SCB_ValidateValues`。

对象/类型/宏和条件编译分支随文件整体迁移，原始完整声明以基线文件为准；G1不改变所有权。

## telemetry.c → services/telemetry.c

函数：`Telemetry_Init`、`Telemetry_ResetSession`、`Telemetry_BeginRun`、`Telemetry_PushRecord`、`Telemetry_GetStatus`、`Telemetry_SetRevision`、`Telemetry_Service`。

对象/类型/宏和条件编译分支随文件整体迁移，原始完整声明以基线文件为准；G1不改变所有权。

## track_sensor.c → sensing/track_sensor.c

函数：`Track_GetState`、`Track_CalculateError`、`Track_PublishObservation`、`Track_SetWeights`、`Track_UpdateError`、`Track_UpdateSymmetry`、`Track_ReadAdc`、`Track_InitAdc`。

对象/类型/宏和条件编译分支随文件整体迁移，原始完整声明以基线文件为准；G1不改变所有权。

## tuning.c → control/tuning.c

函数：`Tuning_IsOrdinary`、`Tuning_ReadActive`、`Tuning_ApplyShared`、`Tuning_Equal`、`Tuning_Event`、`Tuning_Begin`、`Tuning_Send`、`Tuning_Position`、`Tuning_Merge`、`Tuning_RejectPending`、`Tuning_SelectControl`、`Tuning_GetControl`、`Tuning_CompleteControl`、`Tuning_Stop`、`Tuning_FinalizeStop`、`Tuning_ResetSession`、`Tuning_GetStatus`、`Tuning_Service`、`Tuning_ReadBank`、`Tuning_QueryParams`、`Tuning_QueryStatus`、`Tuning_QueryResult`、`Tuning_Set`、`Tuning_Switch`、`Tuning_QueryLogMeta`、`Tuning_LogCommand`、`Tuning_HandleCommand`、`Tuning_Init`。

对象/类型/宏和条件编译分支随文件整体迁移，原始完整声明以基线文件为准；G1不改变所有权。

## wireless.c → services/wireless.c

函数：`Wireless_Adjust`。

对象/类型/宏和条件编译分支随文件整体迁移，原始完整声明以基线文件为准；G1不改变所有权。

## G2～G6 最终迁移导航（2026-10-24）

上文保留G1机械迁移历史，不表示最终仍存在control.c/param.c/datalog.c/scb_protocol.c。
完整逐项定位见[final-symbol-map.json](final-symbol-map.json)，检查入口为`tests/app_migration_check.py`。
该索引以原提交4745aa2339209d5df9a739449685fc7f1cb66429为基准，覆盖44个原App C/H、1395个原项；
当前95个App C/H及6个BSP适配C/H共1772项，包含公共/私有函数、声明、文件级/静态局部对象、
typedef、成员、枚举、宏、布局断言及全部条件编译分支。数字在最终源码冻结后复验，实际结果以validation为准。

每项保存path/symbol/kind/line/conditional；函数保存完整签名、直接调用与函数地址引用调用方，
Core/BSP外部调用位置另列external_call_sites；owner/context说明状态写入上下文，validation给出验证入口。
宏展开、运行时函数指针和并发一致性不能由此索引证明；handler补边/真实C回归与人工时序审查独立记录。

### 完整文件归属

同一原文件可分拆为多个最终owner；下表列实际目标C/H，不新建目录或兼容宏。

| 原App文件 | 最终文件（均相对SmartCar/） |
|---|---|
| app.c | `App/app.c`、`App/app.h`、`App/app_types.h`、`App/control/legacy_roundabout.c`、`App/control/legacy_roundabout.h`、`App/sensing/battery.c`、`App/sensing/battery.h`、`App/services/wireless.c`、`App/services/wireless.h`、`App/system/lifecycle.c`、`App/system/lifecycle.h`、`App/system/safety.c`、`App/system/safety.h`、`App/system/scheduler.c`、`App/system/scheduler.h`、`BSP/bsp_runtime.c`、`BSP/bsp_runtime.h` |
| app.h | `App/app.c`、`App/app.h`、`App/app_types.h`、`App/control/legacy_roundabout.c`、`App/control/legacy_roundabout.h`、`App/sensing/battery.c`、`App/sensing/battery.h`、`App/services/wireless.c`、`App/services/wireless.h`、`App/system/lifecycle.c`、`App/system/lifecycle.h`、`App/system/safety.c`、`App/system/safety.h`、`App/system/scheduler.c`、`App/system/scheduler.h`、`BSP/bsp_runtime.h` |
| app_config.h | `App/app_config.h` |
| control.c | `App/control/control_profile.c`、`App/control/control_profile.h`、`App/control/legacy_control.c`、`App/control/legacy_control.h`、`App/control/tracking.c`、`App/control/tracking.h`、`App/system/lifecycle.c`、`App/system/lifecycle.h` |
| control.h | `App/app_types.h`、`App/control/control_profile.c`、`App/control/control_profile.h`、`App/control/legacy_control.c`、`App/control/legacy_control.h`、`App/control/tracking.c`、`App/control/tracking.h`、`App/system/lifecycle.c`、`App/system/lifecycle.h` |
| dashboard.c | `App/hmi/dashboard.c`、`App/hmi/dashboard.h` |
| dashboard.h | `App/hmi/dashboard.c`、`App/hmi/dashboard.h` |
| datalog.c | `App/services/datalog.h`、`App/services/log_export.c`、`App/services/log_store.c`、`App/services/logger.c`、`App/services/logger.h`、`App/services/run_record.c`、`App/services/run_record.h` |
| datalog.h | `App/services/checksum.h`、`App/services/datalog.h`、`App/services/log_export.c`、`App/services/log_export.h`、`App/services/log_store.c`、`App/services/log_store.h`、`App/services/logger.c`、`App/services/logger.h`、`App/services/run_record.c`、`App/services/run_record.h` |
| electromagnetic_fsm.c | `App/control/electromagnetic_fsm.c`、`App/control/electromagnetic_fsm.h` |
| electromagnetic_fsm.h | `App/control/electromagnetic_fsm.c`、`App/control/electromagnetic_fsm.h` |
| element.c | `App/control/legacy_element.c`、`App/control/legacy_element.h`、`App/system/lifecycle.c` |
| element.h | `App/control/legacy_element.c`、`App/control/legacy_element.h`、`App/system/lifecycle.c`、`App/system/lifecycle.h` |
| filter.c | `App/sensing/filter.c`、`App/sensing/filter.h` |
| filter.h | `App/sensing/filter.c`、`App/sensing/filter.h` |
| gray_calibration.h | `App/sensing/gray_calibration.h` |
| gray_sensor.c | `App/sensing/gray_sensor.c`、`App/sensing/gray_sensor.h` |
| gray_sensor.h | `App/sensing/gray_sensor.c`、`App/sensing/gray_sensor.h` |
| imu_proc.c | `App/sensing/imu_proc.c`、`App/sensing/imu_proc.h`、`App/sensing/legacy_imu.c` |
| imu_proc.h | `App/sensing/imu_proc.c`、`App/sensing/imu_proc.h`、`App/sensing/legacy_imu.c`、`App/sensing/legacy_imu.h` |
| menu.c | `App/hmi/menu.c`、`App/hmi/menu.h`、`App/hmi/menu_keys.c`、`App/hmi/menu_keys.h`、`App/hmi/menu_render.c` |
| menu.h | `App/hmi/menu.c`、`App/hmi/menu.h`、`App/hmi/menu_keys.c`、`App/hmi/menu_keys.h`、`App/hmi/menu_render.c`、`App/hmi/menu_render.h` |
| motor.c | `App/control/motor.c`、`App/control/motor.h` |
| motor.h | `App/control/motor.c`、`App/control/motor.h` |
| optical_fsm.c | `App/control/optical_fsm.c`、`App/control/optical_fsm.h` |
| optical_fsm.h | `App/control/optical_fsm.c`、`App/control/optical_fsm.h` |
| param.c | `App/services/parameter_bank.c`、`App/services/parameter_bank.h`、`App/services/parameter_image.h`、`App/services/parameter_store.c`、`App/services/parameter_store.h`、`App/system/parameter_application.c`、`App/system/parameter_application.h` |
| param.h | `App/app_types.h`、`App/services/parameter_bank.c`、`App/services/parameter_bank.h`、`App/services/parameter_image.h`、`App/services/parameter_store.c`、`App/services/parameter_store.h`、`App/system/parameter_application.c`、`App/system/parameter_application.h` |
| pid.c | `App/control/angle_controller.c`、`App/control/direction_controller.c`、`App/control/direction_controller.h`、`App/control/legacy_control.c`、`App/control/legacy_control.h`、`App/control/legacy_pid.c`、`App/control/legacy_pid.h`、`App/control/pid.c`、`App/control/pid.h`、`App/control/speed_controller.c` |
| pid.h | `App/control/angle_controller.c`、`App/control/angle_controller.h`、`App/control/direction_controller.c`、`App/control/direction_controller.h`、`App/control/legacy_control.c`、`App/control/legacy_control.h`、`App/control/legacy_pid.c`、`App/control/legacy_pid.h`、`App/control/pid.c`、`App/control/pid.h`、`App/control/speed_controller.h` |
| roundabout.c | `App/control/control_profile.c`、`App/control/control_profile.h`、`App/control/legacy_roundabout.c`、`App/control/legacy_roundabout.h`、`App/control/tracking.c`、`App/system/lifecycle.c` |
| roundabout.h | `App/control/control_profile.c`、`App/control/control_profile.h`、`App/control/legacy_roundabout.c`、`App/control/legacy_roundabout.h`、`App/control/tracking.c`、`App/control/tracking.h`、`App/system/lifecycle.c`、`App/system/lifecycle.h` |
| scb_link.c | `App/services/scb_link.c`、`App/services/scb_link.h` |
| scb_link.h | `App/services/scb_link.c`、`App/services/scb_link.h` |
| scb_protocol.c | `App/control/tuning_values.c`、`App/services/checksum.c`、`App/services/scb_command_codec.c`、`App/services/scb_command_codec.h`、`App/services/scb_frame.c`、`App/services/scb_frame.h`、`App/services/scb_json.c`、`App/services/scb_json.h` |
| scb_protocol.h | `App/control/tuning_values.c`、`App/control/tuning_values.h`、`App/services/checksum.c`、`App/services/checksum.h`、`App/services/scb_command_codec.c`、`App/services/scb_command_codec.h`、`App/services/scb_frame.c`、`App/services/scb_frame.h`、`App/services/scb_json.c`、`App/services/scb_json.h` |
| telemetry.c | `App/services/telemetry.c`、`App/services/telemetry.h` |
| telemetry.h | `App/services/telemetry.c`、`App/services/telemetry.h` |
| track_sensor.c | `App/sensing/track_sensor.c`、`App/sensing/track_sensor.h` |
| track_sensor.h | `App/sensing/track_sensor.c`、`App/sensing/track_sensor.h` |
| tuning.c | `App/control/tuning.c`、`App/control/tuning.h`、`App/services/command_dispatch.c`、`App/services/command_dispatch.h`、`App/system/tuning_application.c`、`App/system/tuning_application.h` |
| tuning.h | `App/app_types.h`、`App/control/tuning.c`、`App/control/tuning.h`、`App/control/tuning_values.h`、`App/services/command_dispatch.c`、`App/services/command_dispatch.h`、`App/system/tuning_application.c`、`App/system/tuning_application.h` |
| wireless.c | `App/services/wireless.c`、`App/services/wireless.h` |
| wireless.h | `App/services/wireless.c`、`App/services/wireless.h` |

### 全部原函数的最终实现

261个原定义逐项列出，含private、回调和休眠函数；同名头声明及全部新增函数的签名/调用方在JSON中。
多目标表示原职责拆开，不能只根据同名入口认定迁移完成。

| 原文件::函数 | 最终实现 |
|---|---|
| `app.c::App_IsStopped` | `App/app.c::App_IsStopped`、`App/system/lifecycle.c::Lifecycle_IsStopped` |
| `app.c::App_Init` | `App/app.c::App_Init`、`App/system/lifecycle.c::Lifecycle_Init` |
| `app.c::App_Loop` | `App/app.c::App_Loop`、`App/system/scheduler.c::Scheduler_Loop` |
| `app.c::App_TryStart` | `App/system/lifecycle.c::Lifecycle_TryStart` |
| `app.c::App_ControlISR` | `App/app.c::App_ControlISR`、`App/system/scheduler.c::Scheduler_ControlISR` |
| `app.c::App_TaskISR` | `App/app.c::App_TaskISR`、`App/system/scheduler.c::Scheduler_TaskISR` |
| `app.c::HAL_TIM_PeriodElapsedCallback` | `BSP/bsp_runtime.c::HAL_TIM_PeriodElapsedCallback` |
| `app.c::App_SampleISR` | `App/app.c::App_SampleISR`、`App/system/scheduler.c::Scheduler_SampleISR` |
| `app.c::App_RequestStop` | `App/app.c::App_RequestStop`、`App/system/safety.c::Safety_RequestStop` |
| `app.c::App_FinalizeStop` | `App/system/safety.c::Safety_FinalizeStop` |
| `app.c::App_ResetRunState` | `App/system/lifecycle.c::Lifecycle_ResetRunState` |
| `app.c::App_AbsFloat` | `App/control/legacy_roundabout.c::Roundabout_AbsFloat` |
| `control.c::Control_ApplyProfile` | `App/control/control_profile.c::Control_ApplyProfile` |
| `control.c::Tracking_GetState` | `App/control/tracking.c::Tracking_GetState` |
| `control.c::Tracking_TrySetMode` | `App/system/lifecycle.c::Lifecycle_TrySetMode`、`App/control/tracking.c::Tracking_SetMode` |
| `control.c::Tracking_SetGear` | `App/control/tracking.c::Tracking_SetGear` |
| `control.c::Tracking_CompleteControl` | `App/control/tracking.c::Tracking_CompleteControl` |
| `control.c::Tracking_Stopped` | `App/control/tracking.c::Tracking_Stopped` |
| `control.c::Tracking_AddProtection` | `App/control/tracking.c::Tracking_AddProtection` |
| `control.c::Control_GetKernelProfile` | `App/control/control_profile.c::Control_GetKernelProfile` |
| `control.c::Control_MergeProfile` | `App/control/control_profile.c::Control_MergeProfile` |
| `control.c::Tracking_Observe` | `App/control/tracking.c::Tracking_Observe` |
| `control.c::Tracking_Execute` | `App/control/tracking.c::Tracking_Execute` |
| `control.c::Tracking_AccumulateMotion` | `App/control/tracking.c::Tracking_AccumulateMotion` |
| `control.c::Control_UpdateLegacy` | `App/control/legacy_control.c::Control_UpdateLegacy` |
| `control.c::Control_ResetRunState` | `App/control/tracking.c::Control_ResetRunState` |
| `dashboard.c::Dashboard_Init` | `App/hmi/dashboard.c::Dashboard_Init` |
| `dashboard.c::Dashboard_Update` | `App/hmi/dashboard.c::Dashboard_Update` |
| `datalog.c::Datalog_CalculateCrc16` | `App/services/run_record.c::Datalog_CalculateCrc16` |
| `datalog.c::Datalog_LatchFault` | `App/services/logger.c::Logger_LatchFault` |
| `datalog.c::Datalog_RequestFaultStop` | `App/services/logger.c::Datalog_RequestFaultStop` |
| `datalog.c::Datalog_SetStopSummary` | `App/services/logger.c::Datalog_SetStopSummary` |
| `datalog.c::Datalog_GetStatus` | `App/services/logger.c::Datalog_GetStatus` |
| `datalog.c::Datalog_IsActive` | `App/services/logger.c::Datalog_IsActive` |
| `datalog.c::Datalog_GetCount` | `App/services/logger.c::Datalog_GetCount` |
| `datalog.c::Datalog_Init` | `App/services/logger.c::Datalog_Init`、`App/services/run_record.c::RunRecord_ResetSnapshot` |
| `datalog.c::Datalog_SetProfile` | `App/services/run_record.c::Datalog_SetProfile` |
| `datalog.c::Datalog_Start` | `App/services/logger.c::Datalog_Start`、`App/services/run_record.c::RunRecord_BuildHeader`、`App/services/log_store.c::LogStore_WriteHeader`、`App/services/log_store.c::LogStore_Erase` |
| `datalog.c::Datalog_Clear` | `App/services/logger.c::Datalog_Clear` |
| `datalog.c::Datalog_GetMeta` | `App/services/logger.c::Datalog_GetMeta` |
| `datalog.c::Datalog_CancelExport` | `App/services/log_export.c::Datalog_CancelExport` |
| `datalog.c::Datalog_ServiceExport` | `App/services/log_export.c::Datalog_ServiceExport` |
| `datalog.c::Datalog_IsExporting` | `App/services/log_export.c::Datalog_IsExporting` |
| `datalog.c::Datalog_GetExportStatus` | `App/services/log_export.c::Datalog_GetExportStatus` |
| `datalog.c::Datalog_BeginExport` | `App/services/log_export.c::Datalog_BeginExport` |
| `datalog.c::Datalog_ReadExport` | `App/services/log_export.c::Datalog_ReadExport` |
| `datalog.c::Datalog_EndExport` | `App/services/log_export.c::Datalog_EndExport` |
| `datalog.c::Datalog_PublishControl` | `App/services/run_record.c::Datalog_PublishControl` |
| `datalog.c::Datalog_Push` | `App/services/logger.c::Datalog_Push`、`App/services/run_record.c::RunRecord_CaptureSample` |
| `datalog.c::Datalog_Service` | `App/services/logger.c::Datalog_Service` |
| `datalog.c::Datalog_Flush` | `App/services/logger.c::Datalog_Flush` |
| `datalog.c::Datalog_Stop` | `App/services/logger.c::Datalog_Stop` |
| `datalog.c::Datalog_IsHeaderValid` | `App/services/run_record.c::RunRecord_IsHeaderValid` |
| `datalog.c::Datalog_Dump` | `App/services/log_export.c::Datalog_Dump` |
| `electromagnetic_fsm.c::ElectromagneticFSM_GetState` | `App/control/electromagnetic_fsm.c::ElectromagneticFSM_GetState` |
| `electromagnetic_fsm.c::ElectromagneticFSM_Reset` | `App/control/electromagnetic_fsm.c::ElectromagneticFSM_Reset` |
| `electromagnetic_fsm.c::ElectromagneticFSM_Accumulate` | `App/control/electromagnetic_fsm.c::ElectromagneticFSM_Accumulate` |
| `electromagnetic_fsm.c::ElectromagneticFSM_GetDistance` | `App/control/electromagnetic_fsm.c::ElectromagneticFSM_GetDistance` |
| `electromagnetic_fsm.c::ElectromagneticFSM_ClearDistance` | `App/control/electromagnetic_fsm.c::ElectromagneticFSM_ClearDistance` |
| `electromagnetic_fsm.c::ElectromagneticFSM_ClearAngle` | `App/control/electromagnetic_fsm.c::ElectromagneticFSM_ClearAngle` |
| `electromagnetic_fsm.c::ElectromagneticFSM_Measure` | `App/control/electromagnetic_fsm.c::ElectromagneticFSM_Measure` |
| `electromagnetic_fsm.c::ElectromagneticFSM_Select` | `App/control/electromagnetic_fsm.c::ElectromagneticFSM_Select` |
| `electromagnetic_fsm.c::ElectromagneticFSM_Calculate` | `App/control/electromagnetic_fsm.c::ElectromagneticFSM_Calculate` |
| `electromagnetic_fsm.c::ElectromagneticFSM_Update` | `App/control/electromagnetic_fsm.c::ElectromagneticFSM_Update` |
| `element.c::Element_Judge` | `App/control/legacy_element.c::Element_Judge` |
| `element.c::Element_JudgeStraight` | `App/control/legacy_element.c::Element_JudgeStraight` |
| `element.c::Element_JudgeCrossroads` | `App/control/legacy_element.c::Element_JudgeCrossroads` |
| `element.c::Element_JudgeCrossroadsExit` | `App/control/legacy_element.c::Element_JudgeCrossroadsExit` |
| `element.c::Element_JudgeTeeterboard` | `App/control/legacy_element.c::Element_JudgeTeeterboard` |
| `element.c::Element_JudgeTeeterboardExit` | `App/control/legacy_element.c::Element_JudgeTeeterboardExit` |
| `element.c::Element_JudgeCask` | `App/control/legacy_element.c::Element_JudgeCask` |
| `element.c::Element_JudgeCaskExit` | `App/control/legacy_element.c::Element_JudgeCaskExit` |
| `filter.c::Filter_InitLowpass` | `App/sensing/filter.c::Filter_InitLowpass` |
| `filter.c::Filter_UpdateLowpass` | `App/sensing/filter.c::Filter_UpdateLowpass` |
| `filter.c::Filter_InitHighpass` | `App/sensing/filter.c::Filter_InitHighpass` |
| `filter.c::Filter_UpdateHighpass` | `App/sensing/filter.c::Filter_UpdateHighpass` |
| `gray_sensor.c::Gray_GetState` | `App/sensing/gray_sensor.c::Gray_GetState` |
| `gray_sensor.c::Gray_GetBlack` | `App/sensing/gray_sensor.c::Gray_GetBlack` |
| `gray_sensor.c::Gray_GetWhite` | `App/sensing/gray_sensor.c::Gray_GetWhite` |
| `gray_sensor.c::Gray_ValidateCalibration` | `App/sensing/gray_sensor.c::Gray_ValidateCalibration` |
| `gray_sensor.c::Gray_CheckCalibration` | `App/sensing/gray_sensor.c::Gray_CheckCalibration` |
| `gray_sensor.c::Gray_Normalize` | `App/sensing/gray_sensor.c::Gray_Normalize` |
| `gray_sensor.c::Gray_ComputeError` | `App/sensing/gray_sensor.c::Gray_ComputeError` |
| `gray_sensor.c::Gray_ResetSequence` | `App/sensing/gray_sensor.c::Gray_ResetSequence` |
| `gray_sensor.c::Gray_Init` | `App/sensing/gray_sensor.c::Gray_Init` |
| `gray_sensor.c::Gray_Update` | `App/sensing/gray_sensor.c::Gray_Update` |
| `imu_proc.c::IMU_GetAngleDeltaX` | `App/sensing/imu_proc.c::IMU_GetAngleDeltaX` |
| `imu_proc.c::IMU_GetState` | `App/sensing/imu_proc.c::IMU_GetState` |
| `imu_proc.c::IMU_ZeroAngleX` | `App/sensing/imu_proc.c::IMU_ZeroAngleX` |
| `imu_proc.c::IMU_Init` | `App/sensing/imu_proc.c::IMU_Init` |
| `imu_proc.c::IMU_UpdateAccelVelocity` | `App/sensing/imu_proc.c::IMU_UpdateAccelVelocity`、`App/sensing/legacy_imu.c::LegacyIMU_UpdateAccelVelocity` |
| `imu_proc.c::IMU_UpdateGyroAngle` | `App/sensing/imu_proc.c::IMU_UpdateGyroAngle` |
| `imu_proc.c::IMU_CalibrateAccel` | `App/sensing/imu_proc.c::IMU_CalibrateAccel`、`App/sensing/legacy_imu.c::LegacyIMU_CalibrateAccel` |
| `imu_proc.c::IMU_CalibrateGyro` | `App/sensing/imu_proc.c::IMU_CalibrateGyro` |
| `imu_proc.c::IMU_ResetRunState` | `App/sensing/imu_proc.c::IMU_ResetRunState` |
| `menu.c::Menu_ReadKeyAverage` | `App/hmi/menu_keys.c::Menu_ReadKeyAverage` |
| `menu.c::Menu_ScanKey` | `App/hmi/menu_keys.c::Menu_ScanKey` |
| `menu.c::Menu_HandleKey` | `App/hmi/menu.c::Menu_HandleKey` |
| `menu.c::Menu_DrawContent` | `App/hmi/menu_render.c::Menu_DrawContent` |
| `menu.c::Menu_DrawCursor` | `App/hmi/menu_render.c::Menu_DrawCursor` |
| `menu.c::Menu_Update` | `App/hmi/menu.c::Menu_Update`、`App/hmi/menu_render.c::MenuRender_Reset`、`App/hmi/menu_render.c::MenuRender_PreparePage`、`App/hmi/menu_render.c::MenuRender_Draw` |
| `motor.c::Motor_GetState` | `App/control/motor.c::Motor_GetState` |
| `motor.c::Motor_SetTargets` | `App/control/motor.c::Motor_SetTargets` |
| `motor.c::Motor_SetBaseSpeed` | `App/control/motor.c::Motor_SetBaseSpeed` |
| `motor.c::Motor_SetFanDuty` | `App/control/motor.c::Motor_SetFanDuty` |
| `motor.c::Motor_SetFanDutyIdle` | `App/control/motor.c::Motor_SetFanDutyIdle` |
| `motor.c::Motor_ResetDistance` | `App/control/motor.c::Motor_ResetDistance` |
| `motor.c::Motor_DistributeSpeed` | `App/control/motor.c::Motor_DistributeSpeed` |
| `motor.c::Motor_UpdateControl` | `App/control/motor.c::Motor_UpdateControl` |
| `motor.c::Motor_ReadEncoder` | `App/control/motor.c::Motor_ReadEncoder` |
| `motor.c::Motor_Init` | `App/control/motor.c::Motor_Init` |
| `motor.c::Motor_InitEncoder` | `App/control/motor.c::Motor_InitEncoder` |
| `motor.c::Motor_ResetRunState` | `App/control/motor.c::Motor_ResetRunState` |
| `motor.c::Motor_EmergencyStop` | `App/control/motor.c::Motor_EmergencyStop` |
| `optical_fsm.c::OpticalFSM_GetState` | `App/control/optical_fsm.c::OpticalFSM_GetState` |
| `optical_fsm.c::OpticalFSM_Reset` | `App/control/optical_fsm.c::OpticalFSM_Reset` |
| `optical_fsm.c::OpticalFSM_Select` | `App/control/optical_fsm.c::OpticalFSM_Select` |
| `optical_fsm.c::OpticalFSM_Calculate` | `App/control/optical_fsm.c::OpticalFSM_Calculate` |
| `optical_fsm.c::OpticalFSM_Update` | `App/control/optical_fsm.c::OpticalFSM_Update` |
| `param.c::Param_CalculateCrc16` | `App/services/parameter_store.c::Param_CalculateCrc16` |
| `param.c::Param_CalculateCrcV1` | `App/services/parameter_store.c::Param_CalculateCrcV1` |
| `param.c::Param_CalculateCrcV2` | `App/services/parameter_store.c::Param_CalculateCrcV2` |
| `param.c::Param_CalculateCrcV3` | `App/services/parameter_store.c::Param_CalculateCrcV3` |
| `param.c::Param_ClampFanDuty` | `App/services/parameter_store.c::Param_ClampFanDuty` |
| `param.c::Param_SetOpticalDefaults` | `App/services/parameter_store.c::Param_SetOpticalDefaults` |
| `param.c::Param_SetDefaults` | `App/services/parameter_store.c::Param_SetDefaults` |
| `param.c::Param_LoadBanks` | `App/services/parameter_bank.c::ParamBank_LoadBanks` |
| `param.c::Param_StoreBanks` | `App/services/parameter_bank.c::ParamBank_StoreBanks` |
| `param.c::Param_GetDirection` | `App/services/parameter_bank.c::Param_GetDirection` |
| `param.c::Param_ApplyCurrentDirection` | `App/system/parameter_application.c::Param_ApplyCurrentDirection` |
| `param.c::Param_SetDirection` | `App/system/parameter_application.c::Param_SetDirection`、`App/services/parameter_bank.c::ParamBank_SetDirection` |
| `param.c::Param_GetFan` | `App/services/parameter_store.c::Param_GetFan` |
| `param.c::Param_SetFan` | `App/system/parameter_application.c::Param_SetFan` |
| `param.c::Param_SelectGear` | `App/system/parameter_application.c::Param_SelectGear` |
| `param.c::Param_WriteMirror` | `App/services/parameter_store.c::Param_WriteMirror` |
| `param.c::Param_Load` | `App/system/parameter_application.c::Param_Load`、`App/services/parameter_store.c::ParamStore_Load` |
| `param.c::Param_MarkDirty` | `App/services/parameter_store.c::Param_MarkDirty` |
| `param.c::Param_TryFlushPending` | `App/system/parameter_application.c::Param_TryFlushPending`、`App/services/parameter_store.c::ParamStore_TryFlushPending` |
| `param.c::Param_Save` | `App/system/parameter_application.c::Param_Save` |
| `param.c::Param_ServiceSave` | `App/system/parameter_application.c::Param_ServiceSave` |
| `param.c::Param_FlushPending` | `App/system/parameter_application.c::Param_FlushPending` |
| `param.c::Param_GetSpeedGains` | `App/services/parameter_store.c::Param_GetSpeedGains` |
| `param.c::Param_SetSpeedGains` | `App/system/parameter_application.c::Param_SetSpeedGains` |
| `pid.c::PID_GetState` | `App/control/pid.c::PID_GetState` |
| `pid.c::PID_SetGain` | `App/control/pid.c::PID_SetGain` |
| `pid.c::PID_SetTrackGains` | `App/control/pid.c::PID_SetTrackGains` |
| `pid.c::PID_SetAngleGains` | `App/control/pid.c::PID_SetAngleGains` |
| `pid.c::PID_DirectionStep` | `App/control/direction_controller.c::PID_DirectionStep` |
| `pid.c::PID_UpdateTrack` | `App/control/legacy_pid.c::PID_UpdateTrack`、`App/control/direction_controller.c::PID_DirectionStep` |
| `pid.c::PID_UpdateAngle` | `App/control/legacy_control.c::PID_UpdateAngle`、`App/control/pid.c::PID_CalculateAngle`、`App/control/angle_controller.c::AngleController_Update` |
| `pid.c::PID_UpdateIncrementalLeft` | `App/control/legacy_pid.c::PID_UpdateIncrementalLeft` |
| `pid.c::PID_UpdateIncrementalRight` | `App/control/legacy_pid.c::PID_UpdateIncrementalRight` |
| `pid.c::PID_UpdateSpeed` | `App/control/speed_controller.c::SpeedController_Update` |
| `pid.c::PID_UpdateSpeedLeft` | `App/control/pid.c::PID_UpdateSpeedLeft` |
| `pid.c::PID_UpdateSpeedRight` | `App/control/pid.c::PID_UpdateSpeedRight` |
| `pid.c::PID_ResetSpeed` | `App/control/pid.c::PID_ResetSpeed` |
| `pid.c::PID_ResetAll` | `App/control/pid.c::PID_ResetAll` |
| `roundabout.c::Roundabout_GetProfile` | `App/control/control_profile.c::Roundabout_GetProfile` |
| `roundabout.c::Roundabout_Update` | `App/control/legacy_roundabout.c::Roundabout_Update` |
| `roundabout.c::Roundabout_JudgePreentryLeft` | `App/control/legacy_roundabout.c::Roundabout_JudgePreentryLeft` |
| `roundabout.c::Roundabout_JudgePreentryRight` | `App/control/legacy_roundabout.c::Roundabout_JudgePreentryRight` |
| `roundabout.c::Roundabout_JudgePreentryExit` | `App/control/legacy_roundabout.c::Roundabout_JudgePreentryExit` |
| `roundabout.c::Roundabout_JudgeEntryLeft` | `App/control/legacy_roundabout.c::Roundabout_JudgeEntryLeft` |
| `roundabout.c::Roundabout_JudgeEntryRight` | `App/control/legacy_roundabout.c::Roundabout_JudgeEntryRight` |
| `roundabout.c::Roundabout_JudgeAhead` | `App/control/legacy_roundabout.c::Roundabout_JudgeAhead` |
| `roundabout.c::Roundabout_JudgeEntered` | `App/control/legacy_roundabout.c::Roundabout_JudgeEntered` |
| `roundabout.c::Roundabout_JudgeEnteredAgain` | `App/control/legacy_roundabout.c::Roundabout_JudgeEnteredAgain` |
| `roundabout.c::Roundabout_JudgePreExit` | `App/control/legacy_roundabout.c::Roundabout_JudgePreExit` |
| `roundabout.c::Roundabout_JudgeExit` | `App/control/legacy_roundabout.c::Roundabout_JudgeExit` |
| `roundabout.c::Roundabout_JudgeExited` | `App/control/legacy_roundabout.c::Roundabout_JudgeExited` |
| `roundabout.c::Roundabout_Judge` | `App/control/legacy_roundabout.c::Roundabout_Judge` |
| `roundabout.c::Roundabout_ResetRunState` | `App/control/legacy_roundabout.c::Roundabout_ResetRunState` |
| `scb_link.c::SCB_FindRequest` | `App/services/scb_link.c::SCB_FindRequest` |
| `scb_link.c::SCB_AllocateRequest` | `App/services/scb_link.c::SCB_AllocateRequest` |
| `scb_link.c::SCB_SetHandlers` | `App/services/scb_link.c::SCB_SetHandlers` |
| `scb_link.c::SCB_ResetSession` | `App/services/scb_link.c::SCB_ResetSession` |
| `scb_link.c::SCB_Init` | `App/services/scb_link.c::SCB_Init` |
| `scb_link.c::SCB_QueueEvent` | `App/services/scb_link.c::SCB_QueueEvent` |
| `scb_link.c::SCB_TakeEvent` | `App/services/scb_link.c::SCB_TakeEvent` |
| `scb_link.c::SCB_FormatEvent` | `App/services/scb_link.c::SCB_FormatEvent` |
| `scb_link.c::SCB_QueueResult` | `App/services/scb_link.c::SCB_QueueResult` |
| `scb_link.c::SCB_GetRequestResult` | `App/services/scb_link.c::SCB_GetRequestResult` |
| `scb_link.c::SCB_AcceptRequest` | `App/services/scb_link.c::SCB_AcceptRequest` |
| `scb_link.c::SCB_HandleAck` | `App/services/scb_link.c::SCB_HandleAck` |
| `scb_link.c::SCB_ReceiveFrame` | `App/services/scb_link.c::SCB_ReceiveFrame` |
| `scb_link.c::SCB_StartReliable` | `App/services/scb_link.c::SCB_StartReliable` |
| `scb_link.c::SCB_PrepareFrame` | `App/services/scb_link.c::SCB_PrepareFrame` |
| `scb_link.c::SCB_Service` | `App/services/scb_link.c::SCB_Service` |
| `scb_link.c::SCB_GetStatus` | `App/services/scb_link.c::SCB_GetStatus` |
| `scb_protocol.c::SCB_ReadU16` | `App/services/scb_frame.c::SCB_ReadU16` |
| `scb_protocol.c::SCB_ReadU32` | `App/services/scb_frame.c::SCB_ReadU32` |
| `scb_protocol.c::SCB_WriteU16` | `App/services/scb_frame.c::SCB_WriteU16` |
| `scb_protocol.c::SCB_WriteU32` | `App/services/scb_frame.c::SCB_WriteU32` |
| `scb_protocol.c::SCB_CalculateCrc` | `App/services/scb_frame.c::SCB_CalculateCrc`、`App/services/checksum.c::Checksum_UpdateCrc32` |
| `scb_protocol.c::SCB_BuildFrame` | `App/services/scb_frame.c::SCB_BuildFrame` |
| `scb_protocol.c::SCB_EncodeFrame` | `App/services/scb_frame.c::SCB_EncodeFrame` |
| `scb_protocol.c::SCB_DecodeFrame` | `App/services/scb_frame.c::SCB_DecodeFrame` |
| `scb_protocol.c::SCB_ValidateUtf8` | `App/services/scb_json.c::SCB_ValidateUtf8` |
| `scb_protocol.c::SCB_SkipSpace` | `App/services/scb_json.c::SCB_SkipSpace` |
| `scb_protocol.c::SCB_ReadHex` | `App/services/scb_json.c::SCB_ReadHex` |
| `scb_protocol.c::SCB_AppendUtf8` | `App/services/scb_json.c::SCB_AppendUtf8` |
| `scb_protocol.c::SCB_ParseString` | `App/services/scb_json.c::SCB_ParseString` |
| `scb_protocol.c::SCB_ParseNumber` | `App/services/scb_json.c::SCB_ParseNumber` |
| `scb_protocol.c::SCB_ParseValue` | `App/services/scb_json.c::SCB_ParseValue` |
| `scb_protocol.c::SCB_ParseJson` | `App/services/scb_json.c::SCB_ParseJson` |
| `scb_protocol.c::SCB_FindMember` | `App/services/scb_json.c::SCB_FindMember` |
| `scb_protocol.c::SCB_GetString` | `App/services/scb_json.c::SCB_GetString` |
| `scb_protocol.c::SCB_GetNumber` | `App/services/scb_json.c::SCB_GetNumber` |
| `scb_protocol.c::SCB_StatusName` | `App/services/scb_command_codec.c::SCB_StatusName` |
| `scb_protocol.c::SCB_JsonBegin` | `App/services/scb_json.c::SCB_JsonBegin` |
| `scb_protocol.c::SCB_JsonAppend` | `App/services/scb_json.c::SCB_JsonAppend` |
| `scb_protocol.c::SCB_JsonU32` | `App/services/scb_json.c::SCB_JsonU32` |
| `scb_protocol.c::SCB_JsonI32` | `App/services/scb_json.c::SCB_JsonI32` |
| `scb_protocol.c::SCB_JsonFloat` | `App/services/scb_json.c::SCB_JsonFloat` |
| `scb_protocol.c::SCB_JsonValues` | `App/services/scb_command_codec.c::SCB_JsonValues` |
| `scb_protocol.c::SCB_ParseCommand` | `App/services/scb_command_codec.c::SCB_ParseCommand` |
| `scb_protocol.c::SCB_ValidateValues` | `App/services/scb_command_codec.c::SCB_ValidateValues`、`App/control/tuning_values.c::Tuning_ValidateValues` |
| `telemetry.c::Telemetry_Init` | `App/services/telemetry.c::Telemetry_Init` |
| `telemetry.c::Telemetry_ResetSession` | `App/services/telemetry.c::Telemetry_ResetSession` |
| `telemetry.c::Telemetry_BeginRun` | `App/services/telemetry.c::Telemetry_BeginRun` |
| `telemetry.c::Telemetry_PushRecord` | `App/services/telemetry.c::Telemetry_PushRecord` |
| `telemetry.c::Telemetry_GetStatus` | `App/services/telemetry.c::Telemetry_GetStatus` |
| `telemetry.c::Telemetry_SetRevision` | `App/services/telemetry.c::Telemetry_SetRevision` |
| `telemetry.c::Telemetry_Service` | `App/services/telemetry.c::Telemetry_Service` |
| `track_sensor.c::Track_GetState` | `App/sensing/track_sensor.c::Track_GetState` |
| `track_sensor.c::Track_CalculateError` | `App/sensing/track_sensor.c::Track_CalculateError` |
| `track_sensor.c::Track_PublishObservation` | `App/sensing/track_sensor.c::Track_PublishObservation` |
| `track_sensor.c::Track_SetWeights` | `App/sensing/track_sensor.c::Track_SetWeights` |
| `track_sensor.c::Track_UpdateError` | `App/sensing/track_sensor.c::Track_UpdateError` |
| `track_sensor.c::Track_UpdateSymmetry` | `App/sensing/track_sensor.c::Track_UpdateSymmetry` |
| `track_sensor.c::Track_ReadAdc` | `App/sensing/track_sensor.c::Track_ReadAdc` |
| `track_sensor.c::Track_InitAdc` | `App/sensing/track_sensor.c::Track_InitAdc` |
| `tuning.c::Tuning_IsOrdinary` | `App/system/tuning_application.c::TuningApplication_IsOrdinary` |
| `tuning.c::Tuning_ReadActive` | `App/control/tuning.c::Tuning_ReadActive` |
| `tuning.c::Tuning_ApplyShared` | `App/control/tuning.c::Tuning_ApplyShared` |
| `tuning.c::Tuning_Equal` | `App/control/tuning.c::Tuning_Equal` |
| `tuning.c::Tuning_Event` | `App/control/tuning.c::Tuning_Event`、`App/services/command_dispatch.c::CommandDispatch_QueueTuningEvent`、`App/control/tuning.c::Tuning_CopyEventValues` |
| `tuning.c::Tuning_Begin` | `App/services/command_dispatch.c::CommandDispatch_Begin` |
| `tuning.c::Tuning_Send` | `App/services/command_dispatch.c::CommandDispatch_Send` |
| `tuning.c::Tuning_Position` | `App/services/command_dispatch.c::CommandDispatch_Position` |
| `tuning.c::Tuning_Merge` | `App/system/tuning_application.c::TuningApplication_Merge` |
| `tuning.c::Tuning_RejectPending` | `App/control/tuning.c::Tuning_RejectPending` |
| `tuning.c::Tuning_SelectControl` | `App/control/tuning.c::Tuning_SelectControl` |
| `tuning.c::Tuning_GetControl` | `App/control/tuning.c::Tuning_GetControl` |
| `tuning.c::Tuning_CompleteControl` | `App/control/tuning.c::Tuning_CompleteControl` |
| `tuning.c::Tuning_Stop` | `App/control/tuning.c::Tuning_Stop` |
| `tuning.c::Tuning_FinalizeStop` | `App/system/tuning_application.c::TuningApplication_FinalizeStop` |
| `tuning.c::Tuning_ResetSession` | `App/control/tuning.c::Tuning_ResetSession`、`App/services/command_dispatch.c::CommandDispatch_ResetSession` |
| `tuning.c::Tuning_GetStatus` | `App/control/tuning.c::Tuning_GetStatus` |
| `tuning.c::Tuning_Service` | `App/services/command_dispatch.c::CommandDispatch_Service`、`App/system/tuning_application.c::TuningApplication_ServiceSave`、`App/control/tuning.c::Tuning_TakeResult` |
| `tuning.c::Tuning_ReadBank` | `App/services/command_dispatch.c::CommandDispatch_ReadBank` |
| `tuning.c::Tuning_QueryParams` | `App/services/command_dispatch.c::CommandDispatch_QueryParams` |
| `tuning.c::Tuning_QueryStatus` | `App/services/command_dispatch.c::CommandDispatch_QueryStatus` |
| `tuning.c::Tuning_QueryResult` | `App/services/command_dispatch.c::CommandDispatch_QueryResult` |
| `tuning.c::Tuning_Set` | `App/system/tuning_application.c::TuningApplication_Set`、`App/services/command_dispatch.c::CommandDispatch_Set`、`App/control/tuning.c::Tuning_PrepareSet`、`App/control/tuning.c::Tuning_CommitParked` |
| `tuning.c::Tuning_Switch` | `App/control/tuning.c::Tuning_Switch`、`App/system/tuning_application.c::TuningApplication_Switch`、`App/services/command_dispatch.c::CommandDispatch_Switch` |
| `tuning.c::Tuning_QueryLogMeta` | `App/services/command_dispatch.c::CommandDispatch_QueryLogMeta` |
| `tuning.c::Tuning_LogCommand` | `App/services/command_dispatch.c::CommandDispatch_LogCommand` |
| `tuning.c::Tuning_HandleCommand` | `App/services/command_dispatch.c::CommandDispatch_HandleCommand` |
| `tuning.c::Tuning_Init` | `App/control/tuning.c::Tuning_Init`、`App/services/command_dispatch.c::CommandDispatch_Init` |
| `wireless.c::Wireless_Adjust` | `App/services/wireless.c::Wireless_Adjust` |

### 状态和非函数迁移的特殊项

- 原g_app各标志/控制拍/测距在lifecycle私有；stop_pending/首因/低压计数在safety；电池raw/filtered/filter在battery；串口两32B缓冲在wireless。
- 原App_Loop静态s_led_last位于Scheduler_Loop，仍跨起跑保留，不在运行复位时清零。
- 原g_control监视量与完整拍state没有合并；g_roundabout旧字段归legacy_roundabout_state_t，活动观察仍由电磁FSM及tracking拥有。
- 原菜单全局归menu/menu_keys；菜单绘制页/光标缓存归menu_render；按键滤波和页面状态不复制。
- 原param方向类型移至app_types；四bank在parameter_bank；v1/v2/v3及88B布局在parameter_image；镜像与保存时基在parameter_store，保存快照由system采集。
- 原Datalog_Start静态局部s_check迁为log_store文件私有对象，仍唯一256B；s_snapshot/s_header归run_record，环形槽/batch/统计归logger，export/CSV静态临时对象归log_export。
- 原scb_protocol的frame/JSON/command类型各归对应头，CRC32公式归checksum；SCB状态和日志业务result为独立类型，dispatch显式映射，不依赖数值巧合。
- 原Tuning协议命令/workspace拆到command_dispatch，事务core与即时发布单槽仍唯一；原status字段按新业务结果名code定位。只读const指针不保证跨ISR整包一致，使用契约见architecture。

JSON逐对象和类型成员提供所有原项→真实目标，不用上面摘要替代全量记录。休眠入口及真实已保留调用见
[休眠清单](../../research/2026-10-02-dormant-code-inventory.md)现行位置追加表；历史来源/验证记录保留。
