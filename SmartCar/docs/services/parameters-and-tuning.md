# 参数、持久化与完整拍调参

现行功能说明，核对基线 `68cb508`；2026-10-31迁移。需求经[功能索引](../traceability.md)进入本页，再由实现表定位源码。
当前行为不等于全部已批准/实机验收，冲突见[已知问题](../known-issues.md)。

<a id="f15"></a>
## F15 多入口参数应用
### 三种入口与应用层

菜单操作通过system/parameter_application协调；旧UART仅停车解析d/c及vp/vi/xp/xd，字母定参数、1～4十进制位×0.01，沿原4～7字节帧规则，末尾字节不参与值解析。xp/xd写当前模式方向bank，vp/vi写共享活动速度PI并标dirty；后三秒静默保存时才捕获活动PI到镜像。
SCB使用完整八项事务，不复用旧单缓冲解析，也不开放运行换模式/档。ParamBank持四方向bank，store持镜像，system负责应用至PID/Motor和菜单位置，禁止store反调使用者。

### 状态差异与冲突

`Param_GetSpeedGains`和SCB查询镜像可在flush前仍是旧值，而vp/vi活动PI已改变；P17明确保持，R06未解决。风扇值设置/切档只改Motor状态；初始化、控制拍、首次停车才有实际PWM写入，D03未决定即时应用策略。
旧UART把一次读当完整帧，有R04拆包/粘包问题；合法数字检查不等于已实现分帧。本轮不借文档使其变成可靠协议。
验证逐入口路由、保存时点、活动/镜像值、当前模式档及风扇钳位；既有g4_hmi对2999/3000ms和Flash字节有对照。实机串口分帧、风扇应用待验/待设计。

**需求来源：** [P06 FR-05, FR-15, FR-16, FR-17](../../specs/features/06-hmi-tuning/requirements.md)；[P08 FR-07](../../specs/features/08-storage-chain-fix/requirements.md)；[P11 FR-05](../../specs/features/11-hardening-cleanup/requirements.md)。实现入口：[App/system/parameter_application.c](../../App/system/parameter_application.c)、[App/services/wireless.c](../../App/services/wireless.c)、[App/control/motor.c](../../App/control/motor.c)。

<a id="f16"></a>
## F16 参数v3格式
### 当前固定镜像

参数位于W25Q64末尾4KB `0x7FF000..0x7FFFFF`。magic=`0xA55A3C3C`，version=3，sizeof=88；CRC16-CCITT多项式0x1021、初值0xFFFF，覆盖字节8到87，不含前8字节。非打包随意强转不是新的可移植承诺，目标布局由原类型及静态断言约束。
四bank为两模式×两档，每bank方向KP/K2P/KD和基础速度；光电两档默认2/0.008/15/100，K2D固定1，共享速度PI20/0.75、运行/空闲风扇默认1600/1100。模式不写镜像，每次上电光电低档。标定不入镜像。
有效v1(52B)先补两档空闲1100，再补光电；有效v2(56B)保留所有原值并追加光电。不能用新默认覆盖旧电磁/速度/风扇用户值。布局详表见下方从实际声明核对的偏移。

**需求来源：** [P06 FR-12](../../specs/features/06-hmi-tuning/requirements.md)；[P08 FR-06](../../specs/features/08-storage-chain-fix/requirements.md)；[P13 FR-07](../../specs/features/13-control-structure/requirements.md)；[P14 FR-12, FR-15](../../specs/features/14-dual-mode-tracking/requirements.md)；[P15 FR-08](../../specs/features/15-code-style-consistency/requirements.md)。实现入口：[App/services/parameter_image.h](../../App/services/parameter_image.h)、[App/services/parameter_bank.c](../../App/services/parameter_bank.c)、[App/services/parameter_store.c](../../App/services/parameter_store.c)。

<a id="f17"></a>
## F17 迁移保存与失败
### 载入、保存和失败保证

初始化先Flash再Param_Load。未知magic/version、坏CRC按默认路径报告并尝试写回；读失败或写失败锁存存储不可用，禁止后续起跑存储门禁。有效旧镜像迁移失败，本次RAM已读/已迁移参数可保留，但参数扇区擦写不回滚，重启不保证旧镜像仍能重迁移。
参数写使用static 4096B影子读改擦写，内容不变跳过擦写，修改才擦并回写16页；不是4KB栈局部。写失败可能已部分擦写，不能宣称事务性掉电安全。
保存从指定bank取方向/基础速度，实际flush才捕获活动速度PI和菜单page/arrow。成功清dirty；失败保留dirty并锁存Flash不可用，不靠“每次尝试后清pending”防重试。旧UART最后命令后3000ms保存，起跑/模式切换先尝试flush，失败拒绝操作。菜单松键/BACK按既有时机保存。

### 存储原语与验证

W25Q64页编程不跨页、等待WIP；配置上限页10ms、4KB500ms、64KB2500ms、整片110000ms，SPI失败返回错误，实际超时依赖时基。Flash服务未ready拒绝不动总线。日志写粒度32B、BSP按页拆；LogErase只擦首4KB并重设前沿，块内全区擦注释过时。
核对v1/v2/v3 CRC/偏移、默认、迁移失败RAM与掉电区别、dirty和存储锁存；既有参数/日志夹具是软件模型，Flash故障、掉电、吞吐仍待实体。

**需求来源：** [P02 FR-21, FR-22, FR-23, FR-25](../../specs/features/02-bsp-platform/requirements.md)；[P06 FR-11, FR-13, FR-14, FR-18](../../specs/features/06-hmi-tuning/requirements.md)；[P08 FR-01, FR-02, FR-03, FR-04](../../specs/features/08-storage-chain-fix/requirements.md)；[P11 FR-01, FR-02](../../specs/features/11-hardening-cleanup/requirements.md)。实现入口：[App/services/parameter_store.c](../../App/services/parameter_store.c)、[BSP/bsp_flash.c](../../BSP/bsp_flash.c)、[BSP/bsp_w25q64.c](../../BSP/bsp_w25q64.c)。

<a id="f18"></a>
## F18 完整拍临时调参
### 完整拍事务与状态

SCB八项为kp/k2p/kd/kp_v/ki_v/base_speed/fan_run/fan_idle，五增益须有限，当前限0～100，速度整数0～300脉冲/2ms，风扇整数0～10000；limits由STM32校验区提供，电脑不能独立定值。前台组校验完整请求并预留结果，RECEIVED不等于APPLIED。
事务分别持baseline、pending、last_success、effective、revision/final和单事件槽。TUNE_ENABLE显式开启当前目标；运行只收当前模式档、fan_idle不改、覆盖已开且普通赛道的请求。TIM6完整控制拍再次验证上下文并整组提交；结果含实际值、tick、control_sequence及revision。不得把部分字段先应用或把前台收到当生效。

### 元素、关闭与停车

进入十字/预环/环岛等特殊状态暂停覆盖，保留原FSM基线/部分配置；回普通才恢复。禁用恢复基线而保留最后成功值供停车保存。保护、会话切换和失效上下文取消未生效项，不撤销已生效控制值。
停车成功配置先APPLIED，再报告实际保存；运行只RAM，停车收尾合入last_success后仅尝试一次保存，保存失败可观测。单事件槽必须在同上下文即时消费，final前台短锁复制后只清id，不能覆盖未消费所有权。
核对完整拍原子性、特殊元素过渡拍、会话/保护取消、独立状态和revision回绕；既有runtime_tuning/g4_transactions可选。当前不是P10全局调参语义整改，也没有实车调参通过结论。

**需求来源：** [P16 FR-13, FR-14, FR-15, FR-16, FR-17, FR-18](../../specs/features/16-spi-wifi-link/requirements.md)；[P17 FR-07](../../specs/features/17-app-architecture/requirements.md)。实现入口：[App/control/tuning.c](../../App/control/tuning.c)、[App/control/tuning_values.c](../../App/control/tuning_values.c)、[App/system/tuning_application.c](../../App/system/tuning_application.c)、[App/services/command_dispatch.c](../../App/services/command_dispatch.c)。

## 本页实现与模块反查

下表是当前源码定位；公共/私有辅助函数归所属功能，不把声明视为另一实现。Core生成部分只作边界，用户区按原标记维护。

| 文件 | 主功能 | 定义/配置定位 |
|---|---|---|
| [App/control/motor.c](../../App/control/motor.c) | F08 | `Motor_GetState`、`Motor_SetTargets`、`Motor_SetBaseSpeed`、`Motor_SetFanDuty`、`Motor_SetFanDutyIdle`、`Motor_ResetDistance`、`Motor_DistributeSpeed`、`Motor_UpdateControl`、`Motor_ReadEncoder`、`Motor_Init`、`Motor_InitEncoder`、`Motor_ResetRunState`、`Motor_EmergencyStop` |
| [App/control/tuning.c](../../App/control/tuning.c) | F18 | `Tuning_Event`、`Tuning_ReadActive`、`Tuning_ApplyShared`、`Tuning_Equal`、`Tuning_RejectPending`、`Tuning_GetControl`、`Tuning_Stop`、`Tuning_GetStatus`、`Tuning_SelectControl`、`Tuning_CompleteControl`、`Tuning_ResetSession`、`Tuning_Init`、`Tuning_PrepareSet`、`Tuning_CommitParked`、`Tuning_Switch`、`Tuning_GetRevision`、`Tuning_TakeResult`、`Tuning_CopyUnsaved`、`Tuning_CompleteSave`、`Tuning_CopyEventValues`、`Tuning_TakeRevision`、`Tuning_TakeEvent` |
| [App/control/tuning.h](../../App/control/tuning.h) | F18 | 类型/布局/配置；关联同模块C定义 |
| [App/control/tuning_values.c](../../App/control/tuning_values.c) | F18 | `Tuning_ValidateValues` |
| [App/control/tuning_values.h](../../App/control/tuning_values.h) | F18 | 类型/布局/配置；关联同模块C定义 |
| [App/services/command_dispatch.c](../../App/services/command_dispatch.c) | F23 | `CommandDispatch_MapResult`、`CommandDispatch_ToWire`、`CommandDispatch_ToBusiness`、`CommandDispatch_Begin`、`CommandDispatch_Send`、`CommandDispatch_Position`、`CommandDispatch_ReadBank`、`CommandDispatch_QueryParams`、`CommandDispatch_QueryStatus`、`CommandDispatch_QueryResult`、`CommandDispatch_QueryLogMeta`、`CommandDispatch_MapLogResult`、`CommandDispatch_LogCommand`、`CommandDispatch_HandleCommand`、`CommandDispatch_Set`、`CommandDispatch_Switch`、`CommandDispatch_ResetSession`、`CommandDispatch_Init`、`CommandDispatch_InitLink`、`CommandDispatch_ServiceLink`、`CommandDispatch_Service`、`CommandDispatch_QueueTuningEvent`、`CommandDispatch_PublishRunStarted`、`CommandDispatch_PublishRunStopped` |
| [App/services/parameter_bank.c](../../App/services/parameter_bank.c) | F16 | `ParamBank_LoadBanks`、`ParamBank_StoreBanks`、`Param_GetDirection`、`ParamBank_SetDirection` |
| [App/services/parameter_bank.h](../../App/services/parameter_bank.h) | F16 | 类型/布局/配置；关联同模块C定义 |
| [App/services/parameter_image.h](../../App/services/parameter_image.h) | F16 | 类型/布局/配置；关联同模块C定义 |
| [App/services/parameter_store.c](../../App/services/parameter_store.c) | F17 | `Param_CalculateCrc16`、`Param_CalculateCrcV1`、`Param_CalculateCrcV2`、`Param_CalculateCrcV3`、`Param_ClampFanDuty`、`Param_SetOpticalDefaults`、`Param_SetDefaults`、`Param_WriteMirror`、`Param_GetFan`、`Param_MarkDirty`、`Param_GetSpeedGains`、`ParamStore_Load`、`ParamStore_SetFan`、`ParamStore_SetSpeedGains`、`ParamStore_TryFlushPending`、`ParamStore_GetPosition`、`ParamStore_IsUsable`、`ParamStore_IsDirty`、`ParamStore_RequestSave`、`ParamStore_IsSaveDue` |
| [App/services/parameter_store.h](../../App/services/parameter_store.h) | F17 | 类型/布局/配置；关联同模块C定义 |
| [App/services/wireless.c](../../App/services/wireless.c) | F15 | `Wireless_Adjust` |
| [App/services/wireless.h](../../App/services/wireless.h) | F15 | 类型/布局/配置；关联同模块C定义 |
| [App/system/parameter_application.c](../../App/system/parameter_application.c) | F15 | `Param_ApplyCurrentDirection`、`Param_SetDirection`、`Param_SelectGear`、`Param_Save`、`Param_ServiceSave`、`Param_FlushPending`、`Param_Load`、`Param_SetFan`、`Param_SetSpeedGains`、`Param_TryFlushPending` |
| [App/system/parameter_application.h](../../App/system/parameter_application.h) | F15 | 类型/布局/配置；关联同模块C定义 |
| [App/system/tuning_application.c](../../App/system/tuning_application.c) | F18 | `TuningApplication_IsOrdinary`、`TuningApplication_CaptureContext`、`TuningApplication_Merge`、`TuningApplication_Synchronize`、`TuningApplication_Set`、`TuningApplication_Switch`、`TuningApplication_FinalizeStop`、`TuningApplication_ServiceSave` |
| [App/system/tuning_application.h](../../App/system/tuning_application.h) | F18 | 类型/布局/配置；关联同模块C定义 |
| [BSP/bsp_flash.c](../../BSP/bsp_flash.c) | F17 | `BSP_Flash_Init`、`BSP_Flash_Read`、`BSP_Flash_Write`、`BSP_Flash_LogErase`、`BSP_Flash_LogWrite`、`BSP_Flash_LogRead` |
| [BSP/bsp_flash.h](../../BSP/bsp_flash.h) | F17 | 类型/布局/配置；关联同模块C定义 |
| [BSP/bsp_w25q64.c](../../BSP/bsp_w25q64.c) | F17 | `BSP_W25Q64_Select`、`BSP_W25Q64_Deselect`、`BSP_W25Q64_ReadStatus1`、`BSP_W25Q64_WaitIdle`、`BSP_W25Q64_EnableWrite`、`BSP_W25Q64_Init`、`BSP_W25Q64_Read`、`BSP_W25Q64_WritePage`、`BSP_W25Q64_EraseSector`、`BSP_W25Q64_EraseBlock64K`、`BSP_W25Q64_EraseChip` |
| [BSP/bsp_w25q64.h](../../BSP/bsp_w25q64.h) | F17 | 类型/布局/配置；关联同模块C定义 |

## param_image_v3_t 当前布局

小端目标布局；源头为实际声明及偏移/尺寸断言。单位和语义随字段保留。

| 偏移 | 类型/数量 | 字段 | 说明 |
|---:|---|---|---|
| 0 | uint32_t | `magic` | 原参数魔数，保持 32 位。 |
| 4 | uint16_t | `version` | 持久化版本，v1/v2 仅解码，v3 写入。 |
| 6 | uint16_t | `crc16` | 从字节 8 开始的原 CCITT 校验。 |
| 8 | uint8_t | `page` | 保存的菜单页面编号；加载校验后应用。 |
| 9 | uint8_t | `arrow` | 保存的光标，加载只接受 1..5。 |
| 10 | uint8_t×2 | `reserved` | 原两字节保留区，布局和 CRC 覆盖保持。 |
| 12 | float | `kp_v` | 两模式共用速度比例增益。 |
| 16 | float | `ki_v` | 两模式共用速度积分增益。 |
| 20 | float | `kp_x_low` | 电磁低档方向比例增益。 |
| 24 | float | `k2p_x_low` | 电磁低档方向非线性增益。 |
| 28 | float | `kd_x_low` | 电磁低档方向差分增益。 |
| 32 | int16_t | `base_speed_low` | 电磁低档基础速度，脉冲/2 ms。 |
| 34 | int16_t | `fan_duty_low` | 低档运行风扇万分比，0..10000。 |
| 36 | float | `kp_x_high` | 电磁高档方向比例增益。 |
| 40 | float | `k2p_x_high` | 电磁高档方向非线性增益。 |
| 44 | float | `kd_x_high` | 电磁高档方向差分增益。 |
| 48 | int16_t | `base_speed_high` | 电磁高档基础速度，脉冲/2 ms。 |
| 50 | int16_t | `fan_duty_high` | 高档运行风扇万分比，0..10000。 |
| 52 | int16_t | `fan_duty_idle_low` | 低档空闲风扇万分比，v2 起保存。 |
| 54 | int16_t | `fan_duty_idle_high` | 高档空闲风扇万分比，v2 起保存。 |
| 56 | float | `optical_kp_low` | 光电低档方向比例增益，v3 新增。 |
| 60 | float | `optical_k2p_low` | 光电低档方向非线性增益，v3 新增。 |
| 64 | float | `optical_kd_low` | 光电低档方向差分增益，v3 新增。 |
| 68 | int16_t | `optical_speed_low` | 光电低档基础速度，脉冲/2 ms。 |
| 70 | uint16_t | `optical_reserved_low` | 光电低档两字节保留区，写零。 |
| 72 | float | `optical_kp_high` | 光电高档方向比例增益，v3 新增。 |
| 76 | float | `optical_k2p_high` | 光电高档方向非线性增益，v3 新增。 |
| 80 | float | `optical_kd_high` | 光电高档方向差分增益，v3 新增。 |
| 84 | int16_t | `optical_speed_high` | 光电高档基础速度，脉冲/2 ms。 |
| 86 | uint16_t | `optical_reserved_high` | 光电高档两字节保留区，写零。 |
