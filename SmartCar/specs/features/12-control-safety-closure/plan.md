# Phase 12: 控制闭环与安全停车修复 — 实现计划

## 目标

在不改变控制参数、控制周期和赛道算法的前提下，恢复有效编码器速度反馈，建立唯一、可重复调用的安全停车路径，并保证停车后 LCD 与 IMU 不再交错访问 SPI4。实现必须逐条满足 [requirements.md](requirements.md)。

## Group 1：速度反馈初始化与可复位 PID 状态

### 修改文件与符号

- `App/app.c::App_Init`
  - 用 `Motor_InitEncoder()` 取代直接调用 `BSP_Encoder_Init()`，确保硬件和两路低通滤波器同时初始化。
- `App/motor.c/.h`
  - 保留 `Motor_InitEncoder()` 为唯一 App 层编码器初始化入口。
  - 新增 `Motor_ResetRunState()`：清目标/实际速度、滤波历史和三组里程，但不重新启动硬件。
  - 新增 `Motor_EmergencyStop()`：清目标速度、调用速度 PID 复位、直接将两路电机 PWM 设为 0。
- `App/pid.c/.h`
  - 将位置式速度 PI 的函数内静态积分量和上次目标移为文件内静态状态，保持算法逐拍结果不变。
  - 新增 `PID_ResetSpeed()` 与 `PID_ResetAll()`，分别支持立即停车和完整再次起跑复位。

### 等价性要求

- 未发生停车/重启时，PID 公式、限幅、死区前馈和参数值逐拍不变。
- 首次启动时滤波器从 0 输出开始，`alpha=0.88f`；这是对当前遗漏初始化的缺陷修复，不保留错误的恒零反馈。

## Group 2：统一安全停车请求

### 修改文件与符号

- `App/app.h`
  - 定义 `app_stop_reason_t`：`APP_STOP_NONE=0`、`APP_STOP_OFF_TRACK=1`、`APP_STOP_LOW_BATTERY=2`。
  - 声明 `App_RequestStop(app_stop_reason_t reason)`。
- `App/app.c`
  - 增加 `volatile` 待收尾标志和停车原因。
  - `App_RequestStop` 只执行 FR-2 的有界动作；首先切断 PWM，随后停止采样器和 TIM6/TIM7。
  - `App_ControlISR` 在 `Control_UpdateLegacy()` 返回后复查 `g_app_start_flag`，若已经停车则立即返回。
- `App/control.c`
  - 出赛道分支改为调用统一停车入口。
- `App/app.c::App_TaskISR`
  - 低压计数达阈值后调用统一停车入口。

### 中断约束

- 停车请求不打印串口、不刷 Flash、不刷新 LCD。
- 停止当前正在执行的定时器中断只影响后续更新事件；当前 ISR 通过显式返回结束。

## Group 3：主循环停车收尾与 IWDG 生命周期

### 修改文件与符号

- `App/app.c`
  - 新增私有 `App_FinalizeStop()`：调用 `Datalog_Stop()`，输出一次停车原因，清待收尾标志。
  - `App_Loop()` 每轮先处理停车收尾；收尾完成后才进入停车态 ADC/LCD/串口逻辑。
  - 新增 `s_wdt_started`。首次启动后置 1；停车态若为 1，则主循环喂狗；再次启动只喂狗而不重复初始化。

### 顺序

1. ISR 立即停车并停三路定时器。
2. 调度返回主循环。
3. 主循环喂狗一次，执行日志尾包刷写，再喂狗一次。
4. 输出停车原因。
5. 恢复仪表盘、串口调参和启动键轮询。

## Group 4：再次起跑状态复位

### 修改文件与符号

- `App/control.c/.h::Control_ResetRunState`
- `App/roundabout.c/.h::Roundabout_ResetRunState`
- `App/imu_proc.c/.h::IMU_ResetRunState`
- `App/motor.c/.h::Motor_ResetRunState`
- `App/pid.c/.h::PID_ResetAll`
- `App/app.c` 启动键路径

### 启动顺序

1. `Datalog_Start()`（保持现有位置；失败处理仍归 Phase 08）。
2. 调用所有运行状态复位接口。
3. 清 `g_app_control_ticks`、`g_app_low_power_count`，设置 `g_app_key_flag=1`、`g_app_start_flag=1`、`g_app_run_flag=0`。
4. LED 设为运行常亮、风扇维持既有启动行为。
5. 启动 TIM6、TIM7、TIM15。
6. 首次运行初始化 IWDG；后续运行只喂狗。

## Group 5：文档同步与验证

- 执行 [validation.md](validation.md) 自动化检查与构建。
- 上机项未实际执行时必须明确标记“待上机”，不得写成通过。
- 更新 `README.md` 运行时与停车说明、Phase 01/03 的相关验证描述、`CHANGELOG.md` 和 `roadmap.md` 状态。

## 风险与回退

- 风险：在 ISR 内停止 TIM6/TIM7 后，若主循环不能继续运行，日志尾包不会刷出；电机 PWM 已先清零，因此安全停车不依赖收尾成功。
- 风险：IWDG 已启用后主循环若因 LCD/Flash 永久阻塞仍会复位；这是既有失控恢复机制，不在本阶段关闭。
- 回退条件：若构建失败、正常运行路径参数发生变化、或静态检查发现停车后仍可能调用 `Motor_UpdateControl()`，不得进入上机验证。

> 2026-10-16 Phase 15 G2：现行传感器/控制符号按[完整映射](../15-code-style-consistency/symbol-map.md)迁移，
> 配置、计算顺序及休眠调用边保持；历史 validation 原文及本阶段实车门禁不变。

> 2026-10-16 Phase 15 G3：现行调度、HMI、参数和日志名称已按[符号表](../15-code-style-consistency/symbol-map.md)统一。
> 本文原阶段行为记录与未完成待办不因此重新验收；现行运行链以 Phase 14 和当前头文件契约为准，历史 validation 保留原文。

## Phase 17 当前实现定位（2026-10-24）

App_RequestStop/App_IsStopped外部业务入口保留在App/app.c，实际首因、pending、低压和停车收尾归system/safety，运行标志归lifecycle，电池数值和滤波归sensing/battery。立即切输出/停三线与前台Flash尾包继续分离，首次原因与累计保护不合并，模式保存/reset协调由Lifecycle_TrySetMode执行。原实车未验项保持。

本节是现行归属导航；上文历史计划、验证、提交路径及未完成实机项目保持。实际重构验证见
[Phase17验收](../17-app-architecture/validation.md)，逐符号迁移见[迁移导航](../17-app-architecture/migration-map.md)。

> 2026-11-07 文档纠偏：本页恢复本阶段完整正文（恢复依据 `68cb508`），原路径、符号、决定和结果按当时基线解释。当前功能见 [Docs](../../../Docs/index.md)，后续演进见 [路线图](../../roadmap.md)。历史验证安排不自动成为后续每次改动的门禁；本次未重新验收固件或实机。


## 现行功能导航

- [P12功能索引](../../../Docs/traceability.md#p12)
- [F08 方向角度速度与执行](../../../Docs/control/tracking-and-controllers.md#f08)
- [F02 起停、模式与首因](../../../Docs/system/runtime-and-safety.md#f02-stop)
- [F20 队列前台写入与故障](../../../Docs/services/recording-and-export.md#f20)
- [F18 完整拍临时调参](../../../Docs/services/parameters-and-tuning.md#f18)
- [F01 初始化与三线调度](../../../Docs/system/runtime-and-safety.md#f01)
- [F02 起停、模式与首因](../../../Docs/system/runtime-and-safety.md#f02-start)
- [F07 IMU校准滤波与积分](../../../Docs/sensing/sensors-and-calibration.md#f07)
- [F11 双FSM与唯一执行](../../../Docs/control/tracking-and-controllers.md#f11)
- [F29 时钟DMA采样与驱动](../../../Docs/platform/board-and-bsp.md#f29)
- [F14 菜单按键与显示](../../../Docs/hmi/operation.md#f14)
