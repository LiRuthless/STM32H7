# Phase 08: 存储链修复与风扇参数接通 — 实现计划

2026-10-31文档迁移：**Phase → docs → 代码**。本页保留需求/任务/验收历史，不新增阶段、不修改实现或重新验收。

当前需求导航：[P08功能索引](../../../Docs/traceability.md#p08)。具体输入输出、状态/错误/时序、代码符号与验证方法由下列docs条目唯一维护。

## 任务分组与详细设计入口

历史分组/依赖/风险/回退和用户决定保留在下方原记录；本次未重开原实现任务。以后执行本阶段增量时，先确定对应需求和docs目标，按原前置及已确认决定分组；实现文件与符号不在本页复制维护。

| 原需求 | 详细契约/实现/验证入口 | 执行边界 |
|---|---|---|
| FR-1 Flash初始化 | [F17 迁移保存与失败](../../../Docs/services/parameters-and-tuning.md#f17)；[F01 初始化与三线调度](../../../Docs/system/runtime-and-safety.md#f01) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-2 镜像恢复 | [F17 迁移保存与失败](../../../Docs/services/parameters-and-tuning.md#f17) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-3 保存结果 | [F17 迁移保存与失败](../../../Docs/services/parameters-and-tuning.md#f17) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-4 静默及起跑保存 | [F17 迁移保存与失败](../../../Docs/services/parameters-and-tuning.md#f17)；[F15 多入口参数应用](../../../Docs/services/parameters-and-tuning.md#f15)；[F02 起停、模式与首因](../../../Docs/system/runtime-and-safety.md#f02) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-5 日志可用性 | [F20 队列前台写入与故障](../../../Docs/services/recording-and-export.md#f20)；[F02 起停、模式与首因](../../../Docs/system/runtime-and-safety.md#f02) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-6 镜像迁移 | [F16 参数v3格式](../../../Docs/services/parameters-and-tuning.md#f16)；[F17 迁移保存与失败](../../../Docs/services/parameters-and-tuning.md#f17) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-7 运行空闲风扇 | [F15 多入口参数应用](../../../Docs/services/parameters-and-tuning.md#f15)；[F02 起停、模式与首因](../../../Docs/system/runtime-and-safety.md#f02) | 当前代码与既有承诺冲突；待决定 |
| FR-8 风扇编辑 | [F14 菜单按键与显示](../../../Docs/hmi/operation.md#f14)；[F15 多入口参数应用](../../../Docs/services/parameters-and-tuning.md#f15) | 当前契约及历史替代见docs；不改变原验收状态 |

原分组名（不改变次序/勾选，完整组内容见冻结记录）：

- Group 1：参数镜像 v2 与 Flash 故障处理
- Group 2：串口参数延迟合并保存
- Group 3：双档运行/空闲风扇与五行菜单
- Group 4：文档与验证

执行顺序：先读需求→对应docs→检查未决/依赖→纯规约提交→实现→所选验证及记录。回退以该次完整组为单位，同步回退接口/调用/文档，历史兼容和保护边界不借回退改变。纯文档检查不触发旧组软件门禁。









## 冻结历史基线（68cb508，非现行契约）

以下原文用于保留阶段决定、组顺序、需求历史与真实验证；不继续独立维护算法/布局/源码映射。当前契约先查上方docs条目。旧“本轮”“待执行”“当前”只指原记录时点，不覆盖后续决定。历史任务的全量软件门禁不自动成为以后每次修改的要求。

<details>
<summary>展开原始阶段记录（原文保留）</summary>

# Phase 08: 存储链修复与风扇参数接通 — 实现计划

## 目标与顺序

接通 W25Q64 参数/日志链路、保证串口调参持久化，并使双档风扇运行/空闲参数实际控制 PWM。用户于 2026-10-03 明确要求先实施本阶段；Phase 12 V-9/V-10 台架验证仍待执行，路线图保留该事实并记录本次顺序调整。

## Group 1：参数镜像 v2 与 Flash 故障处理

- `App/param.c/.h`：定义 v1 兼容结构与 v2 当前结构；先读头部区分版本，再分别读/校验。有效 v1 镜像逐字段迁移，旧两个 `fan_duty` 映射为运行值，新增 idle 值 1100，写回 v2。默认值为两档运行 1600、idle 1100。未知版本、读取或校验失败载入默认；所有读/写错误经 USART1 报告。`Param_Load()` 返回 Flash 可用状态供 App 决定是否启用日志。
- `Param_Save()` 收集速度环、当前档方向/速度、当前档运行/空闲风扇和菜单状态；检查 BSP 写结果并报告失败。
- `App/app.c::App_Init` 在 UART 后、参数加载前初始化 Flash。Flash 不可用时继续初始化控制系统，但屏蔽日志启动并向串口报告。

## Group 2：串口参数延迟合并保存

- `App/param.c/.h` 增加 dirty 标记和最后修改 tick；`Param_MarkDirty()` 记录修改时刻，`Param_ServiceSave()` 在静默满 3000ms 后保存，`Param_FlushPending()` 在起跑前立即保存。
- `App/wireless.c::Wireless_Adjust` 仅在既有合法参数命令命中后标脏，保持命令格式、数值解释和成功回显不变。
- `App/app.c::App_Loop` 在停车态处理延迟保存；检测启动键后先刷新待保存参数，再启动日志和定时器。保存失败提示后继续起跑。
- 菜单松键/BACK 保存继续走 `Param_Save()`；保存尝试结束后清除 pending，避免循环重复擦写。

## Group 3：双档运行/空闲风扇与五行菜单

- `App/motor.c/.h`：保留 `fan_duty` 作为当前档运行值，新增 `fan_duty_idle`；参数档切换同时分发两个值。
- `App/app.c`：初始化及停车/安全停车使用 `fan_duty_idle`，运行态使用 `fan_duty`；移除固定宏输出路径，但保留 1600/1100 作为参数默认值。
- `App/menu.c`：不增加行数；第 5 行用 `FAN-R`/`FAN-I` 标明所选运行/空闲值。该行按 OK 切换选择，LEFT/RIGHT 以 100 步进并钳位 0～10000。保留现有 UP/DOWN 光标导航与五项循环。

## Group 4：文档与验证

- 同步 Phase 06 的菜单按键、双档参数镜像 v2 与串口持久化说明；同步 README、Phase 08 requirements/plan/validation、roadmap 和 CHANGELOG。
- 执行 Phase 08 自动化验证；人工硬件项未执行时标记“待上机”，不推断通过。

## 风险与回退

- v1 迁移写回失败时 RAM 保留已验证的旧参数及 idle 默认值，但参数 Flash 后续写入与日志关闭；串口报告错误。恢复 Flash 后重新上电重试迁移。
- v2 镜像只追加字段、不改旧字段偏移；CRC 覆盖范围延伸至新增字段。若出现未知版本或 CRC 无效，使用当前 v2 默认值并报告。
- 串口延迟窗口内掉电仍会丢失尚未写入的改动；按启动键会先写入，缩小该窗口。

> 2026-10-16 Phase 15 G3：现行调度、HMI、参数和日志名称已按[符号表](../15-code-style-consistency/symbol-map.md)统一。
> 本文原阶段行为记录与未完成待办不因此重新验收；现行运行链以 Phase 14 和当前头文件契约为准，历史 validation 保留原文。

## Phase 17 当前实现定位（2026-10-24）

参数版本定义和断言归services/parameter_image.h，镜像/CRC/默认/迁移/失败状态归parameter_store，四方向bank归parameter_bank，PID/风扇/界面应用归system/parameter_application。Param_*业务调用通过实际owner头声明；vp/vi仍只改活动PI并标脏，flush才捕获镜像，3秒边界和v1/v2到v3字节布局保持。

本节是现行归属导航；上文历史计划、验证、提交路径及未完成实机项目保持。实际重构验证见
[Phase17验收](../17-app-architecture/validation.md)，逐符号迁移见[迁移导航](../17-app-architecture/migration-map.md)。

</details>
