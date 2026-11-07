# 完整拍临时调参

现行功能说明，源码核对基线 `8d5ab2faf68432ae03b2c1f91bc4593c7dc7cc3e`；2026-11-07补全。阶段保留完整设计与验收，本文只说明当前实现，不表示新增实机验证。

<a id="f18"></a>
## F18 完整拍临时调参
### 完整拍事务与状态

SCB八项为kp/k2p/kd/kp_v/ki_v/base_speed/fan_run/fan_idle，五增益须有限，当前限0～100，速度整数0～300脉冲/2ms，风扇整数0～10000；limits由STM32校验区提供，电脑不能独立定值。前台组校验完整请求并预留结果，RECEIVED不等于APPLIED。
事务分别持baseline、pending、last_success、effective、revision/final和单事件槽。TUNE_ENABLE显式开启当前目标；运行只收当前模式档、fan_idle不改、覆盖已开且普通赛道的请求。TIM6完整控制拍再次验证上下文并整组提交；结果含实际值、tick、control_sequence及revision。不得把部分字段先应用或把前台收到当生效。

### 元素、关闭与停车

进入十字/预环/环岛等特殊状态暂停覆盖，保留原FSM基线/部分配置；回普通才恢复。TUNE_DISABLE运行时先清enabled，下一TIM6 SelectControl才恢复基线；停车且overridden时立即恢复共享值，保留最后成功值供停车保存。保护与失效上下文取消未生效项；Tuning_Stop若正覆盖则立即恢复共享PI/风扇基线并禁用覆盖，保留最后成功值供停车合入。会话切换仅清pending/staged/final，保持已成功值、effective和enabled，不回滚已生效控制值。
停车成功配置先APPLIED，再报告实际保存；运行只RAM，停车收尾合入last_success后仅尝试一次保存，保存失败可观测。单事件槽必须在同上下文即时消费，final前台短锁复制后只清id，不能覆盖未消费所有权。
核对完整拍原子性、特殊元素过渡拍、会话/保护取消、独立状态和revision回绕；既有runtime_tuning/g4_transactions可选。当前不是P10全局调参语义整改，也没有实车调参通过结论。

**需求来源：** [P16 FR-13, FR-14, FR-15, FR-16, FR-17, FR-18](../../specs/features/16-spi-wifi-link/requirements.md)；[P17 FR-07](../../specs/features/17-app-architecture/requirements.md)。实现入口：[App/control/tuning.c](../../App/control/tuning.c)、[App/control/tuning_values.c](../../App/control/tuning_values.c)、[App/system/tuning_application.c](../../App/system/tuning_application.c)、[App/services/command_dispatch.c](../../App/services/command_dispatch.c)。


## 实现与状态主归属

| 文件 | 主功能 | 定义/配置定位 |
|---|---|---|
| [App/control/tuning.c](../../App/control/tuning.c) | F18 | `Tuning_Event`、`Tuning_ReadActive`、`Tuning_ApplyShared`、`Tuning_Equal`、`Tuning_RejectPending`、`Tuning_GetControl`、`Tuning_Stop`、`Tuning_GetStatus`、`Tuning_SelectControl`、`Tuning_CompleteControl`、`Tuning_ResetSession`、`Tuning_Init`、`Tuning_PrepareSet`、`Tuning_CommitParked`、`Tuning_Switch`、`Tuning_GetRevision`、`Tuning_TakeResult`、`Tuning_CopyUnsaved`、`Tuning_CompleteSave`、`Tuning_CopyEventValues`、`Tuning_TakeRevision`、`Tuning_TakeEvent` |
| [App/control/tuning.h](../../App/control/tuning.h) | F18 | 类型/布局/配置；关联同模块C定义 |
| [App/control/tuning_values.c](../../App/control/tuning_values.c) | F18 | `Tuning_ValidateValues` |
| [App/control/tuning_values.h](../../App/control/tuning_values.h) | F18 | 类型/布局/配置；关联同模块C定义 |
| [App/system/tuning_application.c](../../App/system/tuning_application.c) | F18 | `TuningApplication_IsOrdinary`、`TuningApplication_CaptureContext`、`TuningApplication_Merge`、`TuningApplication_Synchronize`、`TuningApplication_Set`、`TuningApplication_Switch`、`TuningApplication_FinalizeStop`、`TuningApplication_ServiceSave` |
| [App/system/tuning_application.h](../../App/system/tuning_application.h) | F18 | 类型/布局/配置；关联同模块C定义 |

<a id="parameters"></a>
## 事务参数、状态与生效矩阵

| 配置/状态 | 数值、类型、单位及范围 | 来源 / 修改 / 生效 / 保存 |
|---|---|---|
| kp/k2p/kd/kp_v/ki_v | float，有限0～100 | app_config.h TUNING_GAIN_MIN/MAX；JSON double先检查再转float，业务validator复核；请求不钳位；无独立默认，开启时捕获当前上下文 |
| base_speed | int32_t输入，0～300脉冲/2ms | TUNING_SPEED_MIN/MAX；提交后执行值转int16_t，不能先窄化后校验 |
| fan_run / fan_idle | int32_t，0～10000万分比 | TUNING_FAN_MIN/MAX；运行请求fan_idle必须不变；停车合入每档镜像，实际PWM见电机执行 |
| phase | DISABLED / ARMED / ACTIVE / SUSPENDED | tuning.c独占事务；前台短锁修改许可/pending，TIM6选中同拍控制配置；不是协议ACK状态 |
| save_state | uint8_t；0空闲/1待保存/2成功/3失败 | 运行仅RAM；停车合入last_success后尝试一次保存，成功/失败均完成该次保存状态并发事件 |
| revision / final / event | uint32_t版本、最终邮箱、单事件槽 | 版本标记实际参数变化，上传槽保留发生时版本；最终结果由前台短锁复制后只清id；事件须本上下文立即消费，不跨ISR返回留到下一次覆盖 |

1. SCB链路先由command codec解析完整八项，再AcceptRequest分配/复用请求缓存，command_dispatch接管前预留最终结果容量；TuningApplication_Set在锁外校验数值，再短锁捕获stopped/started/mode/gear/ordinary/保护/tick/控制序号并PrepareSet。拒绝请求无部分应用。
2. 停车成功在锁外Merge：方向目标bank→共享PI→运行风扇→空闲风扇，然后CommitParked，立即同步revision/事件，APPLIED与保存结果分开。运行请求仅pending并回RECEIVED，不写Flash。
3. TIM6在双模式观察和原状态选择后SelectControl，复核目标与ordinary/allowed；只有可执行完整拍整组应用，随后CompleteControl发布实际值/位置/版本及最终结果。ordinary严格取**电磁影子主态TRACKING且环岛NORMAL**，即使当前选择光电也不能忽略电磁元素观察状态。
4. 特殊元素暂停，使用原FSM/部分配置；恢复普通态再恢复最后成功值。运行禁用下一TIM6恢复baseline，停车禁用且overridden时立即恢复共享值；保留last_success供保存；保护停车Tuning_Stop恢复共享基线并禁用覆盖，last_success保留；会话变更只取消未应用所有权而不回滚已应用值。
5. 停车先完成日志收尾，再FinalizeStop合入last_success，ServiceSave尝试保存并即刻Synchronize。TuningApplication_Synchronize先发布Telemetry_SetRevision，再把业务事件适配到通信队列，不能将顺序反过来或延至后续采样后。

baseline、pending、last_success、effective是不同职责的对象；getter不是通用原子快照，前台上下文复制使用原短临界区，TIM6/TIM15同级不互抢约束不变。运行覆盖与[持久参数](parameters.md#parameters)、[状态部分配置](../control/profiles.md#f12)协作，不代表Phase10全局调参整改完成。

本次源码核对 tuning.c/tuning_values.c、system/tuning_application.c 和 command_dispatch.c，保留原完整拍、单槽所有权及版本发布顺序；未运行已有软件验证、未声称实车调参可用。范围限值是当前代码值，其调校依据和对端实测仍按原阶段待验。

### 精确门禁与版本边界

PrepareSet先校验mode/gear，再拒绝pending或final未消费；运行且未started或control_ticks≤1000返回BUSY；随后依次校验当前目标及fan_idle不变、ordinary、enabled。进入下一控制拍后SelectControl还要复核allowed、started及无protection_flags，所以RECEIVED可能最终拒绝。

运行版本增加后若为0再加一次，停车CommitParked只做自然uint32加一（可回绕0）；本次如实保留这一区别，不以协议序号临界重建规则替代参数revision行为。Tuning_Equal逐字段数值比较，+0与-0相等；状态暂停/恢复仅实际值变化才推进版本和事件，收到完整SET的staged提交单独推进。
