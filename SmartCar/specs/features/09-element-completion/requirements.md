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

> 2026-11-07 文档纠偏：本页恢复本阶段完整正文（恢复依据 `68cb508`），原路径、符号、决定和结果按当时基线解释。当前功能见 [Docs](../../../Docs/index.md)，后续演进见 [路线图](../../roadmap.md)。历史验证安排不自动成为后续每次改动的门禁；本次未重新验收固件或实机。


## 现行功能导航与稳定需求锚点

下列导航连接本阶段编号与当前说明；本阶段完整需求、设计、映射和验收以上文原正文为准，当前功能说明不覆盖阶段决定。

<a id="fr-01"></a>
### FR-1 目标状态可达

当前说明：[F13 legacy与未接入元素](../../../Docs/control/elements-and-dormant-code.md#f13)；[F09 活动电磁元素](../../../Docs/control/elements-and-dormant-code.md#f09)。

<a id="fr-02"></a>
### FR-2 左环规划

当前说明：[F13 legacy与未接入元素](../../../Docs/control/elements-and-dormant-code.md#f13)。

<a id="fr-03"></a>
### FR-3 跷跷板规划

当前说明：[F13 legacy与未接入元素](../../../Docs/control/elements-and-dormant-code.md#f13)。

<a id="fr-04"></a>
### FR-4 路障规划

当前说明：[F13 legacy与未接入元素](../../../Docs/control/elements-and-dormant-code.md#f13)。

<a id="fr-05"></a>
### FR-5 既有元素保持

当前说明：[F09 活动电磁元素](../../../Docs/control/elements-and-dormant-code.md#f09)。

<a id="fr-06"></a>
### FR-6 新增状态记录

当前说明：[F19 日志格式容量](../../../Docs/services/recording-and-export.md#f19)；[F13 legacy与未接入元素](../../../Docs/control/elements-and-dormant-code.md#f13)。

<a id="tc-01"></a>
### TC-1 元素实施前置约束（第1项）

当前说明：[F13 legacy与未接入元素](../../../Docs/control/elements-and-dormant-code.md#f13)。

<a id="tc-02"></a>
### TC-2 元素实施前置约束（第2项）

当前说明：[F13 legacy与未接入元素](../../../Docs/control/elements-and-dormant-code.md#f13)。

<a id="tc-03"></a>
### TC-3 元素实施前置约束（第3项）

当前说明：[F12 部分配置与全局调参规划](../../../Docs/control/tracking-and-controllers.md#f12)。

<a id="tc-04"></a>
### TC-4 元素实施前置约束（第4项）

当前说明：[F32 标准R08与维护](../../../Docs/guides/validation.md#f32)。
