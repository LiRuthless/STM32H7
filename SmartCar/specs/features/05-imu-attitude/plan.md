# Phase 05: IMU 姿态处理 — 实现计划

> **as-built 规约**：代码已存在，本文档为存量功能的规约化追认，描述现状（含已知缺陷）。任务分组全部已完成，内容为实现结构追溯而非待办。

## 目标

> Phase 13 将本阶段的三轴全局量收归 `imu_state_t`，高通实例改为模块私有；本文件下方旧变量名表示相同的姿态数值，当前接口见 [Phase 13](../13-control-structure/plan.md)。

交付 IMU660RB 姿态处理链路：采样器高速读取原始值 → 上电零偏校准 → 去零偏与单位换算 → 陀螺 y 轴 0.2 Hz 高通 → 梯形积分解算角度，供方向环阻尼（设计意图）与赛道元素状态机（`angle_x` 清零约定）使用。

## 背景与依据

- 关联宪章：[使命](../../mission.md)（范围内第 1/2 条的姿态支撑）；[技术栈](../../techstack.md) 硬性约束 2/3（2 ms 控制拍、TIM15/TIM6 同优先级互不抢占——采样器缓存免锁的前提）。
- 算法出处：`App/imu_proc.c` 移植自源工程 `Sirius20260718` 的 `gyroscope.c`（逐飞库，仓库外，仅按名称引用）；全局变量名与逐飞库保持一致。
- 前置条件：[Phase 01 运行时架构](../01-runtime-architecture/plan.md)、[Phase 02 BSP 平台层](../02-bsp-platform/plan.md)——IMU660RB 驱动（SPI4 10 MHz、CS=PD3、ODR 1.66 kHz）与 TIM15 采样器的细节见 Phase 02 规约，本文档不重复。
- 消费方：[Phase 03](../03-track-following/plan.md)（`accel_x` 阻尼项、`gyro_x`）、[Phase 04](../04-element-fsm/plan.md)（`angle_x`/`angle_y` 与清零约定）。

## 任务分组（Task Groups）

### Group 1: 数据通路（`App/imu_proc.c` + BSP 采样器，源 `gyroscope.c`）

- [x] TIM15 采样器 1 kHz 经 SPI4 读取 IMU 陀螺 + 加速度原始值（ODR 1.66 kHz，每拍均为新数据；单次陀螺+加速度读取约 30 µs），缓存于 `BSP_Sampler`。
- [x] `IMU_UpdateGyroAngle()`（2 ms 控制拍内调用）：`BSP_Sampler_GetGyroRaw()` 取缓存 → 去零偏 → `/IMU_GYRO_RAW_TO_DPS`（=14.3，±2000 dps 量程）换算为 °/s。
- [x] 初始化收拢：`IMU_Init()` 完成 `BSP_IMU660RB_Init()` + 高通滤波器初始化（源工程散在 config.c 的 `Filter_InitHighpass(0.2Hz, 0.002)`，此处集中）。

### Group 2: 滤波与积分（`imu_proc.c` + `filter.c`）

- [x] **仅 gyro_y** 经 0.2 Hz 一阶高通（`gyro_hpf_y`，α=RC/(RC+dt)）抑制积分漂移；gyro_x/gyro_z 不高通。
- [x] 梯形积分：`angle_* += (gyro_* + gyro_last_*) × 0.001`（系数 0.001 = 0.5 × dt，dt = 2 ms），输出 `angle_x/y/z`。
- [x] `angle_x` 在预环岛触发（`Roundabout_JudgePreentryLeft`）、进右环岛（`Roundabout_JudgeEntryRight`）、预环岛超时退出（`Roundabout_JudgePreentryExit`）时由元素状态机清零（见 [Phase 04](../04-element-fsm/plan.md)）。

### Group 3: 零偏校准

- [x] `IMU_CalibrateGyro()`：上电静止时 100 次连读取平均得 `gyro_offset_x/y/z`，并清零角度积分；由 `App_Init` 调用。**源工程中该调用被注释（源 `main.c`），本工程恢复**——有意修改，出处差异如实记录。
- [x] `IMU_CalibrateAccel()`：同法得加速度零偏并清零速度积分——**休眠接口，默认无人调用**。
- [x] `IMU_UpdateAccelVelocity()`：**休眠/保留接口**，照搬源工程实现（含已知错位：读陀螺寄存器填 acc 变量、除以 `IMU_GYRO_RAW_TO_DPS`、积分项误用 `gyro_x/y/z`），函数注释自承认该错位，默认不调用。

## 实现顺序与依赖

```
BSP_IMU660RB_Init (Phase 02) → IMU_Init → IMU_CalibrateGyro（App_Init，上电一次）
                                                    │
TIM15 1kHz: BSP_IMU660RB_GetGyro/GetAcc → s_gyro/s_acc 缓存（BSP_Sampler）
                                                    │
TIM6 2ms: IMU_UpdateGyroAngle ← BSP_Sampler_GetGyroRaw ← 去零偏 → /14.3 → gyro_y 过 0.2Hz 高通 → 梯形积分 → angle_x/y/z
```

关键点：校准发生在 `BSP_Sampler_Init` 之后但 TIM15 未启动前，`IMU_CalibrateGyro` 直接走 `BSP_IMU660RB_GetGyro`（非采样器缓存）；运行期 `IMU_UpdateGyroAngle` 只消费采样器缓存，不在控制拍内触碰 SPI4（与 LCD 互斥语义的架构前提，见 [Phase 01](../01-runtime-architecture/plan.md)）。

## 风险与取舍（as-built 已知问题，如实记录）

| 问题 | 现状 | 整改指向 |
|---|---|---|
| **`accel_x` 运行时恒为 0** | `accel_x` 仅由 `IMU_UpdateAccelVelocity()` 写入，该函数无人调用；采样器虽缓存加速度原始值，App 层无人消费。Phase 03 方向环的 `k2d_x·accel_x` 阻尼项因此实际不产生作用 | Phase 09/10（接入真实加速度前馈或裁剪该项，须与用户确认） |
| `gyro_hpf_x` 初始化但从未使用 | `IMU_Init` 初始化了 x/y 两路高通，仅 y 路被 `IMU_UpdateGyroAngle` 使用 | Phase 11（休眠代码处置） |
| 校准无静止检测 | `IMU_CalibrateGyro()` 直接连读 100 次取平均（循环内无采样间隔延时，阻塞为毫秒级），若上电时车辆被晃动，零偏即被污染，无任何判稳/重试 | Phase 11 |
| `IMU_UpdateAccelVelocity` 错位实现 | 照搬源工程已知错误（读陀螺填 acc、除陀螺系数、积分用 gyro 变量），注释已自承认；保留仅为接口兼容 | Phase 11（休眠代码清单处置） |
| `IMU_CalibrateAccel` 休眠 | 无人调用，加速度零偏始终为 0 | Phase 11 |
| `angle_y`/`angle_z` 积分结果无消费方 | 仅 `Element_JudgeCask`（不可达，Phase 04）读 `angle_y` | Phase 09/11 |

> 2026-10-16 Phase 15 G2：现行传感器/控制符号按[完整映射](../15-code-style-consistency/symbol-map.md)迁移，
> 配置、计算顺序及休眠调用边保持；历史 validation 原文及本阶段实车门禁不变。

> 2026-10-16 Phase 15 G3：现行调度、HMI、参数和日志名称已按[符号表](../15-code-style-consistency/symbol-map.md)统一。
> 本文原阶段行为记录与未完成待办不因此重新验收；现行运行链以 Phase 14 和当前头文件契约为准，历史 validation 保留原文。

## Phase 17 当前实现定位（2026-10-24）

App/sensing/imu_proc继续唯一持有IMU状态、零偏、高通和梯形历史，legacy_imu只接受同一owner显式传入的状态及地址。IMU_UpdateAccelVelocity/CalibrateAccel旧外壳保留且未新增活动调用；原Gyro/raw_accel字段错位、14.3换算、Y轴高通和复位不重校准均未借本次重构修正。

本节是现行归属导航；上文历史计划、验证、提交路径及未完成实机项目保持。实际重构验证见
[Phase17验收](../17-app-architecture/validation.md)，逐符号迁移见[迁移导航](../17-app-architecture/migration-map.md)。
