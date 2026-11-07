# Phase 01: 运行时架构与调度 — 实现计划

> **as-built 规约**：代码已存在，本文档为存量功能的规约化追认，描述现状（含已知缺陷），作为 Phase 08–11 整改的对照基准。任务分组全部已完成，内容为实现结构追溯而非待办。

## 目标

把现有固件的应用总装与调度骨架（初始化链、三线中断契约、主循环停车态、时钟树、保护机制）规约化，固定"谁在什么节拍上做什么"这一全工程最高层契约。

## 背景与依据

- 关联宪章：[mission.md](../../mission.md)（电磁循迹控制链与人机交互依赖本调度骨架）；[techstack.md](../../techstack.md) 硬性约束 2（TIM6 = 2ms 控制拍）、3（TIM15/TIM6 同占优先级 0）、5（CubeMX 再生保护 SPI45SEL 补丁）、6（D-Cache 保持关闭）。
- 前置条件：无（全工程第一个阶段，所有后续 Phase 都建立在本调度骨架之上）。
- 算法出处：源工程 `Sirius20260718`（STC AI8051U 逐飞库）的 `user\main.c`（All_init + main 循环）、`user\isr.c`（pit_track / 辅助中断）、`code\config.c`（全局变量），由 `App/app.c` 吸收合并。
- 下游依赖：Phase 02 [BSP 平台层](../02-bsp-platform/requirements.md)（被调度对象）、Phase 03–07（循迹/元素/IMU/HMI/数据记录全部挂在 TIM6/TIM7/TIM15/主循环四个执行上下文上）。

## 任务分组（实现结构追溯，全部完成）

### Group 1: 入口与初始化链
- [x] `Core/Src/main.c`：CubeMX 骨架，`main()` 仅在 USER CODE 区调用 `App_Init()` / `App_Loop()`；USER CODE SysInit 段含 SPI45SEL→PLL3Q 补丁（`HAL_RCCEx_PeriphCLKConfig` 实测不生效，直接 `MODIFY_REG(RCC->D2CCIP1R, ...)`，techstack 硬性约束 5）。
- [x] `App/app.c::App_Init()`：BSP_UART_Init → Param_Load → BSP_ADC_Init → 电池低通 `Filter_InitLowpass(&g_app_battery_filter, 0.65f)` → BSP_Key_Init → `Motor_InitEncoder`（启动编码器并显式初始化两路 α=0.88 低通）→ BSP_PWM_Init + 风扇 IDLE 占空比（1100）→ BSP_LCD_Init/Clear/背光 100% → BSP_IMU660RB_Init（**失败仅串口提示 "IMU660RB init fail"，不卡死**）→ `IMU_Init()`（陀螺高通滤波器初始化，内部重复调一次 BSP_IMU660RB_Init）→ `IMU_CalibrateGyro()`（静止采样 100 次取零偏，源工程被注释、本工程恢复）→ BSP_DL1B_Init（返回值未检查）→ BSP_Sampler_Init（**只配置 TIM15 不启动**）→ Dashboard_Init（`LCD_TARGET_EXTERNAL=0` 时）。
- [x] SPI45SEL 补丁在 `App_Init()` 开头再写一次：`MX_SPI4_Init` 等会按 .ioc 复位 D2CCIP1R，需在全部 MX_*_Init 之后重写。

### Group 2: 三线中断契约
- [x] `BSP/bsp_sampler.c::TIM15_IRQHandler`（1ms，NVIC 优先级 (0,0)，纯寄存器）：清标志 → 双编码器读清（1ms 窗口值 + 2ms 累计值）→ SPI4 读 IMU 陀螺/加速度原始值（注释标注约 30µs）→ `App_SampleISR()` → 运行中 `Datalog_Push()`。
- [x] `App/app.c::App_ControlISR`（TIM6 2ms，NVIC (0,0)，对应源 pit_track）：`g_app_control_ticks++` → DL1B 轮询取值 →（Start_flag 后）`IMU_UpdateGyroAngle()` 陀螺积分 → 风扇 RUN 占空比（1600）→ `g_app_control_ticks > APP_RUN_DELAY_COUNT(1000)`（2s 起跑延时）后 `Control_UpdateLegacy()` 循迹主流程 → 非运行态目标速度清零/风扇回 IDLE → `Motor_ReadEncoder()`（取采样器 2ms 累计）→ `Motor_UpdateControl()` 速度闭环输出。
- [x] `App/app.c::App_TaskISR`（TIM7 5ms，NVIC (1,0)）：`Datalog_Flush()` → 电池采样滤波 → 低压保护（`g_app_battery_filtered < 1220 且 > 300` 持续 200 拍 ≈ 1s → `App_RequestStop`）→ 运行态喂狗 → 运行中保持状态灯常亮。
- [x] `Core/Src/stm32h7xx_it.c`：TIM6_DAC/TIM7 IRQHandler → `HAL_TIM_PeriodElapsedCallback`（app.c 内全工程唯一实现）分发；TIM15_IRQHandler 不走 HAL（bsp_sampler.c 自定义）；DMA1_Stream0、USART1、EXTI15_10（WiFi_INT 预留）走 HAL 默认链路。

### Group 3: 主循环与启动流程
- [x] `App/app.c::App_Loop()`：停车态先由 `App_FinalizeStop` 刷日志尾包并输出一次停车原因；随后执行循迹链空跑（供显示实时值）→ 电池采样滤波 → `Menu_Update()` / `Dashboard_Update()` → `Wireless_Adjust()` → 状态灯慢闪。IWDG 已启动时由主循环喂狗。
- [x] 启动序列：`Datalog_Start()` → `App_ResetRunState()` 清控制器/状态机/里程/IMU 运行历史 → 状态灯转常亮 → `g_app_key_flag/g_app_start_flag` 置位 → 开 TIM6/TIM7 → `BSP_Sampler_Start()` → 首次起跑 `BSP_WDT_Init()`，后续起跑只喂狗。

### Group 4: 时钟树与 Cache
- [x] `SystemClock_Config()`：HSE 25MHz → PLL1（M=5/N=192/P=2）→ SYSCLK 480MHz（VOS0 超频档位、FLASH_LATENCY_4）→ HCLK=240MHz（AHB÷2）→ APB1/2/3/4=120MHz（÷2，定时器时钟倍频 240MHz，各 TIM 的 PSC 已 ×2 补偿）。
- [x] PLL3Q=80MHz → SPI4 内核时钟（÷8=10MHz，LSM6DSR 上限）；ADC 内核时钟 PLL2P 83.3MHz（异步 ÷2 = 41.6MHz）。
- [x] `SCB_EnableICache()` 开启 I-Cache；**D-Cache 不开**（ADC DMA 缓冲免 Cache 维护的前提）。

### Group 5: 保护机制与状态指示
- [x] 统一安全停车：低压或出赛道调用 `App_RequestStop`，ISR 内立即清 PI/PWM、风扇回 IDLE、停止 TIM15/TIM6/TIM7；主循环调用 `Datalog_Stop` 完成日志收尾，再恢复 LCD/串口。
- [x] 看门狗：`BSP/bsp_wdt.c` IWDG1 纯寄存器（LSI 32k÷64，RLR=2999 ≈ 6s），`APP_WDG_ENABLE` 编译开关；6s 而非 1s 是为菜单保存参数时的扇区擦除留余量。
- [x] 状态灯：板载蓝灯 PE3（NPN 驱动，**高电平点亮**）；停车慢闪（主循环）/ 运行常亮（TIM7 维持）。

## 实现顺序与依赖

Group 1（入口/时钟/初始化链）是其余一切的前提；Group 4（时钟树）决定 Group 2 所有节拍换算（PSC 补偿、SPI4/ADC 内核时钟）；Group 2 依赖 Phase 02 的 BSP（采样器、编码器、IMU、DL1B、Flash）；Group 3 的启动序列必须在 Group 2/5 的就绪顺序之后（先开定时中断，再启采样器，最后启看门狗）。as-built 追溯与原始实现顺序无关，仅表达依赖关系。

## 风险与取舍

| 已知问题 | 现状影响 | 整改指向 |
|---|---|---|
| 原全局裸名与碰撞风险 | Phase 15 已统一 g_app_ 等模块名称，定义/调用/测试同步；历史旧名见符号表 | Phase 15 自动门禁通过，实车待执行 |
| `App_Loop()` 杂务堆积（显示、调参、电池、LED、启动检测混在一个 if 块） | 主循环职责不清 | Phase 11 |
| `bsp_wdt.h` 与 `app_config.h` 注释写"约 1s 超时"，实际 ≈6s | 注释漂移，误导维护 | Phase 11（注释漂移修正） |
| `bsp_dl1b.c/h` 注释写"周期调用（5ms）"，实际由 TIM6 2ms 调用（原在 TIM7 5ms，迁移后注释未跟） | 注释漂移 | Phase 11 |
| `datalog.h` 注释写"TIM6 控制环每拍调用 Push"，实际在 TIM15 上下文（`App_SampleISR`）；容量/耗时注释仍为片内 Flash 时代旧值 | 注释漂移（细节见 [Phase 07](../07-datalog/requirements.md)） | Phase 11 |
| `IMU_Init()` 内重复调用 `BSP_IMU660RB_Init()`（App_Init 已调一次），第二次返回值未检查 | 冗余初始化，无功能影响 | Phase 11 |
| `BSP_DL1B_Init()` 返回值在 App_Init 中未检查（IMU 有提示，DL1B 无） | 激光测距失效时无串口提示 | Phase 11 |
| `BSP_Flash_Init()` 全工程无人调用 → `Param_Load`/`Datalog_*` 实际不落盘 | **功能实际不生效**（本阶段只记录，不修） | Phase 08（最高优先级） |
| 起跑后低压/出赛道只清标志、定时器不停、电机 PI 残留 | 已由 Phase 12 的统一安全停车与再次起跑复位修复；IWDG 停车态改由主循环喂 | Phase 12 已实现，待上机验证 |
| `App_AbsFloat` 等工具函数滞留 app.c | 轻微 | Phase 11 |

> 2026-10-16 Phase 15 G2：现行传感器/控制符号按[完整映射](../15-code-style-consistency/symbol-map.md)迁移，
> 配置、计算顺序及休眠调用边保持；历史 validation 原文及本阶段实车门禁不变。

> 2026-10-16 Phase 15 G3：现行调度、HMI、参数和日志名称已按[符号表](../15-code-style-consistency/symbol-map.md)统一。
> 本文原阶段行为记录与未完成待办不因此重新验收；现行运行链以 Phase 14 和当前头文件契约为准，历史 validation 保留原文。

## Phase 17 当前实现定位（2026-10-24）

应用固定入口仍在App/app.c；前台和三线任务委托App/system/scheduler.c，初始化/起跑/模式协调归lifecycle.c，首因与立即停车/前台收尾归safety.c。原g_app_*改为Lifecycle/Battery/Safety接口，HAL定时回调、时钟补丁、灯和定时链动作归BSP/bsp_runtime.c；原1/2/5ms、两次前台收尾检查和看门狗顺序不变。

本节是现行归属导航；上文历史计划、验证、提交路径及未完成实机项目保持。实际重构验证见
[Phase17验收](../17-app-architecture/validation.md)，逐符号迁移见[迁移导航](../17-app-architecture/migration-map.md)。

> 2026-11-07 文档纠偏：本页恢复本阶段完整正文（恢复依据 `68cb508`），原路径、符号、决定和结果按当时基线解释。当前功能见 [Docs](../../../Docs/index.md)，后续演进见 [路线图](../../roadmap.md)。历史验证安排不自动成为后续每次改动的门禁；本次未重新验收固件或实机。


## 现行功能导航

- [P01功能索引](../../../Docs/traceability.md#p01)
- [F01 初始化与三线调度](../../../Docs/system/runtime-and-safety.md#f01)
- [F29 时钟DMA采样与驱动](../../../Docs/platform/board-and-bsp.md#f29)
- [F07 IMU校准滤波与积分](../../../Docs/sensing/sensors-and-calibration.md#f07)
- [F14 菜单按键与显示](../../../Docs/hmi/operation.md#f14)
- [F15 多入口参数应用](../../../Docs/services/parameters-and-tuning.md#f15)
- [F02 起停、模式与首因](../../../Docs/system/runtime-and-safety.md#f02)
- [F20 队列前台写入与故障](../../../Docs/services/recording-and-export.md#f20)
- [F19 日志格式容量](../../../Docs/services/recording-and-export.md#f19)
- [F09 活动电磁元素](../../../Docs/control/elements-and-dormant-code.md#f09)
- [F08 方向角度速度与执行](../../../Docs/control/tracking-and-controllers.md#f08)
- [F11 双FSM与唯一执行](../../../Docs/control/tracking-and-controllers.md#f11)
- [F03 低压持续判定](../../../Docs/system/runtime-and-safety.md#f03)
