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

## Phase 17 G1 路径增量

旧元素/环岛/PID暂位于App/control，IMU位于App/sensing；仅目录搬迁，不改变接入状态。后续legacy隔离尚未实施。

## Phase 17 最终位置与调用边界（2026-10-24）

上文是2026-10-02及G1时点记录，旧名/路径/当时无调用结论保持；现行位置如下。
本次只隔离、封装与保留，不执行候选删除或功能接入。

| 原编号 | 现行模块/符号 | 当前实际边界 |
|---|---|---|
| 1 | control/legacy_pid.c::PID_UpdateIncrementalLeft/Right | 休眠增量历史各自static保留，ResetAll不新增清它们 |
| 2 | control/legacy_control.c::PID_UpdateAngle → PID_CalculateAngle → angle_controller.c | 旧角度包装保留原Motor_DistributeSpeed副作用；纯角度算法无电机动作，活动链未新增包装调用 |
| 3/4 | sensing/imu_proc.c的原公开外壳 → legacy_imu.c两入口 | 使用同一IMU状态/偏置/历史，旧公式和原读取字段错位保留，未接入活动调度 |
| 5/6 | sensing/track_sensor.c::Track_UpdateSymmetry/Track_InitAdc | 原辅助入口保留；活动误差链仍已有对称度计算，ADC由生命周期原BSP初始化位置负责 |
| 7 | control/motor.c::Motor_Init/Motor_InitEncoder | Motor_Init仍为旧薄包装；Motor_InitEncoder已由原安全闭环整改在生命周期初始化调用，不能继续称两者都无调用 |
| 8/9 | control/legacy_element.c全部Element_* | 私有s_enter_tick和原内部汇总调用保留，不新增活动元素调用 |
| 10 | control/legacy_roundabout.c全部Roundabout_* | 旧判断/执行与空壳保留内部边；活动右环判断在electromagnetic_fsm，未接入旧实现 |
| 11 | control/control_profile.h的ROUNDABOUT_*常量 | 未用阶段数值/宏保持，活动与旧入口共用同一组常量，不因隔离启用阶段 |
| 12/13 | legacy_roundabout.c::s_roundabout各标志/旧角度字段 | 状态私有，reset仍按原起跑路径调用；sign_round与三个旧角度的未生效现状保留，不另复制活动配置 |
| 14 | BSP/bsp_sampler.c::BSP_Sampler_Stop | 已由原安全闭环整改接入首次停车，Phase17保持此实际调用；不按旧调研表认定休眠 |
| 15 | BSP/bsp_w25q64.c维护接口 | 原样保留，不新增调用或裁剪 |
| 16 | sensing/imu_proc.c::s_gyro_hpf_x | 仍初始化/reset，活动角速度只Y轴高通；不启用X轴滤波 |
| 17 | control/pid.c::s_pid.ki_x/kd_v | 字段与设置入口保留，既有算法未使用的现状不变 |
| 18 | services/wireless.c::s_uart_tx_buffer/s_uart_rx_buffer | 各32B私有缓冲，唯一原使用者负责；不增第二套缓冲 |
| 19 | App/app_config.h原未用宏 | 数值/定义保留，不借分类删除或接通 |
| 20 | BSP/lcd/st7735*.c厂商接口 | 第三方边界保持，不作此次架构重构 |

另有control/legacy_pid.c::PID_UpdateTrack仍供停车前台预览及旧环岛原调用使用，不能把整个
legacy文件当作无活动调用。起跑复位壳/原历史读取按原调用图保留；隔离只阻止新增旧算法依赖。
旧整体Control_UpdateLegacy位于legacy_control.c，产品仍只有定义/声明，无活动调用。
2026-10-24用户明确批准两条Observe返回后即时同步，仅恢复拆分前已有的版本发布副作用；
具体规约和正/负回归见Phase17 G4-09，不表示启用旧控制入口。
逐函数真实调用与对象所有权由[最终迁移导航](../features/17-app-architecture/migration-map.md)
及索引核对，新增接入/处置仍须逐项用户确认。历史硬件待验和R07阻塞保持。

## 2026-10-31 现行功能入口

本清单保留各时点调查和决定；当前活动/休眠边界、实际调用及实现位置先见[docs F13](../../docs/control/elements-and-dormant-code.md#f13)。历史“无调用”不自动用于当前调用图；全部保留/接入/删除仍须逐项确认，未因目录隔离关闭。
