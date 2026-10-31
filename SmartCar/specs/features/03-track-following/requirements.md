# Phase 03: 电磁循迹控制链 — 需求

2026-10-31文档迁移：**Phase → docs → 代码**。本页保留需求/任务/验收历史，不新增阶段、不修改实现或重新验收。

当前需求导航：[P03功能索引](../../../docs/traceability.md#p03)。具体输入输出、状态/错误/时序、代码符号与验证方法由下列docs条目唯一维护。

## 需求意图与现行功能条目

编号身份及原用户决定保留；摘要不替代详细契约。涉及冲突时，以docs中并列的当前事实、既有要求和待决定边界阅读，不能自动选代码或旧文档为新设计。

<a id="fr-01"></a>
### FR-1 电感处理

目标与实现依据：[F05 电感偏差与对称度](../../../docs/sensing/sensors-and-calibration.md#f05)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-02"></a>
### FR-2 归一化偏差

目标与实现依据：[F05 电感偏差与对称度](../../../docs/sensing/sensors-and-calibration.md#f05)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-03"></a>
### FR-3 对称度

目标与实现依据：[F05 电感偏差与对称度](../../../docs/sensing/sensors-and-calibration.md#f05)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-04"></a>
### FR-4 方向控制

目标与实现依据：[F08 方向角度速度与执行](../../../docs/control/tracking-and-controllers.md#f08)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-05"></a>
### FR-5 差速

目标与实现依据：[F08 方向角度速度与执行](../../../docs/control/tracking-and-controllers.md#f08)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-06"></a>
### FR-6 独立轮速PI

目标与实现依据：[F08 方向角度速度与执行](../../../docs/control/tracking-and-controllers.md#f08)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-07"></a>
### FR-7 方向PWM

目标与实现依据：[F26 ADC编码器PWM](../../../docs/platform/board-and-bsp.md#f26)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-08"></a>
### FR-8 编码器反馈

目标与实现依据：[F08 方向角度速度与执行](../../../docs/control/tracking-and-controllers.md#f08)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-09"></a>
### FR-9 失线停车

目标与实现依据：[F02 起停、模式与首因](../../../docs/system/runtime-and-safety.md#f02)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-10"></a>
### FR-10 滤波组件

目标与实现依据：[F05 电感偏差与对称度](../../../docs/sensing/sensors-and-calibration.md#f05)；[F07 IMU校准滤波与积分](../../../docs/sensing/sensors-and-calibration.md#f07)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="tc-01"></a>
### TC-1 算法与并发兼容约束（第1项）

目标与实现依据：[F01 初始化与三线调度](../../../docs/system/runtime-and-safety.md#f01)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="tc-02"></a>
### TC-2 算法与并发兼容约束（第2项）

目标与实现依据：[F29 时钟DMA采样与驱动](../../../docs/platform/board-and-bsp.md#f29)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="tc-03"></a>
### TC-3 算法与并发兼容约束（第3项）

目标与实现依据：[F26 ADC编码器PWM](../../../docs/platform/board-and-bsp.md#f26)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="tc-04"></a>
### TC-4 算法与并发兼容约束（第4项）

目标与实现依据：[F29 时钟DMA采样与驱动](../../../docs/platform/board-and-bsp.md#f29)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="tc-05"></a>
### TC-5 算法与并发兼容约束（第5项）

目标与实现依据：[F08 方向角度速度与执行](../../../docs/control/tracking-and-controllers.md#f08)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="tc-06"></a>
### TC-6 算法与并发兼容约束（第6项）

目标与实现依据：[F13 legacy与未接入元素](../../../docs/control/elements-and-dormant-code.md#f13)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="tc-07"></a>
### TC-7 算法与并发兼容约束（第7项）

目标与实现依据：[F08 方向角度速度与执行](../../../docs/control/tracking-and-controllers.md#f08)。状态：当前契约及历史替代见docs；不改变原验收状态。

## 授权与完成边界

本轮仅迁移说明。历史用户决定、动机、来源、兼容性及未决事项完整保留在下方基线；新的行为决定须明确日期、理由和适用范围，更新本需求及docs目标后独立提交，再进入实现。不得用本次文档完成替代AC5、R07原样本或任何硬件待验。

## 冻结历史基线（68cb508，非现行契约）

以下原文用于保留阶段决定、组顺序、需求历史与真实验证；不继续独立维护算法/布局/源码映射。当前契约先查上方docs条目。旧“本轮”“待执行”“当前”只指原记录时点，不覆盖后续决定。历史任务的全量软件门禁不自动成为以后每次修改的要求。

<details>
<summary>展开原始阶段记录（原文保留）</summary>

# Phase 03: 电磁循迹控制链 — 需求

> **2026-10-16 增量状态**：[Phase 14](../14-dual-mode-tracking/requirements.md) FR-05/06/07/10/11/12/16 增加同级光电偏差与独立方向历史，区分电磁error和实际选中偏差；两传感器始终刷新且任一失线停车。原电磁公式、八读滤波、差速和速度PI保持；具体改动映射与验收见 [Phase 14 计划](../14-dual-mode-tracking/plan.md) / [验证](../14-dual-mode-tracking/validation.md)，下文不作为光电需求替身。 2026-10-16实现状态及V-01～15通过证据见上述Phase14验证页，V-16～20仍待上机。

> **as-built 规约**：以下需求逆向自现有代码，描述系统当前实际行为，均可观测、可检验。

## 功能需求

- FR-1: 系统必须以 2 ms 控制拍（TIM6）对 4 路电感（横左/竖左/竖右/横右）做 8 次读取、去最大最小值后 6 次平均，输出 `Track_GetState()->filtered[0..3]`（0~4095，12 bit 标度）。
- FR-2: 系统必须按加权差分归一化公式计算循迹偏差 `error = 100·(weight_x·横差 + weight_y·竖差) / (weight_xx·横和 + weight_abs·|竖差|)`，存入 `Track_GetState()->error`，输出范围约 ±100；分母为 0 时必须返回 0。
- FR-3: 系统必须同时输出 `Track_GetState()->symmetry_x/symmetry_y`（0~100，越小越对称），供赛道元素判断使用（见 [Phase 04](../04-element-fsm/requirements.md)）。
- FR-4: 方向环必须为位置式 PD + 非线性项：`out = kp_x·e + k2p_x·e·|e| + kd_x·Δe − k2d_x·accel_x`，输出限幅 ±1000。TRACKING 态默认增益 KP_x=2、K2P_x=0.008、KD_x=15、K2D_x=1。
- FR-5: 差速分配必须为非对称：转向内侧轮目标速度变化量为 ∓3/2·out，外侧轮为 ±1·out（叠加在 `base_speed` 上）。
- FR-6: 速度环必须为左右轮独立的位置式 PI（默认 KP_v=20.0、KI_v=0.75）：目标换向时清零积分；积分限幅 ±7000；目标非零时叠加 ±500 死区前馈，目标为 0 时不加前馈（保证停车）；输出限幅 ±7500。
- FR-7: 电机输出必须按符号切换 DIR 方向脚（左轮正转 DIR=高、右轮正转 DIR=低），PWM 占空比对外接口为 0~10000 万分比。
- FR-8: 编码器测速必须取 TIM15 采样器的 2 ms 累计计数，经显式初始化为 α=0.88 的一阶低通后得 `Motor_GetState()->left/right.real_speed`，并累加左右里程与均值 `Motor_GetState()->distance`；`App_Init` 必须调用包含滤波初始化的 `Motor_InitEncoder()`，不得只启动编码器硬件。
- FR-9: 出赛道时必须进入统一安全停车：目标速度清零、速度 PI 积分与输出清零、两路电机 PWM 直接归零并停止 TIM15/TIM6/TIM7；不得依赖速度环自然衰减停车。
- FR-10: 滤波组件必须提供一阶低通（编码器、电池复用）与一阶高通（陀螺仪，截止 0.2 Hz，α=RC/(RC+dt)）两种实现。

## 技术约束

- TC-1: 全部控制系数按 TIM6 = 2 ms 控制拍标定（[技术栈](../../techstack.md) 硬性约束 2）；改控制周期必须重标定全套系数。
- TC-2: App 层只经 `bsp.h` 接触硬件资源（硬性约束 1）；PWM/ADC/编码器的 HAL 细节全部在 BSP。
- TC-3: 占空比对外语义一律 0~10000 万分比，CCR 换算只在 BSP 内发生（硬性约束 4）。
- TC-4: 编码器累计变量依赖 TIM15 与 TIM6 同优先级互不抢占的架构前提（硬性约束 3），`BSP_Sampler_ConsumeEnc*` 免锁。
- TC-5: 算法与源工程 `Sirius20260718` 保持语义一致；Phase 13 将原同名全局量收拢入模块结构体，计算入口与原公式保留以便对照。
- TC-6: 休眠代码（增量式 `PID_UpdateIncrementalLeft/R`、`PID_UpdateAngle`）保留编译但默认不调用；启用前必须按 AGENTS.md 规则与用户确认设计。
- TC-7: Phase 13 在保持上述数值与逐拍语义的前提下，将原可写全局量迁移为模块静态结构体和专用写接口；调用方使用返回 `const` 指针的状态 getter。历史符号在上方公式中仅表示逻辑值，映射与验收见 [Phase 13](../13-control-structure/requirements.md)。

## 接口约定（如适用）

| 接口 | 方向 | 语义 |
|---|---|---|
| `Track_GetState()->filtered[4]` | 只读指针 | 4 路电感滤波值，[0]横左 [1]竖左 [2]竖右 [3]横右 |
| `Track_GetState()->error/symmetry_x/symmetry_y` | 只读指针 | 循迹偏差与横/竖电感对称度 |
| `Track_SetWeights()` | 写接口 | 四个权重默认 15/20/22/10，状态机逐拍覆写（见 Phase 04） |
| `PID_GetState()` / `PID_SetGain()` / `PID_SetTrackGains()` | 读/写接口 | 方向及速度增益；状态机逐拍覆写与双档参数冲突仍留 Phase 10 处理 |
| `Motor_GetState()` / `Motor_SetBaseSpeed()` | 读/写接口 | 基础速度、左右目标与实测速度、里程；基础速度为编码器计数/2 ms 标度 |
| `Motor_DistributeSpeed(pid_out)` | 函数 | 差速分配入口，写入 `Motor_GetState()->left/right.target_speed` |
| `Motor_ResetDistance()` | 写接口 | 清零左右轮和平均里程，供环岛状态转移使用 |
| `BSP_PWM_SetDuty(ch, 0~10000)` | 依赖（BSP） | 占空比万分比，BSP 内换算 CCR |

## 非目标（Non-goals）

- 不做增量式速度 PID 的启用与整定（休眠代码，处置见 [休眠代码清单](../../research/2026-10-02-dormant-code-inventory.md)）。
- 不做摄像头/视觉循迹（[使命](../../mission.md) 范围外）。
- 本阶段不修复已知缺陷（除零保护、复制粘贴、魔数等），整改排期见 [roadmap](../../roadmap.md) Phase 09–11。
- 不负责赛道元素判断逻辑本身（属 [Phase 04](../04-element-fsm/plan.md)）。

> 2026-10-16 Phase 15 G2：现行传感器/控制符号按[完整映射](../15-code-style-consistency/symbol-map.md)迁移，
> 配置、计算顺序及休眠调用边保持；历史 validation 原文及本阶段实车门禁不变。

> 2026-10-16 Phase 15 G3：现行调度、HMI、参数和日志名称已按[符号表](../15-code-style-consistency/symbol-map.md)统一。
> 本文原阶段行为记录与未完成待办不因此重新验收；现行运行链以 Phase 14 和当前头文件契约为准，历史 validation 保留原文。

## Phase 17 当前位置（2026-10-23）

App按system/sensing/control/services/hmi归类。现行文件及逐函数位置见[迁移清单](../17-app-architecture/migration-map.md)。
历史路径、符号与验证结果保留；本说明不表示后续职责拆分或硬件验收完成。

</details>
