# G4 参数、调参、HMI与运行事件复核

日期：2026-10-24。产品基线6125495；规约先行03e0053/42d11b0，运行适配9f0cc6c，
模式协调d60a54c。本文记录G4实际事实，不覆盖G5/G6，不标记Phase17或旧阶段实机验收完成。

## 职责与人工内容复核

| 模块 | 状态、顺序、失败留态及复核结论 | 行为证据 |
|---|---|---|
| parameter_bank/image | 唯一四bank；非空镜像显式输入输出，不保留地址；52/56/88B及56/72偏移断言；Store仅四组及两个光电保留字写零 | 原param子项、control_equivalence、g4_hmi |
| parameter_store | 唯一镜像/脏/可用性/dirty_tick；CRC仍44/48/80B；v1/v2迁移、默认、电磁零值、四风扇钳位和重写失败保持；不读PID或菜单 | 原param迁移及失败断言、g4_hmi镜像字节对照 |
| parameter_application | SetDirection复制后先MarkDirty再应用；SelectGear不标脏；无脏/不可用不预读；flush才按PI→page→arrow采集；Save不更新静默起点 | param、runtime_tuning、g4_hmi的vp/vi及2999/3000ms对照 |
| tuning_values | 唯一五float有序/三integer边界校验；NaN/Inf拒绝，不钳位；codec逐字段转换和显式结果映射 | 608项协议原断言、runtime_tuning |
| tuning | baseline/pending/last_success/effective/final分离；运行版本跳0，停车版本允许0；Complete才发布APPLIED；final直到前台take；事件单槽即时清空 | runtime_tuning、g4_transactions |
| tuning_application | 锁外validator/锁内上下文和门禁；停车merge顺序保留；SET锁外末尾重新读取revision；保存失败只尝试一次 | runtime_tuning、g4_hmi、逐调用点审查 |
| command_dispatch | 静态992/993B JSON工作区和meta只各一份；原JSON/日志路由保留；两SCB业务枚举显式映射；handler经Init注册而非公开协议头 | 原runtime/link/export、g4_boundaries |
| menu_keys | 路1后路2各三次、uint32求和/3；严格开区间用未滤波值；第二路覆盖后才更新显示滤波；首次扫描才初始化 | g4_hmi所有阈值/覆盖/滤波序列 |
| menu/menu_render | 页面与编辑、按键沿和绘制哨兵分别唯一owner；BACK保存旧调参页再导航；RST当拍早退，下拍再次清屏；松键保存后绘制 | g4_hmi逐条LCD调用及Flash字节对照、原param断言 |
| lifecycle/safety/scheduler | InitLink→Tuning_Init→注册；原两个前台分支各独立Service→ServiceLink；起停事件仍在原运行位置，全部字段赋值及读取顺序对历史一致 | g4_boundaries、原App中断注入/停车/重启断言 |
| Lifecycle_TrySetMode | 非法mode先拒绝，停车与保存门禁后才写mode；同mode也完整复位；Control→Motor→PID→IMU→参数应用→UART保持 | param新增非法/同mode真实入口，原跨mode/BUSY/保存失败，g4_boundaries顺序 |
| tracking/legacy_control/wireless | 调参改业务值；旧包装不新增调用；Wireless vp/vi只改活动PI，镜像flush前仍旧；d/c和xp/xd回显/清缓冲规则保持 | control_equivalence、runtime_tuning、g4_hmi |

C定义前逐项对照真实输入、状态读写、失败输出、锁和调度；getter/纯转发按实际类别允许省略details。
H仅摘要并指向真实C定义。已替换bank/store/application/menu/keys/render及本批调用方的通用模板，
修正SetDirection阶段注释的错误标脏顺序、store不应用控制、Menu_HandleKey已公开等过期事实。
新C/H无BOM、CRLF、末尾单换行、无Tab/行尾空白；普通行不超过100，原完整协议字符串例外保留。
多行compound literal大括号独占行；排版前后词法token核对相同，不能用此代替功能测试。
菜单来源块保留Sirius快照/平台/日期与未单列作者/独立许可证事实。

## 实际验证

- LCD=0/1 GCC均0警告0错误，text/data/bss分别91656/308/231688、94220/312/231704。
- control_equivalence、scb_protocol（608）、scb_link、datalog_v2、spi_wifi_state、
  spi_log_export、runtime_tuning、g2_system、g3_control全部通过，原断言未删减或放宽。
- 独立FSM、参数/key、完整App（生产未标定及宿主有效标定）子项全部通过。
  新增非法mode及同mode真实入口断言；PE链接器未裁剪的非测试初始化/起跑引用使用进入即失败的替身，
  模式入口及其reset、参数和控制使用真实实现，没有复制业务实现或忽略未定义符号。
- g4_hmi通过6125495/当前真实C对照的全部LCD调用和保存镜像；
  g4_transactions通过final所有权、独立成功值、事件时点、保护/会话取消及两条revision回绕；
  g4_boundaries逐token比较全部历史起停事件语句、Link薄转发与模式协调顺序。
- app_layout_check通过82个C/H、39个App C在Keil各一次及五类GCC目录。
  Phase16仅同步本批真实源清单及已注册dispatch回调owner，资源算法未修改。
- 完整dual_mode_tracking仍在读取原C:/Users/Li/Desktop/LQ_Track.c处失败；
  R07原始样本缺失不伪报通过，不删除SHA256断言；其之前真实gray子项已执行。
- 协议夹具按既有E盘host-temp运行，避免C盘输出路径被旧HARNESS首字符C误识别为命令。
- 最后双LCD的finally回写ELF曾遇Windows OSError22；随后从任务开始干净的HEAD二进制blob
  恢复并逐字节验证受跟踪ELF（805932B），app_config也已恢复，二者Git状态干净。
  hex/bin属于忽略产物，不纳入提交或作为受跟踪恢复证据。

日志统一MDK-GCC/build/phase17-g4-*；构建/宿主产物、临时源码和原始日志不自动纳入Git。

## 静态资源与并发

相对G3双LCD总BSS均减少48B，data不变，text分别增加916B/832B。
锁定ARM编译器-fstack-usage对比G3/G4本批范围：72/118个函数帧，最大均232B（静态有界）。
Tracking_Observe为144→176B，Scheduler_ControlISR为24→48B，
原Tuning_Service为88B，分离后CommandDispatch_Service为120B，TuningApplication_Set为72B。
这是单函数编译帧统计，不能直接当作链路峰值或实测高水位；最终全App调用链预算由G6复核。
原JSON缓冲992/993B保持静态且各一份；pending载体从76B协议命令变为36B业务请求，
baseline/last_success各32B、final52B和事务状态40B仍独立；新增即时发布位和事件槽不构成队列。
未新增堆分配、Flash/上传大副本、ISR JSON或ISR参数Flash访问。

单事件槽的前台生产者只在真正停车后合入/保存；此时timers已停止且pending前台收尾完成，
前台返回前不会起跑。TIM6生产后在同ISR立即同步；TIM15/TIM6同级互不抢占，
前台不能打断此区间。Select版本在Observe后、记录发布前同步；Complete事件在完整序号增加后
及记录发布前同步。Stop/Reset/Switch/Prepare不产生事件，不把final邮箱并入事件槽。

G4只验system自身与新header不直接暴露SCB；datalog.h现存传递SCB类型留G5/G6闭包验收。
AC5、700/900us、SPI波形、吞吐、Flash/实车与R07继续待验。
R08已由父任务按暂存源码复核并通过：本批基线2ee34017e5bc42d7f1bb824e7704c176a74456ab，35个变更C/H、144份当前/历史记录；绑定真实源码与依赖摘要，人工内容结论独立于该检查。

## G4-09 具体批准与闭包复验

曾发现休眠Control_UpdateLegacy的两个Observe路径未消费G4迁移后的版本发布位。
6125495原路径为Observe→SelectControl内部直接Telemetry_SetRevision；完整闭包规约已先行
提交2ee3401，但向两个Observe后添加Synchronize的产品补丁被自动审批连续两次拒绝，
后一次理由为未取得可信用户对该具体副作用的明确批准。两次拒绝的补丁都未落盘，没有绕过审批。
随后明确向用户询问这两处同步且入口仍不接入运行调度，用户于2026-10-24在选择弹窗回答
“允许，保持原有副作用”。具体批准事实已回填原G4-09；正常受审工具随后允许该补丁落盘。

g4_legacy_boundary.py编译并执行真实legacy_control.c，补丁前失败证据为成功路径
`TGPORE`（采集→保护→Observe→MarkRunning→Execute），契约预期`TGPOSRE`
（Observe后同步）。日志phase17-g4-legacy-boundary-blocked.log保留；不是通过证据。
补丁后成功路径TGPOSRE、故障路径TGPOSX及全部保护组合/首因优先级断言通过。
runtime_tuning、control_equivalence、g3_control、g4_transactions、g4_boundaries及独立
FSM/参数/App均已复验通过，日志phase17-g4-post-approval-*。
g3_control只给未调用的整段包装新增同步依赖提供进入即失败的替身，原角度/IMU断言保留；
完整App/调参仍链接真实tuning_application。该测试及补丁均未接入产品活动调用。
具体批准后的最终同版双LCD均0警告0错误，text/data/bss仍为91656/308/231688和
94220/312/231704；phase17-g4-post-approval-build.log确认finally逐字节恢复原配置及
ELF/HEX/BIN，受跟踪配置/ELF的Git状态干净。
锁定ARM单文件编译零警告，Control_UpdateLegacy栈帧16B、PID_UpdateAngle栈帧8B，
均与6125495相同；日志phase17-g4-post-approval-legacy-stack.log。
本闭包软件验证已关闭，仍无产品活动调用；R08提交前绑定、R07与硬件门禁状态分别保持。

## 新拆分模块逐函数内容清单

下表每项已核对C权威注释、H摘要定位（公开项）、参数方向/空值和实际返回；
复杂项另外核对阶段、状态与失败保留。字段/类型/layout断言按上表各唯一owner审查。
标签/EOF自动检查覆盖103个定义且无缺项；人工内容结论来自上述真实语句和历史对照，
不以脚本数量代替内容验收。

| 文件 | 定义 | 已核对的职责 | 验证依据 |
|---|---|---|---|
| App/control/tuning.c | Tuning_Event | 发布本次调用的即时业务事件。 | runtime_tuning / g4_transactions |
| App/control/tuning.c | Tuning_ReadActive | 固定复制活动共享参数和方向监视值。 | runtime_tuning / g4_transactions |
| App/control/tuning.c | Tuning_ApplyShared | 应用共享速度PI和风扇，不改变历史或方向策略。 | runtime_tuning / g4_transactions |
| App/control/tuning.c | Tuning_Equal | 比较八项实际值，避免填充字节影响版本判定。 | runtime_tuning / g4_transactions |
| App/control/tuning.c | Tuning_RejectPending | 终结未生效请求并保留独立最终邮箱。 | runtime_tuning / g4_transactions |
| App/control/tuning.c | Tuning_GetControl | 取得选中模式的本拍调试副本。 | runtime_tuning / g4_transactions |
| App/control/tuning.c | Tuning_Stop | 关闭调试并取消尚未生效的请求。 | runtime_tuning / g4_transactions |
| App/control/tuning.c | Tuning_GetStatus | 复制完整调参状态。 | runtime_tuning / g4_transactions |
| App/control/tuning.c | Tuning_SelectControl | 选择同拍最终参数并应用共享PI/风扇。 | runtime_tuning / g4_transactions |
| App/control/tuning.c | Tuning_CompleteControl | 执行完成后固定APPLIED及切换事件。 | runtime_tuning / g4_transactions |
| App/control/tuning.c | Tuning_ResetSession | 会话变化取消未应用请求而不回滚控制。 | runtime_tuning / g4_transactions |
| App/control/tuning.c | Tuning_Init | 上电初始化调参事务。 | runtime_tuning / g4_transactions |
| App/control/tuning.c | Tuning_PrepareSet | 检查SET门禁并暂存运行请求。 | runtime_tuning / g4_transactions |
| App/control/tuning.c | Tuning_CommitParked | 合入停车请求后的成功事务。 | runtime_tuning / g4_transactions |
| App/control/tuning.c | Tuning_Switch | 开启或关闭当前目标覆盖许可。 | runtime_tuning / g4_transactions |
| App/control/tuning.c | Tuning_GetRevision | 读取当前参数版本。 | runtime_tuning / g4_transactions |
| App/control/tuning.c | Tuning_TakeResult | 取走独立最终结果邮箱。 | runtime_tuning / g4_transactions |
| App/control/tuning.c | Tuning_CopyUnsaved | 复制尚待保存的成功值及目标。 | runtime_tuning / g4_transactions |
| App/control/tuning.c | Tuning_CompleteSave | 记录一次前台保存尝试结果。 | runtime_tuning / g4_transactions |
| App/control/tuning.c | Tuning_CopyEventValues | 在原事件发生上下文读取版本和值。 | runtime_tuning / g4_transactions |
| App/control/tuning.c | Tuning_TakeRevision | 取出当前调用待发布版本。 | runtime_tuning / g4_transactions |
| App/control/tuning.c | Tuning_TakeEvent | 取出当前调用产生的唯一事件。 | runtime_tuning / g4_transactions |
| App/control/tuning_values.c | Tuning_ValidateValues | 使用同源limits复核完整C参数，避免绕过解析入口的半组更新。 | scb_protocol608 / runtime_tuning |
| App/system/parameter_application.c | Param_ApplyCurrentDirection | 把当前模式/档位的 bank 分发给控制器。 | param / g4_hmi / control_equivalence |
| App/system/parameter_application.c | Param_SetDirection | 保存指定 bank，并按当前选择应用。 | param / g4_hmi / control_equivalence |
| App/system/parameter_application.c | Param_SelectGear | 切换活动档位并分发方向与共用风扇值。 | param / g4_hmi / control_equivalence |
| App/system/parameter_application.c | Param_Save | 立即标脏并尝试保存。 | param / g4_hmi / control_equivalence |
| App/system/parameter_application.c | Param_ServiceSave | 静默满 3000 ms 时尝试保存待写参数。 | param / g4_hmi / control_equivalence |
| App/system/parameter_application.c | Param_FlushPending | 保留立即尝试保存的无返回包装。 | param / g4_hmi / control_equivalence |
| App/system/parameter_application.c | Param_Load | 加载镜像并依次恢复活动PI、档位和菜单。 | param / g4_hmi / control_equivalence |
| App/system/parameter_application.c | Param_SetFan | 钳位并保存风扇参数，当前档同步活动值。 | param / g4_hmi / control_equivalence |
| App/system/parameter_application.c | Param_SetSpeedGains | 设置共享速度PI镜像及活动值，保持积分历史。 | param / g4_hmi / control_equivalence |
| App/system/parameter_application.c | Param_TryFlushPending | 将待写 bank 和共享参数同步到参数镜像。 | param / g4_hmi / control_equivalence |
| App/system/tuning_application.c | TuningApplication_IsOrdinary | 读取当前电磁观察是否处于普通赛道。 | runtime_tuning / g4_transactions |
| App/system/tuning_application.c | TuningApplication_CaptureContext | 捕获本次业务调用的瞬时上下文。 | runtime_tuning / g4_transactions |
| App/system/tuning_application.c | TuningApplication_Merge | 将成功调试值写入显式参数bank，不从活动元素推导。 | runtime_tuning / g4_transactions |
| App/system/tuning_application.c | TuningApplication_Synchronize | 即时适配当前业务调用的版本和事件。 | runtime_tuning / g4_transactions |
| App/system/tuning_application.c | TuningApplication_Set | 协调完整设置请求。 | runtime_tuning / g4_transactions |
| App/system/tuning_application.c | TuningApplication_Switch | 协调覆盖许可切换。 | runtime_tuning / g4_transactions |
| App/system/tuning_application.c | TuningApplication_FinalizeStop | 日志收尾后合入未保存成功值。 | runtime_tuning / g4_transactions |
| App/system/tuning_application.c | TuningApplication_ServiceSave | 停车前台尝试保存一次事务参数。 | runtime_tuning / g4_transactions |
| App/services/parameter_bank.c | ParamBank_LoadBanks | 按显式模式/档位装载四个方向 bank。 | param / g4_hmi / control_equivalence |
| App/services/parameter_bank.c | ParamBank_StoreBanks | 把显式四 bank 写入持久化镜像。 | param / g4_hmi / control_equivalence |
| App/services/parameter_bank.c | Param_GetDirection | 读取显式模式/档位的方向 bank。 | param / g4_hmi / control_equivalence |
| App/services/parameter_bank.c | ParamBank_SetDirection | 复制有效目标的一组方向参数。 | param / g4_hmi / control_equivalence |
| App/services/parameter_store.c | Param_CalculateCrc16 | 按原 CCITT 位序计算 CRC16。 | param / g4_hmi / control_equivalence |
| App/services/parameter_store.c | Param_CalculateCrcV1 | 计算 v1 镜像既定覆盖区 CRC。 | param / g4_hmi / control_equivalence |
| App/services/parameter_store.c | Param_CalculateCrcV2 | 计算 v2 镜像既定覆盖区 CRC。 | param / g4_hmi / control_equivalence |
| App/services/parameter_store.c | Param_CalculateCrcV3 | 计算 v3 镜像既定覆盖区 CRC。 | param / g4_hmi / control_equivalence |
| App/services/parameter_store.c | Param_ClampFanDuty | 按原三元式钳位风扇万分比。 | param / g4_hmi / control_equivalence |
| App/services/parameter_store.c | Param_SetOpticalDefaults | 设置原两档光电默认增益与基础速度。 | param / g4_hmi / control_equivalence |
| App/services/parameter_store.c | Param_SetDefaults | 清镜像并设置原公共默认值。 | param / g4_hmi / control_equivalence |
| App/services/parameter_store.c | Param_WriteMirror | 整理 bank 并计算 CRC 后写原 v3 镜像。 | param / g4_hmi / control_equivalence |
| App/services/parameter_store.c | Param_GetFan | 读取两档共用风扇参数。 | param / g4_hmi / control_equivalence |
| App/services/parameter_store.c | Param_MarkDirty | 标记参数待写并更新静默起点。 | param / g4_hmi / control_equivalence |
| App/services/parameter_store.c | Param_GetSpeedGains | 复制持久化镜像中的共享速度PI。 | param / g4_hmi / control_equivalence |
| App/services/parameter_store.c | ParamStore_Load | 读取 v1/v2/v3 镜像并迁移到原 88 B v3。 | param / g4_hmi / control_equivalence |
| App/services/parameter_store.c | ParamStore_SetFan | 只更新指定档位的运行或空闲风扇镜像。 | param / g4_hmi / control_equivalence |
| App/services/parameter_store.c | ParamStore_SetSpeedGains | 只更新共享速度PI镜像。 | param / g4_hmi / control_equivalence |
| App/services/parameter_store.c | ParamStore_TryFlushPending | 将待写 bank 和共享参数同步到参数镜像。 | param / g4_hmi / control_equivalence |
| App/services/parameter_store.c | ParamStore_GetPosition | 复制镜像的菜单位置原字节。 | param / g4_hmi / control_equivalence |
| App/services/parameter_store.c | ParamStore_IsUsable | 读取参数Flash可用状态。 | param / g4_hmi / control_equivalence |
| App/services/parameter_store.c | ParamStore_IsDirty | 读取参数待保存状态。 | param / g4_hmi / control_equivalence |
| App/services/parameter_store.c | ParamStore_RequestSave | 标记立即保存请求。 | param / g4_hmi / control_equivalence |
| App/services/parameter_store.c | ParamStore_IsSaveDue | 检查静默3000ms保存期限。 | param / g4_hmi / control_equivalence |
| App/services/command_dispatch.c | CommandDispatch_MapResult | 将业务状态显式映射为协议状态。 | runtime_tuning / export / boundaries |
| App/services/command_dispatch.c | CommandDispatch_ToWire | 逐字段复制业务与协议的八项参数。 | runtime_tuning / export / boundaries |
| App/services/command_dispatch.c | CommandDispatch_ToBusiness | 逐字段复制业务与协议的八项参数。 | runtime_tuning / export / boundaries |
| App/services/command_dispatch.c | CommandDispatch_Begin | 初始化前台结果的固定JSON前缀。 | runtime_tuning / export / boundaries |
| App/services/command_dispatch.c | CommandDispatch_Send | 结束JSON并更新已预留缓存。 | runtime_tuning / export / boundaries |
| App/services/command_dispatch.c | CommandDispatch_Position | 附加实际发生位置和版本。 | runtime_tuning / export / boundaries |
| App/services/command_dispatch.c | CommandDispatch_ReadBank | 取得目标bank的完整存储值。 | runtime_tuning / export / boundaries |
| App/services/command_dispatch.c | CommandDispatch_QueryParams | 输出完整bank、当前有效快照和同源限值。 | runtime_tuning / export / boundaries |
| App/services/command_dispatch.c | CommandDispatch_QueryStatus | 输出一致运行/调参/上传/日志摘要。 | runtime_tuning / export / boundaries |
| App/services/command_dispatch.c | CommandDispatch_QueryResult | 查询直接业务结果，避免查询自身产生递归包装。 | runtime_tuning / export / boundaries |
| App/services/command_dispatch.c | CommandDispatch_QueryLogMeta | 输出内存元信息和完整512位头文本。 | runtime_tuning / export / boundaries |
| App/services/command_dispatch.c | CommandDispatch_LogCommand | 在前台执行单窗口日志导出命令。 | runtime_tuning / export / boundaries |
| App/services/command_dispatch.c | CommandDispatch_HandleCommand | 分发已被SCB接管并预留最终结果的业务命令。 | runtime_tuning / export / boundaries |
| App/services/command_dispatch.c | CommandDispatch_Set | 校验并接收完整设置，运行态只发布一个待应用快照。 | runtime_tuning / export / boundaries |
| App/services/command_dispatch.c | CommandDispatch_Switch | 开启或关闭当前目标覆盖。 | runtime_tuning / export / boundaries |
| App/services/command_dispatch.c | CommandDispatch_ResetSession | 取消会话尚未完成请求和导出。 | runtime_tuning / export / boundaries |
| App/services/command_dispatch.c | CommandDispatch_Init | 注册业务请求及会话回调。 | runtime_tuning / export / boundaries |
| App/services/command_dispatch.c | CommandDispatch_InitLink | 初始化原可靠通信链路。 | runtime_tuning / export / boundaries |
| App/services/command_dispatch.c | CommandDispatch_ServiceLink | 执行原前台链路服务。 | runtime_tuning / export / boundaries |
| App/services/command_dispatch.c | CommandDispatch_Service | 前台输出事务结果并尝试停车保存。 | runtime_tuning / export / boundaries |
| App/services/command_dispatch.c | CommandDispatch_QueueTuningEvent | 把即时业务事件映射为SCB事件描述。 | runtime_tuning / export / boundaries |
| App/services/command_dispatch.c | CommandDispatch_PublishRunStarted | 将原运行起跑事件入通信队列。 | runtime_tuning / export / boundaries |
| App/services/command_dispatch.c | CommandDispatch_PublishRunStopped | 将原运行停车事件入通信队列。 | runtime_tuning / export / boundaries |
| App/hmi/menu.c | Menu_HandleKey | 执行原菜单导航与调参动作。 | g4_hmi / param |
| App/hmi/menu.c | Menu_Update | 处理停车菜单按键沿、保存及显示。 | g4_hmi / param |
| App/hmi/menu.c | Menu_GetPage | 读取当前页面编号。 | g4_hmi / param |
| App/hmi/menu.c | Menu_GetArrow | 读取当前光标编号。 | g4_hmi / param |
| App/hmi/menu.c | Menu_GetFanEditIdle | 读取当前风扇编辑选择。 | g4_hmi / param |
| App/hmi/menu.c | Menu_RestorePosition | 按镜像合法性恢复页面与光标。 | g4_hmi / param |
| App/hmi/menu_keys.c | Menu_ReadKeyAverage | 对指定按键 ADC 内存读三次求整数均值。 | g4_hmi / param |
| App/hmi/menu_keys.c | Menu_ScanKey | 按原两路 ADC 区间判定按键。 | g4_hmi / param |
| App/hmi/menu_keys.c | MenuKeys_GetAdc1 | 读取第1路按键显示滤波值。 | g4_hmi / param |
| App/hmi/menu_keys.c | MenuKeys_GetAdc2 | 读取第2路按键显示滤波值。 | g4_hmi / param |
| App/hmi/menu_render.c | Menu_DrawContent | 按当前页面绘制内容。 | g4_hmi / param |
| App/hmi/menu_render.c | Menu_DrawCursor | 位置或页面变化时擦除并重绘光标。 | g4_hmi / param |
| App/hmi/menu_render.c | MenuRender_Reset | 清屏并使下次页面绘制失效。 | g4_hmi / param |
| App/hmi/menu_render.c | MenuRender_PreparePage | 页面改变时清屏并复位光标缓存。 | g4_hmi / param |
| App/hmi/menu_render.c | MenuRender_Draw | 依次绘制页面内容与光标。 | g4_hmi / param |

本批额外修改的既有定义：Lifecycle_Init/TryStart/TrySetMode、Safety_RequestStop/FinalizeStop、
Scheduler_Loop/ControlISR、Tracking_Observe/Execute/SetMode、Control_UpdateLegacy、
Wireless_Adjust、SCB_ValidateValues分别按本文职责表和真实调用点复核；其余getter及未改语句
沿用已提交G2/G3契约与本轮完整回归，不借G4改休眠控制行为。
