# 历史专项入口

2026-10-31：本页保留Phase15原专项基线与记录。当前功能/标准适用及R08维护入口见[docs F32](../../../docs/guides/validation.md#f32)，当前模块位置从[docs架构](../../../docs/architecture.md)反查。下面旧路径和计数不作为现行功能清单；原记录不改写。

<details>
<summary>展开原专项清单与工作流</summary>

# Phase 15 注释专项：逐函数与模块检查清单

> 行为保持比较基线：`f3c9baaf510c52df78ec9176924217c84247f426`。
> 2026-10-17 已按当前实现完成注释整改；每行记录实际定义位置及独立审查结论。

## 使用和关闭规则

- CF 表按每个真实定义登记，条件编译两屏的同名 LCD 复位函数分别验收，不合并分支。
- 本清单共 261 个定义位置、259 个文件内唯一函数符号；已与既有 symbol-map 的当前目标名集合
  交叉核对。所有 67 个自维护 C/H 均在 CM 表；另列 21 个 Core 文件，只验收现有 USER CODE。
- Core 没有完整定义在 USER CODE 内的自维护函数，因此 CF 不复制生成函数；其中用户代码段
  的解释、入口引用和版权例外纳入 CM。不得为补函数头而修改生成区。
- C 定义前完整 Doxygen、H 摘要、实际处理阶段、共享输入输出、异常/单位/时序事实分别核对。
  自动提取的缺项和类别只是审查入口，不能把“已有 Doxygen”当成内容通过。
- 类别候选中的纯 getter/单调用转发/空壳须人工确认后才允许省略 details；无确认仍按完整规则审查。
- 每项关闭时把“待实施”改成准确位置，把“待审查”改为审查日期、证据和结论；不可批量填“同上”。
- 记录来源/许可例外的具体文件、基线区间和内容指纹。实现后重定位；不凭旧行号接受豁免。
- CG1～CG4 的每项覆盖、格式、人工内容和非注释 token 保持都通过后，才能关闭该组；功能
  测试结果单列。任何未知事实记具体问题并询问用户，不能填“后续完善”后标通过。
- 基线没有紧邻 C Doxygen 的定义为 203 个；其余 58 个仍有标签/内容需复核。这是 v1.1 的
  迁移工作量，不表示它们全部违反当时以 H 为权威的旧条款。

## CF：逐定义内容与格式

| ID / 组 | C 定义 / 权威说明目标 | H 摘要目标 | 类别候选 / 已确认缺项 | 覆盖 | 格式 | 人工内容 | token 保持 |
|---|---|---|---|---|---|---|---|
| CF-001 / CG4 | `App/app.c:74::App_IsStopped`；定义前 | `App/app.h::App_IsStopped` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-002 / CG4 | `App/app.c:85::App_Init`；定义前 | `App/app.h::App_Init` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-003 / CG4 | `App/app.c:134::App_Loop`；定义前 | `App/app.h::App_Loop` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-004 / CG4 | `App/app.c:218::App_TryStart`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-005 / CG4 | `App/app.c:294::App_ControlISR`；定义前 | `App/app.h::App_ControlISR` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-006 / CG4 | `App/app.c:384::App_TaskISR`；定义前 | `App/app.h::App_TaskISR` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-007 / CG4 | `App/app.c:419::HAL_TIM_PeriodElapsedCallback`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-008 / CG4 | `App/app.c:437::App_SampleISR`；定义前 | `App/app.h::App_SampleISR` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-009 / CG4 | `App/app.c:451::App_RequestStop`；定义前 | `App/app.h::App_RequestStop` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-010 / CG4 | `App/app.c:521::App_FinalizeStop`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-011 / CG4 | `App/app.c:558::App_ResetRunState`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-012 / CG4 | `App/app.c:579::App_AbsFloat`；定义前 | `App/app.h::App_AbsFloat` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-013 / CG2 | `App/control.c:105::Control_ApplyProfile`；定义前 | `App/control.h::Control_ApplyProfile` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-014 / CG2 | `App/control.c:131::Tracking_GetState`；定义前 | `App/control.h::Tracking_GetState` | 纯 getter；职责、返回值、单位及读取上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-015 / CG2 | `App/control.c:145::Tracking_TrySetMode`；定义前 | `App/control.h::Tracking_TrySetMode` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-016 / CG2 | `App/control.c:178::Tracking_SetGear`；定义前 | `App/control.h::Tracking_SetGear` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-017 / CG2 | `App/control.c:191::Tracking_CompleteControl`；定义前 | `App/control.h::Tracking_CompleteControl` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-018 / CG2 | `App/control.c:202::Tracking_Stopped`；定义前 | `App/control.h::Tracking_Stopped` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-019 / CG2 | `App/control.c:229::Tracking_AddProtection`；定义前 | `App/control.h::Tracking_AddProtection` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-020 / CG2 | `App/control.c:243::Control_GetKernelProfile`；定义前 | `App/control.h::Control_GetKernelProfile` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-021 / CG2 | `App/control.c:255::Control_MergeProfile`；定义前 | `App/control.h::Control_MergeProfile` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-022 / CG2 | `App/control.c:284::Tracking_Observe`；定义前 | `App/control.h::Tracking_Observe` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-023 / CG2 | `App/control.c:328::Tracking_Execute`；定义前 | `App/control.h::Tracking_Execute` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-024 / CG2 | `App/control.c:364::Tracking_AccumulateMotion`；定义前 | `App/control.h::Tracking_AccumulateMotion` | 单调用转发；转发目标、返回语义及调用上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-025 / CG2 | `App/control.c:376::Control_UpdateLegacy`；定义前 | `App/control.h::Control_UpdateLegacy` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-026 / CG2 | `App/control.c:421::Control_ResetRunState`；定义前 | `App/control.h::Control_ResetRunState` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-027 / CG4 | `App/dashboard.c:22::Dashboard_Init`；定义前 | `App/dashboard.h::Dashboard_Init` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-028 / CG4 | `App/dashboard.c:43::Dashboard_Update`；定义前 | `App/dashboard.h::Dashboard_Update` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-029 / CG3 | `App/datalog.c:67::Datalog_CalculateCrc16`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-030 / CG3 | `App/datalog.c:88::Datalog_LatchFault`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-031 / CG3 | `App/datalog.c:111::Datalog_RequestFaultStop`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-032 / CG3 | `App/datalog.c:125::Datalog_SetStopSummary`；定义前 | `App/datalog.h::Datalog_SetStopSummary` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-033 / CG3 | `App/datalog.c:142::Datalog_GetStatus`；定义前 | `App/datalog.h::Datalog_GetStatus` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-034 / CG3 | `App/datalog.c:153::Datalog_IsActive`；定义前 | `App/datalog.h::Datalog_IsActive` | 纯 getter；职责、返回值、单位及读取上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-035 / CG3 | `App/datalog.c:163::Datalog_GetCount`；定义前 | `App/datalog.h::Datalog_GetCount` | 纯 getter；职责、返回值、单位及读取上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-036 / CG3 | `App/datalog.c:173::Datalog_Init`；定义前 | `App/datalog.h::Datalog_Init` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-037 / CG3 | `App/datalog.c:189::Datalog_SetProfile`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-038 / CG3 | `App/datalog.c:205::Datalog_Start`；定义前 | `App/datalog.h::Datalog_Start` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-039 / CG3 | `App/datalog.c:274::Datalog_Clear`；定义前 | `App/datalog.h::Datalog_Clear` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-040 / CG3 | `App/datalog.c:287::Datalog_PublishControl`；定义前 | `App/datalog.h::Datalog_PublishControl` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-041 / CG3 | `App/datalog.c:322::Datalog_Push`；定义前 | `App/datalog.h::Datalog_Push` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-042 / CG3 | `App/datalog.c:374::Datalog_Service`；定义前 | `App/datalog.h::Datalog_Service` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-043 / CG3 | `App/datalog.c:417::Datalog_Flush`；定义前 | `App/datalog.h::Datalog_Flush` | 单调用转发；转发目标、返回语义及调用上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-044 / CG3 | `App/datalog.c:427::Datalog_Stop`；定义前 | `App/datalog.h::Datalog_Stop` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-045 / CG3 | `App/datalog.c:445::Datalog_IsHeaderValid`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-046 / CG3 | `App/datalog.c:461::Datalog_Dump`；定义前 | `App/datalog.h::Datalog_Dump` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-047 / CG2 | `App/electromagnetic_fsm.c:27::ElectromagneticFSM_GetState`；定义前 | `App/electromagnetic_fsm.h::ElectromagneticFSM_GetState` | 纯 getter；职责、返回值、单位及读取上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-048 / CG2 | `App/electromagnetic_fsm.c:37::ElectromagneticFSM_Reset`；定义前 | `App/electromagnetic_fsm.h::ElectromagneticFSM_Reset` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-049 / CG2 | `App/electromagnetic_fsm.c:51::ElectromagneticFSM_Accumulate`；定义前 | `App/electromagnetic_fsm.h::ElectromagneticFSM_Accumulate` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-050 / CG2 | `App/electromagnetic_fsm.c:63::ElectromagneticFSM_GetDistance`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-051 / CG2 | `App/electromagnetic_fsm.c:73::ElectromagneticFSM_ClearDistance`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-052 / CG2 | `App/electromagnetic_fsm.c:84::ElectromagneticFSM_ClearAngle`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-053 / CG2 | `App/electromagnetic_fsm.c:96::ElectromagneticFSM_Measure`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-054 / CG2 | `App/electromagnetic_fsm.c:111::ElectromagneticFSM_UpdateDirection`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-055 / CG2 | `App/electromagnetic_fsm.c:127::ElectromagneticFSM_Update`；定义前 | `App/electromagnetic_fsm.h::ElectromagneticFSM_Update` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-056 / CG2 | `App/element.c:31::Element_Judge`；定义前 | `App/element.h::Element_Judge` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-057 / CG2 | `App/element.c:45::Element_JudgeStraight`；定义前 | `App/element.h::Element_JudgeStraight` | 休眠空壳；未接入事实、空操作和调用边已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-058 / CG2 | `App/element.c:55::Element_JudgeCrossroads`；定义前 | `App/element.h::Element_JudgeCrossroads` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-059 / CG2 | `App/element.c:69::Element_JudgeCrossroadsExit`；定义前 | `App/element.h::Element_JudgeCrossroadsExit` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-060 / CG2 | `App/element.c:83::Element_JudgeTeeterboard`；定义前 | `App/element.h::Element_JudgeTeeterboard` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-061 / CG2 | `App/element.c:98::Element_JudgeTeeterboardExit`；定义前 | `App/element.h::Element_JudgeTeeterboardExit` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-062 / CG2 | `App/element.c:118::Element_JudgeCask`；定义前 | `App/element.h::Element_JudgeCask` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-063 / CG2 | `App/element.c:131::Element_JudgeCaskExit`；定义前 | `App/element.h::Element_JudgeCaskExit` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-064 / CG2 | `App/filter.c:17::Filter_InitLowpass`；定义前 | `App/filter.h::Filter_InitLowpass` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-065 / CG2 | `App/filter.c:31::Filter_UpdateLowpass`；定义前 | `App/filter.h::Filter_UpdateLowpass` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-066 / CG2 | `App/filter.c:47::Filter_InitHighpass`；定义前 | `App/filter.h::Filter_InitHighpass` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-067 / CG2 | `App/filter.c:64::Filter_UpdateHighpass`；定义前 | `App/filter.h::Filter_UpdateHighpass` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-068 / CG1 | `App/gray_sensor.c:22::Gray_GetState`；定义前 | `App/gray_sensor.h::Gray_GetState` | 纯 getter；职责、返回值、单位及读取上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-069 / CG1 | `App/gray_sensor.c:32::Gray_GetBlack`；定义前 | `App/gray_sensor.h::Gray_GetBlack` | 纯 getter；职责、返回值、单位及读取上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-070 / CG1 | `App/gray_sensor.c:42::Gray_GetWhite`；定义前 | `App/gray_sensor.h::Gray_GetWhite` | 纯 getter；职责、返回值、单位及读取上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-071 / CG1 | `App/gray_sensor.c:55::Gray_ValidateCalibration`；定义前 | `App/gray_sensor.h::Gray_ValidateCalibration` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-072 / CG1 | `App/gray_sensor.c:80::Gray_CheckCalibration`；定义前 | `App/gray_sensor.h::Gray_CheckCalibration` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-073 / CG1 | `App/gray_sensor.c:110::Gray_Normalize`；定义前 | `App/gray_sensor.h::Gray_Normalize` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-074 / CG1 | `App/gray_sensor.c:143::Gray_ComputeError`；定义前 | `App/gray_sensor.h::Gray_ComputeError` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-075 / CG1 | `App/gray_sensor.c:171::Gray_ResetSequence`；定义前 | `App/gray_sensor.h::Gray_ResetSequence` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-076 / CG1 | `App/gray_sensor.c:182::Gray_Init`；定义前 | `App/gray_sensor.h::Gray_Init` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-077 / CG1 | `App/gray_sensor.c:195::Gray_Update`；定义前 | `App/gray_sensor.h::Gray_Update` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-078 / CG1 | `App/imu_proc.c:46::IMU_GetAngleDeltaX`；定义前 | `App/imu_proc.h::IMU_GetAngleDeltaX` | 纯 getter；职责、返回值、单位及读取上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-079 / CG1 | `App/imu_proc.c:56::IMU_GetState`；定义前 | `App/imu_proc.h::IMU_GetState` | 纯 getter；职责、返回值、单位及读取上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-080 / CG1 | `App/imu_proc.c:66::IMU_ZeroAngleX`；定义前 | `App/imu_proc.h::IMU_ZeroAngleX` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-081 / CG1 | `App/imu_proc.c:76::IMU_Init`；定义前 | `App/imu_proc.h::IMU_Init` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-082 / CG1 | `App/imu_proc.c:88::IMU_UpdateAccelVelocity`；定义前 | `App/imu_proc.h::IMU_UpdateAccelVelocity` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-083 / CG1 | `App/imu_proc.c:110::IMU_UpdateGyroAngle`；定义前 | `App/imu_proc.h::IMU_UpdateGyroAngle` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-084 / CG1 | `App/imu_proc.c:136::IMU_CalibrateAccel`；定义前 | `App/imu_proc.h::IMU_CalibrateAccel` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-085 / CG1 | `App/imu_proc.c:162::IMU_CalibrateGyro`；定义前 | `App/imu_proc.h::IMU_CalibrateGyro` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-086 / CG1 | `App/imu_proc.c:188::IMU_ResetRunState`；定义前 | `App/imu_proc.h::IMU_ResetRunState` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-087 / CG4 | `App/menu.c:67::Menu_ReadKeyAverage`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-088 / CG4 | `App/menu.c:86::Menu_ScanKey`；定义前 | `App/menu.h::Menu_ScanKey` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-089 / CG4 | `App/menu.c:155::Menu_HandleKey`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-090 / CG4 | `App/menu.c:335::Menu_DrawContent`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-091 / CG4 | `App/menu.c:453::Menu_DrawCursor`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-092 / CG4 | `App/menu.c:476::Menu_Update`；定义前 | `App/menu.h::Menu_Update` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-093 / CG2 | `App/motor.c:24::Motor_GetState`；定义前 | `App/motor.h::Motor_GetState` | 纯 getter；职责、返回值、单位及读取上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-094 / CG2 | `App/motor.c:36::Motor_SetTargets`；定义前 | `App/motor.h::Motor_SetTargets` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-095 / CG2 | `App/motor.c:48::Motor_SetBaseSpeed`；定义前 | `App/motor.h::Motor_SetBaseSpeed` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-096 / CG2 | `App/motor.c:59::Motor_SetFanDuty`；定义前 | `App/motor.h::Motor_SetFanDuty` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-097 / CG2 | `App/motor.c:70::Motor_SetFanDutyIdle`；定义前 | `App/motor.h::Motor_SetFanDutyIdle` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-098 / CG2 | `App/motor.c:80::Motor_ResetDistance`；定义前 | `App/motor.h::Motor_ResetDistance` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-099 / CG2 | `App/motor.c:93::Motor_DistributeSpeed`；定义前 | `App/motor.h::Motor_DistributeSpeed` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-100 / CG2 | `App/motor.c:114::Motor_UpdateControl`；定义前 | `App/motor.h::Motor_UpdateControl` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-101 / CG2 | `App/motor.c:152::Motor_ReadEncoder`；定义前 | `App/motor.h::Motor_ReadEncoder` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-102 / CG2 | `App/motor.c:174::Motor_Init`；定义前 | `App/motor.h::Motor_Init` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-103 / CG2 | `App/motor.c:187::Motor_InitEncoder`；定义前 | `App/motor.h::Motor_InitEncoder` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-104 / CG2 | `App/motor.c:199::Motor_ResetRunState`；定义前 | `App/motor.h::Motor_ResetRunState` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-105 / CG2 | `App/motor.c:214::Motor_EmergencyStop`；定义前 | `App/motor.h::Motor_EmergencyStop` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-106 / CG2 | `App/optical_fsm.c:17::OpticalFSM_GetState`；定义前 | `App/optical_fsm.h::OpticalFSM_GetState` | 纯 getter；职责、返回值、单位及读取上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-107 / CG2 | `App/optical_fsm.c:27::OpticalFSM_Reset`；定义前 | `App/optical_fsm.h::OpticalFSM_Reset` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-108 / CG2 | `App/optical_fsm.c:41::OpticalFSM_Update`；定义前 | `App/optical_fsm.h::OpticalFSM_Update` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-109 / CG3 | `App/param.c:132::Param_CalculateCrc16`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-110 / CG3 | `App/param.c:154::Param_CalculateCrcV1`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-111 / CG3 | `App/param.c:166::Param_CalculateCrcV2`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-112 / CG3 | `App/param.c:178::Param_CalculateCrcV3`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-113 / CG3 | `App/param.c:190::Param_ClampFanDuty`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-114 / CG3 | `App/param.c:200::Param_SetOpticalDefaults`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-115 / CG3 | `App/param.c:214::Param_SetDefaults`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-116 / CG3 | `App/param.c:233::Param_LoadBanks`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-117 / CG3 | `App/param.c:250::Param_StoreBanks`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-118 / CG3 | `App/param.c:284::Param_GetDirection`；定义前 | `App/param.h::Param_GetDirection` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-119 / CG3 | `App/param.c:295::Param_ApplyCurrentDirection`；定义前 | `App/param.h::Param_ApplyCurrentDirection` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-120 / CG3 | `App/param.c:314::Param_SetDirection`；定义前 | `App/param.h::Param_SetDirection` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-121 / CG3 | `App/param.c:338::Param_GetFan`；定义前 | `App/param.h::Param_GetFan` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-122 / CG3 | `App/param.c:356::Param_SetFan`；定义前 | `App/param.h::Param_SetFan` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-123 / CG3 | `App/param.c:407::Param_SelectGear`；定义前 | `App/param.h::Param_SelectGear` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-124 / CG3 | `App/param.c:426::Param_WriteMirror`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-125 / CG3 | `App/param.c:447::Param_Load`；定义前 | `App/param.h::Param_Load` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-126 / CG3 | `App/param.c:584::Param_MarkDirty`；定义前 | `App/param.h::Param_MarkDirty` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-127 / CG3 | `App/param.c:596::Param_TryFlushPending`；定义前 | `App/param.h::Param_TryFlushPending` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-128 / CG3 | `App/param.c:625::Param_Save`；定义前 | `App/param.h::Param_Save` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-129 / CG3 | `App/param.c:636::Param_ServiceSave`；定义前 | `App/param.h::Param_ServiceSave` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-130 / CG3 | `App/param.c:648::Param_FlushPending`；定义前 | `App/param.h::Param_FlushPending` | 单调用转发；转发目标、返回语义及调用上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-131 / CG2 | `App/pid.c:45::PID_GetState`；定义前 | `App/pid.h::PID_GetState` | 纯 getter；职责、返回值、单位及读取上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-132 / CG2 | `App/pid.c:57::PID_SetGain`；定义前 | `App/pid.h::PID_SetGain` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-133 / CG2 | `App/pid.c:106::PID_SetTrackGains`；定义前 | `App/pid.h::PID_SetTrackGains` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-134 / CG2 | `App/pid.c:120::PID_SetAngleGains`；定义前 | `App/pid.h::PID_SetAngleGains` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-135 / CG2 | `App/pid.c:137::PID_DirectionStep`；定义前 | `App/pid.h::PID_DirectionStep` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-136 / CG2 | `App/pid.c:178::PID_UpdateTrack`；定义前 | `App/pid.h::PID_UpdateTrack` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-137 / CG2 | `App/pid.c:191::PID_UpdateAngle`；定义前 | `App/pid.h::PID_UpdateAngle` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-138 / CG2 | `App/pid.c:210::PID_UpdateIncrementalLeft`；定义前 | `App/pid.h::PID_UpdateIncrementalLeft` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-139 / CG2 | `App/pid.c:253::PID_UpdateIncrementalRight`；定义前 | `App/pid.h::PID_UpdateIncrementalRight` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-140 / CG2 | `App/pid.c:300::PID_UpdateSpeed`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-141 / CG2 | `App/pid.c:356::PID_UpdateSpeedLeft`；定义前 | `App/pid.h::PID_UpdateSpeedLeft` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-142 / CG2 | `App/pid.c:370::PID_UpdateSpeedRight`；定义前 | `App/pid.h::PID_UpdateSpeedRight` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-143 / CG2 | `App/pid.c:382::PID_ResetSpeed`；定义前 | `App/pid.h::PID_ResetSpeed` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-144 / CG2 | `App/pid.c:397::PID_ResetAll`；定义前 | `App/pid.h::PID_ResetAll` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-145 / CG2 | `App/roundabout.c:84::Roundabout_GetProfile`；定义前 | `App/roundabout.h::Roundabout_GetProfile` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-146 / CG2 | `App/roundabout.c:108::Roundabout_Update`；定义前 | `App/roundabout.h::Roundabout_Update` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-147 / CG2 | `App/roundabout.c:164::Roundabout_JudgePreentryLeft`；定义前 | `App/roundabout.h::Roundabout_JudgePreentryLeft` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-148 / CG2 | `App/roundabout.c:195::Roundabout_JudgePreentryRight`；定义前 | `App/roundabout.h::Roundabout_JudgePreentryRight` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-149 / CG2 | `App/roundabout.c:211::Roundabout_JudgePreentryExit`；定义前 | `App/roundabout.h::Roundabout_JudgePreentryExit` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-150 / CG2 | `App/roundabout.c:229::Roundabout_JudgeEntryLeft`；定义前 | `App/roundabout.h::Roundabout_JudgeEntryLeft` | 休眠空壳；未接入事实、空操作和调用边已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-151 / CG2 | `App/roundabout.c:240::Roundabout_JudgeEntryRight`；定义前 | `App/roundabout.h::Roundabout_JudgeEntryRight` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-152 / CG2 | `App/roundabout.c:260::Roundabout_JudgeAhead`；定义前 | `App/roundabout.h::Roundabout_JudgeAhead` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-153 / CG2 | `App/roundabout.c:274::Roundabout_JudgeEntered`；定义前 | `App/roundabout.h::Roundabout_JudgeEntered` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-154 / CG2 | `App/roundabout.c:288::Roundabout_JudgeEnteredAgain`；定义前 | `App/roundabout.h::Roundabout_JudgeEnteredAgain` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-155 / CG2 | `App/roundabout.c:301::Roundabout_JudgePreExit`；定义前 | `App/roundabout.h::Roundabout_JudgePreExit` | 休眠空壳；未接入事实、空操作和调用边已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-156 / CG2 | `App/roundabout.c:311::Roundabout_JudgeExit`；定义前 | `App/roundabout.h::Roundabout_JudgeExit` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-157 / CG2 | `App/roundabout.c:325::Roundabout_JudgeExited`；定义前 | `App/roundabout.h::Roundabout_JudgeExited` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-158 / CG2 | `App/roundabout.c:342::Roundabout_Judge`；定义前 | `App/roundabout.h::Roundabout_Judge` | 休眠空壳；未接入事实、空操作和调用边已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-159 / CG2 | `App/roundabout.c:352::Roundabout_ResetRunState`；定义前 | `App/roundabout.h::Roundabout_ResetRunState` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-160 / CG1 | `App/track_sensor.c:28::Track_GetState`；定义前 | `App/track_sensor.h::Track_GetState` | 纯 getter；职责、返回值、单位及读取上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-161 / CG1 | `App/track_sensor.c:41::Track_CalculateError`；定义前 | `App/track_sensor.h::Track_CalculateError` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-162 / CG1 | `App/track_sensor.c:58::Track_PublishObservation`；定义前 | `App/track_sensor.h::Track_PublishObservation` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-163 / CG1 | `App/track_sensor.c:73::Track_SetWeights`；定义前 | `App/track_sensor.h::Track_SetWeights` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-164 / CG1 | `App/track_sensor.c:87::Track_UpdateError`；定义前 | `App/track_sensor.h::Track_UpdateError` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-165 / CG1 | `App/track_sensor.c:124::Track_UpdateSymmetry`；定义前 | `App/track_sensor.h::Track_UpdateSymmetry` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-166 / CG1 | `App/track_sensor.c:145::Track_ReadAdc`；定义前 | `App/track_sensor.h::Track_ReadAdc` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-167 / CG1 | `App/track_sensor.c:195::Track_InitAdc`；定义前 | `App/track_sensor.h::Track_InitAdc` | 单调用转发；转发目标、返回语义及调用上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-168 / CG4 | `App/wireless.c:25::Wireless_Adjust`；定义前 | `App/wireless.h::Wireless_Adjust` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-169 / CG1 | `BSP/bsp_adc.c:23::BSP_ADC_Init`；定义前 | `BSP/bsp_adc.h::BSP_ADC_Init` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-170 / CG1 | `BSP/bsp_adc.c:38::BSP_ADC_Read`；定义前 | `BSP/bsp_adc.h::BSP_ADC_Read` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-171 / CG1 | `BSP/bsp_adc.c:59::BSP_ADC_ReadGrayFresh`；定义前 | `BSP/bsp_adc.h::BSP_ADC_ReadGrayFresh` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-172 / CG1 | `BSP/bsp_dl1b.c:43::BSP_DL1B_ReadRegister`；定义前 | 无对应自有声明；不新增接口 | 单调用转发；转发目标、返回语义及调用上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-173 / CG1 | `BSP/bsp_dl1b.c:58::BSP_DL1B_WriteRegister`；定义前 | 无对应自有声明；不新增接口 | 单调用转发；转发目标、返回语义及调用上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-174 / CG1 | `BSP/bsp_dl1b.c:70::BSP_DL1B_Init`；定义前 | `BSP/bsp_dl1b.h::BSP_DL1B_Init` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-175 / CG1 | `BSP/bsp_dl1b.c:126::BSP_DL1B_Update`；定义前 | `BSP/bsp_dl1b.h::BSP_DL1B_Update` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-176 / CG1 | `BSP/bsp_dl1b.c:179::BSP_DL1B_GetDistanceMm`；定义前 | `BSP/bsp_dl1b.h::BSP_DL1B_GetDistanceMm` | 纯 getter；职责、返回值、单位及读取上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-177 / CG2 | `BSP/bsp_encoder.c:14::BSP_Encoder_Init`；定义前 | `BSP/bsp_encoder.h::BSP_Encoder_Init` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-178 / CG2 | `BSP/bsp_encoder.c:27::BSP_Encoder_GetLeft`；定义前 | `BSP/bsp_encoder.h::BSP_Encoder_GetLeft` | 单调用转发；转发目标、返回语义及调用上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-179 / CG2 | `BSP/bsp_encoder.c:37::BSP_Encoder_GetRight`；定义前 | `BSP/bsp_encoder.h::BSP_Encoder_GetRight` | 单调用转发；转发目标、返回语义及调用上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-180 / CG2 | `BSP/bsp_encoder.c:49::BSP_Encoder_ReadClear`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-181 / CG3 | `BSP/bsp_flash.c:22::BSP_Flash_Init`；定义前 | `BSP/bsp_flash.h::BSP_Flash_Init` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-182 / CG3 | `BSP/bsp_flash.c:38::BSP_Flash_Read`；定义前 | `BSP/bsp_flash.h::BSP_Flash_Read` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-183 / CG3 | `BSP/bsp_flash.c:58::BSP_Flash_Write`；定义前 | `BSP/bsp_flash.h::BSP_Flash_Write` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-184 / CG3 | `BSP/bsp_flash.c:103::BSP_Flash_LogErase`；定义前 | `BSP/bsp_flash.h::BSP_Flash_LogErase` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-185 / CG3 | `BSP/bsp_flash.c:128::BSP_Flash_LogWrite`；定义前 | `BSP/bsp_flash.h::BSP_Flash_LogWrite` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-186 / CG3 | `BSP/bsp_flash.c:175::BSP_Flash_LogRead`；定义前 | `BSP/bsp_flash.h::BSP_Flash_LogRead` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-187 / CG1 | `BSP/bsp_gray.c:26::BSP_Gray_SelectChannel`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-188 / CG1 | `BSP/bsp_gray.c:39::BSP_Gray_ResetSequence`；定义前 | `BSP/bsp_gray.h::BSP_Gray_ResetSequence` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-189 / CG1 | `BSP/bsp_gray.c:49::BSP_Gray_Init`；定义前 | `BSP/bsp_gray.h::BSP_Gray_Init` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-190 / CG1 | `BSP/bsp_gray.c:64::BSP_Gray_ReadFrame`；定义前 | `BSP/bsp_gray.h::BSP_Gray_ReadFrame` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-191 / CG1 | `BSP/bsp_imu660rb.c:38::BSP_IMU660RB_SetChipSelect`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-192 / CG1 | `BSP/bsp_imu660rb.c:51::BSP_IMU660RB_WriteRegister`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-193 / CG1 | `BSP/bsp_imu660rb.c:67::BSP_IMU660RB_ReadRegister`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-194 / CG1 | `BSP/bsp_imu660rb.c:87::BSP_IMU660RB_ReadRegisters`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-195 / CG1 | `BSP/bsp_imu660rb.c:103::BSP_IMU660RB_CheckIdentity`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-196 / CG1 | `BSP/bsp_imu660rb.c:126::BSP_IMU660RB_Init`；定义前 | `BSP/bsp_imu660rb.h::BSP_IMU660RB_Init` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-197 / CG1 | `BSP/bsp_imu660rb.c:157::BSP_IMU660RB_GetAcc`；定义前 | `BSP/bsp_imu660rb.h::BSP_IMU660RB_GetAcc` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-198 / CG1 | `BSP/bsp_imu660rb.c:174::BSP_IMU660RB_GetGyro`；定义前 | `BSP/bsp_imu660rb.h::BSP_IMU660RB_GetGyro` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-199 / CG4 | `BSP/bsp_key.c:22::BSP_Key_RequireRelease`；定义前 | `BSP/bsp_key.h::BSP_Key_RequireRelease` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-200 / CG4 | `BSP/bsp_key.c:34::BSP_Key_Init`；定义前 | `BSP/bsp_key.h::BSP_Key_Init` | 单调用转发；转发目标、返回语义及调用上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-201 / CG4 | `BSP/bsp_key.c:45::BSP_Key_PollEvent`；定义前 | `BSP/bsp_key.h::BSP_Key_PollEvent` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-202 / CG4 | `BSP/bsp_key.c:89::BSP_Key_StartPressed`；定义前 | `BSP/bsp_key.h::BSP_Key_StartPressed` | 单调用转发；转发目标、返回语义及调用上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-203 / CG4 | `BSP/bsp_lcd.c:29::BSP_LCD_Init`；定义前 | `BSP/bsp_lcd.h::BSP_LCD_Init` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-204 / CG4 | `BSP/bsp_lcd.c:41::BSP_LCD_Clear`；定义前 | `BSP/bsp_lcd.h::BSP_LCD_Clear` | 单调用转发；转发目标、返回语义及调用上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-205 / CG4 | `BSP/bsp_lcd.c:54::BSP_LCD_SetBacklight`；定义前 | `BSP/bsp_lcd.h::BSP_LCD_SetBacklight` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-206 / CG4 | `BSP/bsp_lcd.c:79::BSP_LCD_ShowChar`；定义前 | `BSP/bsp_lcd.h::BSP_LCD_ShowChar` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-207 / CG4 | `BSP/bsp_lcd.c:136::BSP_LCD_ShowString`；定义前 | `BSP/bsp_lcd.h::BSP_LCD_ShowString` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-208 / CG4 | `BSP/bsp_lcd.c:166::BSP_LCD_ShowInt`；定义前 | `BSP/bsp_lcd.h::BSP_LCD_ShowInt` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-209 / CG4 | `BSP/bsp_lcd.c:214::BSP_LCD_ShowFloat`；定义前 | `BSP/bsp_lcd.h::BSP_LCD_ShowFloat` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-210 / CG2 | `BSP/bsp_pwm.c:27::BSP_PWM_Init`；定义前 | `BSP/bsp_pwm.h::BSP_PWM_Init` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-211 / CG2 | `BSP/bsp_pwm.c:41::BSP_PWM_SetDuty`；定义前 | `BSP/bsp_pwm.h::BSP_PWM_SetDuty` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-212 / CG1 | `BSP/bsp_sampler.c:31::BSP_Sampler_Init`；定义前 | `BSP/bsp_sampler.h::BSP_Sampler_Init` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-213 / CG1 | `BSP/bsp_sampler.c:52::BSP_Sampler_Start`；定义前 | `BSP/bsp_sampler.h::BSP_Sampler_Start` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-214 / CG1 | `BSP/bsp_sampler.c:66::BSP_Sampler_Stop`；定义前 | `BSP/bsp_sampler.h::BSP_Sampler_Stop` | 单调用转发；转发目标、返回语义及调用上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-215 / CG1 | `BSP/bsp_sampler.c:77::BSP_Sampler_GetTick`；定义前 | `BSP/bsp_sampler.h::BSP_Sampler_GetTick` | 纯 getter；职责、返回值、单位及读取上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-216 / CG1 | `BSP/bsp_sampler.c:87::BSP_Sampler_GetEncL_1ms`；定义前 | `BSP/bsp_sampler.h::BSP_Sampler_GetEncL_1ms` | 纯 getter；职责、返回值、单位及读取上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-217 / CG1 | `BSP/bsp_sampler.c:97::BSP_Sampler_GetEncR_1ms`；定义前 | `BSP/bsp_sampler.h::BSP_Sampler_GetEncR_1ms` | 纯 getter；职责、返回值、单位及读取上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-218 / CG1 | `BSP/bsp_sampler.c:110::BSP_Sampler_GetGyroRaw`；定义前 | `BSP/bsp_sampler.h::BSP_Sampler_GetGyroRaw` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-219 / CG1 | `BSP/bsp_sampler.c:125::BSP_Sampler_GetAccRaw`；定义前 | `BSP/bsp_sampler.h::BSP_Sampler_GetAccRaw` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-220 / CG1 | `BSP/bsp_sampler.c:138::BSP_Sampler_ConsumeEncL`；定义前 | `BSP/bsp_sampler.h::BSP_Sampler_ConsumeEncL` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-221 / CG1 | `BSP/bsp_sampler.c:151::BSP_Sampler_ConsumeEncR`；定义前 | `BSP/bsp_sampler.h::BSP_Sampler_ConsumeEncR` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-222 / CG1 | `BSP/bsp_sampler.c:162::TIM15_IRQHandler`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-223 / CG1 | `BSP/bsp_time.c:14::BSP_Time_Init`；定义前 | `BSP/bsp_time.h::BSP_Time_Init` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-224 / CG1 | `BSP/bsp_time.c:28::BSP_Time_GetCycles`；定义前 | `BSP/bsp_time.h::BSP_Time_GetCycles` | 纯 getter；职责、返回值、单位及读取上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-225 / CG1 | `BSP/bsp_time.c:39::BSP_Time_EnterCritical`；定义前 | `BSP/bsp_time.h::BSP_Time_EnterCritical` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-226 / CG1 | `BSP/bsp_time.c:52::BSP_Time_ExitCritical`；定义前 | `BSP/bsp_time.h::BSP_Time_ExitCritical` | 单调用转发；转发目标、返回语义及调用上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-227 / CG1 | `BSP/bsp_time.c:61::BSP_Time_MemoryBarrier`；定义前 | `BSP/bsp_time.h::BSP_Time_MemoryBarrier` | 单调用转发；转发目标、返回语义及调用上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-228 / CG4 | `BSP/bsp_uart.c:23::BSP_UART_Init`；定义前 | `BSP/bsp_uart.h::BSP_UART_Init` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-229 / CG4 | `BSP/bsp_uart.c:38::BSP_UART_Write`；定义前 | `BSP/bsp_uart.h::BSP_UART_Write` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-230 / CG4 | `BSP/bsp_uart.c:59::BSP_UART_WriteString`；定义前 | `BSP/bsp_uart.h::BSP_UART_WriteString` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-231 / CG4 | `BSP/bsp_uart.c:76::BSP_UART_Read`；定义前 | `BSP/bsp_uart.h::BSP_UART_Read` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-232 / CG4 | `BSP/bsp_uart.c:99::BSP_UART_RxAvailable`；定义前 | `BSP/bsp_uart.h::BSP_UART_RxAvailable` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-233 / CG4 | `BSP/bsp_uart.c:109::BSP_UART_RxFlush`；定义前 | `BSP/bsp_uart.h::BSP_UART_RxFlush` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-234 / CG4 | `BSP/bsp_uart.c:119::BSP_UART_PushRx`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-235 / CG4 | `BSP/bsp_uart.c:136::HAL_UART_RxCpltCallback`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-236 / CG4 | `BSP/bsp_uart.c:154::HAL_UART_ErrorCallback`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-237 / CG4 | `BSP/bsp_uart.c:175::fputc`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-238 / CG3 | `BSP/bsp_w25q64.c:41::BSP_W25Q64_Select`；定义前 | 无对应自有声明；不新增接口 | 单调用转发；转发目标、返回语义及调用上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-239 / CG3 | `BSP/bsp_w25q64.c:50::BSP_W25Q64_Deselect`；定义前 | 无对应自有声明；不新增接口 | 单调用转发；转发目标、返回语义及调用上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-240 / CG3 | `BSP/bsp_w25q64.c:63::BSP_W25Q64_ReadStatus1`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-241 / CG3 | `BSP/bsp_w25q64.c:91::BSP_W25Q64_WaitIdle`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-242 / CG3 | `BSP/bsp_w25q64.c:120::BSP_W25Q64_EnableWrite`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-243 / CG3 | `BSP/bsp_w25q64.c:137::BSP_W25Q64_Init`；定义前 | `BSP/bsp_w25q64.h::BSP_W25Q64_Init` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-244 / CG3 | `BSP/bsp_w25q64.c:221::BSP_W25Q64_Read`；定义前 | `BSP/bsp_w25q64.h::BSP_W25Q64_Read` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-245 / CG3 | `BSP/bsp_w25q64.c:258::BSP_W25Q64_WritePage`；定义前 | `BSP/bsp_w25q64.h::BSP_W25Q64_WritePage` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-246 / CG3 | `BSP/bsp_w25q64.c:300::BSP_W25Q64_EraseSector`；定义前 | `BSP/bsp_w25q64.h::BSP_W25Q64_EraseSector` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-247 / CG3 | `BSP/bsp_w25q64.c:337::BSP_W25Q64_EraseBlock64K`；定义前 | `BSP/bsp_w25q64.h::BSP_W25Q64_EraseBlock64K` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-248 / CG3 | `BSP/bsp_w25q64.c:374::BSP_W25Q64_EraseChip`；定义前 | `BSP/bsp_w25q64.h::BSP_W25Q64_EraseChip` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-249 / CG4 | `BSP/bsp_wdt.c:31::BSP_WDT_Init`；定义前 | `BSP/bsp_wdt.h::BSP_WDT_Init` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-250 / CG4 | `BSP/bsp_wdt.c:58::BSP_WDT_Feed`；定义前 | `BSP/bsp_wdt.h::BSP_WDT_Feed` | 单调用转发；转发目标、返回语义及调用上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-251 / CG4 | `BSP/lcd/lcd.c:67::BSP_LCDIO_Init`；定义前 | `BSP/lcd/lcd.h::BSP_LCDIO_Init` | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-252 / CG4 | `BSP/lcd/lcd.c:95::BSP_LCDIO_SetReset`；定义前 | 无对应自有声明；不新增接口 | 单调用转发；转发目标、返回语义及调用上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-253 / CG4 | `BSP/lcd/lcd.c:105::BSP_LCDIO_ClearReset`；定义前 | 无对应自有声明；不新增接口 | 单调用转发；转发目标、返回语义及调用上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-254 / CG4 | `BSP/lcd/lcd.c:115::BSP_LCDIO_SetReset`；定义前 | 无对应自有声明；不新增接口 | 休眠空壳；未接入事实、空操作和调用边已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-255 / CG4 | `BSP/lcd/lcd.c:125::BSP_LCDIO_ClearReset`；定义前 | 无对应自有声明；不新增接口 | 休眠空壳；未接入事实、空操作和调用边已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-256 / CG4 | `BSP/lcd/lcd.c:137::BSP_LCDIO_InitBus`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-257 / CG4 | `BSP/lcd/lcd.c:152::BSP_LCDIO_GetTick`；定义前 | 无对应自有声明；不新增接口 | 单调用转发；转发目标、返回语义及调用上下文已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-258 / CG4 | `BSP/lcd/lcd.c:166::BSP_LCDIO_WriteRegister`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-259 / CG4 | `BSP/lcd/lcd.c:189::BSP_LCDIO_ReadRegister`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-260 / CG4 | `BSP/lcd/lcd.c:209::BSP_LCDIO_SendData`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |
| CF-261 / CG4 | `BSP/lcd/lcd.c:226::BSP_LCDIO_ReceiveData`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；职责、参数/返回、异常、时序及适用阶段已核对 | 已覆盖 | 格式通过 | 内容通过（2026-10-17） | token/预处理通过 |

## CM：模块、字段、状态与格式

每行审查文件头、数据流/调用关系/调度、所有字段/枚举、全局/静态/静态局部状态、阶段分块和
来源例外。H 行重点检查摘要不与 C 矛盾；没有该类对象时逐格写可核对的“不适用及原因”。
Core 只检查已存在的非空用户区，生成区与原厂版权不变。

| ID / 组 | 文件 | 文件/流程说明 | 字段/枚举 | 状态对象 | 版权/来源/排除区登记 | 格式与内容结论 |
|---|---|---|---|---|---|---|
| CM-001 / CG4 | `App/app.c` | C 文件头、调用/调度关系及复杂流程阶段已核对 | 无自有字段/枚举，已核对 | 文件级状态的用途、单位/范围及读写上下文已核对 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-002 / CG4 | `App/app.h` | H 用途摘要、关键约束及 C 实现定位已核对 | 自有字段/枚举成员已逐项说明 | 仅声明接口/常量，不定义运行状态 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-003 / CG4 | `App/app_config.h` | H 用途摘要、关键约束及 C 实现定位已核对 | 无自有字段/枚举，已核对 | 仅声明接口/常量，不定义运行状态 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-004 / CG2 | `App/control.c` | C 文件头、调用/调度关系及复杂流程阶段已核对 | 无自有字段/枚举，已核对 | 文件级状态的用途、单位/范围及读写上下文已核对 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-005 / CG2 | `App/control.h` | H 用途摘要、关键约束及 C 实现定位已核对 | 自有字段/枚举成员已逐项说明 | 仅声明接口/常量，不定义运行状态 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-006 / CG4 | `App/dashboard.c` | C 文件头、调用/调度关系及复杂流程阶段已核对 | 无自有字段/枚举，已核对 | 文件级状态的用途、单位/范围及读写上下文已核对 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-007 / CG4 | `App/dashboard.h` | H 用途摘要、关键约束及 C 实现定位已核对 | 无自有字段/枚举，已核对 | 仅声明接口/常量，不定义运行状态 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-008 / CG3 | `App/datalog.c` | C 文件头、调用/调度关系及复杂流程阶段已核对 | 无自有字段/枚举，已核对 | 文件级状态的用途、单位/范围及读写上下文已核对 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-009 / CG3 | `App/datalog.h` | H 用途摘要、关键约束及 C 实现定位已核对 | 自有字段/枚举成员已逐项说明 | 仅声明接口/常量，不定义运行状态 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-010 / CG2 | `App/electromagnetic_fsm.c` | C 文件头、调用/调度关系及复杂流程阶段已核对 | 无自有字段/枚举，已核对 | 文件级状态的用途、单位/范围及读写上下文已核对 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-011 / CG2 | `App/electromagnetic_fsm.h` | H 用途摘要、关键约束及 C 实现定位已核对 | 自有字段/枚举成员已逐项说明 | 仅声明接口/常量，不定义运行状态 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-012 / CG2 | `App/element.c` | C 文件头、调用/调度关系及复杂流程阶段已核对 | 无自有字段/枚举，已核对 | 文件级状态的用途、单位/范围及读写上下文已核对 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-013 / CG2 | `App/element.h` | H 用途摘要、关键约束及 C 实现定位已核对 | 无自有字段/枚举，已核对 | 仅声明接口/常量，不定义运行状态 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-014 / CG2 | `App/filter.c` | C 文件头、调用/调度关系及复杂流程阶段已核对 | 无自有字段/枚举，已核对 | 文件级状态的用途、单位/范围及读写上下文已核对 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-015 / CG2 | `App/filter.h` | H 用途摘要、关键约束及 C 实现定位已核对 | 自有字段/枚举成员已逐项说明 | 仅声明接口/常量，不定义运行状态 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-016 / CG1 | `App/gray_calibration.h` | H 用途摘要、关键约束及 C 实现定位已核对 | 无自有字段/枚举，已核对 | 仅声明接口/常量，不定义运行状态 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-017 / CG1 | `App/gray_sensor.c` | C 文件头、调用/调度关系及复杂流程阶段已核对 | 无自有字段/枚举，已核对 | 文件级状态的用途、单位/范围及读写上下文已核对 | E-02 原来源与许可指引区块逐字保持 | 格式与内容通过（2026-10-17） |
| CM-018 / CG1 | `App/gray_sensor.h` | H 用途摘要、关键约束及 C 实现定位已核对 | 自有字段/枚举成员已逐项说明 | 仅声明接口/常量，不定义运行状态 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-019 / CG1 | `App/imu_proc.c` | C 文件头、调用/调度关系及复杂流程阶段已核对 | 无自有字段/枚举，已核对 | 文件级状态的用途、单位/范围及读写上下文已核对 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-020 / CG1 | `App/imu_proc.h` | H 用途摘要、关键约束及 C 实现定位已核对 | 自有字段/枚举成员已逐项说明 | 仅声明接口/常量，不定义运行状态 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-021 / CG4 | `App/menu.c` | C 文件头、调用/调度关系及复杂流程阶段已核对 | 无自有字段/枚举，已核对 | 文件级状态的用途、单位/范围及读写上下文已核对 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-022 / CG4 | `App/menu.h` | H 用途摘要、关键约束及 C 实现定位已核对 | 无自有字段/枚举，已核对 | 仅声明接口/常量，不定义运行状态 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-023 / CG2 | `App/motor.c` | C 文件头、调用/调度关系及复杂流程阶段已核对 | 无自有字段/枚举，已核对 | 文件级状态的用途、单位/范围及读写上下文已核对 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-024 / CG2 | `App/motor.h` | H 用途摘要、关键约束及 C 实现定位已核对 | 自有字段/枚举成员已逐项说明 | 仅声明接口/常量，不定义运行状态 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-025 / CG2 | `App/optical_fsm.c` | C 文件头、调用/调度关系及复杂流程阶段已核对 | 无自有字段/枚举，已核对 | 文件级状态的用途、单位/范围及读写上下文已核对 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-026 / CG2 | `App/optical_fsm.h` | H 用途摘要、关键约束及 C 实现定位已核对 | 自有字段/枚举成员已逐项说明 | 仅声明接口/常量，不定义运行状态 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-027 / CG3 | `App/param.c` | C 文件头、调用/调度关系及复杂流程阶段已核对 | 自有字段/枚举成员已逐项说明 | 文件级状态的用途、单位/范围及读写上下文已核对 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-028 / CG3 | `App/param.h` | H 用途摘要、关键约束及 C 实现定位已核对 | 自有字段/枚举成员已逐项说明 | 仅声明接口/常量，不定义运行状态 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-029 / CG2 | `App/pid.c` | C 文件头、调用/调度关系及复杂流程阶段已核对 | 自有字段/枚举成员已逐项说明 | 文件级状态的用途、单位/范围及读写上下文已核对 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-030 / CG2 | `App/pid.h` | H 用途摘要、关键约束及 C 实现定位已核对 | 自有字段/枚举成员已逐项说明 | 仅声明接口/常量，不定义运行状态 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-031 / CG2 | `App/roundabout.c` | C 文件头、调用/调度关系及复杂流程阶段已核对 | 自有字段/枚举成员已逐项说明 | 文件级状态的用途、单位/范围及读写上下文已核对 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-032 / CG2 | `App/roundabout.h` | H 用途摘要、关键约束及 C 实现定位已核对 | 无自有字段/枚举，已核对 | 仅声明接口/常量，不定义运行状态 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-033 / CG1 | `App/track_sensor.c` | C 文件头、调用/调度关系及复杂流程阶段已核对 | 无自有字段/枚举，已核对 | 文件级状态的用途、单位/范围及读写上下文已核对 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-034 / CG1 | `App/track_sensor.h` | H 用途摘要、关键约束及 C 实现定位已核对 | 自有字段/枚举成员已逐项说明 | 仅声明接口/常量，不定义运行状态 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-035 / CG4 | `App/wireless.c` | C 文件头、调用/调度关系及复杂流程阶段已核对 | 无自有字段/枚举，已核对 | 文件级状态的用途、单位/范围及读写上下文已核对 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-036 / CG4 | `App/wireless.h` | H 用途摘要、关键约束及 C 实现定位已核对 | 无自有字段/枚举，已核对 | 仅声明接口/常量，不定义运行状态 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-037 / CG4 | `BSP/bsp.h` | H 用途摘要、关键约束及 C 实现定位已核对 | 无自有字段/枚举，已核对 | 仅声明接口/常量，不定义运行状态 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-038 / CG1 | `BSP/bsp_adc.c` | C 文件头、调用/调度关系及复杂流程阶段已核对 | 无自有字段/枚举，已核对 | 文件级状态的用途、单位/范围及读写上下文已核对 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-039 / CG1 | `BSP/bsp_adc.h` | H 用途摘要、关键约束及 C 实现定位已核对 | 自有字段/枚举成员已逐项说明 | 仅声明接口/常量，不定义运行状态 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-040 / CG1 | `BSP/bsp_dl1b.c` | C 文件头、调用/调度关系及复杂流程阶段已核对 | 无自有字段/枚举，已核对 | 文件级状态的用途、单位/范围及读写上下文已核对 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-041 / CG1 | `BSP/bsp_dl1b.h` | H 用途摘要、关键约束及 C 实现定位已核对 | 无自有字段/枚举，已核对 | 仅声明接口/常量，不定义运行状态 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-042 / CG2 | `BSP/bsp_encoder.c` | C 文件头、调用/调度关系及复杂流程阶段已核对 | 无自有字段/枚举，已核对 | 文件级状态的用途、单位/范围及读写上下文已核对 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-043 / CG2 | `BSP/bsp_encoder.h` | H 用途摘要、关键约束及 C 实现定位已核对 | 无自有字段/枚举，已核对 | 仅声明接口/常量，不定义运行状态 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-044 / CG3 | `BSP/bsp_flash.c` | C 文件头、调用/调度关系及复杂流程阶段已核对 | 无自有字段/枚举，已核对 | 文件级状态的用途、单位/范围及读写上下文已核对 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-045 / CG3 | `BSP/bsp_flash.h` | H 用途摘要、关键约束及 C 实现定位已核对 | 无自有字段/枚举，已核对 | 仅声明接口/常量，不定义运行状态 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-046 / CG1 | `BSP/bsp_gray.c` | C 文件头、调用/调度关系及复杂流程阶段已核对 | 无自有字段/枚举，已核对 | 文件级状态的用途、单位/范围及读写上下文已核对 | E-01 原来源、版权、许可和 SHA 区块逐字保持 | 格式与内容通过（2026-10-17） |
| CM-047 / CG1 | `BSP/bsp_gray.h` | H 用途摘要、关键约束及 C 实现定位已核对 | 自有字段/枚举成员已逐项说明 | 仅声明接口/常量，不定义运行状态 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-048 / CG1 | `BSP/bsp_imu660rb.c` | C 文件头、调用/调度关系及复杂流程阶段已核对 | 无自有字段/枚举，已核对 | 文件级状态的用途、单位/范围及读写上下文已核对 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-049 / CG1 | `BSP/bsp_imu660rb.h` | H 用途摘要、关键约束及 C 实现定位已核对 | 无自有字段/枚举，已核对 | 仅声明接口/常量，不定义运行状态 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-050 / CG4 | `BSP/bsp_key.c` | C 文件头、调用/调度关系及复杂流程阶段已核对 | 无自有字段/枚举，已核对 | 文件级状态的用途、单位/范围及读写上下文已核对 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-051 / CG4 | `BSP/bsp_key.h` | H 用途摘要、关键约束及 C 实现定位已核对 | 自有字段/枚举成员已逐项说明 | 仅声明接口/常量，不定义运行状态 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-052 / CG4 | `BSP/bsp_lcd.c` | C 文件头、调用/调度关系及复杂流程阶段已核对 | 无自有字段/枚举，已核对 | 文件级状态的用途、单位/范围及读写上下文已核对 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-053 / CG4 | `BSP/bsp_lcd.h` | H 用途摘要、关键约束及 C 实现定位已核对 | 无自有字段/枚举，已核对 | 仅声明接口/常量，不定义运行状态 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-054 / CG2 | `BSP/bsp_pwm.c` | C 文件头、调用/调度关系及复杂流程阶段已核对 | 无自有字段/枚举，已核对 | 文件级状态的用途、单位/范围及读写上下文已核对 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-055 / CG2 | `BSP/bsp_pwm.h` | H 用途摘要、关键约束及 C 实现定位已核对 | 自有字段/枚举成员已逐项说明 | 仅声明接口/常量，不定义运行状态 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-056 / CG1 | `BSP/bsp_sampler.c` | C 文件头、调用/调度关系及复杂流程阶段已核对 | 无自有字段/枚举，已核对 | 文件级状态的用途、单位/范围及读写上下文已核对 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-057 / CG1 | `BSP/bsp_sampler.h` | H 用途摘要、关键约束及 C 实现定位已核对 | 无自有字段/枚举，已核对 | 仅声明接口/常量，不定义运行状态 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-058 / CG1 | `BSP/bsp_time.c` | C 文件头、调用/调度关系及复杂流程阶段已核对 | 无自有字段/枚举，已核对 | 文件级状态的用途、单位/范围及读写上下文已核对 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-059 / CG1 | `BSP/bsp_time.h` | H 用途摘要、关键约束及 C 实现定位已核对 | 无自有字段/枚举，已核对 | 仅声明接口/常量，不定义运行状态 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-060 / CG4 | `BSP/bsp_uart.c` | C 文件头、调用/调度关系及复杂流程阶段已核对 | 无自有字段/枚举，已核对 | 文件级状态的用途、单位/范围及读写上下文已核对 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-061 / CG4 | `BSP/bsp_uart.h` | H 用途摘要、关键约束及 C 实现定位已核对 | 无自有字段/枚举，已核对 | 仅声明接口/常量，不定义运行状态 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-062 / CG3 | `BSP/bsp_w25q64.c` | C 文件头、调用/调度关系及复杂流程阶段已核对 | 无自有字段/枚举，已核对 | 文件级状态的用途、单位/范围及读写上下文已核对 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-063 / CG3 | `BSP/bsp_w25q64.h` | H 用途摘要、关键约束及 C 实现定位已核对 | 无自有字段/枚举，已核对 | 仅声明接口/常量，不定义运行状态 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-064 / CG4 | `BSP/bsp_wdt.c` | C 文件头、调用/调度关系及复杂流程阶段已核对 | 无自有字段/枚举，已核对 | 文件级状态的用途、单位/范围及读写上下文已核对 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-065 / CG4 | `BSP/bsp_wdt.h` | H 用途摘要、关键约束及 C 实现定位已核对 | 无自有字段/枚举，已核对 | 仅声明接口/常量，不定义运行状态 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-066 / CG4 | `BSP/lcd/lcd.c` | C 文件头、调用/调度关系及复杂流程阶段已核对 | 无自有字段/枚举，已核对 | 文件级状态的用途、单位/范围及读写上下文已核对 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-067 / CG4 | `BSP/lcd/lcd.h` | H 用途摘要、关键约束及 C 实现定位已核对 | 无自有字段/枚举，已核对 | 仅声明接口/常量，不定义运行状态 | 无新增例外；旧模板和装饰注释已按事实合并 | 格式与内容通过（2026-10-17） |
| CM-068 / CG4 | `Core/Inc/adc.h` | USER CODE 区逐块核对，无适用自维护定义 | 无自有字段/枚举 | 无自有运行状态 | 生成区、USER CODE 标记及厂商版权原文保持 | 范围与内容通过（2026-10-17） |
| CM-069 / CG4 | `Core/Inc/dma.h` | USER CODE 区逐块核对，无适用自维护定义 | 无自有字段/枚举 | 无自有运行状态 | 生成区、USER CODE 标记及厂商版权原文保持 | 范围与内容通过（2026-10-17） |
| CM-070 / CG4 | `Core/Inc/gpio.h` | USER CODE 区逐块核对，无适用自维护定义 | 无自有字段/枚举 | 无自有运行状态 | 生成区、USER CODE 标记及厂商版权原文保持 | 范围与内容通过（2026-10-17） |
| CM-071 / CG4 | `Core/Inc/i2c.h` | USER CODE 区逐块核对，无适用自维护定义 | 无自有字段/枚举 | 无自有运行状态 | 生成区、USER CODE 标记及厂商版权原文保持 | 范围与内容通过（2026-10-17） |
| CM-072 / CG4 | `Core/Inc/main.h` | USER CODE 区逐块核对，无适用自维护定义 | 无自有字段/枚举 | 无自有运行状态 | 生成区、USER CODE 标记及厂商版权原文保持 | 范围与内容通过（2026-10-17） |
| CM-073 / CG4 | `Core/Inc/spi.h` | USER CODE 区逐块核对，无适用自维护定义 | 无自有字段/枚举 | 无自有运行状态 | 生成区、USER CODE 标记及厂商版权原文保持 | 范围与内容通过（2026-10-17） |
| CM-074 / CG4 | `Core/Inc/stm32h7xx_hal_conf.h` | USER CODE 区逐块核对，无适用自维护定义 | 无自有字段/枚举 | 无自有运行状态 | 生成区、USER CODE 标记及厂商版权原文保持 | 范围与内容通过（2026-10-17） |
| CM-075 / CG4 | `Core/Inc/stm32h7xx_it.h` | USER CODE 区逐块核对，无适用自维护定义 | 无自有字段/枚举 | 无自有运行状态 | 生成区、USER CODE 标记及厂商版权原文保持 | 范围与内容通过（2026-10-17） |
| CM-076 / CG4 | `Core/Inc/tim.h` | USER CODE 区逐块核对，无适用自维护定义 | 无自有字段/枚举 | 无自有运行状态 | 生成区、USER CODE 标记及厂商版权原文保持 | 范围与内容通过（2026-10-17） |
| CM-077 / CG4 | `Core/Inc/usart.h` | USER CODE 区逐块核对，无适用自维护定义 | 无自有字段/枚举 | 无自有运行状态 | 生成区、USER CODE 标记及厂商版权原文保持 | 范围与内容通过（2026-10-17） |
| CM-078 / CG4 | `Core/Src/adc.c` | USER CODE 区逐块核对，无适用自维护定义 | 无自有字段/枚举 | 无自有运行状态 | 生成区、USER CODE 标记及厂商版权原文保持 | 范围与内容通过（2026-10-17） |
| CM-079 / CG4 | `Core/Src/dma.c` | USER CODE 区逐块核对，无适用自维护定义 | 无自有字段/枚举 | 无自有运行状态 | 生成区、USER CODE 标记及厂商版权原文保持 | 范围与内容通过（2026-10-17） |
| CM-080 / CG4 | `Core/Src/gpio.c` | USER CODE 区逐块核对，无适用自维护定义 | 无自有字段/枚举 | 无自有运行状态 | 生成区、USER CODE 标记及厂商版权原文保持 | 范围与内容通过（2026-10-17） |
| CM-081 / CG4 | `Core/Src/i2c.c` | USER CODE 区逐块核对，无适用自维护定义 | 无自有字段/枚举 | 无自有运行状态 | 生成区、USER CODE 标记及厂商版权原文保持 | 范围与内容通过（2026-10-17） |
| CM-082 / CG4 | `Core/Src/main.c` | USER CODE 区逐块核对，无适用自维护定义 | 无自有字段/枚举 | 无自有运行状态 | 生成区、USER CODE 标记及厂商版权原文保持 | 范围与内容通过（2026-10-17） |
| CM-083 / CG4 | `Core/Src/spi.c` | USER CODE 区逐块核对，无适用自维护定义 | 无自有字段/枚举 | 无自有运行状态 | 生成区、USER CODE 标记及厂商版权原文保持 | 范围与内容通过（2026-10-17） |
| CM-084 / CG4 | `Core/Src/stm32h7xx_hal_msp.c` | USER CODE 区逐块核对，无适用自维护定义 | 无自有字段/枚举 | 无自有运行状态 | 生成区、USER CODE 标记及厂商版权原文保持 | 范围与内容通过（2026-10-17） |
| CM-085 / CG4 | `Core/Src/stm32h7xx_it.c` | USER CODE 区逐块核对，无适用自维护定义 | 无自有字段/枚举 | 无自有运行状态 | 生成区、USER CODE 标记及厂商版权原文保持 | 范围与内容通过（2026-10-17） |
| CM-086 / CG4 | `Core/Src/system_stm32h7xx.c` | USER CODE 区逐块核对，无适用自维护定义 | 无自有字段/枚举 | 无自有运行状态 | 生成区、USER CODE 标记及厂商版权原文保持 | 范围与内容通过（2026-10-17） |
| CM-087 / CG4 | `Core/Src/tim.c` | USER CODE 区逐块核对，无适用自维护定义 | 无自有字段/枚举 | 无自有运行状态 | 生成区、USER CODE 标记及厂商版权原文保持 | 范围与内容通过（2026-10-17） |
| CM-088 / CG4 | `Core/Src/usart.c` | USER CODE 区逐块核对，无适用自维护定义 | 无自有字段/枚举 | 无自有运行状态 | 生成区、USER CODE 标记及厂商版权原文保持 | 范围与内容通过（2026-10-17） |

## 已确认来源例外

| 编号 | 基线区间 | 保留原因 | 约束 |
|---|---|---|---|
| E-01 | `BSP/bsp_gray.c:6–14` | 龙邱来源、作者、版权、许可、SHA 与适配说明原文 | 原块逐字保留；只豁免其原有格式和超长来源文本，不扩展到其他注释。 |
| E-02 | `App/gray_sensor.c:6–8` | 灰度采集来源与许可指引原文 | 原块逐字保留；不把后续算法解释划入此区。 |

Core 厂商 Header 中的版权和 USER CODE 标记保留；具体区间在各 CM 行随实施证据登记，
未登记前不能以“厂商头”豁免其他自有说明。其余来自旧移植的出处事实仍保留，可独立成块，
不因此豁免新函数说明。

## 证据与状态

- 261 个定义位置均已核对紧邻 C Doxygen、实际参数/返回、调用上下文、异常和共享状态；
  复杂函数另核对对应处理阶段注释。
- 67 个自维护 C/H 的文件、字段/枚举和状态说明已关闭；21 个 Core 文件仅核对既有
  USER CODE 区，未修改生成区。
- 格式扫描、人工内容复核以及相对 `f3c9baa` 的非注释 token/预处理比较均通过；构建和
  回归结果见 [validation.md](validation.md)。
- 模板与成品见 [comment-examples.md](comment-examples.md)，验收方法见
  [validation.md](validation.md) 注释专项 CA/CDOC 项。

## R08 更正记录（2026-10-23）

2026-10-17 的历史内容通过结论保留供追溯；其五模块相关 CF/CM 当前结论以本节和
[逐项证据](comment-evidence.json) 为准，不继续把历史检查当作当前内容已通过的充分依据。
本轮覆盖 bsp_dl1b、bsp_adc、bsp_key、bsp_pwm、pid 的完整 C/H。
除报告六处外，纠正 DL1B H 显示/TIM7、ADC H rank 基数及错误码、PWM H TIM1/TIM8
及 ARR、PID 角度伪限幅与 PI 历史顺序等同类错误，清除这些模块中的模板式流程说明。
每个真实函数及模块的事实在 JSON items 中按模块::符号登记，C/H 各绑定摘要，四项结论分别记录。
本轮十文件66条对象记录，不代表重新完成全项目复审；其他模块随变更补齐。
漏检原因及后续操作见 [持续复核流程](comment-review-workflow.md)。

## Phase 17 G1 目录映射

原表的App平铺路径作为历史记录保留；现行路径逐项见[迁移清单](../17-app-architecture/migration-map.md)及[path-map.json](../17-app-architecture/path-map.json)。G1不改函数名称、注释内容或行为。删除旧路径和新增路径的证据分别保留在comment-evidence.json，不能把旧摘要套用至新文件。

## Phase 17 最终位置与增量复核（2026-10-24）

本表原始基线、1011条历史命名记录及旧行号继续保留；G1目录中间映射不等于最终模块归属。
最终逐文件/函数/声明/静态对象/类型/字段/宏/条件分支见
[Phase17迁移导航](../17-app-architecture/migration-map.md)与其final-symbol-map.json索引。
函数签名、直接调用与注册引用、状态owner和读写上下文随该索引定位，历史git show仍取旧路径。

G2～G5各实现批次及G6注释校核分别对实际C/H和实现依赖重新复核；comment-evidence.json
保留旧路径删除记录并绑定新owner源码，不挪用旧摘要。新增/继承的模板句、错误输入或副作用
说明按真实定义修正，C完整/H摘要必须定位最终C。覆盖/格式/语义/行为分别登记；暂存区R08
通过只证明证据一致，具体内容与既有行为由各组review及真实C对照说明，不能由摘要脚本代替。
实际通过数量、构建/回归和未验项见Phase17 validation，不将AC5与实车改写为已经通过。

</details>
