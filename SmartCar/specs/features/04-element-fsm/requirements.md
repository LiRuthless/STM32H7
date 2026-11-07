# Phase 04: 赛道元素状态机（现状） — 需求

> **2026-10-16 增量状态**：[Phase 14](../14-dual-mode-tracking/requirements.md) FR-09/10/11 已将电磁主/环岛状态收拢为独立上下文，光电模式下继续推进并记录，禁止实际控制/清零副作用；光电四态只覆盖普通循迹与保护。原已启用转移及同拍调用顺序保留，休眠元素仍不接通。改动与验收见 [Phase 14 计划](../14-dual-mode-tracking/plan.md) / [验证](../14-dual-mode-tracking/validation.md)；下文历史状态编号与阈值仍为电磁基准。 2026-10-16实现状态及V-01～15通过证据见上述Phase14验证页，V-16～20仍待上机。

> **as-built 规约**：以下需求逆向自现有代码，描述系统当前实际行为。注意 FR-8~FR-11 描述的是"代码存在但不可达"的现状，而非目标行为。

## 功能需求

- FR-1: 系统必须提供 7 态 kernel 主状态机（TRACKING/ISLAND_L/ISLAND_R/TEETERBOARD/CROSSROADS/CASK/REISLAND），以 2 ms 控制拍执行：入态覆写权重/方向增益/base_speed → 循迹偏差 → 方向 PID → 差速分配 → 本态元素进出判断。
- FR-2: 出赛道保护：4 路电感滤波值之和 ≤300 时，必须清 `g_app_key_flag`/`g_app_start_flag`/`g_app_run_flag`，目标速度归零，车辆停车并回到菜单态。
- FR-3: 十字进入：TRACKING 态下竖两路电感和 >2800 且竖向对称度 `symmetry_y` <25 时，必须转入 CROSSROADS；CROSSROADS 态方向输出必须限幅 ±10；竖两路和 <2800 时必须返回 TRACKING。
- FR-4: 预环岛：TRACKING 态下横两路电感和 >2800 时，必须按左右大小置 `g_roundabout_left_flag`/`g_roundabout_right_flag`，清零 `angle_x` 与三路里程，转入 REISLAND。
- FR-5: 右环岛进入：REISLAND 态下竖两路电感和 <300 且 `g_roundabout_right_flag`==1 且 `!g_control_cask_flag` 时，必须转入 ISLAND_R 并将环岛子状态机置为 ISLAND_LPREENTER，清零 `g_control_track_output`/`angle_x`/里程。
- FR-6: 预环岛超时退出：REISLAND 态行驶里程 >5000 仍未进环时，必须返回 TRACKING 并清双环岛标志、`g_control_track_output`、`angle_x`。
- FR-7: 环岛子状态机必须以编码器里程驱动完成"预入环直行（≥8000）→ 固定打角入环（>13000）→ 环内循迹（>32000）→ 出环直行（>10000）→ 回正常循迹"序列；入环打角为固定 `Motor_DistributeSpeed(-30)`（名为 TURN_LEFT，实际服务右环岛）。
- FR-8: 路障抑制：预环岛判断中 DL1B 距离 <100 mm 时必须置 `g_control_cask_flag`，从而抑制右环岛进入（现状：该逻辑内嵌于预环岛判断函数中）。
- FR-9: 左环岛（ISLAND_L）、跷跷板（TEETERBOARD）、路障（CASK）三态的处理代码存在，但其入态判断当前**不接入**主状态机，实际不可达（现状如实记录；接入排期 Phase 09）。
- FR-10: 实际可达的状态转移仅允许为：TRACKING→CROSSROADS→TRACKING；TRACKING→REISLAND→ISLAND_R→（环岛子状态机）→TRACKING；TRACKING→REISLAND→TRACKING（超时）。
- FR-11: 跷跷板/路障的出态判断（`Element_JudgeTeeterboardExit`/`Element_JudgeCaskExit`）随状态机保留编译，仅在对应态被（未来）进入时生效。

## 技术约束

- TC-1: 元素判断与状态转移全部在 TIM6 2 ms 控制拍内完成（硬性约束 2），阈值均按 12 bit ADC 标度（满量程 4095）与 2 ms 拍编码器计数标定。
- TC-2: 状态以 `#define` 宏表示（非枚举），与源工程保持一致；本阶段不改枚举化（归 Phase 10/11 治理）。
- TC-3: 环岛距离变量（`g_roundabout_enter_distance1`/`g_roundabout_out_distance1`）为模块内可调参数，单位与源工程一致（编码器计数）。
- TC-4: 未启用元素与休眠判断函数保留编译但不调用；接入任何元素前必须逐个与用户确认设计（AGENTS.md 硬性规则）。
- TC-5: 元素判断不得改变 [Phase 03](../03-track-following/requirements.md) 循迹链的对外语义（偏差公式、差速分配、占空比万分比）。

## 接口约定（如适用）

| 接口 | 方向 | 语义 |
|---|---|---|
| `g_control_kernel_state` | 输出（全局） | kernel 主状态机当前态（0~6，宏见 `control.h`） |
| `g_roundabout_state` | 输出（全局） | 环岛子状态机当前态（宏见 `roundabout.h`） |
| `g_roundabout_left_flag` / `g_roundabout_right_flag` | 内部 | 预环岛识别的左/右环岛标志，进环或超时后清零 |
| `g_control_cask_flag` | 内部 | 路障抑制标志：DL1B <100 mm 置 1，预环岛触发时清 0；阻止右环岛进入 |
| `g_roundabout_sign_round` | 只写不读（现状） | ISLAND_L=-1 / ISLAND_R=1，环岛子状态机未消费 |
| `g_roundabout_enter_distance1` / `g_roundabout_out_distance1` | 可调参数 | 预入环直行距离 8000 / 出环直行距离 10000（编码器计数） |
| 依赖输入 | — | `Track_GetState()` 的滤波值与对称度（Phase 03）；`Motor_GetState()` 的里程与 `Motor_ResetDistance()`（Phase 03）；`IMU_ZeroAngleX()`（Phase 05）；`g_app_dl1b_distance_mm`（BSP DL1B）；`g_app_control_ticks`（app.c 节拍计数） |

## 非目标（Non-goals）

- 本阶段**不**接入左环岛、跷跷板、路障元素（Phase 09 范围，接入前逐个与用户确认设计）。
- 本阶段**不**解决"双档参数 vs 状态机每拍覆写"的语义冲突（Phase 10 范围）。
- 不恢复源工程环岛角度法（已注释未移植）；不引入摄像头元素识别。
- 不清理环岛冗余宏与休眠判断函数（Phase 11 范围）。

> 2026-10-16 Phase 15 G2：现行传感器/控制符号按[完整映射](../15-code-style-consistency/symbol-map.md)迁移，
> 配置、计算顺序及休眠调用边保持；历史 validation 原文及本阶段实车门禁不变。

> 2026-10-16 Phase 15 G3：现行调度、HMI、参数和日志名称已按[符号表](../15-code-style-consistency/symbol-map.md)统一。
> 本文原阶段行为记录与未完成待办不因此重新验收；现行运行链以 Phase 14 和当前头文件契约为准，历史 validation 保留原文。

## Phase 17 当前位置（2026-10-23）

App按system/sensing/control/services/hmi归类。现行文件及逐函数位置见[迁移清单](../17-app-architecture/migration-map.md)。
历史路径、符号与验证结果保留；本说明不表示后续职责拆分或硬件验收完成。

> 2026-11-07 文档纠偏：本页恢复本阶段完整正文（恢复依据 `68cb508`），原路径、符号、决定和结果按当时基线解释。当前功能见 [Docs](../../../Docs/index.md)，后续演进见 [路线图](../../roadmap.md)。历史验证安排不自动成为后续每次改动的门禁；本次未重新验收固件或实机。


## 现行功能导航与稳定需求锚点

下列导航连接本阶段编号与当前说明；本阶段完整需求、设计、映射和验收以上文原正文为准，当前功能说明不覆盖阶段决定。

<a id="fr-01"></a>
### FR-1 主态结构

当前说明：[F09 活动电磁元素](../../../Docs/control/elements-and-dormant-code.md#f09)；[F12 部分配置与全局调参规划](../../../Docs/control/tracking-and-controllers.md#f12)；[F11 双FSM与唯一执行](../../../Docs/control/tracking-and-controllers.md#f11)。

<a id="fr-02"></a>
### FR-2 失线保护

当前说明：[F02 起停、模式与首因](../../../Docs/system/runtime-and-safety.md#f02)。

<a id="fr-03"></a>
### FR-3 十字

当前说明：[F09 活动电磁元素](../../../Docs/control/elements-and-dormant-code.md#f09)。

<a id="fr-04"></a>
### FR-4 预环

当前说明：[F09 活动电磁元素](../../../Docs/control/elements-and-dormant-code.md#f09)。

<a id="fr-05"></a>
### FR-5 右环进入

当前说明：[F09 活动电磁元素](../../../Docs/control/elements-and-dormant-code.md#f09)。

<a id="fr-06"></a>
### FR-6 预环超时

当前说明：[F09 活动电磁元素](../../../Docs/control/elements-and-dormant-code.md#f09)。

<a id="fr-07"></a>
### FR-7 右环阶段

当前说明：[F09 活动电磁元素](../../../Docs/control/elements-and-dormant-code.md#f09)；[F12 部分配置与全局调参规划](../../../Docs/control/tracking-and-controllers.md#f12)。

<a id="fr-08"></a>
### FR-8 入环抑制

当前说明：[F09 活动电磁元素](../../../Docs/control/elements-and-dormant-code.md#f09)。

<a id="fr-09"></a>
### FR-9 休眠元素

当前说明：[F13 legacy与未接入元素](../../../Docs/control/elements-and-dormant-code.md#f13)。

<a id="fr-10"></a>
### FR-10 活动转移集合

当前说明：[F09 活动电磁元素](../../../Docs/control/elements-and-dormant-code.md#f09)。

<a id="fr-11"></a>
### FR-11 休眠出态调用边界

当前说明：[F13 legacy与未接入元素](../../../Docs/control/elements-and-dormant-code.md#f13)。

<a id="tc-01"></a>
### TC-1 元素时序与休眠边界（第1项）

当前说明：[F09 活动电磁元素](../../../Docs/control/elements-and-dormant-code.md#f09)。

<a id="tc-02"></a>
### TC-2 元素时序与休眠边界（第2项）

当前说明：[F09 活动电磁元素](../../../Docs/control/elements-and-dormant-code.md#f09)。

<a id="tc-03"></a>
### TC-3 元素时序与休眠边界（第3项）

当前说明：[F09 活动电磁元素](../../../Docs/control/elements-and-dormant-code.md#f09)。

<a id="tc-04"></a>
### TC-4 元素时序与休眠边界（第4项）

当前说明：[F13 legacy与未接入元素](../../../Docs/control/elements-and-dormant-code.md#f13)。

<a id="tc-05"></a>
### TC-5 元素时序与休眠边界（第5项）

当前说明：[F08 方向角度速度与执行](../../../Docs/control/tracking-and-controllers.md#f08)。
