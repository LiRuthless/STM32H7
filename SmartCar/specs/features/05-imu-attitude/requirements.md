# Phase 05: IMU 姿态处理 — 需求

2026-10-31文档迁移：**Phase → docs → 代码**。本页保留需求/任务/验收历史，不新增阶段、不修改实现或重新验收。

当前需求导航：[P05功能索引](../../../docs/traceability.md#p05)。具体输入输出、状态/错误/时序、代码符号与验证方法由下列docs条目唯一维护。

## 需求意图与现行功能条目

编号身份及原用户决定保留；摘要不替代详细契约。涉及冲突时，以docs中并列的当前事实、既有要求和待决定边界阅读，不能自动选代码或旧文档为新设计。

<a id="fr-01"></a>
### FR-1 缓存采样

目标与实现依据：[F07 IMU校准滤波与积分](../../../docs/sensing/sensors-and-calibration.md#f07)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-02"></a>
### FR-2 零偏

目标与实现依据：[F07 IMU校准滤波与积分](../../../docs/sensing/sensors-and-calibration.md#f07)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-03"></a>
### FR-3 角速度

目标与实现依据：[F07 IMU校准滤波与积分](../../../docs/sensing/sensors-and-calibration.md#f07)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-04"></a>
### FR-4 高通

目标与实现依据：[F07 IMU校准滤波与积分](../../../docs/sensing/sensors-and-calibration.md#f07)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-05"></a>
### FR-5 梯形积分

目标与实现依据：[F07 IMU校准滤波与积分](../../../docs/sensing/sensors-and-calibration.md#f07)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-06"></a>
### FR-6 角度基准

目标与实现依据：[F07 IMU校准滤波与积分](../../../docs/sensing/sensors-and-calibration.md#f07)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-07"></a>
### FR-7 休眠速度积分

目标与实现依据：[F07 IMU校准滤波与积分](../../../docs/sensing/sensors-and-calibration.md#f07)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-08"></a>
### FR-8 休眠加速度校准

目标与实现依据：[F07 IMU校准滤波与积分](../../../docs/sensing/sensors-and-calibration.md#f07)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="tc-01"></a>
### TC-1 姿态采样与共享总线（第1项）

目标与实现依据：[F07 IMU校准滤波与积分](../../../docs/sensing/sensors-and-calibration.md#f07)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="tc-02"></a>
### TC-2 姿态采样与共享总线（第2项）

目标与实现依据：[F29 时钟DMA采样与驱动](../../../docs/platform/board-and-bsp.md#f29)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="tc-03"></a>
### TC-3 姿态采样与共享总线（第3项）

目标与实现依据：[F29 时钟DMA采样与驱动](../../../docs/platform/board-and-bsp.md#f29)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="tc-04"></a>
### TC-4 姿态采样与共享总线（第4项）

目标与实现依据：[F29 时钟DMA采样与驱动](../../../docs/platform/board-and-bsp.md#f29)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="tc-05"></a>
### TC-5 姿态采样与共享总线（第5项）

目标与实现依据：[F13 legacy与未接入元素](../../../docs/control/elements-and-dormant-code.md#f13)。状态：当前契约及历史替代见docs；不改变原验收状态。

## 授权与完成边界

本轮仅迁移说明。历史用户决定、动机、来源、兼容性及未决事项完整保留在下方基线；新的行为决定须明确日期、理由和适用范围，更新本需求及docs目标后独立提交，再进入实现。不得用本次文档完成替代AC5、R07原样本或任何硬件待验。

## 冻结历史基线（68cb508，非现行契约）

以下原文用于保留阶段决定、组顺序、需求历史与真实验证；不继续独立维护算法/布局/源码映射。当前契约先查上方docs条目。旧“本轮”“待执行”“当前”只指原记录时点，不覆盖后续决定。历史任务的全量软件门禁不自动成为以后每次修改的要求。

<details>
<summary>展开原始阶段记录（原文保留）</summary>

# Phase 05: IMU 姿态处理 — 需求

> **2026-10-16 增量状态**：[Phase 14](../14-dual-mode-tracking/requirements.md) FR-10/11 已提供只读本拍角度积分增量供电磁独立参考累计；影子路径不得调用IMU_ZeroAngleX或改变实际积分历史。现有校准/积分公式保持、休眠速度积分不接入，映射及验收见 [Phase 14 计划](../14-dual-mode-tracking/plan.md) / [验证](../14-dual-mode-tracking/validation.md)。 2026-10-16实现状态及V-01～15通过证据见上述Phase14验证页，V-16～20仍待上机。

> **as-built 规约**：以下需求逆向自现有代码，描述系统当前实际行为，均可观测、可检验。FR-7/FR-8 描述的是休眠接口的现状，而非目标行为。

## 功能需求

- FR-1: 系统必须以 TIM15 采样器 1 kHz 经 SPI4 读取 IMU660RB 陀螺与加速度原始值（IMU ODR 1.66 kHz，每拍为新数据），控制环经采样器缓存取用，控制拍内不直接访问 SPI4。
- FR-2: 上电时必须执行陀螺零偏校准：静止状态下 100 次连读取平均得 `gyro_offset_x/y/z`，并清零角度积分（源工程此调用被注释，本工程恢复，属有意差异）。
- FR-3: 2 ms 控制拍内必须执行姿态解算：陀螺原始值去零偏后除以 14.3（`IMU_GYRO_RAW_TO_DPS`，±2000 dps 量程）换算为 °/s。
- FR-4: 仅 y 轴角速度必须经 0.2 Hz 一阶高通滤波（α=RC/(RC+dt)）抑制积分漂移；x/z 轴不高通。
- FR-5: 三轴角度必须以梯形积分输出：`angle += (gyro + gyro_last) × 0.001`（0.001 = 0.5 × 2 ms）。
- FR-6: 必须允许元素状态机在预环岛触发、进右环岛、预环岛超时时清零 `angle_x`（环岛打角的角度基准约定，见 [Phase 04](../04-element-fsm/requirements.md)）。
- FR-7: `accel_x/y/z` 与 `velocity_x/y/z` 的更新接口（`IMU_UpdateAccelVelocity`）保留编译但默认不调用——**因此 `accel_x` 运行时恒为 0**，Phase 03 方向环的 `k2d_x·accel_x` 项实际不产生作用（现状如实记录）。
- FR-8: `IMU_CalibrateAccel()` 保留编译但默认不调用；加速度零偏始终为 0。

## 技术约束

- TC-1: 姿态解算在 TIM6 = 2 ms 控制拍执行，积分系数 0.001 与该周期绑定（硬性约束 2）；改控制周期必须同步改积分系数与高通 dt。
- TC-2: 采样器缓存读取依赖 TIM15 与 TIM6 同占优先级 0、互不抢占的免锁前提（硬性约束 3）。
- TC-3: IMU 与 LCD 共用 SPI4：运行期只有采样器访问 IMU，停车刷屏期不读 IMU——互斥语义不得破坏。
- TC-4: IMU 驱动细节（寄存器配置、SPI 时序、自检）归属 Phase 02 BSP 规约，本层只消费 `BSP_Sampler_GetGyroRaw/GetAccRaw` 与 `BSP_IMU660RB_GetGyro/GetAcc`。
- TC-5: 休眠接口（`IMU_UpdateAccelVelocity`、`IMU_CalibrateAccel`）保留现状；x 轴高通实例在 Phase 13 收归 IMU 模块私有，仍不参与运行计算。启用或删除休眠逻辑前必须按 AGENTS.md 规则与用户确认。

## 接口约定（如适用）

| 接口 | 方向 | 语义 |
|---|---|---|
| `IMU_GetState()->gyro[0..2]` | 只读指针 | 角速度 °/s（已去零偏、÷14.3；仅 y 轴过高通） |
| `IMU_GetState()->angle[0..2]` | 只读指针 | 角度积分值（°）；x 轴供环岛打角基准，y/z 轴当前无有效消费方 |
| `IMU_GetState()->accel[0..2]` | 只读指针，现状恒 0 | 加速度（设计意图：方向环阻尼）；运行时从不更新 |
| `IMU_GetState()->raw_gyro/raw_accel[0..2]` | 只读指针 | 最近一次原始值；日志仍使用 TIM15 采样器的原始值 |
| `IMU_ZeroAngleX()` | 写接口 | 在原有预环岛、进环、超时路径仅清零 x 轴积分角度 |
| `IMU_UpdateGyroAngle()` | 函数 | 2 ms 控制拍姿态解算入口 |
| `IMU_CalibrateGyro()` / `IMU_CalibrateAccel()` | 函数 | 上电静止校准（后者休眠） |
| `BSP_Sampler_GetGyroRaw()` | 依赖（BSP） | 采样器 1 kHz 陀螺缓存 |

## 非目标（Non-goals）

- 不做姿态融合（互补滤波/卡尔曼）：现状为纯陀螺积分 + y 轴高通，源工程语义不变。
- 不修复 `IMU_UpdateAccelVelocity` 的错位实现，不启用加速度前馈（Phase 09/10 评估）。
- 不加校准静止检测（Phase 11 评估）。
- 不重复描述 IMU660RB 驱动细节（属 [Phase 02](../02-bsp-platform/plan.md)）。

> 2026-10-16 Phase 15 G2：现行传感器/控制符号按[完整映射](../15-code-style-consistency/symbol-map.md)迁移，
> 配置、计算顺序及休眠调用边保持；历史 validation 原文及本阶段实车门禁不变。

> 2026-10-16 Phase 15 G3：现行调度、HMI、参数和日志名称已按[符号表](../15-code-style-consistency/symbol-map.md)统一。
> 本文原阶段行为记录与未完成待办不因此重新验收；现行运行链以 Phase 14 和当前头文件契约为准，历史 validation 保留原文。

## Phase 17 当前位置（2026-10-23）

App按system/sensing/control/services/hmi归类。现行文件及逐函数位置见[迁移清单](../17-app-architecture/migration-map.md)。
历史路径、符号与验证结果保留；本说明不表示后续职责拆分或硬件验收完成。

</details>
