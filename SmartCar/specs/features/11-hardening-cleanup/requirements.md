# Phase 11: 健壮性与休眠代码清理 — 需求

2026-10-31文档迁移：**Phase → docs → 代码**。本页保留需求/任务/验收历史，不新增阶段、不修改实现或重新验收。

当前需求导航：[P11功能索引](../../../docs/traceability.md#p11)。具体输入输出、状态/错误/时序、代码符号与验证方法由下列docs条目唯一维护。

## 需求意图与现行功能条目

编号身份及原用户决定保留；摘要不替代详细契约。涉及冲突时，以docs中并列的当前事实、既有要求和待决定边界阅读，不能自动选代码或旧文档为新设计。

<a id="fr-01"></a>
### FR-1 有界Flash等待

目标与实现依据：[F17 迁移保存与失败](../../../docs/services/parameters-and-tuning.md#f17)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-02"></a>
### FR-2 静态影子写

目标与实现依据：[F17 迁移保存与失败](../../../docs/services/parameters-and-tuning.md#f17)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-03"></a>
### FR-3 零分母

目标与实现依据：[F05 电感偏差与对称度](../../../docs/sensing/sensors-and-calibration.md#f05)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-04"></a>
### FR-4 休眠计时类型

目标与实现依据：[F13 legacy与未接入元素](../../../docs/control/elements-and-dormant-code.md#f13)。状态：保留未完成/待决定边界。

<a id="fr-05"></a>
### FR-5 数字校验

目标与实现依据：[F15 多入口参数应用](../../../docs/services/parameters-and-tuning.md#f15)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-06"></a>
### FR-6 注释事实

目标与实现依据：[F19 日志格式容量](../../../docs/services/recording-and-export.md#f19)；[F01 初始化与三线调度](../../../docs/system/runtime-and-safety.md#f01)；[F20 队列前台写入与故障](../../../docs/services/recording-and-export.md#f20)；[F28 DL1B阻塞边界](../../../docs/platform/board-and-bsp.md#f28)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-07"></a>
### FR-7 命名承接

目标与实现依据：[F32 标准R08与维护](../../../docs/guides/validation.md#f32)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-08"></a>
### FR-8 按键表驱动规划

目标与实现依据：[F14 菜单按键与显示](../../../docs/hmi/operation.md#f14)。状态：保留未完成/待决定边界。

<a id="fr-09"></a>
### FR-9 字库存储规划

目标与实现依据：[F29 时钟DMA采样与驱动](../../../docs/platform/board-and-bsp.md#f29)。状态：保留未完成/待决定边界。

<a id="fr-10"></a>
### FR-10 DMA地址评估

目标与实现依据：[F29 时钟DMA采样与驱动](../../../docs/platform/board-and-bsp.md#f29)。状态：保留未完成/待决定边界。

<a id="fr-11"></a>
### FR-11 逐项休眠处置

目标与实现依据：[F13 legacy与未接入元素](../../../docs/control/elements-and-dormant-code.md#f13)。状态：保留未完成/待决定边界。

<a id="tc-01"></a>
### TC-1 健壮性实施边界（第1项）

目标与实现依据：[F08 方向角度速度与执行](../../../docs/control/tracking-and-controllers.md#f08)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="tc-02"></a>
### TC-2 健壮性实施边界（第2项）

目标与实现依据：[F29 时钟DMA采样与驱动](../../../docs/platform/board-and-bsp.md#f29)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="tc-03"></a>
### TC-3 健壮性实施边界（第3项）

目标与实现依据：[F17 迁移保存与失败](../../../docs/services/parameters-and-tuning.md#f17)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="tc-04"></a>
### TC-4 健壮性实施边界（第4项）

目标与实现依据：[F32 标准R08与维护](../../../docs/guides/validation.md#f32)。状态：当前契约及历史替代见docs；不改变原验收状态。

## 授权与完成边界

本轮仅迁移说明。历史用户决定、动机、来源、兼容性及未决事项完整保留在下方基线；新的行为决定须明确日期、理由和适用范围，更新本需求及docs目标后独立提交，再进入实现。不得用本次文档完成替代AC5、R07原样本或任何硬件待验。

## 冻结历史基线（68cb508，非现行契约）

以下原文用于保留阶段决定、组顺序、需求历史与真实验证；不继续独立维护算法/布局/源码映射。当前契约先查上方docs条目。旧“本轮”“待执行”“当前”只指原记录时点，不覆盖后续决定。历史任务的全量软件门禁不自动成为以后每次修改的要求。

<details>
<summary>展开原始阶段记录（原文保留）</summary>

# Phase 11: 健壮性与休眠代码清理 — 需求

> **2026-10-16 命名治理转接**：用户确认全量自维护名称统一，FR-7 由
> [Phase 15](../15-code-style-consistency/requirements.md) 承接，目标名及引用见其
> [符号表](../15-code-style-consistency/symbol-map.md)，不再等待 Phase 09/10 完成。
> 本轮仅提交文档，FR-7 尚未实施或验收；FR-4/8/9/10/11、休眠处置和旧验证记录保持原边界。

> **2026-10-16 关联边界**：[Phase 14](../14-dual-mode-tracking/requirements.md) FR-08/16/18/19/20 增加独立于SysTick的采集/控制预算及日志故障锁存，将Flash刷写移到主循环以保留TIM7保护响应；不是本阶段全部健壮性或休眠清理已经完成。不得借双模式启用休眠元素/PID；具体范围见 [Phase 14 计划](../14-dual-mode-tracking/plan.md) / [验证](../14-dual-mode-tracking/validation.md)。 2026-10-16实现状态及V-01～15通过证据见上述Phase14验证页，V-16～20仍待上机。

## 功能需求

### 健壮性（对应 plan Group 1）

- FR-1: `BSP_W25Q64_WaitIdle(timeout_ms)` 从调用时的 `HAL_GetTick()` 起按无符号差值计时；页编程 10 ms、4 KB 擦除 500 ms、64 KB 擦除 2500 ms、整片擦除 110000 ms。每轮读取状态寄存器，WIP 清零返回 0；持续忙至各自上限或任一 SPI 传输失败立即返回 1。`BSP_W25Q64_ReadStatus1`、`BSP_W25Q64_EnableWrite`、初始化、读、页编程和各级擦除均检查 HAL SPI 返回状态；片选在成功或失败后均拉高，失败向现有 `BSP_W25Q64_*` 的 0/1 接口及 Flash 服务层传播，不继续发送该操作的后续命令。地址、布局、正常数据内容和函数签名不变。整片擦除是未被运行链调用的维护 API，仅在看门狗启动前且非 ISR 上下文调用；已启动的 IWDG 约 6 秒超时，无法等待最长 110 秒整片擦除。
- FR-2: `BSP_Flash_Write(offset, buf, len)` 在已有 `s_param_shadow[4096]` 读取成功后，先比较影子缓冲的 `[offset, offset+len)` 与输入；完全相同时立即返回 0，不擦写。存在差异时才将输入复制进影子缓冲并擦除、回写原参数扇区全部 16 页。读取失败返回 1，不擦写；禁止再分配 4KB 栈缓冲或为比较重复读取整扇区。输入边界与现有返回约定保持不变。
- FR-3: `track_sensor.c` 的 `Track_UpdateError()` 和 `Track_UpdateSymmetry()` 分别计算横向、纵向电感和；某一对和为 0 时对应 `symmetry_x` 或 `symmetry_y` 置 0（表示该对无差值），另一对仍按原式独立计算。两对都为 0 时两个对称度均为 0，`Track_UpdateError()` 的主分母为 0 时仍返回偏差 0。非零输入的权重、取整、返回类型及状态机判定条件不变。
- FR-4: `g_element_enter_tick` 与 `g_app_control_ticks` 类型一致（int32_t 语义），任意运行时长下跷跷板超时判断正确（若 Phase 09 已处理，本项为核对关闭）。
- FR-5: `Wireless_Adjust()` 对现有 4～7 字节调参帧，在计算数值或写入 `kp_v/ki_v/kp_x/kd_x` 前，逐一确认 `g_app_uart_rx_buffer[2..len-2]` 都在 ASCII `'0'`～`'9'`。任一字节不合法时清除已读取帧并直接返回，不改参数、不调用 `Param_MarkDirty()`、不发送正常回显。合法帧仍按现有 0.01 缩放及 `vp/vi/xp/xd` 命令格式处理；单字母 `d/c` 和其他长度的既有处理不变。

### 注释漂移修正（对应 plan Group 2）

- FR-6: [plan](plan.md) Group 2 表中所列全部注释条目逐条核对并改至与代码实际一致（容量 262016 条、TIM15 生产、几十 ms 擦除、看门狗约 6s、DL1B 2ms、编码器 1ms、TIM7 职责等）。

### 命名与结构（对应 plan Group 3）

- FR-7（由 Phase 15 实现）：自维护宏、全局、私有对象与函数按[完整映射](../15-code-style-consistency/symbol-map.md)统一模块命名，同步声明、绑定引用和测试。G1/G2/G3 自动门禁已通过，AC5/实车待执行；本项不接入休眠代码，不承接 FR-4/8/9/10/11。
- FR-8: `Menu_ScanKey` 区间判定表驱动化，判定结果与整改前逐点一致。
- FR-9: `font.h` 不再含非 static 的数组定义（改 static 或拆 .c），多 TU 包含无多重定义风险。
- FR-10: `bsp_adc.c` 绝对地址写法的工具链耦合完成评估，结论（维持/收敛）回填规约；链接行为不变。

### 休眠代码（对应 plan Group 4）

- FR-11: [休眠代码清单](../../research/2026-10-02-dormant-code-inventory.md) 中每一项有明确的处置结论（保留 / 接入 / 删除）并已落实；保留项须有注释说明保留理由。

## 技术约束

- TC-1: **行为等价硬约束**：除 FR-1/FR-3/FR-4/FR-5 列明的故障路径改善外，不改变任何正常运行时行为与控制参数数值。
- TC-2: 不修改 ST vendor 组件内部逻辑（`BSP/lcd/st7735*`、`Drivers/`）；`font.h` 属移植字库，允许 FR-9 的存储类调整。
- TC-3: 超时取值覆盖仓库内《W25Q64JV-DTR Revision J》第 90 页（PDF 第 91 页）交流特性表最大值：tPP=3 ms、tSE=400 ms、tBE2=2000 ms、tCE=100 s；FR-1 的分项上限分别加余量。当前 SysTick 优先级 0、TIM7 优先级 1，TIM7 日志写入时 `HAL_GetTick()` 可推进；不得改中断优先级。
- TC-4: 遵守 [techstack](../../techstack.md) 全部硬性约束；命名治理后构建零警告。

## 接口约定

- 2026-10-03 用户确认 FR-1 使用 10/500/2500/110000 ms 分项超时；依据仓库内 W25Q64JV 数据手册最大值及余量，保持 0/1 接口。
- 2026-10-03 用户确认 FR-3 某对电感和为 0 时其对称度取 0，表示无差值；适用于 `Track_UpdateError` 与 `Track_UpdateSymmetry` 两处。
- 重命名后的标识符映射表在 Group 3 实施时回填本节（供 CHANGELOG 与规约同步引用）。
- 休眠代码每项处置结论回填清单文档并同步本节链接。

## 非目标（Non-goals）

- 不改变任何控制参数数值与正常运行时行为（纯结构与健壮性整改）。
- 不动 ST vendor 组件内部逻辑；不升级工具链与 HAL 版本。
- 不在本阶段"接入"休眠功能（接入结论转独立 Phase）。
- 不重排存储布局、不改 CSV 格式、不改菜单交互。

> 2026-10-16 Phase 15 G2：现行传感器/控制符号按[完整映射](../15-code-style-consistency/symbol-map.md)迁移，
> 配置、计算顺序及休眠调用边保持；历史 validation 原文及本阶段实车门禁不变。

> 2026-10-16 Phase 15 G3：现行调度、HMI、参数和日志名称已按[符号表](../15-code-style-consistency/symbol-map.md)统一。
> 本文原阶段行为记录与未完成待办不因此重新验收；现行运行链以 Phase 14 和当前头文件契约为准，历史 validation 保留原文。

</details>
