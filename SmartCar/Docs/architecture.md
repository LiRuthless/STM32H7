# 功能与模块架构

Phase定义动机/任务/验收，docs拥有现行功能/实现契约，代码实现该契约。功能索引见[traceability](traceability.md)。

Core启动/IRQ→BSP固定分发→App薄入口→system编排；sensing提供输入，control观察/执行，services参数/日志/通信，hmi交互。无RTOS、堆或通用事件总线。

## 状态与依赖

- lifecycle起停与模式；safety首因/pending；battery与各传感器私有历史。
- tracking完整拍及监视量；两FSM独立历史；pid增益可见输出、各控制器历史；motor反馈/执行。
- bank/store镜像不反调使用者；system应用协调；tuning纯业务不依赖SCB。
- run_record快照、logger队列/会话、log_store唯一Flash后端、log_export互斥缓存。
- link持协议会话，dispatch业务适配，telemetry独立上传；menu/keys/render/dashboard各持所属状态。
- TIM15/TIM6同级不抢占，队列写完再发布；需要事务用原复制/短锁，const地址不是原子快照。

## 模块到主功能页

每行登记真实物理C/H及主功能；到功能页再看具体函数/状态/错误路径。Core整文件登记为生成/用户区边界，不表示生成区属于自维护。厂商ST7735/font/Drivers作为第三方边界，不列入自维护完整性。

| 模块文件 | 主条目 | 边界 |
|---|---|---|
| `App/app.c` | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 自维护 |
| `App/app.h` | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 自维护 |
| `App/app_config.h` | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 自维护 |
| `App/app_types.h` | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 自维护 |
| `App/control/angle_controller.c` | [F08 方向角度速度与执行](control/tracking-and-controllers.md#f08) | 自维护 |
| `App/control/angle_controller.h` | [F08 方向角度速度与执行](control/tracking-and-controllers.md#f08) | 自维护 |
| `App/control/control_profile.c` | [F12 部分配置与全局调参规划](control/tracking-and-controllers.md#f12) | 自维护 |
| `App/control/control_profile.h` | [F12 部分配置与全局调参规划](control/tracking-and-controllers.md#f12) | 自维护 |
| `App/control/direction_controller.c` | [F08 方向角度速度与执行](control/tracking-and-controllers.md#f08) | 自维护 |
| `App/control/direction_controller.h` | [F08 方向角度速度与执行](control/tracking-and-controllers.md#f08) | 自维护 |
| `App/control/electromagnetic_fsm.c` | [F09 活动电磁元素](control/elements-and-dormant-code.md#f09) | 自维护 |
| `App/control/electromagnetic_fsm.h` | [F09 活动电磁元素](control/elements-and-dormant-code.md#f09) | 自维护 |
| `App/control/legacy_control.c` | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 自维护 |
| `App/control/legacy_control.h` | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 自维护 |
| `App/control/legacy_element.c` | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 自维护 |
| `App/control/legacy_element.h` | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 自维护 |
| `App/control/legacy_pid.c` | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 自维护 |
| `App/control/legacy_pid.h` | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 自维护 |
| `App/control/legacy_roundabout.c` | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 自维护 |
| `App/control/legacy_roundabout.h` | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 自维护 |
| `App/control/motor.c` | [F08 方向角度速度与执行](control/tracking-and-controllers.md#f08) | 自维护 |
| `App/control/motor.h` | [F08 方向角度速度与执行](control/tracking-and-controllers.md#f08) | 自维护 |
| `App/control/optical_fsm.c` | [F10 光电状态机](control/tracking-and-controllers.md#f10) | 自维护 |
| `App/control/optical_fsm.h` | [F10 光电状态机](control/tracking-and-controllers.md#f10) | 自维护 |
| `App/control/pid.c` | [F08 方向角度速度与执行](control/tracking-and-controllers.md#f08) | 自维护 |
| `App/control/pid.h` | [F08 方向角度速度与执行](control/tracking-and-controllers.md#f08) | 自维护 |
| `App/control/speed_controller.c` | [F08 方向角度速度与执行](control/tracking-and-controllers.md#f08) | 自维护 |
| `App/control/speed_controller.h` | [F08 方向角度速度与执行](control/tracking-and-controllers.md#f08) | 自维护 |
| `App/control/tracking.c` | [F11 双FSM与唯一执行](control/tracking-and-controllers.md#f11) | 自维护 |
| `App/control/tracking.h` | [F11 双FSM与唯一执行](control/tracking-and-controllers.md#f11) | 自维护 |
| `App/control/tuning.c` | [F18 完整拍临时调参](services/parameters-and-tuning.md#f18) | 自维护 |
| `App/control/tuning.h` | [F18 完整拍临时调参](services/parameters-and-tuning.md#f18) | 自维护 |
| `App/control/tuning_values.c` | [F18 完整拍临时调参](services/parameters-and-tuning.md#f18) | 自维护 |
| `App/control/tuning_values.h` | [F18 完整拍临时调参](services/parameters-and-tuning.md#f18) | 自维护 |
| `App/hmi/dashboard.c` | [F14 菜单按键与显示](hmi/operation.md#f14) | 自维护 |
| `App/hmi/dashboard.h` | [F14 菜单按键与显示](hmi/operation.md#f14) | 自维护 |
| `App/hmi/menu.c` | [F14 菜单按键与显示](hmi/operation.md#f14) | 自维护 |
| `App/hmi/menu.h` | [F14 菜单按键与显示](hmi/operation.md#f14) | 自维护 |
| `App/hmi/menu_keys.c` | [F14 菜单按键与显示](hmi/operation.md#f14) | 自维护 |
| `App/hmi/menu_keys.h` | [F14 菜单按键与显示](hmi/operation.md#f14) | 自维护 |
| `App/hmi/menu_render.c` | [F14 菜单按键与显示](hmi/operation.md#f14) | 自维护 |
| `App/hmi/menu_render.h` | [F14 菜单按键与显示](hmi/operation.md#f14) | 自维护 |
| `App/sensing/battery.c` | [F03 低压持续判定](system/runtime-and-safety.md#f03) | 自维护 |
| `App/sensing/battery.h` | [F03 低压持续判定](system/runtime-and-safety.md#f03) | 自维护 |
| `App/sensing/filter.c` | [F05 电感偏差与对称度](sensing/sensors-and-calibration.md#f05) | 自维护 |
| `App/sensing/filter.h` | [F05 电感偏差与对称度](sensing/sensors-and-calibration.md#f05) | 自维护 |
| `App/sensing/gray_calibration.h` | [F06 灰度采集标定与质心](sensing/sensors-and-calibration.md#f06) | 自维护 |
| `App/sensing/gray_sensor.c` | [F06 灰度采集标定与质心](sensing/sensors-and-calibration.md#f06) | 自维护 |
| `App/sensing/gray_sensor.h` | [F06 灰度采集标定与质心](sensing/sensors-and-calibration.md#f06) | 自维护 |
| `App/sensing/imu_proc.c` | [F07 IMU校准滤波与积分](sensing/sensors-and-calibration.md#f07) | 自维护 |
| `App/sensing/imu_proc.h` | [F07 IMU校准滤波与积分](sensing/sensors-and-calibration.md#f07) | 自维护 |
| `App/sensing/legacy_imu.c` | [F07 IMU校准滤波与积分](sensing/sensors-and-calibration.md#f07) | 自维护 |
| `App/sensing/legacy_imu.h` | [F07 IMU校准滤波与积分](sensing/sensors-and-calibration.md#f07) | 自维护 |
| `App/sensing/track_sensor.c` | [F05 电感偏差与对称度](sensing/sensors-and-calibration.md#f05) | 自维护 |
| `App/sensing/track_sensor.h` | [F05 电感偏差与对称度](sensing/sensors-and-calibration.md#f05) | 自维护 |
| `App/services/checksum.c` | [F23 SCB帧JSON与命令](services/communication.md#f23) | 自维护 |
| `App/services/checksum.h` | [F23 SCB帧JSON与命令](services/communication.md#f23) | 自维护 |
| `App/services/command_dispatch.c` | [F23 SCB帧JSON与命令](services/communication.md#f23) | 自维护 |
| `App/services/command_dispatch.h` | [F23 SCB帧JSON与命令](services/communication.md#f23) | 自维护 |
| `App/services/datalog.h` | [F19 日志格式容量](services/recording-and-export.md#f19) | 自维护 |
| `App/services/log_export.c` | [F22 二进制流式导出](services/recording-and-export.md#f22) | 自维护 |
| `App/services/log_export.h` | [F22 二进制流式导出](services/recording-and-export.md#f22) | 自维护 |
| `App/services/log_store.c` | [F20 队列前台写入与故障](services/recording-and-export.md#f20) | 自维护 |
| `App/services/log_store.h` | [F20 队列前台写入与故障](services/recording-and-export.md#f20) | 自维护 |
| `App/services/logger.c` | [F20 队列前台写入与故障](services/recording-and-export.md#f20) | 自维护 |
| `App/services/logger.h` | [F20 队列前台写入与故障](services/recording-and-export.md#f20) | 自维护 |
| `App/services/parameter_bank.c` | [F16 参数v3格式](services/parameters-and-tuning.md#f16) | 自维护 |
| `App/services/parameter_bank.h` | [F16 参数v3格式](services/parameters-and-tuning.md#f16) | 自维护 |
| `App/services/parameter_image.h` | [F16 参数v3格式](services/parameters-and-tuning.md#f16) | 自维护 |
| `App/services/parameter_store.c` | [F17 迁移保存与失败](services/parameters-and-tuning.md#f17) | 自维护 |
| `App/services/parameter_store.h` | [F17 迁移保存与失败](services/parameters-and-tuning.md#f17) | 自维护 |
| `App/services/run_record.c` | [F19 日志格式容量](services/recording-and-export.md#f19) | 自维护 |
| `App/services/run_record.h` | [F19 日志格式容量](services/recording-and-export.md#f19) | 自维护 |
| `App/services/scb_command_codec.c` | [F23 SCB帧JSON与命令](services/communication.md#f23) | 自维护 |
| `App/services/scb_command_codec.h` | [F23 SCB帧JSON与命令](services/communication.md#f23) | 自维护 |
| `App/services/scb_frame.c` | [F23 SCB帧JSON与命令](services/communication.md#f23) | 自维护 |
| `App/services/scb_frame.h` | [F23 SCB帧JSON与命令](services/communication.md#f23) | 自维护 |
| `App/services/scb_json.c` | [F23 SCB帧JSON与命令](services/communication.md#f23) | 自维护 |
| `App/services/scb_json.h` | [F23 SCB帧JSON与命令](services/communication.md#f23) | 自维护 |
| `App/services/scb_link.c` | [F24 会话可靠传输与回绕](services/communication.md#f24) | 自维护 |
| `App/services/scb_link.h` | [F24 会话可靠传输与回绕](services/communication.md#f24) | 自维护 |
| `App/services/telemetry.c` | [F25 独立上传](services/communication.md#f25) | 自维护 |
| `App/services/telemetry.h` | [F25 独立上传](services/communication.md#f25) | 自维护 |
| `App/services/wireless.c` | [F15 多入口参数应用](services/parameters-and-tuning.md#f15) | 自维护 |
| `App/services/wireless.h` | [F15 多入口参数应用](services/parameters-and-tuning.md#f15) | 自维护 |
| `App/system/lifecycle.c` | [F02 起停、模式与首因](system/runtime-and-safety.md#f02) | 自维护 |
| `App/system/lifecycle.h` | [F02 起停、模式与首因](system/runtime-and-safety.md#f02) | 自维护 |
| `App/system/parameter_application.c` | [F15 多入口参数应用](services/parameters-and-tuning.md#f15) | 自维护 |
| `App/system/parameter_application.h` | [F15 多入口参数应用](services/parameters-and-tuning.md#f15) | 自维护 |
| `App/system/safety.c` | [F02 起停、模式与首因](system/runtime-and-safety.md#f02) | 自维护 |
| `App/system/safety.h` | [F02 起停、模式与首因](system/runtime-and-safety.md#f02) | 自维护 |
| `App/system/scheduler.c` | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 自维护 |
| `App/system/scheduler.h` | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 自维护 |
| `App/system/tuning_application.c` | [F18 完整拍临时调参](services/parameters-and-tuning.md#f18) | 自维护 |
| `App/system/tuning_application.h` | [F18 完整拍临时调参](services/parameters-and-tuning.md#f18) | 自维护 |
| `BSP/bsp.h` | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 自维护 |
| `BSP/bsp_adc.c` | [F26 ADC编码器PWM](platform/board-and-bsp.md#f26) | 自维护 |
| `BSP/bsp_adc.h` | [F26 ADC编码器PWM](platform/board-and-bsp.md#f26) | 自维护 |
| `BSP/bsp_dl1b.c` | [F28 DL1B阻塞边界](platform/board-and-bsp.md#f28) | 自维护 |
| `BSP/bsp_dl1b.h` | [F28 DL1B阻塞边界](platform/board-and-bsp.md#f28) | 自维护 |
| `BSP/bsp_encoder.c` | [F26 ADC编码器PWM](platform/board-and-bsp.md#f26) | 自维护 |
| `BSP/bsp_encoder.h` | [F26 ADC编码器PWM](platform/board-and-bsp.md#f26) | 自维护 |
| `BSP/bsp_flash.c` | [F17 迁移保存与失败](services/parameters-and-tuning.md#f17) | 自维护 |
| `BSP/bsp_flash.h` | [F17 迁移保存与失败](services/parameters-and-tuning.md#f17) | 自维护 |
| `BSP/bsp_gray.c` | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 自维护 |
| `BSP/bsp_gray.h` | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 自维护 |
| `BSP/bsp_imu660rb.c` | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 自维护 |
| `BSP/bsp_imu660rb.h` | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 自维护 |
| `BSP/bsp_key.c` | [F14 菜单按键与显示](hmi/operation.md#f14) | 自维护 |
| `BSP/bsp_key.h` | [F14 菜单按键与显示](hmi/operation.md#f14) | 自维护 |
| `BSP/bsp_lcd.c` | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 自维护 |
| `BSP/bsp_lcd.h` | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 自维护 |
| `BSP/bsp_pwm.c` | [F26 ADC编码器PWM](platform/board-and-bsp.md#f26) | 自维护 |
| `BSP/bsp_pwm.h` | [F26 ADC编码器PWM](platform/board-and-bsp.md#f26) | 自维护 |
| `BSP/bsp_runtime.c` | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 自维护 |
| `BSP/bsp_runtime.h` | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 自维护 |
| `BSP/bsp_sampler.c` | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 自维护 |
| `BSP/bsp_sampler.h` | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 自维护 |
| `BSP/bsp_time.c` | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 自维护 |
| `BSP/bsp_time.h` | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 自维护 |
| `BSP/bsp_uart.c` | [F27 UART失败返回](platform/board-and-bsp.md#f27) | 自维护 |
| `BSP/bsp_uart.h` | [F27 UART失败返回](platform/board-and-bsp.md#f27) | 自维护 |
| `BSP/bsp_w25q64.c` | [F17 迁移保存与失败](services/parameters-and-tuning.md#f17) | 自维护 |
| `BSP/bsp_w25q64.h` | [F17 迁移保存与失败](services/parameters-and-tuning.md#f17) | 自维护 |
| `BSP/bsp_wdt.c` | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 自维护 |
| `BSP/bsp_wdt.h` | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 自维护 |
| `BSP/bsp_wifi.c` | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 自维护 |
| `BSP/bsp_wifi.h` | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 自维护 |
| `BSP/lcd/lcd.c` | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 自维护 |
| `BSP/lcd/lcd.h` | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 自维护 |
| `Core/Inc/adc.h` | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | Core生成区/USER CODE分界 |
| `Core/Inc/dma.h` | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | Core生成区/USER CODE分界 |
| `Core/Inc/gpio.h` | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | Core生成区/USER CODE分界 |
| `Core/Inc/i2c.h` | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | Core生成区/USER CODE分界 |
| `Core/Inc/main.h` | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | Core生成区/USER CODE分界 |
| `Core/Inc/spi.h` | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | Core生成区/USER CODE分界 |
| `Core/Inc/stm32h7xx_hal_conf.h` | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | Core生成区/USER CODE分界 |
| `Core/Inc/stm32h7xx_it.h` | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | Core生成区/USER CODE分界 |
| `Core/Inc/tim.h` | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | Core生成区/USER CODE分界 |
| `Core/Inc/usart.h` | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | Core生成区/USER CODE分界 |
| `Core/Src/adc.c` | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | Core生成区/USER CODE分界 |
| `Core/Src/dma.c` | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | Core生成区/USER CODE分界 |
| `Core/Src/gpio.c` | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | Core生成区/USER CODE分界 |
| `Core/Src/i2c.c` | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | Core生成区/USER CODE分界 |
| `Core/Src/main.c` | [F04 fatal与底层启停失败](system/runtime-and-safety.md#f04) | Core生成区/USER CODE分界 |
| `Core/Src/spi.c` | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | Core生成区/USER CODE分界 |
| `Core/Src/stm32h7xx_hal_msp.c` | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | Core生成区/USER CODE分界 |
| `Core/Src/stm32h7xx_it.c` | [F04 fatal与底层启停失败](system/runtime-and-safety.md#f04) | Core生成区/USER CODE分界 |
| `Core/Src/system_stm32h7xx.c` | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | Core生成区/USER CODE分界 |
| `Core/Src/tim.c` | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | Core生成区/USER CODE分界 |
| `Core/Src/usart.c` | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | Core生成区/USER CODE分界 |

历史P17逐符号迁移JSON继续保留原基线，只作迁移/源码导航，不代替本页功能契约。硬件、Material与Tools独立治理；电脑工具README和桥接缺口见通信功能。

独立板测模块App/system/board_test.c/.h及board_test_config.h、BSP/bsp_board_test.c/.h归[F33](platform/isolated-board-tests.md#f33)，main/唯一IRQ协作映射也由该条维护。
