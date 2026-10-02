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
- [x] `App/app.c::App_Init()`：BSP_UART_Init → Param_Load → BSP_ADC_Init → 电池低通 `lowpass_init(&filt_battery, 0.65f)` → BSP_Key_Init → `encoder_init`（启动编码器并显式初始化两路 α=0.88 低通）→ BSP_PWM_Init + 风扇 IDLE 占空比（1100）→ BSP_LCD_Init/Clear/背光 100% → BSP_IMU660RB_Init（**失败仅串口提示 "IMU660RB init fail"，不卡死**）→ `imu_proc_init()`（陀螺高通滤波器初始化，内部重复调一次 BSP_IMU660RB_Init）→ `gyro_calibrate()`（静止采样 100 次取零偏，源工程被注释、本工程恢复）→ BSP_DL1B_Init（返回值未检查）→ BSP_Sampler_Init（**只配置 TIM15 不启动**）→ Dashboard_Init（`LCD_TARGET_EXTERNAL=0` 时）。
- [x] SPI45SEL 补丁在 `App_Init()` 开头再写一次：`MX_SPI4_Init` 等会按 .ioc 复位 D2CCIP1R，需在全部 MX_*_Init 之后重写。

### Group 2: 三线中断契约
- [x] `BSP/bsp_sampler.c::TIM15_IRQHandler`（1ms，NVIC 优先级 (0,0)，纯寄存器）：清标志 → 双编码器读清（1ms 窗口值 + 2ms 累计值）→ SPI4 读 IMU 陀螺/加速度原始值（注释标注约 30µs）→ `App_SampleISR()` → 运行中 `Datalog_Push()`。
- [x] `App/app.c::App_ControlISR`（TIM6 2ms，NVIC (0,0)，对应源 pit_track）：`time++` → DL1B 轮询取值 →（Start_flag 后）`read_gyro_angle()` 陀螺积分 → 风扇 RUN 占空比（1600）→ `time > RUN_DELAY_COUNT(1000)`（2s 起跑延时）后 `whole_test()` 循迹主流程 → 非运行态目标速度清零/风扇回 IDLE → `read_encoder()`（取采样器 2ms 累计）→ `motor_control()` 速度闭环输出。
- [x] `App/app.c::App_TaskISR`（TIM7 5ms，NVIC (1,0)）：`Datalog_Flush()` → 电池采样滤波 → 低压保护（`battery_filt < 1220 且 > 300` 持续 200 拍 ≈ 1s → `App_RequestStop`）→ 运行态喂狗 → 运行中保持状态灯常亮。
- [x] `Core/Src/stm32h7xx_it.c`：TIM6_DAC/TIM7 IRQHandler → `HAL_TIM_PeriodElapsedCallback`（app.c 内全工程唯一实现）分发；TIM15_IRQHandler 不走 HAL（bsp_sampler.c 自定义）；DMA1_Stream0、USART1、EXTI15_10（WiFi_INT 预留）走 HAL 默认链路。

### Group 3: 主循环与启动流程
- [x] `App/app.c::App_Loop()`：停车态先由 `App_FinalizeStop` 刷日志尾包并输出一次停车原因；随后执行循迹链空跑（供显示实时值）→ 电池采样滤波 → `menu()` / `Dashboard_Update()` → `wireless_adjust()` → 状态灯慢闪。IWDG 已启动时由主循环喂狗。
- [x] 启动序列：`Datalog_Start()` → `App_ResetRunState()` 清控制器/状态机/里程/IMU 运行历史 → 状态灯转常亮 → `key_flag/Start_flag` 置位 → 开 TIM6/TIM7 → `BSP_Sampler_Start()` → 首次起跑 `BSP_WDT_Init()`，后续起跑只喂狗。

### Group 4: 时钟树与 Cache
- [x] `SystemClock_Config()`：HSE 25MHz → PLL1（M=5/N=192/P=2）→ SYSCLK 480MHz（VOS0 超频档位、FLASH_LATENCY_4）→ HCLK=240MHz（AHB÷2）→ APB1/2/3/4=120MHz（÷2，定时器时钟倍频 240MHz，各 TIM 的 PSC 已 ×2 补偿）。
- [x] PLL3Q=80MHz → SPI4 内核时钟（÷8=10MHz，LSM6DSR 上限）；ADC 内核时钟 PLL2P 83.3MHz（异步 ÷2 = 41.6MHz）。
- [x] `SCB_EnableICache()` 开启 I-Cache；**D-Cache 不开**（ADC DMA 缓冲免 Cache 维护的前提）。

### Group 5: 保护机制与状态指示
- [x] 统一安全停车：低压或出赛道调用 `App_RequestStop`，ISR 内立即清 PI/PWM、风扇回 IDLE、停止 TIM15/TIM6/TIM7；主循环调用 `Datalog_Stop` 完成日志收尾，再恢复 LCD/串口。
- [x] 看门狗：`BSP/bsp_wdt.c` IWDG1 纯寄存器（LSI 32k÷64，RLR=2999 ≈ 6s），`WDG_ENABLE` 编译开关；6s 而非 1s 是为菜单保存参数时的扇区擦除留余量。
- [x] 状态灯：板载蓝灯 PE3（NPN 驱动，**高电平点亮**）；停车慢闪（主循环）/ 运行常亮（TIM7 维持）。

## 实现顺序与依赖

Group 1（入口/时钟/初始化链）是其余一切的前提；Group 4（时钟树）决定 Group 2 所有节拍换算（PSC 补偿、SPI4/ADC 内核时钟）；Group 2 依赖 Phase 02 的 BSP（采样器、编码器、IMU、DL1B、Flash）；Group 3 的启动序列必须在 Group 2/5 的就绪顺序之后（先开定时中断，再启采样器，最后启看门狗）。as-built 追溯与原始实现顺序无关，仅表达依赖关系。

## 风险与取舍

| 已知问题 | 现状影响 | 整改指向 |
|---|---|---|
| 全局命名过泛：`time`/`key_flag`/`uart[32]`/`dat[32]` 等沿用源工程 config.c 命名 | 可读性差、易撞名 | Phase 11（高危命名治理） |
| `App_Loop()` 杂务堆积（显示、调参、电池、LED、启动检测混在一个 if 块） | 主循环职责不清 | Phase 11 |
| `bsp_wdt.h` 与 `app_config.h` 注释写"约 1s 超时"，实际 ≈6s | 注释漂移，误导维护 | Phase 11（注释漂移修正） |
| `bsp_dl1b.c/h` 注释写"周期调用（5ms）"，实际由 TIM6 2ms 调用（原在 TIM7 5ms，迁移后注释未跟） | 注释漂移 | Phase 11 |
| `datalog.h` 注释写"TIM6 控制环每拍调用 Push"，实际在 TIM15 上下文（`App_SampleISR`）；容量/耗时注释仍为片内 Flash 时代旧值 | 注释漂移（细节见 [Phase 07](../07-datalog/requirements.md)） | Phase 11 |
| `imu_proc_init()` 内重复调用 `BSP_IMU660RB_Init()`（App_Init 已调一次），第二次返回值未检查 | 冗余初始化，无功能影响 | Phase 11 |
| `BSP_DL1B_Init()` 返回值在 App_Init 中未检查（IMU 有提示，DL1B 无） | 激光测距失效时无串口提示 | Phase 11 |
| `BSP_Flash_Init()` 全工程无人调用 → `Param_Load`/`Datalog_*` 实际不落盘 | **功能实际不生效**（本阶段只记录，不修） | Phase 08（最高优先级） |
| 起跑后低压/出赛道只清标志、定时器不停、电机 PI 残留 | 已由 Phase 12 的统一安全停车与再次起跑复位修复；IWDG 停车态改由主循环喂 | Phase 12 已实现，待上机验证 |
| `float_abs` 等工具函数滞留 app.c | 轻微 | Phase 11 |
