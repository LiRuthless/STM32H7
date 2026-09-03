# SmartCar — STM32H743VIT6 智能车（电磁循迹）CubeMX 工程配置

基于 **WeAct MiniSTM32H7xx 核心板**，引脚分配见 `../引脚分配/STM32H743VIT6_引脚分配表.md`，
算法参考省赛工程 `Sirius20260718`（电磁循迹 / 环岛 / 速度闭环）。

## 使用方法

用 STM32CubeMX（≥6.11）打开 `SmartCar.ioc`，直接 **GENERATE CODE** 即可生成
与官方 SDK 示例（01-GPIO 等）相同结构的工程（`Core/` + `Drivers/` + `MDK-ARM/`，Keil V5）。
首次生成会提示下载 STM32Cube FW_H7 固件包（V1.12.1）。

## 时钟树（与 SDK 03/07 示例一致）

- HSE 25 MHz → PLL1 (M=5, N=96, P=2) → **SYSCLK 240 MHz**
- HCLK = 120 MHz，APB1/2/3/4 = 120 MHz（定时器时钟均为 120 MHz）
- ADC 内核时钟 = PLL2P 83.3 MHz（ADC 内再 4 分频 ≈ 20.8 MHz）
- I-Cache 开启，**D-Cache 关闭**（避免 ADC DMA 缓冲区的 Cache 一致性问题，后期需要可自行打开并做 Cache 维护）

## 外设配置摘要

| 外设 | 配置 | 用途 |
|------|------|------|
| TIM13_CH1 (PA6) | PWM 17 kHz（ARR=7059-1）| 左电机（DIR=PE7）|
| TIM4_CH3 (PD14) | PWM 17 kHz | 右电机（DIR=PE15）|
| TIM14_CH1 (PA7) | PWM 17 kHz | 负压风扇（可在 CubeMX 改 100 Hz~17 kHz）|
| TIM16_CH1 (PB8) / TIM17_CH1 (PB9) | PWM 17 kHz | 备用 PWM |
| TIM1_CH2N (PE10) | PWM 10 kHz，极性 Low | 板载 ST7735 背光 |
| TIM5 (PA0/PA1) | 编码器 TI12 四倍频，输入滤波 6 | 左编码器（⚠TTa 脚，限 3.3V）|
| TIM3 (PC6/PC7) | 编码器 TI12 四倍频，输入滤波 6 | 右编码器（FT 脚，兼容 5V）|
| TIM6 | 2 ms 中断，NVIC 优先级 0 | 速度环 |
| TIM7 | 5 ms 中断，NVIC 优先级 1 | 循迹环 |
| ADC1 | 10 通道扫描 + DMA1_Stream0 循环，16 位，连续模式 | 电感×4→灰度→按键×2→电池→备用×2（按 Rank 顺序）|
| USART1 (PA9/PA10) | 115200 8N1，中断开启（优先级 3）| 调试/无线串口 |
| SPI4 (PE12/PE14/PE5) | 全双工主机 7.5 MHz | ST7735 + IMU660RB 共用（CS 分离：LCD=PE11、外接屏=PE9、IMU=PD3）|
| SPI2 (PB13/14/15) | 全双工主机 15 MHz | WiFi 模块预留（CS=PB12、INT=PA15 下降沿 EXTI、RST=PD15）|
| I2C2 (PB10/PB11) | Fast Mode 400 kHz（Timing=0x50921113）| DL1B 激光测距（XS=PE8）|
| PC13 | 输入，下拉，标签 KEY | 板载 K1 启动按键（电平检测，按下=高）|
| PE3 / PB5 | 输出 | 板载蓝灯 LED_BLUE / 扩展 LED |
| PE0/PE1/PE4/PE6 | 输入（FT 耐 5V）| 灰度传感器数字量（如需作通道控制请在 CubeMX 改为输出）|
| PA15 | EXTI 下降沿，上拉（优先级 4）| WiFi INT 预留 |

## 生成代码后需要在 USER CODE 区补充的调用

```c
HAL_TIM_PWM_Start(&htim13, TIM_CHANNEL_1);      // 左电机
HAL_TIM_PWM_Start(&htim4,  TIM_CHANNEL_3);      // 右电机
HAL_TIM_PWM_Start(&htim14, TIM_CHANNEL_1);      // 负压
HAL_TIM_PWM_Start(&htim1,  TIM_CHANNEL_2);      // 背光（互补通道）
HAL_TIM_Encoder_Start(&htim5, TIM_CHANNEL_ALL); // 左编码器
HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL); // 右编码器
HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_buf, 10);  // adc_buf 建议 uint16_t[10]
HAL_TIM_Base_Start_IT(&htim6);                  // 2ms 速度环
HAL_TIM_Base_Start_IT(&htim7);                  // 5ms 循迹环
```

## 备注

- 编码器计数读取：`int16_t cnt = (int16_t)__HAL_TIM_GET_COUNTER(&htim5);` 然后清零，按 2ms 周期差分即速度。
- ADC DMA 缓冲区为 16 位半字，Rank 顺序见上表；D-Cache 已关，无需 Cache 维护。
- 备用模拟脚 PB1/PA5 已在扫描序列中（Rank 9/10），不用可直接忽略。
- PD0/PD1/PD4/PD5（IMU 软 SPI 备选）未配置，保留为自由 IO。
