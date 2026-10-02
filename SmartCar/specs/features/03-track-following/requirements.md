# Phase 03: 电磁循迹控制链 — 需求

> **as-built 规约**：以下需求逆向自现有代码，描述系统当前实际行为，均可观测、可检验。

## 功能需求

- FR-1: 系统必须以 2 ms 控制拍（TIM6）对 4 路电感（横左/竖左/竖右/横右）做 8 次读取、去最大最小值后 6 次平均，输出 `Track_GetState()->filtered[0..3]`（0~4095，12 bit 标度）。
- FR-2: 系统必须按加权差分归一化公式计算循迹偏差 `error = 100·(weight_x·横差 + weight_y·竖差) / (weight_xx·横和 + weight_abs·|竖差|)`，存入 `Track_GetState()->error`，输出范围约 ±100；分母为 0 时必须返回 0。
- FR-3: 系统必须同时输出 `Track_GetState()->symmetry_x/symmetry_y`（0~100，越小越对称），供赛道元素判断使用（见 [Phase 04](../04-element-fsm/requirements.md)）。
- FR-4: 方向环必须为位置式 PD + 非线性项：`out = KP_x·e + K2P_x·e·|e| + KD_x·Δe − K2D_x·accel_x`，输出限幅 ±1000。TRACKING 态默认增益 KP_x=2、K2P_x=0.008、KD_x=15、K2D_x=1。
- FR-5: 差速分配必须为非对称：转向内侧轮目标速度变化量为 ∓3/2·out，外侧轮为 ±1·out（叠加在 `base_speed` 上）。
- FR-6: 速度环必须为左右轮独立的位置式 PI（默认 KP_v=20.0、KI_v=0.75）：目标换向时清零积分；积分限幅 ±7000；目标非零时叠加 ±500 死区前馈，目标为 0 时不加前馈（保证停车）；输出限幅 ±7500。
- FR-7: 电机输出必须按符号切换 DIR 方向脚（左轮正转 DIR=高、右轮正转 DIR=低），PWM 占空比对外接口为 0~10000 万分比。
- FR-8: 编码器测速必须取 TIM15 采样器的 2 ms 累计计数，经显式初始化为 α=0.88 的一阶低通后得 `Motor_GetState()->left/right.real_speed`，并累加左右里程与均值 `Motor_GetState()->distance`；`App_Init` 必须调用包含滤波初始化的 `encoder_init()`，不得只启动编码器硬件。
- FR-9: 出赛道时必须进入统一安全停车：目标速度清零、速度 PI 积分与输出清零、两路电机 PWM 直接归零并停止 TIM15/TIM6/TIM7；不得依赖速度环自然衰减停车。
- FR-10: 滤波组件必须提供一阶低通（编码器、电池复用）与一阶高通（陀螺仪，截止 0.2 Hz，α=RC/(RC+dt)）两种实现。

## 技术约束

- TC-1: 全部控制系数按 TIM6 = 2 ms 控制拍标定（[技术栈](../../techstack.md) 硬性约束 2）；改控制周期必须重标定全套系数。
- TC-2: App 层只经 `bsp.h` 接触硬件资源（硬性约束 1）；PWM/ADC/编码器的 HAL 细节全部在 BSP。
- TC-3: 占空比对外语义一律 0~10000 万分比，CCR 换算只在 BSP 内发生（硬性约束 4）。
- TC-4: 编码器累计变量依赖 TIM15 与 TIM6 同优先级互不抢占的架构前提（硬性约束 3），`BSP_Sampler_ConsumeEnc*` 免锁。
- TC-5: 算法与源工程 `Sirius20260718` 保持语义一致；Phase 13 将原同名全局量收拢入模块结构体，计算入口与原公式保留以便对照。
- TC-6: 休眠代码（增量式 `PID_L/R`、`PID_angle`）保留编译但默认不调用；启用前必须按 AGENTS.md 规则与用户确认设计。
- TC-7: Phase 13 在保持上述数值与逐拍语义的前提下，将原可写全局量迁移为模块静态结构体和专用写接口；调用方使用返回 `const` 指针的状态 getter。历史符号在上方公式中仅表示逻辑值，映射与验收见 [Phase 13](../13-control-structure/requirements.md)。

## 接口约定（如适用）

| 接口 | 方向 | 语义 |
|---|---|---|
| `Track_GetState()->filtered[4]` | 只读指针 | 4 路电感滤波值，[0]横左 [1]竖左 [2]竖右 [3]横右 |
| `Track_GetState()->error/symmetry_x/symmetry_y` | 只读指针 | 循迹偏差与横/竖电感对称度 |
| `Track_SetWeights()` | 写接口 | 四个权重默认 15/20/22/10，状态机逐拍覆写（见 Phase 04） |
| `PID_GetState()` / `PID_SetGain()` / `PID_SetTrackGains()` | 读/写接口 | 方向及速度增益；状态机逐拍覆写与双档参数冲突仍留 Phase 10 处理 |
| `Motor_GetState()` / `Motor_SetBaseSpeed()` | 读/写接口 | 基础速度、左右目标与实测速度、里程；基础速度为编码器计数/2 ms 标度 |
| `speed_control(pid_out)` | 函数 | 差速分配入口，写入 `Motor_GetState()->left/right.target_speed` |
| `Motor_ResetDistance()` | 写接口 | 清零左右轮和平均里程，供环岛状态转移使用 |
| `BSP_PWM_SetDuty(ch, 0~10000)` | 依赖（BSP） | 占空比万分比，BSP 内换算 CCR |

## 非目标（Non-goals）

- 不做增量式速度 PID 的启用与整定（休眠代码，处置见 [休眠代码清单](../../research/2026-10-02-dormant-code-inventory.md)）。
- 不做摄像头/视觉循迹（[使命](../../mission.md) 范围外）。
- 本阶段不修复已知缺陷（除零保护、复制粘贴、魔数等），整改排期见 [roadmap](../../roadmap.md) Phase 09–11。
- 不负责赛道元素判断逻辑本身（属 [Phase 04](../04-element-fsm/plan.md)）。
