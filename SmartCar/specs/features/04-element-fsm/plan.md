# Phase 04: 赛道元素状态机（现状） — 实现计划

> **as-built 规约**：代码已存在，本文档为存量功能的规约化追认，如实描述现状——包括实际不可达的状态与职责混杂的逻辑——作为 Phase 09/10 整改的对照基准。任务分组全部已完成，内容为实现结构追溯而非待办。

## 目标

> Phase 13 将本文件所述的重复增益赋值改为带字段存在位的配置表，并以模块 getter/写接口取代电感、电机和 IMU 的旧全局访问；下方状态与阈值仍描述不变的运行语义。当前接口映射见 [Phase 13](../13-control-structure/plan.md)。

交付 kernel 主状态机（`whole_test()`）与赛道元素判断的现状规约：当前实际启用并可达的只有 **十字（CROSSROADS）** 与 **右环岛（REISLAND→ISLAND_R）** 两条路径；左环岛、跷跷板、路障三态的代码存在但判断未接入、不可达。

## 背景与依据

- 关联宪章：[使命](../../mission.md)（范围内第 2 条：赛道元素识别与处理）；[技术栈](../../techstack.md) 硬性约束 2（2 ms 控制拍）。
- 算法出处：源工程 `Sirius20260718` 的 `control.c` / `element.c` / `roundabout.c`（逐飞库，仓库外，仅按名称引用）；环岛状态机源工程中被整体注释的角度法代码未移植，本工程为纯距离驱动版本。
- 前置条件：[Phase 03 电磁循迹控制链](../03-track-following/plan.md)（提供 `adc_filted`/`symmetry_y`/`Distance`/循迹执行链）、[Phase 05 IMU 姿态](../05-imu-attitude/plan.md)（`angle_x` 清零约定）。

## 任务分组（Task Groups）

### Group 1: kernel 主状态机（`App/control.c`，源 `control.c`）

- [x] 7 态宏定义（`control.h`，`#define` 非枚举）：TRACKING(0)/ISLAND_L(1)/ISLAND_R(2)/TEETERBOARD(3)/CROSSROADS(4)/CASK(5)/REISLAND(6)；状态变量 `kernel_state`。
- [x] `whole_test()` 每拍流程：`read_adc()` → 出赛道保护判断 → `switch(kernel_state)`：入态先覆写权重 + 方向增益 + `base_speed`，再 `get_track_error` → `PID_track` → `speed_control`，最后调本态的元素进出判断函数。
- [x] 出赛道保护：4 路电感和 ≤300 时直接清 `key_flag`/`Start_flag`/`Run_flag`，目标速度归零停车。
- [x] 各态增益表（现状硬编码）：

  | 状态 | 权重 x/xx/y/abs | base_speed | 方向增益 | 备注 |
  |---|---|---|---|---|
  | TRACKING | 15/20/22/10 | 245 | 2 / 0.008 / 15 / 1 | 调 `crossroads_judge` + `L_reroundabout_judge` |
  | REISLAND | 20/10/1/10 | 180 | 2 / 0.008 / 15 / 1 | 调 `R_roundabout_judge` + `reroundabout_out_judge` |
  | CROSSROADS | 15/20/2/10 | 240 | 2 / 0.008 / 15 / 1 | `track_out` 限 ±10，调 `crossroads_out_judge` |
  | ISLAND_R | 15/20/22/10 | 150 | KP_a=2/KD_a=1.2/KG_a=0 | `sign_round=1`，转 `roundabout()` 子状态机 |
  | ISLAND_L | 15/20/22/10 | （未覆写） | KP_a=2/KD_a=1.2/KG_a=0 | `sign_round=-1`，转 `roundabout()`；**不可达** |
  | TEETERBOARD | （未覆写） | 100 | 4.5 / 0 / 6.2 / 0.9 | 调 `teeterboard_out_judge`；**不可达** |
  | CASK | 15/20/2/5 | 210 | 2 / 0.008 / 15 / 1 | 调 `cask_out_judge`；**不可达** |

### Group 2: 十字与休眠元素判断（`App/element.c`，源 `element.c`）

- [x] `crossroads_judge()`：竖两路和（`adc_filted[1]+[2]`）> 2800 且 `symmetry_y` < 25 → CROSSROADS。
- [x] `crossroads_out_judge()`：竖两路和 < 2800 → 回 TRACKING。
- [x] 未接入判断（保留代码、主状态机不调用，对应三态不可达）：`teeterboard_judge()`（四路和 <800 进）、`cask_judge()`（`angle_y`>60 进）、`L_roundabout_judge()`（源工程条件已整体注释，空壳）；汇总壳 `element_judge()`、空壳 `straight_judge()` 同样无人调用。
- [x] 跷跷板/路障的出态判断（`teeterboard_out_judge` 四路和 >1000 或超时 300 拍、`cask_out_judge` `angle_y`<20）虽可达性为 0 仍保留编译。

### Group 3: 环岛判断与距离驱动子状态机（`App/roundabout.c`，源 `roundabout.c`）

- [x] `L_reroundabout_judge()`（TRACKING 态每拍调）：横两路和（`adc_filted[0]+[3]`）> 2800 时，按左右大小置 `L_round_flag`/`R_round_flag`，清 `cask_flag`、`angle_x`、里程，转 REISLAND。**函数内同时内嵌 DL1B < 100 mm 置 `cask_flag` 的路障抑制逻辑**（职责混杂，见风险节）。
- [x] `R_roundabout_judge()`（REISLAND 态每拍调）：竖两路和 < 300 且 `R_round_flag==1` 且 `!cask_flag` → 环岛子状态机置 ISLAND_LPREENTER，`kernel_state` 转 ISLAND_R，清 `track_out`/`angle_x`/里程。
- [x] `reroundabout_out_judge()`：REISLAND 态 `Distance` > 5000 未进环 → 回 TRACKING，清双环岛标志、`track_out`、`angle_x`。
- [x] 环岛子状态机（`roundabout()`，纯编码器里程驱动，单位=2 ms 拍编码器计数）：

  ```
  NORMAL → ISLAND_LPREENTER（循迹直行，权重 20/10/1/10，base_speed=150）
         → [Distance ≥ enter_distance1=8000，清里程] → ISLAND_TURN_LEFT（固定 speed_control(-30) 打角）
         → [Distance > 13000，清里程]               → ISLAND_IN（环内循迹，权重沿用，base_speed=150）
         → [Distance > 32000，清里程]               → ISLAND_OUT（清里程于转移时，权重 20/10/1/10）
         → [Distance > out_distance1=10000]         → NORMAL + TRACKING，清双环岛标志
  ```

  注：`ISLAND_TURN_LEFT` 名为"左"实际服务右环岛（`speed_control(-30)` 为向右打角）。
- [x] 休眠/壳代码：`R_reroundabout_judge()`（四路和 >4500 或横两路和 >3000 → REISLAND，**定义但无人调用**）、`entered_entered_judge()`（角度法残留）、`pre_out_judge()`/`judge()`（空壳）、`enter_angle1/2`、`out_angle1`（角度法参数，未使用）。
- [x] 环岛状态宏共 11 个（`roundabout.h`：STATE_NORMAL/ISLAND_LPREENTER/ISLAND_RPREENTER/ISLAND_TURN_LEFT/ISLAND_TURN_RIGHT/ISLAND_IN/ISLAND_EXIT/ISLAND_LENTER/ISLAND_RENTER/ISLAND_OUT/ISLAND_TURN_TURN_LEFT），代码实际引用 5 个（NORMAL、LPREENTER、TURN_LEFT、IN、OUT），其余 6 个无任何使用点。

## 实现顺序与依赖

```
App_ControlISR (TIM6 2ms)
  └─ whole_test()
       ├─ read_adc / 出赛道保护（电感和≤300 → 停车）
       ├─ switch(kernel_state)：覆写增益 → 循迹链（Phase 03）
       └─ 元素判断：element.c（十字）/ roundabout.c（预环岛、右环岛、出环岛）
            └─ ISLAND_R → roundabout() 子状态机（Distance 驱动，Phase 03 read_encoder 提供里程）
```

依赖：`adc_filted`/`symmetry_y`（track_sensor）、`Distance`/里程清零（motor）、`angle_x` 清零（imu_proc）、`dl1b_distance_mm`（app.c 经 BSP DL1B 轮询更新）、`time` 节拍计数（app.c）。

## 风险与取舍（as-built 已知问题，如实记录）

| 问题 | 现状 | 整改指向 |
|---|---|---|
| 三态不可达 | ISLAND_L / TEETERBOARD / CASK 的入态判断未接入主状态机，实际可达路径仅 TRACKING→CROSSROADS→TRACKING 与 TRACKING→REISLAND→ISLAND_R→TRACKING | Phase 09（逐个接入前须与用户确认设计，AGENTS.md 硬性规则） |
| 环岛状态宏 11 个仅用 5 个 | 6 个宏（RPREENTER/TURN_RIGHT/EXIT/LENTER/RENTER/TURN_TURN_LEFT）无使用点 | Phase 09/11 |
| `sign_round` 只写不读 | ISLAND_L/R 态分别写 −1/1，但 `roundabout()` 从不读取，左右环岛打角方向实际由写死的 `speed_control(-30)` 决定 | Phase 09（接入左环岛时一并处理） |
| `cask_flag` 职责混杂 | 路障抑制逻辑（DL1B < 100 mm）内嵌在预环岛判断 `L_reroundabout_judge` 中，与环岛识别耦合 | Phase 09/10 |
| 增益每拍覆写与调参体系冲突 | Phase 13 配置表仍每拍覆写权重/增益/base_speed，菜单/串口双档调参（Phase 06）在运行态仍被覆盖 | Phase 10（调参语义定稿） |
| 距离阈值魔数 | 13000、32000 为内联魔数（仅 8000/10000 有命名 `enter_distance1`/`out_distance1`）；5000/2800/300/25 等阈值同样内联 | Phase 10/11 |
| `R_reroundabout_judge` 无人调用 | 与 `L_reroundabout_judge` 功能重叠的冗余判断 | Phase 11（休眠代码处置） |
| ISLAND_L 态不覆写 base_speed | 若未来接入将沿用前一状态的速度，语义隐患 | Phase 09 |
| 出赛道保护清 `key_flag` | 出赛道即回到菜单态，重新起跑需再按启动键——现状语义如此，保留记录 | 不整改（现状语义） |
