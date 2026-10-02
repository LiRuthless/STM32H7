# Phase 02: BSP 平台层 — 实现计划

> **as-built 规约**：代码已存在，本文档为存量功能的规约化追认，描述现状（含已知缺陷），作为 Phase 08–11 整改的对照基准。任务分组全部已完成，内容为实现结构追溯而非待办。

## 目标

把 `BSP/` 目录全部板级驱动（12 个模块 + LCD vendor 组件）规约化：固定每个驱动的 API 契约、外设/引脚映射、关键机制与依赖方，坐实"App 层只经 `bsp.h` 接触硬件"的分层红线。

## 背景与依据

- 关联宪章：[techstack.md](../../techstack.md) 硬性约束 1（分层红线）、3（TIM15/TIM6 优先级不变量）、4（占空比语义 0~10000）、5（CubeMX 再生保护）、6（D-Cache 关闭）；[mission.md](../../mission.md) 范围内"支撑功能的 BSP 驱动层"。
- 前置条件：无（与 Phase 01 同为基座阶段）；被 [Phase 01](../01-runtime-architecture/requirements.md) 调度骨架与 Phase 03–07 全部应用模块依赖。
- 驱动出处：应用无关的器件流程移植自逐飞库（`zf_device_imu660rb.c` / `zf_device_dl1b.c`）与核心板 SDK（`03-LCD_Test`，ST MCD ST7735 组件）；`Sirius20260718` 仅提供参数语义（占空比万分比、阈值标度等）。

## 设计决策：三种外设接管风格并存

| 风格 | 模块 | 原因 |
|---|---|---|
| CubeMX + HAL（.ioc 为唯一事实源） | ADC1、USART1、I2C2、SPI4、SPI2、TIM1/3/4/5/6/7/13/14/16/17 | 常规外设，CubeMX 维护引脚/时钟/中断 |
| 纯寄存器直配（不占 .ioc） | TIM15（bsp_sampler）、IWDG1（bsp_wdt） | TIM15 需与 HAL 定时器体系隔离保证节拍纯净；HAL 未使能 IWDG 模块，只能寄存器实现 |
| 纯代码初始化（HAL 句柄手工填充） | SPI1（bsp_w25q64） | 板载 W25Q64 引脚焊死，不占用 .ioc 外设配额，避免 CubeMX 再生干扰 |
| CubeMX 配总线 + BSP 配器件寄存器 | IMU660RB（SPI4）、DL1B（I2C2）、ST7735（SPI4 + TIM1 背光） | 器件协议层属驱动职责，不属于 CubeMX |

此为**有意的架构决策**，不是不一致：新增驱动时按上表归类选择风格，并在对应模块头注释中说明。

## 任务分组（实现结构追溯，全部完成）

### Group 1: 通信类驱动
- [x] `bsp_uart.c`：USART1(PA9/PA10) 115200；单字节 RXNE 中断 + 256B 环形缓冲（满丢最旧）；错误回调清 ORE/FE/NE/PE 四标志重新武装；`fputc` 重定向 printf。被 wireless、datalog（CSV 导出）、app（调试提示）依赖。
- [x] `bsp_imu660rb.c`：SPI4 10MHz（与 LCD 共享总线）CS=PD3；WHO_AM_I=0x6B 自检（重试 0xFF 次）；±8g/±2000dps @1.66kHz；移植自逐飞。被 bsp_sampler（1ms 读取）、imu_proc（校准直读）依赖。
- [x] `bsp_dl1b.c`：I2C2 400kHz(PB10/PB11)，7 位地址 0x29，XS=PE8；135 字节 VL53L1X 默认配置数组（提取自逐飞 config lib）；非阻塞轮询；无效距离 8192。被 app（TIM6 轮询）→ control/roundabout 依赖。

### Group 2: 采集与执行类驱动
- [x] `bsp_adc.c`：ADC1 十通道扫描 + DMA1_Stream0 循环；16bit >>4 归一化 12bit；缓冲绝对定位 D2 SRAM1 `0x30000000`；通道枚举 ≡ DMA 下标 ≡ .ioc rank 序。被 track_sensor（电感×4）、menu（按键×2）、app（电池）依赖。
- [x] `bsp_pwm.c`：TIM13_CH1(PA6) 左电机 / TIM4_CH3(PD14) 右电机 / TIM14_CH1(PA7) 风扇，均 17kHz（PSC=2-1, ARR=7059-1）；表驱动（定时器/通道数组）；对外占空比 0~10000 万分比，CCR 换算只在内部。被 motor、app（风扇）依赖。
- [x] `bsp_encoder.c`：TIM5(PA0/PA1) 左 / TIM3(PC6/PC7) 右，TI12 四倍频、输入滤波 6；读即清零；`BSP_ENCODER_L_INVERT/R_INVERT` 方向宏。被 bsp_sampler（1ms 读清）独占依赖。
- [x] `bsp_key.c`：PC13 启动键（下拉，按下高电平）；按下沿 + 30ms 消抖。被 app 主循环依赖。ADC 分压键盘不在此模块（menu 直接读 BSP_ADC）。
- [x] `bsp_sampler.c`：TIM15 纯寄存器（PSC=240-1, ARR=1000-1 → 1kHz），NVIC(0,0)；双编码器读清 + 1ms/2ms 双窗口 + IMU 缓存；`extern App_SampleISR` 反向依赖（**有意的分层破例**，头注释标明）；免锁前提是 TIM15 与 TIM6 同优先级。被 app（钩子）、motor（ConsumeEnc）、imu_proc（GetGyroRaw）、datalog（tick/enc/gyro）依赖。

### Group 3: 存储类驱动
- [x] `bsp_w25q64.c`：SPI1（SCK=PB3/MISO=PB4/MOSI=PD7/CS=PD6）纯代码初始化，30MHz（PLL1Q 480M÷16），MODE0；JEDEC 0xEF4017 自检；页 256B / 扇区 4KB。仅被 bsp_flash 依赖。
- [x] `bsp_flash.c`：参数区 = 末尾 4KB 扇区 0x7FF000（影子缓冲读-改-擦-写，memcmp 内容不变跳过）；日志区 0~0x7FEFFF 边写边擦（擦除前沿 s_log_frontier）；日志写 32B 对齐。被 param、datalog 依赖。**现状缺陷：`BSP_Flash_Init()` 全工程无人调用，s_ready 恒 0，参数/日志链实际不生效（Phase 08 修复）。**

### Group 4: 人机与保护类驱动
- [x] `bsp_lcd.c` + `lcd/`：ST MCD ST7735 vendor 组件（st7735.c 1120 行，BSD 3-Clause，只裁剪不修改）+ SDK 移植胶水层（lcd.c）+ 字库（font.h 8×16 ASCII）；SPI4 共享（CS=PE11/DC=PE13），板载屏 RST 硬接 NRST，背光 TIM1_CH2N(PE10) 10kHz 低电平点亮（CCR 越大越亮）；外接屏备选（CS=PE9/RST=PD9/背光 PD10 GPIO），编译期互斥；8×16 字库**转置块写**（该面板 ROT180 横屏坐标映射特殊，禁止改常规行序）。被 menu、dashboard 依赖。
- [x] `bsp_wdt.c`：IWDG1 纯寄存器（LSI 32k÷64，RLR=2999 ≈ 6s），`WDG_ENABLE` 编译开关，启动后不可关闭。被 app（启动时 Init、TIM7 喂狗）依赖。

## 实现顺序与依赖

Group 3 的 bsp_w25q64 是 bsp_flash 的唯一后端；Group 2 的 bsp_encoder/bsp_imu660rb 被 bsp_sampler 聚合（采样器最后做）；bsp_adc 先于一切 ADC 消费方。as-built 追溯与原始实现顺序无关，仅表达依赖关系：w25q64 → flash → (param/datalog)；encoder + imu660rb → sampler → (motor/imu_proc/datalog)；adc → (track_sensor/menu/app)。

## 风险与取舍

| 已知问题 | 现状影响 | 整改指向 |
|---|---|---|
| `BSP_Flash_Init()` 无人调用 → s_ready=0，参数保存/数据记录全链失效 | **功能实际不生效** | Phase 08（最高优先级） |
| `w25q_wait_idle()` 无超时：芯片故障/掉线时死等，可耗尽看门狗复位 | 故障场景死循环 | Phase 11 |
| `BSP_Flash_Write()` 栈上 4KB 局部缓冲 `current[]` | 深栈帧：`_Min_Stack_Size` 仅 0x400（链接期保留值），实际栈自 AXI SRAM 顶向下生长，当前余量充足未溢出，但无 MPU/栈漆防线 | Phase 11 |
| `bsp_dl1b.c/h` 注释写"周期调用（5ms）"，实际由 TIM6 2ms 调用 | 注释漂移 | Phase 11 |
| `bsp_wdt.h` 注释写"约 1s 超时"，实际 ≈6s | 注释漂移 | Phase 11 |
| `lcd/font.h` 在头文件定义非 static 全局字库数组 | 多文件包含将多重定义；当前仅 bsp_lcd.c 包含，未爆雷 | Phase 11 |
| `bsp_key.h` 注释"下降沿/按下沿"表述含糊（实际按下沿=低→高） | 轻微注释歧义 | Phase 11 |
| ADC 键盘（PA2/PA3）无 BSP 封装，menu 直读 BSP_ADC | 分层可接受现状（语义是"采样值"非"按键事件"） | 现状即设计 |
| `BSP_UART_Write` 阻塞超时 100ms 硬编码 | 极端情况阻塞主循环 | Phase 11 视情 |
