# SmartCar — STM32H743VIT6 智能车（光电/电磁双模式）

> **规约文档入口**：本项目采用规约驱动开发（SDD）。宪章与功能规约见 [`specs/`](specs/roadmap.md)（从 `specs/roadmap.md` 看全局进度）；智能体/协作者请先读 [`AGENTS.md`](AGENTS.md)；变更历史见 [`CHANGELOG.md`](CHANGELOG.md)。

> **维护规则**：自维护 C/H 执行 [代码标准](specs/code-style.md)；后续开发强制保留格式、注释、逻辑审查、当前配置零警告编译和 R08，其他软件脚本按需。完整要求见 [AGENTS](AGENTS.md#验证要求2026-10-31-起)。

基于 **WeAct MiniSTM32H7xx 核心板**，引脚分配见 [引脚表](../Material/引脚分配/STM32H743VIT6_引脚分配表.md)。
电磁算法来源为 [STC 省赛快照](../Material/Sirius20260718/)，包含四路电感循迹、速度闭环及当前接入的十字/右环岛；光电采用 16 路模拟灰度，首版只有普通循迹与保护。两模式各自观察，只有选中模式控制电机；左环岛、跷跷板、路障等休眠入口未接入。

**当前状态（2026-10-31 文档核对）**：Phase17 五类架构的软件收口已有 2026-10-24 验收记录，实机项目仍待验；这不是本次重新运行了固件验证。上电默认光电、低档、板载屏（LCD=0）。[标定文件](App/sensing/gray_calibration.h) 的 32 项仍为 `-1`，因此两种模式均拒绝起跑。填写真实标定后仍须完成 [Phase14 实体清单](specs/features/14-dual-mode-tracking/validation.md)；所有阶段进度与剩余项见 [roadmap](specs/roadmap.md)。

## 构建与烧录入口

在仓库根使用 Git Bash 执行 `bash SmartCar/MDK-GCC/build.sh`，或使用 VS Code 默认构建任务；产物在 `SmartCar/MDK-GCC/`。烧录入口为 `bash SmartCar/MDK-GCC/flash.sh`，连接与执行须符合本次任务授权。

Keil 打开 `MDK-ARM/SmartCar.uvprojx`，使用锁定的 AC5，实际工程超出评估版 32KB 限制，需要正式授权。日常 GCC 是现有另一条构建入口；不把切换 ARMCLANG 或升级工具链当成已验证方案，具体版本与约束见 [techstack](specs/techstack.md)。

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
├── App/           五大类应用模块（目录内不嵌套）
│   ├── app.c/.h、app_config.h、app_types.h  固定入口/配置/公共业务类型
│   ├── system/    scheduler、lifecycle、safety、parameter_application、tuning_application
│   ├── sensing/   track_sensor、gray_sensor、gray_calibration.h、imu_proc、battery、filter、legacy_imu
│   ├── control/   tracking、control_profile、optical_fsm、electromagnetic_fsm
│   │              direction_controller、angle_controller、speed_controller、pid、motor
│   │              tuning、tuning_values、legacy_control、legacy_element、legacy_roundabout、legacy_pid
│   ├── services/  parameter_bank、parameter_store、parameter_image.h
│   │              run_record、logger、log_store、log_export、datalog.h
│   │              checksum、scb_frame、scb_json、scb_command_codec、command_dispatch
│   │              scb_link、telemetry、wireless
│   └── hmi/       menu、menu_keys、menu_render、dashboard
├── MDK-ARM/       Keil V5 工程（已包含 App/BSP 分组与头文件路径）
├── MDK-GCC/       GCC 构建、启动、链接与烧录入口；build 为产物/历史证据
├── tests/         已有宿主及结构检查，后续按需选用
└── specs/         目标、技术约束、路线图、代码标准及功能规约
```

## 运行时结构

- **TIM15（1ms，优先级 0）高速采样器**（`bsp_sampler`，纯寄存器配置，不占 CubeMX 资源）：
  1kHz 读取左右编码器（读即清零+2ms 累计）和 IMU 陀螺/加速度原始值（ODR 配置1.66kHz，数据新鲜度仍需实测），并触发 `App_SampleISR` 做数据记录。与 TIM6 同级互不抢占，SPI4 无交错。
- **TIM6（2ms，优先级0）**：每拍（含起跑等待）采四路电感与16路完整灰度帧，先判断双保护，
  再推进两套状态；Lifecycle_GetControlTicks()>1000才由选中模式提交方向/差速控制，随后编码器滤波/速度PI/PWM。
  本拍最后发布日志控制快照；灰度达到700μs或整个控制拍达到900μs立即安全停车。
- **TIM7（5ms，优先级1）**：电池低压保护、IWDG喂狗、状态灯；不执行Flash写入。
- **主循环**：停车刷新双传感器、菜单/LCD、旧串口及参数保存；运行每次最多写八条日志，不刷LCD或解析旧串口。
  运行与停车两态均在原位置服务SCB链路、上传和调参业务，Flash/LCD阻塞结束后才检查下一次服务到期。
  PC13稳定短按松开生成启动事件，稳定长按1秒切换模式；运行中忽略事件，停车后须先稳定释放。
  起跑按标定→有效双采样→参数保存→日志头写入/读回的顺序检查，通过后复位历史并启动三线。
- **安全停车**：任一失线、采集失败、控制超时、低压或日志故障，立即PWM零、清速度PI、风扇回当前档空闲值，
  停TIM15/TIM6/TIM7。首次原因和全部已观察保护位锁存，主循环收尾/一次报告，继续喂狗；再次起跑清历史和序号，保留参数/标定且不重做IMU零偏校准。
- 屏幕仅停车态刷新，和运行态IMU共用SPI4；SPI1日志写入位于前台，可被高优先级控制抢占。

## 时钟树

- HSE 25 MHz → PLL1 (M=5, N=192, P=2) → **SYSCLK 480 MHz（拉满，VOS0，FLASH_LATENCY_4）**
- HCLK = 240 MHz（AHB ÷2）；**APB1/2/3/4 = 120 MHz（÷2）**——注意 APB 分频≠1 时定时器
  对应定时器内核按240 MHz计算；实际周期由各外设PSC/ARR决定，TIM6目标2ms、驱动PWM约17kHz
- PLL3Q = 80 MHz → SPI4 内核时钟（SPI4 ÷8 = **10 MHz**，LSM6DSR SPI 上限）
- ADC 内核时钟 = PLL2P 83.3 MHz（ADC 内 2 分频 = 41.6 MHz，采样 16.5 cyc，
  10 通道扫描；实际新转换/整帧时序须结合DMA与实测确认）
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
| SPI2 (PB13/14/15) | 主机初始约1.875MHz（PLL1Q 480M÷256），驱动提供/64约7.5MHz接口，当前活动链未调用提速 | SCB/1 Wi-Fi SPI DMA链路，SCK仍待实测 |
| I2C2 (PB10/PB11) | Fast Mode 400 kHz | DL1B 激光测距（XS=PE8，无效距离=8192，2ms 轮询）|
| SPI1 (PB3/PB4/PD7/PD6) | 30 MHz，BSP 代码内配置（不占 .ioc） | 板载 W25Q64 8MB：参数区（末尾 4KB）+ 日志区（约 8MB，边写边擦）|
| 片内 Flash | 固件程序链接于此；不存业务参数/日志 | 参数与日志全部存板载 W25Q64 |
| IWDG1 | 约 6s，代码内寄存器直配 | 失控保护（app_config.h 的 APP_WDG_ENABLE 开关）|

## 灰度标定与模式

- 填写 [App/sensing/gray_calibration.h](App/sensing/gray_calibration.h) 的16路黑底/白线实测值：ch0最左、ch15最右，范围0～4095、每路绝对差至少200；`0`合法，未填写值必须为`-1`。停车时可通过调试器观察`Gray_GetState()->raw/percent`获取读数。
- S0/S1/S2/S3=PE4/PE6/PE0/PE1，模拟输出PC4；每路丢前三个新转换，均后两个。物理方向、极性、串扰及700/900μs预算均需上机确认。
- 上电光电低档；短按松开起跑，长按1秒在停车收尾完成后切换，模式不掉电记忆。切换前保存失败会保留旧模式。
- 两模式都要求白线黑底及电磁信号同时有效；任一缺失均停车。光电首版无元素处理，电磁影子状态仅表示实际轨迹上的判断。

## 调参与菜单

以下完整 ADC 按键菜单适用于 `LCD_TARGET_EXTERNAL=1` 的外接屏；默认 `0` 为板载单页仪表盘，不执行完整菜单交互。PC13 起跑/模式切换键与 ADC 键盘是不同输入。

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
2. **ADC 按键两组是否接反**：使用外接屏菜单时在 HOME 页看 KEY1/KEY2 原始值；默认板载屏可通过调试器读取 ADC。
   **若显示乱码**：检查屏幕排线；本工程字模渲染必须用 SDK 的转置缓冲写法
   （`bsp_lcd.c` 的 `BSP_LCD_ShowChar`，该面板在 ROT180 横屏下坐标映射特殊），不要用常规
   行序写法——已踩过坑。
3. **屏幕**：默认横屏 ROT180，颠倒则改 `BSP/lcd/lcd.c` 的 Orientation；
   背光已按原理图/SDK 修正（PE10 低电平亮、CCR 越大越亮，默认 100%）。
   板载蓝灯 PE3 为**高电平点亮**（NPN 驱动），停车慢闪/运行常亮。
4. **IMU 校准**：初始化完成前保持车静止；`IMU_CalibrateGyro` 连续读取100次，代码未在每次读取间显式延时或同步DRDY，不能据此断言校准固定耗时约1s。
   先完成16路实测标定；K1短按松开后通过全部门禁才起跑。
5. **480MHz 与外设时钟核对**：串口115200能通信只能说明该通信路径可用，不能证明完整时钟树正确。通过调试器核对RCC选择、PLL/总线分频，并测量定时/PWM/SPI时序；故障时对照已知配置定位，不按旧参数片段直接回退整套时钟。
6. **数据记录**：跑一圈→停车→串口发 `d` 导出 CSV（上限87336条，1ms/条=87.336s），验证采样链路。
7. **编译**：使用上文锁定的AC5或现有GCC入口；镜像体积以当次构建结果为准，不能用历史约110KB估算代替当前结果。

## 相对源工程的有意修改

- 修复源 EEPROM 只写 float 前 2 字节的 bug（参数结构体按 4 字节完整存取 + CRC16）。
- 恢复上电陀螺零偏校准（源工程被注释）。
- 蜂鸣器（源工程与 DL1B 引脚冲突未用）不移植，状态指示改用板载蓝灯。
- 删除 STC 专有逻辑：0x7F 串口自动下载、STC 寄存器看门狗（改 IWDG）、`bit` 类型。
- 源快照的 TIM4 速度环/pit_speed 和测试函数不接入；`IMU_UpdateAccelVelocity` 的原实现与接口继续保留，不启用。
- 源工程上电不读 Flash（参数固定默认值）；本工程上电即恢复已存参数。

## Phase 15 命名导航

- Phase15当时范围为67个自维护C/H与21个Core C/H的用户区；不是Phase17拆分后的当前文件统计。固定HAL/向量/运行库名称不变；旧名与新名见[完整符号表](specs/features/15-code-style-consistency/symbol-map.md)。
- 现行入口：`Track_ReadAdc`、`Motor_UpdateControl`、`IMU_UpdateGyroAngle`、`SpeedController_UpdateLeft/Right`（左右轮PI接口）、`Menu_Update`、`Wireless_Adjust`；原命名记录保留，App可写状态现已由各模块私有持有。
- 日志 57 列 CSV、CRC、Flash 布局、32 个未标定 -1、LCD 默认 0 和全部休眠调用边保持；实际验证及待执行项见[Phase 15 validation](specs/features/15-code-style-consistency/validation.md)。

## App 五大类（Phase 17）

| 目录 | 职责 |
|---|---|
| system | 调度、生命周期、安全 |
| sensing | 电感、灰度、IMU、电池、滤波 |
| control | 双模式、控制器、执行和调参 |
| services | 参数、日志、协议、上传 |
| hmi | 菜单、按键、显示 |

按[Phase17架构](specs/features/17-app-architecture/architecture.md)组织；各组实际验证和剩余项目见
[验收记录](specs/features/17-app-architecture/validation.md)。根保留app.c/app.h、app_config.h和app_types.h。
标定在App/sensing/gray_calibration.h，默认32项仍为-1，两模式均禁止起跑。

建议阅读顺序：app入口 → system/scheduler → lifecycle/safety → control/tracking → 两套FSM与
专属控制器 → sensing处理 → parameter_bank/store及system应用协调 → logger/导出 → SCB分层 → hmi。
旧路径、全部函数和状态的现行位置从[迁移导航](specs/features/17-app-architecture/migration-map.md)查找；
legacy_*隔离保留不等于全未调用，例如旧方向停车预览和起跑复位壳仍保留原调用。

## 开发验证与可选工具

日常固件改动按 [AGENTS 验证要求](AGENTS.md#验证要求2026-10-31-起) 执行：格式/命名、注释、上下文逻辑、当前配置零警告编译及 R08 强制保留；不默认生成软件测试脚手架或全量运行宿主测试。真实硬件测试与既有硬件待验项保留，纯文档变更不触发固件构建或行为回归。

已有可选入口如下，仅在任务涉及相应验收目标时选用；这些命令在 SmartCar 目录执行：

- 布局/架构/迁移：`python tests/app_layout_check.py`、`python tests/app_architecture_check.py`、`python tests/app_migration_check.py`；后两者需要 pycparser，运行前确认所用环境，不假定已有依赖。
- 双 LCD：仅在需要验证两种显示配置时分别构建，恢复原配置和已有产物。
- 资源对照：`python tests/phase17_resources.py --output <新的输出目录>`，需要 ARM GCC、Git Bash 及脚本依赖，使用全新输出目录；该工具会生成资源结果，保留历史证据，不能随普通修改例行覆盖执行。
- R08：固件实现提交前按 [契约](specs/features/15-code-style-consistency/r08-review-contract.md) 维护人工记录，在仓库根执行 `python SmartCar/tests/comment_review_check.py --base <本批基线完整提交ID>`。

R07 原始样本、AC5 及实机预算/波形/吞吐验收不得由宿主或 GCC 结果替代。电脑端命令行工具另见 [SmartCarLink](../Tools/SmartCarLink/README.md)；ESP32 桥接工程当前未在本仓库找到，任务书与模拟对端不等于已交付桥接固件。
