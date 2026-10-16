# Phase 15 注释专项：逐函数与模块检查清单

> 基线：`1eca21c9d3c20a54e3599fc505c4625f9cd9e99c`，2026-10-16 只读提取。
> 本轮只编写并提交规约；以下“待实施/待审查”不是已完成注释整改。

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
| CF-001 / CG4 | `App/app.c:58::App_IsStopped`；定义前 | `App/app.h::App_IsStopped` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-002 / CG4 | `App/app.c:63::App_Init`；定义前 | `App/app.h::App_Init` | 流程/算法；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-003 / CG4 | `App/app.c:106::App_Loop`；定义前 | `App/app.h::App_Loop` | 流程/算法；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-004 / CG4 | `App/app.c:184::App_TryStart`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-005 / CG4 | `App/app.c:254::App_ControlISR`；定义前 | `App/app.h::App_ControlISR` | 流程/算法；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-006 / CG4 | `App/app.c:338::App_TaskISR`；定义前 | `App/app.h::App_TaskISR` | 流程/算法；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-007 / CG4 | `App/app.c:372::HAL_TIM_PeriodElapsedCallback`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-008 / CG4 | `App/app.c:384::App_SampleISR`；定义前 | `App/app.h::App_SampleISR` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-009 / CG4 | `App/app.c:392::App_RequestStop`；定义前 | `App/app.h::App_RequestStop` | 流程/算法；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-010 / CG4 | `App/app.c:460::App_FinalizeStop`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-011 / CG4 | `App/app.c:494::App_ResetRunState`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-012 / CG4 | `App/app.c:508::App_AbsFloat`；定义前 | `App/app.h::App_AbsFloat` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-013 / CG2 | `App/control.c:96::Control_ApplyProfile`；定义前 | `App/control.h::Control_ApplyProfile` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-014 / CG2 | `App/control.c:116::Tracking_GetState`；定义前 | `App/control.h::Tracking_GetState` | 纯 getter；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-015 / CG2 | `App/control.c:121::Tracking_TrySetMode`；定义前 | `App/control.h::Tracking_TrySetMode` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-016 / CG2 | `App/control.c:147::Tracking_SetGear`；定义前 | `App/control.h::Tracking_SetGear` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-017 / CG2 | `App/control.c:155::Tracking_CompleteControl`；定义前 | `App/control.h::Tracking_CompleteControl` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-018 / CG2 | `App/control.c:160::Tracking_Stopped`；定义前 | `App/control.h::Tracking_Stopped` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-019 / CG2 | `App/control.c:181::Tracking_AddProtection`；定义前 | `App/control.h::Tracking_AddProtection` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-020 / CG2 | `App/control.c:188::Control_GetKernelProfile`；定义前 | `App/control.h::Control_GetKernelProfile` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-021 / CG2 | `App/control.c:193::Control_MergeProfile`；定义前 | `App/control.h::Control_MergeProfile` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-022 / CG2 | `App/control.c:214::Tracking_Observe`；定义前 | `App/control.h::Tracking_Observe` | 流程/算法；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-023 / CG2 | `App/control.c:251::Tracking_Execute`；定义前 | `App/control.h::Tracking_Execute` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-024 / CG2 | `App/control.c:282::Tracking_AccumulateMotion`；定义前 | `App/control.h::Tracking_AccumulateMotion` | 单调用转发；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-025 / CG2 | `App/control.c:288::Control_UpdateLegacy`；定义前 | `App/control.h::Control_UpdateLegacy` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-026 / CG2 | `App/control.c:327::Control_ResetRunState`；定义前 | `App/control.h::Control_ResetRunState` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-027 / CG4 | `App/dashboard.c:23::Dashboard_Init`；定义前 | `App/dashboard.h::Dashboard_Init` | 流程/算法；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-028 / CG4 | `App/dashboard.c:38::Dashboard_Update`；定义前 | `App/dashboard.h::Dashboard_Update` | 流程/算法；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-029 / CG3 | `App/datalog.c:62::Datalog_CalculateCrc16`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-030 / CG3 | `App/datalog.c:81::Datalog_LatchFault`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-031 / CG3 | `App/datalog.c:102::Datalog_RequestFaultStop`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-032 / CG3 | `App/datalog.c:109::Datalog_SetStopSummary`；定义前 | `App/datalog.h::Datalog_SetStopSummary` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-033 / CG3 | `App/datalog.c:120::Datalog_GetStatus`；定义前 | `App/datalog.h::Datalog_GetStatus` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-034 / CG3 | `App/datalog.c:126::Datalog_IsActive`；定义前 | `App/datalog.h::Datalog_IsActive` | 纯 getter；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-035 / CG3 | `App/datalog.c:131::Datalog_GetCount`；定义前 | `App/datalog.h::Datalog_GetCount` | 纯 getter；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-036 / CG3 | `App/datalog.c:136::Datalog_Init`；定义前 | `App/datalog.h::Datalog_Init` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-037 / CG3 | `App/datalog.c:151::Datalog_SetProfile`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-038 / CG3 | `App/datalog.c:160::Datalog_Start`；定义前 | `App/datalog.h::Datalog_Start` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-039 / CG3 | `App/datalog.c:222::Datalog_Clear`；定义前 | `App/datalog.h::Datalog_Clear` | 流程/算法；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-040 / CG3 | `App/datalog.c:230::Datalog_PublishControl`；定义前 | `App/datalog.h::Datalog_PublishControl` | 流程/算法；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-041 / CG3 | `App/datalog.c:258::Datalog_Push`；定义前 | `App/datalog.h::Datalog_Push` | 流程/算法；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-042 / CG3 | `App/datalog.c:299::Datalog_Service`；定义前 | `App/datalog.h::Datalog_Service` | 流程/算法；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-043 / CG3 | `App/datalog.c:332::Datalog_Flush`；定义前 | `App/datalog.h::Datalog_Flush` | 单调用转发；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-044 / CG3 | `App/datalog.c:337::Datalog_Stop`；定义前 | `App/datalog.h::Datalog_Stop` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-045 / CG3 | `App/datalog.c:353::Datalog_IsHeaderValid`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-046 / CG3 | `App/datalog.c:363::Datalog_Dump`；定义前 | `App/datalog.h::Datalog_Dump` | 流程/算法；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-047 / CG2 | `App/electromagnetic_fsm.c:21::ElectromagneticFSM_GetState`；定义前 | `App/electromagnetic_fsm.h::ElectromagneticFSM_GetState` | 纯 getter；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-048 / CG2 | `App/electromagnetic_fsm.c:26::ElectromagneticFSM_Reset`；定义前 | `App/electromagnetic_fsm.h::ElectromagneticFSM_Reset` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-049 / CG2 | `App/electromagnetic_fsm.c:33::ElectromagneticFSM_Accumulate`；定义前 | `App/electromagnetic_fsm.h::ElectromagneticFSM_Accumulate` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-050 / CG2 | `App/electromagnetic_fsm.c:43::ElectromagneticFSM_GetDistance`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-051 / CG2 | `App/electromagnetic_fsm.c:52::ElectromagneticFSM_ClearDistance`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-052 / CG2 | `App/electromagnetic_fsm.c:62::ElectromagneticFSM_ClearAngle`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-053 / CG2 | `App/electromagnetic_fsm.c:73::ElectromagneticFSM_Measure`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-054 / CG2 | `App/electromagnetic_fsm.c:87::ElectromagneticFSM_UpdateDirection`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-055 / CG2 | `App/electromagnetic_fsm.c:94::ElectromagneticFSM_Update`；定义前 | `App/electromagnetic_fsm.h::ElectromagneticFSM_Update` | 流程/算法；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-056 / CG2 | `App/element.c:21::Element_Judge`；定义前 | `App/element.h::Element_Judge` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-057 / CG2 | `App/element.c:30::Element_JudgeStraight`；定义前 | `App/element.h::Element_JudgeStraight` | 空壳；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-058 / CG2 | `App/element.c:35::Element_JudgeCrossroads`；定义前 | `App/element.h::Element_JudgeCrossroads` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-059 / CG2 | `App/element.c:44::Element_JudgeCrossroadsExit`；定义前 | `App/element.h::Element_JudgeCrossroadsExit` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-060 / CG2 | `App/element.c:53::Element_JudgeTeeterboard`；定义前 | `App/element.h::Element_JudgeTeeterboard` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-061 / CG2 | `App/element.c:63::Element_JudgeTeeterboardExit`；定义前 | `App/element.h::Element_JudgeTeeterboardExit` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-062 / CG2 | `App/element.c:77::Element_JudgeCask`；定义前 | `App/element.h::Element_JudgeCask` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-063 / CG2 | `App/element.c:85::Element_JudgeCaskExit`；定义前 | `App/element.h::Element_JudgeCaskExit` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-064 / CG2 | `App/filter.c:9::Filter_InitLowpass`；定义前 | `App/filter.h::Filter_InitLowpass` | 流程/算法；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-065 / CG2 | `App/filter.c:15::Filter_UpdateLowpass`；定义前 | `App/filter.h::Filter_UpdateLowpass` | 流程/算法；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-066 / CG2 | `App/filter.c:23::Filter_InitHighpass`；定义前 | `App/filter.h::Filter_InitHighpass` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-067 / CG2 | `App/filter.c:32::Filter_UpdateHighpass`；定义前 | `App/filter.h::Filter_UpdateHighpass` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-068 / CG1 | `App/gray_sensor.c:16::Gray_GetState`；定义前 | `App/gray_sensor.h::Gray_GetState` | 纯 getter；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-069 / CG1 | `App/gray_sensor.c:21::Gray_GetBlack`；定义前 | `App/gray_sensor.h::Gray_GetBlack` | 纯 getter；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-070 / CG1 | `App/gray_sensor.c:26::Gray_GetWhite`；定义前 | `App/gray_sensor.h::Gray_GetWhite` | 纯 getter；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-071 / CG1 | `App/gray_sensor.c:31::Gray_ValidateCalibration`；定义前 | `App/gray_sensor.h::Gray_ValidateCalibration` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-072 / CG1 | `App/gray_sensor.c:47::Gray_CheckCalibration`；定义前 | `App/gray_sensor.h::Gray_CheckCalibration` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-073 / CG1 | `App/gray_sensor.c:67::Gray_Normalize`；定义前 | `App/gray_sensor.h::Gray_Normalize` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-074 / CG1 | `App/gray_sensor.c:87::Gray_ComputeError`；定义前 | `App/gray_sensor.h::Gray_ComputeError` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-075 / CG1 | `App/gray_sensor.c:106::Gray_ResetSequence`；定义前 | `App/gray_sensor.h::Gray_ResetSequence` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-076 / CG1 | `App/gray_sensor.c:112::Gray_Init`；定义前 | `App/gray_sensor.h::Gray_Init` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-077 / CG1 | `App/gray_sensor.c:119::Gray_Update`；定义前 | `App/gray_sensor.h::Gray_Update` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-078 / CG1 | `App/imu_proc.c:34::IMU_GetAngleDeltaX`；定义前 | `App/imu_proc.h::IMU_GetAngleDeltaX` | 纯 getter；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-079 / CG1 | `App/imu_proc.c:39::IMU_GetState`；定义前 | `App/imu_proc.h::IMU_GetState` | 纯 getter；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-080 / CG1 | `App/imu_proc.c:44::IMU_ZeroAngleX`；定义前 | `App/imu_proc.h::IMU_ZeroAngleX` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-081 / CG1 | `App/imu_proc.c:49::IMU_Init`；定义前 | `App/imu_proc.h::IMU_Init` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-082 / CG1 | `App/imu_proc.c:56::IMU_UpdateAccelVelocity`；定义前 | `App/imu_proc.h::IMU_UpdateAccelVelocity` | 流程/算法；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-083 / CG1 | `App/imu_proc.c:73::IMU_UpdateGyroAngle`；定义前 | `App/imu_proc.h::IMU_UpdateGyroAngle` | 流程/算法；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-084 / CG1 | `App/imu_proc.c:94::IMU_CalibrateAccel`；定义前 | `App/imu_proc.h::IMU_CalibrateAccel` | 流程/算法；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-085 / CG1 | `App/imu_proc.c:115::IMU_CalibrateGyro`；定义前 | `App/imu_proc.h::IMU_CalibrateGyro` | 流程/算法；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-086 / CG1 | `App/imu_proc.c:136::IMU_ResetRunState`；定义前 | `App/imu_proc.h::IMU_ResetRunState` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-087 / CG4 | `App/menu.c:68::Menu_ReadKeyAverage`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-088 / CG4 | `App/menu.c:79::Menu_ScanKey`；定义前 | `App/menu.h::Menu_ScanKey` | 流程/算法；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-089 / CG4 | `App/menu.c:147::Menu_HandleKey`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-090 / CG4 | `App/menu.c:326::Menu_DrawContent`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-091 / CG4 | `App/menu.c:443::Menu_DrawCursor`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-092 / CG4 | `App/menu.c:459::Menu_Update`；定义前 | `App/menu.h::Menu_Update` | 流程/算法；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-093 / CG2 | `App/motor.c:18::Motor_GetState`；定义前 | `App/motor.h::Motor_GetState` | 纯 getter；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-094 / CG2 | `App/motor.c:23::Motor_SetTargets`；定义前 | `App/motor.h::Motor_SetTargets` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-095 / CG2 | `App/motor.c:29::Motor_SetBaseSpeed`；定义前 | `App/motor.h::Motor_SetBaseSpeed` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-096 / CG2 | `App/motor.c:34::Motor_SetFanDuty`；定义前 | `App/motor.h::Motor_SetFanDuty` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-097 / CG2 | `App/motor.c:39::Motor_SetFanDutyIdle`；定义前 | `App/motor.h::Motor_SetFanDutyIdle` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-098 / CG2 | `App/motor.c:44::Motor_ResetDistance`；定义前 | `App/motor.h::Motor_ResetDistance` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-099 / CG2 | `App/motor.c:51::Motor_DistributeSpeed`；定义前 | `App/motor.h::Motor_DistributeSpeed` | 流程/算法；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-100 / CG2 | `App/motor.c:66::Motor_UpdateControl`；定义前 | `App/motor.h::Motor_UpdateControl` | 流程/算法；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-101 / CG2 | `App/motor.c:99::Motor_ReadEncoder`；定义前 | `App/motor.h::Motor_ReadEncoder` | 流程/算法；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-102 / CG2 | `App/motor.c:116::Motor_Init`；定义前 | `App/motor.h::Motor_Init` | 流程/算法；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-103 / CG2 | `App/motor.c:124::Motor_InitEncoder`；定义前 | `App/motor.h::Motor_InitEncoder` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-104 / CG2 | `App/motor.c:131::Motor_ResetRunState`；定义前 | `App/motor.h::Motor_ResetRunState` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-105 / CG2 | `App/motor.c:141::Motor_EmergencyStop`；定义前 | `App/motor.h::Motor_EmergencyStop` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-106 / CG2 | `App/optical_fsm.c:11::OpticalFSM_GetState`；定义前 | `App/optical_fsm.h::OpticalFSM_GetState` | 纯 getter；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-107 / CG2 | `App/optical_fsm.c:16::OpticalFSM_Reset`；定义前 | `App/optical_fsm.h::OpticalFSM_Reset` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-108 / CG2 | `App/optical_fsm.c:24::OpticalFSM_Update`；定义前 | `App/optical_fsm.h::OpticalFSM_Update` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-109 / CG3 | `App/param.c:130::Param_CalculateCrc16`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-110 / CG3 | `App/param.c:150::Param_CalculateCrcV1`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-111 / CG3 | `App/param.c:161::Param_CalculateCrcV2`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-112 / CG3 | `App/param.c:172::Param_CalculateCrcV3`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-113 / CG3 | `App/param.c:183::Param_ClampFanDuty`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-114 / CG3 | `App/param.c:192::Param_SetOpticalDefaults`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-115 / CG3 | `App/param.c:205::Param_SetDefaults`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-116 / CG3 | `App/param.c:223::Param_LoadBanks`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-117 / CG3 | `App/param.c:239::Param_StoreBanks`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-118 / CG3 | `App/param.c:265::Param_GetDirection`；定义前 | `App/param.h::Param_GetDirection` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-119 / CG3 | `App/param.c:270::Param_ApplyCurrentDirection`；定义前 | `App/param.h::Param_ApplyCurrentDirection` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-120 / CG3 | `App/param.c:279::Param_SetDirection`；定义前 | `App/param.h::Param_SetDirection` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-121 / CG3 | `App/param.c:294::Param_GetFan`；定义前 | `App/param.h::Param_GetFan` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-122 / CG3 | `App/param.c:304::Param_SetFan`；定义前 | `App/param.h::Param_SetFan` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-123 / CG3 | `App/param.c:347::Param_SelectGear`；定义前 | `App/param.h::Param_SelectGear` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-124 / CG3 | `App/param.c:364::Param_WriteMirror`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-125 / CG3 | `App/param.c:378::Param_Load`；定义前 | `App/param.h::Param_Load` | 流程/算法；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-126 / CG3 | `App/param.c:510::Param_MarkDirty`；定义前 | `App/param.h::Param_MarkDirty` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-127 / CG3 | `App/param.c:516::Param_TryFlushPending`；定义前 | `App/param.h::Param_TryFlushPending` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-128 / CG3 | `App/param.c:539::Param_Save`；定义前 | `App/param.h::Param_Save` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-129 / CG3 | `App/param.c:545::Param_ServiceSave`；定义前 | `App/param.h::Param_ServiceSave` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-130 / CG3 | `App/param.c:553::Param_FlushPending`；定义前 | `App/param.h::Param_FlushPending` | 单调用转发；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-131 / CG2 | `App/pid.c:39::PID_GetState`；定义前 | `App/pid.h::PID_GetState` | 纯 getter；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-132 / CG2 | `App/pid.c:44::PID_SetGain`；定义前 | `App/pid.h::PID_SetGain` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-133 / CG2 | `App/pid.c:86::PID_SetTrackGains`；定义前 | `App/pid.h::PID_SetTrackGains` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-134 / CG2 | `App/pid.c:94::PID_SetAngleGains`；定义前 | `App/pid.h::PID_SetAngleGains` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-135 / CG2 | `App/pid.c:101::PID_DirectionStep`；定义前 | `App/pid.h::PID_DirectionStep` | 流程/算法；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-136 / CG2 | `App/pid.c:136::PID_UpdateTrack`；定义前 | `App/pid.h::PID_UpdateTrack` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-137 / CG2 | `App/pid.c:143::PID_UpdateAngle`；定义前 | `App/pid.h::PID_UpdateAngle` | 流程/算法；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-138 / CG2 | `App/pid.c:156::PID_UpdateIncrementalLeft`；定义前 | `App/pid.h::PID_UpdateIncrementalLeft` | 流程/算法；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-139 / CG2 | `App/pid.c:193::PID_UpdateIncrementalRight`；定义前 | `App/pid.h::PID_UpdateIncrementalRight` | 流程/算法；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-140 / CG2 | `App/pid.c:239::PID_UpdateSpeed`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-141 / CG2 | `App/pid.c:288::PID_UpdateSpeedLeft`；定义前 | `App/pid.h::PID_UpdateSpeedLeft` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-142 / CG2 | `App/pid.c:296::PID_UpdateSpeedRight`；定义前 | `App/pid.h::PID_UpdateSpeedRight` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-143 / CG2 | `App/pid.c:303::PID_ResetSpeed`；定义前 | `App/pid.h::PID_ResetSpeed` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-144 / CG2 | `App/pid.c:313::PID_ResetAll`；定义前 | `App/pid.h::PID_ResetAll` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-145 / CG2 | `App/roundabout.c:73::Roundabout_GetProfile`；定义前 | `App/roundabout.h::Roundabout_GetProfile` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-146 / CG2 | `App/roundabout.c:90::Roundabout_Update`；定义前 | `App/roundabout.h::Roundabout_Update` | 流程/算法；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-147 / CG2 | `App/roundabout.c:140::Roundabout_JudgePreentryLeft`；定义前 | `App/roundabout.h::Roundabout_JudgePreentryLeft` | 流程/算法；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-148 / CG2 | `App/roundabout.c:166::Roundabout_JudgePreentryRight`；定义前 | `App/roundabout.h::Roundabout_JudgePreentryRight` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-149 / CG2 | `App/roundabout.c:177::Roundabout_JudgePreentryExit`；定义前 | `App/roundabout.h::Roundabout_JudgePreentryExit` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-150 / CG2 | `App/roundabout.c:190::Roundabout_JudgeEntryLeft`；定义前 | `App/roundabout.h::Roundabout_JudgeEntryLeft` | 空壳；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-151 / CG2 | `App/roundabout.c:195::Roundabout_JudgeEntryRight`；定义前 | `App/roundabout.h::Roundabout_JudgeEntryRight` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-152 / CG2 | `App/roundabout.c:210::Roundabout_JudgeAhead`；定义前 | `App/roundabout.h::Roundabout_JudgeAhead` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-153 / CG2 | `App/roundabout.c:219::Roundabout_JudgeEntered`；定义前 | `App/roundabout.h::Roundabout_JudgeEntered` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-154 / CG2 | `App/roundabout.c:228::Roundabout_JudgeEnteredAgain`；定义前 | `App/roundabout.h::Roundabout_JudgeEnteredAgain` | 流程/算法；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-155 / CG2 | `App/roundabout.c:236::Roundabout_JudgePreExit`；定义前 | `App/roundabout.h::Roundabout_JudgePreExit` | 空壳；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-156 / CG2 | `App/roundabout.c:241::Roundabout_JudgeExit`；定义前 | `App/roundabout.h::Roundabout_JudgeExit` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-157 / CG2 | `App/roundabout.c:250::Roundabout_JudgeExited`；定义前 | `App/roundabout.h::Roundabout_JudgeExited` | 流程/算法；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-158 / CG2 | `App/roundabout.c:262::Roundabout_Judge`；定义前 | `App/roundabout.h::Roundabout_Judge` | 空壳；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-159 / CG2 | `App/roundabout.c:267::Roundabout_ResetRunState`；定义前 | `App/roundabout.h::Roundabout_ResetRunState` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-160 / CG1 | `App/track_sensor.c:22::Track_GetState`；定义前 | `App/track_sensor.h::Track_GetState` | 纯 getter；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-161 / CG1 | `App/track_sensor.c:27::Track_CalculateError`；定义前 | `App/track_sensor.h::Track_CalculateError` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-162 / CG1 | `App/track_sensor.c:35::Track_PublishObservation`；定义前 | `App/track_sensor.h::Track_PublishObservation` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-163 / CG1 | `App/track_sensor.c:44::Track_SetWeights`；定义前 | `App/track_sensor.h::Track_SetWeights` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-164 / CG1 | `App/track_sensor.c:52::Track_UpdateError`；定义前 | `App/track_sensor.h::Track_UpdateError` | 流程/算法；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-165 / CG1 | `App/track_sensor.c:84::Track_UpdateSymmetry`；定义前 | `App/track_sensor.h::Track_UpdateSymmetry` | 流程/算法；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-166 / CG1 | `App/track_sensor.c:100::Track_ReadAdc`；定义前 | `App/track_sensor.h::Track_ReadAdc` | 流程/算法；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-167 / CG1 | `App/track_sensor.c:146::Track_InitAdc`；定义前 | `App/track_sensor.h::Track_InitAdc` | 单调用转发；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-168 / CG4 | `App/wireless.c:18::Wireless_Adjust`；定义前 | `App/wireless.h::Wireless_Adjust` | 流程/算法；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-169 / CG1 | `BSP/bsp_adc.c:30::BSP_ADC_Init`；定义前 | `BSP/bsp_adc.h::BSP_ADC_Init` | 流程/算法；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-170 / CG1 | `BSP/bsp_adc.c:37::BSP_ADC_Read`；定义前 | `BSP/bsp_adc.h::BSP_ADC_Read` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-171 / CG1 | `BSP/bsp_adc.c:47::BSP_ADC_ReadGrayFresh`；定义前 | `BSP/bsp_adc.h::BSP_ADC_ReadGrayFresh` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-172 / CG1 | `BSP/bsp_dl1b.c:56::BSP_DL1B_ReadRegister`；定义前 | 无对应自有声明；不新增接口 | 单调用转发；已有块仍须核对契约、顺序及语义 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-173 / CG1 | `BSP/bsp_dl1b.c:71::BSP_DL1B_WriteRegister`；定义前 | 无对应自有声明；不新增接口 | 单调用转发；已有块仍须核对契约、顺序及语义 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-174 / CG1 | `BSP/bsp_dl1b.c:79::BSP_DL1B_Init`；定义前 | `BSP/bsp_dl1b.h::BSP_DL1B_Init` | 流程/算法；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-175 / CG1 | `BSP/bsp_dl1b.c:130::BSP_DL1B_Update`；定义前 | `BSP/bsp_dl1b.h::BSP_DL1B_Update` | 流程/算法；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-176 / CG1 | `BSP/bsp_dl1b.c:177::BSP_DL1B_GetDistanceMm`；定义前 | `BSP/bsp_dl1b.h::BSP_DL1B_GetDistanceMm` | 纯 getter；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-177 / CG2 | `BSP/bsp_encoder.c:18::BSP_Encoder_Init`；定义前 | `BSP/bsp_encoder.h::BSP_Encoder_Init` | 流程/算法；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-178 / CG2 | `BSP/bsp_encoder.c:26::BSP_Encoder_GetLeft`；定义前 | `BSP/bsp_encoder.h::BSP_Encoder_GetLeft` | 单调用转发；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-179 / CG2 | `BSP/bsp_encoder.c:31::BSP_Encoder_GetRight`；定义前 | `BSP/bsp_encoder.h::BSP_Encoder_GetRight` | 单调用转发；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-180 / CG2 | `BSP/bsp_encoder.c:45::BSP_Encoder_ReadClear`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-181 / CG3 | `BSP/bsp_flash.c:27::BSP_Flash_Init`；定义前 | `BSP/bsp_flash.h::BSP_Flash_Init` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-182 / CG3 | `BSP/bsp_flash.c:35::BSP_Flash_Read`；定义前 | `BSP/bsp_flash.h::BSP_Flash_Read` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-183 / CG3 | `BSP/bsp_flash.c:45::BSP_Flash_Write`；定义前 | `BSP/bsp_flash.h::BSP_Flash_Write` | 流程/算法；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-184 / CG3 | `BSP/bsp_flash.c:85::BSP_Flash_LogErase`；定义前 | `BSP/bsp_flash.h::BSP_Flash_LogErase` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-185 / CG3 | `BSP/bsp_flash.c:99::BSP_Flash_LogWrite`；定义前 | `BSP/bsp_flash.h::BSP_Flash_LogWrite` | 流程/算法；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-186 / CG3 | `BSP/bsp_flash.c:137::BSP_Flash_LogRead`；定义前 | `BSP/bsp_flash.h::BSP_Flash_LogRead` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-187 / CG1 | `BSP/bsp_gray.c:24::BSP_Gray_SelectChannel`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-188 / CG1 | `BSP/bsp_gray.c:32::BSP_Gray_ResetSequence`；定义前 | `BSP/bsp_gray.h::BSP_Gray_ResetSequence` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-189 / CG1 | `BSP/bsp_gray.c:37::BSP_Gray_Init`；定义前 | `BSP/bsp_gray.h::BSP_Gray_Init` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-190 / CG1 | `BSP/bsp_gray.c:44::BSP_Gray_ReadFrame`；定义前 | `BSP/bsp_gray.h::BSP_Gray_ReadFrame` | 流程/算法；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-191 / CG1 | `BSP/bsp_imu660rb.c:47::BSP_IMU660RB_SetChipSelect`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-192 / CG1 | `BSP/bsp_imu660rb.c:59::BSP_IMU660RB_WriteRegister`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-193 / CG1 | `BSP/bsp_imu660rb.c:74::BSP_IMU660RB_ReadRegister`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-194 / CG1 | `BSP/bsp_imu660rb.c:93::BSP_IMU660RB_ReadRegisters`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-195 / CG1 | `BSP/bsp_imu660rb.c:108::BSP_IMU660RB_CheckIdentity`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-196 / CG1 | `BSP/bsp_imu660rb.c:126::BSP_IMU660RB_Init`；定义前 | `BSP/bsp_imu660rb.h::BSP_IMU660RB_Init` | 流程/算法；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-197 / CG1 | `BSP/bsp_imu660rb.c:149::BSP_IMU660RB_GetAcc`；定义前 | `BSP/bsp_imu660rb.h::BSP_IMU660RB_GetAcc` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-198 / CG1 | `BSP/bsp_imu660rb.c:158::BSP_IMU660RB_GetGyro`；定义前 | `BSP/bsp_imu660rb.h::BSP_IMU660RB_GetGyro` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-199 / CG4 | `BSP/bsp_key.c:16::BSP_Key_RequireRelease`；定义前 | `BSP/bsp_key.h::BSP_Key_RequireRelease` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-200 / CG4 | `BSP/bsp_key.c:24::BSP_Key_Init`；定义前 | `BSP/bsp_key.h::BSP_Key_Init` | 单调用转发；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-201 / CG4 | `BSP/bsp_key.c:29::BSP_Key_PollEvent`；定义前 | `BSP/bsp_key.h::BSP_Key_PollEvent` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-202 / CG4 | `BSP/bsp_key.c:67::BSP_Key_StartPressed`；定义前 | `BSP/bsp_key.h::BSP_Key_StartPressed` | 单调用转发；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-203 / CG4 | `BSP/bsp_lcd.c:22::BSP_LCD_Init`；定义前 | `BSP/bsp_lcd.h::BSP_LCD_Init` | 流程/算法；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-204 / CG4 | `BSP/bsp_lcd.c:29::BSP_LCD_Clear`；定义前 | `BSP/bsp_lcd.h::BSP_LCD_Clear` | 单调用转发；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-205 / CG4 | `BSP/bsp_lcd.c:36::BSP_LCD_SetBacklight`；定义前 | `BSP/bsp_lcd.h::BSP_LCD_SetBacklight` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-206 / CG4 | `BSP/bsp_lcd.c:51::BSP_LCD_ShowChar`；定义前 | `BSP/bsp_lcd.h::BSP_LCD_ShowChar` | 流程/算法；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-207 / CG4 | `BSP/bsp_lcd.c:98::BSP_LCD_ShowString`；定义前 | `BSP/bsp_lcd.h::BSP_LCD_ShowString` | 流程/算法；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-208 / CG4 | `BSP/bsp_lcd.c:117::BSP_LCD_ShowInt`；定义前 | `BSP/bsp_lcd.h::BSP_LCD_ShowInt` | 流程/算法；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-209 / CG4 | `BSP/bsp_lcd.c:153::BSP_LCD_ShowFloat`；定义前 | `BSP/bsp_lcd.h::BSP_LCD_ShowFloat` | 流程/算法；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-210 / CG2 | `BSP/bsp_pwm.c:30::BSP_PWM_Init`；定义前 | `BSP/bsp_pwm.h::BSP_PWM_Init` | 流程/算法；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-211 / CG2 | `BSP/bsp_pwm.c:37::BSP_PWM_SetDuty`；定义前 | `BSP/bsp_pwm.h::BSP_PWM_SetDuty` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-212 / CG1 | `BSP/bsp_sampler.c:30::BSP_Sampler_Init`；定义前 | `BSP/bsp_sampler.h::BSP_Sampler_Init` | 流程/算法；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-213 / CG1 | `BSP/bsp_sampler.c:46::BSP_Sampler_Start`；定义前 | `BSP/bsp_sampler.h::BSP_Sampler_Start` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-214 / CG1 | `BSP/bsp_sampler.c:56::BSP_Sampler_Stop`；定义前 | `BSP/bsp_sampler.h::BSP_Sampler_Stop` | 单调用转发；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-215 / CG1 | `BSP/bsp_sampler.c:61::BSP_Sampler_GetTick`；定义前 | `BSP/bsp_sampler.h::BSP_Sampler_GetTick` | 纯 getter；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-216 / CG1 | `BSP/bsp_sampler.c:66::BSP_Sampler_GetEncL_1ms`；定义前 | `BSP/bsp_sampler.h::BSP_Sampler_GetEncL_1ms` | 纯 getter；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-217 / CG1 | `BSP/bsp_sampler.c:71::BSP_Sampler_GetEncR_1ms`；定义前 | `BSP/bsp_sampler.h::BSP_Sampler_GetEncR_1ms` | 纯 getter；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-218 / CG1 | `BSP/bsp_sampler.c:76::BSP_Sampler_GetGyroRaw`；定义前 | `BSP/bsp_sampler.h::BSP_Sampler_GetGyroRaw` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-219 / CG1 | `BSP/bsp_sampler.c:83::BSP_Sampler_GetAccRaw`；定义前 | `BSP/bsp_sampler.h::BSP_Sampler_GetAccRaw` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-220 / CG1 | `BSP/bsp_sampler.c:90::BSP_Sampler_ConsumeEncL`；定义前 | `BSP/bsp_sampler.h::BSP_Sampler_ConsumeEncL` | 流程/算法；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-221 / CG1 | `BSP/bsp_sampler.c:97::BSP_Sampler_ConsumeEncR`；定义前 | `BSP/bsp_sampler.h::BSP_Sampler_ConsumeEncR` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-222 / CG1 | `BSP/bsp_sampler.c:110::TIM15_IRQHandler`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-223 / CG1 | `BSP/bsp_time.c:8::BSP_Time_Init`；定义前 | `BSP/bsp_time.h::BSP_Time_Init` | 流程/算法；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-224 / CG1 | `BSP/bsp_time.c:15::BSP_Time_GetCycles`；定义前 | `BSP/bsp_time.h::BSP_Time_GetCycles` | 纯 getter；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-225 / CG1 | `BSP/bsp_time.c:20::BSP_Time_EnterCritical`；定义前 | `BSP/bsp_time.h::BSP_Time_EnterCritical` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-226 / CG1 | `BSP/bsp_time.c:27::BSP_Time_ExitCritical`；定义前 | `BSP/bsp_time.h::BSP_Time_ExitCritical` | 单调用转发；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-227 / CG1 | `BSP/bsp_time.c:32::BSP_Time_MemoryBarrier`；定义前 | `BSP/bsp_time.h::BSP_Time_MemoryBarrier` | 单调用转发；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-228 / CG4 | `BSP/bsp_uart.c:30::BSP_UART_Init`；定义前 | `BSP/bsp_uart.h::BSP_UART_Init` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-229 / CG4 | `BSP/bsp_uart.c:37::BSP_UART_Write`；定义前 | `BSP/bsp_uart.h::BSP_UART_Write` | 流程/算法；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-230 / CG4 | `BSP/bsp_uart.c:51::BSP_UART_WriteString`；定义前 | `BSP/bsp_uart.h::BSP_UART_WriteString` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-231 / CG4 | `BSP/bsp_uart.c:60::BSP_UART_Read`；定义前 | `BSP/bsp_uart.h::BSP_UART_Read` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-232 / CG4 | `BSP/bsp_uart.c:76::BSP_UART_RxAvailable`；定义前 | `BSP/bsp_uart.h::BSP_UART_RxAvailable` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-233 / CG4 | `BSP/bsp_uart.c:81::BSP_UART_RxFlush`；定义前 | `BSP/bsp_uart.h::BSP_UART_RxFlush` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-234 / CG4 | `BSP/bsp_uart.c:93::BSP_UART_PushRx`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-235 / CG4 | `BSP/bsp_uart.c:112::HAL_UART_RxCpltCallback`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-236 / CG4 | `BSP/bsp_uart.c:126::HAL_UART_ErrorCallback`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-237 / CG4 | `BSP/bsp_uart.c:147::fputc`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-238 / CG3 | `BSP/bsp_w25q64.c:55::BSP_W25Q64_Select`；定义前 | 无对应自有声明；不新增接口 | 单调用转发；已有块仍须核对契约、顺序及语义 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-239 / CG3 | `BSP/bsp_w25q64.c:64::BSP_W25Q64_Deselect`；定义前 | 无对应自有声明；不新增接口 | 单调用转发；已有块仍须核对契约、顺序及语义 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-240 / CG3 | `BSP/bsp_w25q64.c:75::BSP_W25Q64_ReadStatus1`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-241 / CG3 | `BSP/bsp_w25q64.c:101::BSP_W25Q64_WaitIdle`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-242 / CG3 | `BSP/bsp_w25q64.c:128::BSP_W25Q64_EnableWrite`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-243 / CG3 | `BSP/bsp_w25q64.c:140::BSP_W25Q64_Init`；定义前 | `BSP/bsp_w25q64.h::BSP_W25Q64_Init` | 流程/算法；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-244 / CG3 | `BSP/bsp_w25q64.c:215::BSP_W25Q64_Read`；定义前 | `BSP/bsp_w25q64.h::BSP_W25Q64_Read` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-245 / CG3 | `BSP/bsp_w25q64.c:241::BSP_W25Q64_WritePage`；定义前 | `BSP/bsp_w25q64.h::BSP_W25Q64_WritePage` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-246 / CG3 | `BSP/bsp_w25q64.c:275::BSP_W25Q64_EraseSector`；定义前 | `BSP/bsp_w25q64.h::BSP_W25Q64_EraseSector` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-247 / CG3 | `BSP/bsp_w25q64.c:304::BSP_W25Q64_EraseBlock64K`；定义前 | `BSP/bsp_w25q64.h::BSP_W25Q64_EraseBlock64K` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-248 / CG3 | `BSP/bsp_w25q64.c:333::BSP_W25Q64_EraseChip`；定义前 | `BSP/bsp_w25q64.h::BSP_W25Q64_EraseChip` | 流程/算法；C 定义前无完整 Doxygen；体内无阶段说明，需人工判定；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-249 / CG4 | `BSP/bsp_wdt.c:35::BSP_WDT_Init`；定义前 | `BSP/bsp_wdt.h::BSP_WDT_Init` | 流程/算法；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-250 / CG4 | `BSP/bsp_wdt.c:58::BSP_WDT_Feed`；定义前 | `BSP/bsp_wdt.h::BSP_WDT_Feed` | 单调用转发；C 定义前无完整 Doxygen | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-251 / CG4 | `BSP/lcd/lcd.c:57::BSP_LCDIO_Init`；定义前 | `BSP/lcd/lcd.h::BSP_LCDIO_Init` | 流程/算法；C 定义前无完整 Doxygen；H 完整契约待改摘要 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-252 / CG4 | `BSP/lcd/lcd.c:84::BSP_LCDIO_SetReset`；定义前 | 无对应自有声明；不新增接口 | 单调用转发；已有块仍须核对契约、顺序及语义 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-253 / CG4 | `BSP/lcd/lcd.c:93::BSP_LCDIO_ClearReset`；定义前 | 无对应自有声明；不新增接口 | 单调用转发；已有块仍须核对契约、顺序及语义 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-254 / CG4 | `BSP/lcd/lcd.c:102::BSP_LCDIO_SetReset`；定义前 | 无对应自有声明；不新增接口 | 空壳；已有块仍须核对契约、顺序及语义 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-255 / CG4 | `BSP/lcd/lcd.c:111::BSP_LCDIO_ClearReset`；定义前 | 无对应自有声明；不新增接口 | 空壳；已有块仍须核对契约、顺序及语义 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-256 / CG4 | `BSP/lcd/lcd.c:122::BSP_LCDIO_InitBus`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-257 / CG4 | `BSP/lcd/lcd.c:137::BSP_LCDIO_GetTick`；定义前 | 无对应自有声明；不新增接口 | 单调用转发；已有块仍须核对契约、顺序及语义 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-258 / CG4 | `BSP/lcd/lcd.c:150::BSP_LCDIO_WriteRegister`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-259 / CG4 | `BSP/lcd/lcd.c:172::BSP_LCDIO_ReadRegister`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-260 / CG4 | `BSP/lcd/lcd.c:191::BSP_LCDIO_SendData`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |
| CF-261 / CG4 | `BSP/lcd/lcd.c:207::BSP_LCDIO_ReceiveData`；定义前 | 无对应自有声明；不新增接口 | 流程/算法；缺 details；体内无阶段说明，需人工判定 | 待实施 | 待实施 | 待审查 | 待验证 |

## CM：模块、字段、状态与格式

每行审查文件头、数据流/调用关系/调度、所有字段/枚举、全局/静态/静态局部状态、阶段分块和
来源例外。H 行重点检查摘要不与 C 矛盾；没有该类对象时逐格写可核对的“不适用及原因”。
Core 只检查已存在的非空用户区，生成区与原厂版权不变。

| ID / 组 | 文件 | 文件/流程说明 | 字段/枚举 | 状态对象 | 版权/来源/排除区登记 | 格式与内容结论 |
|---|---|---|---|---|---|---|
| CM-001 / CG4 | `App/app.c` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-002 / CG4 | `App/app.h` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-003 / CG4 | `App/app_config.h` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-004 / CG2 | `App/control.c` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-005 / CG2 | `App/control.h` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-006 / CG4 | `App/dashboard.c` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-007 / CG4 | `App/dashboard.h` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-008 / CG3 | `App/datalog.c` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-009 / CG3 | `App/datalog.h` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-010 / CG2 | `App/electromagnetic_fsm.c` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-011 / CG2 | `App/electromagnetic_fsm.h` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-012 / CG2 | `App/element.c` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-013 / CG2 | `App/element.h` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-014 / CG2 | `App/filter.c` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-015 / CG2 | `App/filter.h` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-016 / CG1 | `App/gray_calibration.h` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-017 / CG1 | `App/gray_sensor.c` | 待审查 | 待审查 | 待审查 | E-02 | 待实施/待审查 |
| CM-018 / CG1 | `App/gray_sensor.h` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-019 / CG1 | `App/imu_proc.c` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-020 / CG1 | `App/imu_proc.h` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-021 / CG4 | `App/menu.c` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-022 / CG4 | `App/menu.h` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-023 / CG2 | `App/motor.c` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-024 / CG2 | `App/motor.h` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-025 / CG2 | `App/optical_fsm.c` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-026 / CG2 | `App/optical_fsm.h` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-027 / CG3 | `App/param.c` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-028 / CG3 | `App/param.h` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-029 / CG2 | `App/pid.c` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-030 / CG2 | `App/pid.h` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-031 / CG2 | `App/roundabout.c` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-032 / CG2 | `App/roundabout.h` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-033 / CG1 | `App/track_sensor.c` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-034 / CG1 | `App/track_sensor.h` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-035 / CG4 | `App/wireless.c` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-036 / CG4 | `App/wireless.h` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-037 / CG4 | `BSP/bsp.h` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-038 / CG1 | `BSP/bsp_adc.c` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-039 / CG1 | `BSP/bsp_adc.h` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-040 / CG1 | `BSP/bsp_dl1b.c` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-041 / CG1 | `BSP/bsp_dl1b.h` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-042 / CG2 | `BSP/bsp_encoder.c` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-043 / CG2 | `BSP/bsp_encoder.h` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-044 / CG3 | `BSP/bsp_flash.c` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-045 / CG3 | `BSP/bsp_flash.h` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-046 / CG1 | `BSP/bsp_gray.c` | 待审查 | 待审查 | 待审查 | E-01 | 待实施/待审查 |
| CM-047 / CG1 | `BSP/bsp_gray.h` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-048 / CG1 | `BSP/bsp_imu660rb.c` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-049 / CG1 | `BSP/bsp_imu660rb.h` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-050 / CG4 | `BSP/bsp_key.c` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-051 / CG4 | `BSP/bsp_key.h` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-052 / CG4 | `BSP/bsp_lcd.c` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-053 / CG4 | `BSP/bsp_lcd.h` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-054 / CG2 | `BSP/bsp_pwm.c` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-055 / CG2 | `BSP/bsp_pwm.h` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-056 / CG1 | `BSP/bsp_sampler.c` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-057 / CG1 | `BSP/bsp_sampler.h` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-058 / CG1 | `BSP/bsp_time.c` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-059 / CG1 | `BSP/bsp_time.h` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-060 / CG4 | `BSP/bsp_uart.c` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-061 / CG4 | `BSP/bsp_uart.h` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-062 / CG3 | `BSP/bsp_w25q64.c` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-063 / CG3 | `BSP/bsp_w25q64.h` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-064 / CG4 | `BSP/bsp_wdt.c` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-065 / CG4 | `BSP/bsp_wdt.h` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-066 / CG4 | `BSP/lcd/lcd.c` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-067 / CG4 | `BSP/lcd/lcd.h` | 待审查 | 待审查 | 待审查 | 逐块核对；无例外须明确记无 | 待实施/待审查 |
| CM-068 / CG4 | `Core/Inc/adc.h` | 待审查 | 待审查 | 待审查 | 生成区排除；用户区/版权逐块登记 | 待实施/待审查 |
| CM-069 / CG4 | `Core/Inc/dma.h` | 待审查 | 待审查 | 待审查 | 生成区排除；用户区/版权逐块登记 | 待实施/待审查 |
| CM-070 / CG4 | `Core/Inc/gpio.h` | 待审查 | 待审查 | 待审查 | 生成区排除；用户区/版权逐块登记 | 待实施/待审查 |
| CM-071 / CG4 | `Core/Inc/i2c.h` | 待审查 | 待审查 | 待审查 | 生成区排除；用户区/版权逐块登记 | 待实施/待审查 |
| CM-072 / CG4 | `Core/Inc/main.h` | 待审查 | 待审查 | 待审查 | 生成区排除；用户区/版权逐块登记 | 待实施/待审查 |
| CM-073 / CG4 | `Core/Inc/spi.h` | 待审查 | 待审查 | 待审查 | 生成区排除；用户区/版权逐块登记 | 待实施/待审查 |
| CM-074 / CG4 | `Core/Inc/stm32h7xx_hal_conf.h` | 待审查 | 待审查 | 待审查 | 生成区排除；用户区/版权逐块登记 | 待实施/待审查 |
| CM-075 / CG4 | `Core/Inc/stm32h7xx_it.h` | 待审查 | 待审查 | 待审查 | 生成区排除；用户区/版权逐块登记 | 待实施/待审查 |
| CM-076 / CG4 | `Core/Inc/tim.h` | 待审查 | 待审查 | 待审查 | 生成区排除；用户区/版权逐块登记 | 待实施/待审查 |
| CM-077 / CG4 | `Core/Inc/usart.h` | 待审查 | 待审查 | 待审查 | 生成区排除；用户区/版权逐块登记 | 待实施/待审查 |
| CM-078 / CG4 | `Core/Src/adc.c` | 待审查 | 待审查 | 待审查 | 生成区排除；用户区/版权逐块登记 | 待实施/待审查 |
| CM-079 / CG4 | `Core/Src/dma.c` | 待审查 | 待审查 | 待审查 | 生成区排除；用户区/版权逐块登记 | 待实施/待审查 |
| CM-080 / CG4 | `Core/Src/gpio.c` | 待审查 | 待审查 | 待审查 | 生成区排除；用户区/版权逐块登记 | 待实施/待审查 |
| CM-081 / CG4 | `Core/Src/i2c.c` | 待审查 | 待审查 | 待审查 | 生成区排除；用户区/版权逐块登记 | 待实施/待审查 |
| CM-082 / CG4 | `Core/Src/main.c` | 待审查 | 待审查 | 待审查 | 生成区排除；用户区/版权逐块登记 | 待实施/待审查 |
| CM-083 / CG4 | `Core/Src/spi.c` | 待审查 | 待审查 | 待审查 | 生成区排除；用户区/版权逐块登记 | 待实施/待审查 |
| CM-084 / CG4 | `Core/Src/stm32h7xx_hal_msp.c` | 待审查 | 待审查 | 待审查 | 生成区排除；用户区/版权逐块登记 | 待实施/待审查 |
| CM-085 / CG4 | `Core/Src/stm32h7xx_it.c` | 待审查 | 待审查 | 待审查 | 生成区排除；用户区/版权逐块登记 | 待实施/待审查 |
| CM-086 / CG4 | `Core/Src/system_stm32h7xx.c` | 待审查 | 待审查 | 待审查 | 生成区排除；用户区/版权逐块登记 | 待实施/待审查 |
| CM-087 / CG4 | `Core/Src/tim.c` | 待审查 | 待审查 | 待审查 | 生成区排除；用户区/版权逐块登记 | 待实施/待审查 |
| CM-088 / CG4 | `Core/Src/usart.c` | 待审查 | 待审查 | 待审查 | 生成区排除；用户区/版权逐块登记 | 待实施/待审查 |

## 已确认来源例外

| 编号 | 基线区间 | 保留原因 | 约束 |
|---|---|---|---|
| E-01 | `BSP/bsp_gray.c:5–13` | 龙邱来源、作者、版权、许可、SHA 与适配说明原文 | 原块逐字保留；只豁免其原有格式和超长来源文本，不扩展到其他注释。 |
| E-02 | `App/gray_sensor.c:5–7` | 灰度采集来源与许可指引原文 | 原块逐字保留；不把后续算法解释划入此区。 |

Core 厂商 Header 中的版权和 USER CODE 标记保留；具体区间在各 CM 行随实施证据登记，
未登记前不能以“厂商头”豁免其他自有说明。其余来自旧移植的出处事实仍保留，可独立成块，
不因此豁免新函数说明。

## 证据与状态

- 本轮已完成的是清单覆盖校验、定义位置/目标名集合交叉核对和缺项初筛。
- 所有 CF/CM 的实现、人工语义、格式终验及行为保持仍未执行；不得因清单文件已存在而勾选。
- 模板与成品见 [comment-examples.md](comment-examples.md)，验收方法见
  [validation.md](validation.md) 注释专项 CA/CDOC 项。
