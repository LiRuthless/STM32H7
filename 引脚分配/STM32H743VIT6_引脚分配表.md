# STM32H743VIT6（LQFP100）引脚分配表

> 基于 **WeAct MiniSTM32H7xx 核心板（V12）**，结合智能车电磁循迹项目功能需求分配。
> 引脚信息来源：STM32CubeMX 官方 MCU 数据库 + 核心板原理图（SchDoc V12）+ 官方 SDK 代码验证。
> 约束：不使用摄像头（DCMI 排针引脚可用）；所有功能引脚均在两侧 2×22 排针上引出。

## 一、核心板板载资源占用引脚（不可挪用）

| 引脚(序号) | 板上用途 | 备注 |
|-----------|----------|------|
| PE2 (1) | QSPI Flash IO2 | 8MB QSPI Flash（焊死） |
| PE3 (2) | 蓝色 LED | 高电平点亮，可作状态灯 |
| PC13 (7) | 用户按键 K1 | 直接用作本项目启动按键 ✓ |
| PC14/PC15 (8/9) | LSE 32.768 kHz | 板载晶振 |
| PH0/PH1 (12/13) | HSE 25 MHz | 板载晶振 |
| PB2 (36) | QSPI Flash CLK | 焊死 |
| PD11~PD13 (58~60) | QSPI Flash IO0/IO1/IO3 | 焊死 |
| PC8/PC9 (65/66) | SDMMC1_D0/D1 | TF 卡座 |
| PA11/PA12 (70/71) | USB_DN/DP | Type-C USB |
| PA13/PA14 (72/76) | SWDIO/SWCLK | 下载调试 |
| PC10~PC12 (78~80) | SDMMC1_D2/D3/CK | TF 卡座 |
| PD2 (83) | SDMMC1_CMD | TF 卡座 |
| PD6/PD7 (87/88) | SPI Flash CS/MOSI | 8MB SPI Flash（SPI1，焊死） |
| PB3/PB4 (89/90) | SPI Flash SCK/MISO | 焊死 |
| PB6 (92) | QSPI Flash NCS | 焊死 |
| BOOT0 (94) | BOOT 按键 | 板载 |
| NRST (14) | 复位按键 | 板载 |

> 注：摄像头 DCMI 排针本项目不使用，引脚已释放；板载 ST7735 用作本车显示屏。

## 二、功能对照表（原 AI8051U → STM32H743VIT6）

| 功能 | 原引脚 | STM32 引脚 | 外设配置 |
|------|--------|-----------|----------|
| 左驱动 PWM | P2.7 | **PA6** (30) | TIM13_CH1。有刷：17 kHz；无刷电调：50~490 Hz / 1~2 ms |
| 左电机方向 DIR | P2.6 | **PE7** (37) | GPIO 输出。有刷驱动器方向脚（无刷时不用，可作电调使能） |
| 右驱动 PWM | P2.5 | **PD14** (61) | TIM4_CH3。有刷：17 kHz；无刷电调：50~490 Hz / 1~2 ms |
| 右电机方向 DIR | P5.1 | **PE15** (45) | GPIO 输出。有刷驱动器方向脚（无刷时不用，可作电调使能） |
| 负压电机 PWM | P2.2 | **PA7** (31) | TIM14_CH1，100 Hz~17 kHz 可调 |
| 备用 PWM ×2（与驱动同级） | — | **PB9** (96) / **PB8** (95) | TIM17_CH1 / TIM16_CH1，独立定时器，FT 脚 |
| 备用 ADC ×2（与电感同级） | — | **PB1** (35) / **PA5** (29) | ADC1_INP5 / ADC1_INP19，可加入 DMA 扫描 |
| 扩展 LED | — | **PB5** (91) | GPIO 输出（FT 脚） |
| 左编码器 A/B | P3.5+P2.4 | **PA0/PA1** (22/23) | TIM5 编码器模式（TTa 脚，⚠限 3.3V 编码器） |
| 右编码器 A/B | P3.4+P1.1 | **PC6/PC7** (63/64) | TIM3 编码器模式（FT 脚，兼容 5V 编码器） |
| 电感 1 | P0.5 | **PC0** (15) | ADC1_INP10，DMA 扫描 |
| 电感 2 | P0.6 | **PC1** (16) | ADC1_INP11，DMA 扫描 |
| 电感 3 | P0.0 | **PA4** (28) | ADC1_INP18，DMA 扫描 |
| 电感 4 | P0.2 | **PC5** (33) | ADC1_INP8，DMA 扫描 |
| 按键 ADC1（方向+确定） | P1.4 | **PA2** (24) | ADC1_INP14 |
| 按键 ADC2（复位+返回） | P1.3 | **PA3** (25) | ADC1_INP15 |
| 电池电压检测 | P0.3 | **PB0** (34) | ADC1_INP9 |
| 灰度传感器 ADC | — | **PC4** (32) | ADC1_INP4，加入 ADC1 DMA 扫描序列 |
| 灰度传感器 IO ×4 | — | **PE0/PE1/PE4/PE6** (97/98/3/5) | GPIO（FT 脚耐 5V，排针引出），数字量/通道控制 |
| 串口 TX（调试） | P3.1 | **PA9** (68) | USART1_TX，115200 |
| 串口 RX（调试） | P3.0 | **PA10** (69) | USART1_RX，115200 |
| IMU660RB SCK | P3.3 | **PE12** (42) | 硬件 SPI4_SCK（与屏共用） |
| IMU660RB MOSI | P3.2 | **PE14** (44) | 硬件 SPI4_MOSI（与屏共用） |
| IMU660RB MISO | P4.3 | **PE5** (4) | 硬件 SPI4_MISO |
| IMU660RB CS | P3.7 | **PD3** (84) | GPIO 输出（与屏 CS 分离） |
| IMU 软件 SPI 备选 | — | **PD4/PD5/PD0/PD1** (85/86/81/82) | 软 SPI 备选方案，保留 |
| DL1B SCL | P1.0 | **PB10** (46) | I2C2_SCL，≤400 kHz |
| DL1B SDA | P4.2 | **PB11** (47) | I2C2_SDA |
| DL1B XS | P4.7 | **PE8** (38) | GPIO 输出 |
| ST7735 显示屏（板载） | P1.7 等 | **SPI4：PE12/PE14 + PE11/PE13/PE10** (40~44) | 详见下方“板载屏”小节 |
| 启动按键 | P2.3 | **PC13** (7) | 板载 K1 按键，下降沿触发 |
| 程序下载调试 | — | **PA13/PA14** (72/76) | SWDIO/SWCLK（板载 SW 排针） |

### 板载 ST7735 显示屏（SPI4，硬接线固定）

| 功能 | STM32 引脚 | 说明 |
|------|-----------|------|
| LCD SCK | **PE12** (42) | SPI4_SCK（两屏共用） |
| LCD MOSI(SDA) | **PE14** (44) | SPI4_MOSI（两屏共用） |
| LCD CS | **PE11** (41) | GPIO 输出 |
| LCD DC(WR_RS) | **PE13** (43) | GPIO 输出（两屏共用） |
| LCD 背光 BLK | **PE10** (40) | TIM1_CH2N，可 PWM 调光 |
| LCD RST | 接 NRST | 板载硬接系统复位，不占 GPIO |

### 外接显示屏预留（与板载屏共用 SPI4，⚠两屏不同时使用）

| 功能 | STM32 引脚 | 说明 |
|------|-----------|------|
| 外接屏 SCK / MOSI / DC | 共用 **PE12 / PE14 / PE13** | 与板载屏同总线 |
| 外接屏 CS | **PE9** (39) | GPIO 输出（各自的片选，软件互斥） |
| 外接屏 RST | **PD9** (56) | GPIO 输出 |
| 外接屏 BLK | **PD10** (57) | GPIO 输出 |

> 使用原则：板载屏 CS(PE11) 与外接屏 CS(PE9) 任意时刻只拉低一个；平时调参用板载屏，需要大屏时插上外接屏并在代码中切换。SPI 带宽无冲突（不同时刷新）。

### WiFi 调试模块预留（SPI2 + 2 GPIO）

| 功能 | STM32 引脚 | 说明 |
|------|-----------|------|
| WiFi SPI SCK | **PB13** (52) | SPI2_SCK |
| WiFi SPI MISO | **PB14** (53) | SPI2_MISO |
| WiFi SPI MOSI | **PB15** (54) | SPI2_MOSI |
| WiFi SPI CS | **PB12** (51) | GPIO 输出 |
| WiFi INT | **PA15** (77) | GPIO 输入/EXTI，模块中断/就绪（需关闭 JTAG） |
| WiFi RST | **PD15** (62) | GPIO 输出，模块复位 |

> 原 STC 方案的无线串口 RTS（P3.6）取消，STM32 无线/WiFi 调试统一走 UART4 或 USART1。

## 三、LQFP100 完整引脚表（按封装引脚顺序）

排针说明：PIP10 排针（引脚 63~98 区域为主）、PIP20 排针（引脚 1~47 区域为主），所有分配引脚均在排针上。

| 引脚 | 引脚名 | 分配 | 功能说明 / 板载用途 |
|---:|--------|------|---------------------|
| 1 | PE2 | 🔒板载 | QSPI Flash IO2（8MB，焊死） |
| 2 | PE3 | 🔒板载 | 蓝色 LED（高电平点亮，可作状态灯） |
| 3 | PE4 | **灰度 IO1** | GPIO（FT 耐 5V），数字量/通道控制 |
| 4 | PE5 | **IMU MISO** | SPI4_MISO（硬件方案，与屏共用 SPI4） |
| 5 | PE6 | **灰度 IO2** | GPIO（FT 耐 5V） |
| 6 | VBAT | 电源 | 后备电池供电，接 3.3V |
| 7 | PC13 | **启动按键** | 板载用户按键 K1，下降沿触发 Start |
| 8 | PC14-OSC32_IN | 🔒晶振 | 板载 LSE 32.768 kHz |
| 9 | PC15-OSC32_OUT | 🔒晶振 | 板载 LSE 32.768 kHz |
| 10 | VSS | 电源 | 数字地 |
| 11 | VDD | 电源 | 数字电源 3.3V |
| 12 | PH0-OSC_IN | 🔒晶振 | 板载 HSE 25 MHz |
| 13 | PH1-OSC_OUT | 🔒晶振 | 板载 HSE 25 MHz |
| 14 | NRST | 🔒复位 | 板载复位按键 |
| 15 | PC0 | **电感 1** | ADC1_INP10 |
| 16 | PC1 | **电感 2** | ADC1_INP11 |
| 17 | PC2_C | 未使用 | ADC3 模拟开关脚，为避干扰弃用（可复用：ADC3_INP0） |
| 18 | PC3_C | 未使用 | ADC3 模拟开关脚，为避干扰弃用（可复用：ADC3_INP1） |
| 19 | VSSA | 电源 | 模拟地 |
| 20 | VREF+ | 电源 | ADC 参考电压（板载接 VDDA） |
| 21 | VDDA | 电源 | 模拟电源 3.3V |
| 22 | PA0 | **左编码器 A** | TIM5_CH1，编码器模式（⚠TTa 脚，限 3.3V 编码器） |
| 23 | PA1 | **左编码器 B** | TIM5_CH2，编码器模式（⚠TTa 脚，限 3.3V 编码器） |
| 24 | PA2 | **按键 ADC1** | ADC1_INP14（方向键+确定） |
| 25 | PA3 | **按键 ADC2** | ADC1_INP15（复位+返回） |
| 26 | VSS | 电源 | 数字地 |
| 27 | VDD | 电源 | 数字电源 3.3V |
| 28 | PA4 | **电感 3** | ADC1_INP18（原 DCMI_HSYNC，已释放） |
| 29 | PA5 | **备用 ADC** | ADC1_INP19（与电感同级，可加 DMA 扫描） |
| 30 | PA6 | **左驱动 PWM** | TIM13_CH1。有刷 17 kHz / 无刷 50~490 Hz（原 DCMI_PIXCLK，已释放） |
| 31 | PA7 | **负压电机 PWM** | TIM14_CH1，100 Hz~17 kHz 可调 |
| 32 | PC4 | **灰度 ADC** | ADC1_INP4，灰度传感器模拟量（加入 ADC1 DMA 扫描） |
| 33 | PC5 | **电感 4** | ADC1_INP8，DMA 扫描（邻脚为灰度/电池模拟量，无干扰） |
| 34 | PB0 | **电池电压** | ADC1_INP9，分压采样 |
| 35 | PB1 | **备用 ADC** | ADC1_INP5（与电感同级，可加 DMA 扫描） |
| 36 | PB2 | 🔒板载 | QSPI Flash CLK（焊死） |
| 37 | PE7 | **左电机方向 DIR** | GPIO 输出，有刷用（无刷时可作电调使能） |
| 38 | PE8 | **DL1B XS** | GPIO 输出，激光测距使能/复位（仅初始化时翻转） |
| 39 | PE9 | **外接屏 CS** | GPIO 输出（与板载屏共用 SPI4 总线，互斥使用） |
| 40 | PE10 | **LCD 背光 BLK** | 板载 ST7735，TIM1_CH2N 可 PWM 调光 |
| 41 | PE11 | **LCD CS** | 板载 ST7735 片选（硬接线） |
| 42 | PE12 | **LCD SCK** | 板载 ST7735，SPI4_SCK（硬接线） |
| 43 | PE13 | **LCD DC** | 板载 ST7735 命令/数据（硬接线） |
| 44 | PE14 | **LCD MOSI** | 板载 ST7735，SPI4_MOSI（硬接线） |
| 45 | PE15 | **右电机方向 DIR** | GPIO 输出，有刷用（无刷时可作电调使能） |
| 46 | PB10 | **DL1B SCL** | I2C2_SCL，≤400 kHz |
| 47 | PB11 | **DL1B SDA** | I2C2_SDA |
| 48 | VCAP | 电源 | 内核稳压电容（板载已有） |
| 49 | VSS | 电源 | 数字地 |
| 50 | VDD | 电源 | 数字电源 3.3V |
| 51 | PB12 | **WiFi CS** | GPIO 输出，WiFi 模块片选 |
| 52 | PB13 | **WiFi SCK** | SPI2_SCK |
| 53 | PB14 | **WiFi MISO** | SPI2_MISO |
| 54 | PB15 | **WiFi MOSI** | SPI2_MOSI |
| 55 | PD8 | 未使用 | 可复用：USART3_TX |
| 56 | PD9 | **外接屏 RST** | GPIO 输出（外接屏不用时可作普通 IO） |
| 57 | PD10 | **外接屏 BLK** | GPIO 输出（外接屏不用时可作普通 IO） |
| 58 | PD11 | 🔒板载 | QSPI Flash IO0（焊死） |
| 59 | PD12 | 🔒板载 | QSPI Flash IO1（焊死） |
| 60 | PD13 | 🔒板载 | QSPI Flash IO3（焊死） |
| 61 | PD14 | **右驱动 PWM** | TIM4_CH3。有刷 17 kHz / 无刷 50~490 Hz |
| 62 | PD15 | **WiFi RST** | GPIO 输出（⚠勿用作 TIM4_CH4 PWM，与右驱动 PD14 同一定时器） |
| 63 | PC6 | **右编码器 A** | TIM3_CH1，编码器模式（原 DCMI_D0，已释放） |
| 64 | PC7 | **右编码器 B** | TIM3_CH2，编码器模式（原 DCMI_D1，已释放） |
| 65 | PC8 | 🔒板载 | SDMMC1_D0（TF 卡） |
| 66 | PC9 | 🔒板载 | SDMMC1_D1（TF 卡） |
| 67 | PA8 | 未使用 | 原摄像头 MCO1，已释放（TIM1 已作编码器，PWM 不可用） |
| 68 | PA9 | **串口 TX** | USART1_TX，115200 |
| 69 | PA10 | **串口 RX** | USART1_RX，115200 |
| 70 | PA11 | 🔒板载 | USB_DN（Type-C） |
| 71 | PA12 | 🔒板载 | USB_DP（Type-C） |
| 72 | PA13 | 🔒调试 | SWDIO（板载 SW 排针） |
| 73 | VCAP | 电源 | 内核稳压电容（板载已有） |
| 74 | VSS | 电源 | 数字地 |
| 75 | VDD | 电源 | 数字电源 3.3V |
| 76 | PA14 | 🔒调试 | SWCLK（板载 SW 排针） |
| 77 | PA15 | **WiFi INT** | GPIO 输入/EXTI，WiFi 模块中断（需关闭 JTAG） |
| 78 | PC10 | 🔒板载 | SDMMC1_D2（TF 卡） |
| 79 | PC11 | 🔒板载 | SDMMC1_D3（TF 卡） |
| 80 | PC12 | 🔒板载 | SDMMC1_CK（TF 卡） |
| 81 | PD0 | 软SPI备选 | IMU MISO（软件方案，备选） |
| 82 | PD1 | 软SPI备选 | IMU CS（软件方案，备选） |
| 83 | PD2 | 🔒板载 | SDMMC1_CMD（TF 卡） |
| 84 | PD3 | **IMU CS** | GPIO 输出（硬件 SPI4 方案片选） |
| 85 | PD4 | 软SPI备选 | IMU SCK（软件方案，备选） |
| 86 | PD5 | 软SPI备选 | IMU MOSI（软件方案，备选） |
| 87 | PD6 | 🔒板载 | SPI Flash CS（SPI1，焊死） |
| 88 | PD7 | 🔒板载 | SPI Flash MOSI（SPI1，焊死） |
| 89 | PB3 | 🔒板载 | SPI Flash SCK（SPI1，焊死） |
| 90 | PB4 | 🔒板载 | SPI Flash MISO（SPI1，焊死） |
| 91 | PB5 | **扩展 LED** | GPIO 输出（FT 耐 5V） |
| 92 | PB6 | 🔒板载 | QSPI Flash NCS（焊死） |
| 93 | PB7 | 未使用 | 排针引出（原 DCMI_VSYNC），可复用：I2C1_SDA / TIM4_CH2 |
| 94 | BOOT0 | 🔒启动 | 板载 BOOT 按键 |
| 95 | PB8 | **备用 PWM2** | TIM16_CH1（独立定时器，FT 脚）；⚠占用则 I2C1 不可用 |
| 96 | PB9 | **备用 PWM1** | TIM17_CH1（独立定时器，FT 脚）；⚠占用则 I2C1 不可用 |
| 97 | PE0 | **灰度 IO3** | GPIO（FT 耐 5V），数字量/通道控制 |
| 98 | PE1 | **灰度 IO4** | GPIO（FT 耐 5V） |
| 99 | VSS | 电源 | 数字地 |
| 100 | VDD | 电源 | 数字电源 3.3V |

## 四、外设资源占用汇总

| 外设 | 用途 |
|------|------|
| TIM13_CH1 (PA6) + PE7(DIR) | 左电机：有刷 17 kHz PWM+DIR / 无刷电调 50~490 Hz、1~2 ms |
| TIM4_CH3 (PD14) + PE15(DIR) | 右电机：有刷 17 kHz PWM+DIR / 无刷电调 50~490 Hz、1~2 ms |
| TIM14_CH1 (PA7) | 负压电机 PWM（与左右驱动频率互不影响） |
| TIM17_CH1 (PB9) / TIM16_CH1 (PB8) | 备用 PWM ×2（独立定时器，与驱动同级；占用后 I2C1 不可用） |
| TIM1_CH2N (PE10) | ST7735 背光 PWM 调光（TIM1 其余通道空闲） |
| TIM5（编码器模式，PA0/PA1） | 左轮编码器（TTa 脚，限 3.3V） |
| TIM3（编码器模式，PC6/PC7） | 右轮编码器（FT 脚，兼容 5V） |
| ADC1（8 通道扫描+DMA：PC0/PC1/PA4/PC5/PA2/PA3/PB0/PC4） | 电感×4 + 按键×2 + 电池 + 灰度×1 |
| ADC1 备用（PB1/PA5，可加扫描） | 备用 ADC ×2（与电感同级） |
| GPIO ×4（PE0/PE1/PE4/PE5，FT 脚） | 灰度传感器数字量/控制 |
| TIM6（基本定时器，无引脚） | 速度环中断 2ms（NVIC 优先级 0） |
| TIM7（基本定时器，无引脚） | 循迹环中断 5ms（NVIC 优先级 1） |
| USART1 (PA9/PA10) | 调试串口 |
| SPI4 (PE12/PE14) | 板载 ST7735 + 外接屏 + IMU660RB 共用（CS 分离：屏 PE11 / 外接屏 PE9 / IMU PD3，IMU MISO=PE5） |
| 软件 SPI (PD0/PD1/PD4/PD5) | IMU660RB 备选方案（保留） |
| SPI2 (PB13/PB14/PB15 + CS PB12) | WiFi 调试模块（预留） |
| I2C2 (PB10/PB11) | DL1B 激光测距（硬件 I2C） |
| SWD (PA13/PA14) | 下载调试 |

## 五、注意事项

1. **有刷/无刷兼容**：左右驱动均为 PWM+DIR 组合（PA6+PE7、PD14+PE15），两路分属 TIM13/TIM4，频率互不干扰。
   - 有刷电机（PWM+DIR 驱动器，如 DRV8701/BTN）：PWM 设 17 kHz，DIR 控制转向；
   - 无刷电调（标准舵机信号）：PWM 设 50 Hz、1~2 ms 脉宽（部分电调支持 490 Hz / OneShot，改 ARR/PSC 即可），DIR 脚悬空或作电调使能。
2. **5V 容忍（关键）**：右编码器（PC6/PC7）与灰度 4 个 IO（PE0/PE1/PE4/PE5）为 FT 脚，可直接接 5V 器件。⚠ **左编码器（PA0/PA1）为 TTa 脚，只支持 3.3V 供电的编码器**（板载屏 PE11 硬占 CS，TIM1 编码器不可用，无更优解）；若左编码器为 5V 供电，需加电平转换或与右编码器互换位置。所有 ADC 输入脚（PC0/PC1/PC4/PB0/PA2/PA3/PA4/PA5）同为 TTa，**输入不得超过 3.3V**。
3. **双屏方案（板载 ST7735 + 外接屏共用 SPI4）**：SCK/MOSI/DC 共用（PE12/PE14/PE13），片选分离（板载 PE11 / 外接 PE9），外接屏另用 RST=PD9、BLK=PD10。两屏软件互斥、不同时刷新，SPI 带宽与 CPU 开销均无影响。移植时可直接复用核心板 SDK 中 `03-LCD_Test` 的驱动代码。
   - 代价：板载屏 CS 焊死在 PE11，TIM1 编码器不可用，左编码器只能在 TIM5（见第 2 条 5V 容忍说明）；若左编码器必须是 5V 供电且不可接受，则放弃板载屏、左编码器移回 PE9/PE11（TIM1）即可。
4. **WiFi 预留 SPI2**：SCK/MISO/MOSI=PB13/PB14/PB15，CS=PB12，另配 INT(PA15，需关 JTAG)、RST(PD15)。若 WiFi 走串口方案，可用 USART1(PA9/PA10) 或空闲的 UART4(PD0/PD1 现为 IMU 软 SPI，需腾让)/USART2。
5. **IMU660RB 双方案**：
   - 硬件（推荐）：与屏共用 SPI4 总线（SCK=PE12、MOSI=PE14），IMU 独立 MISO=PE5、CS=PD3；与屏靠 CS 互斥。注意屏幕刷新与 IMU 读取不要在同一时刻占用总线（刷屏一般在停车/菜单时，控制环内只读 IMU，无冲突）。
   - 软件（备选）：PD4/PD5/PD0/PD1 软 SPI，与原 STC 方案一致，移植改动最小；不使用硬件方案时 PE5/PD3 可作普通 GPIO。
6. **4 路电感（主传感器）抗干扰设计**：全部集中在 ADC1，单次扫描序列 + DMA 循环搬运 + 硬件过采样，采样时刻一致；4 路电感（PC0/PC1/PA4/PC5）的邻脚全为电源/静态/慢速模拟脚，与 PWM、编码器等高频信号物理隔离（PA5 特意闲置作隔离带）。按键 ADC（PA2/PA3）对噪声不敏感。灰度 ADC（PC4）邻脚为风扇 PWM（PA7），属次要传感器，可通过软件平均抑制。
7. **灰度循迹接口**：PC4 模拟量已加入 ADC1 DMA 扫描序列（与电感同帧采样，时刻一致）；PE0/PE1/PE4/PE5 为 FT 脚，可直接兼容 5V 供电的灰度模块数字输出。
8. **PC2_C/PC3_C 弃用**：ADC3 模拟开关引脚（需配置 SYSCFG_PMCR），存在串扰风险且紧邻电感 PC1，本方案不用，全部模拟通道归 ADC1。
9. **控制环定时器**：TIM6（速度环 2ms）/TIM7（循迹环 5ms）为基本定时器，不占引脚；NVIC 优先级建议：TIM6(0) > TIM7(1) > ADC DMA(2) > WiFi/串口(3) > SysTick(15)。
10. 板载蓝色 LED（PE3）可直接用作运行状态指示灯；K1（PC13）为启动按键，无需外接。
11. 所有功能引脚均位于两侧 2×22 排针，无虚焊板内信号。
