# Phase 05: IMU 姿态处理 — 实现计划

> **as-built 规约**：代码已存在，本文档为存量功能的规约化追认，描述现状（含已知缺陷）。任务分组全部已完成，内容为实现结构追溯而非待办。

## 目标

交付 IMU660RB 姿态处理链路：采样器高速读取原始值 → 上电零偏校准 → 去零偏与单位换算 → 陀螺 y 轴 0.2 Hz 高通 → 梯形积分解算角度，供方向环阻尼（设计意图）与赛道元素状态机（`angle_x` 清零约定）使用。

## 背景与依据

- 关联宪章：[使命](../../mission.md)（范围内第 1/2 条的姿态支撑）；[技术栈](../../techstack.md) 硬性约束 2/3（2 ms 控制拍、TIM15/TIM6 同优先级互不抢占——采样器缓存免锁的前提）。
- 算法出处：`App/imu_proc.c` 移植自源工程 `Sirius20260718` 的 `gyroscope.c`（逐飞库，仓库外，仅按名称引用）；全局变量名与逐飞库保持一致。
- 前置条件：[Phase 01 运行时架构](../01-runtime-architecture/plan.md)、[Phase 02 BSP 平台层](../02-bsp-platform/plan.md)——IMU660RB 驱动（SPI4 10 MHz、CS=PD3、ODR 1.66 kHz）与 TIM15 采样器的细节见 Phase 02 规约，本文档不重复。
- 消费方：[Phase 03](../03-track-following/plan.md)（`accel_x` 阻尼项、`gyro_x`）、[Phase 04](../04-element-fsm/plan.md)（`angle_x`/`angle_y` 与清零约定）。

## 任务分组（Task Groups）

### Group 1: 数据通路（`App/imu_proc.c` + BSP 采样器，源 `gyroscope.c`）

- [x] TIM15 采样器 1 kHz 经 SPI4 读取 IMU 陀螺 + 加速度原始值（ODR 1.66 kHz，每拍均为新数据；单次陀螺+加速度读取约 30 µs），缓存于 `BSP_Sampler`。
- [x] `read_gyro_angle()`（2 ms 控制拍内调用）：`BSP_Sampler_GetGyroRaw()` 取缓存 → 去零偏 → `/GYRO_RAW_TO_DPS`（=14.3，±2000 dps 量程）换算为 °/s。
- [x] 初始化收拢：`imu_proc_init()` 完成 `BSP_IMU660RB_Init()` + 高通滤波器初始化（源工程散在 config.c 的 `gyro_hpf_init(0.2Hz, 0.002)`，此处集中）。

### Group 2: 滤波与积分（`imu_proc.c` + `filter.c`）

- [x] **仅 gyro_y** 经 0.2 Hz 一阶高通（`gyro_hpf_y`，α=RC/(RC+dt)）抑制积分漂移；gyro_x/gyro_z 不高通。
- [x] 梯形积分：`angle_* += (gyro_* + gyro_last_*) × 0.001`（系数 0.001 = 0.5 × dt，dt = 2 ms），输出 `angle_x/y/z`。
- [x] `angle_x` 在预环岛触发（`L_reroundabout_judge`）、进右环岛（`R_roundabout_judge`）、预环岛超时退出（`reroundabout_out_judge`）时由元素状态机清零（见 [Phase 04](../04-element-fsm/plan.md)）。

### Group 3: 零偏校准

- [x] `gyro_calibrate()`：上电静止时 100 次连读取平均得 `gyro_offset_x/y/z`，并清零角度积分；由 `App_Init` 调用。**源工程中该调用被注释（源 `main.c`），本工程恢复**——有意修改，出处差异如实记录。
- [x] `accel_calibrate()`：同法得加速度零偏并清零速度积分——**休眠接口，默认无人调用**。
- [x] `read_accel_velocity()`：**休眠/保留接口**，照搬源工程实现（含已知错位：读陀螺寄存器填 acc 变量、除以 `GYRO_RAW_TO_DPS`、积分项误用 `gyro_x/y/z`），函数注释自承认该错位，默认不调用。

## 实现顺序与依赖

```
BSP_IMU660RB_Init (Phase 02) → imu_proc_init → gyro_calibrate（App_Init，上电一次）
                                                    │
TIM15 1kHz: BSP_IMU660RB_GetGyro/GetAcc → s_gyro/s_acc 缓存（BSP_Sampler）
                                                    │
TIM6 2ms: read_gyro_angle ← BSP_Sampler_GetGyroRaw ← 去零偏 → /14.3 → gyro_y 过 0.2Hz 高通 → 梯形积分 → angle_x/y/z
```

关键点：校准发生在 `BSP_Sampler_Init` 之后但 TIM15 未启动前，`gyro_calibrate` 直接走 `BSP_IMU660RB_GetGyro`（非采样器缓存）；运行期 `read_gyro_angle` 只消费采样器缓存，不在控制拍内触碰 SPI4（与 LCD 互斥语义的架构前提，见 [Phase 01](../01-runtime-architecture/plan.md)）。

## 风险与取舍（as-built 已知问题，如实记录）

| 问题 | 现状 | 整改指向 |
|---|---|---|
| **`accel_x` 运行时恒为 0** | `accel_x` 仅由 `read_accel_velocity()` 写入，该函数无人调用；采样器虽缓存加速度原始值，App 层无人消费。Phase 03 方向环的 `K2D_x·accel_x` 阻尼项因此实际不产生作用 | Phase 09/10（接入真实加速度前馈或裁剪该项，须与用户确认） |
| `gyro_hpf_x` 初始化但从未使用 | `imu_proc_init` 初始化了 x/y 两路高通，仅 y 路被 `read_gyro_angle` 使用 | Phase 11（休眠代码处置） |
| 校准无静止检测 | `gyro_calibrate()` 直接连读 100 次取平均（循环内无采样间隔延时，阻塞为毫秒级），若上电时车辆被晃动，零偏即被污染，无任何判稳/重试 | Phase 11 |
| `read_accel_velocity` 错位实现 | 照搬源工程已知错误（读陀螺填 acc、除陀螺系数、积分用 gyro 变量），注释已自承认；保留仅为接口兼容 | Phase 11（休眠代码清单处置） |
| `accel_calibrate` 休眠 | 无人调用，加速度零偏始终为 0 | Phase 11 |
| `angle_y`/`angle_z` 积分结果无消费方 | 仅 `cask_judge`（不可达，Phase 04）读 `angle_y` | Phase 09/11 |
