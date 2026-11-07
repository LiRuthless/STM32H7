# Phase 03: 电磁循迹控制链 — 实现计划

> **as-built 规约**：代码已存在，本文档为存量功能的规约化追认，描述现状（含已知缺陷），作为 Phase 08–11 整改的对照基准。任务分组全部已完成，内容为实现结构追溯而非待办。

## 目标

> 本文件 Phase 03 的符号描述是移植落地时的历史记录；Phase 13 已将原全局量映射为模块结构体与只读 getter、专用写入口，并以等价性对照验证。当前接口以本阶段 `requirements.md` 接口表及 [Phase 13](../13-control-structure/plan.md) 为准。

交付完整的电磁循迹控制链：4 路电感采集与滤波 → 加权差分归一化偏差 → 方向环非线性 PD → 差速分配 → 左右轮位置式 PI 速度闭环 → PWM 输出，全部按 TIM6 = 2 ms 控制拍标定。

## 背景与依据

- 关联宪章：[使命](../../mission.md)（范围内第 1 条：电磁循迹控制链）；[技术栈](../../techstack.md) 硬性约束 1/2/3/4。
- 算法出处：省赛源工程 `Sirius20260718`（STC AI8051U 逐飞库，仓库外，仅按名称引用）：`track_sensor.c` 源 `adc.c`，`pid.c` 源 `pid.c`，`motor.c` 源 `motor.c`，`filter.c` 源工程滤波实现。
- 前置条件：[Phase 01 运行时架构](../01-runtime-architecture/plan.md)（TIM6/TIM15 调度）、[Phase 02 BSP 平台层](../02-bsp-platform/plan.md)（ADC DMA、PWM、编码器、采样器）。

## 任务分组（Task Groups）

### Group 1: 电感采集与偏差计算（`App/track_sensor.c`，源 `adc.c`）

- [x] 4 路电感通道映射：`adc_filted[0]`=横左、`[1]`=竖左、`[2]`=竖右、`[3]`=横右（经 `adc_ch_map[]` 映射到 BSP ADC 索引）。
- [x] `Track_ReadAdc()`：对 DMA 循环采集的缓存做 8 次连续读取，每路去最大/最小后余 6 次平均（8−2）/6，输出 `adc_filted[4]`。本工程 ADC 由 DMA 连续采集，此处为纯内存读，开销可忽略。
- [x] `Track_UpdateError()` 加权差分归一化：

  ```
  track_error = 100 · (weight_x·(adc0−adc3) + weight_y·(adc1−adc2))
                     / (weight_xx·(adc0+adc3) + weight_abs·|adc1−adc2|)
  ```

  默认权重 weight_x/xx/y/abs = 15/20/22/10，输出范围约 ±100；分母为 0 时返回 0（除零保护）。函数内顺带更新 `symmetry_x`/`symmetry_y`（横/竖电感对称度，0~100 越小越对称）。`Track_UpdateSymmetry()` 为同公式的独立封装。
- [x] `Track_InitAdc()`：薄封装 `BSP_ADC_Init()`，保持源工程接口不变。

### Group 2: 方向环与速度环（`App/pid.c`，源 `pid.c`）

- [x] `PID_UpdateTrack()` 位置式方向环：`out = kp_x·e + k2p_x·e·|e| + kd_x·Δe − k2d_x·accel_x`，其中 e·|e| 为非线性二次比例项（小偏差柔和、大偏差快速修正），`k2d_x·accel_x` 为前向加速度阻尼项（设计意图，现状见风险节）；输出限幅 ±`PID_MAX_DIR_OUT` = ±1000。
- [x] TRACKING 态默认方向增益（`control.c` 每拍覆写）：KP_x=2、K2P_x=0.008、KD_x=15、K2D_x=1。
- [x] `PID_UpdateSpeedLeft()`/`PID_UpdateSpeedRight()` 位置式速度 PI：默认 KP_v=20.0、KI_v=0.75（源 `main.c`，经 `param.c` 默认表装载）；目标换向清积分；积分限幅 ±(`PID_MAX_SPD_OUT` − 500) 为死区前馈留余量；目标非零时叠加死区前馈 ±`PID_MOTOR_DEAD_ZONE_LEFT/R` = ±500（target=0 不加前馈，保证真正停车）；输出限幅 ±`PID_MAX_SPD_OUT` = ±7500。
- [x] 休眠代码保留：增量式 `PID_UpdateIncrementalLeft()`/`PID_UpdateIncrementalRight()`、角度环 `PID_UpdateAngle()` 均移植保留但默认不调用（出处与处置意向见 [休眠代码清单](../../research/2026-10-02-dormant-code-inventory.md)）。

### Group 3: 差速分配、测速与执行（`App/motor.c`，源 `motor.c`）

- [x] `Motor_DistributeSpeed(pid_out)` 非对称差速：pid_out ≥ 0 时左轮 `base_speed − 3·out/2`（内侧减速）、右轮 `base_speed + out`（外侧加速）；pid_out < 0 时左右对称互换（内侧 ∓3/2·out、外侧 ±1·out）。
- [x] `Motor_InitEncoder()`：启动两路编码器并用 α=0.88 显式初始化 `filt_encoder_L/R`；`App_Init()` 调用该入口，避免滤波器静态零初始化导致速度反馈恒为 0。`Motor_ReadEncoder()`（2 ms 控制拍内调）：取 TIM15 采样器的 2 ms 累计计数 → 一阶低通 → `real_speed_L/R`，同时累加左右里程与均值 `Distance`。
- [x] `Motor_UpdateControl()`：调 `PID_UpdateSpeedLeft/R_pos`，按输出符号设置 DIR 脚（左轮正转 DIR=高、右轮正转 DIR=低），PWM 占空比对外一律 0~10000 万分比（硬性约束 4），CCR 换算只在 BSP 内发生。

### Group 4: 滤波器（`App/filter.c`）

- [x] 一阶低通 `Filter_InitLowpass/update`：`y[n] = α·x[n] + (1−α)·y[n−1]`；实例：编码器 ×2（α=0.88，`motor.c`）、电池（α=0.65，`app.c`）。
- [x] 一阶高通 `Filter_InitHighpass/update`：`α = RC/(RC+dt)`，`y[n] = α·(y[n−1] + x[n] − x[n−1])`；截止 0.2 Hz、dt = 2 ms（`IMU_Init` 内初始化），实例 `gyro_hpf_x/y`（使用现状见 [Phase 05](../05-imu-attitude/plan.md)）。

## 实现顺序与依赖

```
BSP_ADC/PWM/Encoder/Sampler (Phase 02)
        │
        ▼
Track_ReadAdc → Track_UpdateError → PID_UpdateTrack → Motor_DistributeSpeed → target_speed_L/R
        │                                                │
        └── symmetry_x/y（供 Phase 04 元素判断）           ▼
                                   Motor_ReadEncoder → PID_UpdateSpeedLeft/R_pos → Motor_UpdateControl → BSP_PWM
```

调用链入口：`App_ControlISR`（TIM6 2 ms）→ `Control_UpdateLegacy()`（[Phase 04](../04-element-fsm/plan.md) 主状态机内调用本链）→ `Motor_ReadEncoder()` → `Motor_UpdateControl()`。控制拍标定 2 ms（硬性约束 2），改 TIM6 周期必须重标定全部系数。

## 风险与取舍（as-built 已知问题，如实记录）

| 问题 | 现状 | 整改指向 |
|---|---|---|
| 差速 3/2 系数为魔数 | `Motor_DistributeSpeed` 内联硬编码，无命名常量，不可调 | Phase 10（增益表驱动化） |
| `PID_UpdateSpeedLeft`/`PID_UpdateSpeedRight` 大段复制粘贴 | 两函数仅变量名不同，维护需双改 | Phase 11 |
| `= 0.0` 赋整型等风格问题 | `pid.c` 多处 `int32_t error = 0.0;`、`int16_t PID_out = 0.0;` | Phase 11 |
| **`accel_x` 前馈项实际恒为 0** | `accel_x` 仅由 `IMU_UpdateAccelVelocity()` 写入，而该函数默认无人调用（见 [Phase 05](../05-imu-attitude/plan.md)），`k2d_x·accel_x` 项虽接线但运行时不产生作用 | Phase 09/10（接入或裁剪前须与用户确认设计） |
| `symmetry_x/y` 计算无除零保护 | 电感和为 0 时除零（`track_error` 分母有保护，对称度没有） | Phase 11 |
| 权重行内注释漂移 | `weight_y = 22; //28` 等注释残留历史值 | Phase 11（注释漂移修正） |
| 休眠代码（增量式 PID、PID_angle） | 保留在镜像中，占少量 Flash | Phase 11（按休眠代码清单处置） |

> 2026-10-16 Phase 15 G2：现行传感器/控制符号按[完整映射](../15-code-style-consistency/symbol-map.md)迁移，
> 配置、计算顺序及休眠调用边保持；历史 validation 原文及本阶段实车门禁不变。

> 2026-10-16 Phase 15 G3：现行调度、HMI、参数和日志名称已按[符号表](../15-code-style-consistency/symbol-map.md)统一。
> 本文原阶段行为记录与未完成待办不因此重新验收；现行运行链以 Phase 14 和当前头文件契约为准，历史 validation 保留原文。

## Phase 17 当前实现定位（2026-10-24）

电感处理归App/sensing/track_sensor；方向显式计算归control/direction_controller，独立左右PI归speed_controller，角度归angle_controller，pid仅持增益/输出及薄协调，motor仍持目标/反馈/执行。PID速度/角度入口接受显式输入，当前调用方已同步；历史Phase11 git show路径和数值期待保持。

本节是现行归属导航；上文历史计划、验证、提交路径及未完成实机项目保持。实际重构验证见
[Phase17验收](../17-app-architecture/validation.md)，逐符号迁移见[迁移导航](../17-app-architecture/migration-map.md)。

> 2026-11-07 文档纠偏：本页恢复本阶段完整正文（恢复依据 `68cb508`），原路径、符号、决定和结果按当时基线解释。当前功能见 [Docs](../../../Docs/index.md)，后续演进见 [路线图](../../roadmap.md)。历史验证安排不自动成为后续每次改动的门禁；本次未重新验收固件或实机。


## 现行功能导航

- [P03功能索引](../../../Docs/traceability.md#p03)
- [F05 电感偏差与对称度](../../../Docs/sensing/sensors-and-calibration.md#f05)
- [F08 方向角度速度与执行](../../../Docs/control/tracking-and-controllers.md#f08)
- [F26 ADC编码器PWM](../../../Docs/platform/board-and-bsp.md#f26)
- [F02 起停、模式与首因](../../../Docs/system/runtime-and-safety.md#f02)
- [F07 IMU校准滤波与积分](../../../Docs/sensing/sensors-and-calibration.md#f07)
