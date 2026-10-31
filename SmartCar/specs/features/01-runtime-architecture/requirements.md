# Phase 01: 运行时架构与调度 — 需求

2026-10-31文档迁移：**Phase → docs → 代码**。本页保留需求/任务/验收历史，不新增阶段、不修改实现或重新验收。

当前需求导航：[P01功能索引](../../../Docs/traceability.md#p01)。具体输入输出、状态/错误/时序、代码符号与验证方法由下列docs条目唯一维护。

## 需求意图与现行功能条目

编号身份及原用户决定保留；摘要不替代详细契约。涉及冲突时，以docs中并列的当前事实、既有要求和待决定边界阅读，不能自动选代码或旧文档为新设计。

<a id="fr-01"></a>
### FR-1 应用初始化

目标与实现依据：[F01 初始化与三线调度](../../../Docs/system/runtime-and-safety.md#f01)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-02"></a>
### FR-2 IMU失败初始化边界

目标与实现依据：[F29 时钟DMA采样与驱动](../../../Docs/platform/board-and-bsp.md#f29)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-03"></a>
### FR-3 陀螺校准

目标与实现依据：[F07 IMU校准滤波与积分](../../../Docs/sensing/sensors-and-calibration.md#f07)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-04"></a>
### FR-4 固定应用入口

目标与实现依据：[F01 初始化与三线调度](../../../Docs/system/runtime-and-safety.md#f01)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-05"></a>
### FR-5 停车前台

目标与实现依据：[F01 初始化与三线调度](../../../Docs/system/runtime-and-safety.md#f01)；[F14 菜单按键与显示](../../../Docs/hmi/operation.md#f14)；[F15 多入口参数应用](../../../Docs/services/parameters-and-tuning.md#f15)；[F02 起停、模式与首因](../../../Docs/system/runtime-and-safety.md#f02)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-06"></a>
### FR-6 停车灯

目标与实现依据：[F01 初始化与三线调度](../../../Docs/system/runtime-and-safety.md#f01)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-07"></a>
### FR-7 起跑序列

目标与实现依据：[F02 起停、模式与首因](../../../Docs/system/runtime-and-safety.md#f02)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-08"></a>
### FR-8 采样记录

目标与实现依据：[F20 队列前台写入与故障](../../../Docs/services/recording-and-export.md#f20)；[F29 时钟DMA采样与驱动](../../../Docs/platform/board-and-bsp.md#f29)；[F19 日志格式容量](../../../Docs/services/recording-and-export.md#f19)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-09"></a>
### FR-9 控制拍

目标与实现依据：[F01 初始化与三线调度](../../../Docs/system/runtime-and-safety.md#f01)；[F09 活动电磁元素](../../../Docs/control/elements-and-dormant-code.md#f09)；[F08 方向角度速度与执行](../../../Docs/control/tracking-and-controllers.md#f08)；[F11 双FSM与唯一执行](../../../Docs/control/tracking-and-controllers.md#f11)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-10"></a>
### FR-10 辅助拍

目标与实现依据：[F03 低压持续判定](../../../Docs/system/runtime-and-safety.md#f03)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-11"></a>
### FR-11 等待边界

目标与实现依据：[F01 初始化与三线调度](../../../Docs/system/runtime-and-safety.md#f01)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-12"></a>
### FR-12 低压停车

目标与实现依据：[F03 低压持续判定](../../../Docs/system/runtime-and-safety.md#f03)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-13"></a>
### FR-13 看门狗

目标与实现依据：[F01 初始化与三线调度](../../../Docs/system/runtime-and-safety.md#f01)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-14"></a>
### FR-14 状态灯

目标与实现依据：[F01 初始化与三线调度](../../../Docs/system/runtime-and-safety.md#f01)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="tc-01"></a>
### TC-1 控制周期、优先级与平台不变量（第1项）

目标与实现依据：[F01 初始化与三线调度](../../../Docs/system/runtime-and-safety.md#f01)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="tc-02"></a>
### TC-2 控制周期、优先级与平台不变量（第2项）

目标与实现依据：[F01 初始化与三线调度](../../../Docs/system/runtime-and-safety.md#f01)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="tc-03"></a>
### TC-3 控制周期、优先级与平台不变量（第3项）

目标与实现依据：[F01 初始化与三线调度](../../../Docs/system/runtime-and-safety.md#f01)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="tc-04"></a>
### TC-4 控制周期、优先级与平台不变量（第4项）

目标与实现依据：[F29 时钟DMA采样与驱动](../../../Docs/platform/board-and-bsp.md#f29)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="tc-05"></a>
### TC-5 控制周期、优先级与平台不变量（第5项）

目标与实现依据：[F29 时钟DMA采样与驱动](../../../Docs/platform/board-and-bsp.md#f29)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="tc-06"></a>
### TC-6 控制周期、优先级与平台不变量（第6项）

目标与实现依据：[F20 队列前台写入与故障](../../../Docs/services/recording-and-export.md#f20)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="tc-07"></a>
### TC-7 控制周期、优先级与平台不变量（第7项）

目标与实现依据：[F29 时钟DMA采样与驱动](../../../Docs/platform/board-and-bsp.md#f29)。状态：当前契约及历史替代见docs；不改变原验收状态。

## 授权与完成边界

本轮仅迁移说明。历史用户决定、动机、来源、兼容性及未决事项完整保留在下方基线；新的行为决定须明确日期、理由和适用范围，更新本需求及docs目标后独立提交，再进入实现。不得用本次文档完成替代AC5、R07原样本或任何硬件待验。

## 冻结历史基线（68cb508，非现行契约）

以下原文用于保留阶段决定、组顺序、需求历史与真实验证；不继续独立维护算法/布局/源码映射。当前契约先查上方docs条目。旧“本轮”“待执行”“当前”只指原记录时点，不覆盖后续决定。历史任务的全量软件门禁不自动成为以后每次修改的要求。

<details>
<summary>展开原始阶段记录（原文保留）</summary>

# Phase 01: 运行时架构与调度 — 需求

> 2026-10-18 目标增量：[Phase 16](../16-spi-wifi-link/requirements.md) FR-01/04/12/16～18/21
> 计划增加两态前台通信、完整控制拍调参及导出互斥，保持三线周期/优先级和原保护。本轮只提交文档，尚未实现；验收以[Phase 16验证](../16-spi-wifi-link/validation.md)另行记录。

> **2026-10-16 增量状态**：[Phase 14](../14-dual-mode-tracking/requirements.md) FR-07/08/13/16/18/19/20 已在三线周期与优先级不变的前提下，增加双采样/双保护、按键短长按、起跑门禁与控制超时，并把日志刷写移到运行主循环。下文 TIM7 刷写、按下沿起跑及等待期不采集的描述保留为修改前契约；当前对应增量、映射和验收以 [Phase 14 计划](../14-dual-mode-tracking/plan.md) / [验证](../14-dual-mode-tracking/validation.md) 为准，代码与宿主验证已完成，实车待验。 2026-10-16实现状态及V-01～15通过证据见上述Phase14验证页，V-16～20仍待上机。

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
- FR-9: TIM6 控制中断 500Hz（2ms）：节拍计数 → DL1B 轮询 →（启动后）陀螺积分 → 风扇运行占空比 → 起跑延时 1000 拍（2s）后执行 `Control_UpdateLegacy()` 循迹主流程 → 编码器测速（取采样器 2ms 累计）→ 速度闭环与电机输出。非运行态目标速度清零。
- FR-10: TIM7 运行态辅助中断 200Hz（5ms）：`Datalog_Flush` 日志刷写 → 电池采样滤波 → 低压保护判定 → 喂狗 → 运行中维持状态灯常亮；安全停车后 TIM7 停止，停车态喂狗转由主循环承担。
- FR-11: 起跑延时：启动键按下后前 1000 个控制拍（2s）不执行循迹主流程（风扇已转、陀螺已积分），之后进入 `Control_UpdateLegacy()`。

### 保护机制

- FR-12: 低压保护：电池滤波值 < 1220（≈11.5V，12bit 标度）且 > 300（排除采样异常）持续 200 拍（≈1s）时进入统一安全停车：清运行标志与速度 PI、两路电机 PWM 归零、风扇回空闲档、停止 TIM15/TIM6/TIM7，并由主循环完成日志收尾；脱离阈值区间即清零持续计数。
- FR-13: 独立看门狗 IWDG1 超时约 6s（LSI 32kHz÷64，RLR=2999），由 `APP_WDG_ENABLE` 编译开关控制；看门狗在首次起跑时使能且只初始化一次，启动后不可关闭；运行态由 TIM7 喂狗，停车态由主循环喂狗。
- FR-14: 状态灯语义：停车慢闪 / 运行常亮；PE3 高电平点亮（NPN 驱动）。

## 技术约束

- TC-1: 全部控制系数按 TIM6 = 2ms 控制拍标定（techstack 硬性约束 2）；改周期必须重标定并同步规约。
- TC-2: TIM15 与 TIM6 同占 NVIC 优先级 0、互不抢占（techstack 硬性约束 3）——这是采样器累计变量免锁与 SPI4 总线（IMU 运行用 / LCD 停车用）不交错的架构前提。TIM7 优先级 1，不得高于二者。
- TC-3: 主循环只在停车态承担日志收尾、显示、调参与停车态喂狗；运行态（`g_app_key_flag==1`）不刷新屏幕，IMU 只在 TIM15 中断上下文读取。恢复屏幕刷新前必须已经停止 TIM15。
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
| `APP_CTRL_PERIOD_MS` / `APP_CTRL_DT_S` | 2 / 0.002f | TIM6 控制拍 |
| `APP_TASK_PERIOD_MS` | 5 | TIM7 辅助拍 |
| `APP_RUN_DELAY_COUNT` | 1000 | 起跑延时 2s |
| `APP_WDG_ENABLE` | 1 | IWDG 编译开关（注释"约 1s"为漂移，实际 ≈6s） |
| `APP_ADC_FULL_SCALE` | 4095 | 12bit 归一化满量程 |
| `APP_BATTERY_LOW_THRESHOLD` / `APP_BATTERY_MIN_VALID` / `APP_BATTERY_LOW_COUNT` | 1220 / 300 / 200 | 低压保护 |
| `IMU_GYRO_RAW_TO_DPS` / `IMU_ACC_RAW_TO_G` | 14.3f / 4098.0f | ±2000dps / ±8g 换算 |
| `MOTOR_FAN_DUTY_IDLE` / `MOTOR_FAN_DUTY_RUN` | 1100 / 1600 | 风扇占空比（0~10000） |
| `APP_DEBUG_UART_BAUD` | 115200 | USART1 |
| `DATALOG_ENABLE` / `DATALOG_DIV` | 1 / 1 | 数据记录开关/分频 |
| `LCD_TARGET_EXTERNAL` | 0 | 0=板载屏仪表盘，1=外接屏菜单 |

### 全局符号（`App/app.h`，源 config.c 命名沿用）

`g_app_uart_tx_buffer[32]`/`g_app_uart_rx_buffer[32]`（串口缓冲）、`g_app_battery_raw`/`g_app_battery_filtered`/`g_app_low_power_count`、`g_app_control_ticks`（2ms 节拍）、`g_app_key_flag`/`g_app_start_flag`/`g_app_run_flag`、`g_app_battery_filter`、`g_app_dl1b_distance_mm`（8192=无效）。应用层入口：`App_Init`/`App_Loop`/`App_ControlISR`/`App_TaskISR`/`App_SampleISR`。

### 启动键契约

PC13 下拉输入、按下为高电平；主循环轮询、按下沿 + 30ms 消抖（`BSP_Key_StartPressed`），一次按下只触发一次启动序列。

## 非目标（Non-goals）

- 不引入 RTOS 或软件定时器框架（裸机三线中断即全部调度）。
- 不提供运行态屏幕刷新与运行态串口调参以外的交互（屏幕与 IMU 分时复用 SPI4 靠"停车/运行"状态隔离，不做总线仲裁）。
- 不提供用户主动软停车/急停按键接口；统一安全停车入口当前只由低压保护和出赛道判定触发。
- 不覆盖 TIM6 内 `Control_UpdateLegacy()` 之后的算法语义（属 [Phase 03](../03-track-following/requirements.md) / [Phase 04](../04-element-fsm/requirements.md)）、IMU 滤波积分细节（[Phase 05](../05-imu-attitude/requirements.md)）、菜单/调参协议（[Phase 06](../06-hmi-tuning/requirements.md)）、数据记录格式（[Phase 07](../07-datalog/requirements.md)）——本阶段只固定调用链与节拍。
- 不修复现状缺陷（含 `BSP_Flash_Init` 未调用导致的存储链失效），全部按 roadmap 转入 Phase 08–11。

> 2026-10-16 Phase 15 G2：现行传感器/控制符号按[完整映射](../15-code-style-consistency/symbol-map.md)迁移，
> 配置、计算顺序及休眠调用边保持；历史 validation 原文及本阶段实车门禁不变。

> 2026-10-16 Phase 15 G3：现行调度、HMI、参数和日志名称已按[符号表](../15-code-style-consistency/symbol-map.md)统一。
> 本文原阶段行为记录与未完成待办不因此重新验收；现行运行链以 Phase 14 和当前头文件契约为准，历史 validation 保留原文。

## Phase 17 当前位置（2026-10-23）

App按system/sensing/control/services/hmi归类。现行文件及逐函数位置见[迁移清单](../17-app-architecture/migration-map.md)。
历史路径、符号与验证结果保留；本说明不表示后续职责拆分或硬件验收完成。

</details>
