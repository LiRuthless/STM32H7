# Phase 09: 元素功能补齐 — 实现计划

## 目标

逐个接入当前不可达的赛道元素（左环岛 / 跷跷板 / 路障），使 kernel 主状态机 7 态均可按设计到达，且每个元素是一次独立可审查的提交。

## 背景与依据

- 关联宪章：[mission](../../mission.md) 成功标准 5"新元素小步接入"；排期见 [roadmap](../../roadmap.md) Phase 9。现状基准规约见 [Phase 04 元素状态机](../04-element-fsm/requirements.md)。
- 前置条件：无强制依赖；建议 Phase 08 完成后实施（元素参数需落盘才有意义）。
- 现状（均已对照源码核实）：

### 三个 kernel 态不可达

`control.h:14-20` 定义 7 个 `KERNEL_*` 状态，但：

- `KERNEL_TEETERBOARD` 仅由 `teeterboard_judge`（`element.c:51`）置位，`KERNEL_CASK` 仅由 `cask_judge`（`element.c:73`）置位，而这两个函数只被 `element_judge`（`element.c:20`）引用，`element_judge` 全工程无调用点；
- `KERNEL_ISLAND_L` 无任何置位路径：`L_reroundabout_judge`（`roundabout.c:106`）虽能置 `L_round_flag=1`，但 `KERNEL_REISLAND` 态只调用 `R_roundabout_judge`（`control.c:75`），其进入条件要求 `R_round_flag==1`（`roundabout.c:168`）——左环岛永远无法进入对应分支。

### 环岛子状态机名实不符、宏大量闲置

- `roundabout.h:11-21` 共 10 个 `ISLAND_*` 子状态宏，仅 4 个被使用（`ISLAND_LPREENTER/TURN_LEFT/IN/OUT`）；`ISLAND_RPREENTER/TURN_RIGHT/EXIT/LENTER/RENTER/TURN_TURN_LEFT` 闲置。
- `ISLAND_TURN_LEFT` 名为"左"，实际服务右环岛路径（`KERNEL_ISLAND_R` 分支，`roundabout.c:60`）；`sign_round`（`roundabout.c:25`）在 `control.c:116/134` 被赋值但全工程无人读取（写而不读）。
- `entered_entered_judge/pre_out_judge/judge`（`roundabout.c:202/212/244`）、`straight_judge`（`element.c:29`）为源工程注释残留的空壳，从未被调用。

### `cask_flag` 职责混杂

`L_reroundabout_judge` 内嵌"DL1B < 100mm 置位 `cask_flag`"（`roundabout.c:127-130`），使一个名为"路障标志"的变量实际承担两件事：路障元素检测 + 右环岛入环抑制（`R_roundabout_judge` 的 `!cask_flag` 条件）。路障元素的接入必须先理顺这一职责。

### 类型隐患（接入前必须处理）

`in_time` 为 `int16_t`（`element.h:12`）而 `time` 为 `int32_t`（`app.c:33`），`teeterboard_judge` 的 `in_time = time`（`element.c:56`）在运行约 65s 后发生截断，超时判断（`element.c:67`）将出错。

## 任务分组（Task Groups）

> **每个 Group 开工前必须与用户确认该元素的设计（判断条件 / 状态转移 / 参数取值），确认结论回填本规约后再实现。** 以下阈值为源码中的既有草拟值，仅作讨论基线。

### Group 1: 左环岛接入
> 待确认：左环分支的判断条件、打角方向/距离门限取值、与右环路径的对称方式、`sign_round` 的启用或删除。
- [ ] 与用户确认设计并回填本节
- [ ] 在 `KERNEL_REISLAND` 或 `KERNEL_TRACKING` 路径补左环分支：`L_round_flag==1` 时进入 `KERNEL_ISLAND_L`
- [ ] 实现与右环对称的打角方向与距离门限（`roundabout()` 复用或派生，消除 `ISLAND_TURN_LEFT` 名实不符）
- [ ] 处置 `sign_round`（启用为方向选择或删除）

### Group 2: 跷跷板接入
> 待确认：进入条件（草拟：4 路电感和 < 800）、离开条件（草拟：4 路和 > 1000 或超时 300 拍=600ms）、`KERNEL_TEETERBOARD` 态增益取值。
- [ ] 与用户确认设计并回填本节
- [ ] 先将 `in_time` 类型修正为 `int32_t`（前置，消除截断）
- [ ] 将 `teeterboard_judge/teeterboard_out_judge` 挂入主状态机调用链（`KERNEL_TRACKING` 进、`KERNEL_TEETERBOARD` 出）

### Group 3: 路障接入与 `cask_flag` 职责理顺
> 待确认：进入条件（草拟：`angle_y > 60`）、离开条件（草拟：`angle_y < 20`）、`cask_flag` 的拆分方式（环岛抑制标志 vs 路障元素标志）、DL1B 近距判据的去留。
- [ ] 与用户确认设计并回填本节
- [ ] 拆分 `cask_flag`：环岛抑制与路障元素检测各自单职
- [ ] 将 `cask_judge/cask_out_judge` 挂入主状态机调用链

## 实现顺序与依赖

- 三个 Group 各自独立、分别提交；建议顺序 1 → 2 → 3（左环岛复用既有右环框架，风险最低；路障涉及职责拆分，放最后）。
- 每组内部：先改判断挂载点，再调参数；参数初值沿用源码草拟值，上机标定后回填本规约。
- 与 [Phase 10](../10-control-consistency/plan.md) 的协调：本阶段只负责"可达、行为正确"，增益表驱动化与调参语义统一由 Phase 10 完成，避免重复返工。

## 风险与取舍

- **行为回归风险最高**：判断条件挂在 `KERNEL_TRACKING` 每拍路径上，误触发会破坏已验证的循迹/十字/右环岛。对策：每组先以保守阈值接入，上机逐项验证后再收紧；回归清单见 [validation](validation.md)。
- **跷跷板超时语义**：`time` 全程递增，超时 300 拍以进入时刻为基准；`in_time` 类型修正先行，否则长距离后误判。
- **路障与环岛的互斥**：`cask_flag` 拆分后需重新定义"环岛入环抑制"的触发条件，避免拆分后环岛误进或路障漏检——这是 Group 3 必须确认的核心点。
- 明确排除：不改变已验证的十字/右环岛行为；不引入新传感器；不做元素识别算法重写（沿用源工程阈值框架）。
