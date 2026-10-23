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
