# 平台、外设与BSP

现行核对基线8d5ab2f（2026-11-07）。本页保留F26～29父功能和旧链接入口；详细机制、真实参数、错误与源码主说明已按主题拆分。原始采集与物理输出归platform，观测算法归sensing，控制意图归control，安全编排归system，镜像/会话/协议业务归services。拆页不改变F身份，不新增固件验收结论。

<a id="f26"></a>
## F26 ADC编码器PWM
### 板级基础接口

三个子主题各有独立状态/参数和错误语义，当前需求应进入具体主说明：

- [F26.ADC：十rank通道、引脚、DMA、新转换](adc.md#f26-adc)；[灰度原码扫描与完整帧](adc.md#gray-raw)。
- [F26.ENCODER：读即清零、方向与累计边界](encoders.md#f26-encoder)。
- [F26.PWM：通道、频率、CCR和方向电平](pwm-and-direction.md#f26-pwm)。

来源：[Phase02 FR6～10](../../specs/features/02-bsp-platform/requirements.md)。业务速度/里程见[电机执行](../control/motor-execution.md#f08-motor)。

<a id="f27"></a>
## F27 UART失败返回
### 接收与阻塞发送

[UART主说明](uart.md#f27)展开256B环、满时丢旧、重武装与实际len−TxXferCount失败返回。Phase02 FR5“未发数量”冲突仍待决定，未改接口。

<a id="f28"></a>
## F28 DL1B阻塞边界
### 当前测距路径

[DL1B主说明](dl1b.md#f28)展开复位/配置、状态读取、有效与8192无效传播。阻塞HAL与高优先级时基风险保留，10ms不是严格ISR退出保证。

<a id="f29"></a>
## F29 时钟DMA采样与驱动
### 硬件边界与时序

- [时钟、DMA、PRIMASK/DWT、PC13与看门狗](clocks-and-dma.md#f29-clock-dma)。
- [TIM15采样、累计消费及两种编译路径](sampler.md#f29-sampler)。
- [IMU原码与失败发布风险](imu660rb.md#f29-imu)。
- [W25Q64与Flash原语/布局/擦写副作用](w25q64-and-flash.md#f29-flash)。
- [LCD两种屏、格式化、共享SPI4](lcd.md#f29-lcd)。
- [SPI2 WiFi许可/固定DMA/超时回收](spi-wifi.md#f29-wifi)。

### 驱动错误与验证

各子页单独列出失败返回、未检查HAL状态、状态持有者和实机待验范围；[已知问题](../known-issues.md)保留真实冲突。正常路径来自Core→Lifecycle/BSP，独立最小初始化、唯一IRQ与测试证据边界见[F33](isolated-board-tests.md#f33)。厂商组件/生成区只作边界，不把目录中示例存在视为硬件验收通过。

## 本页实现与模块反查

本表只路由主归属，详细符号/机制在对应主题维护；Core生成配置与USER CODE、BSP接管共同决定现行行为。

| 文件 | 主功能 | 详细主说明 |
|---|---|---|
| [App/system/scheduler.c](../../App/system/scheduler.c) | F01 | [实现、状态和参数](../system/runtime-and-safety.md#f01) |
| [BSP/bsp.h](../../BSP/bsp.h) | F29.CLOCK-DMA | [实现、状态和参数](clocks-and-dma.md#f29-clock-dma) |
| [BSP/bsp_adc.c](../../BSP/bsp_adc.c) | F26.ADC | [实现、状态和参数](adc.md#f26-adc) |
| [BSP/bsp_adc.h](../../BSP/bsp_adc.h) | F26.ADC | [实现、状态和参数](adc.md#f26-adc) |
| [BSP/bsp_dl1b.c](../../BSP/bsp_dl1b.c) | F28 | [实现、状态和参数](dl1b.md#f28) |
| [BSP/bsp_dl1b.h](../../BSP/bsp_dl1b.h) | F28 | [实现、状态和参数](dl1b.md#f28) |
| [BSP/bsp_encoder.c](../../BSP/bsp_encoder.c) | F26.ENCODER | [实现、状态和参数](encoders.md#f26-encoder) |
| [BSP/bsp_encoder.h](../../BSP/bsp_encoder.h) | F26.ENCODER | [实现、状态和参数](encoders.md#f26-encoder) |
| [BSP/bsp_gray.c](../../BSP/bsp_gray.c) | 灰度原码 | [实现、状态和参数](adc.md#gray-raw) |
| [BSP/bsp_gray.h](../../BSP/bsp_gray.h) | 灰度原码 | [实现、状态和参数](adc.md#gray-raw) |
| [BSP/bsp_imu660rb.c](../../BSP/bsp_imu660rb.c) | F29.IMU | [实现、状态和参数](imu660rb.md#f29-imu) |
| [BSP/bsp_imu660rb.h](../../BSP/bsp_imu660rb.h) | F29.IMU | [实现、状态和参数](imu660rb.md#f29-imu) |
| [BSP/bsp_lcd.c](../../BSP/bsp_lcd.c) | F29.LCD | [实现、状态和参数](lcd.md#f29-lcd) |
| [BSP/bsp_lcd.h](../../BSP/bsp_lcd.h) | F29.LCD | [实现、状态和参数](lcd.md#f29-lcd) |
| [BSP/bsp_pwm.c](../../BSP/bsp_pwm.c) | F26.PWM | [实现、状态和参数](pwm-and-direction.md#f26-pwm) |
| [BSP/bsp_pwm.h](../../BSP/bsp_pwm.h) | F26.PWM | [实现、状态和参数](pwm-and-direction.md#f26-pwm) |
| [BSP/bsp_runtime.c](../../BSP/bsp_runtime.c) | F29.CLOCK-DMA | [实现、状态和参数](clocks-and-dma.md#f29-clock-dma) |
| [BSP/bsp_runtime.h](../../BSP/bsp_runtime.h) | F29.CLOCK-DMA | [实现、状态和参数](clocks-and-dma.md#f29-clock-dma) |
| [BSP/bsp_sampler.c](../../BSP/bsp_sampler.c) | F29.SAMPLER | [实现、状态和参数](sampler.md#f29-sampler) |
| [BSP/bsp_sampler.h](../../BSP/bsp_sampler.h) | F29.SAMPLER | [实现、状态和参数](sampler.md#f29-sampler) |
| [BSP/bsp_time.c](../../BSP/bsp_time.c) | F29.CLOCK-DMA | [实现、状态和参数](clocks-and-dma.md#f29-clock-dma) |
| [BSP/bsp_time.h](../../BSP/bsp_time.h) | F29.CLOCK-DMA | [实现、状态和参数](clocks-and-dma.md#f29-clock-dma) |
| [BSP/bsp_uart.c](../../BSP/bsp_uart.c) | F27 | [实现、状态和参数](uart.md#f27) |
| [BSP/bsp_uart.h](../../BSP/bsp_uart.h) | F27 | [实现、状态和参数](uart.md#f27) |
| [BSP/bsp_wifi.c](../../BSP/bsp_wifi.c) | F29.WIFI | [实现、状态和参数](spi-wifi.md#f29-wifi) |
| [BSP/bsp_wifi.h](../../BSP/bsp_wifi.h) | F29.WIFI | [实现、状态和参数](spi-wifi.md#f29-wifi) |
| [BSP/lcd/lcd.c](../../BSP/lcd/lcd.c) | F29.LCD | [实现、状态和参数](lcd.md#f29-lcd) |
| [BSP/lcd/lcd.h](../../BSP/lcd/lcd.h) | F29.LCD | [实现、状态和参数](lcd.md#f29-lcd) |
| [Core/Inc/adc.h](../../Core/Inc/adc.h) | F29.CLOCK-DMA | [实现、状态和参数](clocks-and-dma.md#f29-clock-dma) |
| [Core/Inc/dma.h](../../Core/Inc/dma.h) | F29.CLOCK-DMA | [实现、状态和参数](clocks-and-dma.md#f29-clock-dma) |
| [Core/Inc/gpio.h](../../Core/Inc/gpio.h) | F29.CLOCK-DMA | [实现、状态和参数](clocks-and-dma.md#f29-clock-dma) |
| [Core/Inc/i2c.h](../../Core/Inc/i2c.h) | F29.CLOCK-DMA | [实现、状态和参数](clocks-and-dma.md#f29-clock-dma) |
| [Core/Inc/main.h](../../Core/Inc/main.h) | F29.CLOCK-DMA | [实现、状态和参数](clocks-and-dma.md#f29-clock-dma) |
| [Core/Inc/spi.h](../../Core/Inc/spi.h) | F29.CLOCK-DMA | [实现、状态和参数](clocks-and-dma.md#f29-clock-dma) |
| [Core/Inc/stm32h7xx_hal_conf.h](../../Core/Inc/stm32h7xx_hal_conf.h) | F29.CLOCK-DMA | [实现、状态和参数](clocks-and-dma.md#f29-clock-dma) |
| [Core/Inc/stm32h7xx_it.h](../../Core/Inc/stm32h7xx_it.h) | F29.CLOCK-DMA | [实现、状态和参数](clocks-and-dma.md#f29-clock-dma) |
| [Core/Inc/tim.h](../../Core/Inc/tim.h) | F29.CLOCK-DMA | [实现、状态和参数](clocks-and-dma.md#f29-clock-dma) |
| [Core/Inc/usart.h](../../Core/Inc/usart.h) | F29.CLOCK-DMA | [实现、状态和参数](clocks-and-dma.md#f29-clock-dma) |
| [Core/Src/adc.c](../../Core/Src/adc.c) | F26.ADC | [实现、状态和参数](adc.md#f26-adc) |
| [Core/Src/dma.c](../../Core/Src/dma.c) | F29.CLOCK-DMA | [实现、状态和参数](clocks-and-dma.md#f29-clock-dma) |
| [Core/Src/gpio.c](../../Core/Src/gpio.c) | F29.CLOCK-DMA | [实现、状态和参数](clocks-and-dma.md#f29-clock-dma) |
| [Core/Src/i2c.c](../../Core/Src/i2c.c) | F28 | [实现、状态和参数](dl1b.md#f28) |
| [Core/Src/spi.c](../../Core/Src/spi.c) | F29.CLOCK-DMA | [实现、状态和参数](clocks-and-dma.md#f29-clock-dma) |
| [Core/Src/stm32h7xx_hal_msp.c](../../Core/Src/stm32h7xx_hal_msp.c) | F29.CLOCK-DMA | [实现、状态和参数](clocks-and-dma.md#f29-clock-dma) |
| [Core/Src/system_stm32h7xx.c](../../Core/Src/system_stm32h7xx.c) | F29.CLOCK-DMA | [实现、状态和参数](clocks-and-dma.md#f29-clock-dma) |
| [Core/Src/tim.c](../../Core/Src/tim.c) | F29.CLOCK-DMA | [实现、状态和参数](clocks-and-dma.md#f29-clock-dma) |
| [Core/Src/usart.c](../../Core/Src/usart.c) | F27 | [实现、状态和参数](uart.md#f27) |
| [BSP/bsp_flash.c](../../BSP/bsp_flash.c) | F29.FLASH | [实现、状态和参数](w25q64-and-flash.md#f29-flash) |
| [BSP/bsp_flash.h](../../BSP/bsp_flash.h) | F29.FLASH | [实现、状态和参数](w25q64-and-flash.md#f29-flash) |
| [BSP/bsp_w25q64.c](../../BSP/bsp_w25q64.c) | F29.FLASH | [实现、状态和参数](w25q64-and-flash.md#f29-flash) |
| [BSP/bsp_w25q64.h](../../BSP/bsp_w25q64.h) | F29.FLASH | [实现、状态和参数](w25q64-and-flash.md#f29-flash) |
| [BSP/bsp_key.c](../../BSP/bsp_key.c) | PC13 | [实现、状态和参数](clocks-and-dma.md#keys) |
| [BSP/bsp_key.h](../../BSP/bsp_key.h) | PC13 | [实现、状态和参数](clocks-and-dma.md#keys) |
| [BSP/bsp_wdt.c](../../BSP/bsp_wdt.c) | IWDG | [实现、状态和参数](clocks-and-dma.md#watchdog) |
| [BSP/bsp_wdt.h](../../BSP/bsp_wdt.h) | IWDG | [实现、状态和参数](clocks-and-dma.md#watchdog) |

板测BSP专用胶水bsp_board_test与App/system/board_test的主说明见[F33](isolated-board-tests.md#f33)，不作为普通驱动的另一份主契约。
