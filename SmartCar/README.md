# SmartCar — STM32H743VIT6 智能车（电磁循迹）

基于 **WeAct MiniSTM32H7xx 核心板**，引脚分配见 `../引脚分配/STM32H743VIT6_引脚分配表.md`。
应用层算法移植自省赛工程 `Sirius20260718`（STC AI8051U 逐飞库）：4 路电感循迹 + 速度闭环
+ 环岛/十字/跷跷板/障碍元素状态机 + IMU660RB 姿态 + DL1B 激光测距 + ADC 按键菜单 + 串口调参。

## 代码结构

```
SmartCar/
├── Core/          CubeMX 生成（main.c 仅在 USER CODE 区调用 App_Init/App_Loop）
├── Drivers/       HAL + CMSIS
├── BSP/           板级层：收拢全部 HAL 依赖
│   ├── bsp_uart / bsp_adc / bsp_pwm / bsp_encoder / bsp_key / bsp_wdt
│   ├── bsp_lcd (+lcd/ ST7735 驱动，移植自核心板 SDK 03-LCD_Test)
│   ├── bsp_imu660rb（SPI4，CS=PD3）/ bsp_dl1b（I2C2）/ bsp_flash（Bank2 Sector7 模拟 EEPROM）
├── App/           应用层：控制算法，不直接碰 HAL
│   ├── app.c           应用总装：初始化、主循环、TIM6/TIM7 中断
│   ├── track_sensor    4 路电感采集 + 加权差分归一化偏差（源 adc.c）
│   ├── pid / filter    方向 PD（非线性）+ 速度位置式 PI、一阶低通/高通
│   ├── motor           差速分配、速度环执行、编码器测速/里程
│   ├── imu_proc        IMU 读取、零偏校准、高通、角度积分（源 gyroscope.c）
│   ├── element / roundabout / control   元素判断、环岛状态机、kernel 主状态机
│   ├── menu / wireless / param          按键菜单、串口调参、Flash 参数存取
└── MDK-ARM/       Keil V5 工程（已包含 App/BSP 分组与头文件路径）
```

## 运行时结构

- **TIM6（2ms，优先级 0）完整控制环**（对应源工程 pit_track）：陀螺仪积分 →
  read_adc → kernel 状态机设参 → get_track_error → PID_track → speed_control 差速 →
  read_encoder → PID_L/R_pos 速度环 → PWM 输出。所有系数按 2ms 标定，与源工程一致。
- **TIM7（5ms，优先级 1）辅助任务**：DL1B 测距、电池低压保护（≈11.5V 持续 1s 强制停车）、
  IWDG 喂狗、状态灯（PE3 蓝灯：运行常亮/停车慢闪）。
- **主循环**：停车状态跑菜单（屏幕刷新/按键/串口调参）；PC13 启动键按下后开 TIM6/TIM7、
  使能 IWDG（约 6s 超时，给 Flash 参数保存的扇区擦除留余量）。
- 屏幕只在停车时刷新；IMU 只在运行时读取——两者共用 SPI4，互不冲突。

## 时钟树（与 SDK 03/07 示例一致）

- HSE 25 MHz → PLL1 (M=5, N=96, P=2) → **SYSCLK 240 MHz**
- HCLK = 120 MHz，APB1/2/3/4 = 120 MHz（定时器时钟均为 120 MHz）
- ADC 内核时钟 = PLL2P 83.3 MHz（ADC 内再 4 分频 ≈ 20.8 MHz）
- I-Cache 开启，**D-Cache 关闭**（ADC DMA 缓冲免 Cache 维护；
  且 DMA 缓冲已绝对定位到 D2 SRAM1 0x30000000，DMA1 可达——DTCM 不可被 DMA1 访问）

## 外设配置摘要

| 外设 | 配置 | 用途 |
|------|------|------|
| TIM13_CH1 (PA6) | PWM 17 kHz（ARR=7059-1）| 左电机（DIR=PE7，正转=高）|
| TIM4_CH3 (PD14) | PWM 17 kHz | 右电机（DIR=PE15，正转=低）|
| TIM14_CH1 (PA7) | PWM 17 kHz | 负压风扇（源工程为 100Hz，本工程 17kHz；如需改回在 CubeMX 调）|
| TIM16_CH1 (PB8) / TIM17_CH1 (PB9) | PWM 17 kHz | 备用 PWM |
| TIM1_CH2N (PE10) | PWM 10 kHz，极性 Low | 板载 ST7735 背光 |
| TIM5 (PA0/PA1) | 编码器 TI12 四倍频，滤波 6 | 左编码器（⚠TTa 脚，限 3.3V）|
| TIM3 (PC6/PC7) | 编码器 TI12 四倍频，滤波 6 | 右编码器（FT 脚，兼容 5V）|
| TIM6 / TIM7 | 2ms / 5ms 中断 | 控制环 / 辅助任务 |
| ADC1 | 10 通道扫描+DMA 循环，16bit（BSP 内 >>4 归一化到 12bit）| 电感×4→灰度→按键×2→电池→备用×2（Rank 序）|
| USART1 (PA9/PA10) | 115200 8N1，中断接收+环形缓冲 | 调试 printf / 无线调参共用 |
| SPI4 (PE12/PE14/PE5) | 全双工主机 7.5 MHz | ST7735 + IMU660RB 共用（CS：LCD=PE11、IMU=PD3）|
| SPI2 (PB13/14/15) | 主机 15 MHz | WiFi 模块预留 |
| I2C2 (PB10/PB11) | Fast Mode 400 kHz | DL1B 激光测距（XS=PE8，无效距离=8192）|
| Flash Bank2 Sector7 | 0x081E0000，128KB | 参数存储（结构体+CRC16，整扇区读-改-擦-写）|
| IWDG1 | 约 6s，代码内寄存器直配 | 失控保护（app_config.h 的 WDG_ENABLE 开关）|

## 调参与菜单

- **ADC 分压键盘**：KEY_ADC1(PA2)=方向+确定组（UP/DOWN/OK/LEFT/RIGHT），
  KEY_ADC2(PA3)=RST/ADJUST1/ADJUST2/FOUR/BACK（⚠上机先按 HOME 页显示的原始值核对两组是否接反）。
- **页面**：HOME（实时电感/偏差/电池）→ ADC_ERR / SPD_DIS / GYRO 监视页；
  ADJUST1（慢速档）/ ADJUST2（快速档）调参：KP_x/K2P_x/KD_x（±0.001）、base_speed/fan_duty（±100），
  松手或 BACK 时写入 Flash。
- **串口调参**（115200）：`vp15`→KP_v=0.15、`xi20`→KD_x=0.20 式命令（字母定参数，数值×0.01）。
- **PID 默认值**：KP_v=20.0、KI_v=0.75；方向环权重 weight_x/xx/y/abs=15/20/22/10。
- 占空比统一 0~10000 万分比（BSP 内部换算 CCR），源工程的限幅 7500/死区 500/风扇 1100/1600 直接沿用。

## 上机检查清单

1. **编码器方向**：空转车轮，确认 `real_speed_L/R` 前进为正；反了改 `BSP/bsp_encoder.h`
   的 `BSP_ENCODER_L_INVERT/R_INVERT`。
2. **按键两组是否接反**：HOME 页看 KEY1/KEY2 原始值（见上调参一节）。
3. **背光极性**：若不亮/反相，改 `BSP/bsp_lcd.c` 中 `BSP_LCD_SetBacklight` 的 CCR 换算方向。
4. **屏幕方向**：默认横屏 ROT180，颠倒则改 `BSP/lcd/lcd.c` 的 Orientation。
5. **IMU 校准**：上电保持车静止约 1s（gyro_calibrate 采样 100 次）。
6. **编译**：本工程代码量约 110KB，需要**正式版 Keil 授权**（评估版 32KB 限制会报
   L6050U）；或用 ARMCLANG/GCC 工具链自行构建。

## 相对源工程的有意修改

- 修复源 EEPROM 只写 float 前 2 字节的 bug（参数结构体按 4 字节完整存取 + CRC16）。
- 恢复上电陀螺零偏校准（源工程被注释）。
- 蜂鸣器（源工程与 DL1B 引脚冲突未用）不移植，状态指示改用板载蓝灯。
- 删除 STC 专有逻辑：0x7F 串口自动下载、STC 寄存器看门狗（改 IWDG）、`bit` 类型。
- 删除半废弃代码：TIM4 速度环/pit_speed、read_accel_velocity（接口保留）、测试函数。
- 源工程上电不读 Flash（参数固定默认值）；本工程上电即恢复已存参数。
