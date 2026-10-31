# Phase 09: 元素功能补齐 — 需求

2026-10-31文档迁移：**Phase → docs → 代码**。本页保留需求/任务/验收历史，不新增阶段、不修改实现或重新验收。

当前需求导航：[P09功能索引](../../../Docs/traceability.md#p09)。具体输入输出、状态/错误/时序、代码符号与验证方法由下列docs条目唯一维护。

## 需求意图与现行功能条目

编号身份及原用户决定保留；摘要不替代详细契约。涉及冲突时，以docs中并列的当前事实、既有要求和待决定边界阅读，不能自动选代码或旧文档为新设计。

<a id="fr-01"></a>
### FR-1 目标状态可达

目标与实现依据：[F13 legacy与未接入元素](../../../Docs/control/elements-and-dormant-code.md#f13)；[F09 活动电磁元素](../../../Docs/control/elements-and-dormant-code.md#f09)。状态：规划/待逐元素决定。

<a id="fr-02"></a>
### FR-2 左环规划

目标与实现依据：[F13 legacy与未接入元素](../../../Docs/control/elements-and-dormant-code.md#f13)。状态：规划/待逐元素决定。

<a id="fr-03"></a>
### FR-3 跷跷板规划

目标与实现依据：[F13 legacy与未接入元素](../../../Docs/control/elements-and-dormant-code.md#f13)。状态：规划/待逐元素决定。

<a id="fr-04"></a>
### FR-4 路障规划

目标与实现依据：[F13 legacy与未接入元素](../../../Docs/control/elements-and-dormant-code.md#f13)。状态：规划/待逐元素决定。

<a id="fr-05"></a>
### FR-5 既有元素保持

目标与实现依据：[F09 活动电磁元素](../../../Docs/control/elements-and-dormant-code.md#f09)。状态：规划/待逐元素决定。

<a id="fr-06"></a>
### FR-6 新增状态记录

目标与实现依据：[F19 日志格式容量](../../../Docs/services/recording-and-export.md#f19)；[F13 legacy与未接入元素](../../../Docs/control/elements-and-dormant-code.md#f13)。状态：规划/待逐元素决定。

<a id="tc-01"></a>
### TC-1 元素实施前置约束（第1项）

目标与实现依据：[F13 legacy与未接入元素](../../../Docs/control/elements-and-dormant-code.md#f13)。状态：规划/待逐元素决定。

<a id="tc-02"></a>
### TC-2 元素实施前置约束（第2项）

目标与实现依据：[F13 legacy与未接入元素](../../../Docs/control/elements-and-dormant-code.md#f13)。状态：规划/待逐元素决定。

<a id="tc-03"></a>
### TC-3 元素实施前置约束（第3项）

目标与实现依据：[F12 部分配置与全局调参规划](../../../Docs/control/tracking-and-controllers.md#f12)。状态：规划/待逐元素决定。

<a id="tc-04"></a>
### TC-4 元素实施前置约束（第4项）

目标与实现依据：[F32 标准R08与维护](../../../Docs/guides/validation.md#f32)。状态：规划/待逐元素决定。

## 授权与完成边界

本轮仅迁移说明。历史用户决定、动机、来源、兼容性及未决事项完整保留在下方基线；新的行为决定须明确日期、理由和适用范围，更新本需求及docs目标后独立提交，再进入实现。不得用本次文档完成替代AC5、R07原样本或任何硬件待验。

## 冻结历史基线（68cb508，非现行契约）

以下原文用于保留阶段决定、组顺序、需求历史与真实验证；不继续独立维护算法/布局/源码映射。当前契约先查上方docs条目。旧“本轮”“待执行”“当前”只指原记录时点，不覆盖后续决定。历史任务的全量软件门禁不自动成为以后每次修改的要求。

<details>
<summary>展开原始阶段记录（原文保留）</summary>

# Phase 09: 元素功能补齐 — 需求

> 标注"待确认"的条目以 [plan](plan.md) 各 Group 开工前的用户确认结论为准，确认后回填本条目的具体数值。

## 功能需求

- FR-1: kernel 主状态机 7 个状态均可达：`CONTROL_KERNEL_ISLAND_L`、`CONTROL_KERNEL_TEETERBOARD`、`CONTROL_KERNEL_CASK` 存在明确的进入与离开路径，不再有死态。
- FR-2: 左环岛：检测到左环特征后进入 `CONTROL_KERNEL_ISLAND_L` 分支，完成预入环→打角入环→环内循迹→出环的完整流程，随后返回正常循迹；打角方向与距离门限与右环岛对称（具体值待确认）。
- FR-3: 跷跷板：进入判断（草拟：4 路电感和 < 800）与离开判断（草拟：和 > 1000，或进入后超时 300 拍）接入主状态机；`g_element_enter_tick` 类型修正为 `int32_t`，超时判断在任意运行时长下正确。
- FR-4: 路障：进入判断（草拟：`angle_y > 60`）与离开判断（草拟：`angle_y < 20`）接入主状态机；`g_control_cask_flag` 职责拆分——"环岛入环抑制"与"路障元素检测"为两个独立标志，各自命名与语义单一。
- FR-5: 元素接入期间，已验证的十字与右环岛行为不发生变化（回归基准见 [validation](validation.md)）。
- FR-6: 每个元素接入后，数据记录的 `g_control_kernel_state` / `g_roundabout_state` 字段能反映新状态（依赖 Phase 08 存储链可用）。

## 技术约束

- TC-1: 不引入新传感器；判断仅使用现有 4 路电感、IMU 姿态角、DL1B 距离、编码器距离。
- TC-2: 判断函数仍挂接在 2ms 控制拍路径上，单拍开销不得显著增加（无阻塞、无浮点除零风险）。
- TC-3: 新分支的增益取值遵循现有"每态一组增益"的结构（表驱动化留给 [Phase 10](../10-control-consistency/requirements.md)），数值初值沿用源码草拟值并上机标定。
- TC-4: 遵守 [techstack](../../techstack.md) 全部硬性约束；状态值变化需同步 [Phase 04](../04-element-fsm/requirements.md) 规约。

## 接口约定

- 状态机接口：`g_control_kernel_state`（`control.h`）取值集合不变（7 态），本阶段只补转移路径，不改状态编号。
- 环岛子状态机：`g_roundabout_state` 取值集合（`roundabout.h`）允许裁剪闲置宏或按左环需要扩展；若裁剪，`datalog` 导出字段含义同步说明。
- `g_control_cask_flag` 拆分后的新标志命名与语义在 Group 3 确认后回填本节。
- 判断阈值参数（电感和门限、角度门限、距离门限、超时拍数）以模块内常量形式集中，便于后续接入调参体系。

## 非目标（Non-goals)

- 不改变已验证的十字/右环岛行为与参数。
- 不做"状态机增益表 vs 用户调参"的语义统一（[Phase 10](../10-control-consistency/requirements.md)）。
- 不清理与元素无关的休眠代码（[Phase 11](../11-hardening-cleanup/plan.md) Group 4）；仅 `g_control_cask_flag` 拆分与 `ISLAND_*` 闲置宏的元素侧处置属本阶段。
- 不引入摄像头、超声等新传感器，不改动硬件。

> 2026-10-16 Phase 15 G2：现行传感器/控制符号按[完整映射](../15-code-style-consistency/symbol-map.md)迁移，
> 配置、计算顺序及休眠调用边保持；历史 validation 原文及本阶段实车门禁不变。

</details>
