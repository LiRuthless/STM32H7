# Phase 10: 调参语义与状态机一致性 — 需求

2026-10-31文档迁移：**Phase → docs → 代码**。本页保留需求/任务/验收历史，不新增阶段、不修改实现或重新验收。

当前需求导航：[P10功能索引](../../../Docs/traceability.md#p10)。具体输入输出、状态/错误/时序、代码符号与验证方法由下列docs条目唯一维护。

## 需求意图与现行功能条目

编号身份及原用户决定保留；摘要不替代详细契约。涉及冲突时，以docs中并列的当前事实、既有要求和待决定边界阅读，不能自动选代码或旧文档为新设计。

<a id="fr-01"></a>
### FR-1 全局合成策略

目标与实现依据：[F12 部分配置与全局调参规划](../../../Docs/control/tracking-and-controllers.md#f12)；[F18 完整拍临时调参](../../../Docs/services/parameters-and-tuning.md#f18)。状态：规划/待决定，不计已实现。

<a id="fr-02"></a>
### FR-2 运行参数生效

目标与实现依据：[F12 部分配置与全局调参规划](../../../Docs/control/tracking-and-controllers.md#f12)；[F15 多入口参数应用](../../../Docs/services/parameters-and-tuning.md#f15)；[F18 完整拍临时调参](../../../Docs/services/parameters-and-tuning.md#f18)。状态：规划/待决定，不计已实现。

<a id="fr-03"></a>
### FR-3 基准保持

目标与实现依据：[F12 部分配置与全局调参规划](../../../Docs/control/tracking-and-controllers.md#f12)；[F08 方向角度速度与执行](../../../Docs/control/tracking-and-controllers.md#f08)。状态：规划/待决定，不计已实现。

<a id="fr-04"></a>
### FR-4 表驱动

目标与实现依据：[F12 部分配置与全局调参规划](../../../Docs/control/tracking-and-controllers.md#f12)。状态：结构部分已承接；全局目标待决定。

<a id="fr-05"></a>
### FR-5 状态类型规划

目标与实现依据：[F12 部分配置与全局调参规划](../../../Docs/control/tracking-and-controllers.md#f12)。状态：规划/待决定，不计已实现。

<a id="tc-01"></a>
### TC-1 合成策略与格式兼容（第1项）

目标与实现依据：[F12 部分配置与全局调参规划](../../../Docs/control/tracking-and-controllers.md#f12)。状态：规划/待决定，不计已实现。

<a id="tc-02"></a>
### TC-2 合成策略与格式兼容（第2项）

目标与实现依据：[F12 部分配置与全局调参规划](../../../Docs/control/tracking-and-controllers.md#f12)。状态：规划/待决定，不计已实现。

<a id="tc-03"></a>
### TC-3 合成策略与格式兼容（第3项）

目标与实现依据：[F16 参数v3格式](../../../Docs/services/parameters-and-tuning.md#f16)。状态：规划/待决定，不计已实现。

<a id="tc-04"></a>
### TC-4 合成策略与格式兼容（第4项）

目标与实现依据：[F19 日志格式容量](../../../Docs/services/recording-and-export.md#f19)。状态：规划/待决定，不计已实现。

<a id="tc-05"></a>
### TC-5 合成策略与格式兼容（第5项）

目标与实现依据：[F32 标准R08与维护](../../../Docs/guides/validation.md#f32)。状态：规划/待决定，不计已实现。

## 授权与完成边界

本轮仅迁移说明。历史用户决定、动机、来源、兼容性及未决事项完整保留在下方基线；新的行为决定须明确日期、理由和适用范围，更新本需求及docs目标后独立提交，再进入实现。不得用本次文档完成替代AC5、R07原样本或任何硬件待验。

## 冻结历史基线（68cb508，非现行契约）

以下原文用于保留阶段决定、组顺序、需求历史与真实验证；不继续独立维护算法/布局/源码映射。当前契约先查上方docs条目。旧“本轮”“待执行”“当前”只指原记录时点，不覆盖后续决定。历史任务的全量软件门禁不自动成为以后每次修改的要求。

<details>
<summary>展开原始阶段记录（原文保留）</summary>

# Phase 10: 调参语义与状态机一致性 — 需求

> **2026-10-16 关联边界**：[Phase 14](../14-dual-mode-tracking/requirements.md) FR-12/14/15 仅建立光电/电磁参数bank隔离、当前模式调参和v3保存；电磁状态表的每拍覆写数值与顺序继续保留，本阶段的电磁调参优先级问题没有因此解决。光电从自身参数bank控制，不套用电磁状态表。映射及验收见 [Phase 14 计划](../14-dual-mode-tracking/plan.md) / [验证](../14-dual-mode-tracking/validation.md)。 2026-10-16实现状态及V-01～15通过证据见上述Phase14验证页，V-16～20仍待上机。

> "用户调参如何与状态机增益叠加"的具体语义以 [plan](plan.md) Group 1 的用户确认结论为准，确认后回填 FR-1/FR-2。

## 功能需求

- FR-1: 全工程存在唯一的调参语义：任一控制增益在任意时刻的生效值可由一条明确规则推出（基准表 + 用户调参的叠加/替换关系），不存在"运行时被无条件覆写"的参数。
- FR-2: 菜单/串口对方向环与速度参数的修改，在起跑后的运行中按 FR-1 语义实际生效（可用串口改参前后实车行为或数据记录差异验证）。
- FR-3: 用户调参为零值/默认值时，运行行为与整改前逐拍一致（等价性基准）。
- FR-4: 各状态增益集中为表驱动结构；新增/修改一个状态的增益只改表的一行，不再散落多处抄写。
- FR-5: `KERNEL_*` 状态改为枚举类型；编译期能对非法状态值告警（配合 `-Wall`）。

## 技术约束

- TC-1: 不改变任何控制参数的数值与实际控制行为（FR-2 语义生效前的等价性为硬约束）。
- TC-2: 表查找与合成逻辑在 2ms 控制拍内的开销可忽略（无阻塞、无动态内存）。
- TC-3: 若双档存储语义变化，参数镜像通过 version 字段升级处理，不得静默错读旧数据（衔接 [Phase 08](../08-storage-chain-fix/requirements.md) 的 TC-1）。
- TC-4: 状态枚举化后，`datalog` 记录的 `g_control_kernel_state` 数值含义保持兼容或同步说明。
- TC-5: 遵守 [techstack](../../techstack.md) 全部硬性约束。

## 接口约定

- 增益表的结构与字段集合在 Group 2 设计后回填本节（预期字段：`weight_x/xx/y/abs`、`base_speed`、`kp_x/k2p_x/kd_x/k2d_x`，按状态索引）。
- 用户调参入口（菜单 ADJUST1/2、串口 `vp/vi/xp/xd`）的语义说明同步更新到 [Phase 06](../06-hmi-tuning/requirements.md)。
- 与 [Phase 09](../09-element-completion/requirements.md) 的接口：Phase 09 新增状态分支必须复用表驱动赋值，不得引入新的逐 case 抄写块。

## 非目标（Non-goals）

- 不调参：所有增益数值保持现状，本阶段是语义与结构整改。
- 不改状态机的状态集合、转移条件与元素判断逻辑（Phase 09 范围）。
- 不改 PID 算法本身（[Phase 03](../03-track-following/requirements.md) 范围）。
- 不引入新的调参通道或上位机协议。

> 2026-10-16 Phase 15 G2：现行传感器/控制符号按[完整映射](../15-code-style-consistency/symbol-map.md)迁移，
> 配置、计算顺序及休眠调用边保持；历史 validation 原文及本阶段实车门禁不变。

> 2026-10-16 Phase 15 G3：现行调度、HMI、参数和日志名称已按[符号表](../15-code-style-consistency/symbol-map.md)统一。
> 本文原阶段行为记录与未完成待办不因此重新验收；现行运行链以 Phase 14 和当前头文件契约为准，历史 validation 保留原文。

</details>
