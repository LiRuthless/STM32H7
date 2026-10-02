# Phase 05: IMU 姿态处理 — 需求

> **as-built 规约**：以下需求逆向自现有代码，描述系统当前实际行为，均可观测、可检验。FR-7/FR-8 描述的是休眠接口的现状，而非目标行为。

## 功能需求

- FR-1: 系统必须以 TIM15 采样器 1 kHz 经 SPI4 读取 IMU660RB 陀螺与加速度原始值（IMU ODR 1.66 kHz，每拍为新数据），控制环经采样器缓存取用，控制拍内不直接访问 SPI4。
- FR-2: 上电时必须执行陀螺零偏校准：静止状态下 100 次连读取平均得 `gyro_offset_x/y/z`，并清零角度积分（源工程此调用被注释，本工程恢复，属有意差异）。
- FR-3: 2 ms 控制拍内必须执行姿态解算：陀螺原始值去零偏后除以 14.3（`GYRO_RAW_TO_DPS`，±2000 dps 量程）换算为 °/s。
- FR-4: 仅 y 轴角速度必须经 0.2 Hz 一阶高通滤波（α=RC/(RC+dt)）抑制积分漂移；x/z 轴不高通。
- FR-5: 三轴角度必须以梯形积分输出：`angle += (gyro + gyro_last) × 0.001`（0.001 = 0.5 × 2 ms）。
- FR-6: 必须允许元素状态机在预环岛触发、进右环岛、预环岛超时时清零 `angle_x`（环岛打角的角度基准约定，见 [Phase 04](../04-element-fsm/requirements.md)）。
- FR-7: `accel_x/y/z` 与 `velocity_x/y/z` 的更新接口（`read_accel_velocity`）保留编译但默认不调用——**因此 `accel_x` 运行时恒为 0**，Phase 03 方向环的 `K2D_x·accel_x` 项实际不产生作用（现状如实记录）。
- FR-8: `accel_calibrate()` 保留编译但默认不调用；加速度零偏始终为 0。

## 技术约束

- TC-1: 姿态解算在 TIM6 = 2 ms 控制拍执行，积分系数 0.001 与该周期绑定（硬性约束 2）；改控制周期必须同步改积分系数与高通 dt。
- TC-2: 采样器缓存读取依赖 TIM15 与 TIM6 同占优先级 0、互不抢占的免锁前提（硬性约束 3）。
- TC-3: IMU 与 LCD 共用 SPI4：运行期只有采样器访问 IMU，停车刷屏期不读 IMU——互斥语义不得破坏。
- TC-4: IMU 驱动细节（寄存器配置、SPI 时序、自检）归属 Phase 02 BSP 规约，本层只消费 `BSP_Sampler_GetGyroRaw/GetAccRaw` 与 `BSP_IMU660RB_GetGyro/GetAcc`。
- TC-5: 休眠接口（`read_accel_velocity`、`accel_calibrate`）保留现状；x 轴高通实例在 Phase 13 收归 IMU 模块私有，仍不参与运行计算。启用或删除休眠逻辑前必须按 AGENTS.md 规则与用户确认。

## 接口约定（如适用）

| 接口 | 方向 | 语义 |
|---|---|---|
| `IMU_GetState()->gyro[0..2]` | 只读指针 | 角速度 °/s（已去零偏、÷14.3；仅 y 轴过高通） |
| `IMU_GetState()->angle[0..2]` | 只读指针 | 角度积分值（°）；x 轴供环岛打角基准，y/z 轴当前无有效消费方 |
| `IMU_GetState()->accel[0..2]` | 只读指针，现状恒 0 | 加速度（设计意图：方向环阻尼）；运行时从不更新 |
| `IMU_GetState()->raw_gyro/raw_accel[0..2]` | 只读指针 | 最近一次原始值；日志仍使用 TIM15 采样器的原始值 |
| `IMU_ZeroAngleX()` | 写接口 | 在原有预环岛、进环、超时路径仅清零 x 轴积分角度 |
| `read_gyro_angle()` | 函数 | 2 ms 控制拍姿态解算入口 |
| `gyro_calibrate()` / `accel_calibrate()` | 函数 | 上电静止校准（后者休眠） |
| `BSP_Sampler_GetGyroRaw()` | 依赖（BSP） | 采样器 1 kHz 陀螺缓存 |

## 非目标（Non-goals）

- 不做姿态融合（互补滤波/卡尔曼）：现状为纯陀螺积分 + y 轴高通，源工程语义不变。
- 不修复 `read_accel_velocity` 的错位实现，不启用加速度前馈（Phase 09/10 评估）。
- 不加校准静止检测（Phase 11 评估）。
- 不重复描述 IMU660RB 驱动细节（属 [Phase 02](../02-bsp-platform/plan.md)）。
