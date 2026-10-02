# Phase 02: BSP 平台层 — 需求

> as-built 规约化追认：以下需求逆向自已存在的 `BSP/` 代码，描述现状（含缺陷），作为整改对照基准。按模块组织；每模块给出 API、外设/引脚映射、关键机制、被谁依赖。需求编号供 [validation.md](validation.md) 与后续 Phase 引用。

## 功能需求

按模块分节列出（FR-1~FR-26）；各模块小节同时给出 API、外设/引脚映射、关键机制与被谁依赖（兼作接口约定）。

## 总体

- FR-1: App 层只经 `bsp.h` 接触硬件；全部 HAL/CubeMX 外设头文件依赖收拢在 `BSP/`（techstack 硬性约束 1）。
- FR-2: `bsp.h` 为唯一聚合头，包含全部 BSP 模块头与 CubeMX 外设头；App 模块只 `#include "bsp.h"`。

## bsp_uart（调试/调参串口）

- API：`BSP_UART_Init / Write / WriteString / Read / RxAvailable / RxFlush`；`fputc` 重定向 printf。
- 外设/引脚：USART1 PA9(TX)/PA10(RX)，115200 8N1，NVIC (3,0)。
- FR-3: 接收为单字节中断 + 256 字节环形缓冲（2 的幂位运算回绕）；**缓冲满丢弃最旧字节**为新字节让位。
- FR-4: UART 错误回调清 ORE/FE/NE/PE 四标志并重新武装接收（一次总线错误后接收不得永久停摆）。
- FR-5: 发送为阻塞式（超时 100ms），返回未发送字节数，0 = 全部发完。
- 被谁依赖：wireless（调参）、datalog（CSV 导出）、app（IMU 失败提示）及全局 printf。

## bsp_adc（十通道模拟采集）

- API：`BSP_ADC_Init`（校准 + 启动 DMA 循环）/ `BSP_ADC_Read(ch)`（返回 12bit 值 0~4095）。
- 外设/引脚：ADC1 扫描 + DMA1_Stream0 循环；16bit 分辨率、连续模式、采样 16.5cyc、内核时钟 41.6MHz。
- FR-6: 通道枚举 ≡ DMA 缓冲下标 ≡ .ioc rank 序（变更 .ioc rank 必须同步枚举）：

| 枚举 | 通道/引脚 | 用途 |
|---|---|---|
| IND_H_L / IND_V_L / IND_V_R / IND_H_R | INP10(PC0) / INP11(PC1) / INP18(PA4) / INP8(PC5) | 电感×4（横左/竖左/竖右/横右） |
| GRAY | INP4(PC4) | 灰度 |
| KEY1 / KEY2 | INP14(PA2) / INP15(PA3) | ADC 分压按键×2 |
| VBAT | INP9(PB0) | 电池电压 |
| SPARE1 / SPARE2 | INP5(PB1) / INP19(PA5) | 备用 |

- FR-7: 读取时 16bit 原始值 >>4 归一化到 12bit（源工程全部阈值基于此标度，`ADC_FULL_SCALE=4095`）。
- TC-1: DMA 目标缓冲绝对定位 D2 SRAM1 `0x30000000`（`__attribute__((at))` / section）；DTCM 不可被 DMA1 访问；D-Cache 关闭故免 Cache 维护（techstack 约束 6）。
- 被谁依赖：track_sensor（电感）、menu（按键组）、app（电池电压）。

## bsp_pwm（电机/风扇输出）

- API：`BSP_PWM_Init` / `BSP_PWM_SetDuty(ch, duty)`。
- 外设/引脚：TIM13_CH1(PA6)=左电机、TIM4_CH3(PD14)=右电机、TIM14_CH1(PA7)=负压风扇，均 17kHz（PSC=2-1，ARR=7059-1，定时器时钟 240MHz）。电机方向脚：左 PE7（正转=高）、右 PE15（正转=低，由 motor 层操作，不在本模块）。
- FR-8: 对外占空比语义统一 0~10000 万分比（`BSP_PWM_DUTY_MAX`），超限内部截断；CCR 换算只在 BSP 内发生（techstack 约束 4）。
- FR-9: 通道到定时器/通道号为表驱动（数组映射），新增通道只扩表。
- 被谁依赖：motor（左右电机）、app（风扇 IDLE/RUN）。

## bsp_encoder（双轮编码器）

- API：`BSP_Encoder_Init` / `BSP_Encoder_GetLeft / GetRight`（读计数并清零，前进为正）。
- 外设/引脚：TIM5(PA0/PA1)=左轮（32bit 定时器，⚠PA0/PA1 为 TTa 脚限 3.3V）、TIM3(PC6/PC7)=右轮（FT 脚兼容 5V）；TI12 四倍频正交、输入滤波 6。
- FR-10: 读即清零语义；方向由 `BSP_ENCODER_L_INVERT(=1)` / `R_INVERT(=0)` 宏修正（接线反了改宏）。
- 被谁依赖：仅 bsp_sampler（1ms 读清）；应用层一律经采样器取数，不直接调用。

## bsp_key（启动键）

- API：`BSP_Key_Init` / `BSP_Key_StartPressed`。
- 外设/引脚：PC13（板载 K1，下拉输入，按下为高电平），主循环轮询。
- FR-11: 按下沿检测 + 30ms 消抖，一次按下只返回一次有效。
- 非目标：ADC 分压键盘（PA2/PA3）不在此模块——由 menu 直接经 `BSP_ADC_Read` 读取分压值判定。
- 被谁依赖：app 主循环（启动序列触发）。

## bsp_sampler（TIM15 高速采样器）

- API：`BSP_Sampler_Init（只配置不启动）/ Start / Stop / GetTick / GetEncL_1ms / GetEncR_1ms / GetGyroRaw / GetAccRaw / ConsumeEncL / ConsumeEncR`。
- 外设：TIM15 纯寄存器配置（PSC=240-1，ARR=1000-1 → 1kHz），NVIC (0,0)，不占 CubeMX。
- FR-12: 1ms 每拍：读清双编码器（提供 1ms 窗口值与 2ms 累计值两套数据）、SPI4 读 IMU 陀螺/加速度原始值（约 30µs）、调用 `App_SampleISR` 钩子。
- FR-13: `ConsumeEncL/R` 返回自上次调用以来的累计并清零，供 TIM6 控制环 2ms 消费。
- TC-2: 免锁前提 = TIM15 与 TIM6 同 NVIC 优先级 0 互不抢占（techstack 约束 3）；改优先级即破坏本契约。
- TC-3: `extern App_SampleISR` 是**有意的分层破例**（BSP 反向依赖 App 钩子），仅此一处，不得扩散。
- 被谁依赖：app（钩子）、motor（编码器累计）、imu_proc（陀螺缓存）、datalog（tick/编码器/陀螺）。

## bsp_imu660rb（惯性测量）

- API：`BSP_IMU660RB_Init`（0=成功）/ `GetAcc / GetGyro`（三轴原始值）。
- 外设/引脚：SPI4 10MHz（PLL3Q 80M÷8，与 LCD 共享总线），CS=PD3。
- FR-14: 初始化自检 WHO_AM_I(0x0F) 期望读回 0x6B（重试上限 0xFF 次）；随后写寄存器序列（移植自逐飞 zf_device_imu660rb.c）：±8g / ±2000dps，ODR 1.66kHz（1ms 采样每拍取到新数据）。
- FR-15: 原始值换算：陀螺 ÷14.3 = °/s，加速度 ÷4098 = g（`app_config.h` 宏）；轴符号约定与逐飞库一致。
- 被谁依赖：bsp_sampler（运行期 1ms 读取）、imu_proc（校准期直读）。

## bsp_dl1b（激光测距）

- API：`BSP_DL1B_Init`（0=成功）/ `BSP_DL1B_Update`（周期轮询，非阻塞）/ `BSP_DL1B_GetDistanceMm`。
- 外设/引脚：I2C2(PB10/PB11) 400kHz Fast Mode，7 位地址 0x29（HAL 左移 0x52），XS 使能脚 PE8。
- FR-16: 初始化执行 XS 复位时序（高 50ms→低 10ms→高 50ms）→ 固件状态检查 → 写入 135 字节 VL53L1X 默认配置（提取自逐飞 config lib）→ 等待生效（超时退出）。
- FR-17: Update 非阻塞轮询：数据就绪则清中断并取距离；无效/超量程（>4000mm 或读失败）返回 `BSP_DL1B_INVALID = 8192`。
- 被谁依赖：app（TIM6 2ms 轮询——注释中"5ms"为漂移，见 [plan.md](plan.md) 风险节）→ control/roundabout。

## bsp_lcd + lcd/（ST7735 显示）

- API：`BSP_LCD_Init / Clear / SetBacklight(0~100) / ShowChar / ShowString / ShowInt / ShowFloat`；`BSP_LCD_W/H`（横屏 160×80）。
- 外设/引脚：SPI4 共享总线；板载屏 CS=PE11、DC=PE13、RST 硬接 NRST、背光 TIM1_CH2N(PE10) 10kHz（OCNPolarity=LOW，**低电平点亮、CCR 越大越亮**）；外接屏备选 CS=PE9/RST=PD9/背光 PD10 GPIO，由 `LCD_TARGET_EXTERNAL` 编译期互斥选择。
- FR-18: 底层为 ST MCD ST7735 vendor 组件（`lcd/st7735.c` 1120 行，BSD 3-Clause）+ SDK 03-LCD_Test 移植胶水层（`lcd/lcd.c`），**只裁剪不修改**（techstack 组件约定）。
- FR-19: 默认横屏 ROT180、HannStar 面板、RGB565；8×16 ASCII 字库**转置块写**（`BSP_LCD_ShowChar` 的 `write[16][8]` 转置缓冲——该面板坐标映射特殊，禁止改常规行序写法，已踩坑）。
- FR-20: `ShowFloat` 用整数拆分实现（MicroLIB 不支持 %f）；`ShowInt` 右对齐补空格。
- 隐患（现状记录）：`lcd/font.h` 在头文件定义非 static 全局字库 `asc2_1608[95][16]`，多文件包含将多重定义（当前仅 bsp_lcd.c 包含）。
- 被谁依赖：menu（外接屏完整菜单）、dashboard（板载屏仪表盘）。
- 总线契约：LCD 只在停车态使用，IMU 只在运行态读取（见 [Phase 01](../01-runtime-architecture/requirements.md) TC-3），SPI4 无需仲裁。

## bsp_w25q64（板载 SPI Flash）

- API：`BSP_W25Q64_Init / Read / WritePage / EraseSector(4KB) / EraseBlock64K / EraseChip`；尺寸宏 `W25Q64_TOTAL_SIZE=8MB / PAGE=256B / SECTOR=4KB`。
- 外设/引脚：SPI1（SCK=PB3/MISO=PB4/MOSI=PD7/CS=PD6，板载焊死），**纯代码初始化不占 .ioc**，30MHz（PLL1Q 480M÷16），MODE0。
- FR-21: Init 退出掉电 + JEDEC ID 自检（期望 0xEF4017），失败返回 1。
- FR-22: WritePage 不跨页（调用方保证，函数内校验）；擦除/编程后等待 WIP 清 0。
- 隐患（现状记录）：`w25q_wait_idle()` 无超时，芯片故障时死等。
- 被谁依赖：仅 bsp_flash。

## bsp_flash（参数/日志存储服务）

- API：`BSP_Flash_Init / Read / Write`（参数区）+ `BSP_Flash_LogErase / LogWrite / LogRead`（日志区）；`BSP_FLASH_LOG_SIZE = 8MB - 4KB`。
- 布局：参数区 = 末尾 4KB 扇区 `0x7FF000`；日志区 = `0x000000~0x7FEFFF`。
- FR-23: 参数写为整扇区读-改-擦-写（static 影子缓冲 4KB）；memcmp 内容不变则跳过擦写；片内 Flash 不使用。
- FR-24: 日志区「边写边擦」追加写：LogErase 只擦首个扇区（起跑几乎无延时），LogWrite 跨入未擦除扇区时自动先擦（几十 ms）；offset/len 必须 32B 对齐，按 256B 页拆分编程。
- FR-25: 全部接口在 `s_ready=0`（Init 未调用/自检失败）时返回错误，不产生总线动作。
- 隐患（现状记录）：`BSP_Flash_Write` 栈上 4KB 局部缓冲 `current[]`；**`BSP_Flash_Init()` 全工程无人调用，参数/日志链现状实际不生效**（Phase 08 修复，最高优先级）。
- 被谁依赖：param（参数区）、datalog（日志区）。

## bsp_wdt（独立看门狗）

- API：`BSP_WDT_Init`（使能，启动后不可关闭）/ `BSP_WDT_Feed`。
- 外设：IWDG1 纯寄存器（HAL 未使能 IWDG 模块，Drivers 无 stm32h7xx_hal_iwdg.c）。
- FR-26: LSI 32kHz ÷64，RLR=2999 → 超时 ≈6s（给参数保存的扇区擦除留余量，不能取 1s）；窗口禁用；`WDG_ENABLE` 编译开关（关闭为空实现）。
- 隐患（现状记录）：`bsp_wdt.h` 头注释写"约 1s 超时"，与实际 6s 漂移（.c 注释正确）。
- 被谁依赖：app（启动键按下时 Init、TIM7 5ms 喂狗，见 [Phase 01](../01-runtime-architecture/requirements.md) FR-10/FR-13）。

## 技术约束（汇总）

- TC-4: 三种外设接管风格并存为显式设计决策（CubeMX+HAL / 纯寄存器 / 纯代码初始化 / 总线 CubeMX + 器件 BSP），新增驱动按 [plan.md](plan.md) 表格归类，不得随意混用。
- TC-5: HAL 固件包 V1.11.2、AC5、EIDE GCC 工具链版本锁定（techstack 约束 7）；vendor 组件（st7735）只裁剪不修改。
- TC-6: 接口命名 `BSP_模块_动作()`；模块文件小写下划线；移植算法注释标明出处。
- TC-7: DMA/总线缓冲不得放入 DTCM；新增 DMA 缓冲须满足 D2 SRAM 定位 + D-Cache 关闭前提（techstack 约束 6）。

## 非目标（Non-goals）

- 不为 ADC 键盘（PA2/PA3）封装按键事件层（现状由 menu 直读采样值，语义够用）。
- 不实现 SPI4 总线仲裁/互斥锁（靠 Phase 01 的停车/运行状态隔离）。
- 不启用备用外设（SPI2 WiFi、TIM16/17 备用 PWM、灰度 IO）——预留不占规约。
- 不修复本文件记录的现状缺陷（w25q 无超时、栈上 4KB、font.h 非 static、注释漂移、Flash Init 未调用等），全部按 roadmap 转入 Phase 08–11。
- 不覆盖 BSP 之上的算法语义（滤波/PID/状态机/菜单/记录格式），属 Phase 03–07。
