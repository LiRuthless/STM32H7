# Phase 09: 元素功能补齐 — 实现计划

2026-10-31文档迁移：**Phase → docs → 代码**。本页保留需求/任务/验收历史，不新增阶段、不修改实现或重新验收。

当前需求导航：[P09功能索引](../../../docs/traceability.md#p09)。具体输入输出、状态/错误/时序、代码符号与验证方法由下列docs条目唯一维护。

## 任务分组与详细设计入口

历史分组/依赖/风险/回退和用户决定保留在下方原记录；本次未重开原实现任务。以后执行本阶段增量时，先确定对应需求和docs目标，按原前置及已确认决定分组；实现文件与符号不在本页复制维护。

| 原需求 | 详细契约/实现/验证入口 | 执行边界 |
|---|---|---|
| FR-1 目标状态可达 | [F13 legacy与未接入元素](../../../docs/control/elements-and-dormant-code.md#f13)；[F09 活动电磁元素](../../../docs/control/elements-and-dormant-code.md#f09) | 规划/待逐元素决定 |
| FR-2 左环规划 | [F13 legacy与未接入元素](../../../docs/control/elements-and-dormant-code.md#f13) | 规划/待逐元素决定 |
| FR-3 跷跷板规划 | [F13 legacy与未接入元素](../../../docs/control/elements-and-dormant-code.md#f13) | 规划/待逐元素决定 |
| FR-4 路障规划 | [F13 legacy与未接入元素](../../../docs/control/elements-and-dormant-code.md#f13) | 规划/待逐元素决定 |
| FR-5 既有元素保持 | [F09 活动电磁元素](../../../docs/control/elements-and-dormant-code.md#f09) | 规划/待逐元素决定 |
| FR-6 新增状态记录 | [F19 日志格式容量](../../../docs/services/recording-and-export.md#f19)；[F13 legacy与未接入元素](../../../docs/control/elements-and-dormant-code.md#f13) | 规划/待逐元素决定 |

原分组名（不改变次序/勾选，完整组内容见冻结记录）：

- 任务分组（Task Groups）
- Group 1: 左环岛接入
- Group 2: 跷跷板接入
- Group 3: 路障接入与 `g_control_cask_flag` 职责理顺

执行顺序：先读需求→对应docs→检查未决/依赖→纯规约提交→实现→所选验证及记录。回退以该次完整组为单位，同步回退接口/调用/文档，历史兼容和保护边界不借回退改变。纯文档检查不触发旧组软件门禁。







## 冻结历史基线（68cb508，非现行契约）

以下原文用于保留阶段决定、组顺序、需求历史与真实验证；不继续独立维护算法/布局/源码映射。当前契约先查上方docs条目。旧“本轮”“待执行”“当前”只指原记录时点，不覆盖后续决定。历史任务的全量软件门禁不自动成为以后每次修改的要求。

<details>
<summary>展开原始阶段记录（原文保留）</summary>

# Phase 09: 元素功能补齐 — 实现计划

## 目标

逐个接入当前不可达的赛道元素（左环岛 / 跷跷板 / 路障），使 kernel 主状态机 7 态均可按设计到达，且每个元素是一次独立可审查的提交。

## 背景与依据

- 关联宪章：[mission](../../mission.md) 成功标准 5"新元素小步接入"；排期见 [roadmap](../../roadmap.md) Phase 9。现状基准规约见 [Phase 04 元素状态机](../04-element-fsm/requirements.md)。
- 前置条件：无强制依赖；建议 Phase 08 完成后实施（元素参数需落盘才有意义）。
- 现状（均已对照源码核实）：

### 三个 kernel 态不可达

`control.h:14-20` 定义 7 个 `KERNEL_*` 状态，但：

- `CONTROL_KERNEL_TEETERBOARD` 仅由 `Element_JudgeTeeterboard`（`element.c:51`）置位，`CONTROL_KERNEL_CASK` 仅由 `Element_JudgeCask`（`element.c:73`）置位，而这两个函数只被 `Element_Judge`（`element.c:20`）引用，`Element_Judge` 全工程无调用点；
- `CONTROL_KERNEL_ISLAND_L` 无任何置位路径：`Roundabout_JudgePreentryLeft`（`roundabout.c:106`）虽能置 `g_roundabout_left_flag=1`，但 `CONTROL_KERNEL_REISLAND` 态只调用 `Roundabout_JudgeEntryRight`（`control.c:75`），其进入条件要求 `g_roundabout_right_flag==1`（`roundabout.c:168`）——左环岛永远无法进入对应分支。

### 环岛子状态机名实不符、宏大量闲置

- `roundabout.h:11-21` 共 10 个 `ISLAND_*` 子状态宏，仅 4 个被使用（`ROUNDABOUT_ISLAND_LPREENTER/TURN_LEFT/IN/OUT`）；`ROUNDABOUT_ISLAND_RPREENTER/TURN_RIGHT/EXIT/LENTER/RENTER/TURN_TURN_LEFT` 闲置。
- `ROUNDABOUT_ISLAND_TURN_LEFT` 名为"左"，实际服务右环岛路径（`CONTROL_KERNEL_ISLAND_R` 分支，`roundabout.c:60`）；`g_roundabout_sign_round`（`roundabout.c:25`）在 `control.c:116/134` 被赋值但全工程无人读取（写而不读）。
- `Roundabout_JudgeEnteredAgain/Roundabout_JudgePreExit/Roundabout_Judge`（`roundabout.c:202/212/244`）、`Element_JudgeStraight`（`element.c:29`）为源工程注释残留的空壳，从未被调用。

### `g_control_cask_flag` 职责混杂

`Roundabout_JudgePreentryLeft` 内嵌"DL1B < 100mm 置位 `g_control_cask_flag`"（`roundabout.c:127-130`），使一个名为"路障标志"的变量实际承担两件事：路障元素检测 + 右环岛入环抑制（`Roundabout_JudgeEntryRight` 的 `!g_control_cask_flag` 条件）。路障元素的接入必须先理顺这一职责。

### 类型隐患（接入前必须处理）

`g_element_enter_tick` 为 `int16_t`（`element.h:12`）而 `g_app_control_ticks` 为 `int32_t`（`app.c:33`），`Element_JudgeTeeterboard` 的 `g_element_enter_tick = g_app_control_ticks`（`element.c:56`）在运行约 65s 后发生截断，超时判断（`element.c:67`）将出错。

## 任务分组（Task Groups）

> **每个 Group 开工前必须与用户确认该元素的设计（判断条件 / 状态转移 / 参数取值），确认结论回填本规约后再实现。** 以下阈值为源码中的既有草拟值，仅作讨论基线。

### Group 1: 左环岛接入
> 待确认：左环分支的判断条件、打角方向/距离门限取值、与右环路径的对称方式、`g_roundabout_sign_round` 的启用或删除。
- [ ] 与用户确认设计并回填本节
- [ ] 在 `CONTROL_KERNEL_REISLAND` 或 `CONTROL_KERNEL_TRACKING` 路径补左环分支：`g_roundabout_left_flag==1` 时进入 `CONTROL_KERNEL_ISLAND_L`
- [ ] 实现与右环对称的打角方向与距离门限（`Roundabout_Update()` 复用或派生，消除 `ROUNDABOUT_ISLAND_TURN_LEFT` 名实不符）
- [ ] 处置 `g_roundabout_sign_round`（启用为方向选择或删除）

### Group 2: 跷跷板接入
> 待确认：进入条件（草拟：4 路电感和 < 800）、离开条件（草拟：4 路和 > 1000 或超时 300 拍=600ms）、`CONTROL_KERNEL_TEETERBOARD` 态增益取值。
- [ ] 与用户确认设计并回填本节
- [ ] 先将 `g_element_enter_tick` 类型修正为 `int32_t`（前置，消除截断）
- [ ] 将 `Element_JudgeTeeterboard/Element_JudgeTeeterboardExit` 挂入主状态机调用链（`CONTROL_KERNEL_TRACKING` 进、`CONTROL_KERNEL_TEETERBOARD` 出）

### Group 3: 路障接入与 `g_control_cask_flag` 职责理顺
> 待确认：进入条件（草拟：`angle_y > 60`）、离开条件（草拟：`angle_y < 20`）、`g_control_cask_flag` 的拆分方式（环岛抑制标志 vs 路障元素标志）、DL1B 近距判据的去留。
- [ ] 与用户确认设计并回填本节
- [ ] 拆分 `g_control_cask_flag`：环岛抑制与路障元素检测各自单职
- [ ] 将 `Element_JudgeCask/Element_JudgeCaskExit` 挂入主状态机调用链

## 实现顺序与依赖

- 三个 Group 各自独立、分别提交；建议顺序 1 → 2 → 3（左环岛复用既有右环框架，风险最低；路障涉及职责拆分，放最后）。
- 每组内部：先改判断挂载点，再调参数；参数初值沿用源码草拟值，上机标定后回填本规约。
- 与 [Phase 10](../10-control-consistency/plan.md) 的协调：本阶段只负责"可达、行为正确"，增益表驱动化与调参语义统一由 Phase 10 完成，避免重复返工。

## 风险与取舍

- **行为回归风险最高**：判断条件挂在 `CONTROL_KERNEL_TRACKING` 每拍路径上，误触发会破坏已验证的循迹/十字/右环岛。对策：每组先以保守阈值接入，上机逐项验证后再收紧；回归清单见 [validation](validation.md)。
- **跷跷板超时语义**：`g_app_control_ticks` 全程递增，超时 300 拍以进入时刻为基准；`g_element_enter_tick` 类型修正先行，否则长距离后误判。
- **路障与环岛的互斥**：`g_control_cask_flag` 拆分后需重新定义"环岛入环抑制"的触发条件，避免拆分后环岛误进或路障漏检——这是 Group 3 必须确认的核心点。
- 明确排除：不改变已验证的十字/右环岛行为；不引入新传感器；不做元素识别算法重写（沿用源工程阈值框架）。

> 2026-10-16 Phase 15 G2：现行传感器/控制符号按[完整映射](../15-code-style-consistency/symbol-map.md)迁移，
> 配置、计算顺序及休眠调用边保持；历史 validation 原文及本阶段实车门禁不变。

> 2026-10-16 Phase 15 G3：现行调度、HMI、参数和日志名称已按[符号表](../15-code-style-consistency/symbol-map.md)统一。
> 本文原阶段行为记录与未完成待办不因此重新验收；现行运行链以 Phase 14 和当前头文件契约为准，历史 validation 保留原文。

</details>
