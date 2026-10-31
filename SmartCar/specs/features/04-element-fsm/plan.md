# Phase 04: 赛道元素状态机（现状） — 实现计划

2026-10-31文档迁移：**Phase → docs → 代码**。本页保留需求/任务/验收历史，不新增阶段、不修改实现或重新验收。

当前需求导航：[P04功能索引](../../../docs/traceability.md#p04)。具体输入输出、状态/错误/时序、代码符号与验证方法由下列docs条目唯一维护。

## 任务分组与详细设计入口

历史分组/依赖/风险/回退和用户决定保留在下方原记录；本次未重开原实现任务。以后执行本阶段增量时，先确定对应需求和docs目标，按原前置及已确认决定分组；实现文件与符号不在本页复制维护。

| 原需求 | 详细契约/实现/验证入口 | 执行边界 |
|---|---|---|
| FR-1 主态结构 | [F09 活动电磁元素](../../../docs/control/elements-and-dormant-code.md#f09)；[F12 部分配置与全局调参规划](../../../docs/control/tracking-and-controllers.md#f12)；[F11 双FSM与唯一执行](../../../docs/control/tracking-and-controllers.md#f11) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-2 失线保护 | [F02 起停、模式与首因](../../../docs/system/runtime-and-safety.md#f02) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-3 十字 | [F09 活动电磁元素](../../../docs/control/elements-and-dormant-code.md#f09) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-4 预环 | [F09 活动电磁元素](../../../docs/control/elements-and-dormant-code.md#f09) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-5 右环进入 | [F09 活动电磁元素](../../../docs/control/elements-and-dormant-code.md#f09) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-6 预环超时 | [F09 活动电磁元素](../../../docs/control/elements-and-dormant-code.md#f09) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-7 右环阶段 | [F09 活动电磁元素](../../../docs/control/elements-and-dormant-code.md#f09)；[F12 部分配置与全局调参规划](../../../docs/control/tracking-and-controllers.md#f12) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-8 入环抑制 | [F09 活动电磁元素](../../../docs/control/elements-and-dormant-code.md#f09) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-9 休眠元素 | [F13 legacy与未接入元素](../../../docs/control/elements-and-dormant-code.md#f13) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-10 活动转移集合 | [F09 活动电磁元素](../../../docs/control/elements-and-dormant-code.md#f09) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-11 休眠出态调用边界 | [F13 legacy与未接入元素](../../../docs/control/elements-and-dormant-code.md#f13) | 当前契约及历史替代见docs；不改变原验收状态 |

原分组名（不改变次序/勾选，完整组内容见冻结记录）：

- 任务分组（Task Groups）
- Group 1: kernel 主状态机（`App/control.c`，源 `control.c`）
- Group 2: 十字与休眠元素判断（`App/element.c`，源 `element.c`）
- Group 3: 环岛判断与距离驱动子状态机（`App/roundabout.c`，源 `roundabout.c`）

执行顺序：先读需求→对应docs→检查未决/依赖→纯规约提交→实现→所选验证及记录。回退以该次完整组为单位，同步回退接口/调用/文档，历史兼容和保护边界不借回退改变。纯文档检查不触发旧组软件门禁。












## 冻结历史基线（68cb508，非现行契约）

以下原文用于保留阶段决定、组顺序、需求历史与真实验证；不继续独立维护算法/布局/源码映射。当前契约先查上方docs条目。旧“本轮”“待执行”“当前”只指原记录时点，不覆盖后续决定。历史任务的全量软件门禁不自动成为以后每次修改的要求。

<details>
<summary>展开原始阶段记录（原文保留）</summary>

# Phase 04: 赛道元素状态机（现状） — 实现计划

> **as-built 规约**：代码已存在，本文档为存量功能的规约化追认，如实描述现状——包括实际不可达的状态与职责混杂的逻辑——作为 Phase 09/10 整改的对照基准。任务分组全部已完成，内容为实现结构追溯而非待办。

## 目标

> Phase 13 将本文件所述的重复增益赋值改为带字段存在位的配置表，并以模块 getter/写接口取代电感、电机和 IMU 的旧全局访问；下方状态与阈值仍描述不变的运行语义。当前接口映射见 [Phase 13](../13-control-structure/plan.md)。

交付 kernel 主状态机（`Control_UpdateLegacy()`）与赛道元素判断的现状规约：当前实际启用并可达的只有 **十字（CROSSROADS）** 与 **右环岛（REISLAND→ISLAND_R）** 两条路径；左环岛、跷跷板、路障三态的代码存在但判断未接入、不可达。

## 背景与依据

- 关联宪章：[使命](../../mission.md)（范围内第 2 条：赛道元素识别与处理）；[技术栈](../../techstack.md) 硬性约束 2（2 ms 控制拍）。
- 算法出处：源工程 `Sirius20260718` 的 `control.c` / `element.c` / `roundabout.c`（逐飞库，仓库外，仅按名称引用）；环岛状态机源工程中被整体注释的角度法代码未移植，本工程为纯距离驱动版本。
- 前置条件：[Phase 03 电磁循迹控制链](../03-track-following/plan.md)（提供 `adc_filted`/`symmetry_y`/`Distance`/循迹执行链）、[Phase 05 IMU 姿态](../05-imu-attitude/plan.md)（`angle_x` 清零约定）。

## 任务分组（Task Groups）

### Group 1: kernel 主状态机（`App/control.c`，源 `control.c`）

- [x] 7 态宏定义（`control.h`，`#define` 非枚举）：TRACKING(0)/ISLAND_L(1)/ISLAND_R(2)/TEETERBOARD(3)/CROSSROADS(4)/CASK(5)/REISLAND(6)；状态变量 `g_control_kernel_state`。
- [x] `Control_UpdateLegacy()` 每拍流程：`Track_ReadAdc()` → 出赛道保护判断 → `switch(g_control_kernel_state)`：入态先覆写权重 + 方向增益 + `base_speed`，再 `Track_UpdateError` → `PID_UpdateTrack` → `Motor_DistributeSpeed`，最后调本态的元素进出判断函数。
- [x] 出赛道保护：4 路电感和 ≤300 时直接清 `g_app_key_flag`/`g_app_start_flag`/`g_app_run_flag`，目标速度归零停车。
- [x] 各态增益表（现状硬编码）：

  | 状态 | 权重 x/xx/y/abs | base_speed | 方向增益 | 备注 |
  |---|---|---|---|---|
  | TRACKING | 15/20/22/10 | 245 | 2 / 0.008 / 15 / 1 | 调 `Element_JudgeCrossroads` + `Roundabout_JudgePreentryLeft` |
  | REISLAND | 20/10/1/10 | 180 | 2 / 0.008 / 15 / 1 | 调 `Roundabout_JudgeEntryRight` + `Roundabout_JudgePreentryExit` |
  | CROSSROADS | 15/20/2/10 | 240 | 2 / 0.008 / 15 / 1 | `g_control_track_output` 限 ±10，调 `Element_JudgeCrossroadsExit` |
  | ISLAND_R | 15/20/22/10 | 150 | KP_a=2/KD_a=1.2/KG_a=0 | `g_roundabout_sign_round=1`，转 `Roundabout_Update()` 子状态机 |
  | ISLAND_L | 15/20/22/10 | （未覆写） | KP_a=2/KD_a=1.2/KG_a=0 | `g_roundabout_sign_round=-1`，转 `Roundabout_Update()`；**不可达** |
  | TEETERBOARD | （未覆写） | 100 | 4.5 / 0 / 6.2 / 0.9 | 调 `Element_JudgeTeeterboardExit`；**不可达** |
  | CASK | 15/20/2/5 | 210 | 2 / 0.008 / 15 / 1 | 调 `Element_JudgeCaskExit`；**不可达** |

### Group 2: 十字与休眠元素判断（`App/element.c`，源 `element.c`）

- [x] `Element_JudgeCrossroads()`：竖两路和（`adc_filted[1]+[2]`）> 2800 且 `symmetry_y` < 25 → CROSSROADS。
- [x] `Element_JudgeCrossroadsExit()`：竖两路和 < 2800 → 回 TRACKING。
- [x] 未接入判断（保留代码、主状态机不调用，对应三态不可达）：`Element_JudgeTeeterboard()`（四路和 <800 进）、`Element_JudgeCask()`（`angle_y`>60 进）、`Roundabout_JudgeEntryLeft()`（源工程条件已整体注释，空壳）；汇总壳 `Element_Judge()`、空壳 `Element_JudgeStraight()` 同样无人调用。
- [x] 跷跷板/路障的出态判断（`Element_JudgeTeeterboardExit` 四路和 >1000 或超时 300 拍、`Element_JudgeCaskExit` `angle_y`<20）虽可达性为 0 仍保留编译。

### Group 3: 环岛判断与距离驱动子状态机（`App/roundabout.c`，源 `roundabout.c`）

- [x] `Roundabout_JudgePreentryLeft()`（TRACKING 态每拍调）：横两路和（`adc_filted[0]+[3]`）> 2800 时，按左右大小置 `g_roundabout_left_flag`/`g_roundabout_right_flag`，清 `g_control_cask_flag`、`angle_x`、里程，转 REISLAND。**函数内同时内嵌 DL1B < 100 mm 置 `g_control_cask_flag` 的路障抑制逻辑**（职责混杂，见风险节）。
- [x] `Roundabout_JudgeEntryRight()`（REISLAND 态每拍调）：竖两路和 < 300 且 `g_roundabout_right_flag==1` 且 `!g_control_cask_flag` → 环岛子状态机置 ISLAND_LPREENTER，`g_control_kernel_state` 转 ISLAND_R，清 `g_control_track_output`/`angle_x`/里程。
- [x] `Roundabout_JudgePreentryExit()`：REISLAND 态 `Distance` > 5000 未进环 → 回 TRACKING，清双环岛标志、`g_control_track_output`、`angle_x`。
- [x] 环岛子状态机（`Roundabout_Update()`，纯编码器里程驱动，单位=2 ms 拍编码器计数）：

  ```
  NORMAL → ISLAND_LPREENTER（循迹直行，权重 20/10/1/10，base_speed=150）
         → [Distance ≥ enter_distance1=8000，清里程] → ISLAND_TURN_LEFT（固定 speed_control(-30) 打角）
         → [Distance > 13000，清里程]               → ISLAND_IN（环内循迹，权重沿用，base_speed=150）
         → [Distance > 32000，清里程]               → ISLAND_OUT（清里程于转移时，权重 20/10/1/10）
         → [Distance > out_distance1=10000]         → NORMAL + TRACKING，清双环岛标志
  ```

  注：`ROUNDABOUT_ISLAND_TURN_LEFT` 名为"左"实际服务右环岛（`Motor_DistributeSpeed(-30)` 为向右打角）。
- [x] 休眠/壳代码：`Roundabout_JudgePreentryRight()`（四路和 >4500 或横两路和 >3000 → REISLAND，**定义但无人调用**）、`Roundabout_JudgeEnteredAgain()`（角度法残留）、`Roundabout_JudgePreExit()`/`Roundabout_Judge()`（空壳）、`g_roundabout_enter_angle1/2`、`g_roundabout_out_angle1`（角度法参数，未使用）。
- [x] 环岛状态宏共 11 个（`roundabout.h`：STATE_NORMAL/ISLAND_LPREENTER/ISLAND_RPREENTER/ISLAND_TURN_LEFT/ISLAND_TURN_RIGHT/ISLAND_IN/ISLAND_EXIT/ISLAND_LENTER/ISLAND_RENTER/ISLAND_OUT/ISLAND_TURN_TURN_LEFT），代码实际引用 5 个（NORMAL、LPREENTER、TURN_LEFT、IN、OUT），其余 6 个无任何使用点。

## 实现顺序与依赖

```
App_ControlISR (TIM6 2ms)
  └─ Control_UpdateLegacy()
       ├─ Track_ReadAdc / 出赛道保护（电感和≤300 → 停车）
       ├─ switch(kernel_state)：覆写增益 → 循迹链（Phase 03）
       └─ 元素判断：element.c（十字）/ roundabout.c（预环岛、右环岛、出环岛）
            └─ ISLAND_R → Roundabout_Update() 子状态机（Distance 驱动，Phase 03 Motor_ReadEncoder 提供里程）
```

依赖：`adc_filted`/`symmetry_y`（track_sensor）、`Distance`/里程清零（motor）、`angle_x` 清零（imu_proc）、`g_app_dl1b_distance_mm`（app.c 经 BSP DL1B 轮询更新）、`g_app_control_ticks` 节拍计数（app.c）。

## 风险与取舍（as-built 已知问题，如实记录）

| 问题 | 现状 | 整改指向 |
|---|---|---|
| 三态不可达 | ISLAND_L / TEETERBOARD / CASK 的入态判断未接入主状态机，实际可达路径仅 TRACKING→CROSSROADS→TRACKING 与 TRACKING→REISLAND→ISLAND_R→TRACKING | Phase 09（逐个接入前须与用户确认设计，AGENTS.md 硬性规则） |
| 环岛状态宏 11 个仅用 5 个 | 6 个宏（RPREENTER/TURN_RIGHT/EXIT/LENTER/RENTER/TURN_TURN_LEFT）无使用点 | Phase 09/11 |
| `g_roundabout_sign_round` 只写不读 | ISLAND_L/R 态分别写 −1/1，但 `Roundabout_Update()` 从不读取，左右环岛打角方向实际由写死的 `Motor_DistributeSpeed(-30)` 决定 | Phase 09（接入左环岛时一并处理） |
| `g_control_cask_flag` 职责混杂 | 路障抑制逻辑（DL1B < 100 mm）内嵌在预环岛判断 `Roundabout_JudgePreentryLeft` 中，与环岛识别耦合 | Phase 09/10 |
| 增益每拍覆写与调参体系冲突 | Phase 13 配置表仍每拍覆写权重/增益/base_speed，菜单/串口双档调参（Phase 06）在运行态仍被覆盖 | Phase 10（调参语义定稿） |
| 距离阈值魔数 | 13000、32000 为内联魔数（仅 8000/10000 有命名 `g_roundabout_enter_distance1`/`g_roundabout_out_distance1`）；5000/2800/300/25 等阈值同样内联 | Phase 10/11 |
| `Roundabout_JudgePreentryRight` 无人调用 | 与 `Roundabout_JudgePreentryLeft` 功能重叠的冗余判断 | Phase 11（休眠代码处置） |
| ISLAND_L 态不覆写 base_speed | 若未来接入将沿用前一状态的速度，语义隐患 | Phase 09 |
| 出赛道保护清 `g_app_key_flag` | 出赛道即回到菜单态，重新起跑需再按启动键——现状语义如此，保留记录 | 不整改（现状语义） |

> 2026-10-16 Phase 15 G2：现行传感器/控制符号按[完整映射](../15-code-style-consistency/symbol-map.md)迁移，
> 配置、计算顺序及休眠调用边保持；历史 validation 原文及本阶段实车门禁不变。

> 2026-10-16 Phase 15 G3：现行调度、HMI、参数和日志名称已按[符号表](../15-code-style-consistency/symbol-map.md)统一。
> 本文原阶段行为记录与未完成待办不因此重新验收；现行运行链以 Phase 14 和当前头文件契约为准，历史 validation 保留原文。

## Phase 17 当前实现定位（2026-10-24）

活动判断位于control/electromagnetic_fsm，光电观察位于optical_fsm，tracking同帧推进并选择唯一执行口；部分配置和8000/10000门限由control_profile持有。旧Element/Roundabout/Control整体入口分别在legacy_element/legacy_roundabout/legacy_control保留编译与原调用，未新增活动接入，不因移动而启用左环、跷跷板或路障。

本节是现行归属导航；上文历史计划、验证、提交路径及未完成实机项目保持。实际重构验证见
[Phase17验收](../17-app-architecture/validation.md)，逐符号迁移见[迁移导航](../17-app-architecture/migration-map.md)。

</details>
