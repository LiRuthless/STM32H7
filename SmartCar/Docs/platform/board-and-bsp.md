# 平台、外设与BSP

现行功能说明，核对基线 `68cb508`；2026-10-31迁移。需求经[功能索引](../traceability.md)进入本页，再由实现表定位源码。
当前行为不等于全部已批准/实机验收，冲突见[已知问题](../known-issues.md)。

<a id="f26"></a>
## F26 ADC编码器PWM
### 板级基础接口

ADC1持续十rank DMA：枚举下标须与IOC rank一致，16bit原始右移4转12bit；DMA缓冲D2 SRAM1。普通读取不保证每次都是新转换，灰度fresh接口单独处理。
编码器读即清零，方向宏左反向1、右0按接线约定；TIM15保留1ms窗口和供TIM6消费的2ms累计，同级不抢占前提不变。
PWM外部0～10000，内部截断后换CCR，方向决策App、GPIO物理写BSP。电机与风扇通道映射由BSP表决定，不在App直写寄存器。
校验枚举/rank、引脚、读清/累计、边界和DIR极性；板上空转/测波形验证方向和占空比。单独有编译入口或厂商示例不代表当前SmartCar硬件模块已测试通过。

**需求来源：** [P02 FR-06, FR-07, FR-08, FR-09, FR-10](../../specs/features/02-bsp-platform/requirements.md)；[P03 FR-07](../../specs/features/03-track-following/requirements.md)。实现入口：[BSP/bsp_adc.c](../../BSP/bsp_adc.c)、[BSP/bsp_encoder.c](../../BSP/bsp_encoder.c)、[BSP/bsp_pwm.c](../../BSP/bsp_pwm.c)。

<a id="f27"></a>
## F27 UART失败返回
### 接收与阻塞发送

USART1 115200，单字节中断接收、256B环形缓冲，满时丢最旧字节；错误回调清ORE/FE/NE/PE并重新武装。前台消费，不能把一次Read结果当协议帧边界。
`BSP_UART_Write(buf,len)`空/0直接0；HAL阻塞超时参数100ms，成功0；失败返回`len - huart1.TxXferCount`（已发数量折算）。P02 FR5要求“未发送数量、0全发完”，与当前失败语义相反，且开始前失败可能返回0，D01未决定。C/H如实写公式不构成批准该接口。
审查所有调用方是否忽略返回；未来须分别验证未开始、部分发送、成功及并发发送上下文。本轮不改返回值，不把硬件串口无异常等同所有错误路径正确。

**需求来源：** [P02 FR-03, FR-04, FR-05](../../specs/features/02-bsp-platform/requirements.md)。实现入口：[BSP/bsp_uart.c](../../BSP/bsp_uart.c)。

<a id="f28"></a>
## F28 DL1B阻塞边界
### 当前测距路径

DL1B I2C2/PB10 PB11，XS=PE8，初始化复位高50ms/低10ms/高50ms，再固件检查和135B配置。Update在TIM6中调用，状态就绪时清中断、查范围状态及2B距离；0～4000mm有效，其余发布8192。
寄存器接口实际为阻塞HAL_I2C_Mem_Read/Write，传10ms超时，退出取决于HAL时基是否推进。P02“非阻塞轮询”与代码冲突；10ms参数不能视为高优先级ISR中的严格退出上界。保护发生在后续900us检查，不保证被阻塞时该检查有机会运行。
D02/R02未决定重新设计采集还是明确保留风险；不声称所有坏读必卡死。距离<100仅用于抑制右环，不代表CASK完整功能。
审查调用上下文和等待依赖，实测正常/异常I2C、总预算与故障传播；现有距离显示/宿主替身不能证明有界。

**需求来源：** [P02 FR-16, FR-17](../../specs/features/02-bsp-platform/requirements.md)。实现入口：[BSP/bsp_dl1b.c](../../BSP/bsp_dl1b.c)、[App/system/scheduler.c](../../App/system/scheduler.c)。

<a id="f29"></a>
## F29 时钟DMA采样与驱动
### 硬件边界与时序

App只调用公开BSP，BSP可依赖HAL/CubeMX；采样器钩子和runtime HAL回调是两个明确固定分派边，不再称“唯一一处”。Core提供main/外设/IRQ，HAL_TIM_PeriodElapsedCallback唯一强实现归bsp_runtime。
480MHz目标、I-Cache开/D-Cache关。ADC DMA在0x30000000(D2)；SPI2 TX/RX在AXI 0x2407F000/0x2407F400，不能把“所有新增DMA必须D2”当通用约束；DMA1不可访问DTCM。
SPI4/5由PLL3Q选择，main USER CODE补丁及全部MX初始化后的BSP恢复调用都保留。IOC只管托管外设，TIM15、SPI1/Flash、IWDG代码接管不在IOC独立表达。
SPI2 Mode0/8bit/MSB/软件CS，PLL1Q480MHz/256=1.875MHz；BSP有/64=7.5MHz接口，当前活动链未调用。READY低一次许可只启动一笔1024B DMA，超时10ms回收，晚回调/边沿状态需按原所有权处理。DMA/SPI优先级3，不能把通信等待带入TIM6。

### 驱动错误与验证

IMU660RB SPI4、±2000dps/±8g、ODR1.66kHz，WHO_AM_I0x6B；名义ODR不保证读取到每个独立新样本。初始化能返回失败并继续，但读函数仍有R03发布无效/未初始化数组风险。
W25Q64 JEDEC0xEF4017、8MB、SPI1目标30MHz；BSP LCD基于ST7735与胶水，转置字模及低有效背光约定不能随文档改变。厂商组件/生成区是边界，不纳入全量自维护整改。
核对初始化/回调/优先级/内存/时钟来源，既有spi_wifi_state覆盖许可/超时等软件注入；真实SCK、SPI共享、电气、超时和吞吐待验。

**需求来源：** [P01 FR-02](../../specs/features/01-runtime-architecture/requirements.md)；[P02 FR-01, FR-02, FR-12, FR-13, FR-14](../../specs/features/02-bsp-platform/requirements.md)；[P11 FR-09, FR-10](../../specs/features/11-hardening-cleanup/requirements.md)；[P12 FR-07](../../specs/features/12-control-safety-closure/requirements.md)；[P14 FR-08](../../specs/features/14-dual-mode-tracking/requirements.md)；[P16 FR-01, FR-02, FR-03, FR-04](../../specs/features/16-spi-wifi-link/requirements.md)。实现入口：[BSP/bsp_runtime.c](../../BSP/bsp_runtime.c)、[BSP/bsp_time.c](../../BSP/bsp_time.c)、[BSP/bsp_sampler.c](../../BSP/bsp_sampler.c)、[BSP/bsp_wifi.c](../../BSP/bsp_wifi.c)、[BSP/bsp_imu660rb.c](../../BSP/bsp_imu660rb.c)、[BSP/bsp_lcd.c](../../BSP/bsp_lcd.c)、[BSP/lcd/lcd.c](../../BSP/lcd/lcd.c)。

## 本页实现与模块反查

下表是当前源码定位；完整符号表仅在模块主功能页维护，协作行转向主条目。公共/私有辅助函数归所属功能，不把声明视为另一实现。Core生成部分只作边界，用户区按原标记维护。

| 文件 | 主功能 | 定义/配置定位 |
|---|---|---|
| [App/system/scheduler.c](../../App/system/scheduler.c) | F01 | 协作模块；完整符号表见[F01主条目](../system/runtime-and-safety.md#f01) |
| [BSP/bsp.h](../../BSP/bsp.h) | F29 | 类型/布局/配置；关联同模块C定义 |
| [BSP/bsp_adc.c](../../BSP/bsp_adc.c) | F26 | `BSP_ADC_Init`、`BSP_ADC_Read`、`BSP_ADC_ReadGrayFresh` |
| [BSP/bsp_adc.h](../../BSP/bsp_adc.h) | F26 | 类型/布局/配置；关联同模块C定义 |
| [BSP/bsp_dl1b.c](../../BSP/bsp_dl1b.c) | F28 | `BSP_DL1B_ReadRegister`、`BSP_DL1B_WriteRegister`、`BSP_DL1B_Init`、`BSP_DL1B_Update`、`BSP_DL1B_GetDistanceMm` |
| [BSP/bsp_dl1b.h](../../BSP/bsp_dl1b.h) | F28 | 类型/布局/配置；关联同模块C定义 |
| [BSP/bsp_encoder.c](../../BSP/bsp_encoder.c) | F26 | `BSP_Encoder_Init`、`BSP_Encoder_GetLeft`、`BSP_Encoder_GetRight`、`BSP_Encoder_ReadClear` |
| [BSP/bsp_encoder.h](../../BSP/bsp_encoder.h) | F26 | 类型/布局/配置；关联同模块C定义 |
| [BSP/bsp_gray.c](../../BSP/bsp_gray.c) | F29 | `BSP_Gray_SelectChannel`、`BSP_Gray_ResetSequence`、`BSP_Gray_Init`、`BSP_Gray_ReadFrame` |
| [BSP/bsp_gray.h](../../BSP/bsp_gray.h) | F29 | 类型/布局/配置；关联同模块C定义 |
| [BSP/bsp_imu660rb.c](../../BSP/bsp_imu660rb.c) | F29 | `BSP_IMU660RB_SetChipSelect`、`BSP_IMU660RB_WriteRegister`、`BSP_IMU660RB_ReadRegister`、`BSP_IMU660RB_ReadRegisters`、`BSP_IMU660RB_CheckIdentity`、`BSP_IMU660RB_Init`、`BSP_IMU660RB_GetAcc`、`BSP_IMU660RB_GetGyro` |
| [BSP/bsp_imu660rb.h](../../BSP/bsp_imu660rb.h) | F29 | 类型/布局/配置；关联同模块C定义 |
| [BSP/bsp_lcd.c](../../BSP/bsp_lcd.c) | F29 | `BSP_LCD_Init`、`BSP_LCD_Clear`、`BSP_LCD_SetBacklight`、`BSP_LCD_ShowChar`、`BSP_LCD_ShowString`、`BSP_LCD_ShowInt`、`BSP_LCD_ShowFloat` |
| [BSP/bsp_lcd.h](../../BSP/bsp_lcd.h) | F29 | 类型/布局/配置；关联同模块C定义 |
| [BSP/bsp_pwm.c](../../BSP/bsp_pwm.c) | F26 | `BSP_PWM_Init`、`BSP_PWM_SetDuty`、`BSP_PWM_SetDirection` |
| [BSP/bsp_pwm.h](../../BSP/bsp_pwm.h) | F26 | 类型/布局/配置；关联同模块C定义 |
| [BSP/bsp_runtime.c](../../BSP/bsp_runtime.c) | F29 | `BSP_Runtime_ConfigureSpiClock`、`BSP_Runtime_StartTimers`、`BSP_Runtime_StopTimers`、`BSP_Runtime_SetRunLed`、`BSP_Runtime_ToggleLed`、`HAL_TIM_PeriodElapsedCallback` |
| [BSP/bsp_runtime.h](../../BSP/bsp_runtime.h) | F29 | 类型/布局/配置；关联同模块C定义 |
| [BSP/bsp_sampler.c](../../BSP/bsp_sampler.c) | F29 | `BSP_Sampler_Init`、`BSP_Sampler_Start`、`BSP_Sampler_Stop`、`BSP_Sampler_GetTick`、`BSP_Sampler_GetEncL_1ms`、`BSP_Sampler_GetEncR_1ms`、`BSP_Sampler_GetGyroRaw`、`BSP_Sampler_GetAccRaw`、`BSP_Sampler_ConsumeEncL`、`BSP_Sampler_ConsumeEncR`、`TIM15_IRQHandler` |
| [BSP/bsp_sampler.h](../../BSP/bsp_sampler.h) | F29 | 类型/布局/配置；关联同模块C定义 |
| [BSP/bsp_time.c](../../BSP/bsp_time.c) | F29 | `BSP_Time_Init`、`BSP_Time_GetCycles`、`BSP_Time_EnterCritical`、`BSP_Time_ExitCritical`、`BSP_Time_MemoryBarrier`、`BSP_Time_GetTick` |
| [BSP/bsp_time.h](../../BSP/bsp_time.h) | F29 | 类型/布局/配置；关联同模块C定义 |
| [BSP/bsp_uart.c](../../BSP/bsp_uart.c) | F27 | `BSP_UART_Init`、`BSP_UART_Write`、`BSP_UART_WriteString`、`BSP_UART_Read`、`BSP_UART_RxAvailable`、`BSP_UART_RxFlush`、`BSP_UART_PushRx`、`HAL_UART_RxCpltCallback`、`HAL_UART_ErrorCallback`、`fputc` |
| [BSP/bsp_uart.h](../../BSP/bsp_uart.h) | F27 | 类型/布局/配置；关联同模块C定义 |
| [BSP/bsp_wifi.c](../../BSP/bsp_wifi.c) | F29 | `BSP_WiFi_SetCs`、`BSP_WiFi_Init`、`BSP_WiFi_ReleaseRx`、`BSP_WiFi_TryExchange`、`BSP_WiFi_Service`、`BSP_WiFi_GetStatus`、`BSP_WiFi_SetFastClock`、`HAL_GPIO_EXTI_Callback`、`HAL_SPI_TxRxCpltCallback`、`HAL_SPI_ErrorCallback` |
| [BSP/bsp_wifi.h](../../BSP/bsp_wifi.h) | F29 | 类型/布局/配置；关联同模块C定义 |
| [BSP/lcd/lcd.c](../../BSP/lcd/lcd.c) | F29 | `BSP_LCDIO_Init`、`BSP_LCDIO_SetReset`、`BSP_LCDIO_ClearReset`、`BSP_LCDIO_InitBus`、`BSP_LCDIO_GetTick`、`BSP_LCDIO_WriteRegister`、`BSP_LCDIO_ReadRegister`、`BSP_LCDIO_SendData`、`BSP_LCDIO_ReceiveData` |
| [BSP/lcd/lcd.h](../../BSP/lcd/lcd.h) | F29 | 类型/布局/配置；关联同模块C定义 |
| [Core/Inc/adc.h](../../Core/Inc/adc.h) | F29 | 类型/布局/配置；关联同模块C定义 |
| [Core/Inc/dma.h](../../Core/Inc/dma.h) | F29 | 类型/布局/配置；关联同模块C定义 |
| [Core/Inc/gpio.h](../../Core/Inc/gpio.h) | F29 | 类型/布局/配置；关联同模块C定义 |
| [Core/Inc/i2c.h](../../Core/Inc/i2c.h) | F29 | 类型/布局/配置；关联同模块C定义 |
| [Core/Inc/main.h](../../Core/Inc/main.h) | F29 | 类型/布局/配置；关联同模块C定义 |
| [Core/Inc/spi.h](../../Core/Inc/spi.h) | F29 | 类型/布局/配置；关联同模块C定义 |
| [Core/Inc/stm32h7xx_hal_conf.h](../../Core/Inc/stm32h7xx_hal_conf.h) | F29 | 类型/布局/配置；关联同模块C定义 |
| [Core/Inc/stm32h7xx_it.h](../../Core/Inc/stm32h7xx_it.h) | F29 | 类型/布局/配置；关联同模块C定义 |
| [Core/Inc/tim.h](../../Core/Inc/tim.h) | F29 | 类型/布局/配置；关联同模块C定义 |
| [Core/Inc/usart.h](../../Core/Inc/usart.h) | F29 | 类型/布局/配置；关联同模块C定义 |
| [Core/Src/adc.c](../../Core/Src/adc.c) | F29 | `MX_ADC1_Init`、`HAL_ADC_MspInit`、`HAL_ADC_MspDeInit` |
| [Core/Src/dma.c](../../Core/Src/dma.c) | F29 | `MX_DMA_Init` |
| [Core/Src/gpio.c](../../Core/Src/gpio.c) | F29 | `MX_GPIO_Init` |
| [Core/Src/i2c.c](../../Core/Src/i2c.c) | F29 | `MX_I2C2_Init`、`HAL_I2C_MspInit`、`HAL_I2C_MspDeInit` |
| [Core/Src/spi.c](../../Core/Src/spi.c) | F29 | `MX_SPI2_Init`、`MX_SPI4_Init`、`HAL_SPI_MspInit`、`HAL_SPI_MspDeInit` |
| [Core/Src/stm32h7xx_hal_msp.c](../../Core/Src/stm32h7xx_hal_msp.c) | F29 | `HAL_MspInit` |
| [Core/Src/system_stm32h7xx.c](../../Core/Src/system_stm32h7xx.c) | F29 | `SystemInit`、`SystemCoreClockUpdate` |
| [Core/Src/tim.c](../../Core/Src/tim.c) | F29 | `MX_TIM1_Init`、`MX_TIM3_Init`、`MX_TIM4_Init`、`MX_TIM5_Init`、`MX_TIM6_Init`、`MX_TIM7_Init`、`MX_TIM13_Init`、`MX_TIM14_Init`、`MX_TIM16_Init`、`MX_TIM17_Init`、`HAL_TIM_PWM_MspInit`、`HAL_TIM_Encoder_MspInit`、`HAL_TIM_Base_MspInit`、`HAL_TIM_MspPostInit`、`HAL_TIM_PWM_MspDeInit`、`HAL_TIM_Encoder_MspDeInit`、`HAL_TIM_Base_MspDeInit` |
| [Core/Src/usart.c](../../Core/Src/usart.c) | F29 | `MX_USART1_UART_Init`、`HAL_UART_MspInit`、`HAL_UART_MspDeInit` |
