# Phase 15：完整符号映射与文件清单

> 基线：`578b964b412895bd890557d8504a4c66d812d96e`；映射在 991bd62 先行提交。
> 2026-10-16 用户授权实现；G1 已迁移，G2/G3 待实施。定义行仍指基线位置。
> 每行是旧符号到目标符号的最终选择；相同表示保留。位置为基线声明起始行，字段使用所属类型起始行；迁移后会变化。
> 不对整个文件进行无作用域的字符串替换：`time/page/mode/distance` 等也可能是字段或局部量。
> 迁移对象是绑定到该定义的引用；第三方字段、协议字符串、历史证据不随同名文本迁移。

## 阅读规则与完整性

- 表覆盖 67 个自维护 C/H 的函数定义、类型、保护宏/业务宏/枚举、文件级对象和结构体字段；
  头文件函数声明由同名 C 定义行约束，不另造目标名称。Core 仅列用户区局部变量例外。
- 函数指针、手工 extern、初始化器、宏实参和测试 C 夹具均是迁移引用。引用列是基线文本命中
  文件（含定义/声明/注释，不等于活动调用链）；实施时用作用域核对，不把同名字段当全局调用。
- 未单列的自动局部变量/参数已经符合小写下划线，保持原名；静态局部变量及全部不合规局部名
  已在表中列出。成员更名不改变顺序/类型/偏移；其余字段明确列为旧名等于新名。
- 字符串中的源文件、SHA、CSV 列、串口协议和历史验证文本不替换；测试提取的旧版源码
  保持基线，通过测试侧适配当前端 API，不回写历史提交。
- `PID_speed → PID_UpdateSpeed` 覆盖 Phase 13 FR-8 的旧名称决定，静态属性保持。
- `ISLAND_LPREENTER` 等历史左右名称只加模块前缀，数值和右环岛实际路径保持，不借命名改算法。
- 编译断言保留 typedef-char 机制，只给类型和宏加模块前缀/后缀，不引入新框架。
- 同名的条件编译定义（如 LCD 复位函数）合并为一行，两屏分支都须同步，不删除任何分支。

## 文件清单与验收归属

路径相对 `SmartCar/`；同一文件表头中的验证项适用于该表全部行。

| 精确文件 | 后续实现组 |
|---|---|
| `App/app.c` | G3 |
| `App/app.h` | G3 |
| `App/app_config.h` | G3 |
| `App/control.c` | G2 |
| `App/control.h` | G2 |
| `App/dashboard.c` | G3 |
| `App/dashboard.h` | G3 |
| `App/datalog.c` | G3 |
| `App/datalog.h` | G3 |
| `App/electromagnetic_fsm.c` | G2 |
| `App/electromagnetic_fsm.h` | G2 |
| `App/element.c` | G2 |
| `App/element.h` | G2 |
| `App/filter.c` | G2 |
| `App/filter.h` | G2 |
| `App/gray_calibration.h` | G2 |
| `App/gray_sensor.c` | G2 |
| `App/gray_sensor.h` | G2 |
| `App/imu_proc.c` | G2 |
| `App/imu_proc.h` | G2 |
| `App/menu.c` | G3 |
| `App/menu.h` | G3 |
| `App/motor.c` | G2 |
| `App/motor.h` | G2 |
| `App/optical_fsm.c` | G2 |
| `App/optical_fsm.h` | G2 |
| `App/param.c` | G3 |
| `App/param.h` | G3 |
| `App/pid.c` | G2 |
| `App/pid.h` | G2 |
| `App/roundabout.c` | G2 |
| `App/roundabout.h` | G2 |
| `App/track_sensor.c` | G2 |
| `App/track_sensor.h` | G2 |
| `App/wireless.c` | G3 |
| `App/wireless.h` | G3 |
| `BSP/bsp.h` | G1 |
| `BSP/bsp_adc.c` | G1 |
| `BSP/bsp_adc.h` | G1 |
| `BSP/bsp_dl1b.c` | G1 |
| `BSP/bsp_dl1b.h` | G1 |
| `BSP/bsp_encoder.c` | G1 |
| `BSP/bsp_encoder.h` | G1 |
| `BSP/bsp_flash.c` | G1 |
| `BSP/bsp_flash.h` | G1 |
| `BSP/bsp_gray.c` | G1 |
| `BSP/bsp_gray.h` | G1 |
| `BSP/bsp_imu660rb.c` | G1 |
| `BSP/bsp_imu660rb.h` | G1 |
| `BSP/bsp_key.c` | G1 |
| `BSP/bsp_key.h` | G1 |
| `BSP/bsp_lcd.c` | G1 |
| `BSP/bsp_lcd.h` | G1 |
| `BSP/bsp_pwm.c` | G1 |
| `BSP/bsp_pwm.h` | G1 |
| `BSP/bsp_sampler.c` | G1 |
| `BSP/bsp_sampler.h` | G1 |
| `BSP/bsp_time.c` | G1 |
| `BSP/bsp_time.h` | G1 |
| `BSP/bsp_uart.c` | G1 |
| `BSP/bsp_uart.h` | G1 |
| `BSP/bsp_w25q64.c` | G1 |
| `BSP/bsp_w25q64.h` | G1 |
| `BSP/bsp_wdt.c` | G1 |
| `BSP/bsp_wdt.h` | G1 |
| `BSP/lcd/lcd.c` | G1 |
| `BSP/lcd/lcd.h` | G1 |

Core 全部 21 个 C/H 仅检查以下用户区；没有用户区内容的文件不得为凑齐范围修改：

| Core 文件 | 边界 |
|---|---|
| `Core/Inc/adc.h` | 无非空自维护用户区；不修改 |
| `Core/Inc/dma.h` | 无非空自维护用户区；不修改 |
| `Core/Inc/gpio.h` | 无非空自维护用户区；不修改 |
| `Core/Inc/i2c.h` | 无非空自维护用户区；不修改 |
| `Core/Inc/main.h` | 无非空自维护用户区；不修改 |
| `Core/Inc/spi.h` | 无非空自维护用户区；不修改 |
| `Core/Inc/stm32h7xx_hal_conf.h` | 无非空自维护用户区；不修改 |
| `Core/Inc/stm32h7xx_it.h` | 无非空自维护用户区；不修改 |
| `Core/Inc/tim.h` | 无非空自维护用户区；不修改 |
| `Core/Inc/usart.h` | 无非空自维护用户区；不修改 |
| `Core/Src/adc.c` | 无非空自维护用户区；不修改 |
| `Core/Src/dma.c` | 无非空自维护用户区；不修改 |
| `Core/Src/gpio.c` | 无非空自维护用户区；不修改 |
| `Core/Src/i2c.c` | 无非空自维护用户区；不修改 |
| `Core/Src/main.c` | 仅现有用户区：`Includes`, `SysInit`, `2`, `WHILE`, `3`, `Error_Handler_Debug`, `6` |
| `Core/Src/spi.c` | 无非空自维护用户区；不修改 |
| `Core/Src/stm32h7xx_hal_msp.c` | 无非空自维护用户区；不修改 |
| `Core/Src/stm32h7xx_it.c` | 仅现有用户区：`NonMaskableInt_IRQn 1` |
| `Core/Src/system_stm32h7xx.c` | 无非空自维护用户区；不修改 |
| `Core/Src/tim.c` | 无非空自维护用户区；不修改 |
| `Core/Src/usart.c` | 无非空自维护用户区；不修改 |

辅助引用文件：`tests/control_equivalence.py`、`tests/dual_mode_tracking.py`、
`tests/datalog_v2.py` 只允许在后续代码组调整名称/格式匹配及测试夹具；本轮完全不修改。
启动文件、`.ioc`、Keil 工程、GCC 构建/链接脚本均不需因本次命名修改。

## 固定名称与排除清单

| 编号 | 固定对象 | 原因与允许操作 |
|---|---|---|
| E-01 | `HAL_TIM_PeriodElapsedCallback`、`HAL_UART_RxCpltCallback`、`HAL_UART_ErrorCallback`、`TIM15_IRQHandler`、`fputc` | HAL/向量表/运行库按名称链接；只整理自有函数体和注释。 |
| E-02 | `ST7735Ctx`、厂商 ST7735/HAL/CMSIS 类型、对象、成员、寄存器名 | 厂商组件契约；自有胶水对象 `st7735_pObj/st7735_pIO` 不属例外，见映射。 |
| E-03 | `main`、`MX_*`、`SystemClock_Config`、`Error_Handler`、Core 的 `HAL_*`/`*IRQHandler`、`SystemInit/SystemCoreClock*`、生成外设句柄及引脚宏 | Core 生成区、启动与外设契约；保持名称及生成区原文，只改用户区中的自有引用或局部量。 |
| E-04 | `Drivers/**`、`BSP/lcd/st7735.c/.h`、`BSP/lcd/st7735_reg.c/.h`、`BSP/lcd/font.h`、参考工程 | 排除全量风格迁移；字库存储类问题仍归 Phase 11 FR-9。 |
| E-05 | `__CC_ARM`、`USE_HAL_DRIVER`、`STM32H743xx`、编译器属性、`.ARM.__at_0x30000000`、链接符号、字符串协议 | 外部工具链或数据契约；不作本地命名替换。 |

## 逐文件最终映射

### App/app.c

验证：V-01/02/03/05/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 函数 :55 | `App_IsStopped` | `App_IsStopped` | `App/app.c`、`App/app.h`、`App/control.c`、`tests/dual_mode_tracking.py` |
| 函数 :60 | `App_Init` | `App_Init` | `App/app.c`、`App/app.h`、`Core/Src/main.c`、`tests/dual_mode_tracking.py` |
| 函数 :105 | `App_Loop` | `App_Loop` | `App/app.c`、`App/app.h`、`Core/Src/main.c`、`tests/dual_mode_tracking.py` |
| 私有函数 :168 | `App_TryStart` | `App_TryStart` | `App/app.c`、`tests/dual_mode_tracking.py` |
| 函数 :192 | `App_ControlISR` | `App_ControlISR` | `App/app.c`、`App/app.h`、`tests/dual_mode_tracking.py` |
| 函数 :230 | `App_TaskISR` | `App_TaskISR` | `App/app.c`、`App/app.h`、`tests/dual_mode_tracking.py` |
| 函数 :259 | `HAL_TIM_PeriodElapsedCallback` | `HAL_TIM_PeriodElapsedCallback` | 固定入口 E-01；`App/app.c` |
| 函数 :273 | `App_SampleISR` | `App_SampleISR` | `App/app.c`、`App/app.h`、`BSP/bsp_sampler.c`、`tests/dual_mode_tracking.py` |
| 函数 :283 | `App_RequestStop` | `App_RequestStop` | `App/app.c`、`App/app.h`、`App/control.c`、`App/datalog.c`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 私有函数 :316 | `App_FinalizeStop` | `App_FinalizeStop` | `App/app.c` |
| 私有函数 :346 | `App_ResetRunState` | `App_ResetRunState` | `App/app.c` |
| 函数 :361 | `float_abs` | `App_AbsFloat` | `App/app.c`、`App/app.h`、`App/roundabout.c`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :5 | `LED_RUN_LEVEL` | `APP_LED_RUN_LEVEL` | `App/app.c` |
| 对象 :29 | `uart` | `g_app_uart_tx_buffer` | `App/app.c`、`App/app.h`、`App/wireless.c`、`tests/dual_mode_tracking.py` |
| 对象 :30 | `dat` | `g_app_uart_rx_buffer` | `App/app.c`、`App/app.h`、`App/wireless.c`、`BSP/bsp_dl1b.c`、`BSP/bsp_imu660rb.c`、`tests/dual_mode_tracking.py` |
| 对象 :32 | `battery` | `g_app_battery_raw` | `App/app.c`、`App/app.h`、`App/datalog.c`、`App/datalog.h` |
| 对象 :33 | `battery_filt` | `g_app_battery_filtered` | `App/app.c`、`App/app.h`、`App/dashboard.c`、`App/datalog.c`、`App/menu.c`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 对象 :34 | `low_power_num` | `g_app_low_power_count` | `App/app.c` |
| 对象 :36 | `time` | `g_app_control_ticks` | `App/app.c`、`App/app.h`、`App/app_config.h`、`App/control.c`、`App/element.c`、`App/optical_fsm.c`、`App/optical_fsm.h`、`Core/Inc/stm32h7xx_hal_conf.h`、`Core/Src/system_stm32h7xx.c`、`tests/dual_mode_tracking.py` |
| 对象 :38 | `key_flag` | `g_app_key_flag` | `App/app.c`、`App/app.h`、`tests/dual_mode_tracking.py` |
| 对象 :39 | `Start_flag` | `g_app_start_flag` | `App/app.c`、`App/app.h`、`tests/dual_mode_tracking.py` |
| 对象 :40 | `Run_flag` | `g_app_run_flag` | `App/app.c`、`App/app.h`、`App/control.c`、`tests/dual_mode_tracking.py` |
| 对象 :42 | `filt_battery` | `g_app_battery_filter` | `App/app.c` |
| 对象 :44 | `dl1b_distance_mm` | `g_app_dl1b_distance_mm` | `App/app.c`、`App/app.h`、`App/control.c`、`App/dashboard.c`、`App/datalog.c`、`App/roundabout.c`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 对象 :47 | `s_stop_pending` | `s_stop_pending` | `App/app.c`、`tests/dual_mode_tracking.py` |
| 对象 :48 | `s_stop_reason` | `s_stop_reason` | `App/app.c`、`tests/dual_mode_tracking.py` |
| 对象 :49 | `s_wdt_started` | `s_wdt_started` | `App/app.c` |
| 对象 :50 | `s_flash_available` | `s_flash_available` | `App/app.c` |
| 局部 App_Loop :149 | `led_last` | `s_led_last` | `App/app.c` |

### App/app.h

验证：V-01/02/03/05/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 类型 :19 | `app_stop_reason_t` | `app_stop_reason_t` | `App/app.c`、`App/app.h`、`App/control.c`、`App/datalog.c`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :2 | `__APP_H` | `SMARTCAR_APP_APP_H` | `App/app.h` |
| 宏/枚举 :19 | `APP_STOP_NONE` | `APP_STOP_NONE` | `App/app.c`、`App/app.h`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :19 | `APP_STOP_OFF_TRACK` | `APP_STOP_OFF_TRACK` | `App/app.c`、`App/app.h`、`App/control.c`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :19 | `APP_STOP_LOW_BATTERY` | `APP_STOP_LOW_BATTERY` | `App/app.c`、`App/app.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :19 | `APP_STOP_GRAY_LOST` | `APP_STOP_GRAY_LOST` | `App/app.c`、`App/app.h`、`App/control.c`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :19 | `APP_STOP_GRAY_ACQUIRE` | `APP_STOP_GRAY_ACQUIRE` | `App/app.c`、`App/app.h`、`App/control.c`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :19 | `APP_STOP_CONTROL_OVERRUN` | `APP_STOP_CONTROL_OVERRUN` | `App/app.c`、`App/app.h`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :19 | `APP_STOP_LOG_OVERFLOW` | `APP_STOP_LOG_OVERFLOW` | `App/app.c`、`App/app.h`、`App/datalog.c`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :19 | `APP_STOP_LOG_WRITE` | `APP_STOP_LOG_WRITE` | `App/app.c`、`App/app.h`、`App/datalog.c`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :19 | `APP_STOP_LOG_FULL` | `APP_STOP_LOG_FULL` | `App/app.c`、`App/app.h`、`App/datalog.c`、`tests/dual_mode_tracking.py` |
| 对象声明 :7 | `uart` | `g_app_uart_tx_buffer` | `App/app.c`、`App/app.h`、`App/wireless.c`、`tests/dual_mode_tracking.py` |
| 对象声明 :8 | `dat` | `g_app_uart_rx_buffer` | `App/app.c`、`App/app.h`、`App/wireless.c`、`BSP/bsp_dl1b.c`、`BSP/bsp_imu660rb.c`、`tests/dual_mode_tracking.py` |
| 对象声明 :9 | `battery` | `g_app_battery_raw` | `App/app.c`、`App/app.h`、`App/datalog.c`、`App/datalog.h` |
| 对象声明 :10 | `battery_filt` | `g_app_battery_filtered` | `App/app.c`、`App/app.h`、`App/dashboard.c`、`App/datalog.c`、`App/menu.c`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 对象声明 :11 | `time` | `g_app_control_ticks` | `App/app.c`、`App/app.h`、`App/app_config.h`、`App/control.c`、`App/element.c`、`App/optical_fsm.c`、`App/optical_fsm.h`、`Core/Inc/stm32h7xx_hal_conf.h`、`Core/Src/system_stm32h7xx.c`、`tests/dual_mode_tracking.py` |
| 对象声明 :12 | `key_flag` | `g_app_key_flag` | `App/app.c`、`App/app.h`、`tests/dual_mode_tracking.py` |
| 对象声明 :13 | `Start_flag` | `g_app_start_flag` | `App/app.c`、`App/app.h`、`tests/dual_mode_tracking.py` |
| 对象声明 :14 | `Run_flag` | `g_app_run_flag` | `App/app.c`、`App/app.h`、`App/control.c`、`tests/dual_mode_tracking.py` |
| 对象声明 :17 | `dl1b_distance_mm` | `g_app_dl1b_distance_mm` | `App/app.c`、`App/app.h`、`App/control.c`、`App/dashboard.c`、`App/datalog.c`、`App/roundabout.c`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |

### App/app_config.h

验证：V-01/02/03/05/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 宏/枚举 :2 | `__APP_CONFIG_H` | `SMARTCAR_APP_APP_CONFIG_H` | `App/app_config.h` |
| 宏/枚举 :3 | `CTRL_PERIOD_MS` | `APP_CTRL_PERIOD_MS` | `App/app_config.h` |
| 宏/枚举 :4 | `TASK_PERIOD_MS` | `APP_TASK_PERIOD_MS` | `App/app_config.h` |
| 宏/枚举 :6 | `CTRL_DT_S` | `APP_CTRL_DT_S` | `App/app_config.h`、`App/imu_proc.c` |
| 宏/枚举 :7 | `RUN_DELAY_COUNT` | `APP_RUN_DELAY_COUNT` | `App/app.c`、`App/app_config.h`、`App/control.c`、`App/optical_fsm.c` |
| 宏/枚举 :8 | `WDG_ENABLE` | `APP_WDG_ENABLE` | `App/app_config.h`、`BSP/bsp_wdt.c`、`BSP/bsp_wdt.h` |
| 宏/枚举 :8 | `ADC_FULL_SCALE` | `APP_ADC_FULL_SCALE` | `App/app_config.h`、`App/track_sensor.c` |
| 宏/枚举 :9 | `BATTERY_LOW_THRESHOLD` | `APP_BATTERY_LOW_THRESHOLD` | `App/app.c`、`App/app_config.h` |
| 宏/枚举 :9 | `BATTERY_MIN_VALID` | `APP_BATTERY_MIN_VALID` | `App/app.c`、`App/app_config.h` |
| 宏/枚举 :10 | `BATTERY_LOW_COUNT` | `APP_BATTERY_LOW_COUNT` | `App/app.c`、`App/app_config.h` |
| 宏/枚举 :11 | `GYRO_RAW_TO_DPS` | `IMU_GYRO_RAW_TO_DPS` | `App/app_config.h`、`App/datalog.c`、`App/imu_proc.c` |
| 宏/枚举 :11 | `ACC_RAW_TO_G` | `IMU_ACC_RAW_TO_G` | `App/app_config.h` |
| 宏/枚举 :14 | `FAN_DUTY_IDLE` | `MOTOR_FAN_DUTY_IDLE` | `App/app_config.h`、`App/param.c` |
| 宏/枚举 :16 | `FAN_DUTY_RUN` | `MOTOR_FAN_DUTY_RUN` | `App/app_config.h`、`App/param.c` |
| 宏/枚举 :16 | `DEBUG_UART_BAUD` | `APP_DEBUG_UART_BAUD` | `App/app_config.h` |
| 宏/枚举 :19 | `DATALOG_ENABLE` | `DATALOG_ENABLE` | `App/app.c`、`App/app_config.h`、`App/datalog.c`、`tests/datalog_v2.py` |
| 宏/枚举 :19 | `DATALOG_DIV` | `DATALOG_DIV` | `App/app_config.h`、`App/datalog.c`、`tests/datalog_v2.py` |
| 宏/枚举 :20 | `LCD_TARGET_EXTERNAL` | `LCD_TARGET_EXTERNAL` | `App/app.c`、`App/app_config.h`、`App/dashboard.c`、`App/dashboard.h`、`BSP/bsp_lcd.c`、`BSP/lcd/lcd.c`、`BSP/lcd/lcd.h` |

### App/control.c

验证：V-01/02/03/04/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 函数 :59 | `Control_ApplyProfile` | `Control_ApplyProfile` | `App/control.c`、`App/control.h`、`App/roundabout.c`、`tests/dual_mode_tracking.py` |
| 函数 :68 | `Tracking_GetState` | `Tracking_GetState` | `App/app.c`、`App/control.c`、`App/control.h`、`App/dashboard.c`、`App/datalog.c`、`App/menu.c`、`App/param.c`、`App/wireless.c`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 函数 :69 | `Tracking_TrySetMode` | `Tracking_TrySetMode` | `App/app.c`、`App/control.c`、`App/control.h`、`tests/dual_mode_tracking.py` |
| 函数 :81 | `Tracking_SetGear` | `Tracking_SetGear` | `App/control.c`、`App/control.h`、`App/param.c` |
| 函数 :82 | `Tracking_CompleteControl` | `Tracking_CompleteControl` | `App/app.c`、`App/control.c`、`App/control.h` |
| 函数 :83 | `Tracking_Stopped` | `Tracking_Stopped` | `App/app.c`、`App/control.c`、`App/control.h` |
| 函数 :91 | `Tracking_AddProtection` | `Tracking_AddProtection` | `App/app.c`、`App/control.c`、`App/control.h`、`App/datalog.c`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 函数 :97 | `Control_GetKernelProfile` | `Control_GetKernelProfile` | `App/control.c`、`App/control.h`、`App/electromagnetic_fsm.c`、`tests/control_equivalence.py` |
| 函数 :99 | `Control_MergeProfile` | `Control_MergeProfile` | `App/control.c`、`App/control.h`、`App/electromagnetic_fsm.c`、`tests/control_equivalence.py` |
| 函数 :107 | `Tracking_Observe` | `Tracking_Observe` | `App/app.c`、`App/control.c`、`App/control.h` |
| 函数 :126 | `Tracking_Execute` | `Tracking_Execute` | `App/app.c`、`App/control.c`、`App/control.h` |
| 函数 :147 | `Tracking_AccumulateMotion` | `Tracking_AccumulateMotion` | `App/app.c`、`App/control.c`、`App/control.h` |
| 函数 :151 | `whole_test` | `Control_UpdateLegacy` | `App/control.c`、`App/control.h`、`tests/dual_mode_tracking.py` |
| 函数 :172 | `Control_ResetRunState` | `Control_ResetRunState` | `App/app.c`、`App/control.c`、`App/control.h`、`tests/dual_mode_tracking.py` |
| 对象 :23 | `track_out` | `g_control_track_output` | `App/app.c`、`App/control.c`、`App/control.h`、`App/dashboard.c`、`App/datalog.c`、`App/datalog.h`、`App/roundabout.c`、`tests/dual_mode_tracking.py` |
| 对象 :24 | `kernel_state` | `g_control_kernel_state` | `App/control.c`、`App/control.h`、`App/dashboard.c`、`App/datalog.c`、`App/datalog.h`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`App/element.c`、`App/roundabout.c`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 对象 :25 | `cask_flag` | `g_control_cask_flag` | `App/control.c`、`App/control.h`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`App/roundabout.c` |
| 对象 :28 | `s_kernel_profiles` | `s_kernel_profiles` | `App/control.c` |
| 对象 :67 | `s_tracking` | `s_tracking` | `App/control.c` |

### App/control.h

验证：V-01/02/03/04/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 类型 :33 | `control_profile_t` | `control_profile_t` | `App/control.c`、`App/control.h`、`App/electromagnetic_fsm.h`、`App/roundabout.c`、`App/roundabout.h` |
| 类型 :45 | `tracking_mode_t` | `tracking_mode_t` | `App/control.h` |
| 类型 :55 | `tracking_result_t` | `tracking_result_t` | `App/control.c`、`App/control.h` |
| 类型 :56 | `tracking_state_t` | `tracking_state_t` | `App/control.c`、`App/control.h`、`App/datalog.c`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :1 | `_CONTROL_H_` | `SMARTCAR_APP_CONTROL_H` | `App/control.h` |
| 宏/枚举 :3 | `KERNEL_TRACKING` | `CONTROL_KERNEL_TRACKING` | `App/control.c`、`App/control.h`、`App/electromagnetic_fsm.c`、`App/element.c`、`App/roundabout.c`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :4 | `KERNEL_ISLAND_L` | `CONTROL_KERNEL_ISLAND_L` | `App/control.c`、`App/control.h`、`tests/control_equivalence.py` |
| 宏/枚举 :5 | `KERNEL_ISLAND_R` | `CONTROL_KERNEL_ISLAND_R` | `App/control.c`、`App/control.h`、`App/electromagnetic_fsm.c`、`App/roundabout.c`、`tests/control_equivalence.py` |
| 宏/枚举 :6 | `KERNEL_TEETERBOARD` | `CONTROL_KERNEL_TEETERBOARD` | `App/control.c`、`App/control.h`、`App/element.c`、`tests/control_equivalence.py` |
| 宏/枚举 :6 | `KERNEL_CROSSROADS` | `CONTROL_KERNEL_CROSSROADS` | `App/control.c`、`App/control.h`、`App/electromagnetic_fsm.c`、`App/element.c`、`tests/control_equivalence.py` |
| 宏/枚举 :7 | `KERNEL_CASK` | `CONTROL_KERNEL_CASK` | `App/control.c`、`App/control.h`、`App/element.c`、`tests/control_equivalence.py` |
| 宏/枚举 :7 | `KERNEL_REISLAND` | `CONTROL_KERNEL_REISLAND` | `App/control.c`、`App/control.h`、`App/electromagnetic_fsm.c`、`App/roundabout.c`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :14 | `CONTROL_PROFILE_WEIGHTS` | `CONTROL_PROFILE_WEIGHTS` | `App/control.c`、`App/control.h`、`App/roundabout.c`、`tests/control_equivalence.py` |
| 宏/枚举 :17 | `CONTROL_PROFILE_BASE` | `CONTROL_PROFILE_BASE` | `App/control.c`、`App/control.h`、`App/roundabout.c`、`tests/control_equivalence.py` |
| 宏/枚举 :18 | `CONTROL_PROFILE_TRACK` | `CONTROL_PROFILE_TRACK` | `App/control.c`、`App/control.h`、`App/roundabout.c`、`tests/control_equivalence.py` |
| 宏/枚举 :20 | `CONTROL_PROFILE_ANGLE` | `CONTROL_PROFILE_ANGLE` | `App/control.c`、`App/control.h`、`tests/control_equivalence.py` |
| 宏/枚举 :39 | `PROTECT_EM_LOST` | `TRACKING_PROTECT_EM_LOST` | `App/app.c`、`App/control.c`、`App/control.h`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :41 | `PROTECT_GRAY_LOST` | `TRACKING_PROTECT_GRAY_LOST` | `App/app.c`、`App/control.c`、`App/control.h` |
| 宏/枚举 :41 | `PROTECT_CALIBRATION` | `TRACKING_PROTECT_CALIBRATION` | `App/app.c`、`App/control.c`、`App/control.h` |
| 宏/枚举 :42 | `PROTECT_ACQUIRE` | `TRACKING_PROTECT_ACQUIRE` | `App/app.c`、`App/control.c`、`App/control.h`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :43 | `PROTECT_OVERRUN` | `TRACKING_PROTECT_OVERRUN` | `App/app.c`、`App/control.h`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :43 | `PROTECT_LOG_OVERFLOW` | `TRACKING_PROTECT_LOG_OVERFLOW` | `App/app.c`、`App/control.h`、`App/datalog.c` |
| 宏/枚举 :43 | `PROTECT_LOG_WRITE` | `TRACKING_PROTECT_LOG_WRITE` | `App/app.c`、`App/control.h`、`App/datalog.c` |
| 宏/枚举 :45 | `PROTECT_LOG_FULL` | `TRACKING_PROTECT_LOG_FULL` | `App/app.c`、`App/control.h`、`App/datalog.c` |
| 宏/枚举 :45 | `PROTECT_LOW_BATTERY` | `TRACKING_PROTECT_LOW_BATTERY` | `App/app.c`、`App/control.h`、`tests/datalog_v2.py` |
| 宏/枚举 :45 | `TRACK_MODE_EM` | `TRACKING_MODE_EM` | `App/control.h` |
| 宏/枚举 :45 | `TRACK_MODE_OPTICAL` | `TRACKING_MODE_OPTICAL` | `App/control.c`、`App/control.h`、`App/dashboard.c`、`App/menu.c`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :55 | `TRACKING_OK` | `TRACKING_OK` | `App/control.c`、`App/control.h`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :55 | `TRACKING_BUSY` | `TRACKING_BUSY` | `App/control.c`、`App/control.h`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :55 | `TRACKING_SAVE_FAILED` | `TRACKING_SAVE_FAILED` | `App/control.c`、`App/control.h`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :55 | `TRACKING_ARG` | `TRACKING_ARG` | `App/control.c`、`App/control.h` |
| 对象声明 :24 | `track_out` | `g_control_track_output` | `App/app.c`、`App/control.c`、`App/control.h`、`App/dashboard.c`、`App/datalog.c`、`App/datalog.h`、`App/roundabout.c`、`tests/dual_mode_tracking.py` |
| 对象声明 :25 | `kernel_state` | `g_control_kernel_state` | `App/control.c`、`App/control.h`、`App/dashboard.c`、`App/datalog.c`、`App/datalog.h`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`App/element.c`、`App/roundabout.c`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 对象声明 :26 | `cask_flag` | `g_control_cask_flag` | `App/control.c`、`App/control.h`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`App/roundabout.c` |
| 字段 control_profile_t :33 | `fields` | `fields` | `App/control.c`、`App/control.h`、`tests/control_equivalence.py` |
| 字段 control_profile_t :33 | `weights` | `weights` | `App/control.c`、`App/control.h`、`App/datalog.c`、`App/datalog.h`、`App/electromagnetic_fsm.c`、`App/track_sensor.c`、`App/track_sensor.h`、`tests/control_equivalence.py` |
| 字段 control_profile_t :33 | `base_speed` | `base_speed` | `App/control.c`、`App/control.h`、`App/datalog.c`、`App/datalog.h`、`App/menu.c`、`App/motor.c`、`App/motor.h`、`App/optical_fsm.c`、`App/optical_fsm.h`、`App/param.c`、`App/param.h`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 字段 control_profile_t :33 | `track` | `track` | `App/control.c`、`App/control.h`、`App/electromagnetic_fsm.c`、`App/element.c`、`App/roundabout.c`、`tests/control_equivalence.py`、`tests/datalog_v2.py` |
| 字段 control_profile_t :33 | `angle` | `angle` | `App/control.c`、`App/control.h`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`App/element.c`、`App/imu_proc.c`、`App/imu_proc.h`、`App/pid.c`、`tests/control_equivalence.py` |
| 字段 tracking_state_t :56 | `mode` | `mode` | `App/app.c`、`App/control.c`、`App/control.h`、`App/dashboard.c`、`App/datalog.c`、`App/datalog.h`、`App/menu.c`、`App/param.c`、`App/param.h`、`App/wireless.c`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 字段 tracking_state_t :56 | `gear` | `gear` | `App/control.c`、`App/control.h`、`App/dashboard.c`、`App/datalog.c`、`App/datalog.h`、`App/menu.c`、`App/param.c`、`App/param.h`、`App/wireless.c`、`tests/dual_mode_tracking.py` |
| 字段 tracking_state_t :56 | `active_error` | `active_error` | `App/control.c`、`App/control.h`、`App/datalog.c` |
| 字段 tracking_state_t :56 | `track_out` | `track_out` | `App/app.c`、`App/control.c`、`App/control.h`、`App/dashboard.c`、`App/datalog.c`、`App/datalog.h`、`App/roundabout.c`、`tests/dual_mode_tracking.py` |
| 字段 tracking_state_t :56 | `control_sequence` | `control_sequence` | `App/control.c`、`App/control.h`、`App/datalog.c`、`App/datalog.h`、`tests/datalog_v2.py` |
| 字段 tracking_state_t :56 | `protection_flags` | `protection_flags` | `App/app.c`、`App/control.c`、`App/control.h`、`App/datalog.c`、`App/datalog.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |

### App/dashboard.c

验证：V-01/02/03/05/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 函数 :27 | `Dashboard_Init` | `Dashboard_Init` | `App/app.c`、`App/dashboard.c`、`App/dashboard.h` |
| 函数 :44 | `Dashboard_Update` | `Dashboard_Update` | `App/app.c`、`App/dashboard.c`、`App/dashboard.h` |
| 宏/枚举 :3 | `DASH_REFRESH_MS` | `DASHBOARD_REFRESH_MS` | `App/dashboard.c` |
| 对象 :21 | `s_last_refresh` | `s_last_refresh` | `App/dashboard.c` |

### App/dashboard.h

验证：V-01/02/03/05/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 宏/枚举 :2 | `__DASHBOARD_H` | `SMARTCAR_APP_DASHBOARD_H` | `App/dashboard.h` |

### App/datalog.c

验证：V-01/02/03/05/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 私有函数 :35 | `crc16` | `Datalog_CalculateCrc16` | `App/datalog.c`、`tests/datalog_v2.py` |
| 私有函数 :41 | `latch_fault` | `Datalog_LatchFault` | `App/datalog.c` |
| 私有函数 :48 | `run_fault` | `Datalog_RequestFaultStop` | `App/datalog.c` |
| 函数 :52 | `Datalog_SetStopSummary` | `Datalog_SetStopSummary` | `App/app.c`、`App/datalog.c`、`App/datalog.h`、`tests/datalog_v2.py` |
| 函数 :59 | `Datalog_GetStatus` | `Datalog_GetStatus` | `App/app.c`、`App/datalog.c`、`App/datalog.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 函数 :64 | `Datalog_IsActive` | `Datalog_IsActive` | `App/datalog.c`、`App/datalog.h`、`tests/datalog_v2.py` |
| 函数 :65 | `Datalog_GetCount` | `Datalog_GetCount` | `App/datalog.c`、`App/datalog.h` |
| 函数 :66 | `Datalog_Init` | `Datalog_Init` | `App/datalog.c`、`App/datalog.h` |
| 私有函数 :71 | `set_profile` | `Datalog_SetProfile` | `App/datalog.c` |
| 函数 :73 | `Datalog_Start` | `Datalog_Start` | `App/app.c`、`App/datalog.c`、`App/datalog.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 函数 :99 | `Datalog_Clear` | `Datalog_Clear` | `App/datalog.c`、`App/datalog.h`、`App/wireless.c`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 函数 :105 | `Datalog_PublishControl` | `Datalog_PublishControl` | `App/app.c`、`App/datalog.c`、`App/datalog.h`、`tests/datalog_v2.py` |
| 函数 :123 | `Datalog_Push` | `Datalog_Push` | `App/app.c`、`App/datalog.c`、`App/datalog.h`、`tests/datalog_v2.py` |
| 函数 :138 | `Datalog_Service` | `Datalog_Service` | `App/app.c`、`App/datalog.c`、`App/datalog.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 函数 :151 | `Datalog_Flush` | `Datalog_Flush` | `App/datalog.c`、`App/datalog.h` |
| 函数 :152 | `Datalog_Stop` | `Datalog_Stop` | `App/app.c`、`App/datalog.c`、`App/datalog.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 私有函数 :158 | `valid_header` | `Datalog_IsHeaderValid` | `App/datalog.c` |
| 函数 :166 | `Datalog_Dump` | `Datalog_Dump` | `App/datalog.c`、`App/datalog.h`、`App/wireless.c`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :7 | `LOG_MAGIC` | `DATALOG_MAGIC` | `App/datalog.c` |
| 宏/枚举 :8 | `BATCH_RECORDS` | `DATALOG_BATCH_RECORDS` | `App/datalog.c` |
| 宏/枚举 :9 | `ASSERT_LAYOUT` | `DATALOG_ASSERT_LAYOUT` | `App/datalog.c` |
| 对象 :27 | `s_buf` | `s_buf` | `App/datalog.c`、`tests/datalog_v2.py` |
| 对象 :28 | `s_snapshot` | `s_snapshot` | `App/datalog.c` |
| 对象 :29 | `s_header` | `s_header` | `App/datalog.c`、`tests/datalog_v2.py` |
| 对象 :30 | `s_batch` | `s_batch` | `App/datalog.c`、`tests/datalog_v2.py` |
| 对象 :31 | `s_head` | `s_head` | `App/datalog.c` |
| 对象 :31 | `s_tail` | `s_tail` | `App/datalog.c` |
| 对象 :32 | `s_status` | `s_status` | `App/datalog.c`、`tests/datalog_v2.py` |
| 对象 :33 | `s_header_valid` | `s_header_valid` | `App/datalog.c` |
| 对象 :34 | `s_write_failed` | `s_write_failed` | `App/datalog.c` |
| 局部 Datalog_Start :75 | `check` | `s_check` | `App/datalog.c`、`Core/Inc/stm32h7xx_hal_conf.h`、`tests/control_equivalence.py`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 断言类型实参 :20 | `record_size` | `datalog_record_size_assert_t` | `App/datalog.c`、`App/datalog.h` |
| 断言类型实参 :21 | `header_size` | `datalog_header_size_assert_t` | `App/datalog.c`、`App/datalog.h` |
| 断言类型实参 :22 | `record_offsets` | `datalog_record_offsets_assert_t` | `App/datalog.c` |
| 断言类型实参 :24 | `header_offsets` | `datalog_header_offsets_assert_t` | `App/datalog.c` |
| 断言类型实参 :26 | `log_capacity` | `datalog_log_capacity_assert_t` | `App/datalog.c` |

### App/datalog.h

验证：V-01/02/03/05/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 类型 :9 | `datalog_record_t` | `datalog_record_t` | `App/datalog.c`、`App/datalog.h`、`tests/datalog_v2.py` |
| 类型 :21 | `datalog_profile_t` | `datalog_profile_t` | `App/datalog.c`、`App/datalog.h` |
| 类型 :22 | `datalog_header_t` | `datalog_header_t` | `App/datalog.c`、`App/datalog.h`、`tests/datalog_v2.py` |
| 类型 :35 | `datalog_fault_t` | `datalog_fault_t` | `App/datalog.h` |
| 类型 :38 | `datalog_status_t` | `datalog_status_t` | `App/app.c`、`App/datalog.c`、`App/datalog.h` |
| 宏/枚举 :2 | `DATALOG_H` | `SMARTCAR_APP_DATALOG_H` | `App/datalog.h` |
| 宏/枚举 :4 | `DATALOG_RECORD_SIZE` | `DATALOG_RECORD_SIZE` | `App/datalog.c`、`App/datalog.h`、`tests/control_equivalence.py` |
| 宏/枚举 :5 | `DATALOG_HEADER_SIZE` | `DATALOG_HEADER_SIZE` | `App/datalog.h`、`tests/control_equivalence.py` |
| 宏/枚举 :6 | `DATALOG_CAPACITY` | `DATALOG_CAPACITY` | `App/datalog.c`、`App/datalog.h` |
| 宏/枚举 :7 | `DATALOG_BUFFER_RECORDS` | `DATALOG_BUFFER_RECORDS` | `App/datalog.c`、`App/datalog.h` |
| 宏/枚举 :35 | `DATALOG_FAULT_NONE` | `DATALOG_FAULT_NONE` | `App/datalog.h` |
| 宏/枚举 :35 | `DATALOG_FAULT_CONFIG` | `DATALOG_FAULT_CONFIG` | `App/datalog.c`、`App/datalog.h`、`tests/datalog_v2.py` |
| 宏/枚举 :35 | `DATALOG_FAULT_UNAVAILABLE` | `DATALOG_FAULT_UNAVAILABLE` | `App/datalog.c`、`App/datalog.h`、`tests/datalog_v2.py` |
| 宏/枚举 :35 | `DATALOG_FAULT_HEADER` | `DATALOG_FAULT_HEADER` | `App/datalog.c`、`App/datalog.h`、`tests/datalog_v2.py` |
| 宏/枚举 :35 | `DATALOG_FAULT_OVERFLOW` | `DATALOG_FAULT_OVERFLOW` | `App/datalog.c`、`App/datalog.h`、`tests/datalog_v2.py` |
| 宏/枚举 :35 | `DATALOG_FAULT_WRITE` | `DATALOG_FAULT_WRITE` | `App/datalog.c`、`App/datalog.h`、`tests/datalog_v2.py` |
| 宏/枚举 :35 | `DATALOG_FAULT_FULL` | `DATALOG_FAULT_FULL` | `App/datalog.c`、`App/datalog.h`、`tests/datalog_v2.py` |
| 宏/枚举 :35 | `DATALOG_FAULT_READ` | `DATALOG_FAULT_READ` | `App/datalog.c`、`App/datalog.h` |
| 字段 datalog_record_t :9 | `tick` | `tick` | `App/datalog.c`、`App/datalog.h`、`Core/Inc/stm32h7xx_hal_conf.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 字段 datalog_record_t :9 | `adc` | `adc` | `App/datalog.c`、`App/datalog.h`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`App/track_sensor.c`、`BSP/bsp.h`、`BSP/bsp_adc.c`、`Core/Inc/adc.h`、`Core/Src/adc.c`、`Core/Src/main.c`、`tests/dual_mode_tracking.py` |
| 字段 datalog_record_t :9 | `speed_l` | `speed_l` | `App/datalog.c`、`App/datalog.h` |
| 字段 datalog_record_t :9 | `speed_r` | `speed_r` | `App/datalog.c`、`App/datalog.h` |
| 字段 datalog_record_t :9 | `track_error` | `track_error` | `App/datalog.c`、`App/datalog.h`、`tests/control_equivalence.py` |
| 字段 datalog_record_t :9 | `track_out` | `track_out` | `App/app.c`、`App/control.c`、`App/control.h`、`App/dashboard.c`、`App/datalog.c`、`App/datalog.h`、`App/roundabout.c`、`tests/dual_mode_tracking.py` |
| 字段 datalog_record_t :9 | `gyro_x` | `gyro_x` | `App/datalog.c`、`App/datalog.h`、`tests/control_equivalence.py` |
| 字段 datalog_record_t :9 | `gyro_y` | `gyro_y` | `App/datalog.c`、`App/datalog.h`、`tests/control_equivalence.py` |
| 字段 datalog_record_t :9 | `gyro_z` | `gyro_z` | `App/datalog.c`、`App/datalog.h`、`tests/control_equivalence.py` |
| 字段 datalog_record_t :9 | `dl1b_mm` | `dl1b_mm` | `App/datalog.c`、`App/datalog.h`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h` |
| 字段 datalog_record_t :9 | `battery` | `battery` | `App/app.c`、`App/app.h`、`App/datalog.c`、`App/datalog.h` |
| 字段 datalog_record_t :9 | `kernel_state` | `kernel_state` | `App/control.c`、`App/control.h`、`App/dashboard.c`、`App/datalog.c`、`App/datalog.h`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`App/element.c`、`App/roundabout.c`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 字段 datalog_record_t :9 | `roundabout_state` | `roundabout_state` | `App/control.c`、`App/datalog.c`、`App/datalog.h`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`App/roundabout.c`、`App/roundabout.h`、`tests/dual_mode_tracking.py` |
| 字段 datalog_record_t :9 | `gray_raw` | `gray_raw` | `App/datalog.c`、`App/datalog.h` |
| 字段 datalog_record_t :9 | `gray_sequence` | `gray_sequence` | `App/datalog.c`、`App/datalog.h`、`tests/datalog_v2.py` |
| 字段 datalog_record_t :9 | `control_sequence` | `control_sequence` | `App/control.c`、`App/control.h`、`App/datalog.c`、`App/datalog.h`、`tests/datalog_v2.py` |
| 字段 datalog_record_t :9 | `em_error` | `em_error` | `App/datalog.c`、`App/datalog.h` |
| 字段 datalog_record_t :9 | `optical_error` | `optical_error` | `App/datalog.c`、`App/datalog.h` |
| 字段 datalog_record_t :9 | `protection_flags` | `protection_flags` | `App/app.c`、`App/control.c`、`App/control.h`、`App/datalog.c`、`App/datalog.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 字段 datalog_record_t :9 | `mode` | `mode` | `App/app.c`、`App/control.c`、`App/control.h`、`App/dashboard.c`、`App/datalog.c`、`App/datalog.h`、`App/menu.c`、`App/param.c`、`App/param.h`、`App/wireless.c`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 字段 datalog_record_t :9 | `optical_state` | `optical_state` | `App/datalog.c`、`App/datalog.h` |
| 字段 datalog_record_t :9 | `gray_valid` | `gray_valid` | `App/datalog.c`、`App/datalog.h`、`App/optical_fsm.c`、`App/optical_fsm.h`、`tests/dual_mode_tracking.py` |
| 字段 datalog_record_t :9 | `stop_reason` | `stop_reason` | `App/control.c`、`App/control.h`、`App/datalog.c`、`App/datalog.h`、`App/optical_fsm.c`、`App/optical_fsm.h`、`tests/dual_mode_tracking.py` |
| 字段 datalog_record_t :9 | `reserved` | `reserved` | `App/datalog.c`、`App/datalog.h`、`App/param.c`、`Core/Inc/adc.h`、`Core/Inc/dma.h`、`Core/Inc/gpio.h`、`Core/Inc/i2c.h`、`Core/Inc/main.h`、`Core/Inc/spi.h`、`Core/Inc/stm32h7xx_hal_conf.h`、`Core/Inc/stm32h7xx_it.h`、`Core/Inc/tim.h`、`Core/Inc/usart.h`、`Core/Src/adc.c`、`Core/Src/dma.c`、`Core/Src/gpio.c`、`Core/Src/i2c.c`、`Core/Src/main.c`、`Core/Src/spi.c`、`Core/Src/stm32h7xx_hal_msp.c`、`Core/Src/stm32h7xx_it.c`、`Core/Src/system_stm32h7xx.c`、`Core/Src/tim.c`、`Core/Src/usart.c` |
| 字段 datalog_profile_t :21 | `kp` | `kp` | `App/control.c`、`App/datalog.c`、`App/datalog.h`、`App/menu.c`、`App/param.c`、`App/param.h`、`App/pid.c`、`App/pid.h`、`App/wireless.c`、`tests/dual_mode_tracking.py` |
| 字段 datalog_profile_t :21 | `k2p` | `k2p` | `App/control.c`、`App/datalog.c`、`App/datalog.h`、`App/menu.c`、`App/param.c`、`App/param.h`、`App/pid.c`、`App/pid.h` |
| 字段 datalog_profile_t :21 | `kd` | `kd` | `App/control.c`、`App/datalog.c`、`App/datalog.h`、`App/menu.c`、`App/param.c`、`App/param.h`、`App/pid.c`、`App/pid.h`、`App/wireless.c` |
| 字段 datalog_profile_t :21 | `base_speed` | `base_speed` | `App/control.c`、`App/control.h`、`App/datalog.c`、`App/datalog.h`、`App/menu.c`、`App/motor.c`、`App/motor.h`、`App/optical_fsm.c`、`App/optical_fsm.h`、`App/param.c`、`App/param.h`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 字段 datalog_profile_t :21 | `reserved` | `reserved` | `App/datalog.c`、`App/datalog.h`、`App/param.c`、`Core/Inc/adc.h`、`Core/Inc/dma.h`、`Core/Inc/gpio.h`、`Core/Inc/i2c.h`、`Core/Inc/main.h`、`Core/Inc/spi.h`、`Core/Inc/stm32h7xx_hal_conf.h`、`Core/Inc/stm32h7xx_it.h`、`Core/Inc/tim.h`、`Core/Inc/usart.h`、`Core/Src/adc.c`、`Core/Src/dma.c`、`Core/Src/gpio.c`、`Core/Src/i2c.c`、`Core/Src/main.c`、`Core/Src/spi.c`、`Core/Src/stm32h7xx_hal_msp.c`、`Core/Src/stm32h7xx_it.c`、`Core/Src/system_stm32h7xx.c`、`Core/Src/tim.c`、`Core/Src/usart.c` |
| 字段 datalog_header_t :22 | `magic` | `magic` | `App/datalog.c`、`App/datalog.h`、`App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 datalog_header_t :22 | `version` | `version` | `App/datalog.c`、`App/datalog.h`、`App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 datalog_header_t :22 | `crc16` | `crc16` | `App/datalog.c`、`App/datalog.h`、`App/param.c`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 字段 datalog_header_t :22 | `header_size` | `header_size` | `App/datalog.c`、`App/datalog.h` |
| 字段 datalog_header_t :22 | `record_size` | `record_size` | `App/datalog.c`、`App/datalog.h` |
| 字段 datalog_header_t :22 | `sample_period_us` | `sample_period_us` | `App/datalog.c`、`App/datalog.h` |
| 字段 datalog_header_t :22 | `control_period_us` | `control_period_us` | `App/datalog.c`、`App/datalog.h` |
| 字段 datalog_header_t :22 | `mode` | `mode` | `App/app.c`、`App/control.c`、`App/control.h`、`App/dashboard.c`、`App/datalog.c`、`App/datalog.h`、`App/menu.c`、`App/param.c`、`App/param.h`、`App/wireless.c`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 字段 datalog_header_t :22 | `gear` | `gear` | `App/control.c`、`App/control.h`、`App/dashboard.c`、`App/datalog.c`、`App/datalog.h`、`App/menu.c`、`App/param.c`、`App/param.h`、`App/wireless.c`、`tests/dual_mode_tracking.py` |
| 字段 datalog_header_t :22 | `channel_count` | `channel_count` | `App/datalog.c`、`App/datalog.h` |
| 字段 datalog_header_t :22 | `noise` | `noise` | `App/datalog.c`、`App/datalog.h` |
| 字段 datalog_header_t :22 | `lost` | `lost` | `App/app.c`、`App/datalog.c`、`App/datalog.h` |
| 字段 datalog_header_t :22 | `min_contrast` | `min_contrast` | `App/datalog.c`、`App/datalog.h` |
| 字段 datalog_header_t :22 | `reserved` | `reserved` | `App/datalog.c`、`App/datalog.h`、`App/param.c`、`Core/Inc/adc.h`、`Core/Inc/dma.h`、`Core/Inc/gpio.h`、`Core/Inc/i2c.h`、`Core/Inc/main.h`、`Core/Inc/spi.h`、`Core/Inc/stm32h7xx_hal_conf.h`、`Core/Inc/stm32h7xx_it.h`、`Core/Inc/tim.h`、`Core/Inc/usart.h`、`Core/Src/adc.c`、`Core/Src/dma.c`、`Core/Src/gpio.c`、`Core/Src/i2c.c`、`Core/Src/main.c`、`Core/Src/spi.c`、`Core/Src/stm32h7xx_hal_msp.c`、`Core/Src/stm32h7xx_it.c`、`Core/Src/system_stm32h7xx.c`、`Core/Src/tim.c`、`Core/Src/usart.c` |
| 字段 datalog_header_t :22 | `black` | `black` | `App/datalog.c`、`App/datalog.h`、`App/gray_sensor.c`、`App/gray_sensor.h`、`tests/datalog_v2.py` |
| 字段 datalog_header_t :22 | `white` | `white` | `App/datalog.c`、`App/datalog.h`、`App/gray_sensor.c`、`App/gray_sensor.h`、`tests/datalog_v2.py` |
| 字段 datalog_header_t :22 | `weights` | `weights` | `App/control.c`、`App/control.h`、`App/datalog.c`、`App/datalog.h`、`App/electromagnetic_fsm.c`、`App/track_sensor.c`、`App/track_sensor.h`、`tests/control_equivalence.py` |
| 字段 datalog_header_t :22 | `em` | `em` | `App/app.c`、`App/control.c`、`App/datalog.c`、`App/datalog.h`、`tests/datalog_v2.py` |
| 字段 datalog_header_t :22 | `optical` | `optical` | `App/control.c`、`App/datalog.c`、`App/datalog.h`、`tests/datalog_v2.py` |
| 字段 datalog_header_t :22 | `kp_v` | `kp_v` | `App/datalog.c`、`App/datalog.h` |
| 字段 datalog_header_t :22 | `ki_v` | `ki_v` | `App/datalog.c`、`App/datalog.h` |
| 字段 datalog_header_t :22 | `fan_run` | `fan_run` | `App/datalog.c`、`App/datalog.h` |
| 字段 datalog_header_t :22 | `fan_idle` | `fan_idle` | `App/datalog.c`、`App/datalog.h` |
| 字段 datalog_header_t :22 | `gray_budget_us` | `gray_budget_us` | `App/datalog.c`、`App/datalog.h` |
| 字段 datalog_header_t :22 | `control_budget_us` | `control_budget_us` | `App/datalog.c`、`App/datalog.h` |
| 字段 datalog_header_t :22 | `run_delay_count` | `run_delay_count` | `App/datalog.c`、`App/datalog.h` |
| 字段 datalog_header_t :22 | `capacity` | `capacity` | `App/datalog.c`、`App/datalog.h`、`tests/datalog_v2.py` |
| 字段 datalog_header_t :22 | `datalog_div` | `datalog_div` | `App/datalog.c`、`App/datalog.h` |
| 字段 datalog_header_t :22 | `reserved_tail` | `reserved_tail` | `App/datalog.c`、`App/datalog.h` |
| 字段 datalog_status_t :38 | `active` | `active` | `App/datalog.c`、`App/datalog.h` |
| 字段 datalog_status_t :38 | `fault` | `fault` | `App/app.c`、`App/datalog.c`、`App/datalog.h`、`Core/Src/stm32h7xx_it.c`、`tests/datalog_v2.py` |
| 字段 datalog_status_t :38 | `reason` | `reason` | `App/app.c`、`App/app.h`、`App/control.c`、`App/control.h`、`App/datalog.c`、`App/datalog.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 字段 datalog_status_t :38 | `protection_flags` | `protection_flags` | `App/app.c`、`App/control.c`、`App/control.h`、`App/datalog.c`、`App/datalog.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 字段 datalog_status_t :38 | `accepted` | `accepted` | `App/app.c`、`App/datalog.c`、`App/datalog.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 字段 datalog_status_t :38 | `written` | `written` | `App/app.c`、`App/datalog.c`、`App/datalog.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 字段 datalog_status_t :38 | `rejected` | `rejected` | `App/app.c`、`App/datalog.c`、`App/datalog.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 字段 datalog_status_t :38 | `pending` | `pending` | `App/app.c`、`App/datalog.c`、`App/datalog.h`、`tests/datalog_v2.py` |

### App/electromagnetic_fsm.c

验证：V-01/02/03/04/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 函数 :9 | `ElectromagneticFSM_GetState` | `ElectromagneticFSM_GetState` | `App/control.c`、`App/datalog.c`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 函数 :10 | `ElectromagneticFSM_Reset` | `ElectromagneticFSM_Reset` | `App/control.c`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`tests/dual_mode_tracking.py` |
| 函数 :15 | `ElectromagneticFSM_Accumulate` | `ElectromagneticFSM_Accumulate` | `App/control.c`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`tests/dual_mode_tracking.py` |
| 私有函数 :17 | `distance` | `ElectromagneticFSM_GetDistance` | `App/electromagnetic_fsm.c` |
| 私有函数 :18 | `clear_distance` | `ElectromagneticFSM_ClearDistance` | `App/electromagnetic_fsm.c` |
| 私有函数 :20 | `clear_angle` | `ElectromagneticFSM_ClearAngle` | `App/electromagnetic_fsm.c` |
| 私有函数 :21 | `measure` | `ElectromagneticFSM_Measure` | `App/electromagnetic_fsm.c` |
| 私有函数 :28 | `direction` | `ElectromagneticFSM_UpdateDirection` | `App/electromagnetic_fsm.c` |
| 函数 :34 | `ElectromagneticFSM_Update` | `ElectromagneticFSM_Update` | `App/control.c`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`tests/dual_mode_tracking.py` |
| 对象 :7 | `s_em` | `s_em` | `App/electromagnetic_fsm.c`、`tests/control_equivalence.py` |
| 对象 :8 | `s_history` | `s_history` | `App/electromagnetic_fsm.c`、`App/optical_fsm.c` |

### App/electromagnetic_fsm.h

验证：V-01/02/03/04/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 类型 :4 | `electromagnetic_inputs_t` | `electromagnetic_inputs_t` | `App/control.c`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`tests/dual_mode_tracking.py` |
| 类型 :10 | `electromagnetic_state_t` | `electromagnetic_state_t` | `App/control.c`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :2 | `ELECTROMAGNETIC_FSM_H` | `SMARTCAR_APP_ELECTROMAGNETIC_FSM_H` | `App/electromagnetic_fsm.h` |
| 字段 electromagnetic_inputs_t :4 | `adc` | `adc` | `App/datalog.c`、`App/datalog.h`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`App/track_sensor.c`、`BSP/bsp.h`、`BSP/bsp_adc.c`、`Core/Inc/adc.h`、`Core/Src/adc.c`、`Core/Src/main.c`、`tests/dual_mode_tracking.py` |
| 字段 electromagnetic_inputs_t :4 | `dl1b_mm` | `dl1b_mm` | `App/datalog.c`、`App/datalog.h`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h` |
| 字段 electromagnetic_inputs_t :4 | `accel` | `accel` | `App/control.c`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`App/imu_proc.c`、`App/imu_proc.h`、`App/optical_fsm.c`、`App/optical_fsm.h`、`App/pid.c`、`App/pid.h` |
| 字段 electromagnetic_inputs_t :4 | `angle_delta` | `angle_delta` | `App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h` |
| 字段 electromagnetic_inputs_t :4 | `enabled` | `enabled` | `App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`Core/Src/system_stm32h7xx.c` |
| 字段 electromagnetic_state_t :10 | `kernel_state` | `kernel_state` | `App/control.c`、`App/control.h`、`App/dashboard.c`、`App/datalog.c`、`App/datalog.h`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`App/element.c`、`App/roundabout.c`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 字段 electromagnetic_state_t :10 | `roundabout_state` | `roundabout_state` | `App/control.c`、`App/datalog.c`、`App/datalog.h`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`App/roundabout.c`、`App/roundabout.h`、`tests/dual_mode_tracking.py` |
| 字段 electromagnetic_state_t :10 | `left_flag` | `left_flag` | `App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h` |
| 字段 electromagnetic_state_t :10 | `right_flag` | `right_flag` | `App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h` |
| 字段 electromagnetic_state_t :10 | `cask_flag` | `cask_flag` | `App/control.c`、`App/control.h`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`App/roundabout.c` |
| 字段 electromagnetic_state_t :10 | `reset_distance` | `reset_distance` | `App/control.c`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`tests/dual_mode_tracking.py` |
| 字段 electromagnetic_state_t :10 | `zero_angle` | `zero_angle` | `App/control.c`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`tests/dual_mode_tracking.py` |
| 字段 electromagnetic_state_t :10 | `left_distance` | `left_distance` | `App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`tests/dual_mode_tracking.py` |
| 字段 electromagnetic_state_t :10 | `right_distance` | `right_distance` | `App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`tests/dual_mode_tracking.py` |
| 字段 electromagnetic_state_t :10 | `angle` | `angle` | `App/control.c`、`App/control.h`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`App/element.c`、`App/imu_proc.c`、`App/imu_proc.h`、`App/pid.c`、`tests/control_equivalence.py` |
| 字段 electromagnetic_state_t :10 | `error` | `error` | `App/control.c`、`App/dashboard.c`、`App/datalog.c`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`App/gray_sensor.c`、`App/gray_sensor.h`、`App/menu.c`、`App/optical_fsm.c`、`App/optical_fsm.h`、`App/pid.c`、`App/pid.h`、`App/track_sensor.c`、`App/track_sensor.h`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 字段 electromagnetic_state_t :10 | `symmetry_x` | `symmetry_x` | `App/control.c`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`App/menu.c`、`App/track_sensor.c`、`App/track_sensor.h`、`tests/control_equivalence.py` |
| 字段 electromagnetic_state_t :10 | `symmetry_y` | `symmetry_y` | `App/control.c`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`App/element.c`、`App/menu.c`、`App/track_sensor.c`、`App/track_sensor.h`、`tests/control_equivalence.py` |
| 字段 electromagnetic_state_t :10 | `output` | `output` | `App/control.c`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`App/filter.c`、`App/filter.h`、`App/optical_fsm.c`、`App/optical_fsm.h`、`App/pid.c`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 字段 electromagnetic_state_t :10 | `steering` | `steering` | `App/control.c`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`tests/dual_mode_tracking.py` |
| 字段 electromagnetic_state_t :10 | `configuration` | `configuration` | `App/control.c`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`Core/Inc/stm32h7xx_hal_conf.h`、`Core/Src/system_stm32h7xx.c`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |

### App/element.c

验证：V-01/02/03/04/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 函数 :20 | `element_judge` | `Element_Judge` | `App/element.c`、`App/element.h` |
| 函数 :29 | `straight_judge` | `Element_JudgeStraight` | `App/element.c`、`App/element.h` |
| 函数 :35 | `crossroads_judge` | `Element_JudgeCrossroads` | `App/element.c`、`App/element.h` |
| 函数 :44 | `crossroads_out_judge` | `Element_JudgeCrossroadsExit` | `App/element.c`、`App/element.h` |
| 函数 :53 | `teeterboard_judge` | `Element_JudgeTeeterboard` | `App/element.c`、`App/element.h` |
| 函数 :63 | `teeterboard_out_judge` | `Element_JudgeTeeterboardExit` | `App/element.c`、`App/element.h` |
| 函数 :77 | `cask_judge` | `Element_JudgeCask` | `App/element.c`、`App/element.h` |
| 函数 :85 | `cask_out_judge` | `Element_JudgeCaskExit` | `App/element.c`、`App/element.h` |
| 对象声明 :15 | `time` | `g_app_control_ticks` | `App/app.c`、`App/app.h`、`App/app_config.h`、`App/control.c`、`App/element.c`、`App/optical_fsm.c`、`App/optical_fsm.h`、`Core/Inc/stm32h7xx_hal_conf.h`、`Core/Src/system_stm32h7xx.c`、`tests/dual_mode_tracking.py` |
| 对象 :17 | `in_time` | `g_element_enter_tick` | `App/element.c`、`App/element.h` |

### App/element.h

验证：V-01/02/03/04/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 宏/枚举 :1 | `_ELEMENT_H_` | `SMARTCAR_APP_ELEMENT_H` | `App/element.h` |
| 对象声明 :12 | `in_time` | `g_element_enter_tick` | `App/element.c`、`App/element.h` |

### App/filter.c

验证：V-01/02/03/04/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 函数 :13 | `lowpass_init` | `Filter_InitLowpass` | `App/app.c`、`App/filter.c`、`App/filter.h`、`App/menu.c`、`App/motor.c` |
| 函数 :23 | `lowpass_update` | `Filter_UpdateLowpass` | `App/app.c`、`App/filter.c`、`App/filter.h`、`App/menu.c`、`App/motor.c` |
| 函数 :30 | `gyro_hpf_init` | `Filter_InitHighpass` | `App/filter.c`、`App/filter.h`、`App/imu_proc.c` |
| 函数 :39 | `gyro_hpf_update` | `Filter_UpdateHighpass` | `App/filter.c`、`App/filter.h`、`App/imu_proc.c` |

### App/filter.h

验证：V-01/02/03/04/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 类型 :5 | `LowPassFilter` | `filter_lowpass_t` | `App/app.c`、`App/filter.c`、`App/filter.h`、`App/menu.c`、`App/motor.c` |
| 类型 :16 | `gyro_hpf_t` | `filter_highpass_t` | `App/filter.c`、`App/filter.h`、`App/imu_proc.c` |
| 宏/枚举 :2 | `_FILTER_H_` | `SMARTCAR_APP_FILTER_H` | `App/filter.h` |
| 字段 LowPassFilter :5 | `last_output` | `last_output` | `App/filter.c`、`App/filter.h` |
| 字段 LowPassFilter :5 | `alpha` | `alpha` | `App/filter.c`、`App/filter.h`、`App/motor.c`、`App/motor.h` |
| 字段 gyro_hpf_t :16 | `alpha` | `alpha` | `App/filter.c`、`App/filter.h`、`App/motor.c`、`App/motor.h` |
| 字段 gyro_hpf_t :16 | `last_input` | `last_input` | `App/filter.c`、`App/filter.h` |
| 字段 gyro_hpf_t :16 | `output` | `output` | `App/control.c`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`App/filter.c`、`App/filter.h`、`App/optical_fsm.c`、`App/optical_fsm.h`、`App/pid.c`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |

### App/gray_calibration.h

验证：V-01/02/03/04/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 宏/枚举 :2 | `GRAY_CALIBRATION_H` | `SMARTCAR_APP_GRAY_CALIBRATION_H` | `App/gray_calibration.h` |
| 对象 :7 | `gray_black` | `s_gray_black` | `App/gray_calibration.h`、`App/gray_sensor.c`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 对象 :11 | `gray_white` | `s_gray_white` | `App/gray_calibration.h`、`App/gray_sensor.c`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |

### App/gray_sensor.c

验证：V-01/02/03/04/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 函数 :10 | `Gray_GetState` | `Gray_GetState` | `App/app.c`、`App/control.c`、`App/datalog.c`、`App/gray_sensor.c`、`App/gray_sensor.h`、`tests/dual_mode_tracking.py` |
| 函数 :11 | `Gray_GetBlack` | `Gray_GetBlack` | `App/datalog.c`、`App/gray_sensor.c`、`App/gray_sensor.h` |
| 函数 :12 | `Gray_GetWhite` | `Gray_GetWhite` | `App/datalog.c`、`App/gray_sensor.c`、`App/gray_sensor.h` |
| 函数 :13 | `Gray_ValidateCalibration` | `Gray_ValidateCalibration` | `App/datalog.c`、`App/gray_sensor.c`、`App/gray_sensor.h`、`tests/dual_mode_tracking.py` |
| 函数 :25 | `Gray_CheckCalibration` | `Gray_CheckCalibration` | `App/app.c`、`App/datalog.c`、`App/gray_sensor.c`、`App/gray_sensor.h` |
| 函数 :38 | `Gray_Normalize` | `Gray_Normalize` | `App/datalog.c`、`App/gray_sensor.c`、`App/gray_sensor.h`、`tests/dual_mode_tracking.py` |
| 函数 :48 | `Gray_ComputeError` | `Gray_ComputeError` | `App/gray_sensor.c`、`App/gray_sensor.h`、`tests/dual_mode_tracking.py` |
| 函数 :62 | `Gray_ResetSequence` | `Gray_ResetSequence` | `App/app.c`、`App/gray_sensor.c`、`App/gray_sensor.h` |
| 函数 :63 | `Gray_Init` | `Gray_Init` | `App/app.c`、`App/gray_sensor.c`、`App/gray_sensor.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 函数 :67 | `Gray_Update` | `Gray_Update` | `App/app.c`、`App/control.c`、`App/gray_sensor.c`、`App/gray_sensor.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 对象 :9 | `s_gray` | `s_gray` | `App/gray_sensor.c` |

### App/gray_sensor.h

验证：V-01/02/03/04/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 类型 :4 | `gray_state_t` | `gray_state_t` | `App/control.c`、`App/datalog.c`、`App/gray_sensor.c`、`App/gray_sensor.h`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :2 | `GRAY_SENSOR_H` | `SMARTCAR_APP_GRAY_SENSOR_H` | `App/gray_sensor.h` |
| 字段 gray_state_t :4 | `raw` | `raw` | `App/datalog.c`、`App/gray_sensor.c`、`App/gray_sensor.h`、`App/track_sensor.c`、`App/track_sensor.h`、`BSP/bsp_gray.c`、`BSP/bsp_gray.h`、`BSP/bsp_key.c`、`BSP/bsp_sampler.c`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 字段 gray_state_t :4 | `percent` | `percent` | `App/gray_sensor.c`、`App/gray_sensor.h`、`BSP/bsp_lcd.c`、`BSP/bsp_lcd.h`、`tests/dual_mode_tracking.py` |
| 字段 gray_state_t :4 | `strength` | `strength` | `App/gray_sensor.c`、`App/gray_sensor.h` |
| 字段 gray_state_t :4 | `error` | `error` | `App/control.c`、`App/dashboard.c`、`App/datalog.c`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`App/gray_sensor.c`、`App/gray_sensor.h`、`App/menu.c`、`App/optical_fsm.c`、`App/optical_fsm.h`、`App/pid.c`、`App/pid.h`、`App/track_sensor.c`、`App/track_sensor.h`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 字段 gray_state_t :4 | `sequence` | `sequence` | `App/datalog.c`、`App/gray_sensor.c`、`App/gray_sensor.h`、`BSP/bsp_gray.c`、`BSP/bsp_gray.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 字段 gray_state_t :4 | `scan_cycles` | `scan_cycles` | `App/gray_sensor.c`、`App/gray_sensor.h`、`BSP/bsp_gray.c`、`BSP/bsp_gray.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 字段 gray_state_t :4 | `calibration_valid` | `calibration_valid` | `App/app.c`、`App/control.c`、`App/datalog.c`、`App/gray_sensor.c`、`App/gray_sensor.h`、`tests/dual_mode_tracking.py` |
| 字段 gray_state_t :4 | `line_valid` | `line_valid` | `App/app.c`、`App/control.c`、`App/datalog.c`、`App/gray_sensor.c`、`App/gray_sensor.h`、`tests/dual_mode_tracking.py` |
| 字段 gray_state_t :4 | `acquisition_status` | `acquisition_status` | `App/app.c`、`App/control.c`、`App/datalog.c`、`App/gray_sensor.c`、`App/gray_sensor.h`、`tests/dual_mode_tracking.py` |

### App/imu_proc.c

验证：V-01/02/03/04/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 函数 :15 | `IMU_GetAngleDeltaX` | `IMU_GetAngleDeltaX` | `App/control.c`、`App/imu_proc.c`、`App/imu_proc.h` |
| 函数 :19 | `IMU_GetState` | `IMU_GetState` | `App/control.c`、`App/dashboard.c`、`App/element.c`、`App/imu_proc.c`、`App/imu_proc.h`、`App/menu.c`、`App/pid.c`、`tests/control_equivalence.py` |
| 函数 :24 | `IMU_ZeroAngleX` | `IMU_ZeroAngleX` | `App/control.c`、`App/imu_proc.c`、`App/imu_proc.h`、`App/roundabout.c`、`tests/dual_mode_tracking.py` |
| 函数 :50 | `imu_proc_init` | `IMU_Init` | `App/app.c`、`App/imu_proc.c`、`App/imu_proc.h`、`tests/control_equivalence.py` |
| 函数 :63 | `read_accel_velocity` | `IMU_UpdateAccelVelocity` | `App/imu_proc.c`、`App/imu_proc.h` |
| 函数 :85 | `read_gyro_angle` | `IMU_UpdateGyroAngle` | `App/app.c`、`App/imu_proc.c`、`App/imu_proc.h`、`tests/control_equivalence.py` |
| 函数 :109 | `accel_calibrate` | `IMU_CalibrateAccel` | `App/imu_proc.c`、`App/imu_proc.h` |
| 函数 :133 | `gyro_calibrate` | `IMU_CalibrateGyro` | `App/app.c`、`App/imu_proc.c`、`App/imu_proc.h`、`tests/control_equivalence.py` |
| 函数 :155 | `IMU_ResetRunState` | `IMU_ResetRunState` | `App/app.c`、`App/control.c`、`App/imu_proc.c`、`App/imu_proc.h`、`tests/control_equivalence.py` |
| 对象 :13 | `s_imu` | `s_imu` | `App/imu_proc.c` |
| 对象 :14 | `s_angle_delta_x` | `s_angle_delta_x` | `App/imu_proc.c` |
| 对象 :16 | `s_gyro_hpf_x` | `s_gyro_hpf_x` | `App/imu_proc.c` |
| 对象 :17 | `s_gyro_hpf_y` | `s_gyro_hpf_y` | `App/imu_proc.c` |
| 对象 :29 | `accel_offset_x` | `s_accel_offset_x` | `App/imu_proc.c` |
| 对象 :30 | `accel_offset_y` | `s_accel_offset_y` | `App/imu_proc.c` |
| 对象 :31 | `accel_offset_z` | `s_accel_offset_z` | `App/imu_proc.c` |
| 对象 :32 | `gyro_offset_x` | `s_gyro_offset_x` | `App/imu_proc.c` |
| 对象 :33 | `gyro_offset_y` | `s_gyro_offset_y` | `App/imu_proc.c` |
| 对象 :34 | `gyro_offset_z` | `s_gyro_offset_z` | `App/imu_proc.c` |
| 对象 :37 | `s_accel_last_x` | `s_accel_last_x` | `App/imu_proc.c` |
| 对象 :38 | `s_accel_last_y` | `s_accel_last_y` | `App/imu_proc.c` |
| 对象 :39 | `s_accel_last_z` | `s_accel_last_z` | `App/imu_proc.c` |
| 对象 :40 | `s_gyro_last_x` | `s_gyro_last_x` | `App/imu_proc.c` |
| 对象 :41 | `s_gyro_last_y` | `s_gyro_last_y` | `App/imu_proc.c` |
| 对象 :42 | `s_gyro_last_z` | `s_gyro_last_z` | `App/imu_proc.c` |

### App/imu_proc.h

验证：V-01/02/03/04/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 类型 :7 | `imu_state_t` | `imu_state_t` | `App/imu_proc.c`、`App/imu_proc.h`、`tests/control_equivalence.py` |
| 宏/枚举 :2 | `_IMU_PROC_H_` | `SMARTCAR_APP_IMU_PROC_H` | `App/imu_proc.h` |
| 字段 imu_state_t :7 | `accel` | `accel` | `App/control.c`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`App/imu_proc.c`、`App/imu_proc.h`、`App/optical_fsm.c`、`App/optical_fsm.h`、`App/pid.c`、`App/pid.h` |
| 字段 imu_state_t :7 | `gyro` | `gyro` | `App/dashboard.c`、`App/imu_proc.c`、`App/imu_proc.h`、`App/menu.c`、`App/pid.c`、`tests/control_equivalence.py` |
| 字段 imu_state_t :7 | `velocity` | `velocity` | `App/imu_proc.c`、`App/imu_proc.h` |
| 字段 imu_state_t :7 | `angle` | `angle` | `App/control.c`、`App/control.h`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`App/element.c`、`App/imu_proc.c`、`App/imu_proc.h`、`App/pid.c`、`tests/control_equivalence.py` |
| 字段 imu_state_t :7 | `raw_gyro` | `raw_gyro` | `App/imu_proc.c`、`App/imu_proc.h` |
| 字段 imu_state_t :7 | `raw_accel` | `raw_accel` | `App/imu_proc.c`、`App/imu_proc.h` |

### App/menu.c

验证：V-01/02/03/05/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 私有函数 :63 | `key_read_avg` | `Menu_ReadKeyAverage` | `App/menu.c` |
| 函数 :77 | `key_scan` | `Menu_ScanKey` | `App/menu.c`、`App/menu.h` |
| 函数 :140 | `key_action` | `Menu_HandleKey` | `App/menu.c`、`tests/dual_mode_tracking.py` |
| 私有函数 :250 | `menu_draw_content` | `Menu_DrawContent` | `App/menu.c` |
| 私有函数 :326 | `menu_draw_cursor` | `Menu_DrawCursor` | `App/menu.c` |
| 函数 :342 | `menu` | `Menu_Update` | `App/app.c`、`App/menu.c`、`App/menu.h` |
| 宏/枚举 :5 | `LCD_COL_LABEL` | `MENU_LCD_COL_LABEL` | `App/menu.c` |
| 宏/枚举 :5 | `LCD_COL_VAL` | `MENU_LCD_COL_VAL` | `App/menu.c` |
| 宏/枚举 :6 | `LCD_COL_CURSOR` | `MENU_LCD_COL_CURSOR` | `App/menu.c` |
| 宏/枚举 :7 | `LCD_ROW` | `MENU_LCD_ROW` | `App/menu.c` |
| 宏/枚举 :7 | `TXT_FG` | `MENU_TXT_FG` | `App/menu.c` |
| 宏/枚举 :9 | `TXT_BG` | `MENU_TXT_BG` | `App/menu.c` |
| 宏/枚举 :10 | `CUR_FG` | `MENU_CUR_FG` | `App/menu.c` |
| 对象 :41 | `page` | `g_menu_page` | `App/menu.c`、`App/menu.h`、`App/param.c`、`tests/dual_mode_tracking.py` |
| 对象 :42 | `arrow` | `g_menu_arrow` | `App/menu.c`、`App/menu.h`、`App/param.c`、`tests/dual_mode_tracking.py` |
| 对象 :43 | `mode` | `s_mode` | `App/app.c`、`App/control.c`、`App/control.h`、`App/dashboard.c`、`App/datalog.c`、`App/datalog.h`、`App/menu.c`、`App/param.c`、`App/param.h`、`App/wireless.c`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 对象 :44 | `key_adc1` | `g_menu_key_adc1` | `App/menu.c`、`App/menu.h` |
| 对象 :45 | `key_adc2` | `g_menu_key_adc2` | `App/menu.c`、`App/menu.h` |
| 对象 :47 | `param_dirty` | `s_param_dirty` | `App/menu.c` |
| 对象 :48 | `fan_edit_idle` | `s_fan_edit_idle` | `App/menu.c` |
| 对象 :51 | `last_displayed_page` | `s_last_displayed_page` | `App/menu.c` |
| 对象 :52 | `last_displayed_arrow` | `s_last_displayed_arrow` | `App/menu.c` |
| 对象 :54 | `filt_key1` | `s_filt_key1` | `App/menu.c` |
| 对象 :55 | `filt_key2` | `s_filt_key2` | `App/menu.c` |
| 对象 :56 | `filt_inited` | `s_filt_inited` | `App/menu.c` |
| 局部 menu :344 | `last_key` | `s_last_key` | `App/menu.c` |

### App/menu.h

验证：V-01/02/03/05/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 宏/枚举 :2 | `__MENU_H` | `SMARTCAR_APP_MENU_H` | `App/menu.h` |
| 宏/枚举 :4 | `OK` | `MENU_KEY_OK` | `App/menu.c`、`App/menu.h` |
| 宏/枚举 :6 | `UP` | `MENU_KEY_UP` | `App/menu.c`、`App/menu.h` |
| 宏/枚举 :7 | `DOWN` | `MENU_KEY_DOWN` | `App/menu.c`、`App/menu.h` |
| 宏/枚举 :8 | `BACK` | `MENU_KEY_BACK` | `App/menu.c`、`App/menu.h` |
| 宏/枚举 :9 | `LEFT` | `MENU_KEY_LEFT` | `App/menu.c`、`App/menu.h` |
| 宏/枚举 :9 | `RIGHT` | `MENU_KEY_RIGHT` | `App/menu.c`、`App/menu.h`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :10 | `RST` | `MENU_KEY_RST` | `App/menu.c`、`App/menu.h`、`BSP/bsp_lcd.h`、`BSP/lcd/lcd.c`、`BSP/lcd/lcd.h` |
| 宏/枚举 :11 | `ADJUST1` | `MENU_KEY_ADJUST1` | `App/menu.c`、`App/menu.h`、`App/param.h` |
| 宏/枚举 :11 | `ADJUST2` | `MENU_KEY_ADJUST2` | `App/menu.c`、`App/menu.h`、`App/param.h`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :12 | `FOUR` | `MENU_KEY_FOUR` | `App/menu.c`、`App/menu.h` |
| 宏/枚举 :13 | `PAGE_HOME` | `MENU_PAGE_HOME` | `App/menu.c`、`App/menu.h`、`App/param.c` |
| 宏/枚举 :13 | `PAGE_ADC_ERR` | `MENU_PAGE_ADC_ERR` | `App/menu.c`、`App/menu.h`、`App/param.c` |
| 宏/枚举 :14 | `PAGE_SPD_DIS` | `MENU_PAGE_SPD_DIS` | `App/menu.c`、`App/menu.h`、`App/param.c` |
| 宏/枚举 :15 | `PAGE_GYRO` | `MENU_PAGE_GYRO` | `App/menu.c`、`App/menu.h`、`App/param.c` |
| 宏/枚举 :16 | `PAGE_ADJUST1` | `MENU_PAGE_ADJUST1` | `App/menu.c`、`App/menu.h`、`App/param.c` |
| 宏/枚举 :16 | `PAGE_ADJUST2` | `MENU_PAGE_ADJUST2` | `App/menu.c`、`App/menu.h`、`App/param.c` |
| 对象声明 :26 | `page` | `g_menu_page` | `App/menu.c`、`App/menu.h`、`App/param.c`、`tests/dual_mode_tracking.py` |
| 对象声明 :27 | `arrow` | `g_menu_arrow` | `App/menu.c`、`App/menu.h`、`App/param.c`、`tests/dual_mode_tracking.py` |
| 对象声明 :28 | `key_adc1` | `g_menu_key_adc1` | `App/menu.c`、`App/menu.h` |
| 对象声明 :29 | `key_adc2` | `g_menu_key_adc2` | `App/menu.c`、`App/menu.h` |

### App/motor.c

验证：V-01/02/03/04/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 函数 :21 | `Motor_GetState` | `Motor_GetState` | `App/app.c`、`App/control.c`、`App/dashboard.c`、`App/menu.c`、`App/motor.c`、`App/motor.h`、`App/pid.c`、`App/roundabout.c`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 函数 :26 | `Motor_SetTargets` | `Motor_SetTargets` | `App/motor.c`、`App/motor.h`、`tests/control_equivalence.py` |
| 函数 :32 | `Motor_SetBaseSpeed` | `Motor_SetBaseSpeed` | `App/control.c`、`App/motor.c`、`App/motor.h`、`App/param.c`、`tests/control_equivalence.py` |
| 函数 :37 | `Motor_SetFanDuty` | `Motor_SetFanDuty` | `App/motor.c`、`App/motor.h`、`App/param.c` |
| 函数 :42 | `Motor_SetFanDutyIdle` | `Motor_SetFanDutyIdle` | `App/motor.c`、`App/motor.h`、`App/param.c` |
| 函数 :47 | `Motor_ResetDistance` | `Motor_ResetDistance` | `App/control.c`、`App/motor.c`、`App/motor.h`、`App/roundabout.c`、`tests/dual_mode_tracking.py` |
| 函数 :58 | `speed_control` | `Motor_DistributeSpeed` | `App/control.c`、`App/motor.c`、`App/motor.h`、`App/pid.c`、`App/roundabout.c`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 函数 :76 | `motor_control` | `Motor_UpdateControl` | `App/app.c`、`App/motor.c`、`App/motor.h` |
| 函数 :114 | `read_encoder` | `Motor_ReadEncoder` | `App/app.c`、`App/motor.c`、`App/motor.h`、`tests/control_equivalence.py` |
| 函数 :133 | `motor_init` | `Motor_Init` | `App/motor.c`、`App/motor.h` |
| 函数 :143 | `encoder_init` | `Motor_InitEncoder` | `App/app.c`、`App/motor.c`、`App/motor.h`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 函数 :152 | `Motor_ResetRunState` | `Motor_ResetRunState` | `App/app.c`、`App/control.c`、`App/motor.c`、`App/motor.h`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 函数 :164 | `Motor_EmergencyStop` | `Motor_EmergencyStop` | `App/app.c`、`App/motor.c`、`App/motor.h` |
| 对象 :14 | `s_motor` | `s_motor` | `App/motor.c` |
| 对象 :18 | `s_filter_left` | `s_filter_left` | `App/motor.c` |
| 对象 :19 | `s_filter_right` | `s_filter_right` | `App/motor.c` |
| 局部 motor_control :78 | `speed_outL` | `speed_out_left` | `App/motor.c` |
| 局部 motor_control :79 | `speed_outR` | `speed_out_right` | `App/motor.c` |
| 局部 read_encoder :116 | `encoder_L` | `encoder_left` | `App/motor.c` |
| 局部 read_encoder :117 | `encoder_R` | `encoder_right` | `App/motor.c` |

### App/motor.h

验证：V-01/02/03/04/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 类型 :11 | `motor_wheel_state_t` | `motor_wheel_state_t` | `App/motor.h` |
| 类型 :17 | `motor_state_t` | `motor_state_t` | `App/motor.c`、`App/motor.h`、`App/pid.c`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :1 | `_MOTOR_H_` | `SMARTCAR_APP_MOTOR_H` | `App/motor.h` |
| 字段 motor_wheel_state_t :11 | `target_speed` | `target_speed` | `App/motor.c`、`App/motor.h`、`App/pid.c`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 字段 motor_wheel_state_t :11 | `real_speed` | `real_speed` | `App/control.c`、`App/dashboard.c`、`App/menu.c`、`App/motor.c`、`App/motor.h`、`App/pid.c`、`tests/dual_mode_tracking.py` |
| 字段 motor_wheel_state_t :11 | `distance` | `distance` | `App/dashboard.c`、`App/electromagnetic_fsm.c`、`App/menu.c`、`App/motor.c`、`App/motor.h`、`App/roundabout.c`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 字段 motor_state_t :17 | `left` | `left` | `App/control.c`、`App/dashboard.c`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`App/menu.c`、`App/motor.c`、`App/motor.h`、`App/pid.c`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 字段 motor_state_t :17 | `right` | `right` | `App/control.c`、`App/dashboard.c`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`App/menu.c`、`App/motor.c`、`App/motor.h`、`App/pid.c`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 字段 motor_state_t :17 | `distance` | `distance` | `App/dashboard.c`、`App/electromagnetic_fsm.c`、`App/menu.c`、`App/motor.c`、`App/motor.h`、`App/roundabout.c`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 字段 motor_state_t :17 | `base_speed` | `base_speed` | `App/control.c`、`App/control.h`、`App/datalog.c`、`App/datalog.h`、`App/menu.c`、`App/motor.c`、`App/motor.h`、`App/optical_fsm.c`、`App/optical_fsm.h`、`App/param.c`、`App/param.h`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 字段 motor_state_t :17 | `fan_duty` | `fan_duty` | `App/app.c`、`App/menu.c`、`App/motor.c`、`App/motor.h` |
| 字段 motor_state_t :17 | `fan_duty_idle` | `fan_duty_idle` | `App/app.c`、`App/menu.c`、`App/motor.c`、`App/motor.h` |
| 字段 motor_state_t :17 | `alpha` | `alpha` | `App/filter.c`、`App/filter.h`、`App/motor.c`、`App/motor.h` |

### App/optical_fsm.c

验证：V-01/02/03/04/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 函数 :5 | `OpticalFSM_GetState` | `OpticalFSM_GetState` | `App/control.c`、`App/datalog.c`、`App/optical_fsm.c`、`App/optical_fsm.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 函数 :6 | `OpticalFSM_Reset` | `OpticalFSM_Reset` | `App/control.c`、`App/optical_fsm.c`、`App/optical_fsm.h`、`tests/dual_mode_tracking.py` |
| 函数 :7 | `OpticalFSM_Update` | `OpticalFSM_Update` | `App/control.c`、`App/optical_fsm.c`、`App/optical_fsm.h`、`tests/dual_mode_tracking.py` |
| 对象 :3 | `s_optical` | `s_optical` | `App/optical_fsm.c` |
| 对象 :4 | `s_history` | `s_history` | `App/electromagnetic_fsm.c`、`App/optical_fsm.c` |

### App/optical_fsm.h

验证：V-01/02/03/04/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 类型 :4 | `optical_phase_t` | `optical_phase_t` | `App/optical_fsm.h` |
| 类型 :5 | `optical_inputs_t` | `optical_inputs_t` | `App/control.c`、`App/optical_fsm.c`、`App/optical_fsm.h`、`tests/dual_mode_tracking.py` |
| 类型 :12 | `optical_state_t` | `optical_state_t` | `App/control.c`、`App/optical_fsm.c`、`App/optical_fsm.h`、`tests/datalog_v2.py` |
| 宏/枚举 :2 | `OPTICAL_FSM_H` | `SMARTCAR_APP_OPTICAL_FSM_H` | `App/optical_fsm.h` |
| 宏/枚举 :4 | `OPTICAL_WAIT` | `OPTICAL_WAIT` | `App/optical_fsm.c`、`App/optical_fsm.h`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :4 | `OPTICAL_TRACKING` | `OPTICAL_TRACKING` | `App/control.c`、`App/optical_fsm.c`、`App/optical_fsm.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :4 | `OPTICAL_LOST` | `OPTICAL_LOST` | `App/optical_fsm.c`、`App/optical_fsm.h`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :4 | `OPTICAL_STOPPED` | `OPTICAL_STOPPED` | `App/optical_fsm.c`、`App/optical_fsm.h`、`tests/dual_mode_tracking.py` |
| 字段 optical_inputs_t :5 | `running` | `running` | `App/control.c`、`App/control.h`、`App/optical_fsm.c`、`App/optical_fsm.h` |
| 字段 optical_inputs_t :5 | `gray_valid` | `gray_valid` | `App/datalog.c`、`App/datalog.h`、`App/optical_fsm.c`、`App/optical_fsm.h`、`tests/dual_mode_tracking.py` |
| 字段 optical_inputs_t :5 | `em_valid` | `em_valid` | `App/optical_fsm.c`、`App/optical_fsm.h` |
| 字段 optical_inputs_t :5 | `stop_reason` | `stop_reason` | `App/control.c`、`App/control.h`、`App/datalog.c`、`App/datalog.h`、`App/optical_fsm.c`、`App/optical_fsm.h`、`tests/dual_mode_tracking.py` |
| 字段 optical_inputs_t :5 | `time` | `time` | `App/app.c`、`App/app.h`、`App/app_config.h`、`App/control.c`、`App/element.c`、`App/optical_fsm.c`、`App/optical_fsm.h`、`Core/Inc/stm32h7xx_hal_conf.h`、`Core/Src/system_stm32h7xx.c`、`tests/dual_mode_tracking.py` |
| 字段 optical_inputs_t :5 | `error` | `error` | `App/control.c`、`App/dashboard.c`、`App/datalog.c`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`App/gray_sensor.c`、`App/gray_sensor.h`、`App/menu.c`、`App/optical_fsm.c`、`App/optical_fsm.h`、`App/pid.c`、`App/pid.h`、`App/track_sensor.c`、`App/track_sensor.h`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 字段 optical_inputs_t :5 | `base_speed` | `base_speed` | `App/control.c`、`App/control.h`、`App/datalog.c`、`App/datalog.h`、`App/menu.c`、`App/motor.c`、`App/motor.h`、`App/optical_fsm.c`、`App/optical_fsm.h`、`App/param.c`、`App/param.h`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 字段 optical_inputs_t :5 | `accel` | `accel` | `App/control.c`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`App/imu_proc.c`、`App/imu_proc.h`、`App/optical_fsm.c`、`App/optical_fsm.h`、`App/pid.c`、`App/pid.h` |
| 字段 optical_inputs_t :5 | `gains` | `gains` | `App/optical_fsm.c`、`App/optical_fsm.h`、`App/param.c`、`App/pid.c`、`App/pid.h` |
| 字段 optical_state_t :12 | `state` | `state` | `App/control.c`、`App/control.h`、`App/datalog.c`、`App/optical_fsm.c`、`App/optical_fsm.h`、`App/pid.c`、`App/roundabout.c`、`App/roundabout.h`、`Core/Src/system_stm32h7xx.c`、`tests/control_equivalence.py`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 字段 optical_state_t :12 | `error` | `error` | `App/control.c`、`App/dashboard.c`、`App/datalog.c`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`App/gray_sensor.c`、`App/gray_sensor.h`、`App/menu.c`、`App/optical_fsm.c`、`App/optical_fsm.h`、`App/pid.c`、`App/pid.h`、`App/track_sensor.c`、`App/track_sensor.h`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 字段 optical_state_t :12 | `output` | `output` | `App/control.c`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`App/filter.c`、`App/filter.h`、`App/optical_fsm.c`、`App/optical_fsm.h`、`App/pid.c`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 字段 optical_state_t :12 | `base_speed` | `base_speed` | `App/control.c`、`App/control.h`、`App/datalog.c`、`App/datalog.h`、`App/menu.c`、`App/motor.c`、`App/motor.h`、`App/optical_fsm.c`、`App/optical_fsm.h`、`App/param.c`、`App/param.h`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |

### App/param.c

验证：V-01/02/03/05/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 私有函数 :95 | `crc16_ccitt` | `Param_CalculateCrc16` | `App/param.c` |
| 私有函数 :104 | `param_crc_v1` | `Param_CalculateCrcV1` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 私有函数 :105 | `param_crc_v2` | `Param_CalculateCrcV2` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 私有函数 :106 | `param_crc` | `Param_CalculateCrcV3` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 私有函数 :107 | `clamp_fan_duty` | `Param_ClampFanDuty` | `App/param.c` |
| 私有函数 :108 | `optical_defaults` | `Param_SetOpticalDefaults` | `App/param.c` |
| 私有函数 :116 | `param_set_defaults` | `Param_SetDefaults` | `App/param.c` |
| 私有函数 :125 | `load_banks` | `Param_LoadBanks` | `App/param.c` |
| 私有函数 :132 | `store_banks` | `Param_StoreBanks` | `App/param.c` |
| 函数 :144 | `Param_GetDirection` | `Param_GetDirection` | `App/control.c`、`App/datalog.c`、`App/menu.c`、`App/param.c`、`App/param.h`、`App/wireless.c`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 函数 :146 | `Param_ApplyCurrentDirection` | `Param_ApplyCurrentDirection` | `App/control.c`、`App/param.c`、`App/param.h`、`tests/dual_mode_tracking.py` |
| 函数 :152 | `Param_SetDirection` | `Param_SetDirection` | `App/menu.c`、`App/param.c`、`App/param.h`、`App/wireless.c`、`tests/dual_mode_tracking.py` |
| 函数 :159 | `Param_GetFan` | `Param_GetFan` | `App/datalog.c`、`App/param.c`、`App/param.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 函数 :165 | `Param_SetFan` | `Param_SetFan` | `App/menu.c`、`App/param.c`、`App/param.h` |
| 函数 :174 | `Param_SelectGear` | `Param_SelectGear` | `App/menu.c`、`App/param.c`、`App/param.h` |
| 私有函数 :180 | `param_write_mirror` | `Param_WriteMirror` | `App/param.c` |
| 函数 :187 | `Param_Load` | `Param_Load` | `App/app.c`、`App/param.c`、`App/param.h`、`tests/dual_mode_tracking.py` |
| 函数 :234 | `Param_MarkDirty` | `Param_MarkDirty` | `App/menu.c`、`App/param.c`、`App/param.h`、`App/wireless.c`、`tests/dual_mode_tracking.py` |
| 函数 :235 | `Param_TryFlushPending` | `Param_TryFlushPending` | `App/app.c`、`App/control.c`、`App/param.c`、`App/param.h`、`tests/dual_mode_tracking.py` |
| 函数 :244 | `Param_Save` | `Param_Save` | `App/menu.c`、`App/param.c`、`App/param.h` |
| 函数 :245 | `Param_ServiceSave` | `Param_ServiceSave` | `App/app.c`、`App/param.c`、`App/param.h`、`tests/dual_mode_tracking.py` |
| 函数 :247 | `Param_FlushPending` | `Param_FlushPending` | `App/param.c`、`App/param.h` |
| 类型 :13 | `param_store_v1_t` | `param_store_v1_t` | `App/param.c`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 类型 :34 | `param_store_v2_t` | `param_store_v2_t` | `App/param.c`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 类型 :57 | `param_store_t` | `param_store_t` | `App/param.c`、`tests/control_equivalence.py` |
| 类型 :87 | `param_v1_size` | `param_v1_size_assert_t` | `App/param.c` |
| 类型 :88 | `param_v2_size` | `param_v2_size_assert_t` | `App/param.c` |
| 类型 :89 | `param_v3_size` | `param_v3_size_assert_t` | `App/param.c` |
| 类型 :90 | `param_v3_offset` | `param_v3_offset_assert_t` | `App/param.c` |
| 宏/枚举 :5 | `PARAM_MAGIC` | `PARAM_MAGIC` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :7 | `PARAM_VERSION` | `PARAM_VERSION` | `App/param.c` |
| 对象 :91 | `s_param` | `s_param` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 对象 :92 | `s_direction` | `s_direction` | `App/param.c` |
| 对象 :93 | `s_dirty` | `s_dirty` | `App/param.c` |
| 对象 :93 | `s_flash_usable` | `s_flash_usable` | `App/param.c` |
| 对象 :94 | `s_dirty_tick` | `s_dirty_tick` | `App/param.c` |
| 字段 param_store_v1_t :13 | `magic` | `magic` | `App/datalog.c`、`App/datalog.h`、`App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_v1_t :13 | `version` | `version` | `App/datalog.c`、`App/datalog.h`、`App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_v1_t :13 | `crc16` | `crc16` | `App/datalog.c`、`App/datalog.h`、`App/param.c`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 字段 param_store_v1_t :13 | `page` | `page` | `App/menu.c`、`App/menu.h`、`App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_v1_t :13 | `arrow` | `arrow` | `App/menu.c`、`App/menu.h`、`App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_v1_t :13 | `reserved` | `reserved` | `App/datalog.c`、`App/datalog.h`、`App/param.c`、`Core/Inc/adc.h`、`Core/Inc/dma.h`、`Core/Inc/gpio.h`、`Core/Inc/i2c.h`、`Core/Inc/main.h`、`Core/Inc/spi.h`、`Core/Inc/stm32h7xx_hal_conf.h`、`Core/Inc/stm32h7xx_it.h`、`Core/Inc/tim.h`、`Core/Inc/usart.h`、`Core/Src/adc.c`、`Core/Src/dma.c`、`Core/Src/gpio.c`、`Core/Src/i2c.c`、`Core/Src/main.c`、`Core/Src/spi.c`、`Core/Src/stm32h7xx_hal_msp.c`、`Core/Src/stm32h7xx_it.c`、`Core/Src/system_stm32h7xx.c`、`Core/Src/tim.c`、`Core/Src/usart.c` |
| 字段 param_store_v1_t :13 | `KP_v` | `kp_v` | `App/datalog.c`、`App/param.c`、`App/pid.c`、`App/pid.h`、`App/wireless.c`、`tests/control_equivalence.py`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 字段 param_store_v1_t :13 | `KI_v` | `ki_v` | `App/datalog.c`、`App/param.c`、`App/pid.c`、`App/pid.h`、`App/wireless.c`、`tests/control_equivalence.py`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 字段 param_store_v1_t :13 | `KP_x_low` | `kp_x_low` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_v1_t :13 | `K2P_x_low` | `k2p_x_low` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_v1_t :13 | `KD_x_low` | `kd_x_low` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_v1_t :13 | `base_speed_low` | `base_speed_low` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_v1_t :13 | `fan_duty_low` | `fan_duty_low` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_v1_t :13 | `KP_x_high` | `kp_x_high` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_v1_t :13 | `K2P_x_high` | `k2p_x_high` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_v1_t :13 | `KD_x_high` | `kd_x_high` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_v1_t :13 | `base_speed_high` | `base_speed_high` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_v1_t :13 | `fan_duty_high` | `fan_duty_high` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_v2_t :34 | `magic` | `magic` | `App/datalog.c`、`App/datalog.h`、`App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_v2_t :34 | `version` | `version` | `App/datalog.c`、`App/datalog.h`、`App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_v2_t :34 | `crc16` | `crc16` | `App/datalog.c`、`App/datalog.h`、`App/param.c`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 字段 param_store_v2_t :34 | `page` | `page` | `App/menu.c`、`App/menu.h`、`App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_v2_t :34 | `arrow` | `arrow` | `App/menu.c`、`App/menu.h`、`App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_v2_t :34 | `reserved` | `reserved` | `App/datalog.c`、`App/datalog.h`、`App/param.c`、`Core/Inc/adc.h`、`Core/Inc/dma.h`、`Core/Inc/gpio.h`、`Core/Inc/i2c.h`、`Core/Inc/main.h`、`Core/Inc/spi.h`、`Core/Inc/stm32h7xx_hal_conf.h`、`Core/Inc/stm32h7xx_it.h`、`Core/Inc/tim.h`、`Core/Inc/usart.h`、`Core/Src/adc.c`、`Core/Src/dma.c`、`Core/Src/gpio.c`、`Core/Src/i2c.c`、`Core/Src/main.c`、`Core/Src/spi.c`、`Core/Src/stm32h7xx_hal_msp.c`、`Core/Src/stm32h7xx_it.c`、`Core/Src/system_stm32h7xx.c`、`Core/Src/tim.c`、`Core/Src/usart.c` |
| 字段 param_store_v2_t :34 | `KP_v` | `kp_v` | `App/datalog.c`、`App/param.c`、`App/pid.c`、`App/pid.h`、`App/wireless.c`、`tests/control_equivalence.py`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 字段 param_store_v2_t :34 | `KI_v` | `ki_v` | `App/datalog.c`、`App/param.c`、`App/pid.c`、`App/pid.h`、`App/wireless.c`、`tests/control_equivalence.py`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 字段 param_store_v2_t :34 | `KP_x_low` | `kp_x_low` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_v2_t :34 | `K2P_x_low` | `k2p_x_low` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_v2_t :34 | `KD_x_low` | `kd_x_low` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_v2_t :34 | `base_speed_low` | `base_speed_low` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_v2_t :34 | `fan_duty_low` | `fan_duty_low` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_v2_t :34 | `KP_x_high` | `kp_x_high` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_v2_t :34 | `K2P_x_high` | `k2p_x_high` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_v2_t :34 | `KD_x_high` | `kd_x_high` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_v2_t :34 | `base_speed_high` | `base_speed_high` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_v2_t :34 | `fan_duty_high` | `fan_duty_high` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_v2_t :34 | `fan_duty_idle_low` | `fan_duty_idle_low` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_v2_t :34 | `fan_duty_idle_high` | `fan_duty_idle_high` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_t :57 | `magic` | `magic` | `App/datalog.c`、`App/datalog.h`、`App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_t :57 | `version` | `version` | `App/datalog.c`、`App/datalog.h`、`App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_t :57 | `crc16` | `crc16` | `App/datalog.c`、`App/datalog.h`、`App/param.c`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 字段 param_store_t :57 | `page` | `page` | `App/menu.c`、`App/menu.h`、`App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_t :57 | `arrow` | `arrow` | `App/menu.c`、`App/menu.h`、`App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_t :57 | `reserved` | `reserved` | `App/datalog.c`、`App/datalog.h`、`App/param.c`、`Core/Inc/adc.h`、`Core/Inc/dma.h`、`Core/Inc/gpio.h`、`Core/Inc/i2c.h`、`Core/Inc/main.h`、`Core/Inc/spi.h`、`Core/Inc/stm32h7xx_hal_conf.h`、`Core/Inc/stm32h7xx_it.h`、`Core/Inc/tim.h`、`Core/Inc/usart.h`、`Core/Src/adc.c`、`Core/Src/dma.c`、`Core/Src/gpio.c`、`Core/Src/i2c.c`、`Core/Src/main.c`、`Core/Src/spi.c`、`Core/Src/stm32h7xx_hal_msp.c`、`Core/Src/stm32h7xx_it.c`、`Core/Src/system_stm32h7xx.c`、`Core/Src/tim.c`、`Core/Src/usart.c` |
| 字段 param_store_t :57 | `KP_v` | `kp_v` | `App/datalog.c`、`App/param.c`、`App/pid.c`、`App/pid.h`、`App/wireless.c`、`tests/control_equivalence.py`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 字段 param_store_t :57 | `KI_v` | `ki_v` | `App/datalog.c`、`App/param.c`、`App/pid.c`、`App/pid.h`、`App/wireless.c`、`tests/control_equivalence.py`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 字段 param_store_t :57 | `KP_x_low` | `kp_x_low` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_t :57 | `K2P_x_low` | `k2p_x_low` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_t :57 | `KD_x_low` | `kd_x_low` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_t :57 | `base_speed_low` | `base_speed_low` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_t :57 | `fan_duty_low` | `fan_duty_low` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_t :57 | `KP_x_high` | `kp_x_high` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_t :57 | `K2P_x_high` | `k2p_x_high` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_t :57 | `KD_x_high` | `kd_x_high` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_t :57 | `base_speed_high` | `base_speed_high` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_t :57 | `fan_duty_high` | `fan_duty_high` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_t :57 | `fan_duty_idle_low` | `fan_duty_idle_low` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_t :57 | `fan_duty_idle_high` | `fan_duty_idle_high` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_t :57 | `optical_kp_low` | `optical_kp_low` | `App/param.c`、`tests/dual_mode_tracking.py` |
| 字段 param_store_t :57 | `optical_k2p_low` | `optical_k2p_low` | `App/param.c` |
| 字段 param_store_t :57 | `optical_kd_low` | `optical_kd_low` | `App/param.c` |
| 字段 param_store_t :57 | `optical_speed_low` | `optical_speed_low` | `App/param.c` |
| 字段 param_store_t :57 | `optical_reserved_low` | `optical_reserved_low` | `App/param.c` |
| 字段 param_store_t :57 | `optical_kp_high` | `optical_kp_high` | `App/param.c` |
| 字段 param_store_t :57 | `optical_k2p_high` | `optical_k2p_high` | `App/param.c` |
| 字段 param_store_t :57 | `optical_kd_high` | `optical_kd_high` | `App/param.c` |
| 字段 param_store_t :57 | `optical_speed_high` | `optical_speed_high` | `App/param.c` |
| 字段 param_store_t :57 | `optical_reserved_high` | `optical_reserved_high` | `App/param.c` |

### App/param.h

验证：V-01/02/03/05/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 类型 :5 | `param_direction_t` | `param_direction_t` | `App/control.c`、`App/datalog.c`、`App/menu.c`、`App/param.c`、`App/param.h`、`App/wireless.c`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :2 | `__PARAM_H` | `SMARTCAR_APP_PARAM_H` | `App/param.h` |
| 宏/枚举 :12 | `PARAM_GEAR_LOW` | `PARAM_GEAR_LOW` | `App/menu.c`、`App/param.c`、`App/param.h` |
| 宏/枚举 :13 | `PARAM_GEAR_HIGH` | `PARAM_GEAR_HIGH` | `App/menu.c`、`App/param.h` |
| 字段 param_direction_t :5 | `kp` | `kp` | `App/control.c`、`App/datalog.c`、`App/datalog.h`、`App/menu.c`、`App/param.c`、`App/param.h`、`App/pid.c`、`App/pid.h`、`App/wireless.c`、`tests/dual_mode_tracking.py` |
| 字段 param_direction_t :5 | `k2p` | `k2p` | `App/control.c`、`App/datalog.c`、`App/datalog.h`、`App/menu.c`、`App/param.c`、`App/param.h`、`App/pid.c`、`App/pid.h` |
| 字段 param_direction_t :5 | `kd` | `kd` | `App/control.c`、`App/datalog.c`、`App/datalog.h`、`App/menu.c`、`App/param.c`、`App/param.h`、`App/pid.c`、`App/pid.h`、`App/wireless.c` |
| 字段 param_direction_t :5 | `base_speed` | `base_speed` | `App/control.c`、`App/control.h`、`App/datalog.c`、`App/datalog.h`、`App/menu.c`、`App/motor.c`、`App/motor.h`、`App/optical_fsm.c`、`App/optical_fsm.h`、`App/param.c`、`App/param.h`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |

### App/pid.c

验证：V-01/02/03/04/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 函数 :26 | `PID_GetState` | `PID_GetState` | `App/datalog.c`、`App/menu.c`、`App/param.c`、`App/pid.c`、`App/pid.h`、`App/roundabout.c`、`tests/control_equivalence.py`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 函数 :31 | `PID_SetGain` | `PID_SetGain` | `App/param.c`、`App/pid.c`、`App/pid.h`、`App/wireless.c`、`tests/control_equivalence.py` |
| 函数 :50 | `PID_SetTrackGains` | `PID_SetTrackGains` | `App/control.c`、`App/param.c`、`App/pid.c`、`App/pid.h`、`tests/control_equivalence.py` |
| 函数 :58 | `PID_SetAngleGains` | `PID_SetAngleGains` | `App/control.c`、`App/pid.c`、`App/pid.h` |
| 函数 :75 | `PID_DirectionStep` | `PID_DirectionStep` | `App/electromagnetic_fsm.c`、`App/optical_fsm.c`、`App/pid.c`、`App/pid.h` |
| 函数 :104 | `PID_track` | `PID_UpdateTrack` | `App/app.c`、`App/pid.c`、`App/pid.h`、`App/roundabout.c`、`tests/control_equivalence.py` |
| 函数 :117 | `PID_angle` | `PID_UpdateAngle` | `App/pid.c`、`App/pid.h` |
| 函数 :134 | `PID_L` | `PID_UpdateIncrementalLeft` | `App/pid.c`、`App/pid.h` |
| 函数 :163 | `PID_R` | `PID_UpdateIncrementalRight` | `App/pid.c`、`App/pid.h` |
| 私有函数 :190 | `PID_speed` | `PID_UpdateSpeed` | `App/pid.c` |
| 函数 :222 | `PID_L_pos` | `PID_UpdateSpeedLeft` | `App/motor.c`、`App/pid.c`、`App/pid.h`、`tests/control_equivalence.py` |
| 函数 :230 | `PID_R_pos` | `PID_UpdateSpeedRight` | `App/motor.c`、`App/pid.c`、`App/pid.h`、`tests/control_equivalence.py` |
| 函数 :239 | `PID_ResetSpeed` | `PID_ResetSpeed` | `App/motor.c`、`App/pid.c`、`App/pid.h` |
| 函数 :251 | `PID_ResetAll` | `PID_ResetAll` | `App/app.c`、`App/control.c`、`App/pid.c`、`App/pid.h`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 类型 :18 | `speed_pi_state_t` | `pid_speed_pi_state_t` | `App/pid.c` |
| 对象 :16 | `s_pid` | `s_pid` | `App/pid.c` |
| 对象 :23 | `s_speed_left` | `s_speed_left` | `App/pid.c` |
| 对象 :24 | `s_speed_right` | `s_speed_right` | `App/pid.c` |
| 对象 :66 | `s_track_error_last` | `s_track_error_last` | `App/pid.c` |
| 对象 :67 | `s_angle_err_last` | `s_angle_err_last` | `App/pid.c` |
| 字段 speed_pi_state_t :18 | `integral` | `integral` | `App/pid.c` |
| 字段 speed_pi_state_t :18 | `last_target` | `last_target` | `App/pid.c` |
| 局部 PID_DirectionStep :80 | `P_out` | `p_out` | `App/pid.c` |
| 局部 PID_DirectionStep :81 | `P2_out` | `p2_out` | `App/pid.c` |
| 局部 PID_DirectionStep :82 | `D_out` | `d_out` | `App/pid.c` |
| 局部 PID_DirectionStep :83 | `D2_out` | `d2_out` | `App/pid.c` |
| 局部 PID_DirectionStep :85 | `PID_out` | `pid_out` | `App/pid.c` |
| 局部 PID_L :136 | `P_outL` | `s_p_out_left` | `App/pid.c` |
| 局部 PID_L :137 | `I_outL` | `s_i_out_left` | `App/pid.c` |
| 局部 PID_L :138 | `D_outL` | `s_d_out_left` | `App/pid.c` |
| 局部 PID_L :139 | `PID_sumL` | `s_pid_sum_left` | `App/pid.c` |
| 局部 PID_L :141 | `errorL` | `s_error_left` | `App/pid.c` |
| 局部 PID_L :142 | `Last_errorL` | `s_last_error_left` | `App/pid.c` |
| 局部 PID_R :165 | `P_outR` | `s_p_out_right` | `App/pid.c` |
| 局部 PID_R :166 | `I_outR` | `s_i_out_right` | `App/pid.c` |
| 局部 PID_R :167 | `D_outR` | `s_d_out_right` | `App/pid.c` |
| 局部 PID_R :168 | `PID_sumR` | `s_pid_sum_right` | `App/pid.c` |
| 局部 PID_R :170 | `errorR` | `s_error_right` | `App/pid.c` |
| 局部 PID_R :171 | `Last_errorR` | `s_last_error_right` | `App/pid.c` |

### App/pid.h

验证：V-01/02/03/04/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 类型 :6 | `pid_track_gains_t` | `pid_track_gains_t` | `App/control.h`、`App/optical_fsm.h`、`App/param.c`、`App/pid.c`、`App/pid.h`、`tests/control_equivalence.py` |
| 类型 :13 | `pid_angle_gains_t` | `pid_angle_gains_t` | `App/control.h`、`App/pid.c`、`App/pid.h` |
| 类型 :19 | `pid_gain_id_t` | `pid_gain_id_t` | `App/pid.c`、`App/pid.h` |
| 类型 :26 | `pid_state_t` | `pid_state_t` | `App/pid.c`、`App/pid.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :2 | `_PID_H_` | `SMARTCAR_APP_PID_H` | `App/pid.h` |
| 宏/枚举 :40 | `MAX_DIR_OUT` | `PID_MAX_DIR_OUT` | `App/pid.c`、`App/pid.h` |
| 宏/枚举 :42 | `MAX_SPD_OUT` | `PID_MAX_SPD_OUT` | `App/pid.c`、`App/pid.h` |
| 宏/枚举 :43 | `MOTOR_DEAD_ZONE_L` | `PID_MOTOR_DEAD_ZONE_LEFT` | `App/pid.c`、`App/pid.h` |
| 宏/枚举 :45 | `MOTOR_DEAD_ZONE_R` | `PID_MOTOR_DEAD_ZONE_RIGHT` | `App/pid.c`、`App/pid.h` |
| 宏/枚举 :19 | `PID_GAIN_KP_V` | `PID_GAIN_KP_V` | `App/param.c`、`App/pid.c`、`App/pid.h`、`App/wireless.c`、`tests/control_equivalence.py` |
| 宏/枚举 :19 | `PID_GAIN_KI_V` | `PID_GAIN_KI_V` | `App/param.c`、`App/pid.c`、`App/pid.h`、`App/wireless.c`、`tests/control_equivalence.py` |
| 宏/枚举 :19 | `PID_GAIN_KD_V` | `PID_GAIN_KD_V` | `App/pid.c`、`App/pid.h` |
| 宏/枚举 :19 | `PID_GAIN_KP_X` | `PID_GAIN_KP_X` | `App/pid.c`、`App/pid.h` |
| 宏/枚举 :19 | `PID_GAIN_K2P_X` | `PID_GAIN_K2P_X` | `App/pid.c`、`App/pid.h` |
| 宏/枚举 :19 | `PID_GAIN_KI_X` | `PID_GAIN_KI_X` | `App/pid.c`、`App/pid.h` |
| 宏/枚举 :19 | `PID_GAIN_KD_X` | `PID_GAIN_KD_X` | `App/pid.c`、`App/pid.h` |
| 宏/枚举 :19 | `PID_GAIN_K2D_X` | `PID_GAIN_K2D_X` | `App/pid.c`、`App/pid.h` |
| 宏/枚举 :19 | `PID_GAIN_KP_A` | `PID_GAIN_KP_A` | `App/pid.c`、`App/pid.h` |
| 宏/枚举 :19 | `PID_GAIN_KD_A` | `PID_GAIN_KD_A` | `App/pid.c`、`App/pid.h` |
| 宏/枚举 :19 | `PID_GAIN_KG_A` | `PID_GAIN_KG_A` | `App/pid.c`、`App/pid.h` |
| 字段 pid_track_gains_t :6 | `kp` | `kp` | `App/control.c`、`App/datalog.c`、`App/datalog.h`、`App/menu.c`、`App/param.c`、`App/param.h`、`App/pid.c`、`App/pid.h`、`App/wireless.c`、`tests/dual_mode_tracking.py` |
| 字段 pid_track_gains_t :6 | `k2p` | `k2p` | `App/control.c`、`App/datalog.c`、`App/datalog.h`、`App/menu.c`、`App/param.c`、`App/param.h`、`App/pid.c`、`App/pid.h` |
| 字段 pid_track_gains_t :6 | `kd` | `kd` | `App/control.c`、`App/datalog.c`、`App/datalog.h`、`App/menu.c`、`App/param.c`、`App/param.h`、`App/pid.c`、`App/pid.h`、`App/wireless.c` |
| 字段 pid_track_gains_t :6 | `k2d` | `k2d` | `App/pid.c`、`App/pid.h` |
| 字段 pid_angle_gains_t :13 | `kp` | `kp` | `App/control.c`、`App/datalog.c`、`App/datalog.h`、`App/menu.c`、`App/param.c`、`App/param.h`、`App/pid.c`、`App/pid.h`、`App/wireless.c`、`tests/dual_mode_tracking.py` |
| 字段 pid_angle_gains_t :13 | `kd` | `kd` | `App/control.c`、`App/datalog.c`、`App/datalog.h`、`App/menu.c`、`App/param.c`、`App/param.h`、`App/pid.c`、`App/pid.h`、`App/wireless.c` |
| 字段 pid_angle_gains_t :13 | `kg` | `kg` | `App/pid.c`、`App/pid.h` |
| 字段 pid_state_t :26 | `KP_v` | `kp_v` | `App/datalog.c`、`App/param.c`、`App/pid.c`、`App/pid.h`、`App/wireless.c`、`tests/control_equivalence.py`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 字段 pid_state_t :26 | `KI_v` | `ki_v` | `App/datalog.c`、`App/param.c`、`App/pid.c`、`App/pid.h`、`App/wireless.c`、`tests/control_equivalence.py`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 字段 pid_state_t :26 | `KD_v` | `kd_v` | `App/pid.c`、`App/pid.h` |
| 字段 pid_state_t :26 | `KP_x` | `kp_x` | `App/menu.c`、`App/pid.c`、`App/pid.h`、`App/wireless.c`、`tests/control_equivalence.py` |
| 字段 pid_state_t :26 | `K2P_x` | `k2p_x` | `App/menu.c`、`App/pid.c`、`App/pid.h`、`tests/control_equivalence.py` |
| 字段 pid_state_t :26 | `KI_x` | `ki_x` | `App/pid.c`、`App/pid.h` |
| 字段 pid_state_t :26 | `KD_x` | `kd_x` | `App/menu.c`、`App/pid.c`、`App/pid.h`、`App/wireless.c`、`tests/control_equivalence.py` |
| 字段 pid_state_t :26 | `K2D_x` | `k2d_x` | `App/pid.c`、`App/pid.h`、`tests/control_equivalence.py` |
| 字段 pid_state_t :26 | `KP_a` | `kp_a` | `App/pid.c`、`App/pid.h`、`tests/control_equivalence.py` |
| 字段 pid_state_t :26 | `KD_a` | `kd_a` | `App/pid.c`、`App/pid.h`、`tests/control_equivalence.py` |
| 字段 pid_state_t :26 | `KG_a` | `kg_a` | `App/pid.c`、`App/pid.h`、`tests/control_equivalence.py` |
| 字段 pid_state_t :26 | `angle_err` | `angle_err` | `App/pid.c`、`App/pid.h`、`App/roundabout.c` |
| 字段 pid_state_t :26 | `angle_out` | `angle_out` | `App/pid.c`、`App/pid.h` |
| 字段 pid_state_t :26 | `out_l` | `out_l` | `App/pid.c`、`App/pid.h`、`tests/control_equivalence.py` |
| 字段 pid_state_t :26 | `out_r` | `out_r` | `App/pid.c`、`App/pid.h`、`tests/control_equivalence.py` |

### App/roundabout.c

验证：V-01/02/03/04/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 函数 :58 | `Roundabout_GetProfile` | `Roundabout_GetProfile` | `App/electromagnetic_fsm.c`、`App/roundabout.c`、`App/roundabout.h` |
| 函数 :69 | `roundabout` | `Roundabout_Update` | `App/roundabout.c`、`App/roundabout.h` |
| 函数 :123 | `L_reroundabout_judge` | `Roundabout_JudgePreentryLeft` | `App/roundabout.c`、`App/roundabout.h` |
| 函数 :149 | `R_reroundabout_judge` | `Roundabout_JudgePreentryRight` | `App/roundabout.c`、`App/roundabout.h` |
| 函数 :160 | `reroundabout_out_judge` | `Roundabout_JudgePreentryExit` | `App/roundabout.c`、`App/roundabout.h` |
| 函数 :176 | `L_roundabout_judge` | `Roundabout_JudgeEntryLeft` | `App/element.c`、`App/roundabout.c`、`App/roundabout.h` |
| 函数 :183 | `R_roundabout_judge` | `Roundabout_JudgeEntryRight` | `App/element.c`、`App/roundabout.c`、`App/roundabout.h` |
| 函数 :200 | `ahead_judge` | `Roundabout_JudgeAhead` | `App/roundabout.c`、`App/roundabout.h` |
| 函数 :211 | `entered_judge` | `Roundabout_JudgeEntered` | `App/roundabout.c`、`App/roundabout.h` |
| 函数 :220 | `entered_entered_judge` | `Roundabout_JudgeEnteredAgain` | `App/roundabout.c`、`App/roundabout.h` |
| 函数 :230 | `pre_out_judge` | `Roundabout_JudgePreExit` | `App/roundabout.c`、`App/roundabout.h` |
| 函数 :237 | `exit_judge` | `Roundabout_JudgeExit` | `App/roundabout.c`、`App/roundabout.h` |
| 函数 :248 | `outed_judge` | `Roundabout_JudgeExited` | `App/roundabout.c`、`App/roundabout.h` |
| 函数 :262 | `judge` | `Roundabout_Judge` | `App/roundabout.c`、`App/roundabout.h` |
| 函数 :269 | `Roundabout_ResetRunState` | `Roundabout_ResetRunState` | `App/app.c`、`App/roundabout.c`、`App/roundabout.h`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :34 | `ROUND_PROFILE_PREENTER` | `ROUNDABOUT_ROUND_PROFILE_PREENTER` | `App/roundabout.c`、`tests/control_equivalence.py` |
| 宏/枚举 :34 | `ROUND_PROFILE_TURN` | `ROUNDABOUT_ROUND_PROFILE_TURN` | `App/roundabout.c`、`tests/control_equivalence.py` |
| 宏/枚举 :34 | `ROUND_PROFILE_IN` | `ROUNDABOUT_ROUND_PROFILE_IN` | `App/roundabout.c`、`tests/control_equivalence.py` |
| 宏/枚举 :34 | `ROUND_PROFILE_OUT` | `ROUNDABOUT_ROUND_PROFILE_OUT` | `App/roundabout.c`、`tests/control_equivalence.py` |
| 对象声明 :18 | `dl1b_distance_mm` | `g_app_dl1b_distance_mm` | `App/app.c`、`App/app.h`、`App/control.c`、`App/dashboard.c`、`App/datalog.c`、`App/roundabout.c`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 对象 :21 | `roundabout_state` | `g_roundabout_state` | `App/control.c`、`App/datalog.c`、`App/datalog.h`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`App/roundabout.c`、`App/roundabout.h`、`tests/dual_mode_tracking.py` |
| 对象 :22 | `L_round_flag` | `g_roundabout_left_flag` | `App/roundabout.c`、`App/roundabout.h` |
| 对象 :23 | `R_round_flag` | `g_roundabout_right_flag` | `App/roundabout.c`、`App/roundabout.h` |
| 对象 :25 | `sign_round` | `g_roundabout_sign_round` | `App/roundabout.c`、`App/roundabout.h` |
| 对象 :27 | `enter_distance1` | `g_roundabout_enter_distance1` | `App/electromagnetic_fsm.c`、`App/roundabout.c`、`App/roundabout.h` |
| 对象 :28 | `out_distance1` | `g_roundabout_out_distance1` | `App/electromagnetic_fsm.c`、`App/roundabout.c`、`App/roundabout.h` |
| 对象 :30 | `enter_angle1` | `g_roundabout_enter_angle1` | `App/roundabout.c`、`App/roundabout.h` |
| 对象 :31 | `enter_angle2` | `g_roundabout_enter_angle2` | `App/roundabout.c`、`App/roundabout.h` |
| 对象 :32 | `out_angle1` | `g_roundabout_out_angle1` | `App/roundabout.c`、`App/roundabout.h` |
| 对象 :41 | `s_round_profiles` | `s_round_profiles` | `App/roundabout.c` |

### App/roundabout.h

验证：V-01/02/03/04/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 宏/枚举 :1 | `_ROUNDABOUT_H_` | `SMARTCAR_APP_ROUNDABOUT_H` | `App/roundabout.h` |
| 宏/枚举 :4 | `STATE_NORMAL` | `ROUNDABOUT_STATE_NORMAL` | `App/electromagnetic_fsm.c`、`App/roundabout.c`、`App/roundabout.h` |
| 宏/枚举 :6 | `ISLAND_LPREENTER` | `ROUNDABOUT_ISLAND_LPREENTER` | `App/electromagnetic_fsm.c`、`App/roundabout.c`、`App/roundabout.h`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :7 | `ISLAND_RPREENTER` | `ROUNDABOUT_ISLAND_RPREENTER` | `App/roundabout.h` |
| 宏/枚举 :10 | `ISLAND_TURN_LEFT` | `ROUNDABOUT_ISLAND_TURN_LEFT` | `App/electromagnetic_fsm.c`、`App/roundabout.c`、`App/roundabout.h`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :11 | `ISLAND_TURN_RIGHT` | `ROUNDABOUT_ISLAND_TURN_RIGHT` | `App/roundabout.h` |
| 宏/枚举 :13 | `ISLAND_IN` | `ROUNDABOUT_ISLAND_IN` | `App/electromagnetic_fsm.c`、`App/roundabout.c`、`App/roundabout.h`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :14 | `ISLAND_EXIT` | `ROUNDABOUT_ISLAND_EXIT` | `App/roundabout.h` |
| 宏/枚举 :15 | `ISLAND_LENTER` | `ROUNDABOUT_ISLAND_LENTER` | `App/roundabout.h` |
| 宏/枚举 :15 | `ISLAND_RENTER` | `ROUNDABOUT_ISLAND_RENTER` | `App/roundabout.h` |
| 宏/枚举 :16 | `ISLAND_OUT` | `ROUNDABOUT_ISLAND_OUT` | `App/electromagnetic_fsm.c`、`App/roundabout.c`、`App/roundabout.h`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :17 | `ISLAND_TURN_TURN_LEFT` | `ROUNDABOUT_ISLAND_TURN_TURN_LEFT` | `App/roundabout.h` |
| 对象声明 :25 | `roundabout_state` | `g_roundabout_state` | `App/control.c`、`App/datalog.c`、`App/datalog.h`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`App/roundabout.c`、`App/roundabout.h`、`tests/dual_mode_tracking.py` |
| 对象声明 :26 | `L_round_flag` | `g_roundabout_left_flag` | `App/roundabout.c`、`App/roundabout.h` |
| 对象声明 :27 | `R_round_flag` | `g_roundabout_right_flag` | `App/roundabout.c`、`App/roundabout.h` |
| 对象声明 :29 | `sign_round` | `g_roundabout_sign_round` | `App/roundabout.c`、`App/roundabout.h` |
| 对象声明 :31 | `enter_distance1` | `g_roundabout_enter_distance1` | `App/electromagnetic_fsm.c`、`App/roundabout.c`、`App/roundabout.h` |
| 对象声明 :32 | `out_distance1` | `g_roundabout_out_distance1` | `App/electromagnetic_fsm.c`、`App/roundabout.c`、`App/roundabout.h` |
| 对象声明 :34 | `enter_angle1` | `g_roundabout_enter_angle1` | `App/roundabout.c`、`App/roundabout.h` |
| 对象声明 :35 | `enter_angle2` | `g_roundabout_enter_angle2` | `App/roundabout.c`、`App/roundabout.h` |
| 对象声明 :36 | `out_angle1` | `g_roundabout_out_angle1` | `App/roundabout.c`、`App/roundabout.h` |

### App/track_sensor.c

验证：V-01/02/03/04/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 函数 :22 | `Track_GetState` | `Track_GetState` | `App/app.c`、`App/control.c`、`App/dashboard.c`、`App/datalog.c`、`App/element.c`、`App/menu.c`、`App/pid.c`、`App/roundabout.c`、`App/track_sensor.c`、`App/track_sensor.h`、`tests/control_equivalence.py`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 函数 :27 | `Track_CalculateError` | `Track_CalculateError` | `App/electromagnetic_fsm.c`、`App/track_sensor.c`、`App/track_sensor.h` |
| 函数 :33 | `Track_PublishObservation` | `Track_PublishObservation` | `App/control.c`、`App/track_sensor.c`、`App/track_sensor.h` |
| 函数 :40 | `Track_SetWeights` | `Track_SetWeights` | `App/control.c`、`App/track_sensor.c`、`App/track_sensor.h`、`tests/control_equivalence.py` |
| 函数 :63 | `get_track_error` | `Track_UpdateError` | `App/app.c`、`App/roundabout.c`、`App/track_sensor.c`、`App/track_sensor.h`、`tests/control_equivalence.py` |
| 函数 :97 | `symmetry_adc` | `Track_UpdateSymmetry` | `App/track_sensor.c`、`App/track_sensor.h`、`tests/control_equivalence.py` |
| 函数 :119 | `read_adc` | `Track_ReadAdc` | `App/app.c`、`App/control.c`、`App/track_sensor.c`、`App/track_sensor.h`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 函数 :163 | `front_adc_init` | `Track_InitAdc` | `App/track_sensor.c`、`App/track_sensor.h` |
| 对象 :15 | `s_track` | `s_track` | `App/track_sensor.c` |
| 对象 :18 | `s_adc_sum` | `s_adc_sum` | `App/track_sensor.c` |
| 对象 :19 | `s_max` | `s_max` | `App/track_sensor.c` |
| 对象 :20 | `s_min` | `s_min` | `App/track_sensor.c` |
| 对象 :49 | `adc_ch_map` | `s_adc_channel_map` | `App/track_sensor.c` |

### App/track_sensor.h

验证：V-01/02/03/04/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 类型 :6 | `track_weights_t` | `track_weights_t` | `App/control.h`、`App/electromagnetic_fsm.c`、`App/track_sensor.c`、`App/track_sensor.h`、`tests/control_equivalence.py` |
| 类型 :13 | `track_state_t` | `track_state_t` | `App/control.c`、`App/datalog.c`、`App/element.c`、`App/roundabout.c`、`App/track_sensor.c`、`App/track_sensor.h`、`tests/datalog_v2.py` |
| 宏/枚举 :2 | `_TRACK_SENSOR_H_` | `SMARTCAR_APP_TRACK_SENSOR_H` | `App/track_sensor.h` |
| 字段 track_weights_t :6 | `x` | `x` | `App/filter.c`、`App/filter.h`、`App/imu_proc.c`、`App/imu_proc.h`、`App/param.c`、`App/track_sensor.c`、`App/track_sensor.h`、`App/wireless.c`、`BSP/bsp_imu660rb.c`、`BSP/bsp_imu660rb.h`、`BSP/bsp_lcd.c`、`BSP/bsp_lcd.h`、`BSP/bsp_sampler.c`、`BSP/bsp_sampler.h`、`tests/control_equivalence.py`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 字段 track_weights_t :6 | `xx` | `xx` | `App/track_sensor.c`、`App/track_sensor.h` |
| 字段 track_weights_t :6 | `y` | `y` | `App/filter.c`、`App/filter.h`、`App/imu_proc.c`、`App/imu_proc.h`、`App/menu.c`、`App/track_sensor.c`、`App/track_sensor.h`、`BSP/bsp_imu660rb.c`、`BSP/bsp_imu660rb.h`、`BSP/bsp_lcd.c`、`BSP/bsp_lcd.h`、`BSP/bsp_sampler.c`、`BSP/bsp_sampler.h`、`tests/control_equivalence.py`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 字段 track_weights_t :6 | `abs` | `abs` | `App/electromagnetic_fsm.c`、`App/pid.c`、`App/track_sensor.c`、`App/track_sensor.h` |
| 字段 track_state_t :13 | `raw` | `raw` | `App/datalog.c`、`App/gray_sensor.c`、`App/gray_sensor.h`、`App/track_sensor.c`、`App/track_sensor.h`、`BSP/bsp_gray.c`、`BSP/bsp_gray.h`、`BSP/bsp_key.c`、`BSP/bsp_sampler.c`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 字段 track_state_t :13 | `filtered` | `filtered` | `App/app.c`、`App/control.c`、`App/dashboard.c`、`App/datalog.c`、`App/element.c`、`App/menu.c`、`App/roundabout.c`、`App/track_sensor.c`、`App/track_sensor.h`、`tests/dual_mode_tracking.py` |
| 字段 track_state_t :13 | `error` | `error` | `App/control.c`、`App/dashboard.c`、`App/datalog.c`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`App/gray_sensor.c`、`App/gray_sensor.h`、`App/menu.c`、`App/optical_fsm.c`、`App/optical_fsm.h`、`App/pid.c`、`App/pid.h`、`App/track_sensor.c`、`App/track_sensor.h`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 字段 track_state_t :13 | `symmetry_x` | `symmetry_x` | `App/control.c`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`App/menu.c`、`App/track_sensor.c`、`App/track_sensor.h`、`tests/control_equivalence.py` |
| 字段 track_state_t :13 | `symmetry_y` | `symmetry_y` | `App/control.c`、`App/electromagnetic_fsm.c`、`App/electromagnetic_fsm.h`、`App/element.c`、`App/menu.c`、`App/track_sensor.c`、`App/track_sensor.h`、`tests/control_equivalence.py` |
| 字段 track_state_t :13 | `weights` | `weights` | `App/control.c`、`App/control.h`、`App/datalog.c`、`App/datalog.h`、`App/electromagnetic_fsm.c`、`App/track_sensor.c`、`App/track_sensor.h`、`tests/control_equivalence.py` |

### App/wireless.c

验证：V-01/02/03/05/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 函数 :23 | `wireless_adjust` | `Wireless_Adjust` | `App/app.c`、`App/wireless.c`、`App/wireless.h`、`tests/dual_mode_tracking.py` |

### App/wireless.h

验证：V-01/02/03/05/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 宏/枚举 :2 | `__WIRELESS_H` | `SMARTCAR_APP_WIRELESS_H` | `App/wireless.h` |

### BSP/bsp.h

验证：V-01/02/03/06/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 宏/枚举 :2 | `__BSP_H` | `SMARTCAR_BSP_BSP_H` | `BSP/bsp.h` |

### BSP/bsp_adc.c

验证：V-01/02/03/06/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 函数 :34 | `BSP_ADC_Init` | `BSP_ADC_Init` | `App/app.c`、`App/track_sensor.c`、`BSP/bsp_adc.c`、`BSP/bsp_adc.h`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 函数 :46 | `BSP_ADC_Read` | `BSP_ADC_Read` | `App/app.c`、`App/menu.c`、`App/track_sensor.c`、`BSP/bsp_adc.c`、`BSP/bsp_adc.h`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 函数 :56 | `BSP_ADC_ReadGrayFresh` | `BSP_ADC_ReadGrayFresh` | `BSP/bsp_adc.c`、`BSP/bsp_adc.h`、`BSP/bsp_gray.c` |
| 对象 :22 | `s_adc_buf` | `s_adc_buf` | `BSP/bsp_adc.c`、`tests/dual_mode_tracking.py` |

### BSP/bsp_adc.h

验证：V-01/02/03/06/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 类型 :12 | `bsp_adc_channel_t` | `bsp_adc_channel_t` | `App/menu.c`、`App/track_sensor.c`、`BSP/bsp_adc.c`、`BSP/bsp_adc.h`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 类型 :30 | `bsp_gray_status_t` | `bsp_gray_status_t` | `BSP/bsp_adc.c`、`BSP/bsp_adc.h`、`BSP/bsp_gray.c`、`BSP/bsp_gray.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :2 | `__BSP_ADC_H` | `SMARTCAR_BSP_BSP_ADC_H` | `BSP/bsp_adc.h` |
| 宏/枚举 :12 | `BSP_ADC_IND_H_L` | `BSP_ADC_IND_H_L` | `App/track_sensor.c`、`BSP/bsp_adc.h`、`tests/control_equivalence.py` |
| 宏/枚举 :12 | `BSP_ADC_IND_V_L` | `BSP_ADC_IND_V_L` | `App/track_sensor.c`、`BSP/bsp_adc.h`、`tests/control_equivalence.py` |
| 宏/枚举 :12 | `BSP_ADC_IND_V_R` | `BSP_ADC_IND_V_R` | `App/track_sensor.c`、`BSP/bsp_adc.h`、`tests/control_equivalence.py` |
| 宏/枚举 :12 | `BSP_ADC_IND_H_R` | `BSP_ADC_IND_H_R` | `App/track_sensor.c`、`BSP/bsp_adc.h`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :12 | `BSP_ADC_GRAY` | `BSP_ADC_GRAY` | `BSP/bsp_adc.c`、`BSP/bsp_adc.h`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :12 | `BSP_ADC_KEY1` | `BSP_ADC_KEY1` | `App/menu.c`、`BSP/bsp_adc.h`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :12 | `BSP_ADC_KEY2` | `BSP_ADC_KEY2` | `App/menu.c`、`BSP/bsp_adc.h`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :12 | `BSP_ADC_VBAT` | `BSP_ADC_VBAT` | `App/app.c`、`BSP/bsp_adc.h` |
| 宏/枚举 :12 | `BSP_ADC_SPARE1` | `BSP_ADC_SPARE1` | `BSP/bsp_adc.h` |
| 宏/枚举 :12 | `BSP_ADC_SPARE2` | `BSP_ADC_SPARE2` | `BSP/bsp_adc.h` |
| 宏/枚举 :12 | `BSP_ADC_CH_NUM` | `BSP_ADC_CH_NUM` | `BSP/bsp_adc.c`、`BSP/bsp_adc.h` |
| 宏/枚举 :30 | `BSP_GRAY_OK` | `BSP_GRAY_OK` | `App/gray_sensor.c`、`BSP/bsp_adc.c`、`BSP/bsp_adc.h`、`BSP/bsp_gray.c`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :30 | `BSP_GRAY_ARG` | `BSP_GRAY_ARG` | `BSP/bsp_adc.c`、`BSP/bsp_adc.h`、`BSP/bsp_gray.c`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :30 | `BSP_GRAY_TIMEOUT` | `BSP_GRAY_TIMEOUT` | `BSP/bsp_adc.c`、`BSP/bsp_adc.h`、`BSP/bsp_gray.c`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :30 | `BSP_GRAY_DMA_ERROR` | `BSP_GRAY_DMA_ERROR` | `BSP/bsp_adc.c`、`BSP/bsp_adc.h`、`tests/dual_mode_tracking.py` |

### BSP/bsp_dl1b.c

验证：V-01/02/03/06/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 私有函数 :56 | `dl1b_read` | `BSP_DL1B_ReadRegister` | `BSP/bsp_dl1b.c` |
| 私有函数 :63 | `dl1b_write` | `BSP_DL1B_WriteRegister` | `BSP/bsp_dl1b.c` |
| 函数 :75 | `BSP_DL1B_Init` | `BSP_DL1B_Init` | `App/app.c`、`BSP/bsp_dl1b.c`、`BSP/bsp_dl1b.h`、`tests/dual_mode_tracking.py` |
| 函数 :130 | `BSP_DL1B_Update` | `BSP_DL1B_Update` | `App/app.c`、`BSP/bsp_dl1b.c`、`BSP/bsp_dl1b.h`、`tests/dual_mode_tracking.py` |
| 函数 :179 | `BSP_DL1B_GetDistanceMm` | `BSP_DL1B_GetDistanceMm` | `App/app.c`、`BSP/bsp_dl1b.c`、`BSP/bsp_dl1b.h`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :2 | `DL1B_I2C_ADDR` | `BSP_DL1B_I2C_ADDR` | `BSP/bsp_dl1b.c` |
| 宏/枚举 :3 | `DL1B_I2C_TIMEOUT_MS` | `BSP_DL1B_I2C_TIMEOUT_MS` | `BSP/bsp_dl1b.c` |
| 宏/枚举 :4 | `DL1B_TIMEOUT_COUNT` | `BSP_DL1B_TIMEOUT_COUNT` | `BSP/bsp_dl1b.c` |
| 宏/枚举 :5 | `DL1B_I2C_SLAVE__DEVICE_ADDRESS` | `BSP_DL1B_I2C_SLAVE_DEVICE_ADDRESS` | `BSP/bsp_dl1b.c` |
| 宏/枚举 :6 | `DL1B_GPIO__TIO_HV_STATUS` | `BSP_DL1B_GPIO_TIO_HV_STATUS` | `BSP/bsp_dl1b.c` |
| 宏/枚举 :7 | `DL1B_SYSTEM__INTERRUPT_CLEAR` | `BSP_DL1B_SYSTEM_INTERRUPT_CLEAR` | `BSP/bsp_dl1b.c` |
| 宏/枚举 :8 | `DL1B_RESULT__RANGE_STATUS` | `BSP_DL1B_RESULT_RANGE_STATUS` | `BSP/bsp_dl1b.c` |
| 宏/枚举 :11 | `DL1B_RESULT__FINAL_CROSSTALK_CORRECTED_RANGE_MM_SD0` | `BSP_DL1B_RESULT_FINAL_CROSSTALK_CORRECTED_RANGE_MM_SD0` | `BSP/bsp_dl1b.c` |
| 宏/枚举 :12 | `DL1B_FIRMWARE__SYSTEM_STATUS` | `BSP_DL1B_FIRMWARE_SYSTEM_STATUS` | `BSP/bsp_dl1b.c` |
| 对象 :28 | `s_dl1b_init_flag` | `s_dl1b_init_flag` | `BSP/bsp_dl1b.c` |
| 对象 :29 | `s_dl1b_distance_mm` | `s_dl1b_distance_mm` | `BSP/bsp_dl1b.c` |
| 对象 :32 | `s_dl1b_default_configuration` | `s_dl1b_default_configuration` | `BSP/bsp_dl1b.c` |

### BSP/bsp_dl1b.h

验证：V-01/02/03/06/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 宏/枚举 :2 | `__BSP_DL1B_H` | `SMARTCAR_BSP_BSP_DL1B_H` | `BSP/bsp_dl1b.h` |
| 宏/枚举 :4 | `BSP_DL1B_INVALID` | `BSP_DL1B_INVALID` | `App/app.c`、`BSP/bsp_dl1b.c`、`BSP/bsp_dl1b.h` |

### BSP/bsp_encoder.c

验证：V-01/02/03/06/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 函数 :22 | `BSP_Encoder_Init` | `BSP_Encoder_Init` | `App/motor.c`、`BSP/bsp_encoder.c`、`BSP/bsp_encoder.h`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 函数 :33 | `BSP_Encoder_GetLeft` | `BSP_Encoder_GetLeft` | `BSP/bsp_encoder.c`、`BSP/bsp_encoder.h`、`BSP/bsp_sampler.c` |
| 函数 :41 | `BSP_Encoder_GetRight` | `BSP_Encoder_GetRight` | `BSP/bsp_encoder.c`、`BSP/bsp_encoder.h`、`BSP/bsp_sampler.c` |
| 私有函数 :51 | `encoder_read_clear` | `BSP_Encoder_ReadClear` | `BSP/bsp_encoder.c` |

### BSP/bsp_encoder.h

验证：V-01/02/03/06/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 宏/枚举 :2 | `__BSP_ENCODER_H` | `SMARTCAR_BSP_BSP_ENCODER_H` | `BSP/bsp_encoder.h` |
| 宏/枚举 :4 | `BSP_ENCODER_L_INVERT` | `BSP_ENCODER_L_INVERT` | `BSP/bsp_encoder.c`、`BSP/bsp_encoder.h` |
| 宏/枚举 :6 | `BSP_ENCODER_R_INVERT` | `BSP_ENCODER_R_INVERT` | `BSP/bsp_encoder.c`、`BSP/bsp_encoder.h` |

### BSP/bsp_flash.c

验证：V-01/02/03/06/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 函数 :30 | `BSP_Flash_Init` | `BSP_Flash_Init` | `App/app.c`、`BSP/bsp_flash.c`、`BSP/bsp_flash.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 函数 :42 | `BSP_Flash_Read` | `BSP_Flash_Read` | `App/param.c`、`BSP/bsp_flash.c`、`BSP/bsp_flash.h`、`tests/dual_mode_tracking.py` |
| 函数 :57 | `BSP_Flash_Write` | `BSP_Flash_Write` | `App/param.c`、`BSP/bsp_flash.c`、`BSP/bsp_flash.h`、`tests/dual_mode_tracking.py` |
| 函数 :101 | `BSP_Flash_LogErase` | `BSP_Flash_LogErase` | `App/datalog.c`、`BSP/bsp_flash.c`、`BSP/bsp_flash.h`、`tests/dual_mode_tracking.py` |
| 函数 :122 | `BSP_Flash_LogWrite` | `BSP_Flash_LogWrite` | `App/datalog.c`、`BSP/bsp_flash.c`、`BSP/bsp_flash.h`、`tests/dual_mode_tracking.py` |
| 函数 :166 | `BSP_Flash_LogRead` | `BSP_Flash_LogRead` | `App/datalog.c`、`BSP/bsp_flash.c`、`BSP/bsp_flash.h`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :2 | `PARAM_ADDR` | `BSP_FLASH_PARAM_ADDR` | `BSP/bsp_flash.c` |
| 宏/枚举 :4 | `FLASH_WORD_SIZE` | `BSP_FLASH_WORD_SIZE` | `BSP/bsp_flash.c` |
| 对象 :21 | `s_ready` | `s_ready` | `BSP/bsp_flash.c` |
| 对象 :22 | `s_log_frontier` | `s_log_frontier` | `BSP/bsp_flash.c` |
| 对象 :23 | `s_param_shadow` | `s_param_shadow` | `BSP/bsp_flash.c` |

### BSP/bsp_flash.h

验证：V-01/02/03/06/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 宏/枚举 :2 | `__BSP_FLASH_H` | `SMARTCAR_BSP_BSP_FLASH_H` | `BSP/bsp_flash.h` |
| 宏/枚举 :5 | `BSP_FLASH_PARAM_SECTOR_SIZE` | `BSP_FLASH_PARAM_SECTOR_SIZE` | `BSP/bsp_flash.h` |
| 宏/枚举 :14 | `BSP_FLASH_LOG_SIZE` | `BSP_FLASH_LOG_SIZE` | `App/datalog.c`、`BSP/bsp_flash.c`、`BSP/bsp_flash.h`、`tests/dual_mode_tracking.py` |

### BSP/bsp_gray.c

验证：V-01/02/03/06/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 私有函数 :13 | `select_channel` | `BSP_Gray_SelectChannel` | `BSP/bsp_gray.c` |
| 函数 :22 | `BSP_Gray_ResetSequence` | `BSP_Gray_ResetSequence` | `App/gray_sensor.c`、`BSP/bsp_gray.c`、`BSP/bsp_gray.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 函数 :23 | `BSP_Gray_Init` | `BSP_Gray_Init` | `App/gray_sensor.c`、`BSP/bsp_gray.c`、`BSP/bsp_gray.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 函数 :24 | `BSP_Gray_ReadFrame` | `BSP_Gray_ReadFrame` | `App/gray_sensor.c`、`BSP/bsp_gray.c`、`BSP/bsp_gray.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 对象 :12 | `s_sequence` | `s_sequence` | `BSP/bsp_gray.c` |

### BSP/bsp_gray.h

验证：V-01/02/03/06/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 类型 :4 | `bsp_gray_frame_t` | `bsp_gray_frame_t` | `App/gray_sensor.c`、`BSP/bsp_gray.c`、`BSP/bsp_gray.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :2 | `BSP_GRAY_H` | `SMARTCAR_BSP_BSP_GRAY_H` | `BSP/bsp_gray.h` |
| 字段 bsp_gray_frame_t :4 | `raw` | `raw` | `App/datalog.c`、`App/gray_sensor.c`、`App/gray_sensor.h`、`App/track_sensor.c`、`App/track_sensor.h`、`BSP/bsp_gray.c`、`BSP/bsp_gray.h`、`BSP/bsp_key.c`、`BSP/bsp_sampler.c`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 字段 bsp_gray_frame_t :4 | `sequence` | `sequence` | `App/datalog.c`、`App/gray_sensor.c`、`App/gray_sensor.h`、`BSP/bsp_gray.c`、`BSP/bsp_gray.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 字段 bsp_gray_frame_t :4 | `scan_cycles` | `scan_cycles` | `App/gray_sensor.c`、`App/gray_sensor.h`、`BSP/bsp_gray.c`、`BSP/bsp_gray.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |

### BSP/bsp_imu660rb.c

验证：V-01/02/03/06/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 私有函数 :42 | `imu_cs` | `BSP_IMU660RB_SetChipSelect` | `BSP/bsp_imu660rb.c` |
| 私有函数 :49 | `imu_write_register` | `BSP_IMU660RB_WriteRegister` | `BSP/bsp_imu660rb.c` |
| 私有函数 :58 | `imu_read_register` | `BSP_IMU660RB_ReadRegister` | `BSP/bsp_imu660rb.c` |
| 私有函数 :70 | `imu_read_registers` | `BSP_IMU660RB_ReadRegisters` | `BSP/bsp_imu660rb.c` |
| 私有函数 :80 | `imu_self_check` | `BSP_IMU660RB_CheckIdentity` | `BSP/bsp_imu660rb.c` |
| 函数 :102 | `BSP_IMU660RB_Init` | `BSP_IMU660RB_Init` | `App/app.c`、`App/imu_proc.c`、`BSP/bsp_imu660rb.c`、`BSP/bsp_imu660rb.h`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 函数 :126 | `BSP_IMU660RB_GetAcc` | `BSP_IMU660RB_GetAcc` | `App/imu_proc.c`、`BSP/bsp_imu660rb.c`、`BSP/bsp_imu660rb.h`、`BSP/bsp_sampler.c`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 函数 :138 | `BSP_IMU660RB_GetGyro` | `BSP_IMU660RB_GetGyro` | `App/imu_proc.c`、`BSP/bsp_imu660rb.c`、`BSP/bsp_imu660rb.h`、`BSP/bsp_sampler.c`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :2 | `IMU_SPI_TIMEOUT_MS` | `BSP_IMU660RB_SPI_TIMEOUT_MS` | `BSP/bsp_imu660rb.c` |
| 宏/枚举 :2 | `IMU660RB_SPI_R` | `BSP_IMU660RB_SPI_R` | `BSP/bsp_imu660rb.c` |
| 宏/枚举 :4 | `IMU660RB_TIMEOUT_COUNT` | `BSP_IMU660RB_TIMEOUT_COUNT` | `BSP/bsp_imu660rb.c` |
| 宏/枚举 :5 | `IMU660RB_CHIP_ID` | `BSP_IMU660RB_CHIP_ID` | `BSP/bsp_imu660rb.c` |
| 宏/枚举 :6 | `IMU660RB_CHIP_ID_VALUE` | `BSP_IMU660RB_CHIP_ID_VALUE` | `BSP/bsp_imu660rb.c` |
| 宏/枚举 :6 | `IMU660RB_INT1_CTRL` | `BSP_IMU660RB_INT1_CTRL` | `BSP/bsp_imu660rb.c` |
| 宏/枚举 :7 | `IMU660RB_CTRL1_XL` | `BSP_IMU660RB_CTRL1_XL` | `BSP/bsp_imu660rb.c` |
| 宏/枚举 :7 | `IMU660RB_CTRL2_G` | `BSP_IMU660RB_CTRL2_G` | `BSP/bsp_imu660rb.c` |
| 宏/枚举 :10 | `IMU660RB_CTRL3_C` | `BSP_IMU660RB_CTRL3_C` | `BSP/bsp_imu660rb.c` |
| 宏/枚举 :10 | `IMU660RB_CTRL4_C` | `BSP_IMU660RB_CTRL4_C` | `BSP/bsp_imu660rb.c` |
| 宏/枚举 :11 | `IMU660RB_CTRL5_C` | `BSP_IMU660RB_CTRL5_C` | `BSP/bsp_imu660rb.c` |
| 宏/枚举 :14 | `IMU660RB_CTRL6_C` | `BSP_IMU660RB_CTRL6_C` | `BSP/bsp_imu660rb.c` |
| 宏/枚举 :14 | `IMU660RB_CTRL7_G` | `BSP_IMU660RB_CTRL7_G` | `BSP/bsp_imu660rb.c` |
| 宏/枚举 :15 | `IMU660RB_CTRL9_XL` | `BSP_IMU660RB_CTRL9_XL` | `BSP/bsp_imu660rb.c` |
| 宏/枚举 :15 | `IMU660RB_ACC_ADDRESS` | `BSP_IMU660RB_ACC_ADDRESS` | `BSP/bsp_imu660rb.c` |
| 宏/枚举 :17 | `IMU660RB_GYRO_ADDRESS` | `BSP_IMU660RB_GYRO_ADDRESS` | `BSP/bsp_imu660rb.c` |
| 宏/枚举 :18 | `IMU660RB_ACC_SAMPLE` | `BSP_IMU660RB_ACC_SAMPLE` | `BSP/bsp_imu660rb.c` |
| 宏/枚举 :20 | `IMU660RB_GYR_SAMPLE` | `BSP_IMU660RB_GYR_SAMPLE` | `BSP/bsp_imu660rb.c` |

### BSP/bsp_imu660rb.h

验证：V-01/02/03/06/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 宏/枚举 :2 | `__BSP_IMU660RB_H` | `SMARTCAR_BSP_BSP_IMU660RB_H` | `BSP/bsp_imu660rb.h` |

### BSP/bsp_key.c

验证：V-01/02/03/06/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 函数 :5 | `BSP_Key_RequireRelease` | `BSP_Key_RequireRelease` | `App/app.c`、`BSP/bsp_key.c`、`BSP/bsp_key.h`、`tests/dual_mode_tracking.py` |
| 函数 :11 | `BSP_Key_Init` | `BSP_Key_Init` | `App/app.c`、`BSP/bsp_key.c`、`BSP/bsp_key.h`、`tests/dual_mode_tracking.py` |
| 函数 :12 | `BSP_Key_PollEvent` | `BSP_Key_PollEvent` | `App/app.c`、`BSP/bsp_key.c`、`BSP/bsp_key.h`、`tests/dual_mode_tracking.py` |
| 函数 :32 | `BSP_Key_StartPressed` | `BSP_Key_StartPressed` | `BSP/bsp_key.c`、`BSP/bsp_key.h` |
| 对象 :3 | `s_raw` | `s_raw` | `BSP/bsp_key.c` |
| 对象 :3 | `s_stable` | `s_stable` | `BSP/bsp_key.c` |
| 对象 :3 | `s_armed` | `s_armed` | `BSP/bsp_key.c` |
| 对象 :3 | `s_pressed` | `s_pressed` | `BSP/bsp_key.c` |
| 对象 :3 | `s_long` | `s_long` | `BSP/bsp_key.c` |
| 对象 :4 | `s_changed` | `s_changed` | `BSP/bsp_key.c` |
| 对象 :4 | `s_pressed_at` | `s_pressed_at` | `BSP/bsp_key.c` |

### BSP/bsp_key.h

验证：V-01/02/03/06/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 类型 :11 | `bsp_key_event_t` | `bsp_key_event_t` | `App/app.c`、`BSP/bsp_key.c`、`BSP/bsp_key.h` |
| 宏/枚举 :2 | `__BSP_KEY_H` | `SMARTCAR_BSP_BSP_KEY_H` | `BSP/bsp_key.h` |
| 宏/枚举 :11 | `BSP_KEY_NONE` | `BSP_KEY_NONE` | `BSP/bsp_key.c`、`BSP/bsp_key.h`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :11 | `BSP_KEY_START` | `BSP_KEY_START` | `App/app.c`、`BSP/bsp_key.c`、`BSP/bsp_key.h`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :11 | `BSP_KEY_TOGGLE` | `BSP_KEY_TOGGLE` | `App/app.c`、`BSP/bsp_key.c`、`BSP/bsp_key.h`、`tests/dual_mode_tracking.py` |

### BSP/bsp_lcd.c

验证：V-01/02/03/06/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 函数 :17 | `BSP_LCD_Init` | `BSP_LCD_Init` | `App/app.c`、`BSP/bsp_lcd.c`、`BSP/bsp_lcd.h`、`tests/dual_mode_tracking.py` |
| 函数 :24 | `BSP_LCD_Clear` | `BSP_LCD_Clear` | `App/app.c`、`App/dashboard.c`、`App/menu.c`、`BSP/bsp_lcd.c`、`BSP/bsp_lcd.h`、`tests/dual_mode_tracking.py` |
| 函数 :31 | `BSP_LCD_SetBacklight` | `BSP_LCD_SetBacklight` | `App/app.c`、`BSP/bsp_lcd.c`、`BSP/bsp_lcd.h`、`tests/dual_mode_tracking.py` |
| 函数 :44 | `BSP_LCD_ShowChar` | `BSP_LCD_ShowChar` | `BSP/bsp_lcd.c`、`BSP/bsp_lcd.h` |
| 函数 :76 | `BSP_LCD_ShowString` | `BSP_LCD_ShowString` | `App/dashboard.c`、`App/menu.c`、`BSP/bsp_lcd.c`、`BSP/bsp_lcd.h`、`tests/dual_mode_tracking.py` |
| 函数 :88 | `BSP_LCD_ShowInt` | `BSP_LCD_ShowInt` | `App/dashboard.c`、`App/menu.c`、`BSP/bsp_lcd.c`、`BSP/bsp_lcd.h`、`tests/dual_mode_tracking.py` |
| 函数 :111 | `BSP_LCD_ShowFloat` | `BSP_LCD_ShowFloat` | `App/menu.c`、`BSP/bsp_lcd.c`、`BSP/bsp_lcd.h`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :5 | `FONT_W` | `BSP_LCD_FONT_WIDTH` | `BSP/bsp_lcd.c` |
| 宏/枚举 :6 | `FONT_H` | `BSP_LCD_FONT_HEIGHT` | `BSP/bsp_lcd.c` |
| 对象 :11 | `BSP_LCD_W` | `g_bsp_lcd_width` | `BSP/bsp_lcd.c`、`BSP/bsp_lcd.h` |
| 对象 :12 | `BSP_LCD_H` | `g_bsp_lcd_height` | `BSP/bsp_lcd.c`、`BSP/bsp_lcd.h` |

### BSP/bsp_lcd.h

验证：V-01/02/03/06/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 宏/枚举 :2 | `__BSP_LCD_H` | `SMARTCAR_BSP_BSP_LCD_H` | `BSP/bsp_lcd.h` |
| 宏/枚举 :4 | `LCD_WHITE` | `BSP_LCD_WHITE` | `App/app.c`、`App/dashboard.c`、`App/menu.c`、`BSP/bsp_lcd.h` |
| 宏/枚举 :6 | `LCD_BLACK` | `BSP_LCD_BLACK` | `App/dashboard.c`、`App/menu.c`、`BSP/bsp_lcd.c`、`BSP/bsp_lcd.h` |
| 宏/枚举 :6 | `LCD_BLUE` | `BSP_LCD_BLUE` | `App/dashboard.c`、`BSP/bsp_lcd.h` |
| 宏/枚举 :7 | `LCD_RED` | `BSP_LCD_RED` | `App/dashboard.c`、`App/menu.c`、`BSP/bsp_lcd.h` |
| 宏/枚举 :7 | `LCD_GREEN` | `BSP_LCD_GREEN` | `BSP/bsp_lcd.h` |
| 宏/枚举 :8 | `LCD_YELLOW` | `BSP_LCD_YELLOW` | `BSP/bsp_lcd.h` |
| 宏/枚举 :11 | `LCD_CYAN` | `BSP_LCD_CYAN` | `BSP/bsp_lcd.h` |
| 宏/枚举 :12 | `LCD_MAGENTA` | `BSP_LCD_MAGENTA` | `App/dashboard.c`、`BSP/bsp_lcd.h` |
| 宏/枚举 :13 | `LCD_GRAY` | `BSP_LCD_GRAY` | `BSP/bsp_lcd.h` |
| 对象声明 :21 | `BSP_LCD_W` | `g_bsp_lcd_width` | `BSP/bsp_lcd.c`、`BSP/bsp_lcd.h` |
| 对象声明 :22 | `BSP_LCD_H` | `g_bsp_lcd_height` | `BSP/bsp_lcd.c`、`BSP/bsp_lcd.h` |

### BSP/bsp_pwm.c

验证：V-01/02/03/06/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 函数 :34 | `BSP_PWM_Init` | `BSP_PWM_Init` | `App/app.c`、`App/motor.c`、`BSP/bsp_pwm.c`、`BSP/bsp_pwm.h`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 函数 :45 | `BSP_PWM_SetDuty` | `BSP_PWM_SetDuty` | `App/app.c`、`App/motor.c`、`BSP/bsp_pwm.c`、`BSP/bsp_pwm.h`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 对象 :14 | `s_pwm_tim` | `s_pwm_tim` | `BSP/bsp_pwm.c` |
| 对象 :21 | `s_pwm_chan` | `s_pwm_chan` | `BSP/bsp_pwm.c` |

### BSP/bsp_pwm.h

验证：V-01/02/03/06/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 类型 :13 | `bsp_pwm_channel_t` | `bsp_pwm_channel_t` | `BSP/bsp_pwm.c`、`BSP/bsp_pwm.h`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :2 | `__BSP_PWM_H` | `SMARTCAR_BSP_BSP_PWM_H` | `BSP/bsp_pwm.h` |
| 宏/枚举 :4 | `BSP_PWM_DUTY_MAX` | `BSP_PWM_DUTY_MAX` | `BSP/bsp_pwm.c`、`BSP/bsp_pwm.h` |
| 宏/枚举 :13 | `BSP_PWM_MOTOR_L` | `BSP_PWM_MOTOR_L` | `App/motor.c`、`BSP/bsp_pwm.c`、`BSP/bsp_pwm.h`、`tests/control_equivalence.py` |
| 宏/枚举 :13 | `BSP_PWM_MOTOR_R` | `BSP_PWM_MOTOR_R` | `App/motor.c`、`BSP/bsp_pwm.c`、`BSP/bsp_pwm.h`、`tests/control_equivalence.py` |
| 宏/枚举 :13 | `BSP_PWM_FAN` | `BSP_PWM_FAN` | `App/app.c`、`BSP/bsp_pwm.c`、`BSP/bsp_pwm.h`、`tests/control_equivalence.py` |
| 宏/枚举 :13 | `BSP_PWM_CH_NUM` | `BSP_PWM_CH_NUM` | `BSP/bsp_pwm.c`、`BSP/bsp_pwm.h` |

### BSP/bsp_sampler.c

验证：V-01/02/03/06/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 函数 :33 | `BSP_Sampler_Init` | `BSP_Sampler_Init` | `App/app.c`、`BSP/bsp_sampler.c`、`BSP/bsp_sampler.h`、`tests/dual_mode_tracking.py` |
| 函数 :49 | `BSP_Sampler_Start` | `BSP_Sampler_Start` | `App/app.c`、`BSP/bsp_sampler.c`、`BSP/bsp_sampler.h`、`tests/dual_mode_tracking.py` |
| 函数 :59 | `BSP_Sampler_Stop` | `BSP_Sampler_Stop` | `App/app.c`、`BSP/bsp_sampler.c`、`BSP/bsp_sampler.h`、`tests/dual_mode_tracking.py` |
| 函数 :64 | `BSP_Sampler_GetTick` | `BSP_Sampler_GetTick` | `App/datalog.c`、`BSP/bsp_sampler.c`、`BSP/bsp_sampler.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 函数 :69 | `BSP_Sampler_GetEncL_1ms` | `BSP_Sampler_GetEncL_1ms` | `App/datalog.c`、`BSP/bsp_sampler.c`、`BSP/bsp_sampler.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 函数 :74 | `BSP_Sampler_GetEncR_1ms` | `BSP_Sampler_GetEncR_1ms` | `App/datalog.c`、`BSP/bsp_sampler.c`、`BSP/bsp_sampler.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 函数 :79 | `BSP_Sampler_GetGyroRaw` | `BSP_Sampler_GetGyroRaw` | `App/datalog.c`、`App/imu_proc.c`、`BSP/bsp_sampler.c`、`BSP/bsp_sampler.h`、`tests/control_equivalence.py`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 函数 :86 | `BSP_Sampler_GetAccRaw` | `BSP_Sampler_GetAccRaw` | `BSP/bsp_sampler.c`、`BSP/bsp_sampler.h` |
| 函数 :93 | `BSP_Sampler_ConsumeEncL` | `BSP_Sampler_ConsumeEncL` | `App/motor.c`、`BSP/bsp_sampler.c`、`BSP/bsp_sampler.h`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 函数 :100 | `BSP_Sampler_ConsumeEncR` | `BSP_Sampler_ConsumeEncR` | `App/motor.c`、`BSP/bsp_sampler.c`、`BSP/bsp_sampler.h`、`tests/control_equivalence.py`、`tests/dual_mode_tracking.py` |
| 函数 :112 | `TIM15_IRQHandler` | `TIM15_IRQHandler` | 固定入口 E-01；`BSP/bsp_sampler.c` |
| 对象 :20 | `s_tick` | `s_tick` | `BSP/bsp_sampler.c` |
| 对象 :21 | `s_enc_l_1ms` | `s_enc_l_1ms` | `BSP/bsp_sampler.c` |
| 对象 :22 | `s_enc_r_1ms` | `s_enc_r_1ms` | `BSP/bsp_sampler.c` |
| 对象 :23 | `s_enc_l_sum` | `s_enc_l_sum` | `BSP/bsp_sampler.c` |
| 对象 :24 | `s_enc_r_sum` | `s_enc_r_sum` | `BSP/bsp_sampler.c` |
| 对象 :25 | `s_gyro` | `s_gyro` | `BSP/bsp_sampler.c` |
| 对象 :26 | `s_acc` | `s_acc` | `BSP/bsp_sampler.c` |

### BSP/bsp_sampler.h

验证：V-01/02/03/06/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 宏/枚举 :2 | `__BSP_SAMPLER_H` | `SMARTCAR_BSP_BSP_SAMPLER_H` | `BSP/bsp_sampler.h` |

### BSP/bsp_time.c

验证：V-01/02/03/06/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 函数 :3 | `BSP_Time_Init` | `BSP_Time_Init` | `BSP/bsp_gray.c`、`BSP/bsp_time.c`、`BSP/bsp_time.h`、`tests/dual_mode_tracking.py` |
| 函数 :9 | `BSP_Time_GetCycles` | `BSP_Time_GetCycles` | `App/app.c`、`BSP/bsp_adc.c`、`BSP/bsp_gray.c`、`BSP/bsp_time.c`、`BSP/bsp_time.h`、`tests/dual_mode_tracking.py` |
| 函数 :10 | `BSP_EnterCritical` | `BSP_Time_EnterCritical` | `App/app.c`、`App/control.c`、`App/datalog.c`、`BSP/bsp_time.c`、`BSP/bsp_time.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 函数 :11 | `BSP_ExitCritical` | `BSP_Time_ExitCritical` | `App/app.c`、`App/control.c`、`App/datalog.c`、`BSP/bsp_time.c`、`BSP/bsp_time.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 函数 :12 | `BSP_MemoryBarrier` | `BSP_Time_MemoryBarrier` | `App/datalog.c`、`BSP/bsp_time.c`、`BSP/bsp_time.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |

### BSP/bsp_time.h

验证：V-01/02/03/06/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 宏/枚举 :2 | `BSP_TIME_H` | `SMARTCAR_BSP_BSP_TIME_H` | `BSP/bsp_time.h` |
| 宏/枚举 :4 | `BSP_GRAY_BUDGET_CYCLES` | `BSP_GRAY_BUDGET_CYCLES` | `BSP/bsp_adc.c`、`BSP/bsp_gray.c`、`BSP/bsp_time.h` |
| 宏/枚举 :5 | `BSP_CONTROL_BUDGET_CYCLES` | `BSP_CONTROL_BUDGET_CYCLES` | `App/app.c`、`BSP/bsp_time.h` |

### BSP/bsp_uart.c

验证：V-01/02/03/06/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 函数 :34 | `BSP_UART_Init` | `BSP_UART_Init` | `App/app.c`、`BSP/bsp_uart.c`、`BSP/bsp_uart.h`、`tests/dual_mode_tracking.py` |
| 函数 :47 | `BSP_UART_Write` | `BSP_UART_Write` | `App/datalog.c`、`BSP/bsp_uart.c`、`BSP/bsp_uart.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 函数 :65 | `BSP_UART_WriteString` | `BSP_UART_WriteString` | `App/app.c`、`App/control.c`、`App/datalog.c`、`App/gray_sensor.c`、`App/param.c`、`App/wireless.c`、`BSP/bsp_uart.c`、`BSP/bsp_uart.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 函数 :78 | `BSP_UART_Read` | `BSP_UART_Read` | `App/wireless.c`、`BSP/bsp_uart.c`、`BSP/bsp_uart.h`、`tests/dual_mode_tracking.py` |
| 函数 :97 | `BSP_UART_RxAvailable` | `BSP_UART_RxAvailable` | `BSP/bsp_uart.c`、`BSP/bsp_uart.h` |
| 函数 :105 | `BSP_UART_RxFlush` | `BSP_UART_RxFlush` | `BSP/bsp_uart.c`、`BSP/bsp_uart.h` |
| 私有函数 :116 | `uart_rx_push` | `BSP_UART_PushRx` | `BSP/bsp_uart.c` |
| 函数 :134 | `HAL_UART_RxCpltCallback` | `HAL_UART_RxCpltCallback` | 固定入口 E-01；`BSP/bsp_uart.c` |
| 函数 :147 | `HAL_UART_ErrorCallback` | `HAL_UART_ErrorCallback` | 固定入口 E-01；`BSP/bsp_uart.c` |
| 函数 :164 | `fputc` | `fputc` | 固定入口 E-01；`BSP/bsp_uart.c` |
| 宏/枚举 :2 | `UART_RX_BUF_SIZE` | `BSP_UART_RX_BUF_SIZE` | `BSP/bsp_uart.c` |
| 对象 :20 | `s_rx_buf` | `s_rx_buf` | `BSP/bsp_uart.c` |
| 对象 :21 | `s_rx_head` | `s_rx_head` | `BSP/bsp_uart.c` |
| 对象 :22 | `s_rx_tail` | `s_rx_tail` | `BSP/bsp_uart.c` |
| 对象 :23 | `s_rx_byte` | `s_rx_byte` | `BSP/bsp_uart.c` |

### BSP/bsp_uart.h

验证：V-01/02/03/06/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 宏/枚举 :2 | `__BSP_UART_H` | `SMARTCAR_BSP_BSP_UART_H` | `BSP/bsp_uart.h` |

### BSP/bsp_w25q64.c

验证：V-01/02/03/06/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 私有函数 :51 | `w25q_cs_low` | `BSP_W25Q64_Select` | `BSP/bsp_w25q64.c` |
| 私有函数 :56 | `w25q_cs_high` | `BSP_W25Q64_Deselect` | `BSP/bsp_w25q64.c` |
| 私有函数 :61 | `w25q_read_status1` | `BSP_W25Q64_ReadStatus1` | `BSP/bsp_w25q64.c` |
| 私有函数 :81 | `w25q_wait_idle` | `BSP_W25Q64_WaitIdle` | `BSP/bsp_w25q64.c` |
| 私有函数 :103 | `w25q_write_enable` | `BSP_W25Q64_EnableWrite` | `BSP/bsp_w25q64.c` |
| 函数 :119 | `BSP_W25Q64_Init` | `BSP_W25Q64_Init` | `BSP/bsp_flash.c`、`BSP/bsp_w25q64.c`、`BSP/bsp_w25q64.h`、`tests/datalog_v2.py` |
| 函数 :198 | `BSP_W25Q64_Read` | `BSP_W25Q64_Read` | `BSP/bsp_flash.c`、`BSP/bsp_w25q64.c`、`BSP/bsp_w25q64.h`、`tests/datalog_v2.py` |
| 函数 :228 | `BSP_W25Q64_WritePage` | `BSP_W25Q64_WritePage` | `BSP/bsp_flash.c`、`BSP/bsp_w25q64.c`、`BSP/bsp_w25q64.h`、`tests/datalog_v2.py` |
| 函数 :267 | `BSP_W25Q64_EraseSector` | `BSP_W25Q64_EraseSector` | `BSP/bsp_flash.c`、`BSP/bsp_w25q64.c`、`BSP/bsp_w25q64.h`、`tests/datalog_v2.py` |
| 函数 :300 | `BSP_W25Q64_EraseBlock64K` | `BSP_W25Q64_EraseBlock64K` | `BSP/bsp_w25q64.c`、`BSP/bsp_w25q64.h` |
| 函数 :333 | `BSP_W25Q64_EraseChip` | `BSP_W25Q64_EraseChip` | `BSP/bsp_w25q64.c`、`BSP/bsp_w25q64.h` |
| 宏/枚举 :2 | `W25Q_CMD_WRITE_ENABLE` | `BSP_W25Q64_CMD_WRITE_ENABLE` | `BSP/bsp_w25q64.c` |
| 宏/枚举 :2 | `W25Q_CMD_READ_STATUS1` | `BSP_W25Q64_CMD_READ_STATUS1` | `BSP/bsp_w25q64.c` |
| 宏/枚举 :3 | `W25Q_CMD_READ_DATA` | `BSP_W25Q64_CMD_READ_DATA` | `BSP/bsp_w25q64.c` |
| 宏/枚举 :4 | `W25Q_CMD_PAGE_PROGRAM` | `BSP_W25Q64_CMD_PAGE_PROGRAM` | `BSP/bsp_w25q64.c` |
| 宏/枚举 :5 | `W25Q_CMD_ERASE_SECTOR` | `BSP_W25Q64_CMD_ERASE_SECTOR` | `BSP/bsp_w25q64.c` |
| 宏/枚举 :5 | `W25Q_CMD_ERASE_BLOCK64` | `BSP_W25Q64_CMD_ERASE_BLOCK64` | `BSP/bsp_w25q64.c` |
| 宏/枚举 :6 | `W25Q_CMD_ERASE_CHIP` | `BSP_W25Q64_CMD_ERASE_CHIP` | `BSP/bsp_w25q64.c` |
| 宏/枚举 :7 | `W25Q_CMD_JEDEC_ID` | `BSP_W25Q64_CMD_JEDEC_ID` | `BSP/bsp_w25q64.c` |
| 宏/枚举 :7 | `W25Q_CMD_RELEASE_PD` | `BSP_W25Q64_CMD_RELEASE_PD` | `BSP/bsp_w25q64.c` |
| 宏/枚举 :8 | `W25Q_STATUS_WIP` | `BSP_W25Q64_STATUS_WIP` | `BSP/bsp_w25q64.c` |
| 宏/枚举 :8 | `W25Q_JEDEC_ID` | `BSP_W25Q64_JEDEC_ID` | `BSP/bsp_w25q64.c` |
| 宏/枚举 :9 | `W25Q_CS_GPIO_PORT` | `BSP_W25Q64_CS_GPIO_PORT` | `BSP/bsp_w25q64.c` |
| 宏/枚举 :9 | `W25Q_CS_PIN` | `BSP_W25Q64_CS_PIN` | `BSP/bsp_w25q64.c` |
| 宏/枚举 :12 | `W25Q_SPI_TIMEOUT` | `BSP_W25Q64_SPI_TIMEOUT` | `BSP/bsp_w25q64.c` |
| 宏/枚举 :12 | `W25Q_PAGE_TIMEOUT_MS` | `BSP_W25Q64_PAGE_TIMEOUT_MS` | `BSP/bsp_w25q64.c` |
| 宏/枚举 :15 | `W25Q_SECTOR_TIMEOUT_MS` | `BSP_W25Q64_SECTOR_TIMEOUT_MS` | `BSP/bsp_w25q64.c` |
| 宏/枚举 :16 | `W25Q_BLOCK_TIMEOUT_MS` | `BSP_W25Q64_BLOCK_TIMEOUT_MS` | `BSP/bsp_w25q64.c` |
| 宏/枚举 :17 | `W25Q_CHIP_TIMEOUT_MS` | `BSP_W25Q64_CHIP_TIMEOUT_MS` | `BSP/bsp_w25q64.c` |
| 对象 :40 | `s_hspi1` | `s_hspi1` | `BSP/bsp_w25q64.c` |

### BSP/bsp_w25q64.h

验证：V-01/02/03/06/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 宏/枚举 :2 | `__BSP_W25Q64_H` | `SMARTCAR_BSP_BSP_W25Q64_H` | `BSP/bsp_w25q64.h` |
| 宏/枚举 :4 | `W25Q64_TOTAL_SIZE` | `BSP_W25Q64_TOTAL_SIZE` | `BSP/bsp_flash.c`、`BSP/bsp_flash.h`、`BSP/bsp_w25q64.c`、`BSP/bsp_w25q64.h`、`tests/datalog_v2.py` |
| 宏/枚举 :6 | `W25Q64_PAGE_SIZE` | `BSP_W25Q64_PAGE_SIZE` | `BSP/bsp_flash.c`、`BSP/bsp_w25q64.c`、`BSP/bsp_w25q64.h`、`tests/datalog_v2.py` |
| 宏/枚举 :7 | `W25Q64_SECTOR_SIZE` | `BSP_W25Q64_SECTOR_SIZE` | `BSP/bsp_flash.c`、`BSP/bsp_flash.h`、`BSP/bsp_w25q64.h`、`tests/datalog_v2.py` |

### BSP/bsp_wdt.c

验证：V-01/02/03/06/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 函数 :38 | `BSP_WDT_Init` | `BSP_WDT_Init` | `App/app.c`、`BSP/bsp_wdt.c`、`BSP/bsp_wdt.h`、`tests/dual_mode_tracking.py` |
| 函数 :63 | `BSP_WDT_Feed` | `BSP_WDT_Feed` | `App/app.c`、`App/datalog.c`、`BSP/bsp_wdt.c`、`BSP/bsp_wdt.h`、`tests/datalog_v2.py`、`tests/dual_mode_tracking.py` |
| 宏/枚举 :2 | `IWDG_KEY_RELOAD` | `BSP_WDT_KEY_RELOAD` | `BSP/bsp_wdt.c` |
| 宏/枚举 :3 | `IWDG_KEY_ENABLE` | `BSP_WDT_KEY_ENABLE` | `BSP/bsp_wdt.c` |
| 宏/枚举 :4 | `IWDG_KEY_ACCESS` | `BSP_WDT_KEY_ACCESS` | `BSP/bsp_wdt.c` |
| 宏/枚举 :5 | `IWDG_PRESCALER_64` | `BSP_WDT_PRESCALER_64` | `BSP/bsp_wdt.c` |
| 宏/枚举 :6 | `IWDG_RELOAD` | `BSP_WDT_RELOAD` | `BSP/bsp_wdt.c` |
| 宏/枚举 :6 | `IWDG_WINDOW_FULL` | `BSP_WDT_WINDOW_FULL` | `BSP/bsp_wdt.c` |
| 对象 :28 | `s_hiwdg` | `s_hiwdg` | `BSP/bsp_wdt.c` |

### BSP/bsp_wdt.h

验证：V-01/02/03/06/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 宏/枚举 :2 | `__BSP_WDT_H` | `SMARTCAR_BSP_BSP_WDT_H` | `BSP/bsp_wdt.h` |

### BSP/lcd/lcd.c

验证：V-01/02/03/06/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 私有函数 :12 | `lcd_rst_set` | `BSP_LCDIO_SetReset` | `BSP/lcd/lcd.c` |
| 私有函数 :13 | `lcd_rst_reset` | `BSP_LCDIO_ClearReset` | `BSP/lcd/lcd.c` |
| 函数 :52 | `LCD_Init` | `BSP_LCDIO_Init` | `BSP/bsp_lcd.c`、`BSP/lcd/lcd.c`、`BSP/lcd/lcd.h` |
| 私有函数 :72 | `lcd_init` | `BSP_LCDIO_InitBus` | `BSP/lcd/lcd.c` |
| 私有函数 :82 | `lcd_gettick` | `BSP_LCDIO_GetTick` | `BSP/lcd/lcd.c` |
| 私有函数 :87 | `lcd_writereg` | `BSP_LCDIO_WriteRegister` | `BSP/lcd/lcd.c` |
| 私有函数 :100 | `lcd_readreg` | `BSP_LCDIO_ReadRegister` | `BSP/lcd/lcd.c` |
| 私有函数 :112 | `lcd_senddata` | `BSP_LCDIO_SendData` | `BSP/lcd/lcd.c` |
| 私有函数 :121 | `lcd_recvdata` | `BSP_LCDIO_ReceiveData` | `BSP/lcd/lcd.c` |
| 宏/枚举 :12 | `LCD_CS_SET` | `BSP_LCDIO_CS_SET` | `BSP/lcd/lcd.c` |
| 宏/枚举 :12 | `LCD_CS_RESET` | `BSP_LCDIO_CS_RESET` | `BSP/lcd/lcd.c` |
| 宏/枚举 :18 | `LCD_RS_SET` | `BSP_LCDIO_RS_SET` | `BSP/lcd/lcd.c` |
| 宏/枚举 :20 | `LCD_RS_RESET` | `BSP_LCDIO_RS_RESET` | `BSP/lcd/lcd.c` |
| 宏/枚举 :21 | `LCD_SPI` | `BSP_LCDIO_SPI` | `BSP/lcd/lcd.c` |
| 对象 :37 | `st7735_pIO` | `s_st7735_io` | `BSP/lcd/lcd.c` |
| 对象 :48 | `st7735_pObj` | `g_bsp_lcdio_object` | `BSP/bsp_lcd.c`、`BSP/lcd/lcd.c`、`BSP/lcd/lcd.h` |

### BSP/lcd/lcd.h

验证：V-01/02/03/06/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 宏/枚举 :2 | `__LCD_H` | `SMARTCAR_BSP_LCD_LCD_H` | `BSP/lcd/lcd.h` |
| 对象声明 :13 | `st7735_pObj` | `g_bsp_lcdio_object` | `BSP/bsp_lcd.c`、`BSP/lcd/lcd.c`、`BSP/lcd/lcd.h` |
| 对象声明 :14 | `ST7735Ctx` | `ST7735Ctx` | 厂商对象 E-02；`BSP/bsp_lcd.c`、`BSP/lcd/lcd.c`、`BSP/lcd/lcd.h` |

### Core/Src/main.c

验证：V-01/02/03/05/07。公共声明同步同名头文件。

| 类别 / 定义行 | 旧名 | 新名 | 引用文件 / 例外 |
|---|---|---|---|
| 局部 USER CODE SysInit :98 | `PeriphClkInit` | `periph_clk_init` | `Core/Src/main.c` |

## M-01 规约更正（2026-10-16，用户已确认）

用户确认删除 datalog_status_t 下 uint8_t/uint16_t/uint32_t 及
electromagnetic_inputs_t 下 int16_t 四条伪字段；它们是基础类型，实际成员已完整列入映射。
现为 1008 条符号记录，441 条改名保持；不修改 C 类型、真实字段、顺序、布局或行为。
本增量在对应 G2/G3 实现之前独立提交，G1 已有提交与验证证据保持。
