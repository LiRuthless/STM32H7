# 运行时、生命周期与安全

现行功能说明，核对基线 `68cb508`；2026-10-31迁移。需求经[功能索引](../traceability.md)进入本页，再由实现表定位源码。
当前行为不等于全部已批准/实机验收，冲突见[已知问题](../known-issues.md)。

<a id="f01"></a>
## F01 初始化与三线调度
### 边界与正常流程

App固定入口委托system；lifecycle持有start/run/key、2ms控制计数、测距和看门狗状态，scheduler只编排。上电先UART→Flash→参数，再ADC/电池滤波/按键/编码器/PWM、LCD、IMU校准、DL1B及采样器配置；此时不启动TIM15。SCB初始化后初始化调参并注册业务handler，避免重复初始化。

| 上下文 | 周期/优先级 | 输入→处理→发布 |
|---|---|---|
| 停车前台 | 循环 | 顶部与停车分支二次检查收尾→通信→双传感/预览→电池→所选显示→旧串口→延迟保存→LED/PC13事件 |
| 运行前台 | 循环 | 最多8条Flash消费→命令业务和链路服务→必要停车收尾→返回，不刷新LCD |
| TIM15 | 1ms / 0 | 读清编码器、IMU原始采样→App采样钩子→完整记录和独立上传复制 |
| TIM6 | 2ms / 0 | 双传感与保护→DL1B/姿态→两FSM观察→选中执行→反馈/PI/PWM→完整拍调参及记录发布→900us检查 |
| TIM7 | 5ms / 1 | 电池更新→持续低压判断→喂狗→运行灯；没有Flash消费 |

TIM15/TIM6同级不抢占；不能把const getter当跨中断原子整包。TIM6计数前1000拍不执行车辆循迹，第1001拍开始；等待期间双采集、保护、姿态和日志仍工作。风扇已可转动。
IWDG首次成功起跑初始化，之后不能关闭；运行TIM7喂狗，停车前台喂狗。约6s为配置估算，不等于实测复位时间。PE3高亮，停车每250ms翻转。

### 实现与验证

`App_Init/App_Loop/App_ControlISR/App_TaskISR/App_SampleISR`是固定薄入口；实际流程在`Lifecycle_Init`和三个`Scheduler_*ISR`/`Scheduler_Loop`。审查次序、停前台二次检查和1000/1001边界；既有g2_system、dual_mode_tracking的App夹具仅作按需宿主证据。AC5、真实并发、700/900us及实车未验。

**需求来源：** [P01 FR-01, FR-04, FR-05, FR-06, FR-09, FR-11, FR-13, FR-14](../../specs/features/01-runtime-architecture/requirements.md)；[P02 FR-26](../../specs/features/02-bsp-platform/requirements.md)；[P12 FR-05](../../specs/features/12-control-safety-closure/requirements.md)；[P17 FR-04](../../specs/features/17-app-architecture/requirements.md)。实现入口：[App/app.c](../../App/app.c)、[App/system/scheduler.c](../../App/system/scheduler.c)、[App/system/lifecycle.c](../../App/system/lifecycle.c)、[BSP/bsp_runtime.c](../../BSP/bsp_runtime.c)。

<a id="f02"></a>
## F02 起停、模式与首因
<a id="f02-start"></a>
### F02.START 起跑与模式切换

PC13短按在稳定释放后起跑，长按达到1000ms触发一次切换；两沿30ms消抖，运行忽略，停车必须先稳定释放再武装。上电默认光电(1)、低档(0)，模式不掉电记忆。
`Lifecycle_TryStart`只在停车且无导出占用时继续，先立即电机清零，再校验全部标定，读一次新鲜双传感并验证；随后刷待保存参数、建立/回读校验日志头，成功才复位控制/电机/PID/IMU/安全历史并发布首快照，最后启动TIM6/TIM7和TIM15、发起跑事件及看门狗。
32项标定仍全为-1，两模式都拒绝起跑。任何保存或日志门禁失败不启用定时链；已发生采样/保存/擦写不回滚。
`Lifecycle_TrySetMode`先编号/停车/保存检查，成功后才写模式、依序复位控制/电机/PID/IMU和应用方向参数；同模式也执行保存和复位。失败返回ARG/BUSY/SAVE_FAILED并保留旧模式，成功OK。

<a id="f02-stop"></a>
### F02.STOP 立即停车与前台收尾

`Safety_RequestStop(NONE)`直接返回。其他原因先映射累计保护位，再在原PRIMASK临界区锁存首因、pending并清运行标志；随后发布摘要。只首次请求取消调参、`Motor_EmergencyStop`清目标/活动速度PI/PWM、风扇写当前空闲值、停止TIM15及TIM6/TIM7、更新观察状态和事件。
`Safety_FinalizeStop`仅前台执行日志尾包、调参停车保存、摘要输出、清pending、要求松键，前后可喂狗。重复请求累计位但不覆盖首因、不重复收尾。

| 请求来源 | 原因 |
|---|---|
| TIM6双传感 | 采集/标定、灰度失线、电感和≤300；当拍采集优先于灰度、灰度优先于电感 |
| TIM6末尾预算 | 周期差达到432000个480MHz周期(900us) |
| TIM7 | 持续低压 |
| 记录与前台存储 | 队满、写错、容量满 |

### 冲突与保证边界

D07：P12要求最先锁首因，当前先累计保护位；两个临界区之间可被抢占，最早调用者不保证先锁存。D09：旧`stop: off track/low battery`精确文本已与当前`stop: reason=... protect=... fault=... accepted/written/rejected/pending`不同；未在本轮批准兼容变更。D03：进入停车会写一次空闲PWM，停车后改参数/切档只改状态，不承诺立即改物理输出。
审查保护先于执行、所有异常出口不再执行编码器/电机、停止采样后才访问LCD。既有App故障注入/g4边界可按需复用；本轮不重验，实车待验。

**需求来源：** [P01 FR-07](../../specs/features/01-runtime-architecture/requirements.md)；[P03 FR-09](../../specs/features/03-track-following/requirements.md)；[P04 FR-02](../../specs/features/04-element-fsm/requirements.md)；[P06 FR-09](../../specs/features/06-hmi-tuning/requirements.md)；[P12 FR-02, FR-03, FR-04, FR-06](../../specs/features/12-control-safety-closure/requirements.md)；[P14 FR-13, FR-16](../../specs/features/14-dual-mode-tracking/requirements.md)；[P15 FR-07](../../specs/features/15-code-style-consistency/requirements.md)。实现入口：[App/system/lifecycle.c](../../App/system/lifecycle.c)、[App/system/safety.c](../../App/system/safety.c)、[App/control/motor.c](../../App/control/motor.c)、[BSP/bsp_key.c](../../BSP/bsp_key.c)。

<a id="f03"></a>
## F03 低压持续判定
### 判定与状态

`Battery_Update`对ADC电池值按当前整数截断和α=0.65低通更新。仅TIM7在采样后调用`Safety_CheckLowVoltage`：滤波值严格小于1220且严格大于300时累加`int16_t s_low_count`，`++count > 200`才返回1，即连续第201个5ms检查，约1.005s；脱离区间清零。`Safety_Reset`在成功起跑前清计数。
因此阈值常量200不能表述为第200拍触发；电压换算只是板上分压估算，真实阈值仍待测。停车前台更新电池用于显示，不自行执行TIM7低压停车。

### 输入输出与验证

输入是滤波ADC码，不是浮点伏特；输出布尔值，调用者请求低压停车。短路求值保留两次getter的读取语义。核对300/1220严格边界、200/201、区间退出及reset；已有g2_system有真实安全函数边界刺激。实车低压和电源故障未验，不由源码判定电气阈值正确。

**需求来源：** [P01 FR-10, FR-12](../../specs/features/01-runtime-architecture/requirements.md)。实现入口：[App/system/safety.c](../../App/system/safety.c)、[App/sensing/battery.c](../../App/sensing/battery.c)。

<a id="f04"></a>
## F04 fatal与底层启停失败
### 当前失败路径，不是新的安全策略

Core的`Error_Handler`关中断并停留；HardFault/MemManage/BusFault/UsageFault为原地循环，NMI也保留等待。`BSP_Runtime_StartTimers/StopTimers`调用HAL但保持原有返回值忽略行为。
当前没有完整错误传播、故障快照或所有fatal统一断PWM机制；不能把死循环解释为已验证安全停车。IWDG是否已经启动、异常优先级和当时外设输出决定实际后果，本轮未测，也没有批准恢复设计。

### 定位与待办

实现：`Core/Src/main.c::Error_Handler`、`Core/Src/stm32h7xx_it.c::*Fault_Handler`、`BSP/bsp_runtime.c`。前台/异常入口不可随意调用阻塞Flash收尾。未来若设计fatal保护，先明确可运行上下文、输出切断和复位行为，再提交所属Phase与本条目标契约。
本轮核对源中循环、关中断及返回值处理；没有故障注入或MCU复位结果。该条补齐现状文档，不表示缺失安全设计已修复。

**来源与边界：** 本条由本轮审计补录平台异常现状；原Phase没有独立的fatal恢复FR，不虚构已批准目标。起停协作见[F02](#f02)，未来设计须先在所属Phase新增明确需求再修改本条。实现入口：[Core/Src/main.c](../../Core/Src/main.c)、[Core/Src/stm32h7xx_it.c](../../Core/Src/stm32h7xx_it.c)、[BSP/bsp_runtime.c](../../BSP/bsp_runtime.c)。

## 本页实现与模块反查

下表是当前源码定位；完整符号表仅在模块主功能页维护，协作行转向主条目。公共/私有辅助函数归所属功能，不把声明视为另一实现。Core生成部分只作边界，用户区按原标记维护。

| 文件 | 主功能 | 定义/配置定位 |
|---|---|---|
| [App/app.c](../../App/app.c) | F01 | `App_Init`、`App_Loop`、`App_ControlISR`、`App_TaskISR`、`App_SampleISR`、`App_RequestStop`、`App_IsStopped` |
| [App/app.h](../../App/app.h) | F01 | 类型/布局/配置；关联同模块C定义 |
| [App/app_config.h](../../App/app_config.h) | F01 | 类型/布局/配置；关联同模块C定义 |
| [App/app_types.h](../../App/app_types.h) | F01 | 类型/布局/配置；关联同模块C定义 |
| [App/control/motor.c](../../App/control/motor.c) | F08 | 协作模块；完整符号表见[F08主条目](../control/tracking-and-controllers.md#f08) |
| [App/sensing/battery.c](../../App/sensing/battery.c) | F03 | `Battery_Init`、`Battery_Update`、`Battery_GetRaw`、`Battery_GetFiltered` |
| [App/sensing/battery.h](../../App/sensing/battery.h) | F03 | 类型/布局/配置；关联同模块C定义 |
| [App/system/lifecycle.c](../../App/system/lifecycle.c) | F02 | `Lifecycle_Init`、`Lifecycle_TryStart`、`Lifecycle_ResetRunState`、`Lifecycle_IsStopped`、`Lifecycle_IsStarted`、`Lifecycle_IsKeyActive`、`Lifecycle_IsRunning`、`Lifecycle_GetControlTicks`、`Lifecycle_GetDistanceMm`、`Lifecycle_IsWatchdogStarted`、`Lifecycle_AdvanceControlTick`、`Lifecycle_SetDistanceMm`、`Lifecycle_MarkRunning`、`Lifecycle_ClearRunFlags`、`Lifecycle_TestSetRunState`、`Lifecycle_TrySetMode` |
| [App/system/lifecycle.h](../../App/system/lifecycle.h) | F02 | 类型/布局/配置；关联同模块C定义 |
| [App/system/safety.c](../../App/system/safety.c) | F02 | `Safety_RequestStop`、`Safety_FinalizeStop`、`Safety_IsStopPending`、`Safety_GetStopReason`、`Safety_Reset`、`Safety_CheckLowVoltage` |
| [App/system/safety.h](../../App/system/safety.h) | F02 | 类型/布局/配置；关联同模块C定义 |
| [App/system/scheduler.c](../../App/system/scheduler.c) | F01 | `Scheduler_Loop`、`Scheduler_ControlISR`、`Scheduler_TaskISR`、`Scheduler_SampleISR` |
| [App/system/scheduler.h](../../App/system/scheduler.h) | F01 | 类型/布局/配置；关联同模块C定义 |
| [BSP/bsp_key.c](../../BSP/bsp_key.c) | F14 | 协作模块；完整符号表见[F14主条目](../hmi/operation.md#f14) |
| [BSP/bsp_runtime.c](../../BSP/bsp_runtime.c) | F29 | 协作模块；完整符号表见[F29主条目](../platform/board-and-bsp.md#f29) |
| [BSP/bsp_wdt.c](../../BSP/bsp_wdt.c) | F01 | `BSP_WDT_Init`、`BSP_WDT_Feed` |
| [BSP/bsp_wdt.h](../../BSP/bsp_wdt.h) | F01 | 类型/布局/配置；关联同模块C定义 |
| [Core/Src/main.c](../../Core/Src/main.c) | F04 | `main`、`SystemClock_Config`、`Error_Handler`、`assert_failed` |
| [Core/Src/stm32h7xx_it.c](../../Core/Src/stm32h7xx_it.c) | F04 | `NMI_Handler`、`HardFault_Handler`、`MemManage_Handler`、`BusFault_Handler`、`UsageFault_Handler`、`SVC_Handler`、`DebugMon_Handler`、`PendSV_Handler`、`SysTick_Handler`、`DMA1_Stream0_IRQHandler`、`USART1_IRQHandler`、`EXTI15_10_IRQHandler`、`TIM6_DAC_IRQHandler`、`TIM7_IRQHandler`、`DMA1_Stream1_IRQHandler`、`DMA1_Stream2_IRQHandler`、`SPI2_IRQHandler` |
