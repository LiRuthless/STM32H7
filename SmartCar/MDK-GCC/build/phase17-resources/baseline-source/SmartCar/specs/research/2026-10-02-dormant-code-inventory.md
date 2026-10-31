# 调研：休眠代码清单与处置意向

- 日期：2026-10-02
- 状态：待评估

## 问题

移植与演进过程中积累了一批"存在但无调用"的代码。用户明确：其中**一部分是测试辅助，一部分是尚未接入的新功能**，处置前需逐项与用户确认。本清单是逐项确认与 Phase 11 清理的共同依据。

## 盘点口径

- **无调用**：全工程 Grep 无调用点（2026-10-02 基线）。
- **只写不读 / 恒无效**：有引用但逻辑上不起作用。
- 不含 ST vendor 组件（`BSP/lcd/st7735*.c`）中组件层自带、项目未用的接口——属 vendor 保留代码，裁剪代价高于保留，单独归类。

## 休眠代码清单

| # | 代码 | 位置 | 现状 | 初步处置意向（待用户确认） |
|---|---|---|---|---|
| 1 | `PID_L()` / `PID_R()`（增量式速度环） | `App/pid.c` | 无调用；源工程即死代码 | 测试辅助候选：与位置式对照测试用 |
| 2 | `PID_angle(target)`（角度环 PD） | `App/pid.c` | 无调用；环岛打角已改固定差速 | 待接入候选：Phase 09 左环岛/精确打角可能复用 |
| 3 | `read_accel_velocity()` | `App/imu_proc.c` | 无调用；注释自承认移植了源工程错位实现 | 待评估；注意 `accel_x` 只由它写入→方向环 D2 前馈**运行时恒为 0** |
| 4 | `accel_calibrate()` | `App/imu_proc.c` | 无调用 | 待评估：加速度零偏校准是否需要 |
| 5 | `symmetry_adc()` | `App/track_sensor.c` | 无调用（对称度在 `get_track_error()` 内顺带算） | 候选删除或改为调试显示用 |
| 6 | `front_adc_init()` | `App/track_sensor.c` | 无调用（薄封装） | 候选删除 |
| 7 | `motor_init()` / `encoder_init()` | `App/motor.c` | 无调用（App_Init 直接调 BSP） | 候选删除或改为 App 层统一入口 |
| 8 | `element_judge()` / `straight_judge()` | `App/element.c` | 汇总壳无调用 / 空壳 | 待接入候选：Phase 09 元素补齐的总入口 |
| 9 | `teeterboard_judge()` / `cask_judge()` | `App/element.c` | 只被无调用的 `element_judge` 引用 | **待接入新功能**：Phase 09 Group 2/3 |
| 10 | `R_reroundabout_judge()` / `entered_entered_judge()` / `L_roundabout_judge()` / `pre_out_judge()` / `judge()` | `App/roundabout.c` | 均无调用 | 待评估：部分可能是左环岛预留（Phase 09 Group 1） |
| 11 | 环岛状态宏 6 个未用（RPREENTER/TURN_RIGHT/EXIT/LENTER/RENTER/TURN_TURN_LEFT） | `App/roundabout.h` | 11 个宏实际用 5 个 | 待接入候选：左环岛对称流程 |
| 12 | `sign_round` | `App/roundabout.c` | **只写不读**：control.c 赋值，全工程无读取点；左右环岛方向选择实际未生效 | 缺陷倾向：Phase 09 Group 1 处置 |
| 13 | `enter_angle1/2`、`out_angle1` | `App/roundabout.c` | 定义未使用（角度驱动的遗留，现纯距离驱动） | 待评估：角度驱动方案是否复活 |
| 14 | `BSP_Sampler_Stop()` | `BSP/bsp_sampler.c` | 无调用者 | 保留：调试/测试辅助 |
| 15 | `BSP_W25Q64_EraseBlock64K()` / `EraseChip()` | `BSP/bsp_w25q64.c` | 无调用者（通用 API） | 保留：维护工具向（EraseChip 注释已标慎用） |
| 16 | `gyro_hpf_x` 实例 | `App/filter.c` | 初始化但 gyro_x 从未滤波（仅 y 轴过 HPF） | 待评估：x 轴是否也需要高通 |
| 17 | `KI_x`、`KD_v` | `App/pid.c` | 导出但无作用（对应项已注释/未用） | 候选删除或注释说明 |
| 18 | `uart[]` / `dat[]` | `App/app.c` | 名义全局，实际仅 wireless.c 使用 | 候选收编：移入 wireless.c |
| 19 | 宏 `ACC_RAW_TO_G` / `CTRL_PERIOD_MS` / `TASK_PERIOD_MS` / `DEBUG_UART_BAUD` | `App/app_config.h` | 未被引用或仅注释出现 | 候选删除或落实为真实配置源 |
| 20 | ST7735 组件层未用接口（DrawBitmap/SetPixel/ReadID 等） | `BSP/lcd/st7735*.c` | vendor 组件自带 | 保留不裁剪（vendor 代码） |

## 结论与建议

1. **不在本次文档任务中删除任何代码**。逐项处置在 Phase 09（待接入类）与 Phase 11（清理类）执行，执行前逐项与用户确认本表"初步处置意向"列。
2. #3 与 #12 不只是死代码问题，而是**隐性缺陷**（D2 前馈恒 0、方向选择未生效），建议随 Phase 09 优先确认。
3. 确认后本表"处置意向"列更新，采纳项转入 roadmap 对应阶段并回链本文件。

## 原始决策记录

- 用户（2026-10-02）："一部分未使用的代码是用作测试，或还未加入的新功能，可以由你来将代码继续完善，具体如何实现，你逐个稳态问我"——据此建立本清单与"逐个确认"流程，并写入 `AGENTS.md` 工作规则。
