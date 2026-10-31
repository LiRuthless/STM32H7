# SmartCar/App 文件与函数存在必要性审查报告

- 审查日期：2026-10-24
- 代码基线：`7961f2da3e31acd0cc59457606c8d7c2348af06f`
- 审查范围：`SmartCar/App` 下全部 95 个 C/H 文件、365 个 App 函数定义
- 审查方式：逐文件源码、全仓调用关系、Phase 01～17 相关规约及休眠代码清单交叉核对
- 结构校验：`python SmartCar/tests/app_architecture_check.py` 通过，确认 95 C/H、365 个定义、五类目录及 Keil 清单一致

## 1. 结论先行

1. **“函数行为必须存在”不等于“必须保留为独立函数”。** 除 Core/BSP 固定入口、公开模块契约和状态隔离边界外，很多私有辅助函数可以内联或合并，但其计算、写入顺序和失败语义不能随函数名一起删除。
2. **当前活动控制、安全停车、双传感器保护、参数持久化、日志、SCB 通信和两套 LCD 配置均有真实依赖，不能按“代码看起来重复”直接删减。**
3. **`legacy_*` 不是当前运行链普遍必需，但也不能整目录删除。** 其中仍有起跑复位、停车前台预览、旧环岛内部调用等真实边；其余休眠算法属于 Phase 09/11 的待接入或待处置代码，项目规则要求逐项确认。
4. **最适合精简的是无活动调用的旧整体入口、旧角度包装、旧增量 PI 和已休眠元素/环岛分支。** 推荐先决定“继续保留作未来功能/测试”还是“彻底移除”；若移除，必须同步声明、调用、状态、常量、规约和测试，而不是只删函数体。
5. **头文件和聚合头不是冗余注释文件。** 它们承担类型、布局、条件编译和依赖隔离；尤其 `datalog.h`、`parameter_image.h`、`app_types.h` 直接影响协议或持久化兼容。
6. 本报告是静态必要性审查，不把 GCC/宿主通过解释为实车必要性功能已验证。AC5、700/900 μs、SPI 波形、持续吞吐和实车回归仍是项目既有待验项。

## 2. 判定口径

| 标记 | 含义 | 能否直接删除 |
|---|---|---|
| **运行必需** | 当前活动初始化、前台、ISR、控制、安全或记录链使用 | 否；删除会破坏构建或运行行为 |
| **边界必需** | 固定入口、跨模块公开契约、条件编译配置或状态封装需要 | 否；可在同步所有调用方后改边界 |
| **行为必需/边界可并** | 操作必须保留，但独立函数不是唯一实现方式 | 只能内联/合并实现，不能删行为 |
| **条件必需** | 当前 LCD 配置可能未编译，但另一受支持配置需要 | 不能因当前宏值直接删 |
| **休眠保留** | 当前活动链不调用或仅旧内部调用，作为未来功能、对照或历史行为保留 | 不是运行必需；须专项决策后完整移除或接入 |
| **可简化** | 单层转发、重复 getter 或过细边界有合并空间 | 可调整结构，但需证明无外部契约/并发影响 |

文件层面的“保留”也不表示物理文件绝对不能合并；它表示当前职责和依赖有存在依据。任何实现调整都应先按 `SmartCar/AGENTS.md` 完成规约先行，并另行获得休眠代码处置确认。

## 3. 逐文件、逐函数审查

> 下表中的行号指函数定义所在 C 文件；同名 H 的保留理由在对应文件条目中单独说明。“可并”只建议改变代码组织，不建议删除其功能。

### 3.1 `hmi/`：人机交互（8 文件，17 函数）

| 文件 | 存在原因 | 修改/删除条件 |
|---|---|---|
| `hmi/dashboard.c` | 板载屏固定标签与停车态 100 ms 数据刷新；当前默认 LCD=0 正在使用。 | 只有正式取消板载屏并同步生命周期、调度及 Phase 06/17 后才删。 |
| `hmi/dashboard.h` | 向 lifecycle/scheduler 公开板载显示接口。 | 随功能删除，或实现与全部调用方合为单一编译单元后私有化。 |
| `hmi/menu.c` | 页面、光标、编辑和按键沿唯一 owner；page/arrow 在两种 LCD 下均持久化。 | 取消菜单还涉及 88 B 参数镜像兼容，不能只删外接显示代码。 |
| `hmi/menu.h` | scheduler、参数应用、render 的跨模块契约。 | 依赖消失或重新合并模块后才删。 |
| `hmi/menu_keys.c` | 两路 ADC 三读均值、区间判键、第二路覆盖与显示滤波。 | 正式取消 ADC 键盘后才删；仅为少文件而合回 menu 收益很低。 |
| `hmi/menu_keys.h` | 菜单扫描、ADC 显示和键值常量的共享边界。 | 使用方全部并入同文件或按键功能删除后才删。 |
| `hmi/menu_render.c` | 所有菜单 LCD 调用与显示缓存唯一 owner；隔离导航和绘制。 | 可经新架构决策合回 menu，但会撤销 Phase 17 G4 已验证边界。 |
| `hmi/menu_render.h` | Reset/PreparePage/Draw 三阶段公开契约。 | 实现合回 menu 且全部私有化后才删。 |

| 函数（定义行） | 必要性 | 必须存在的原因 / 修改方案 |
|---|---|---|
| `Dashboard_Init` (`dashboard.c:24`) | 条件必需 | 板载初始化清屏、固定标签、清时基；并入 Update 会改变 LCD 时序。 |
| `Dashboard_Update` (`dashboard.c:45`) | 条件必需 | 板载 100 ms 数据刷新及停车前台 SPI4 边界，无冗余可删。 |
| `Menu_HandleKey` (`menu.c:41`) | 条件必需 | 外接菜单导航/编辑及 BACK 先保存语义；可经规约和测试同步后私有化，不能删功能。 |
| `Menu_Update` (`menu.c:236`) | 条件必需 | 外接菜单总编排，固定 RST、保存、页面准备和绘制顺序。 |
| `Menu_GetPage` (`menu.c:280`) | 运行必需 | render 与实际参数保存均读取；LCD=0 也不能删。 |
| `Menu_GetArrow` (`menu.c:289`) | 运行必需 | 光标绘制和持久化共同使用，避免公开可写状态。 |
| `Menu_GetFanEditIdle` (`menu.c:298`) | 条件必需 | FAN-R/FAN-I 标签与数值显示的唯一只读通道。 |
| `Menu_RestorePosition` (`menu.c:309`) | 运行必需 | 上电无条件恢复并校验菜单 page/arrow，LCD=0 仍执行。 |
| `Menu_ReadKeyAverage` (`menu_keys.c:28`) | 行为必需/边界可并 | 两路各三读均值的共同私有算法；可内联但会复制逻辑，无删除收益。 |
| `Menu_ScanKey` (`menu_keys.c:46`) | 条件必需 | 外接 ADC 键盘唯一入口；判键原值与显示滤波值不可混删。 |
| `MenuKeys_GetAdc1` (`menu_keys.c:111`) | 条件必需 | HOME 页 K1 接线/校准显示。 |
| `MenuKeys_GetAdc2` (`menu_keys.c:120`) | 条件必需 | HOME 页第二组按键接线/校准显示，物理通道不同。 |
| `Menu_DrawContent` (`menu_render.c:45`) | 条件必需 | 六类页面实际文本、坐标、颜色和实时值绘制。 |
| `Menu_DrawCursor` (`menu_render.c:165`) | 条件必需 | 仅变化时擦/画光标；与内容每轮刷新是不同策略。 |
| `MenuRender_Reset` (`menu_render.c:185`) | 条件必需 | RST 当拍清屏并使缓存失效；不能用 PreparePage 替代。 |
| `MenuRender_PreparePage` (`menu_render.c:196`) | 条件必需 | 页面切换清屏和缓存准备，保持既定调用时点。 |
| `MenuRender_Draw` (`menu_render.c:210`) | 边界必需/可简化 | 隐藏内容→光标两阶段顺序；若取消 render 模块可整体内联，当前不应让 menu 越过私有边界。 |

**HMI 结论：**没有可无条件删除项。若只追求减少文件，可在新架构规约下合并 keys/render/menu，但这只是文件组织变化；若取消一种 LCD 方案，则属于产品功能收缩，必须同时处理条件调用、持久化和验证。

### 3.2 `sensing/`：传感器与滤波（13 文件，37 函数）

| 文件 | 存在原因 | 修改/删除条件 |
|---|---|---|
| `sensing/track_sensor.c/.h` | 四电感状态、八读去极值、纯误差算法及共享观察的唯一 owner；控制、日志、HMI 共用类型。 | 只有完整取消电磁执行、影子观察、保护和记录后才可整体删；不能并入 BSP。 |
| `sensing/gray_sensor.c/.h` | 手工标定校验、归一化、质心、整帧发布与失败保留；BSP 仅负责采集。 | 取消光电还涉及双保护和 v2 日志/CSV 兼容；不得把算法移入 BSP。 |
| `sensing/gray_calibration.h` | Phase 14 指定的 16 路黑/白实测集中填写点，生产值与测试夹具分离。 | 改用其他标定机制需完整兼容设计；可移入 C 缩小可见性但无必要。 |
| `sensing/imu_proc.c/.h` | 活动姿态、零偏、积分、Y 高通历史唯一 owner，并向旧算法显式借出同一状态。 | 活动 IMU 取消前不可删；不应与 legacy 合并。 |
| `sensing/legacy_imu.c/.h` | 隔离两条旧加速度算法，无第二套状态，并有 G3 对照验证。 | 用户决定放弃旧加速度路线后，与两个公共外壳、状态和测试整组删除。 |
| `sensing/filter.c/.h` | 电池、编码器、按键低通和 IMU 高通共用的显式状态纯算法。 | 所有消费者换用另一已验证实现后才可替换，不能并入某一个传感器。 |
| `sensing/battery.c/.h` | 电池原始/滤波值与低通历史唯一 owner；低压、HMI、日志共用。 | 取消低压保护、显示和日志后才可删；不能并入 safety 反向依赖。 |

| 函数（定义行） | 必要性 | 必须存在的原因 / 修改方案 |
|---|---|---|
| `Track_GetState` (`track_sensor.c:28`) | 运行必需 | 调度、控制、日志、HMI 的只读电感状态入口。 |
| `Track_CalculateError` (`:42`) | 运行必需 | 电磁 FSM 使用的纯误差算法；不能与旧 UpdateError 合并。 |
| `Track_PublishObservation` (`:59`) | 运行必需 | 发布 FSM 已算的权重/误差/对称度，避免重算及顺序变化。 |
| `Track_SetWeights` (`:74`) | 运行必需 | profile 部分应用及旧预览需要设置四项权重。 |
| `Track_UpdateError` (`:88`) | 运行必需 | 停车前台预览和旧环岛仍调用；不是纯休眠函数。 |
| `Track_UpdateSymmetry` (`:125`) | 测试辅助/可精简 | 产品无调用且与 UpdateError 重复。若不需独立调试，删 C/H 及 control_equivalence 单调和映射，保留 UpdateError 内计算。 |
| `Track_ReadAdc` (`:147`) | 运行必需 | 四路八读去一最大/最小后平均，起跑和控制均调用。 |
| `Track_InitAdc` (`:197`) | **可删除辅助** | 无实际调用，生命周期已直接 `BSP_ADC_Init`。删 C/H 与无用测试映射即可，不新增替代调用。 |
| `Gray_GetState` (`gray_sensor.c:22`) | 运行必需 | 起跑门禁、双保护、控制、日志读取完整灰度状态。 |
| `Gray_GetBlack` (`:32`) | 运行必需 | 日志头保存黑底标定以支持赛后重算。 |
| `Gray_GetWhite` (`:42`) | 运行必需 | 日志头保存白线标定，与黑底不可缺一组。 |
| `Gray_ValidateCalibration` (`:56`) | 运行必需 | 同时校验当前表和 Flash 历史会话标定，不能只校验静态表。 |
| `Gray_CheckCalibration` (`:81`) | 运行必需 | 更新有效标志并区分前台报告/ISR 静默，起跑与记录门禁依赖。 |
| `Gray_Normalize` (`:112`) | 运行必需 | 当前帧和 CSV 按历史标定重算共同使用，不可私有化后丢导出调用。 |
| `Gray_ComputeError` (`:145`) | 行为必需/边界可简化 | 质心与有效线算法必需；公开入口支持真实 C 边界测试，可私有化但需替代覆盖。 |
| `Gray_ResetSequence` (`:173`) | 运行必需 | 同步复位 BSP/App 帧序号，不能只清一侧。 |
| `Gray_Init` (`:185`) | 运行必需 | 上电按清状态→BSP→标定报告顺序初始化。 |
| `Gray_Update` (`:198`) | 运行必需 | 完整帧采集、归一化、质心及失败保留上帧，不能因未标定而删。 |
| `IMU_GetAngleDeltaX` (`imu_proc.c:47`) | 运行必需 | 电磁影子按拍累计 X 角增量。 |
| `IMU_GetState` (`:57`) | 运行必需 | 控制、预览、HMI 及旧算法读取姿态。 |
| `IMU_ZeroAngleX` (`:67`) | 运行必需 | 选中电磁执行及旧环岛清累计角；影子禁用不等于接口无用。 |
| `IMU_Init` (`:78`) | 运行必需 | 初始化 IMU 与高通实例；X 实例可单独评估，不影响本函数。 |
| `IMU_UpdateAccelVelocity` (`:91`) | 休眠保留 | 旧加速度积分公共外壳；放弃该路线时与 LegacyIMU 实现、字段、测试整组删，不应直接启用。 |
| `IMU_UpdateGyroAngle` (`:103`) | 运行必需 | TIM6 去偏、Y 高通和三轴梯形积分，发布本拍 X 增量。 |
| `IMU_CalibrateAccel` (`:130`) | 休眠保留 | 旧百次加速度校准外壳；与积分路线同组处置，不能只加初始化调用。 |
| `IMU_CalibrateGyro` (`:142`) | 运行必需 | 上电 TIM15 启动前校准三轴零偏并清角度。 |
| `IMU_ResetRunState` (`:169`) | 运行必需 | 起跑/模式切换清历史但保留零偏；裁休眠字段只删对应语句。 |
| `LegacyIMU_UpdateAccelVelocity` (`legacy_imu.c:21`) | 休眠保留 | 旧公式及字段错位是历史行为；随公共外壳整组删，不能单独删留空壳。 |
| `LegacyIMU_CalibrateAccel` (`:47`) | 休眠保留 | 旧加速度校准实现；与校准外壳同组处置。 |
| `Filter_InitLowpass` (`filter.c:17`) | 运行必需 | 电池、编码器、按键各自历史初始化。 |
| `Filter_UpdateLowpass` (`:31`) | 运行必需 | 三类消费者的共同一阶低通，保持浮点与截断边界。 |
| `Filter_InitHighpass` (`:47`) | 运行必需 | 活动 Y 轴初始化/复位仍必需，即使裁掉 X 实例。 |
| `Filter_UpdateHighpass` (`:64`) | 运行必需 | 活动 IMU Y 轴高通算法。 |
| `Battery_Init` (`battery.c:25`) | 运行必需 | 上电初始化低通；起跑有意不清历史。 |
| `Battery_Update` (`:35`) | 运行必需 | 前台/TIM7 ADC 窄化与滤波，安全策略留在 safety。 |
| `Battery_GetRaw` (`:46`) | 测试诊断/可精简 | 产品无调用，仅 G2 窄化断言。若不要原始诊断，删 C/H 并改测试只验滤波；进一步删 `s_raw` 需另改 owner 规约。 |
| `Battery_GetFiltered` (`:56`) | 运行必需 | 低压保护、日志、两种显示读取，不能用 raw 替代。 |

**额外非函数精简项：**`imu_proc.c:18` 的 `s_gyro_hpf_x` 只初始化/复位、从不更新。若确认不规划 X 高通，可只删该对象及 `IMU_Init`/`IMU_ResetRunState` 两处初始化，保留 Y 轴与通用高通 API。

**灰度现实风险：**`gray_calibration.h:12-49` 的生产黑白标定 32 项仍全为 `-1`，`Lifecycle_TryStart` 因而会拒绝两种模式起跑。这是安全门禁，不应通过填假值、跳过校验或删除灰度代码绕过；当前优先事项是实车标定和验收。

### 3.3 App 根与 `system/`：固定入口、生命周期、安全、调度（14 文件，51 函数）

| 文件 | 存在原因 | 修改/删除条件 |
|---|---|---|
| `app.c/.h` | Core/BSP 固定调用的业务门面；隔离 HAL 与 system，并统一停车门禁。 | 只有取消固定 App ABI 并同步 Core/BSP/测试才可改，不建议为了删单行转发而改。 |
| `app_config.h` | dt、延时、低压、日志、LCD、风扇和调参限值的真实配置源。 | 文件必须留；个别无使用宏见本节末尾。 |
| `app_types.h` | 停车、参数、调参、模式结果的业务共享类型，避免引入 SCB 类型。 | 类型布局影响接口/持久化，不能随清理擅改。 |
| `system/lifecycle.c/.h` | key/start/run、拍数、距离、看门狗唯一 owner；上电、起跑、切模式和 reset 总协调。 | 与 scheduler/safety 合并会破坏状态和临界区边界。 |
| `system/scheduler.c/.h` | 前台与 TIM6/TIM7/TIM15 四条实际工作流；固定多次停车检查与预算路径。 | 取消根转发层才可能合并 H；工作流本身不可删。 |
| `system/safety.c/.h` | 首因/pending/低压计数唯一 owner，分离 ISR 立即停车与前台阻塞收尾。 | 不可并入 scheduler 后减少重读或移动 Flash 操作。 |
| `system/parameter_application.c/.h` | 协调 bank、持久化镜像、活动 PID/电机和菜单；镜像旧值与活动值有意不同。 | 不能由 store getter 取代；跨 HMI/无线/调参/生命周期使用。 |
| `system/tuning_application.c/.h` | 上下文快照、锁内外分工、参数合入、即时发布和停车一次保存。 | 并回 control/tuning 会重新引入存储/通信反向依赖。 |

| 函数（定义行） | 必要性 | 必须存在的原因 / 修改方案 |
|---|---|---|
| `App_Init` (`app.c:18`) | 边界必需 | `main.c` 固定总装入口；单行不等于无用。 |
| `App_Loop` (`:27`) | 边界必需 | `main.c` 主循环唯一 App 入口。 |
| `App_ControlISR` (`:36`) | 边界必需 | BSP TIM6 回调只依赖 App 门面。 |
| `App_TaskISR` (`:45`) | 边界必需 | BSP TIM7 的 5 ms 固定分派边界。 |
| `App_SampleISR` (`:54`) | 边界必需 | BSP 采样器与 scheduler 解耦的 1 ms 记录入口。 |
| `App_RequestStop` (`:64`) | 运行必需 | 调度、日志和旧控制统一首因停车入口。 |
| `App_IsStopped` (`:74`) | 运行必需 | 起跑、模式、导出、调参门禁；每次原位置重读有并发意义。 |
| `Lifecycle_Init` (`lifecycle.c:55`) | 运行必需 | 固定完整上电顺序：BSP/Flash/参数/传感器/校准/链路/调参。 |
| `Lifecycle_TryStart` (`:114`) | 运行必需 | 双传感器、标定、参数、日志门禁及定时链启动事务。 |
| `Lifecycle_ResetRunState` (`:197`) | 行为必需/边界可并 | 起跑跨模块精确 reset 顺序；可机械内联但无删除职责的收益。 |
| `Lifecycle_IsStopped` (`:214`) | 运行必需 | start/key/pending 短路与 volatile 重读不可缓存。 |
| `Lifecycle_IsStarted` (`:224`) | 运行必需 | 调度、命令和调参读取启动状态。 |
| `Lifecycle_IsKeyActive` (`:234`) | 运行必需 | 前台三个独立分支分别重读，不能合并成一次。 |
| `Lifecycle_IsRunning` (`:244`) | 测试诊断 | 生产无调用，宿主验证等待期；不是当前运行必需，若删需同步 `s_run_flag`、测试与旧状态设计。 |
| `Lifecycle_GetControlTicks` (`:254`) | 运行必需 | 调度、tracking、调参和旧元素共用拍数。 |
| `Lifecycle_GetDistanceMm` (`:264`) | 运行必需 | tracking、日志、dashboard、旧环岛读取测距。 |
| `Lifecycle_IsWatchdogStarted` (`:274`) | 运行必需 | 停车前台和收尾前后决定是否喂狗。 |
| `Lifecycle_AdvanceControlTick` (`:283`) | 运行必需 | 在采集前原位置推进拍数，不可挪时。 |
| `Lifecycle_SetDistanceMm` (`:293`) | 运行必需 | 保持 DL1B 更新→读取→写入顺序。 |
| `Lifecycle_MarkRunning` (`:302`) | 运行/休眠兼容 | 活动等待期结束及旧控制包装发布 run；删除涉及旧行为与诊断。 |
| `Lifecycle_ClearRunFlags` (`:312`) | 安全必需 | 在既有 PRIMASK 临界区清 start/run/key，不自行改锁。 |
| `Lifecycle_TestSetRunState` (`:326`) | 测试必需 | 仅 `SMARTCAR_HOST_TEST` 编译，用于布置私有运行状态，不进生产。 |
| `Lifecycle_TrySetMode` (`:343`) | 运行必需 | 参数校验、保存门禁、成功后跨模块 reset；同模式也不能早退。 |
| `Safety_RequestStop` (`safety.c:38`) | 安全必需 | 保护映射、首因锁存和立即停车，处理 NONE/重复/非法原因。 |
| `Safety_FinalizeStop` (`:111`) | 安全必需 | 前台尾包、调参合入、报告和清 pending；三个重读点均须保留。 |
| `Safety_IsStopPending` (`:149`) | 安全必需 | lifecycle/scheduler 的 volatile 单次读取接口。 |
| `Safety_GetStopReason` (`:159`) | 测试诊断 | 生产无外部调用，但首因诊断与 G2/dual-mode 断言使用；可删性取决于是否放弃该可观测性。 |
| `Safety_Reset` (`:169`) | 安全必需 | 起跑前清首因/pending/低压计数，不能提前在收尾清首因。 |
| `Safety_CheckLowVoltage` (`:183`) | 安全必需 | 保持短路读次数、严格阈值及 `>200` 计数语义。 |
| `Scheduler_Loop` (`scheduler.c:41`) | 运行必需 | 前台采样、HMI、保存、起跑与多次停车收尾编排。 |
| `Scheduler_ControlISR` (`:123`) | 运行必需 | 2 ms 双采样、保护、观察、执行、调参、记录及 900 μs 检查。 |
| `Scheduler_TaskISR` (`:218`) | 运行必需 | 5 ms 电池/低压/喂狗/灯，低压停车后仍喂狗。 |
| `Scheduler_SampleISR` (`:241`) | 运行必需 | 1 ms RAM 日志生产，禁止 ISR 写 Flash。 |
| `Param_ApplyCurrentDirection` (`parameter_application.c:20`) | 运行必需 | 当前 bank 应用到 PID/电机，固定先增益后基速。 |
| `Param_SetDirection` (`:39`) | 运行必需 | 写 bank、标脏、按当前目标应用；菜单/无线/调参共用。 |
| `Param_SelectGear` (`:61`) | 运行必需 | 切档并应用方向和风扇；不标脏、不复位 PI。 |
| `Param_Save` (`:78`) | 运行必需 | BACK 先保存及松键保存的立即保存入口。 |
| `Param_ServiceSave` (`:89`) | 运行必需 | 3000 ms 延迟保存；无脏不读时钟。 |
| `Param_FlushPending` (`:101`) | **可删除包装** | 无产品/测试调用，起跑已直接用 `Param_TryFlushPending`。删 C 与 `parameter_application.h` 声明并更新 Phase08/17 导航。 |
| `Param_Load` (`:112`) | 运行必需 | 即使读取失败也应用 RAM 默认/迁移值，并恢复活动 PI、低档和菜单。 |
| `Param_SetFan` (`:137`) | 运行必需 | 镜像、当前档活动值与 dirty 协调；store 钳位后读回有意保留。 |
| `Param_SetSpeedGains` (`:167`) | 运行必需 | 同步镜像和活动速度 PI；不能统一 Wireless 只改活动值的旧路径。 |
| `Param_TryFlushPending` (`:182`) | 运行必需 | 可用/脏门禁及真实保存时才捕获活动 PID/菜单快照。 |
| `TuningApplication_IsOrdinary` (`tuning_application.c:21`) | 行为必需/边界可并 | 电磁普通赛道决定覆盖许可，不能只看 mode。 |
| `TuningApplication_CaptureContext` (`:33`) | 并发必需 | 前台锁内与 TIM6 同拍捕获 stopped/mode/gear/保护/序号等。 |
| `TuningApplication_Merge` (`:54`) | 行为必需/边界可并 | Set 与停车收尾共同按方向→速度→两风扇合入并标脏。 |
| `TuningApplication_Synchronize` (`:70`) | 时序必需 | 即时 revision/event 发布；活动与已批准保留的旧包装均调用。 |
| `TuningApplication_Set` (`:92`) | 运行必需 | 锁外校验、锁内 Prepare、停车合入；末尾 revision 必须锁外执行。 |
| `TuningApplication_Switch` (`:115`) | 运行必需 | 覆盖启停、基线恢复及锁外同步语义。 |
| `TuningApplication_FinalizeStop` (`:129`) | 运行必需 | 停车只合入未保存成功值，不清 dirty、不写 Flash。 |
| `TuningApplication_ServiceSave` (`:142`) | 运行必需 | 停车脏成功值只尝试保存一次，失败也不能每轮重写 Flash。 |

**根配置/类型的可精简项：**

- `app_config.h:16` 的 `APP_CTRL_PERIOD_MS`、`:17` 的 `APP_TASK_PERIOD_MS`、`:41` 的 `APP_DEBUG_UART_BAUD` 没有生产 C 使用，只作旧测试名称映射/规约说明；`IMU_ACC_RAW_TO_G` (`:34`) 没有可执行代码使用。若确认不需要这些“文档式配置”，可删宏并同步 Phase01/17 与 `control_equivalence.py` 映射；**不能因此改 CubeMX 定时器/USART，也不能用 ACC 宏替换旧 IMU 公式中的 GYRO 宏**。
- `app_types.h` 的 `tracking_mode_t` 别名未使用，但 `TRACKING_MODE_EM/OPTICAL` 枚举值大量使用。可改匿名 enum 保留常量；不要把 `tracking_state_t.mode` 改为 enum，避免布局变化。
- `lifecycle.c:43` 的 `s_flash_available` 只在一次初始化内写后读，可改为局部变量；需同步私有对象/BSS 证据。
- `lifecycle.c:37` 的 `s_run_flag` 虽无生产读取方，但仍有测试诊断和旧入口写入语义，不能像普通死变量直接删。

### 3.4 `control/`：双模式、控制器、调参与旧算法（30 文件，120 函数）

| 文件（每行均为 `.c/.h` 一对） | 存在原因与删除/合并边界 |
|---|---|
| `tracking.c/.h` | 双模式同帧观察、调参选择、唯一执行口、保护累计和完整拍序号；状态/保护位/监视访问跨日志、HMI、通信使用，不能合并两执行输出字段。 |
| `control_profile.c/.h` | 活动 FSM 与旧环岛共用七主态/四环岛配置及距离门限；部分 fields 未置位组必须沿用旧值。 |
| `direction_controller.c/.h` | 电磁、光电、旧预览共用唯一非线性 PD，历史由调用方隔离。 |
| `angle_controller.c/.h` | 角度前拍 owner 和纯计算边界；虽活动仅 reset，仍支撑旧角度算法，不能与电机包装混合。 |
| `speed_controller.c/.h` | 活动左右位置 PI 与独立历史唯一 owner，已消除公式重复，不应并回 motor。 |
| `pid.c/.h` | 增益/可见输出统一 façade；参数、HMI、日志、调参与算法之间必需。 |
| `motor.c/.h` | 双轮目标/反馈/里程/风扇/滤波/PWM/紧急停车唯一 owner；只有 `Motor_Init` 是明确候选。 |
| `electromagnetic_fsm.c/.h` | 活动电磁影子状态机、独立距离/角度/方向历史；不能吸收直接写硬件的 legacy。 |
| `optical_fsm.c/.h` | 光电四态、失线锁存、独立历史与执行意图，是 Phase 14 核心。 |
| `legacy_control.c/.h` | 两入口当前均休眠，保存旧整段控制和角度电机副作用；用户决定放弃后才整组删。 |
| `legacy_element.c/.h` | 全部休眠，作为 Phase 09 元素设计素材；不得整批接入或与活动 FSM 混合。 |
| `legacy_pid.c/.h` | **不可整删**：预览和方向历史 reset 活动；仅两个增量 PI 休眠。 |
| `legacy_roundabout.c/.h` | **不可整删**：每次起跑仍调用旧状态 reset；其余旧算法逐项决策。H 中重复宏可去重。 |
| `tuning.c/.h` | 完整拍事务唯一 owner；baseline/pending/last_success/effective/final、单事件槽不可合并。 |
| `tuning_values.c/.h` | 应用与协议共享纯数值校验，避免 codec 拖入事务/BSP，不能复制规则。 |

| 函数（定义行） | 必要性 | 必须存在的原因 / 修改方案 |
|---|---|---|
| `Tracking_GetState` (`tracking.c:37`) | 运行必需 | 参数、调参、安全、命令、日志、HMI 的唯一只读视图。 |
| `Tracking_SetGear` (`:50`) | 运行必需 | 只写档位，参数应用由 Param 协调，顺序不可改。 |
| `Tracking_CompleteControl` (`:63`) | 运行必需 | 错误路径也推进完整拍序号；不能挪进 Execute。 |
| `Tracking_Stopped` (`:74`) | 安全必需 | 停车光电观察、清监视/拍输出，刻意不推进电磁。 |
| `Tracking_AddProtection` (`:101`) | 安全必需 | 原临界区累计全部保护位，不能用普通赋值。 |
| `Tracking_Observe` (`:116`) | 运行必需 | 同帧两 FSM、调参与方向各推进一次；重复调用会多推历史。 |
| `Tracking_Execute` (`:191`) | 运行必需 | 仅选中模式提交速度/转向/复位意图，保留 steering 与 output 差异。 |
| `Tracking_AccumulateMotion` (`:235`) | 运行必需 | 左右独立累计，先平均会改变整数截断。 |
| `Control_ResetRunState` (`:247`) | 运行必需 | 上电/起跑/切模式清双 FSM 等；不得吞并旧环岛 reset。 |
| `Tracking_GetTrackOutput` (`:264`) | 运行必需 | 执行、dashboard 和旧环岛读取方向监视。 |
| `Tracking_SetTrackOutput` (`:274`) | 运行必需 | 活动停车预览仍写；只写监视，不写完整拍快照。 |
| `Tracking_GetKernelState` (`:283`) | 运行必需 | dashboard 活动读取主态。 |
| `Tracking_SetKernelState` (`:293`) | 运行/休眠兼容 | 活动观察/reset 与旧元素/环岛共同发布主态。 |
| `Tracking_GetCaskFlag` (`:302`) | 休眠保留 | 仅旧右环读取；随旧环岛整组处置。 |
| `Tracking_SetCaskFlag` (`:312`) | 运行/休眠兼容 | 活动 reset 使用，不能与 getter 整对删。 |
| `Tracking_GetRoundaboutState` (`:321`) | 休眠保留 | 仅旧环岛读取，活动 FSM 用私有状态；随旧实现决策。 |
| `Tracking_SetRoundaboutState` (`:331`) | 运行/休眠兼容 | 活动观察/reset 与旧环岛各阶段均写。 |
| `Tracking_SetMode` (`:341`) | 边界必需 | Lifecycle 保存成功后单次写 mode；门禁不应搬回 setter。 |
| `Control_ApplyProfile` (`control_profile.c:83`) | 运行必需 | 按 fields 部分写共享控制，未置位组沿用，不可整结构覆盖。 |
| `Control_GetKernelProfile` (`:111`) | 运行必需 | 七主态唯一配置源及越界回 TRACKING。 |
| `Control_MergeProfile` (`:124`) | 运行必需 | 配置先合入电磁私有上下文而非直接写硬件。 |
| `Roundabout_GetProfile` (`:192`) | 运行必需 | 活动与旧环岛共用四阶段表；名称旧不等于休眠。 |
| `Control_GetRoundaboutEnterDistance` (`:218`) | 运行必需 | 活动/旧判断共用 8000 门限。 |
| `Control_GetRoundaboutOutDistance` (`:227`) | 运行必需 | 共用 10000 门限；对象可改 const 但 getter 无删除收益。 |
| `PID_DirectionStep` (`direction_controller.c:21`) | 运行必需 | 三条路径共用唯一方向公式，窄化后限幅顺序不可改。 |
| `AngleController_Update` (`angle_controller.c:23`) | 休眠算法/边界必需 | 旧角度纯计算与历史 owner；随角度路线保留或整组处置。 |
| `AngleController_Reset` (`:39`) | 运行必需 | 起跑/切模式经 PID_ResetAll 清前拍，不能随休眠计算删除。 |
| `SpeedController_Update` (`speed_controller.c:32`) | 行为必需/边界可并 | 单轮 PI 核心消除左右公式重复且保持独立状态。 |
| `SpeedController_UpdateLeft` (`:91`) | 运行必需 | 左轮历史/死区适配。 |
| `SpeedController_UpdateRight` (`:107`) | 运行必需 | 右轮独立历史/死区适配。 |
| `SpeedController_Reset` (`:117`) | 安全必需 | 停车/起跑清左右积分与目标历史。 |
| `PID_GetState` (`pid.c:19`) | 运行必需 | 控制、保存、日志、菜单、调参、旧环岛共享只读增益/输出。 |
| `PID_SetGain` (`:31`) | 运行必需 | 参数/无线/调参统一单增益路由；未知枚举无操作。 |
| `PID_SetTrackGains` (`:80`) | 运行必需 | 档位与状态 profile 写四方向增益。 |
| `PID_SetAngleGains` (`:94`) | 运行必需 | 活动 profile 会写角度增益，输出休眠不等于参数可删。 |
| `PID_CalculateAngle` (`:109`) | 休眠算法/边界必需 | 纯角度 façade，与带 Motor 的 UpdateAngle 不可合并。 |
| `PID_UpdateSpeedLeft` (`:123`) | 运行必需 | 以共享增益更新活动左 PI。 |
| `PID_UpdateSpeedRight` (`:135`) | 运行必需 | 活动右 PI。 |
| `PID_ResetSpeed` (`:145`) | 安全必需 | 立即停车/全复位清活动 PI；不得新增清休眠增量历史。 |
| `PID_ResetAll` (`:158`) | 运行必需 | 起跑/切模式清三类历史；不能 memset 整状态而清增益。 |
| `Motor_GetState` (`motor.c:24`) | 运行必需 | 调度、参数、菜单、调参、tracking、旧环岛共享状态。 |
| `Motor_SetTargets` (`:36`) | 安全必需 | reset/emergency 安全清目标。 |
| `Motor_SetBaseSpeed` (`:48`) | 运行必需 | 参数/profile/光电执行设置基速。 |
| `Motor_SetFanDuty` (`:59`) | 运行必需 | 运行风扇参数应用。 |
| `Motor_SetFanDutyIdle` (`:70`) | 运行必需 | 空闲风扇参数和停车语义。 |
| `Motor_ResetDistance` (`:80`) | 运行必需 | 活动环岛复位意图、起跑及旧环岛使用。 |
| `Motor_DistributeSpeed` (`:94`) | 运行必需 | 活动非对称差速核心，也供旧角度/环岛。 |
| `Motor_UpdateControl` (`:115`) | 运行必需 | 每拍双 PI、方向与 PWM 执行。 |
| `Motor_ReadEncoder` (`:156`) | 运行必需 | 读清采样、低通和累计里程，顺序关系反馈有效性。 |
| `Motor_Init` (`:178`) | **可删除休眠包装** | 无产品调用，Lifecycle 已直接 BSP_PWM_Init。确认不保留旧入口后删 C/H；不要改接 Lifecycle，因为会新增方向脚写入。 |
| `Motor_InitEncoder` (`:191`) | 运行必需 | 初始化编码器和两低通，是 Phase 12 修复反馈恒零的关键。 |
| `Motor_ResetRunState` (`:204`) | 运行必需 | 起跑/切模式清目标、反馈、里程、滤波。 |
| `Motor_EmergencyStop` (`:220`) | 安全必需 | 清目标/PI 并立即 PWM=0，绝不能按薄包装精简。 |
| `ElectromagneticFSM_GetState` (`electromagnetic_fsm.c:31`) | 运行必需 | tracking、调参、查询、日志读取电磁观察。 |
| `ElectromagneticFSM_Reset` (`:42`) | 运行必需 | 起跑/切模式清上下文/历史并恢复权重。 |
| `ElectromagneticFSM_Accumulate` (`:56`) | 运行必需 | 左右独立累计，与 Motor 滤波增量一致。 |
| `ElectromagneticFSM_GetDistance` (`:68`) | 行为必需/边界可并 | 多分支读取均值里程；进环清零后必须立即读到零。 |
| `ElectromagneticFSM_ClearDistance` (`:79`) | 行为必需/边界可并 | 清私有里程并发共享复位意图，顺序不可改。 |
| `ElectromagneticFSM_ClearAngle` (`:90`) | 行为必需/边界可并 | 清影子角并发意图，不能直接清共享 IMU。 |
| `ElectromagneticFSM_Measure` (`:103`) | 行为必需/边界可并 | 多循迹分支共用对称度与误差测量。 |
| `ElectromagneticFSM_Select` (`:119`) | 运行必需 | 活动状态/策略选择主体。 |
| `ElectromagneticFSM_Calculate` (`:245`) | 运行必需 | 消费一次许可并推进方向，防止同拍二次历史。 |
| `ElectromagneticFSM_Update` (`:279`) | 测试兼容/可精简 | 产品无调用。可保留旧入口；若精简，先把 dual_mode 测试改为 Select→Calculate，再删 C/H，不减少覆盖。 |
| `OpticalFSM_GetState` (`optical_fsm.c:19`) | 运行必需 | 执行、查询、日志读取光电状态。 |
| `OpticalFSM_Reset` (`:30`) | 运行必需 | 起跑/切模式回 WAIT 并清历史，保留 base_speed。 |
| `OpticalFSM_Select` (`:44`) | 运行必需 | 四态、1000/1001、LOST 锁存和停车优先级。 |
| `OpticalFSM_Calculate` (`:81`) | 运行必需 | 最终调参值确定后一次方向计算，不能并回 Select。 |
| `OpticalFSM_Update` (`:100`) | 运行必需 | 与电磁包装不同，停车路径实际调用。 |
| `Control_UpdateLegacy` (`legacy_control.c:30`) | 休眠保留 | 旧整段控制无活动调用；已批准保留 Observe 后同步副作用。放弃时与 G4 legacy 测试整组删，不能只删保护/同步段。 |
| `PID_UpdateAngle` (`:80`) | 休眠保留 | 旧角度计算后取负并差速；删除须确认 Phase09 不复用，不能用纯 Calculate 替代。 |
| `Element_Judge` (`legacy_element.c:29`) | 休眠保留 | 旧元素汇总顺序及覆写；放弃旧汇总时连同内部调用边处置。 |
| `Element_JudgeStraight` (`:43`) | 休眠空壳 | 无运行必要性；用户确认后删定义/声明，无调用替换。 |
| `Element_JudgeCrossroads` (`:53`) | 休眠保留 | 旧十字进入，仅汇总调用；与活动判断状态目标不同。 |
| `Element_JudgeCrossroadsExit` (`:67`) | 休眠保留 | 旧十字退出，放弃旧实现时删除。 |
| `Element_JudgeTeeterboard` (`:81`) | 休眠保留 | Phase09 跷跷板候选；不可直接启用/改阈值。 |
| `Element_JudgeTeeterboardExit` (`:97`) | 休眠保留 | 两独立退出判断及截断现状待 Phase09 决策。 |
| `Element_JudgeCask` (`:117`) | 休眠保留 | Phase09 路障候选，进入/退出作为同一设计项。 |
| `Element_JudgeCaskExit` (`:130`) | 休眠保留 | 与路障进入判断同组保留或删除。 |
| `PID_UpdateTrack` (`legacy_pid.c:20`) | 运行必需 | 停车前台预览活动调用，不能因位于 legacy 文件删除。 |
| `PID_UpdateIncrementalLeft` (`:37`) | 休眠保留 | 左增量 PI 对照/未来算法；确认后删 C/H 与 G3 用例，不接活动 Motor。 |
| `PID_UpdateIncrementalRight` (`:83`) | 休眠保留 | 右增量 PI；可提取私有 helper 去重但独立历史和入口须留。 |
| `LegacyPID_ResetTrackHistory` (`:122`) | 运行必需 | 活动起跑/切模式清预览历史；不得扩展清增量历史。 |
| `Roundabout_AbsFloat` (`legacy_roundabout.c:39`) | 休眠保留 | 仅旧角度分支用，保留负零/NaN 语义；随调用者同组处置。 |
| `Roundabout_Update` (`:54`) | 休眠保留 | 旧四阶段 Motor/PID/距离副作用，不可与活动 FSM 合并。 |
| `Roundabout_JudgePreentryLeft` (`:113`) | 休眠保留 | 旧预环/DL1B 抑制，Phase09 设计素材。 |
| `Roundabout_JudgePreentryRight` (`:145`) | 休眠保留 | 旧备用预环条件，确认冗余后删除。 |
| `Roundabout_JudgePreentryExit` (`:162`) | 休眠保留 | 保留清输出/角度但不清里程的旧边界。 |
| `Roundabout_JudgeEntryLeft` (`:180`) | 休眠空壳 | 仅旧汇总调用；放弃左环旧壳后连同该调用删，不补实现。 |
| `Roundabout_JudgeEntryRight` (`:191`) | 休眠保留 | 旧右标志/cask 条件，随旧汇总链处置。 |
| `Roundabout_JudgeAhead` (`:211`) | 休眠保留 | 旧 Update 的 ≥8000 打角分支，同链保留/删除。 |
| `Roundabout_JudgeEntered` (`:225`) | 休眠保留 | 旧 Update 的 >13000 环内分支。 |
| `Roundabout_JudgeEnteredAgain` (`:240`) | 休眠保留 | 角度法残留；删除时连同 AbsFloat。 |
| `Roundabout_JudgePreExit` (`:253`) | 休眠空壳 | 用户确认后删定义/声明，无调用替换。 |
| `Roundabout_JudgeExit` (`:263`) | 休眠保留 | 旧 Update 的 >32000 出环分支。 |
| `Roundabout_JudgeExited` (`:278`) | 休眠保留 | 旧 >10000 完成出环且不清里程/角度。 |
| `Roundabout_Judge` (`:295`) | 休眠空壳 | 用户确认后删定义/声明，不接调度。 |
| `Roundabout_ResetRunState` (`:306`) | **运行必需** | 每次起跑清旧状态；不可并入通用 reset 或随旧算法整文件删除。 |
| `Tuning_Event` (`tuning.c:51`) | 行为必需/边界可并 | 统一单事件槽发布点，不能改队列或直接 SCB。 |
| `Tuning_ReadActive` (`:63`) | 运行必需 | 查询、基线、effective 初始化共用八项活动值读取。 |
| `Tuning_ApplyShared` (`:80`) | 行为必需/边界可并 | Stop/Select/Switch 共享 PI/风扇应用顺序且不清历史。 |
| `Tuning_Equal` (`:96`) | 行为必需/边界可并 | 八字段数值比较防止无效 revision，不能按结构体内存比较。 |
| `Tuning_RejectPending` (`:110`) | 行为必需/边界可并 | pending 转独立 final，承载拒绝与结果邮箱语义。 |
| `Tuning_GetControl` (`:127`) | 运行必需 | 两模式观察/执行读取本拍 effective/overridden。 |
| `Tuning_Stop` (`:138`) | 安全必需 | 停车取消并恢复 baseline，无 Flash。 |
| `Tuning_GetStatus` (`:159`) | 并发必需 | 临界区复制查询状态，不暴露可写指针。 |
| `Tuning_SelectControl` (`:177`) | 运行必需 | 每拍复核 pending、覆盖和共享量，完整拍核心。 |
| `Tuning_CompleteControl` (`:252`) | 运行必需 | 完整拍后才 APPLIED/恢复事件，不能提前报成功。 |
| `Tuning_ResetSession` (`:285`) | 运行必需 | 会话取消清 pending/staged/final，不清成功值/enabled。 |
| `Tuning_Init` (`:299`) | 运行必需 | 上电初始化，不能移到起跑而破坏停车预开启。 |
| `Tuning_PrepareSet` (`:319`) | 并发必需 | 前台锁内 SET 门禁/pending，停车合入不能提前。 |
| `Tuning_CommitParked` (`:370`) | 运行必需 | 停车 merge 后提交，独立 revision 回绕和事件时点。 |
| `Tuning_Switch` (`:394`) | 运行必需 | 覆盖许可/baseline 及目标/元素/停车门禁，不并入 SET。 |
| `Tuning_GetRevision` (`:445`) | 并发必需 | 锁外最终版本读取，保留完整拍先提交的竞态语义。 |
| `Tuning_TakeResult` (`:457`) | 并发必需 | 复制 final 后只清 id，请求槽释放/ACK 依赖。 |
| `Tuning_CopyUnsaved` (`:482`) | 运行必需 | 停车 merge 与一次保存复制待保存成功值。 |
| `Tuning_CompleteSave` (`:499`) | 运行必需 | 成功/失败均只尝试一次的保存结果及事件。 |
| `Tuning_CopyEventValues` (`:514`) | 运行必需 | 保存事件取 last_success，其余取 effective，不可统一。 |
| `Tuning_TakeRevision` (`:527`) | 时序必需 | 取并清即时版本位，活动与旧包装同步依赖。 |
| `Tuning_TakeEvent` (`:544`) | 时序必需 | 取并清单事件槽，不是多余第二队列。 |
| `Tuning_ValidateValues` (`tuning_values.c:16`) | 边界必需 | 应用与 codec 共用八项 NaN/Inf/范围校验；失败不钳位。 |

**控制层可执行修改方案摘要：**

- 无行为去重：`legacy_roundabout.h:14-24` 的 11 个 `ROUNDABOUT_*` 宏与 `control_profile.h:36-46` 重复，可保后者为唯一来源，并显式包含正确头；不删除状态值、不改日志数值。
- 低优先级去重：左右增量 PI 可提取私有参数化 helper，但必须保留两个公共入口、左右各六份历史、`d=0`、限幅及 ResetAll 不清增量历史。
- 休眠清理必须按“旧整段控制 / 角度执行 / 左环 / 跷跷板 / 路障 / 旧右环 / 角度法 / 空壳 / 增量 PI”逐项确认；`PID_UpdateTrack`、`LegacyPID_ResetTrackHistory`、`Roundabout_ResetRunState` 明确排除在删除集合外。
- 不建议把旧判断接回活动 FSM、把 Roundabout reset 并入通用 reset、把 PID_ResetAll 改为 memset，或借清理修改阈值、调参覆写、方向公式和窄化顺序。

### 3.5 `services/`：参数、日志、协议与通信（30 文件，140 函数）

| 文件 | 存在原因与删除/合并边界 |
|---|---|
| `checksum.c/.h` | 帧和 raw 导出共用唯一可续算 CRC32；并入帧层会让日志反向依赖协议。 |
| `command_dispatch.c/.h` | SCB 与业务事务唯一适配，并向 system 提供无 SCB 类型入口；不可并入 link/tuning。 |
| `datalog.h` | 日志 v2 唯一 96B/256B 类型、布局、结果与公共 API 聚合，不能按“声明已分散”删除。 |
| `log_export.c` | raw 流式导出、租期、重复块、CRC 与 57 列 CSV，owner 与 Flash 队列不同。 |
| `log_export.h` | 仅再次包含 datalog.h，无独立声明；**可精简**，方案见本节末。 |
| `log_store.c/.h` | App 唯一日志 Flash 字节后端及写头回读校验，不能暴露 BSP 给 logger/export。 |
| `logger.c/.h` | 会话、1024 槽队列、落盘统计、首故障和有界收尾唯一 owner；H 限制内部 getter。 |
| `parameter_bank.c/.h` | 四方向 bank 唯一 owner，显式保存来源；不能用活动 PID 或可写全局替代。 |
| `parameter_image.h` | v1/v2/v3 52/56/88B 落盘布局与偏移断言，禁止重排/打包/删字段。 |
| `parameter_store.c/.h` | 镜像、CRC、默认/迁移、dirty 与实际 Flash 保存；store 不反向依赖控制/HMI。 |
| `run_record.c/.h` | 唯一 96B 快照、256B 当前头与头校验；不暴露可写对象。 |
| `scb_command_codec.c/.h` | 命令字段、八项值、状态名与 LOG_DATA 8B 头；可独立测试且无业务状态。 |
| `scb_frame.c/.h` | 固定 32B 头、消息身份、1024B 帧、CRC 和小端格式，外部协议核心。 |
| `scb_json.c/.h` | 无堆、有界 UTF-8/JSON 与固定 writer；共享 8 层栈不可重入是契约。 |
| `scb_link.c/.h` | 会话、ACK、重发、去重、结果缓存、事件/gap 与发送优先级唯一 owner。 |
| `telemetry.c/.h` | 独立 256 槽上传、run/revision 与 10 ms 组包，不能并入 Flash logger。 |
| `wireless.c/.h` | 停车主循环仍调用旧 USART1 协议；SCB 不替代其物理接口和 CSV/清日志能力。 |

| 函数（定义行） | 必要性 | 必须存在的原因 / 修改方案 |
|---|---|---|
| `Checksum_UpdateCrc32` (`checksum.c:16`) | 运行必需 | 协议和日志共用唯一 CRC 公式，且日志不依赖 SCB。 |
| `ParamBank_LoadBanks` (`parameter_bank.c:17`) | 运行必需 | 上电默认/迁移后统一从 v3 镜像装四 bank。 |
| `ParamBank_StoreBanks` (`:45`) | 行为必需/边界可并 | 显式 bank 写回镜像并清光电保留字段，持久化不取活动 PID。 |
| `Param_GetDirection` (`:78`) | 运行必需 | 控制、记录、命令、无线、菜单共用只读 bank。 |
| `ParamBank_SetDirection` (`:92`) | 运行必需 | 校验并复制 bank，非法无副作用；应用/标脏留给协调层。 |
| `Param_CalculateCrc16` (`parameter_store.c:28`) | 行为必需/边界可并 | 三版参数 CRC 共用固定位序/初值，不能随日志 CRC 改式。 |
| `Param_CalculateCrcV1` (`:50`) | 兼容必需 | v1 字节8起44B覆盖，迁移不可省。 |
| `Param_CalculateCrcV2` (`:62`) | 兼容必需 | v2 48B覆盖，不能用 v3 长度。 |
| `Param_CalculateCrcV3` (`:74`) | 运行必需 | 当前 88B 镜像读写校验。 |
| `Param_ClampFanDuty` (`:86`) | 行为必需/边界可并 | 加载与设置共用 0..10000 边界。 |
| `Param_SetOpticalDefaults` (`:97`) | 兼容必需 | 默认/旧迁移只补光电字段，不覆盖有效旧电磁参数。 |
| `Param_SetDefaults` (`:112`) | 运行必需 | 读错仍建立完整默认 RAM。 |
| `Param_WriteMirror` (`:133`) | 运行必需 | 唯一参数 Flash 写入路径，整理 bank/CRC 并保留失败副作用。 |
| `Param_GetFan` (`:155`) | 运行必需 | 镜像风扇供切档/日志/查询；不是活动 PWM getter。 |
| `Param_MarkDirty` (`:170`) | 运行必需 | 标脏并刷新 3 秒静默起点，与立即保存不同。 |
| `Param_GetSpeedGains` (`:182`) | 运行必需 | 镜像 PI 可在 flush 前落后于活动值，不能换 PID_GetState。 |
| `ParamStore_Load` (`:196`) | 运行必需 | Flash 读取、三版校验迁移、钳位和装 bank 核心。 |
| `ParamStore_SetFan` (`:324`) | 运行必需 | 只写镜像，不直接 PWM/标脏，保持 owner。 |
| `ParamStore_SetSpeedGains` (`:363`) | 运行必需 | 只写镜像 PI，活动 PID 由应用层另写。 |
| `ParamStore_TryFlushPending` (`:377`) | 运行必需 | 仅实际保存消费快照；失败保留 dirty。 |
| `ParamStore_GetPosition` (`:407`) | 运行必需 | 复制 page/arrow，不越权判断菜单合法性。 |
| `ParamStore_IsUsable` (`:417`) | 运行必需 | 不可用时避免无意义 PID/菜单快照和写入。 |
| `ParamStore_IsDirty` (`:426`) | 运行必需 | 保存门禁，避免无意义 Flash/快照。 |
| `ParamStore_RequestSave` (`:435`) | 运行必需 | 立即保存只置 dirty 不刷新静默时基。 |
| `ParamStore_IsSaveDue` (`:445`) | 运行必需 | dirty 且无符号差达到 3000 ms；无脏不读时钟。 |
| `Datalog_CalculateCrc16` (`run_record.c:102`) | 行为必需/边界可并 | 日志头字节8..255格式核心，构造/校验共用。 |
| `Datalog_SetProfile` (`:125`) | 行为必需/边界可并 | 两模式 bank 转头部 profile，保留字段清零。 |
| `RunRecord_ResetSnapshot` (`:138`) | 运行必需 | 初始化只清 96B 快照，不清头/上传。 |
| `RunRecord_CheckCalibration` (`:148`) | 运行必需 | 起跑建头前无诊断文本的标定门禁。 |
| `RunRecord_BuildHeader` (`:160`) | 运行必需 | 固定日志 v2 字段、顺序和 CRC。 |
| `RunRecord_GetHeader` (`:202`) | 运行必需 | 借出唯一内存头，避免扫描 Flash/第二份头。 |
| `Datalog_PublishControl` (`:213`) | 并发必需 | TIM6 逐字段发布完整拍，结尾 DMB 不可省。 |
| `RunRecord_CaptureSample` (`:248`) | 并发必需 | 完整拍加 1 ms 采样，上传先于 Flash 槽发布。 |
| `RunRecord_IsHeaderValid` (`:271`) | 运行必需 | CSV 回读头短路、CRC 与标定校验。 |
| `Logger_LatchFault` (`logger.c:37`) | 运行必需 | 唯一首故障锁存，READ 与其他故障行为不同。 |
| `Datalog_RequestFaultStop` (`:61`) | 安全必需 | 固定故障→保护→停车顺序。 |
| `Datalog_SetStopSummary` (`:75`) | 安全必需 | 首停车原因与保护累计的短临界区接口。 |
| `Datalog_GetStatus` (`:93`) | 运行必需 | 安全/导出读取并更新 pending，不是纯 getter。 |
| `Datalog_IsActive` (`:104`) | 运行必需 | 导出区分已停止与待收尾。 |
| `Datalog_GetCount` (`:114`) | 运行必需 | 导出/查询只计成功落盘前缀，不含 pending。 |
| `Datalog_Init` (`:125`) | 运行必需 | 清队列/状态/快照/有效位，不能随意并入 Start。 |
| `Datalog_Start` (`:142`) | 运行必需 | 建会话、擦除、写头校验，是起跑门禁。 |
| `Datalog_Clear` (`:189`) | 运行必需 | 旧串口清日志及导出互斥、故障/RAM摘要语义。 |
| `Datalog_GetMeta` (`:207`) | 运行必需 | 临界区复制缓存头/written，查询不扫描 Flash。 |
| `Datalog_Push` (`:230`) | 并发必需 | TIM15 门禁、采样、RAM 入队及故障停车。 |
| `Datalog_Service` (`:274`) | 运行必需 | 前台最多八条写 Flash，成功才释放，失败不重试。 |
| `Datalog_Flush` (`:317`) | 兼容保留 | 零活动调用但规约明确保留单次服务包装；若撤销公共 API，先确认外部用户并改 datalog.h/规约。 |
| `Datalog_Stop` (`:328`) | 安全必需 | 停车/CSV 共用的最多 128 批有界尾包。 |
| `Logger_GetStopReason` (`:344`) | 运行必需 | 快照只读首因，避免 GetStatus 的 pending 副作用。 |
| `Logger_HasHeader` (`:354`) | 运行必需 | 头有效位与头字节分 owner，导出开始/CSV依赖。 |
| `Datalog_CancelExport` (`log_export.c:30`) | 运行必需 | 超时/读错/END/会话重置释放互斥，不清日志或调参。 |
| `Datalog_ServiceExport` (`:41`) | 运行必需 | 10 秒租期与回绕，重复块不续租。 |
| `Datalog_IsExporting` (`:55`) | 运行必需 | 起跑/清日志/Start 共用互斥，先服务租期并非纯 getter。 |
| `Datalog_GetExportStatus` (`:67`) | 公共契约 | 仅测试直接调用，但验证重复块/CRC/身份，是规定查询接口。 |
| `Datalog_BeginExport` (`:81`) | 运行必需 | 停车冻结 written 前缀、单窗口、非零 ID 和失败输出。 |
| `Datalog_ReadExport` (`:119`) | 运行必需 | raw 顺序读/最近块借用；dispatch 同调用内复制，日志不含 wire 头。 |
| `Datalog_EndExport` (`:171`) | 运行必需 | 完整交付才给 total/CRC 并释放，提前 END 回 BUSY。 |
| `Datalog_Dump` (`:194`) | 运行必需 | 旧串口阻塞 57 列 CSV，不能被 SCB 导出替代。 |
| `LogStore_Erase` (`log_store.c:19`) | 边界必需 | 唯一擦除入口，薄转发隔离 BSP。 |
| `LogStore_WriteHeader` (`:32`) | 运行必需 | 写头、回读、比较及静态 256B 缓冲短路顺序。 |
| `LogStore_Write` (`:51`) | 运行必需 | 字节批次写入，不新增擦除/重试/喂狗。 |
| `LogStore_Read` (`:64`) | 运行必需 | raw/CSV 共用读取，保留底层失败部分副作用。 |
| `SCB_ReadU16` (`scb_frame.c:20`) | 运行必需 | 小端/未对齐安全读取，帧与 LOG_DATA 使用。 |
| `SCB_ReadU32` (`:31`) | 运行必需 | 帧、链路、dispatch 和测试共用 u32 读取。 |
| `SCB_WriteU16` (`:43`) | 运行必需 | 帧与 LOG_DATA 长度/保留字段兼容。 |
| `SCB_WriteU32` (`:55`) | 运行必需 | 帧、日志和上传共用。 |
| `SCB_CalculateCrc` (`:71`) | 公共契约 | 保留原公共名薄包装，实际公式只在 checksum。 |
| `SCB_BuildFrame` (`:82`) | 行为必需/边界可并 | 编解码共用类型、可靠位、身份和负载完整校验。 |
| `SCB_EncodeFrame` (`:140`) | 运行必需 | 外部 1024B 字节格式与清零填充核心。 |
| `SCB_DecodeFrame` (`:176`) | 运行必需 | 失败不发布 frame、不执行业务。 |
| `SCB_StatusName` (`scb_command_codec.c:21`) | 运行必需 | 固定 10 种状态文本与未知回退，是 wire 契约。 |
| `SCB_JsonValues` (`:38`) | 运行必需 | 固定八项顺序和数值格式，不可结构体强转。 |
| `SCB_ParseCommand` (`:70`) | 运行必需 | 完整 JSON→业务快照，区分语法/业务失败，避免部分执行。 |
| `SCB_ValidateValues` (`:209`) | 公共契约 | 仅测试直接调用但覆盖真实映射；不应删除或额外插入重复运行校验。 |
| `SCB_EncodeLogData` (`:246`) | 运行必需 | 8B wire 头+raw 数据复制及 960B 极值/guard。 |
| `SCB_ValidateUtf8` (`scb_json.c:25`) | 行为必需/边界可并 | 最短 UTF-8、代理、码点、NUL 校验，非法命令不得执行。 |
| `SCB_SkipSpace` (`:84`) | 行为必需/边界可并 | 只接受四种 JSON 空白。 |
| `SCB_ReadHex` (`:103`) | 行为必需/边界可并 | 四位 Unicode 转义解析。 |
| `SCB_AppendUtf8` (`:143`) | 行为必需/边界可并 | 有界字符串池与标量写入。 |
| `SCB_ParseString` (`:172`) | 行为必需/边界可并 | 字符串/转义/代理对及借用寿命。 |
| `SCB_ParseNumber` (`:273`) | 行为必需/边界可并 | 63B 数字文法和无堆转换，不能换 strtod 改行为。 |
| `SCB_ParseValue` (`:421`) | 行为必需/边界可并 | 显式栈非递归解析核心。 |
| `SCB_ParseJson` (`:474`) | 运行必需 | 所有 JSON 共用 8 层栈和重复键规则。 |
| `SCB_FindMember` (`:584`) | 运行必需 | 仅查直接对象成员，不能递归误取同名键。 |
| `SCB_GetString` (`:608`) | 运行必需 | 类型/越界失败与文本借用寿命明确。 |
| `SCB_GetNumber` (`:624`) | 运行必需 | 有限数字复制，失败不写输出。 |
| `SCB_JsonBegin` (`:642`) | 运行必需 | writer 容量和 failed 锁存起点。 |
| `SCB_JsonAppend` (`:657`) | 运行必需 | 溢出不发截断 JSON。 |
| `SCB_JsonU32` (`:675`) | 运行必需 | 无堆完整 uint32 十进制。 |
| `SCB_JsonI32` (`:694`) | 运行必需 | 含 INT32_MIN 的有符号输出。 |
| `SCB_JsonFloat` (`:711`) | 运行必需 | 九位科学记数、binary32 往返、负零/非有限兼容。 |
| `SCB_FindRequest` (`scb_link.c:65`) | 行为必需/边界可并 | 32 项请求缓存共同查找。 |
| `SCB_AllocateRequest` (`:84`) | 行为必需/边界可并 | 只分配空/可淘汰槽，pending/ready/在途不可淘汰。 |
| `SCB_SetHandlers` (`:111`) | 运行必需 | 注册私有业务回调，system 无需协议类型。 |
| `SCB_ResetSession` (`:123`) | 运行必需 | 初始化、重建、超时、run_id 回绕共用。 |
| `SCB_Init` (`:153`) | 运行必需 | 上电上传/BSP/会话初始化链。 |
| `SCB_QueueEvent` (`:167`) | 并发必需 | ISR 只复制事件描述并记录 gap，不格式化。 |
| `SCB_TakeEvent` (`:203`) | 行为必需/边界可并 | 前台先旧事件后 gap，保持顺序。 |
| `SCB_FormatEvent` (`:233`) | 行为必需/边界可并 | 前台固定事件 JSON。 |
| `SCB_QueueResult` (`:282`) | 运行必需 | 已预留槽写结果，RECEIVED 与最终结果共存。 |
| `SCB_GetRequestResult` (`:308`) | 运行必需 | 重连查询/去重核心，不暴露可写缓存。 |
| `SCB_AcceptRequest` (`:331`) | 行为必需/边界可并 | 去重、预留、分发与独立拒绝，副作用前容量门禁。 |
| `SCB_HandleAck` (`:393`) | 行为必需/边界可并 | ACK 只释放传输，不等于 APPLIED/SAVED。 |
| `SCB_ReceiveFrame` (`:412`) | 运行必需 | 坏帧不 ACK、不续租、不执行。 |
| `SCB_StartReliable` (`:485`) | 行为必需/边界可并 | 保存可靠帧副本，重发保持 seq/全部字节。 |
| `SCB_PrepareFrame` (`:511`) | 运行必需 | 发送优先级及 5/200/800 ms 规则核心。 |
| `SCB_Service` (`:621`) | 运行必需 | 非阻塞 RX/TX、READY 和启动计时边界。 |
| `SCB_GetStatus` (`:669`) | 兼容保留 | 零活动调用但 G5 明确保留公共诊断接口；正式撤销需确认外部用户并改规约/头/映射。 |
| `Telemetry_Init` (`telemetry.c:39`) | 运行必需 | 仅上电清上传，不在起跑重复清 revision。 |
| `Telemetry_ResetSession` (`:56`) | 运行必需 | 丢旧会话积压，只推进 tail/计 drop。 |
| `Telemetry_BeginRun` (`:70`) | 运行必需 | 实际起跑分配 run_id/重置索引，回绕先重建会话。 |
| `Telemetry_PushRecord` (`:89`) | 并发必需 | TIM15 复制 108B；满队只丢上传，不影响 Flash。 |
| `Telemetry_GetStatus` (`:114`) | 并发必需 | 查询和事件共享同一时点 run/revision 统计。 |
| `Telemetry_SetRevision` (`:130`) | 运行必需 | 只发布后续采样 revision，不重标已入队槽。 |
| `Telemetry_Service` (`:146`) | 运行必需 | 10 ms 最多 10 条的 24+96N 组包及缺口/版本边界。 |
| `CommandDispatch_MapResult` (`command_dispatch.c:41`) | 行为必需/边界可并 | 显式业务→SCB 枚举映射，未知回 IO_ERROR，不能强转。 |
| `CommandDispatch_ToWire` (`:76`) | 行为必需/边界可并 | 业务值→协议值，防止因布局相同共享类型。 |
| `CommandDispatch_ToBusiness` (`:94`) | 行为必需/边界可并 | SET 完整协议快照→业务值。 |
| `CommandDispatch_Begin` (`:114`) | 行为必需/边界可并 | 统一 op/status 前缀和错误重建。 |
| `CommandDispatch_Send` (`:133`) | 行为必需/边界可并 | 结束并入结果缓存；溢出改 IO_ERROR，不发截断结果。 |
| `CommandDispatch_Position` (`:154`) | 行为必需/边界可并 | 附已捕获 tick/sequence/revision，不重读后状态。 |
| `CommandDispatch_ReadBank` (`:174`) | 行为必需/边界可并 | GET_PARAMS 读取存储值，区别于 effective。 |
| `CommandDispatch_QueryParams` (`:193`) | 运行必需 | values/effective/limits，锁内只复制、锁外编码。 |
| `CommandDispatch_QueryStatus` (`:243`) | 运行必需 | 运行/调参/上传/日志只读内存，不扫 Flash。 |
| `CommandDispatch_QueryResult` (`:307`) | 运行必需 | 结果只展开一层，LOG_DATA 只回定位信息。 |
| `CommandDispatch_QueryLogMeta` (`:359`) | 运行必需 | 512 位 header hex，保留无效空串/小写hex/零Flash。 |
| `CommandDispatch_MapLogResult` (`:390`) | 行为必需/边界可并 | 六日志结果显式映射，核心不依赖 SCB 数值。 |
| `CommandDispatch_LogCommand` (`:419`) | 运行必需 | LOG_BEGIN/READ/END 适配，raw 同次调用编码入缓存。 |
| `CommandDispatch_HandleCommand` (`:476`) | 运行必需 | 经函数指针真实注册调用；无私下 C 调用不是死代码。 |
| `CommandDispatch_Set` (`:523`) | 运行必需 | SET 业务适配及 RECEIVED/APPLIED，APPLIED 才附值/位置。 |
| `CommandDispatch_Switch` (`:553`) | 运行必需 | TUNE_ENABLE/DISABLE 与业务开关隔离。 |
| `CommandDispatch_ResetSession` (`:567`) | 运行必需 | 真实会话回调清 pending/final/导出，不回滚有效值。 |
| `CommandDispatch_Init` (`:577`) | 运行必需 | 只注册两个回调，不重复初始化链路/事务。 |
| `CommandDispatch_InitLink` (`:586`) | 边界必需 | 薄转发保持 system 无 SCB 依赖。 |
| `CommandDispatch_ServiceLink` (`:595`) | 边界必需 | 独立调度调用点，不并入业务 Service。 |
| `CommandDispatch_Service` (`:604`) | 运行必需 | 固定导出租期→最终结果→停车保存顺序。 |
| `CommandDispatch_QueueTuningEvent` (`:634`) | 时序必需 | 在发生上下文固定 run/tick/sequence/values。 |
| `CommandDispatch_PublishRunStarted` (`:673`) | 运行必需 | 仅成功起跑且采样启动后发布。 |
| `CommandDispatch_PublishRunStopped` (`:692`) | 运行必需 | 仅首次停车发布并带首因，重复/NONE 不发布。 |
| `Wireless_Adjust` (`wireless.c:31`) | 运行必需 | 停车旧串口 d/c/vp/vi/xp/xd、CSV、清日志及延迟保存仍活动。 |

**`log_export.h` 的具体精简方案：**它是当前服务层唯一没有独立功能内容的文件。先更新 Phase 17 规约与迁移导航；将 `log_export.c` 改为直接包含 `services/datalog.h`；删除 `logger.c` 中多余的 `services/log_export.h` 包含并按直接依赖原则保留 `datalog.h`；删除该 H；同步最终符号/注释/资源清单。验证双 LCD GCC、Keil 清单及 `datalog_v2`、`spi_log_export`、`g5_boundaries`，不改变 Flash/CSV/SCB 字节。若不愿承担追溯更新成本，保留这个 10 行头完全合理，且无运行时开销。

## 4. 建议的修改清单与优先级

以下都是**建议而非已实施改动**。按照项目规则，先形成对应功能规约并独立提交，再改代码；休眠入口还需用户逐项明确选择“保留、接入或删除”。

### 4.1 第一组：不改变功能边界的辅助清理候选

| 对象 | 当前问题 | 具体修改范围 | 预期影响 |
|---|---|---|---|
| `Track_InitAdc` | 无调用，生命周期已直接初始化 ADC | 删 `track_sensor.c/.h` 接口及无用测试映射 | 无替代调用；减少一个误导性初始化入口 |
| `Param_FlushPending` | 无调用，起跑直接使用有返回值的 TryFlush | 删 `parameter_application.c/.h` 包装，更新 Phase08/17 导航 | 保存行为不变 |
| `log_export.h` | 只有 include 转发 | 两个使用方直接包含 datalog.h，删 H 并更新追溯 | 无运行时变化，仅少一个文件 |
| `legacy_roundabout.h` 重复宏 | 与 control_profile.h 重复定义同一组状态值 | 保留 control_profile.h 唯一来源，修正 include | 数值不变，避免日后两份定义漂移 |
| `s_flash_available` | 仅一次初始化内写后读，却占静态存储 | 改为 Lifecycle_Init 局部变量，更新对象/BSS证据 | 初始化顺序不变，减少不必要静态状态 |
| 四个无生产使用宏 | APP_CTRL_PERIOD_MS、APP_TASK_PERIOD_MS、IMU_ACC_RAW_TO_G、APP_DEBUG_UART_BAUD 不驱动实际硬件配置 | 确认后删宏并同步 Phase01/17 与旧测试映射；周期/波特率说明保留在真实配置或规约 | 消除“改 App 宏就会改硬件”的误解，不碰 CubeMX 配置 |

这组也不应混成一次大重构；建议按模块分批，以便双 LCD 构建和真实 C 回归定位差异。

### 4.2 第二组：先决定诊断/兼容价值，再精简

| 对象 | 两种可选方案 |
|---|---|
| `Track_UpdateSymmetry` | 保留作独立调试/等价测试；或删 C/H 与单独测试调用，继续由 Track_UpdateError 计算对称度。 |
| `Battery_GetRaw` | 保留原始窄化诊断；或删 getter 并把 G2 改为滤波结果断言。进一步删 `s_raw` 需另改 owner 规约。 |
| `Lifecycle_IsRunning` / `s_run_flag` | 保留等待期诊断和旧状态语义；或整组评估 getter、MarkRunning、状态字段、TestSet 签名及旧调用，不能只删 getter。 |
| `Safety_GetStopReason` | 保留首因诊断与测试；若放弃该公共诊断，再同步删除接口和断言，不影响 Safety 内部首因锁存。 |
| `ElectromagneticFSM_Update` | 保留旧兼容包装；或先把测试逐项改为 Select→Calculate，再删 C/H，活动 Tracking 不变。 |
| `Datalog_Flush` / `SCB_GetStatus` | 当前推荐保留明确公共兼容 API；只有确认无外部用户并批准兼容变更后才删除。 |
| `tracking_mode_t` 别名 | 保留作语义文档；或改匿名 enum 保留两个常量，不改变任何 mode 字段类型/布局。 |

### 4.3 第三组：休眠代码必须按功能整组决策

- **旧加速度路线：**`IMU_UpdateAccelVelocity`、`IMU_CalibrateAccel`、`legacy_imu.c/.h`、对应偏置/历史和 G3 测试作为一组。若接入，须先解决旧公式和字段错位；若删除，须确认不再需要方向环加速度前馈路线。
- **旧控制与角度执行：**`Control_UpdateLegacy`、`PID_UpdateAngle` 分别决策；同步 G4 legacy boundary、G3 对照和声明。不能把带 Motor 副作用的旧角度包装替换成纯计算。
- **旧增量速度 PI：**左右入口、独立历史及 G3 测试作为一组；删除后不得影响活动位置 PI，也不能通过新增 reset 清历史来“顺便修复”。
- **旧元素与环岛：**按左环、跷跷板、路障、旧右环、角度法和空壳逐项决策，同步内部调用与私有状态。`PID_UpdateTrack`、`LegacyPID_ResetTrackHistory`、`Roundabout_ResetRunState` 是活动例外，必须保留。
- **其他非函数休眠状态：**`s_gyro_hpf_x` 可在确认不规划 X 高通后只删对象和两处初始化；`pid_state_t.ki_x/kd_v` 涉及参数/协议语义，应随 Phase10/11 单独评估，不能因当前公式未读就删字段。

### 4.4 不建议以“精简”为目标的改法

- 删除根 App 单行转发、合并 system 三模块或取消 logger/run_record/log_store/log_export 边界。
- 把纯控制器、传感器或参数 store 改为反读车辆状态，或让 system/日志核心引入 SCB 类型。
- 因当前 LCD=0 删除外接菜单，或因光电未标定删除灰度活动算法。
- 把“仅测试调用”直接等同“无用”；现有真实 C 测试正是状态私有化和协议边界的验证手段。
- 借清理修改控制阈值、调参优先级、Flash/CSV/SCB 格式、临界区、ISR 顺序或休眠行为。

## 5. 复核结果与限制

- 已用最终符号索引逐项比对：本报告覆盖 **365/365 个 App 函数定义**，函数定义行号无不一致。
- 文件覆盖为根 4 + system 10 + sensing 13 + control 30 + services 30 + hmi 8 = **95 个 C/H**。
- `git diff --check -- Reports/app-file-function-necessity-review.md` 通过；Markdown 表格列数检查通过。
- `python SmartCar/tests/app_architecture_check.py` 通过：95 C/H、365 定义、五类目录、私有对象及两个 Keil 清单一致。
- 本次只新增本报告，未修改 `SmartCar/App` 或工程配置，未触碰已有 Hardware/build 在途改动；因此没有把历史构建/宿主结果冒充为本轮重新执行的功能验证。
- 报告结论基于当前源码与静态调用/规约证据。是否删除公共兼容接口、诊断能力或休眠功能，最终取决于用户对产品范围的决定，而不是单靠调用计数。
