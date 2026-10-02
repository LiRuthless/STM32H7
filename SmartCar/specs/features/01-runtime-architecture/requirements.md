# Phase 01: 运行时架构与调度 — 需求

> as-built 规约化追认：以下需求逆向自已存在的代码，描述现状（含缺陷），作为整改对照基准。需求编号供 [validation.md](validation.md) 与后续 Phase 引用。

## 功能需求

### 启动与初始化

- FR-1: 上电后按固定顺序完成初始化链：UART(115200) → 参数加载（Param_Load，Flash 校验失败用默认并回写）→ ADC 十通道 DMA → 电池低通（α=0.65）→ 按键/编码器/PWM（风扇空闲占空比 1100）→ LCD（清屏+背光 100%）→ IMU → 陀螺零偏校准 → DL1B → 采样器配置（不启动）→ 仪表盘初始化。观测：串口可通信、屏幕点亮显示。
- FR-2: IMU660RB 初始化失败时系统**不得卡死**，串口输出 `IMU660RB init fail` 后继续启动。
- FR-3: 上电静止期间执行陀螺零偏校准（采样 100 次取平均），校准后角度积分从零起算。
- FR-4: `main()` 的应用入口只有 `App_Init()` / `App_Loop()` 两个调用，且位于 CubeMX USER CODE 区（重新生成代码不丢失）。

### 主循环（停车态）

- FR-5: 停车态（未按启动键）主循环先完成待处理的日志尾包刷写与停车原因回显，再持续执行：循迹链空跑刷新显示值、电池电压采样滤波、菜单/仪表盘刷新（`LCD_TARGET_EXTERNAL` 编译期二选一）、串口无线调参解析；IWDG 已启动时由主循环喂狗。
- FR-6: 停车态状态灯（PE3 蓝灯）以约 250ms 周期闪烁（4Hz 半周期）。
- FR-7: PC13 启动键按下沿（30ms 消抖）触发启动序列：日志区复位 → 清控制器/状态机/里程/IMU 运行历史 → 状态灯转常亮 → 开 TIM6/TIM7 中断 → 启 TIM15 采样器 → 首次起跑初始化 IWDG（后续起跑只喂狗）。观测：按键后 LED 由闪烁变常亮，车于约 2s 后起跑。

### 三线中断契约

- FR-8: TIM15 采样中断 1kHz：每拍读清左右编码器（提供 1ms 窗口值与 2ms 累计值两套接口）、经 SPI4 读取 IMU 陀螺/加速度原始值（单次约 30µs）、调用 `App_SampleISR`（运行中每拍 `Datalog_Push` 一条 32B 记录）。
- FR-9: TIM6 控制中断 500Hz（2ms）：节拍计数 → DL1B 轮询 →（启动后）陀螺积分 → 风扇运行占空比 → 起跑延时 1000 拍（2s）后执行 `whole_test()` 循迹主流程 → 编码器测速（取采样器 2ms 累计）→ 速度闭环与电机输出。非运行态目标速度清零。
- FR-10: TIM7 运行态辅助中断 200Hz（5ms）：`Datalog_Flush` 日志刷写 → 电池采样滤波 → 低压保护判定 → 喂狗 → 运行中维持状态灯常亮；安全停车后 TIM7 停止，停车态喂狗转由主循环承担。
- FR-11: 起跑延时：启动键按下后前 1000 个控制拍（2s）不执行循迹主流程（风扇已转、陀螺已积分），之后进入 `whole_test()`。

### 保护机制

- FR-12: 低压保护：电池滤波值 < 1220（≈11.5V，12bit 标度）且 > 300（排除采样异常）持续 200 拍（≈1s）时进入统一安全停车：清运行标志与速度 PI、两路电机 PWM 归零、风扇回空闲档、停止 TIM15/TIM6/TIM7，并由主循环完成日志收尾；脱离阈值区间即清零持续计数。
- FR-13: 独立看门狗 IWDG1 超时约 6s（LSI 32kHz÷64，RLR=2999），由 `WDG_ENABLE` 编译开关控制；看门狗在首次起跑时使能且只初始化一次，启动后不可关闭；运行态由 TIM7 喂狗，停车态由主循环喂狗。
- FR-14: 状态灯语义：停车慢闪 / 运行常亮；PE3 高电平点亮（NPN 驱动）。

## 技术约束

- TC-1: 全部控制系数按 TIM6 = 2ms 控制拍标定（techstack 硬性约束 2）；改周期必须重标定并同步规约。
- TC-2: TIM15 与 TIM6 同占 NVIC 优先级 0、互不抢占（techstack 硬性约束 3）——这是采样器累计变量免锁与 SPI4 总线（IMU 运行用 / LCD 停车用）不交错的架构前提。TIM7 优先级 1，不得高于二者。
- TC-3: 主循环只在停车态承担日志收尾、显示、调参与停车态喂狗；运行态（`key_flag==1`）不刷新屏幕，IMU 只在 TIM15 中断上下文读取。恢复屏幕刷新前必须已经停止 TIM15。
- TC-4: CubeMX 重新生成代码必须保留 `main.c` USER CODE SysInit 的 SPI45SEL→PLL3Q 补丁（直接写 `RCC->D2CCIP1R`；`HAL_RCCEx_PeriphCLKConfig` 实测不生效），且 `App_Init()` 开头需在全部 MX_*_Init 之后重写一次（techstack 硬性约束 5）。
- TC-5: D-Cache 保持关闭（techstack 硬性约束 6）；I-Cache 开启。
- TC-6: 无 RTOS；禁止在中断上下文做阻塞式 Flash 擦写以外的长耗时操作——TIM7 内 Flash 页编程（约 0.7ms/页）与扇区擦除（几十 ms，仅日志跨扇区时）为现状接受的例外。
- TC-7: IWDG1 用纯寄存器配置（HAL 未使能 IWDG 模块）；TIM15 同（不占 CubeMX 资源）。

## 接口约定

### 中断向量与节拍

| 中断源 | 周期 | NVIC (抢占,子) | 入口 | 职责 |
|---|---|---|---|---|
| TIM15 | 1ms | (0,0) | `TIM15_IRQHandler`（bsp_sampler.c，纯寄存器） | 编码器/IMU 采样 + `App_SampleISR` |
| TIM6_DAC | 2ms | (0,0) | `App_ControlISR` | 完整控制环 |
| TIM7 | 5ms | (1,0) | `App_TaskISR` | 日志刷写/电池/喂狗/状态灯 |
| DMA1_Stream0 | — | (2,0) | HAL DMA 链路 | ADC1 循环采集 |
| USART1 | — | (3,0) | HAL UART 链路 | 单字节接收入环形缓冲 |

TIM6/TIM7 经 `HAL_TIM_PeriodElapsedCallback`（app.c 唯一实现）分发；TIM15 不走 HAL 回调。

### 时钟树（SystemClock_Config + SysInit 补丁）

| 节点 | 数值 | 说明 |
|---|---|---|
| HSE | 25 MHz | 外部晶振 |
| PLL1 | M=5 / N=192 / P=2 | VCO 960MHz |
| SYSCLK | 480 MHz | VOS0，FLASH_LATENCY_4 |
| HCLK | 240 MHz | AHB ÷2 |
| APB1/2/3/4 | 120 MHz | ÷2；定时器时钟倍频 240MHz，PSC 已 ×2 补偿 |
| PLL3Q | 80 MHz | SPI4/5 内核时钟（SPI4 ÷8 = 10MHz） |
| ADC 内核 | PLL2P 83.3 MHz | 异步 ÷2 = 41.6MHz，采样 16.5cyc |

### 全局配置常量（`App/app_config.h`）

| 宏 | 值 | 语义 |
|---|---|---|
| `CTRL_PERIOD_MS` / `CTRL_DT_S` | 2 / 0.002f | TIM6 控制拍 |
| `TASK_PERIOD_MS` | 5 | TIM7 辅助拍 |
| `RUN_DELAY_COUNT` | 1000 | 起跑延时 2s |
| `WDG_ENABLE` | 1 | IWDG 编译开关（注释"约 1s"为漂移，实际 ≈6s） |
| `ADC_FULL_SCALE` | 4095 | 12bit 归一化满量程 |
| `BATTERY_LOW_THRESHOLD` / `BATTERY_MIN_VALID` / `BATTERY_LOW_COUNT` | 1220 / 300 / 200 | 低压保护 |
| `GYRO_RAW_TO_DPS` / `ACC_RAW_TO_G` | 14.3f / 4098.0f | ±2000dps / ±8g 换算 |
| `FAN_DUTY_IDLE` / `FAN_DUTY_RUN` | 1100 / 1600 | 风扇占空比（0~10000） |
| `DEBUG_UART_BAUD` | 115200 | USART1 |
| `DATALOG_ENABLE` / `DATALOG_DIV` | 1 / 1 | 数据记录开关/分频 |
| `LCD_TARGET_EXTERNAL` | 0 | 0=板载屏仪表盘，1=外接屏菜单 |

### 全局符号（`App/app.h`，源 config.c 命名沿用）

`uart[32]`/`dat[32]`（串口缓冲）、`battery`/`battery_filt`/`low_power_num`、`time`（2ms 节拍）、`key_flag`/`Start_flag`/`Run_flag`、`filt_battery`、`dl1b_distance_mm`（8192=无效）。应用层入口：`App_Init`/`App_Loop`/`App_ControlISR`/`App_TaskISR`/`App_SampleISR`。

### 启动键契约

PC13 下拉输入、按下为高电平；主循环轮询、按下沿 + 30ms 消抖（`BSP_Key_StartPressed`），一次按下只触发一次启动序列。

## 非目标（Non-goals）

- 不引入 RTOS 或软件定时器框架（裸机三线中断即全部调度）。
- 不提供运行态屏幕刷新与运行态串口调参以外的交互（屏幕与 IMU 分时复用 SPI4 靠"停车/运行"状态隔离，不做总线仲裁）。
- 不提供用户主动软停车/急停按键接口；统一安全停车入口当前只由低压保护和出赛道判定触发。
- 不覆盖 TIM6 内 `whole_test()` 之后的算法语义（属 [Phase 03](../03-track-following/requirements.md) / [Phase 04](../04-element-fsm/requirements.md)）、IMU 滤波积分细节（[Phase 05](../05-imu-attitude/requirements.md)）、菜单/调参协议（[Phase 06](../06-hmi-tuning/requirements.md)）、数据记录格式（[Phase 07](../07-datalog/requirements.md)）——本阶段只固定调用链与节拍。
- 不修复现状缺陷（含 `BSP_Flash_Init` 未调用导致的存储链失效），全部按 roadmap 转入 Phase 08–11。
