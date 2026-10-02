# Phase 04: 赛道元素状态机（现状） — 需求

> **as-built 规约**：以下需求逆向自现有代码，描述系统当前实际行为。注意 FR-8~FR-11 描述的是"代码存在但不可达"的现状，而非目标行为。

## 功能需求

- FR-1: 系统必须提供 7 态 kernel 主状态机（TRACKING/ISLAND_L/ISLAND_R/TEETERBOARD/CROSSROADS/CASK/REISLAND），以 2 ms 控制拍执行：入态覆写权重/方向增益/base_speed → 循迹偏差 → 方向 PID → 差速分配 → 本态元素进出判断。
- FR-2: 出赛道保护：4 路电感滤波值之和 ≤300 时，必须清 `key_flag`/`Start_flag`/`Run_flag`，目标速度归零，车辆停车并回到菜单态。
- FR-3: 十字进入：TRACKING 态下竖两路电感和 >2800 且竖向对称度 `symmetry_y` <25 时，必须转入 CROSSROADS；CROSSROADS 态方向输出必须限幅 ±10；竖两路和 <2800 时必须返回 TRACKING。
- FR-4: 预环岛：TRACKING 态下横两路电感和 >2800 时，必须按左右大小置 `L_round_flag`/`R_round_flag`，清零 `angle_x` 与三路里程，转入 REISLAND。
- FR-5: 右环岛进入：REISLAND 态下竖两路电感和 <300 且 `R_round_flag`==1 且 `!cask_flag` 时，必须转入 ISLAND_R 并将环岛子状态机置为 ISLAND_LPREENTER，清零 `track_out`/`angle_x`/里程。
- FR-6: 预环岛超时退出：REISLAND 态行驶里程 >5000 仍未进环时，必须返回 TRACKING 并清双环岛标志、`track_out`、`angle_x`。
- FR-7: 环岛子状态机必须以编码器里程驱动完成"预入环直行（≥8000）→ 固定打角入环（>13000）→ 环内循迹（>32000）→ 出环直行（>10000）→ 回正常循迹"序列；入环打角为固定 `speed_control(-30)`（名为 TURN_LEFT，实际服务右环岛）。
- FR-8: 路障抑制：预环岛判断中 DL1B 距离 <100 mm 时必须置 `cask_flag`，从而抑制右环岛进入（现状：该逻辑内嵌于预环岛判断函数中）。
- FR-9: 左环岛（ISLAND_L）、跷跷板（TEETERBOARD）、路障（CASK）三态的处理代码存在，但其入态判断当前**不接入**主状态机，实际不可达（现状如实记录；接入排期 Phase 09）。
- FR-10: 实际可达的状态转移仅允许为：TRACKING→CROSSROADS→TRACKING；TRACKING→REISLAND→ISLAND_R→（环岛子状态机）→TRACKING；TRACKING→REISLAND→TRACKING（超时）。
- FR-11: 跷跷板/路障的出态判断（`teeterboard_out_judge`/`cask_out_judge`）随状态机保留编译，仅在对应态被（未来）进入时生效。

## 技术约束

- TC-1: 元素判断与状态转移全部在 TIM6 2 ms 控制拍内完成（硬性约束 2），阈值均按 12 bit ADC 标度（满量程 4095）与 2 ms 拍编码器计数标定。
- TC-2: 状态以 `#define` 宏表示（非枚举），与源工程保持一致；本阶段不改枚举化（归 Phase 10/11 治理）。
- TC-3: 环岛距离变量（`enter_distance1`/`out_distance1`）为模块内可调参数，单位与源工程一致（编码器计数）。
- TC-4: 未启用元素与休眠判断函数保留编译但不调用；接入任何元素前必须逐个与用户确认设计（AGENTS.md 硬性规则）。
- TC-5: 元素判断不得改变 [Phase 03](../03-track-following/requirements.md) 循迹链的对外语义（偏差公式、差速分配、占空比万分比）。

## 接口约定（如适用）

| 接口 | 方向 | 语义 |
|---|---|---|
| `kernel_state` | 输出（全局） | kernel 主状态机当前态（0~6，宏见 `control.h`） |
| `roundabout_state` | 输出（全局） | 环岛子状态机当前态（宏见 `roundabout.h`） |
| `L_round_flag` / `R_round_flag` | 内部 | 预环岛识别的左/右环岛标志，进环或超时后清零 |
| `cask_flag` | 内部 | 路障抑制标志：DL1B <100 mm 置 1，预环岛触发时清 0；阻止右环岛进入 |
| `sign_round` | 只写不读（现状） | ISLAND_L=-1 / ISLAND_R=1，环岛子状态机未消费 |
| `enter_distance1` / `out_distance1` | 可调参数 | 预入环直行距离 8000 / 出环直行距离 10000（编码器计数） |
| 依赖输入 | — | `Track_GetState()` 的滤波值与对称度（Phase 03）；`Motor_GetState()` 的里程与 `Motor_ResetDistance()`（Phase 03）；`IMU_ZeroAngleX()`（Phase 05）；`dl1b_distance_mm`（BSP DL1B）；`time`（app.c 节拍计数） |

## 非目标（Non-goals）

- 本阶段**不**接入左环岛、跷跷板、路障元素（Phase 09 范围，接入前逐个与用户确认设计）。
- 本阶段**不**解决"双档参数 vs 状态机每拍覆写"的语义冲突（Phase 10 范围）。
- 不恢复源工程环岛角度法（已注释未移植）；不引入摄像头元素识别。
- 不清理环岛冗余宏与休眠判断函数（Phase 11 范围）。
