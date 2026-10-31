# Phase 14: 光电/电磁双模式循迹 — 实现计划

2026-10-31文档迁移：**Phase → docs → 代码**。本页保留需求/任务/验收历史，不新增阶段、不修改实现或重新验收。

当前需求导航：[P14功能索引](../../../Docs/traceability.md#p14)。具体输入输出、状态/错误/时序、代码符号与验证方法由下列docs条目唯一维护。

## 任务分组与详细设计入口

历史分组/依赖/风险/回退和用户决定保留在下方原记录；本次未重开原实现任务。以后执行本阶段增量时，先确定对应需求和docs目标，按原前置及已确认决定分组；实现文件与符号不在本页复制维护。

| 原需求 | 详细契约/实现/验证入口 | 执行边界 |
|---|---|---|
| FR-01 来源 | [F06 灰度采集标定与质心](../../../Docs/sensing/sensors-and-calibration.md#f06) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-02 选择引脚 | [F06 灰度采集标定与质心](../../../Docs/sensing/sensors-and-calibration.md#f06) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-03 真实新转换 | [F06 灰度采集标定与质心](../../../Docs/sensing/sensors-and-calibration.md#f06) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-04 完整帧预算 | [F06 灰度采集标定与质心](../../../Docs/sensing/sensors-and-calibration.md#f06) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-05 手工标定 | [F06 灰度采集标定与质心](../../../Docs/sensing/sensors-and-calibration.md#f06) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-06 质心 | [F06 灰度采集标定与质心](../../../Docs/sensing/sensors-and-calibration.md#f06) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-07 双采集 | [F11 双FSM与唯一执行](../../../Docs/control/tracking-and-controllers.md#f11) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-08 BSP时基 | [F29 时钟DMA采样与驱动](../../../Docs/platform/board-and-bsp.md#f29)；[F06 灰度采集标定与质心](../../../Docs/sensing/sensors-and-calibration.md#f06) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-09 光电状态机 | [F10 光电状态机](../../../Docs/control/tracking-and-controllers.md#f10) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-10 电磁独立观察 | [F09 活动电磁元素](../../../Docs/control/elements-and-dormant-code.md#f09) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-11 唯一执行 | [F11 双FSM与唯一执行](../../../Docs/control/tracking-and-controllers.md#f11) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-12 并列参数 | [F16 参数v3格式](../../../Docs/services/parameters-and-tuning.md#f16)；[F12 部分配置与全局调参规划](../../../Docs/control/tracking-and-controllers.md#f12)；[F15 多入口参数应用](../../../Docs/services/parameters-and-tuning.md#f15) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-13 模式键 | [F02 起停、模式与首因](../../../Docs/system/runtime-and-safety.md#f02-start)；[F14 菜单按键与显示](../../../Docs/hmi/operation.md#f14) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-14 HMI路由 | [F14 菜单按键与显示](../../../Docs/hmi/operation.md#f14)；[F15 多入口参数应用](../../../Docs/services/parameters-and-tuning.md#f15) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-15 参数v3 | [F16 参数v3格式](../../../Docs/services/parameters-and-tuning.md#f16)；[F17 迁移保存与失败](../../../Docs/services/parameters-and-tuning.md#f17) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-16 保护与起跑 | [F02 起停、模式与首因](../../../Docs/system/runtime-and-safety.md#f02-start)；[F03 低压持续判定](../../../Docs/system/runtime-and-safety.md#f03)；[F04 fatal与底层启停失败](../../../Docs/system/runtime-and-safety.md#f04)；[F20 队列前台写入与故障](../../../Docs/services/recording-and-export.md#f20) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-17 日志格式 | [F19 日志格式容量](../../../Docs/services/recording-and-export.md#f19) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-18 生产发布 | [F20 队列前台写入与故障](../../../Docs/services/recording-and-export.md#f20)；[F11 双FSM与唯一执行](../../../Docs/control/tracking-and-controllers.md#f11)；[F25 独立上传](../../../Docs/services/communication.md#f25) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-19 前台写入 | [F20 队列前台写入与故障](../../../Docs/services/recording-and-export.md#f20)；[F17 迁移保存与失败](../../../Docs/services/parameters-and-tuning.md#f17) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-20 日志故障 | [F20 队列前台写入与故障](../../../Docs/services/recording-and-export.md#f20)；[F02 起停、模式与首因](../../../Docs/system/runtime-and-safety.md#f02-start) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-21 导出兼容 | [F21 CSV与会话边界](../../../Docs/services/recording-and-export.md#f21)；[F22 二进制流式导出](../../../Docs/services/recording-and-export.md#f22-export) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-22 追溯边界 | [F32 标准R08与维护](../../../Docs/guides/validation.md#f32)；[F29 时钟DMA采样与驱动](../../../Docs/platform/board-and-bsp.md#f29)；[F30 构建烧录与再生](../../../Docs/guides/build-and-flash.md#f30)；[F13 legacy与未接入元素](../../../Docs/control/elements-and-dormant-code.md#f13) | 当前契约及历史替代见docs；不改变原验收状态 |

原分组名（不改变次序/勾选，完整组内容见冻结记录）：

- Group 1：BSP 与灰度数据链
- Group 2：独立状态上下文与唯一执行口
- Group 3：模式、并列参数及界面
- Group 4：版本化日志与前台消费者
- Group 5：统一门禁、保护与再次起跑
- Group 6：验收回填
- Group 5追加：前台分支竞态闭环（规约增量先提交）

执行顺序：先读需求→对应docs→检查未决/依赖→纯规约提交→实现→所选验证及记录。回退以该次完整组为单位，同步回退接口/调用/文档，历史兼容和保护边界不借回退改变。纯文档检查不触发旧组软件门禁。























## 冻结历史基线（68cb508，非现行契约）

以下原文用于保留阶段决定、组顺序、需求历史与真实验证；不继续独立维护算法/布局/源码映射。当前契约先查上方docs条目。旧“本轮”“待执行”“当前”只指原记录时点，不覆盖后续决定。历史任务的全量软件门禁不自动成为以后每次修改的要求。

<details>
<summary>展开原始阶段记录（原文保留）</summary>

# Phase 14: 光电/电磁双模式循迹 — 实现计划

> **2026-10-16 Group 1～6代码、宿主验证及文档回填完成；前台竞态追加修复已验证。标定与实车验收待执行。**本文件保留上一轮交接历史，并记录本次已授权实现的映射与提交。目标契约及用户决定见 [requirements.md](requirements.md)，结果见 [validation.md](validation.md)。

## 交付与提交顺序

1. 在 `chore/constitution-dual-mode-tracking` 提交 mission/techstack/roadmap 的纯文档调整，新增 Phase 14，保留既有实车待验状态。
2. 从该提交创建 `feature/14-dual-mode-tracking`，单独提交本阶段规约三件套、关联规约增量指引、CHANGELOG 的“新增规约”条目及路线图文档进度。本次到此停止，不编写测试或驱动，不运行代码生成，不烧录。
3. 后续由用户另行启动实现；先复核下表文件与规约，再按任务组修改。发现新歧义或硬件预算不满足时，先询问并单独提交规约增量，不先改实现。
4. 每次提交前检查全仓库状态，展示本次明确文件清单、差异摘要和中文提交信息；逐路径暂存并核对，禁止 `git add .`/`-A`/`commit -a`，不推送。其他目录现有删除/移动及硬件在途改动不属于本任务。

## 源函数与目标职责

| TC387 来源 | STM32H743 目标 | 明确差异 |
|---|---|---|
| `Tracking_init` / PIN 初始化 | `BSP_Gray_Init`、`.ioc` GPIO 输出配置 | 使用已确认 S0～S3 接法，初值0000，不调用 TC387 ADC 初始化。 |
| `Tracking_Value_once` / `ADC_Read(ADC8)` | `BSP_ADC_ReadGrayFresh` + `BSP_Gray_ReadFrame` 内部单通道步骤 | 使用已有 ADC1 rank5，读取硬件推进后的五个新值；丢三均二，保留12bit，不忙读同一缓存。 |
| `Tracking_Value_Acquire` / `LQ_Tracking_Value` | 16通道临时帧→`Gray_Update`→`Gray_GetState` | 只发布完整帧，序号与状态分开；额外输出百分比、强度、有效性、偏差和扫描耗时。 |
| `Test_Tracking` | 不移植；由宿主输入验证、停车显示和上机清单替代 | 不复制其无限循环或对调度、LED/LCD的接管。 |

来源文件、校验值、版权信息必须保留在新增 BSP/灰度模块注释及本规约中。禁止把附件注释内“主循环调用采集”等描述当成绕过三线契约的指令。

## 需求—实现—验证映射

以下为本次已实现的需求映射；硬件验收状态以validation.md为准。新接口名及数据格式以 requirements 为准。

| 需求 | 文件与符号 | 验证 |
|---|---|---|
| FR-01 | 新 `BSP/bsp_gray.c/.h`、`App/gray_sensor.c/.h` 的来源注释及版本校验记录 | V-02 |
| FR-02 | `SmartCar.ioc`、`Core/Src/gpio.c`、`Core/Inc/main.h`、`BSP/bsp_gray.c::BSP_Gray_Init` | V-03、V-16 |
| FR-03 | `BSP/bsp_adc.c/.h::BSP_ADC_ReadGrayFresh`，新 `BSP/bsp_gray.c::BSP_Gray_ReadFrame` | V-04、V-16、V-17 |
| FR-04 | `BSP/bsp_gray.c::BSP_Gray_ReadFrame`、`App/gray_sensor.c::Gray_Update` | V-04、V-17 |
| FR-05 | 新 `App/gray_calibration.h`、`gray_sensor.c::Gray_ValidateCalibration`、`App/app.c` 起跑门禁 | V-05、V-16 |
| FR-06 | `gray_sensor.c::Gray_Normalize/Gray_ComputeError`、`gray_state_t` | V-06、V-16 |
| FR-07 | `App/app.c::App_Loop/App_ControlISR/App_SampleISR`、`track_sensor.c::Track_ReadAdc`、控制快照 | V-07、V-13、V-17 |
| FR-08 | 新 `BSP/bsp_time.c/.h::BSP_Time_*`、`BSP/bsp.h`、`bsp_adc`/`bsp_gray` 的有界等待 | V-04、V-17 |
| FR-09 | 新 `App/optical_fsm.c/.h::OpticalFSM_*`、`App/app.c` 启停与复位输入 | V-08、V-18 |
| FR-10 | 新 `App/electromagnetic_fsm.c/.h::ElectromagneticFSM_*`，`control.c`、`element.c`、`roundabout.c`、`motor.c`/`imu_proc.c` 只读增量入口 | V-09、V-10、V-18 |
| FR-11 | `App/control.c/.h::Control_UpdateLegacy/Tracking_*`、`pid.c/.h::PID_UpdateTrack` 及分模式方向历史、`track_sensor.c` | V-10、V-11、V-18 |
| FR-12 | `App/param.c/.h::Param_GetDirection/Param_SetDirection/Param_SelectGear/Param_GetFan/Param_SetFan`、两模式配置及 `control.c` 状态表 | V-11、V-12、V-19 |
| FR-13 | `BSP/bsp_key.c/.h::BSP_Key_PollEvent/BSP_Key_RequireRelease`，`app.c::App_TryStart` 切换/启动流程，`Tracking_TrySetMode` | V-08、V-11、V-18 |
| FR-14 | `App/menu.c::Menu_HandleKey/Menu_DrawContent`、`dashboard.c::Dashboard_Update`、`wireless.c::Wireless_Adjust` | V-11、V-19 |
| FR-15 | `App/param.c/.h::param_store_v1_t/param_store_v2_t/param_store_t/Param_Load/Param_Save/Param_TryFlushPending` | V-12、V-19 |
| FR-16 | `App/app.c/.h::App_TryStart/App_RequestStop/App_ResetRunState/App_FinalizeStop/App_ControlISR`、`motor.c::Motor_EmergencyStop` | V-07、V-08、V-14、V-17、V-18 |
| FR-17 | `App/datalog.c/.h` 的头部/记录序列化与布局断言，`BSP/bsp_flash.c/.h` 的边界检查 | V-13、V-15、V-20 |
| FR-18 | `App/datalog.c::Datalog_Push`、SPSC索引/快照，`app.c::App_SampleISR` | V-13、V-14、V-17、V-20 |
| FR-19 | `App/app.c::App_Loop/App_TaskISR`、`datalog.c::Datalog_Service/Datalog_Flush/Datalog_Stop` | V-13、V-14、V-17、V-20 |
| FR-20 | `App/app.c` 起跑门禁、`app_stop_reason_t`、`datalog.c::Datalog_Start/Datalog_Push/Datalog_Service/Datalog_GetStatus/Datalog_SetStopSummary`、`app_config.h` | V-14、V-18、V-20 |
| FR-21 | `App/datalog.c::Datalog_Dump/Datalog_Clear`、`wireless.c` 的 d/c 分支、`menu.c` 模式/档保存边界 | V-15、V-19、V-20 |
| FR-22 | `MDK-ARM/SmartCar.uvprojx` 源文件登记、`MDK-GCC/build.sh` 通配收集核对、`tests/control_equivalence.py` 及新增宿主测试、关联规约 | V-01、V-02、V-10、DOC-01～DOC-05 |

新模块选用 `gray_sensor`、`optical_fsm`、`electromagnetic_fsm`；不创建第二套硬件 ADC 驱动，不将 TC387 函数不加封装塞入 App。已有 `track_sensor` 保持电磁采样职责，`control` 负责模式与唯一输出提交，`param` 负责两模式参数 bank，`datalog` 负责格式和故障状态。

## 实现任务组

2026-10-16：Group 1～6代码、两LCD构建、宿主验证及文档回填已完成；手工标定仍全部为 -1。

### Group 1：BSP 与灰度数据链

- 新增 BSP 周期计数和灰度模块；在 bsp.h 汇总接口；将 ADC DMA 缓冲和计数当成会异步更新的数据，新增 rank5 新转换读取。
- 每次 PC4 取样先等待 NDTR 位于 rank5 之前，再等待它通过 rank5 的 DMA 写入位置；即使 rank5 恰在函数入口完成，也等待下一次，避免重复使用入口前的值。整个扫描共享同一700μs起点，DMA未启用/错误/停滞均有明确失败路径。
- 首先在隔离环境验证硬件计数算法、80次新转换、丢三均二、失败保留输出，再实现手工校验和强度/质心计算。初始16组黑/白值全部为-1，不能以全0/4095或合成信号解除实车门禁。
- 同步 `.ioc` 的 GPIO 输出及生成文件，保护 SPI45SEL 补丁、ADC十rank和其他外设。Keil登记新源文件，GCC不更改工具链。
- 完成后执行 V-01～V-06；真实扫描和串扰检查暂不能执行时，V-16/V-17仍待上机，不调整已确认预算来制造通过。

### Group 2：独立状态上下文与唯一执行口

- 从旧 `Control_UpdateLegacy/element/Roundabout_Update` 分离“判断/计算控制意图”和“写实际控制”的路径。现有启用转移按 requirements 表逐项搬入独立电磁上下文，不接 dormant 入口；旧可写全局状态的活动写入点统一由上下文管理。
- 每套状态使用私有上下文；将原带副作用的 judge 拆为对上下文的操作，实际电磁选中时才提交清零、增益、目标速度和转向意图。纯影子路径不得经过 Motor_Set*/Reset*、PID_Set* 或 IMU_ZeroAngleX。
- 电磁相对左右里程消费滤波后的左右增量，保持旧截断和先判断后读取的顺序；IMU 暴露只读本拍积分增量而不是让观察器重置实际角度。未选中时状态仍随真实运动推进，不虚构电磁模式下的车辆轨迹。
- 新增四态光电状态机及独立方向历史；共享方向公式可内部复用，但两模式的 error_last、状态配置和参数 bank 不共享。继续保留真实速度闭环实现。
- 修改现有等价性测试的适配层以调用新入口，保留电磁数值/状态顺序断言。旧脚本中“参数/日志格式完全不变”的断言应迁至 v1/v2历史布局回归，加上v3/v2日志断言；不得直接删除算法断言以取得通过。
- 完成后执行 V-01、V-08～V-11；尤其覆盖同拍十字/预环岛竞争、进环清零后再判断超时的顺序。

### Group 3：模式、并列参数及界面

- 实现明确 mode/gear 索引的参数 bank 与 v3 序列化；有效v1/v2迁移到88B，旧原始布局作为只读解码类型保留。共用速度PI和风扇，不把影子状态配置写入镜像。
- 菜单、串口都经参数接口更新当前模式；保留xp/xd语法/回显和3秒延迟保存。保存是按被编辑的mode/gear定向保存，不从停车时残留的另一套PID状态采集参数。
- PC13改为START/TOGGLE事件，修正短按的启动时刻为松开后；先持久化旧模式改动，成功才切换。菜单局部脏标志也纳入待保存判断，不能只处理串口脏标志。失败不重置历史或改变模式。
- 外接菜单模式标签和板载K区域使用ASCII，保持160×80与既有字库；本阶段不为板载仪表盘新增ADC按键菜单，板载配置仍可通过串口分别调整两模式。完整光电状态及电磁子状态通过日志查看。
- 完成后执行 V-01、V-08、V-11、V-12；实体Flash和界面结果按V-19回填，未实测不得标通过。

### Group 4：版本化日志与前台消费者

- 实现256B头/96B记录的显式布局、CRC、保留区、边界/大小断言及CSV57列。header捕获本次固定标定和两模式当前档存储参数，不能运行中跟随菜单变化。
- 使用1024槽SPSC与至少16bit索引，消费者批次缓冲768B，写成三个256B页；head只在完整记录拷贝后发布，tail只在成功写入后发布。维护accepted/written/rejected/pending和首个fault。
- 把刷写移到运行主循环，每次调用最多一批；Flash页/扇区事务会被高优先级控制抢占，但不会再阻塞TIM7电池保护。运行主循环保持禁LCD和调参。Flash轮询原有超时在此主循环可由SysTick推进，不把它搬回高优先级ISR。
- 写事务期间若ISR已停车，返回后先处理停止标志，禁止启动下一批运行写入；健康停止再有限排空，已写错则不重试故障批次。若尾包排空过程中再次出现写错，仅报告并停止排空，不能卡死停车界面。
- 首条从256写、每8条768B；Flash服务按原32B对齐允许96B记录跨页/跨扇区，验证参数扇区绝不擦写。旧32B日志不兼容，升级操作前先导出。
- 完成后执行 V-01、V-13～V-15；跨扇区吞吐与实际连续日志见V-20，不能凭1024槽证明任意Flash故障下不溢出。

### Group 5：统一门禁、保护与再次起跑

- 起跑门禁顺序：等待停车收尾完成→校验所有标定→取得双传感器有效快照→刷新待保存参数→日志配置/Flash/会话头检查→复位两套运行历史和采样序号→启动三线定时器。失败时只回显起跑拒绝，不置Start/Run/key标志。电机PWM在门禁全过程保持零。
- 控制拍从等待期即更新两套采样/保护；通过后才生成状态机意图和实际输出。TIM6总耗时检查包括采集、IMU/DL1B、状态判断、编码器和电机任务及错误路径；达到900μs就立即安全停车。
- 日志满容量要把已排队记录纳入判断。最后一条合法记录接受后请求停车，首条越界记录不得入队；缓冲溢出只能计入rejected并停车，不能丢样后继续运行。
- `App_RequestStop`仍是有界路径，允许从TIM15/TIM6/TIM7或主循环调用；保留首次原因和全部保护位，硬件立即置零后停TIM15/6/7；日志尾包和UART报告只在主循环。停车后先收尾再LCD，IWDG由主循环继续喂。
- 首次原因/fault的检查与锁存必须原子完成，使用BSP提供的极短临界区并恢复调用前中断屏蔽状态；临界区只含标志与枚举读写，不含SPI/UART/LCD、循环或Flash，避免前台/低优先级写错覆盖高优先级首次故障。
- 自动复位两套上下文和方向/速度历史，不改用户参数和手工标定，不再做IMU零偏校准；按键必须先稳定释放。完成V-01、V-07～V-15后，V-16～V-20均需实体设备。

### Group 6：验收回填

- 每组提交前记录实际构建、宿主测试、故障注入结果；保存失败证据，不把计划中的测试记成已执行。
- 标定由用户填写后先架空轮测左右方向和停车，再使用同时具备白线黑底及电磁信号的赛道验收。只有单一赛道信号时应触发保护，这是已确认行为。
- 回填本阶段验证、路线图和CHANGELOG；Phase08/12/13的既有实车待验项分别追踪，不因Phase14宿主测试通过而自动完成。

## 完成与回退条件

- **文档任务完成**：规约三件套、所有FR映射、用户决定、格式字节数/列数/容量、关联增量指引和文档范围检查通过，并作纯文档独立提交；随后停止。
- **后续实现完成**：代码已按规约实现，V-01～V-15实际通过，人工V-16～V-20与手工标定均通过，文档记录真实版本和结果。任一硬件项未执行时阶段不得勾选完成。
- 保留相邻实现提交作为回退点。若出现灰度超时、错误停车、影子副作用、存储格式不符或参数串写，停止实车验证，先保持PWM零和旧数据备份；回退整个相关任务组，不以禁用双保护/日志门禁或填假标定作为回退策略。
- 以上“本轮仅文档”限制属于2026-10-15的历史任务，2026-10-16用户已另行授权实现。本次已构建和宿主验证，未烧录。

## 本次实现追溯（2026-10-16）

| 任务组 | 提交 | 实现入口与实际验证 |
|---|---|---|
| Group 1 | f2a6edc | bsp_time/bsp_adc/bsp_gray/gray_sensor；V-01～06宿主/配置检查。 |
| Group 2 | 11013b0 | 私有electromagnetic_fsm/optical_fsm，Tracking_Observe/Execute、PID_DirectionStep、IMU_GetAngleDeltaX；V-08～10及原等价性。 |
| Group 3 | 8b02310 | 参数显式bank/v3、按键、菜单/串口路由、Tracking_TrySetMode；V-08/11/12。 |
| Group 4 | a2d5d08 | 日志v2/SPSC/Datalog_Service/前台写入/CSV、BSP临界区；V-13～15日志侧。 |
| Group 5 | 5c4274e | App_TryStart、App_ControlISR、App_RequestStop与收尾；V-05/07/08/11/14全链路整合。 |
| Group 5追加 | e260754规约 → d9e00db实现 | 前台分支定点中断的先收尾后界面回归；旧版失败/修复通过，两LCD0警告。 |
| Group 6 | 本次纯文档回填 | V-01～15实际结果、README/路线图/CHANGELOG与关联增量状态；V-16～20及标定仍待上机。 |

- 电磁距离经Tracking_AccumulateMotion→ElectromagneticFSM_Accumulate消费左右滤波增量；判断发生在本拍read_encoder之前。等待期只更新观测误差、保留初始状态与方向历史。
- Tracking_CompleteControl/Datalog_PublishControl在控制拍末发布序号及完整快照；超时路径补发停车摘要。Tracking_Stopped仅更新停止状态/活动输出，不重复推进电磁角度或状态。
- 新增BSP_Time_EnterCritical/BSP_Time_ExitCritical/BSP_Time_MemoryBarrier支持首次故障/停车原因锁存及SPSC发布，恢复调用前PRIMASK；Flash/UART/LCD均不在临界区。
- 不修改mission/techstack的治理约束，只同步本次实现状态；其他阶段实车结果与Phase09/10范围保留。

## Group 5追加：前台分支竞态闭环（规约增量先提交）

- 2026-10-16只读自查发现：App_Loop顶部读取stop_pending为0后，TIM6/TIM15/TIM7可停车并将key_flag清0，后续会直接进入停车界面分支，遗漏同轮收尾。目标顺序已由FR-16确定，无新设计选择。
- 影响仅App/app.c::App_Loop及tests/dual_mode_tracking.py::app_tests：在停车分支入口再查s_stop_pending并完成App_FinalizeStop，之后才采样/LCD/调参。运行分支仍每次最多一批，返回先处理停车。
- V-14追加定点调度：在顶部stop检查后、key_flag运行分支读取前注入一次真实App_RequestStop，日志带不足八条尾包；要求同轮先排空/报告，随后才允许停车采样或LCD/串口读取，报告一次。宿主临时副本只插入调度钩子，不改真实条件、控制或持久化算法。
- 本增量纯文档提交完成后再修实现；随后重跑两LCD构建及三个宿主入口，将结果回填。

- 追加闭环已完成：e260754先提交规约，d9e00db修复App_Loop分支竞态并加入真实C定点调度用例；最终两LCD及全部宿主回归通过，实际结果见validation.md。

> 2026-10-16 Phase 15 G3：现行调度、HMI、参数和日志名称已按[符号表](../15-code-style-consistency/symbol-map.md)统一。
> 本文原阶段行为记录与未完成待办不因此重新验收；现行运行链以 Phase 14 和当前头文件契约为准，历史 validation 保留原文。

## Phase 17 当前实现定位（2026-10-24）

双模式协调归control/tracking，模式切换门禁归system/lifecycle，两个FSM仍各自拥有观察和方向历史。灰度标定位置为App/sensing/gray_calibration.h，32项仍为-1且两模式均受起跑门禁；参数/记录按Phase17拆分owner，v3/v2格式不变。完整dual_mode_tracking入口的R07外部原始LQ_Track.c缺失仍独立记录，不删除或弱化检查。

本节是现行归属导航；上文历史计划、验证、提交路径及未完成实机项目保持。实际重构验证见
[Phase17验收](../17-app-architecture/validation.md)，逐符号迁移见[迁移导航](../17-app-architecture/migration-map.md)。

</details>
