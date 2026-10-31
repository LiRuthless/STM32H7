# G2：运行编排、生命周期、安全与硬件边界 — 阶段入口

本页保留Phase17当时的设计/验收契约，不再独立维护现行功能和代码映射。

## 当前docs入口

- [system/runtime-and-safety.md](../../../docs/system/runtime-and-safety.md)
- [platform/board-and-bsp.md](../../../docs/platform/board-and-bsp.md)

旧迁移清单/JSON/评审及真实结果仍按原基线解释，不用历史章节替代现行docs或硬件验证。

## 冻结历史基线（68cb508，非现行契约）

以下原文用于保留阶段决定、组顺序、需求历史与真实验证；不继续独立维护算法/布局/源码映射。当前契约先查上方docs条目。旧“本轮”“待执行”“当前”只指原记录时点，不覆盖后续决定。历史任务的全量软件门禁不自动成为以后每次修改的要求。

<details>
<summary>展开原始阶段记录（原文保留）</summary>

# G2：运行编排、生命周期、安全与硬件边界

日期：2026-10-23。对应 FR-04/05/11/12，验证 V-02/03/04/05/06/07/08。
这是完整五大类重构授权内的行为保持拆分；必须先作纯规约提交，再修改本组代码。
依据 G1 搬迁后的 `App/app.c`、`app.h` 及全部 `g_app_*`、`HAL_*` 调用点。
本文件定义唯一实现，不改变控制、通信、Flash 或硬件事实；没有新增用户决策事项。

## 文件与符号去向

| 原位置/符号 | 唯一目标 | 职责 |
|---|---|---|
| app.c::App_Init | system/lifecycle.c::Lifecycle_Init | 原有完整初始化顺序 |
| App_Loop、App_ControlISR、App_TaskISR、App_SampleISR 函数体 | system/scheduler.c::Scheduler_Loop/ControlISR/TaskISR/SampleISR | 前台及三条调度线 |
| App_TryStart、App_ResetRunState | system/lifecycle.c::Lifecycle_TryStart、私有 Lifecycle_ResetRunState | 起跑事务及历史复位 |
| App_RequestStop、App_FinalizeStop | system/safety.c::Safety_RequestStop/FinalizeStop | 首因锁存、立即停车、前台收尾 |
| App_IsStopped 条件 | lifecycle.c::Lifecycle_IsStopped | 原三个条件及求值顺序 |
| 电池原始值、滤波值、滤波器 | sensing/battery.c | 单一电池采样状态所有者 |
| g_app_low_power_count | system/safety.c 的 s_low_count | 低压保护决策及持续计数 |
| HAL_TIM_PeriodElapsedCallback | BSP/bsp_runtime.c，同名固定回调 | TIM6/7 分派；其他实例无动作 |
| MODIFY_REG SPI45SEL 补丁、LED、TIM6/7 起停 | BSP/bsp_runtime.c/h | HAL、CubeMX 句柄和寄存器留 BSP |
| HAL_GetTick | BSP/bsp_time.c/h::BSP_Time_GetTick | 单次读取 HAL 毫秒计数 |
| motor.c 的方向 GPIO 写入 | BSP/bsp_pwm.c/h::BSP_PWM_SetDirection | 保持物理电平和左右调用顺序 |
| app.h::app_stop_reason_t | App/app_types.h | 原枚举名称、顺序和数值 0～8 不变 |
| g_app_uart_tx_buffer/rx_buffer | services/wireless.c 的 s_uart_tx_buffer/rx_buffer | 各 32 字节，静态清零，原读写与清零范围不变 |
| App_AbsFloat | G2 继续留 app.c/app.h | 由 G3 单独迁为旧环岛私有函数，不在本组重复变更 |

根 app.c 的五个外部入口仍保持签名：`void App_Init(void)`、`void App_Loop(void)`、
`void App_ControlISR(void)`、`void App_TaskISR(void)`、`void App_SampleISR(void)`，
分别单调用转发上表目标。既有 `void App_RequestStop(app_stop_reason_t reason)` 和
`uint8_t App_IsStopped(void)` 保留为业务门面，转发 Safety_RequestStop/Lifecycle_IsStopped。
根 app.h 仅依赖 `<stdint.h>` 和 app_types.h，删除 main.h 和全部状态 extern 声明。
App 内仅经 bsp.h 获取板级接口；不再直接引用 HAL/CubeMX 类型、句柄、引脚宏或寄存器。
新模块直接位于五类目录，无子目录；编译清单增量覆盖新增 C 一次。

## 生命周期状态和接口

全部接口在 system/lifecycle.h 声明；模块前缀 `Lifecycle`。
以下状态由 lifecycle.c 的 static 对象独占，存储类型与初始化不变：

| 原对象 | 新私有对象 | 类型/初值 | 写入上下文 |
|---|---|---|---|
| g_app_control_ticks | s_control_ticks | int32_t，0 | 起跑复位、TIM6 自增 |
| g_app_key_flag | s_key_flag | volatile uint8_t，0 | 起跑发布、首次停车清零 |
| g_app_start_flag | s_start_flag | volatile uint8_t，0 | 起跑发布、首次停车清零 |
| g_app_run_flag | s_run_flag | volatile uint8_t，0 | TIM6 等待后、旧 Control_UpdateLegacy、首次停车 |
| g_app_dl1b_distance_mm | s_dl1b_distance_mm | uint16_t，BSP_DL1B_INVALID | TIM6 原测距更新处 |
| s_wdt_started | 同名 | uint8_t，0 | 首次成功起跑后置 1 |
| s_flash_available | 同名 | uint8_t，0 | Param_Load 返回赋值处 |

| 签名 | 输入、输出、状态副作用及调用约束 |
|---|---|
| `void Lifecycle_Init(void)` | 原 App_Init 全流程；上电前台、可阻塞；不额外重置 static 状态，不启动定时器 |
| `uint8_t Lifecycle_TryStart(void)` | 原私有起跑入口改内部模块接口；成功 1，拒绝 0；只由停车前台启动事件调用；错误及已完成操作不回滚 |
| `uint8_t Lifecycle_IsStopped(void)` | 依次读取 `!s_start_flag && !s_key_flag && !Safety_IsStopPending()`；短路与 volatile 重读保留 |
| `uint8_t Lifecycle_IsStarted(void)` | 单次读取 s_start_flag；不锁、不缓存、不转换为整体快照 |
| `uint8_t Lifecycle_IsKeyActive(void)` | 单次读取 s_key_flag，同上 |
| `uint8_t Lifecycle_IsRunning(void)` | 单次读取 s_run_flag，同上 |
| `int32_t Lifecycle_GetControlTicks(void)` | 当前 2ms 拍计数，原 int32_t 语义，无额外屏障 |
| `uint16_t Lifecycle_GetDistanceMm(void)` | 最近一次控制拍 DL1B 距离，mm；8192 无效，失败保留规则由原 BSP 调用决定 |
| `void Lifecycle_AdvanceControlTick(void)` | 仅原 TIM6 自增点执行 `++s_control_ticks`，不改为饱和或无符号计数 |
| `void Lifecycle_SetDistanceMm(uint16_t distance_mm)` | 仅原 TIM6 赋值点调用，一次写入，不夹紧、不采集 |
| `void Lifecycle_MarkRunning(void)` | s_run_flag=1；仅原两个赋值点调用，包括未接入的 Control_UpdateLegacy 内部路径 |
| `void Lifecycle_ClearRunFlags(void)` | 严格执行 `s_start_flag = s_run_flag = s_key_flag = 0`；Safety 首次停车原 PRIMASK 临界区内调用，不自行进入/退出临界区 |
| `uint8_t Lifecycle_IsWatchdogStarted(void)` | 读 s_wdt_started；供 scheduler/safety 在原位置决定是否喂狗，不把喂狗移入 getter |

起跑发布的链式 `s_key_flag = s_start_flag = 1` 留在 lifecycle.c 原位置。
私有 Lifecycle_ResetRunState 依次执行：s_control_ticks=0、Safety_Reset、Gray_ResetSequence、
Motor_ResetRunState、PID_ResetAll、Control_ResetRunState、Roundabout_ResetRunState、
IMU_ResetRunState。Safety_Reset 内部依次写低压计数、首因、pending，
因此原跨对象写入顺序保持。滤波器、滤波值、距离、参数及 IMU 零偏不随起跑复位。

control.c/element.c/roundabout.c/tuning.c 的 ticks、distance、start/run 访问，以及
dashboard.c/datalog.c 的 distance 访问，逐表达式换成对应 getter；禁止提前缓存、合并重读。
element.c 和 roundabout.c 的手写 extern 一并删除。Control_UpdateLegacy 原 run_flag=1 换成
Lifecycle_MarkRunning；不新增活动调用，不改变其未接入状态。

## 电池模块

新增 sensing/battery.c/h，模块前缀 `Battery`。仅依赖 bsp.h 和 filter.h，
不依赖 app.h、system、日志、通信或 HMI。私有 int16_t s_raw/s_filtered 初值 0；
filter_lowpass_t s_filter 静态零初始化，仍由初始化流程在原位置设置 0.65f。

| 签名 | 契约 |
|---|---|
| `void Battery_Init(void)` | 原 Filter_InitLowpass(...,0.65f)，不额外清除采样值 |
| `void Battery_Update(void)` | BSP_ADC_Read(BSP_ADC_VBAT) 先转 int16_t 写 s_raw，再转 float 输入 Filter_UpdateLowpass，结果转 int16_t 写 s_filtered；保留转换顺序 |
| `int16_t Battery_GetRaw(void)` | 单次复制原 ADC 标度值，无锁、无副作用 |
| `int16_t Battery_GetFiltered(void)` | 单次复制滤波值，无锁、无副作用 |

scheduler 停车分支调用 Battery_Update，不执行低压保护；TIM7 先 Battery_Update，后
Safety_CheckLowVoltage，返回 1 才在原位置 App_RequestStop(APP_STOP_LOW_BATTERY)，
然后仍喂狗及按 start 标志写 LED。HMI 和日志仅改 Battery_GetFiltered 读值。
不把 TIM7 逻辑加上新的 started 门禁，不改变既有计数溢出或重复请求语义。

## 安全模块

新增 system/safety.c/h，前缀 `Safety`。独占 `static volatile uint8_t s_stop_pending=0`
和 `static volatile app_stop_reason_t s_stop_reason=APP_STOP_NONE`，以及
原 g_app_low_power_count 对应的 `static int16_t s_low_count=0`。
接口如下：

| 签名 | 契约 |
|---|---|
| `void Safety_RequestStop(app_stop_reason_t reason)` | 原 App_RequestStop 完整行为；ISR/前台可调、无阻塞 Flash；首因及所有副作用见下文 |
| `void Safety_FinalizeStop(void)` | 原 App_FinalizeStop；仅前台且 pending 为真时调，可阻塞；不新增内部早退门禁 |
| `uint8_t Safety_IsStopPending(void)` | 一次 volatile 读取 pending，供每个原检查点及测试使用 |
| `app_stop_reason_t Safety_GetStopReason(void)` | 一次 volatile 读取首因，用于诊断及测试；不清零 |
| `void Safety_Reset(void)` | 低压计数=0、首因=NONE，然后 pending=0；仅起跑原复位流程，定时链尚未启动时调用 |
| `uint8_t Safety_CheckLowVoltage(void)` | 按原顺序两次读取 Battery_GetFiltered，判断 `< APP_BATTERY_LOW_THRESHOLD && > APP_BATTERY_MIN_VALID`；满足时先 ++s_low_count，再比较 `> APP_BATTERY_LOW_COUNT`；真返回1，否则0；不满足区间则计数=0返回0；不采样、不发停车请求 |

Safety_RequestStop 首先检查 reason==APP_STOP_NONE 并直接返回，不写状态、保护位或事件，
不进入临界区、不操作硬件。非零原因按原 switch 将原因映射保护位，调用 Tracking_AddProtection，
再进入 BSP_Time_EnterCritical。first 判断保持 `(s_stop_reason == APP_STOP_NONE)`，
first 时按原顺序写 reason、pending、Lifecycle_ClearRunFlags，退出并恢复原 PRIMASK。
然后 Datalog_SetStopSummary，非 first 返回。first 继续依次 Tuning_Stop、
Motor_EmergencyStop、空闲风扇、Sampler_Stop、TIM6_Stop、TIM7_Stop、Tracking_Stopped、
RUN_STOPPED 事件。SCB 队列失败仍忽略，不重试，不覆盖首因。
非法非零枚举仍走原 switch 的 default，不新增范围检查或修复。

FinalizeStop 入口读取 reason 到局部；原位置有条件喂狗、DATALOG_ENABLE 下 Datalog_Stop、
Tuning_FinalizeStop、再次有条件喂狗、原 192 字节 message 与相同 snprintf/UART 文本；
最后 pending=0、BSP_Key_RequireRelease。首因不在收尾清零，只在下次成功起跑复位。
保护累计位置、首因优先顺序、临界区边界及日志阻塞上下文均不变化。

## 调度与起跑顺序不变量

Scheduler_Loop 保持以下独立检查，不改成 else，也不缓存 key/pending：
1. wdt_started 且 key==0 时喂狗；顶部 pending 收尾。
2. `if (Lifecycle_IsKeyActive())` 进入运行服务，Datalog_Service → Tuning_Service →
   SCB_Service → pending 收尾 → return。
3. 独立 `if (Lifecycle_IsKeyActive() == 0)` 进入停车服务；首先再检查 pending 收尾，
   然后 Tuning/SCB、原传感器和偏差、方向 PID、电池、按 LCD 编译分支界面、Wireless、
   Param_ServiceSave、250ms LED 翻转、按键事件及模式切换/起跑。

顶部检查之后可能发生停车 ISR；第 3 步二次收尾必须先于 UART、灰度和 LCD 操作。
停灯 s_led_last 仍为 scheduler 停车分支的 static uint32_t、初值0，不因起跑/停车重置。

Scheduler_ControlISR 保留每个保护计算、调用、goto complete、Datalog 二次摘要发布及
900us 判断位置。ticks 仍在采集前增加；第1000拍只等待，第1001拍开始执行控制。
等待期风扇、双FSM观察、编码器和累计运动等原顺序不变；不得合并两次 ticks 比较。
Scheduler_SampleISR 保留 started 检查后 Datalog_Push；不得在此调用 Flash。

Lifecycle_TryStart 保留门禁顺序：非停车 → 导出占用 → 电机切零 → 完整标定 →
双传感器新采样 → 灰度采集错误 → 灰度失线 → 电感和<=300 → 参数刷写 → 日志开始。
拒绝消息逐字保持，拒绝不启动定时链、不分配 run_id。成功后按原顺序复位运行历史、
Telemetry_BeginRun、Datalog_PublishControl、LED亮、发布 key/start、要求按键释放、
TIM6起、TIM7起、Sampler_Start、RUN_STARTED事件、首次启动或后续喂看门狗。
不新增异常回滚；原 HAL 起停返回值仍忽略，不扩展本组为错误处理修复。

## BSP 接口及调用点

新增 BSP/bsp_runtime.c/h，前缀 `BSP_Runtime`，bsp.h 聚合此头：

| 签名 | 唯一底层动作及上下文 |
|---|---|
| `void BSP_Runtime_ConfigureSpiClock(void)` | 原 MODIFY_REG(RCC->D2CCIP1R,...PLL3) 原样执行；Lifecycle_Init 第一动作，在所有 MX 初始化后 |
| `void BSP_Runtime_StartTimers(void)` | HAL_TIM_Base_Start_IT(&htim6) 然后 &htim7；前台起跑原位置 |
| `void BSP_Runtime_StopTimers(void)` | 忽略返回 HAL_TIM_Base_Stop_IT(&htim6) 然后 &htim7；首次停车原位置，前台/ISR |
| `void BSP_Runtime_SetRunLed(void)` | PE3 蓝灯 GPIO_PIN_SET；起跑和 TIM7 原位置，无缓存判断 |
| `void BSP_Runtime_ToggleLed(void)` | 原 HAL_GPIO_TogglePin；停车前台250ms判断位置 |

同一 BSP 文件定义唯一 HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)，
保持 `if TIM6` → App_ControlISR，`else if TIM7` → App_TaskISR；其他实例不调用。
回调不改定时器配置，不加入 TIM15 分支，现有采样器钩子保持。

bsp_time 增加 `uint32_t BSP_Time_GetTick(void)`，只返回 HAL_GetTick() 一次，单位ms、
uint32_t 自然回绕，不增加临界区。scheduler/dashboard/param/datalog/scb_link/telemetry
中全部 HAL_GetTick 调用逐点替换，不合并采样；说明注释同步真实接口。

bsp_pwm 增加 `void BSP_PWM_SetDirection(bsp_pwm_channel_t channel, uint8_t high)`：
L/R 依 channel 选择原 MOTOR_L/R_DIR_GPIO_Port/Pin，high 非0写 SET，否则 RESET；
FAN/其他枚举值无动作。不改变占空比。motor.c 原每个 GPIO 写改成该接口，
正向左高右低、反向左低右高，初始化左高再右高；不合并方向/PWM调用，
不把符号判断、幅值或死区移入 BSP。

BSP 公开头的现有传递 HAL 包含不作为本组扩张整改对象；App 源码不得再使用它泄漏的
HAL 名称。构建暴露缺少标准类型/字符串/数学声明时，为使用者显式补标准库头，
不恢复 app.h::main.h 依赖、不添加 HAL 兼容别名。

## 宿主测试接口与验证

生产接口独占状态，测试不得依靠包含 app.c 获取私有对象。现有实际集成测试改为编译
app.c、scheduler/lifecycle/safety/battery 真模块，新增内部头供测试读取；
App_TryStart断言改 Lifecycle_TryStart，s_stop_pending/s_stop_reason 断言改 Safety getter。
全部原拒绝、首次原因、累计位、计数、输出及日志断言保留，不以重写模型替代真实执行。

runtime_tuning 的人工状态布置通过仅宿主构建开启的接口：在 lifecycle.h/c 的
`#if defined(SMARTCAR_HOST_TEST)` 分支定义
`void Lifecycle_TestSetRunState(int32_t ticks, uint8_t key, uint8_t start, uint8_t run)`，
只按参数写 ticks/key/start/run，不操作硬件、不自行复位其他模块；生产工程不定义该宏。
测试原直接改 pending/reason 改 Safety_Reset；不得用公开可写指针或新全局量旁路封装。
control_equivalence 等独立算法夹具的原 g_app 状态改测试本地变量和 getter/command stub，
保留历史 git show 的符号映射及原输入向量；不改基线文件。

| 新验证项 | 方法与唯一通过标准 |
|---|---|
| G2-V1 | LCD=0/1 GCC 均0错误0警告，Keil源列表覆盖新增模块且无重复；App递归无HAL直接调用、CubeMX头/类型/寄存器使用 |
| G2-V2 | dual_mode_tracking 实际App集成子项、runtime_tuning、control_equivalence 继续执行全部旧断言；任何缺失外部R07样本单独记阻塞 |
| G2-V3 | 前台注入从 app.c 移至 scheduler.c，精确匹配唯一 `if (Lifecycle_IsKeyActive())` 前一位置，仍在顶部pending检查之后；临时副本插入且断言匹配次数为1。注入停车后灰度/UART/LCD fake全部断言 !Safety_IsStopPending()；只报告一次且尾包全部写完 |
| G2-V4 | 对真实 bsp_runtime.c、bsp_time新增函数及bsp_pwm方向函数以HAL fake验证：时钟补丁参数、TIM6→7起停、两个LED动作、TIM6/7/其他回调、左右方向及无效channel、tick回绕；原GPIO/PWM fake观察保留，不能用永远成功空stub替代硬件包装测试 |
| G2-V5 | 电池固定序列验证同原Filter计算和int16截断；安全模块在APP_BATTERY_MIN_VALID/LOW_THRESHOLD边界归零，连续区间第APP_BATTERY_LOW_COUNT次不停车、再一次返回低压；Safety_Reset清计数保留滤波历史；NONE停车请求零副作用；运行集成仍证实低压先停车后喂狗 |
| G2-V6 | datalog_v2/scb_protocol/scb_link/spi_wifi_state/spi_log_export全部旧断言；协议、CSV、二进制镜像、SCB事件顺序不改变 |
| G2-V7 | 全仓库搜旧 g_app 对象无生产定义/extern/读写残留，历史证据和测试基线映射除外；新增函数/字段完整CS注释与R08事实依赖，更新暂存证据后执行comment_review_check |

验证结果在 validation.md 回填执行命令、结果及未执行项。RAM/栈与G1比较，包装调用成本
不得被写成硬件预算通过证据；AC5与实机700/900us继续独立待验。失败时回退本组完整源码、
工程和测试改动，保留本规约及失败记录；不修改算法参数或协议解决重构验证失败。

</details>
