# SmartCar — STM32H743VIT6 智能车（光电/电磁双模式）

> **规约文档入口**：本项目采用规约驱动开发（SDD）。宪章与功能规约见 [`specs/`](specs/roadmap.md)（从 `specs/roadmap.md` 看全局进度）；智能体/协作者请先读 [`AGENTS.md`](AGENTS.md)；变更历史见 [`CHANGELOG.md`](CHANGELOG.md)。

> **代码写作标准**：以后新增、修改和审查自维护固件 C/H 必须遵守
> [`specs/code-style.md`](specs/code-style.md)。2026-10-16 已按
> [Phase 15 规约](specs/features/15-code-style-consistency/requirements.md)完成 G1/G2/G3 全量实现，
> 各组双 LCD GCC 零警告零错误、三个现有宿主验证通过；AC5 和实车待验，不覆盖其他阶段门禁。

> **注释专项复核（2026-10-16）**：命名/排版已完成 G1～G3，注释内容及格式重新验收。
> 写作标准 v1.1 改为“C 完整、H 摘要”，统一文件、函数、阶段、字段和状态说明。
> 上述 G1～G3 为前次实现/自动验证记录，不代表 v1.1 注释验收通过。本轮只编写并提交规约，
> C/H 尚未按专项整改；旧构建/功能结果及实车待验状态保持。
> 模板及完整成品见 [注释样例](specs/features/15-code-style-consistency/comment-examples.md)，
> 逐函数/模块缺项见 [检查清单](specs/features/15-code-style-consistency/comment-review.md)。

基于 **WeAct MiniSTM32H7xx 核心板**，引脚分配见 `../引脚分配/STM32H743VIT6_引脚分配表.md`。
应用层算法移植自省赛工程 `Sirius20260718`（STC AI8051U 逐飞库）：4 路电感循迹 + 速度闭环
+ 当前启用的十字/右环岛 + IMU660RB 姿态 + DL1B 激光测距 + ADC 按键菜单 + 串口调参。
Phase14增加16路模拟灰度普通循迹、同级独立状态机/方向参数、双传感器保护及完整日志；未启用休眠元素。

**2026-10-16：代码和宿主验证已完成，实车仍待验。** 上电默认光电、低档；标定文件全部保留 `-1`，
因此当前构建在两种模式下都会拒绝起跑。填写真实标定并完成[Phase14实体清单](specs/features/14-dual-mode-tracking/validation.md)后再验收。

## 代码结构

```
SmartCar/
├── Core/          CubeMX 生成（main.c 仅在 USER CODE 区调用 App_Init/App_Loop）
├── Drivers/       HAL + CMSIS
├── BSP/           板级层：收拢全部 HAL 依赖
│   ├── bsp_uart / bsp_adc / bsp_pwm / bsp_encoder / bsp_key / bsp_wdt
│   ├── bsp_gray / bsp_time（16路复用采集、DMA新转换与DWT预算）
│   ├── bsp_sampler（TIM15 1ms 高速采样器：编码器+IMU，纯寄存器配置）
│   ├── bsp_lcd (+lcd/ ST7735 驱动，移植自核心板 SDK 03-LCD_Test)
│   ├── bsp_imu660rb（SPI4，CS=PD3）/ bsp_dl1b（I2C2）
│   ├── bsp_w25q64（板载 8MB SPI Flash，SPI1 纯代码配置）/ bsp_flash（参数/日志存储服务）
├── App/           应用层：控制算法，不直接碰 HAL
│   ├── app.c           应用总装：初始化、主循环、TIM6/TIM7 中断
│   ├── gray_sensor / gray_calibration    灰度归一化、质心、手工标定
│   ├── optical_fsm / electromagnetic_fsm    两套私有状态及控制意图
│   ├── track_sensor    4 路电感采集 + 加权差分归一化偏差（源 adc.c）
│   ├── pid / filter    方向 PD（非线性）+ 速度位置式 PI、一阶低通/高通
│   ├── motor           差速分配、速度环执行、编码器测速/里程
│   ├── imu_proc        IMU 读取、零偏校准、高通、角度积分（源 gyroscope.c）
│   ├── control          双模式协调及唯一执行口（保留原状态配置表）
│   ├── element / roundabout    原元素基准与配置；活动判断已收拢到电磁私有上下文
│   ├── menu / wireless / param          按键菜单、串口调参、Flash 参数存取
│   ├── datalog         TIM6完整快照 → TIM15入队 → 主循环W25Q64写入，CSV v2导出
└── MDK-ARM/       Keil V5 工程（已包含 App/BSP 分组与头文件路径）
```

## 运行时结构

- **TIM15（1ms，优先级 0）高速采样器**（`bsp_sampler`，纯寄存器配置，不占 CubeMX 资源）：
  1kHz 读取左右编码器（读即清零+2ms 累计）和 IMU 陀螺/加速度原始值（ODR 1.66kHz，
  每拍都是新数据），并触发 `App_SampleISR` 做数据记录。与 TIM6 同级互不抢占，SPI4 无交错。
- **TIM6（2ms，优先级0）**：每拍（含起跑等待）采四路电感与16路完整灰度帧，先判断双保护，
  再推进两套状态；g_app_control_ticks>1000才由选中模式提交方向/差速控制，随后编码器滤波/速度PI/PWM。
  本拍最后发布日志控制快照；灰度达到700μs或整个控制拍达到900μs立即安全停车。
- **TIM7（5ms，优先级1）**：电池低压保护、IWDG喂狗、状态灯；不执行Flash写入。
- **主循环**：停车刷新双传感器、菜单/LCD/串口及参数保存；运行每次最多写八条日志，不刷LCD/解析调参。
  PC13稳定短按松开生成启动事件，稳定长按1秒切换模式；运行中忽略事件，停车后须先稳定释放。
  起跑按标定→有效双采样→参数保存→日志头写入/读回的顺序检查，通过后复位历史并启动三线。
- **安全停车**：任一失线、采集失败、控制超时、低压或日志故障，立即PWM零、清速度PI、风扇回当前档空闲值，
  停TIM15/TIM6/TIM7。首次原因和全部已观察保护位锁存，主循环收尾/一次报告，继续喂狗；再次起跑清历史和序号，保留参数/标定且不重做IMU零偏校准。
- 屏幕仅停车态刷新，和运行态IMU共用SPI4；SPI1日志写入位于前台，可被高优先级控制抢占。

## 时钟树

- HSE 25 MHz → PLL1 (M=5, N=192, P=2) → **SYSCLK 480 MHz（拉满，VOS0，FLASH_LATENCY_4）**
- HCLK = 240 MHz（AHB ÷2）；**APB1/2/3/4 = 120 MHz（÷2）**——注意 APB 分频≠1 时定时器
  时钟倍频为 240 MHz，所有定时器 PSC 已 ×2 补偿（TIM6 仍 2ms、PWM 仍 17kHz，全部不变）
- PLL3Q = 80 MHz → SPI4 内核时钟（SPI4 ÷8 = **10 MHz**，LSM6DSR SPI 上限）
- ADC 内核时钟 = PLL2P 83.3 MHz（ADC 内 2 分频 = 41.6 MHz，采样 16.5 cyc，
  10 通道扫描帧率约 126 kHz，原 ~26 kHz）
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
| ADC1 | 10 通道扫描+DMA 循环，16bit 41.6MHz（BSP 内 >>4 归一化到 12bit）| 电感×4→灰度→按键×2→电池→备用×2（Rank 序）|
| USART1 (PA9/PA10) | 115200 8N1，中断接收+环形缓冲 | 调试 printf / 无线调参共用 |
| SPI4 (PE12/PE14/PE5) | 全双工主机 **10 MHz**（PLL3Q 80M÷8） | ST7735 + IMU660RB 共用（CS：LCD=PE11、IMU=PD3；IMU ODR 1.66kHz）|
| SPI2 (PB13/14/15) | 主机 15 MHz（PLL1Q 480M÷32） | WiFi 模块预留 |
| I2C2 (PB10/PB11) | Fast Mode 400 kHz | DL1B 激光测距（XS=PE8，无效距离=8192，2ms 轮询）|
| SPI1 (PB3/PB4/PD7/PD6) | 30 MHz，BSP 代码内配置（不占 .ioc） | 板载 W25Q64 8MB：参数区（末尾 4KB）+ 日志区（约 8MB，边写边擦）|
| 片内 Flash | **不使用** | 参数与日志全部存板载 W25Q64 |
| IWDG1 | 约 6s，代码内寄存器直配 | 失控保护（app_config.h 的 APP_WDG_ENABLE 开关）|

## 灰度标定与模式

- 填写 [App/gray_calibration.h](App/gray_calibration.h) 的16路黑底/白线实测值：ch0最左、ch15最右，范围0～4095、每路绝对差至少200；`0`合法，未填写值必须为`-1`。停车时可通过调试器观察`Gray_GetState()->raw/percent`获取读数。
- S0/S1/S2/S3=PE4/PE6/PE0/PE1，模拟输出PC4；每路丢前三个新转换，均后两个。物理方向、极性、串扰及700/900μs预算均需上机确认。
- 上电光电低档；短按松开起跑，长按1秒在停车收尾完成后切换，模式不掉电记忆。切换前保存失败会保留旧模式。
- 两模式都要求白线黑底及电磁信号同时有效；任一缺失均停车。光电首版无元素处理，电磁影子状态仅表示实际轨迹上的判断。

## 调参与菜单

- **ADC 分压键盘**：KEY_ADC1(PA2)=方向+确定组（UP/DOWN/OK/LEFT/RIGHT），
  KEY_ADC2(PA3)=RST/ADJUST1/ADJUST2/FOUR/BACK（⚠上机先按 HOME 页显示的原始值核对两组是否接反）。
- **页面**：HOME（实时电感/偏差/电池）→ ADC_ERR / SPD_DIS / GYRO 监视页；
  ADJUST1（慢速档）/ ADJUST2（快速档）调参当前模式：KP_x/K2P_x/KD_x（±0.001）、base_speed（±100），
  第 5 项以 OK 切换运行/空闲风扇值（FAN-R/FAN-I，±100，范围 0～10000）；松键或 BACK 保存。
- **串口调参**（115200）：`vp15`→KP_v=0.15、`xd20`→KD_x=0.20（字母定参数，数值×0.01）；xp/xd编辑当前模式，vp/vi共用；参数命令静默3秒后合并写入Flash，起跑/切换前刷新。菜单标签O1/O2/E1/E2，板载K区显示O1K0等。
- **数据记录**：起跑必须`DATALOG_ENABLE=1`且`DATALOG_DIV=1`。头256B、每条96B，TIM15每1ms入队，传感器每2ms更新；1024槽有效1023条，前台每批八条/768B，少于八条留待停车收尾。容量87336条（87.336s）；溢出、写错或满容量均停车，写错不重试。
  停车串口`d`导出元信息、57列CSV、RAM状态尾行及EOF；`c`只复位日志区，不启动或伪造标定。新起跑覆盖旧会话；升级前先导出旧32B日志，新固件不解析旧格式，也不提供断电续录/恢复条数。
- **参数**：88B v3镜像保留v1/v2迁移。每档各存电磁/光电KP/K2P/KD/base_speed，速度PI与风扇共用；保存从指定bank取值。光电两档初值2/0.008/15/100，K2D=1，尚未实车整定；电磁原状态表每拍覆写语义继续保留。
- 占空比统一0～10000万分比；每档运行/空闲风扇默认1600/1100。速度PI默认20/0.75，电磁原权重15/20/22/10。

## 上机检查清单

1. **编码器方向**：空转车轮，确认 `Motor_GetState()->left.real_speed/right.real_speed` 前进为正；反了改 `BSP/bsp_encoder.h`
   的 `BSP_ENCODER_L_INVERT/R_INVERT`。
2. **按键两组是否接反**：HOME 页看 KEY1/KEY2 原始值（见上调参一节）。
   **若显示乱码**：检查屏幕排线；本工程字模渲染必须用 SDK 的转置缓冲写法
   （`bsp_lcd.c` 的 `BSP_LCD_ShowChar`，该面板在 ROT180 横屏下坐标映射特殊），不要用常规
   行序写法——已踩过坑。
3. **屏幕**：默认横屏 ROT180，颠倒则改 `BSP/lcd/lcd.c` 的 Orientation；
   背光已按原理图/SDK 修正（PE10 低电平亮、CCR 越大越亮，默认 100%）。
   板载蓝灯 PE3 为**高电平点亮**（NPN 驱动），停车慢闪/运行常亮。
4. **IMU 校准**：上电保持车静止约 1s（`IMU_CalibrateGyro` 采样 100 次）。
   先完成16路实测标定；K1短按松开后通过全部门禁才起跑。
5. **480MHz 自检**：烧录后串口 115200 能正常通信即说明时钟树正确；若完全不启动，
   回退：main.c 的 PLLN 改回 96、FLASH_LATENCY_4 改回 1、APB 分频改回 DIV1，
   且 tim.c/.ioc 中各 TIM 的 PSC 还原（TIM1=12-1、TIM4/13/14/16/17=0、TIM6/7=120-1）。
6. **数据记录**：跑一圈→停车→串口发 `d` 导出 CSV（上限87336条，1ms/条=87.336s），验证采样链路。
7. **编译**：本工程代码量约 110KB，需要**正式版 Keil 授权**（评估版 32KB 限制会报
   L6050U）；或用 ARMCLANG/GCC 工具链自行构建。

## 相对源工程的有意修改

- 修复源 EEPROM 只写 float 前 2 字节的 bug（参数结构体按 4 字节完整存取 + CRC16）。
- 恢复上电陀螺零偏校准（源工程被注释）。
- 蜂鸣器（源工程与 DL1B 引脚冲突未用）不移植，状态指示改用板载蓝灯。
- 删除 STC 专有逻辑：0x7F 串口自动下载、STC 寄存器看门狗（改 IWDG）、`bit` 类型。
- 源快照的 TIM4 速度环/pit_speed 和测试函数不接入；`IMU_UpdateAccelVelocity` 的原实现与接口继续保留，不启用。
- 源工程上电不读 Flash（参数固定默认值）；本工程上电即恢复已存参数。

## Phase 15 命名导航

- 全量 67 个自维护 C/H 与 21 个 Core 用户区均已核对，固定 HAL/向量/运行库名称不变；旧名与新名见[完整符号表](specs/features/15-code-style-consistency/symbol-map.md)。
- 典型入口：`Track_ReadAdc`、`Motor_UpdateControl`、`IMU_UpdateGyroAngle`、`PID_UpdateSpeed`、`Menu_Update`、`Wireless_Adjust`；外部全局统一 `g_<module>_`，私有对象统一 `s_`。
- 日志 57 列 CSV、CRC、Flash 布局、32 个未标定 -1、LCD 默认 0 和全部休眠调用边保持；实际验证及待执行项见[Phase 15 validation](specs/features/15-code-style-consistency/validation.md)。
