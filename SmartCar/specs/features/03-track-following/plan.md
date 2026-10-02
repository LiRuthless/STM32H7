# Phase 03: 电磁循迹控制链 — 实现计划

> **as-built 规约**：代码已存在，本文档为存量功能的规约化追认，描述现状（含已知缺陷），作为 Phase 08–11 整改的对照基准。任务分组全部已完成，内容为实现结构追溯而非待办。

## 目标

交付完整的电磁循迹控制链：4 路电感采集与滤波 → 加权差分归一化偏差 → 方向环非线性 PD → 差速分配 → 左右轮位置式 PI 速度闭环 → PWM 输出，全部按 TIM6 = 2 ms 控制拍标定。

## 背景与依据

- 关联宪章：[使命](../../mission.md)（范围内第 1 条：电磁循迹控制链）；[技术栈](../../techstack.md) 硬性约束 1/2/3/4。
- 算法出处：省赛源工程 `Sirius20260718`（STC AI8051U 逐飞库，仓库外，仅按名称引用）：`track_sensor.c` 源 `adc.c`，`pid.c` 源 `pid.c`，`motor.c` 源 `motor.c`，`filter.c` 源工程滤波实现。
- 前置条件：[Phase 01 运行时架构](../01-runtime-architecture/plan.md)（TIM6/TIM15 调度）、[Phase 02 BSP 平台层](../02-bsp-platform/plan.md)（ADC DMA、PWM、编码器、采样器）。

## 任务分组（Task Groups）

### Group 1: 电感采集与偏差计算（`App/track_sensor.c`，源 `adc.c`）

- [x] 4 路电感通道映射：`adc_filted[0]`=横左、`[1]`=竖左、`[2]`=竖右、`[3]`=横右（经 `adc_ch_map[]` 映射到 BSP ADC 索引）。
- [x] `read_adc()`：对 DMA 循环采集的缓存做 8 次连续读取，每路去最大/最小后余 6 次平均（8−2）/6，输出 `adc_filted[4]`。本工程 ADC 由 DMA 连续采集，此处为纯内存读，开销可忽略。
- [x] `get_track_error()` 加权差分归一化：

  ```
  track_error = 100 · (weight_x·(adc0−adc3) + weight_y·(adc1−adc2))
                     / (weight_xx·(adc0+adc3) + weight_abs·|adc1−adc2|)
  ```

  默认权重 weight_x/xx/y/abs = 15/20/22/10，输出范围约 ±100；分母为 0 时返回 0（除零保护）。函数内顺带更新 `symmetry_x`/`symmetry_y`（横/竖电感对称度，0~100 越小越对称）。`symmetry_adc()` 为同公式的独立封装。
- [x] `front_adc_init()`：薄封装 `BSP_ADC_Init()`，保持源工程接口不变。

### Group 2: 方向环与速度环（`App/pid.c`，源 `pid.c`）

- [x] `PID_track()` 位置式方向环：`out = KP_x·e + K2P_x·e·|e| + KD_x·Δe − K2D_x·accel_x`，其中 e·|e| 为非线性二次比例项（小偏差柔和、大偏差快速修正），`K2D_x·accel_x` 为前向加速度阻尼项（设计意图，现状见风险节）；输出限幅 ±`MAX_DIR_OUT` = ±1000。
- [x] TRACKING 态默认方向增益（`control.c` 每拍覆写）：KP_x=2、K2P_x=0.008、KD_x=15、K2D_x=1。
- [x] `PID_L_pos()`/`PID_R_pos()` 位置式速度 PI：默认 KP_v=20.0、KI_v=0.75（源 `main.c`，经 `param.c` 默认表装载）；目标换向清积分；积分限幅 ±(`MAX_SPD_OUT` − 500) 为死区前馈留余量；目标非零时叠加死区前馈 ±`MOTOR_DEAD_ZONE_L/R` = ±500（target=0 不加前馈，保证真正停车）；输出限幅 ±`MAX_SPD_OUT` = ±7500。
- [x] 休眠代码保留：增量式 `PID_L()`/`PID_R()`、角度环 `PID_angle()` 均移植保留但默认不调用（出处与处置意向见 [休眠代码清单](../../research/2026-10-02-dormant-code-inventory.md)）。

### Group 3: 差速分配、测速与执行（`App/motor.c`，源 `motor.c`）

- [x] `speed_control(pid_out)` 非对称差速：pid_out ≥ 0 时左轮 `base_speed − 3·out/2`（内侧减速）、右轮 `base_speed + out`（外侧加速）；pid_out < 0 时左右对称互换（内侧 ∓3/2·out、外侧 ±1·out）。
- [x] `encoder_init()`：启动两路编码器并用 α=0.88 显式初始化 `filt_encoder_L/R`；`App_Init()` 调用该入口，避免滤波器静态零初始化导致速度反馈恒为 0。`read_encoder()`（2 ms 控制拍内调）：取 TIM15 采样器的 2 ms 累计计数 → 一阶低通 → `real_speed_L/R`，同时累加左右里程与均值 `Distance`。
- [x] `motor_control()`：调 `PID_L_pos/R_pos`，按输出符号设置 DIR 脚（左轮正转 DIR=高、右轮正转 DIR=低），PWM 占空比对外一律 0~10000 万分比（硬性约束 4），CCR 换算只在 BSP 内发生。

### Group 4: 滤波器（`App/filter.c`）

- [x] 一阶低通 `lowpass_init/update`：`y[n] = α·x[n] + (1−α)·y[n−1]`；实例：编码器 ×2（α=0.88，`motor.c`）、电池（α=0.65，`app.c`）。
- [x] 一阶高通 `gyro_hpf_init/update`：`α = RC/(RC+dt)`，`y[n] = α·(y[n−1] + x[n] − x[n−1])`；截止 0.2 Hz、dt = 2 ms（`imu_proc_init` 内初始化），实例 `gyro_hpf_x/y`（使用现状见 [Phase 05](../05-imu-attitude/plan.md)）。

## 实现顺序与依赖

```
BSP_ADC/PWM/Encoder/Sampler (Phase 02)
        │
        ▼
read_adc → get_track_error → PID_track → speed_control → target_speed_L/R
        │                                                │
        └── symmetry_x/y（供 Phase 04 元素判断）           ▼
                                   read_encoder → PID_L_pos/R_pos → motor_control → BSP_PWM
```

调用链入口：`App_ControlISR`（TIM6 2 ms）→ `whole_test()`（[Phase 04](../04-element-fsm/plan.md) 主状态机内调用本链）→ `read_encoder()` → `motor_control()`。控制拍标定 2 ms（硬性约束 2），改 TIM6 周期必须重标定全部系数。

## 风险与取舍（as-built 已知问题，如实记录）

| 问题 | 现状 | 整改指向 |
|---|---|---|
| 差速 3/2 系数为魔数 | `speed_control` 内联硬编码，无命名常量，不可调 | Phase 10（增益表驱动化） |
| `PID_L_pos`/`PID_R_pos` 大段复制粘贴 | 两函数仅变量名不同，维护需双改 | Phase 11 |
| `= 0.0` 赋整型等风格问题 | `pid.c` 多处 `int32_t error = 0.0;`、`int16_t PID_out = 0.0;` | Phase 11 |
| **`accel_x` 前馈项实际恒为 0** | `accel_x` 仅由 `read_accel_velocity()` 写入，而该函数默认无人调用（见 [Phase 05](../05-imu-attitude/plan.md)），`K2D_x·accel_x` 项虽接线但运行时不产生作用 | Phase 09/10（接入或裁剪前须与用户确认设计） |
| `symmetry_x/y` 计算无除零保护 | 电感和为 0 时除零（`track_error` 分母有保护，对称度没有） | Phase 11 |
| 权重行内注释漂移 | `weight_y = 22; //28` 等注释残留历史值 | Phase 11（注释漂移修正） |
| 休眠代码（增量式 PID、PID_angle） | 保留在镜像中，占少量 Flash | Phase 11（按休眠代码清单处置） |
