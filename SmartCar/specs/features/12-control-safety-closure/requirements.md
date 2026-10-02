# Phase 12: 控制闭环与安全停车修复 — 需求

## 背景与现状证据

本阶段处理 2026-10-02 代码审查发现、且必须先于存储链与赛道元素演进修复的安全问题：

1. `App_Init()` 只调用 `BSP_Encoder_Init()`，没有调用 `encoder_init()`；`filt_encoder_L/R` 因静态零初始化而保持 `alpha=0`，`lowpass_update()` 输出恒为初值 0，速度 PI 无有效反馈。
2. 出赛道与低压保护只清 `key_flag/Start_flag/Run_flag`，没有直接清电机 PWM，也没有清速度 PI 积分。
3. TIM6/TIM7/TIM15 启动后没有停车路径；停车态恢复 LCD 刷新时，TIM15 仍通过同一个 SPI4 访问 IMU，破坏“LCD 与 IMU 分时使用 SPI4”的架构前提。
4. IWDG 在首次起跑后不可关闭；若停车时停止 TIM7，必须改由主循环继续喂狗，否则约 6 s 后复位。

## 用户决策记录

- 日期：2026-10-02。
- 用户确认的停车语义：触发出赛道或低压后，立即将两路电机 PWM 置 0、清空速度 PI 积分、停止 TIM6/TIM7/TIM15、停止日志并刷出剩余记录、风扇回空闲档；主循环恢复 LCD/串口；再次按启动键时重新清状态并启动三路定时器。
- 实现解释：ISR 内只执行有确定上界的立即安全动作；可能阻塞的日志尾包刷写放到主循环完成。该拆分不改变用户确认的最终停车结果，避免在优先级 0/1 ISR 内新增 Flash 阻塞操作。

## 功能需求

### FR-1 编码器速度滤波器必须初始化

- `App_Init()` 必须通过 App 层编码器初始化入口完成两路编码器硬件启动和滤波器初始化。
- `filt_encoder_L` 与 `filt_encoder_R` 的 `alpha` 必须显式设为现有标定值 `0.88f`，初始输出必须为 `0.0f`。
- 初始化完成后，非零编码器输入必须能够产生非零 `real_speed_L/R`；不得依赖 C 静态零初始化形成隐式配置。
- 现有 2 ms 速度语义、编码器方向宏和滤波公式不变。

### FR-2 停车请求必须立即切断驱动输出

- 新增统一的 `App_RequestStop(reason)`，允许从 TIM6/TIM7 ISR 调用；`reason` 至少区分 `APP_STOP_OFF_TRACK` 与 `APP_STOP_LOW_BATTERY`。
- 第一次停车请求必须按以下顺序完成有界操作：
  1. 将 `Start_flag`、`Run_flag`、`key_flag` 清 0；
  2. 将左右目标速度清 0；
  3. 清空左右速度 PI 的积分和输出状态；
  4. 将左右电机 PWM 直接写为 0，不得再经过速度 PI 计算；
  5. 将风扇占空比设为 `FAN_DUTY_IDLE`；
  6. 停止 TIM15，再停止 TIM6/TIM7 中断调度；
  7. 置位主循环待收尾标志并保存首次停车原因。
- 同一次停车过程中的重复请求不得覆盖首次原因，也不得重复执行日志收尾。
- 若 TIM6 的 `whole_test()` 内发出停车请求，`App_ControlISR()` 返回前不得再次执行 `read_encoder()` 或 `motor_control()`。

### FR-3 两条既有保护路径必须统一进入停车请求

- `whole_test()` 的电感总和不大于 300 分支必须调用 `App_RequestStop(APP_STOP_OFF_TRACK)`，不得自行零散修改三个运行标志。
- `App_TaskISR()` 的低压持续计数超过 `BATTERY_LOW_COUNT` 分支必须调用 `App_RequestStop(APP_STOP_LOW_BATTERY)`。
- 阈值 `300`、`BATTERY_LOW_THRESHOLD=1220`、`BATTERY_MIN_VALID=300`、`BATTERY_LOW_COUNT=200` 与低压滤波算法不在本阶段修改。

### FR-4 阻塞式停车收尾必须在主循环完成

- 主循环检测到待收尾标志后，必须先确认 TIM15 已停止，再调用 `Datalog_Stop()`；`Datalog_Stop()` 负责将不足一页的剩余记录刷写并将日志置为非活动状态。
- 日志收尾完成后才允许刷新 LCD，保证停车态不存在 TIM15 IMU 访问与 LCD SPI4 访问交错。
- 串口必须输出一次且仅一次停车原因：出赛道输出 `stop: off track\r\n`，低压输出 `stop: low battery\r\n`。
- Flash/日志失败不得阻止电机停车、LCD 与串口恢复；存储错误处理的完整可观测性仍由 Phase 08 负责。

### FR-5 IWDG 在停车态必须继续被喂

- 新增“看门狗已经启动”的 RAM 状态；首次起跑只初始化一次 IWDG，后续再次起跑不得依赖重新初始化。
- IWDG 启动后，运行态仍由 TIM7 每 5 ms 喂狗；停车态 TIM7 已停止时，由 `App_Loop()` 持续喂狗。
- 未启动 IWDG 的首次停车等待阶段不得假定看门狗已启用。

### FR-6 再次起跑必须从确定状态开始

- 每次有效启动键按下后，在启动定时器前必须重置：
  - `time`、`low_power_num`；
  - 左右目标速度、实际速度、编码器滤波历史、左右里程与总里程；
  - 方向 PID 历史、左右速度 PI 积分/输出、角度 PID 历史；
  - `kernel_state=KERNEL_TRACKING`、`cask_flag=0`、`track_out=0`；
  - `roundabout_state=STATE_NORMAL`、左右环岛标志清 0、`sign_round=1`；
  - IMU 运行积分量 `angle_x/y/z`、`velocity_x/y/z` 及梯形积分历史。
- 不重新执行上电陀螺零偏校准，不修改已经加载的用户参数与控制增益。
- 重置完成后按 TIM6、TIM7、TIM15 的既有启动流程启动；TIM6 仍保留 `RUN_DELAY_COUNT=1000` 的 2 s 起跑延时。

### FR-7 SPI4 分时不变量必须恢复

- TIM15 运行时，主循环不得刷新 LCD；停车收尾完成且 TIM15 已停止后，主循环才可刷新 LCD。
- 再次起跑前不得存在尚未完成的 LCD SPI4 事务；当前主循环串行执行模型满足此条件，不新增并发显示任务。
- TIM15 与 TIM6 的 NVIC 优先级仍为 0，TIM7 仍为 1，本阶段不得修改优先级。

## 接口与代码映射

| 需求 | 文件 | 符号/修改点 |
|---|---|---|
| FR-1 | `App/app.c`, `App/motor.c`, `App/motor.h` | `App_Init`, `encoder_init`, `filt_encoder_L/R` |
| FR-2 | `App/app.c`, `App/app.h`, `App/motor.c`, `App/motor.h`, `App/pid.c`, `App/pid.h` | `app_stop_reason_t`, `App_RequestStop`, `Motor_EmergencyStop`, `PID_ResetSpeed` |
| FR-3 | `App/control.c`, `App/app.c` | `whole_test`, `App_TaskISR` |
| FR-4 | `App/app.c`, `App/datalog.c` | `App_FinalizeStop`（文件内私有）, `Datalog_Stop` |
| FR-5 | `App/app.c` | `s_wdt_started`, `App_Loop`, 启动路径、`App_TaskISR` |
| FR-6 | `App/app.c`, `App/control.c/.h`, `App/roundabout.c/.h`, `App/imu_proc.c/.h`, `App/motor.c/.h`, `App/pid.c/.h` | 各模块 `*_ResetRunState` / `PID_ResetAll` |
| FR-7 | `App/app.c`, `BSP/bsp_sampler.c` | 停车顺序、主循环显示入口、`BSP_Sampler_Stop` |

## 技术约束

- TC-1: ISR 立即停车路径不得进行 Flash 擦写、页编程、UART 阻塞发送、LCD 刷新或动态内存操作。
- TC-2: 不改变 TIM6=2 ms、TIM7=5 ms、TIM15=1 ms 周期和 NVIC 优先级。
- TC-3: 不改变任何 PID 数值、循迹阈值、低压阈值、风扇占空比常量或赛道元素判定条件。
- TC-4: 本阶段只修复控制反馈初始化、停车和再次起跑生命周期；不接通 W25Q64、不改变参数存储格式、不启用新赛道元素。
- TC-5: GCC 构建必须保持 `-Wall` 下 0 警告 0 错误；Keil 工程文件中既有 App 文件无需新增源文件。

## 非目标

- 不处理 W25Q64 初始化、4 KB 栈缓冲、Flash 超时和日志跨扇区实时性；这些仍属于 Phase 08/11，但其实施依赖在 roadmap 中重新排序。
- 不决定 `fan_duty` 参数链取舍，不改变串口调参持久化策略。
- 不改变 ADC 滤波算法、串口组帧方式、App/BSP 分层范围。
- 不接入左环岛、跷跷板或路障。
