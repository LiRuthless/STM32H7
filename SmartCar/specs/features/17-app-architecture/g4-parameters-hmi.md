# G4 参数、完整拍调参、命令适配与 HMI 实施契约

日期：2026-10-23。对应 FR-07/09/11/12/13、V-04/05/06/07/08。用户已授权完整五大类重构；
本文件仅规定既有行为的职责拆分，不新增参数、菜单项、通信命令或保护。以 G1 保持的真实源码为
语义来源，接口使用已批准 G2/G3 owner。实施前本文件须独立提交；本组无未决设计项。

## G4-01 文件、依赖及所有权

文件直接放在五类目录中，不再嵌套。删除拆空的 services/param.c；不存在将原实现简单改名后
继续混合全部职责的 façade。原接口名字只有下表所列目标声明一次，调用者改为包含真实 owner 头。

| 源符号/状态 | 目标文件及决定 |
|---|---|
| param.c::s_direction[2][2]、Param_GetDirection、LoadBanks、StoreBanks | services/parameter_bank.c/h；唯一四 bank owner；加载/写镜像时显式传镜像指针 |
| param.c::s_param、s_dirty、s_flash_usable、s_dirty_tick；三个 param_store 类型及大小/偏移断言 | services/parameter_store.c/h；镜像类型放 services/parameter_image.h；仅 store 持有镜像可写地址 |
| Param_CalculateCrc16/V1/V2/V3、ClampFanDuty、SetOpticalDefaults、SetDefaults、WriteMirror | parameter_store.c 私有；原公式/默认/布局/转换/错误文本不变 |
| Param_Load 的读头/分版本读取/迁移/钳位/装 bank/重写部分 | parameter_store.c::ParamStore_Load；不应用 PID、gear 或菜单 |
| Param_Load 的 PI/低档/菜单恢复部分；ApplyCurrentDirection/SetDirection/SetFan/SelectGear/SetSpeedGains | system/parameter_application.c/h；原 Param_* 名字保留，由明确的协调层依次调用 store/bank/control/HMI |
| Param_GetFan、Param_GetSpeedGains | parameter_store.c/h，保留原签名、只读镜像语义 |
| Param_MarkDirty | parameter_store.c/h，保留 void(void)，用 G2 BSP_Time_GetTick 取同一毫秒时间 |
| Param_TryFlushPending/Save/ServiceSave/FlushPending | parameter_application.c/h 保留名字；store 只接受已捕获的保存快照 |
| menu.c::Menu_ReadKeyAverage/ScanKey；key_adc1/2、filt_key1/2、filt_inited | hmi/menu_keys.c/h；ADC、滤波和按键分类唯一 owner |
| menu.c::Menu_HandleKey/Update；page/arrow/mode/param_dirty/fan_edit_idle/last_key | hmi/menu.c/h；页面、编辑和按键沿唯一 owner |
| menu.c::Menu_DrawContent/DrawCursor；last_displayed_page/arrow；布局/颜色宏 | hmi/menu_render.c/h；绘制与刷新缓存唯一 owner |
| dashboard.c::Dashboard_Init/Update、s_last_refresh | hmi/dashboard.c/h 原样保留，硬件时间及业务字段改读 owner 接口 |
| wireless.c::Wireless_Adjust | services/wireless.c/h；原字符解析和回显保留，使用 G2 已分配的同一静态 UART 缓冲及参数协调 API |
| tuning.c::s_tuning/baseline/last_success/pending/pending_id/owner_mode/gear/last_mode/gear/has_success/dirty/staged/transition/final | control/tuning.c/h 唯一事务 owner；改用业务类型，不含 SCB/JSON/日志/上传/菜单或参数 Flash 头 |
| SCB_ValidateValues的纯数值算法 | control/tuning_values.c/h::Tuning_ValidateValues；独立于事务、BSP和其它业务，只有业务类型/限值/标准C依赖 |
| Tuning_ReadActive、ApplyShared、Equal、RejectPending、SelectControl、GetControl、CompleteControl、Stop、GetStatus | tuning.c 业务/控制部分，下面明确输入和结果出口 |
| Tuning_IsOrdinary | system/tuning_application.c私有，保留原电磁kernel==TRACKING且round==NORMAL表达式，在原锁内构造context.ordinary |
| Tuning_Set/Switch 的门禁和事务状态写入 | tuning.c 纯事务方法 + system/tuning_application.c/h 的上下文捕获/参数协调，不复制事务状态 |
| Tuning_Merge/FinalizeStop/Service 中保存部分 | system/tuning_application.c/h；显式参数合入，前台保存与回报 |
| Tuning_Event 的 SCB 映射；Begin/Send/Position/ReadBank/QueryParams/QueryStatus/QueryResult/QueryLogMeta/LogCommand/HandleCommand | services/command_dispatch.c/h；前台路由、JSON、二进制结果和事件适配唯一 owner |
| s_json[992]、s_cached[993]、s_query_json、s_log_meta、query keys/phases/saves/hex 常量 | command_dispatch.c；沿用同一容量与静态存储，不增加第二份工作区或把 256B 头放栈 |
| Tuning_Init 的状态初始化 | tuning.c::Tuning_Init；仅上电，仍读取活动值，不在起跑清 enabled |
| Tuning_Init 的 SCB_SetHandlers | CommandDispatch_Init，仍在 SCB_Init 后、原 Tuning_Init 所在初始化阶段注册 |
| Tuning_ResetSession 的事务清除 / Datalog_CancelExport | Tuning_ResetSession 只清原 pending/staged/final；CommandDispatch_ResetSession 先调用它再取消导出 |

parameter_bank 不依赖 store、control、system、HMI、协议或 BSP；parameter_store 仅依赖 bank、镜像/
业务类型、app_config、标准 C 和 bsp.h。控制/菜单/调参/活动 getter 不得藏在 store 的回调中。
本方案不用函数指针回调；system 显式编排参数使用者，services 的 command_dispatch 单向调用
system/tuning_application、control/tuning 和服务 API。control/tuning 不调用 command_dispatch。

## G4-02 参数镜像、bank 和应用接口

param_direction_t 的 float kp/k2p/kd、int16_t base_speed 保持，放共享业务类型头 app_types.h。
services/parameter_image.h 将原 param_store_v1_t/param_store_v2_t/param_store_t 分别
命名为 param_image_v1_t/param_image_v2_t/param_image_v3_t，定义原全部字段，保留 52/56/88B 及光电偏移 56/72 断言。
不打包、不重排、不改变保留字节、magic=0xA55A3C3C、version=3 或字节 8 起 CRC 的 44/48/80B。
参数存储只处理页面字节，默认 page=0/arrow=1 来自镜像 schema 常量，不能包含 menu.h。
合法页面校验在 Menu_RestorePosition 中执行，读取成功不意味着页面一定合法。

新增内部接口（非空指针均由内部调用者保证；不为拆分添加容错/校验）：

| 声明 | 精确输入、输出、副作用 |
|---|---|
| `void ParamBank_LoadBanks(const param_image_v3_t *image)` | 原 LoadBanks 四次赋值顺序；不标脏、不调用使用者 |
| `void ParamBank_StoreBanks(param_image_v3_t *image)` | 原 StoreBanks 四组字段及两个 optical_reserved 清零；不保留 image 地址 |
| `uint8_t ParamBank_SetDirection(uint8_t mode, uint8_t gear, const param_direction_t *profile)` | mode/gear>=2 或空返回0、无写；否则只复制该 bank 返回1 |
| `uint8_t ParamStore_Load(void)` | 原读取/迁移/重写；设置原 flash_usable 并返回；不应用运行状态 |
| `void ParamStore_GetPosition(uint8_t *page, uint8_t *arrow)` | 返回镜像原字节，不校验、不写菜单 |
| `void ParamStore_SetFan(uint8_t gear, uint8_t idle, int16_t duty)` | 原 gear 检查、0..10000 钳位、非零 idle 语义；只写目标镜像槽 |
| `void ParamStore_SetSpeedGains(float kp, float ki)` | 只按原顺序写镜像两字段，不写 PID、不标脏 |
| `uint8_t ParamStore_IsUsable(void)`、`ParamStore_IsDirty(void)` | 只返回各自原标量；不清状态、不读硬件 |
| `void ParamStore_RequestSave(void)` | 仅 s_dirty=1；绝不更新时间，保持 Param_Save 与 MarkDirty 的区别 |
| `uint8_t ParamStore_IsSaveDue(void)` | 原短路 s_dirty && uint32_t(now-dirty_tick)>=3000；只有 dirty 时读 BSP_Time_GetTick |
| `uint8_t ParamStore_TryFlushPending(const param_save_snapshot_t *runtime)` | 保留 flash不可用返回0/无dirty返回1；只有可用且dirty时读取 runtime，再原 WriteMirror；失败锁存不可用且保留dirty，成功清dirty |

param_save_snapshot_t 只有 float kp_v/ki_v 与 uint8_t page/arrow。store 在真正写入时依次复制
kp_v、ki_v、page、arrow，然后 StoreBanks→magic→version→CRC→BSP_Flash_Write(0,88B)。
该快照是一次保存调用的输入，不成为第二份持久状态。镜像只由 store 写，bank 不保存镜像指针。

保留接口的具体协调顺序：

- Param_Load：ParamStore_Load 完成全部读/重写后，取镜像 PI 并依次 PID_SetGain(KP_V)、
  PID_SetGain(KI_V)；Param_SelectGear(LOW)；取镜像 page/arrow 并 Menu_RestorePosition；返回
  原 flash_usable。读错仍装默认 RAM、仍应用 PI/gear/menu；读错不重写，重写失败仍应用 RAM。
- Param_ApplyCurrentDirection：分别读当前 mode/gear，GetDirection 后以 kp/k2p/kd/1.0f 写
  PID_SetTrackGains，然后 Motor_SetBaseSpeed；不新增无效索引降级。
- Param_SetDirection：Bank_Set 成功后先 Param_MarkDirty，再检查当前 mode/gear，匹配才应用；
  无效输入返回0，成功返回1。不能把标脏挪到应用之后。
- Param_SetFan：gear无效立即返回；store钳位及写槽，若当前gear匹配则按idle分支写活动风扇；
  最后 MarkDirty。共用两档风扇不拆成四bank，也不在此新增PWM直写。
- Param_SelectGear：无效立即返回；Tracking_SetGear→ApplyCurrentDirection→运行风扇→空闲风扇；
  不标脏、不复位PI历史、不改变mode。
- Param_SetSpeedGains：先镜像kp/ki，再活动PID的KP_V、KI_V，最后MarkDirty；不复位历史。
- Param_TryFlushPending：先读 Usable，不可用立即0；再读Dirty，无脏立即1；仅需保存时按原
  次序捕获 PID_GetState()->kp_v、ki_v、Menu_GetPage、Menu_GetArrow，再交store写入。
  不注册 capture 回调，不在无脏/不可用路径预读菜单或PID。
- Param_Save：RequestSave（不更新dirty_tick）→TryFlushPending并忽略返回。
  Param_ServiceSave：IsSaveDue为真才TryFlushPending；Param_FlushPending原样忽略返回。

所有默认值逐项保持：共享PI=20/0.75，光电两档=2/0.008/15/100，风扇来自原宏；电磁默认
方向字段仍清零，不能猜测可跑值。v1补空闲风扇与光电，v2只补光电，v3直接复制；无效镜像
回默认并重写；四风扇钳位后按原规则重写，方向/速度不另校验。参数错误UART文本逐字保留。

特别保持“镜像读值”和“活动读值”的差别：Wireless 的 vp/vi 只改 PID_SetGain 再 MarkDirty，
不改为 Param_SetSpeedGains。flush之前 Param_GetSpeedGains仍读旧镜像；真正flush才采集活动PI。
同理菜单绘制显示活动PID/电机，GET_PARAMS.values读取bank/镜像；不能统一 getter 掩盖这种差异。

## G4-03 调参业务类型与事务接口

app_types.h 放确实跨模块使用的 `tuning_values_t`：float kp/k2p/kd/kp_v/ki_v、int32_t
base_speed/fan_run/fan_idle，顺序/位宽与旧 scb_values_t 相同。control/tuning_values.h 中定义独立
`tuning_result_code_t`（TUNING_OK/RECEIVED/APPLIED/SAVED/INVALID_ARG/OUT_OF_RANGE/BUSY/
SPECIAL_ELEMENT/NOT_FOUND/IO_ERROR）、`tuning_request_t`（uint8_t mode/gear + values）、
`tuning_position_t`（uint32_t tick/control_sequence/revision）、`tuning_reply_t`（code+values+
position，是否带values由code==APPLIED判定）。这不是 typedef scb_* 或协议别名。
tuning_values.h只包含app_types.h和标准类型；TUNING_GAIN/SPEED/FAN限值仍同源来自app_config.h。
control/tuning.h包含tuning_values.h而不重复声明validator或结果枚举；
Tuning_ValidateValues唯一实现在control/tuning_values.c，完整事务模块不提供第二份实现。
原 tuning_status_t 的 effective 改为 tuning_values_t，其余字段和值不变。
业务事件枚举 tuning_event_kind_t 只有 TUNING_EVENT_TUNE_APPLIED/TUNE_SUSPENDED/
TUNE_RESUMED/PARAM_SAVED/PARAM_SAVE_FAILED；tuning_event_t 只有 kind/code，不含协议字段。
final保留原id/code/values/tick/sequence/revision，命名tuning_result_t；不能与effective或事件槽合并。

`tuning_context_t` 是瞬时业务输入：uint8_t stopped/started/mode/gear/ordinary；uint16_t
protection_flags；uint32_t control_ticks/sample_tick/control_sequence。system/tuning_application
在旧临界区内从 G2 Lifecycle、G3 tracking/FSM及BSP采样tick捕获；不保存上下文，不重新采样
传感器、不扩大临界区到Flash/JSON。TIM6的对应输入直接取同拍已确定状态。

| 方法 | 输入/输出与所有权 |
|---|---|
| `tuning_result_code_t Tuning_ValidateValues(const tuning_values_t *values)` | 原五float有序比较、三int范围；空INVALID_ARG，NaN/Inf/越界OUT_OF_RANGE，不钳位 |
| `void Tuning_Init(void)` | 仅memset s_tuning/s_final，pending_id=0，has_success/dirty/staged/transition=0，再ReadActive；不清baseline/last_success/owner/last_target，不注册SCB，不清bank，不起跑重置 |
| `uint32_t Tuning_GetRevision(void)` | 只读当前s_tuning.revision，不加新临界区；用于原SET锁外最后一次revision读取 |
| `void Tuning_ReadActive(tuning_values_t *values)` | 原PID/电机八项读取，无历史推进；供停车查询和应用协调取值 |
| `void Tuning_PrepareSet(uint32_t id, const tuning_request_t *request, const tuning_context_t *context, tuning_result_code_t validated, tuning_reply_t *reply)` | validated在旧锁外计算；方法在旧锁内执行原门禁；运行成功复制pending并返回RECEIVED；停车成功返回OK并捕获原回复位置，尚不改bank/成功状态 |
| `void Tuning_CommitParked(const tuning_request_t *request, tuning_reply_t *reply)` | 仅PrepareSet停车OK且system已完成Merge后调用；按原顺序写last_success/target/has_success/dirty/save_state、ReadActive(effective)、revision++、发布APPLIED事件描述；reply改APPLIED并写revision，保留Prepare捕获的tick/sequence |
| `void Tuning_Switch(uint8_t enable, const tuning_context_t *context, tuning_reply_t *reply)` | 原开启/关闭/目标门禁、基线捕获与回复位置；enable来自命令名，无字符串或协议类型 |
| `void Tuning_SelectControl(uint8_t ordinary, uint8_t allowed, const pid_track_gains_t *original, int16_t base_speed, const tuning_context_t *context)` | 原Select算法，仅原全局读取换context；不改变history，不格式化、不Flash |
| `uint8_t Tuning_GetControl(tuning_values_t *values)` | 原副本与overridden返回 |
| `void Tuning_CompleteControl(const tuning_context_t *context)` | 完整序号已增加后提交last_success/final，固定APPLIED位置；错误提前停车不能提交 |
| `void Tuning_Stop(void)`、`Tuning_ResetSession(void)`、`Tuning_GetStatus(tuning_status_t *out)` | 原事务临界区保持；Reset只清pending/staged/final，不取消日志，不清成功值/开关/effective |
| `uint8_t Tuning_TakeResult(tuning_reply_t *reply, uint32_t *id)` | 原Service临界区内复制final后只清final.id；空返回0。请求门禁在此之前仍拒绝下一SET，不能等ACK才释放或提前在Complete释放 |
| `uint8_t Tuning_CopyUnsaved(tuning_request_t *request)` | 只在dirty时复制last_success及明确目标返回1；无成功返回0，不清dirty |
| `void Tuning_CompleteSave(uint8_t saved)` | 仅原前台保存尝试后调用：清dirty，save_state=2/3，并发布保存事件描述；失败也只尝试一次 |
| `void Tuning_CopyEventValues(tuning_event_kind_t kind, uint32_t *revision, tuning_values_t *values)` | 按原Event先复制当前revision，再按kind取last_success或effective；无临界区和状态推进 |
| `uint8_t Tuning_TakeRevision(uint32_t *revision)` | 复制尚待适配的版本并清发布位，核心不调用Telemetry |
| `uint8_t Tuning_TakeEvent(tuning_event_t *event)` | 取本次调用产生的唯一kind/code描述并清槽，不在发布时提前采集values或revision |

停车CommitParked的revision仅原++（允许回绕0），Select的++后若0再++保持；不能统一两条路径。
ReadActive、ApplyShared、Equal、RejectPending 公式/执行次序不改；ApplyShared仍写KP_V→KI_V→
fan_run→fan_idle。业务方法不读参数存储、不发送事件、不包含SCB/Datalog/Telemetry头。

新增revision发布位及一个固定事件槽只用于立即取出，不构成第二条上传队列。一次业务方法最多
产生一个事件：staged时不产生暂停/恢复，暂停恢复仅非staged；每次方法结束必须立即由system
调用下述Synchronize清空，禁止跨下一次业务调用堆积/覆盖。最终结果邮箱仍独立保留至前台Service，
不能用事件槽代替final。没有动态分配、新大缓冲或新ISR格式化。

单槽不丢事件依赖实际调用者互斥，不能以“紧接调用”替代并发证明：

1. 前台产生事件只有CommitParked和CompleteSave。前者必须PrepareSet在旧锁内确认
   Lifecycle_IsStopped真才进入，后者必须Service中同一IsStopped门禁真才进入。此条件是
   !start && !key && !stop_pending；Safety_RequestStop先停采样/TIM6/TIM7，前台Finalize才释放
   pending。前台此调用返回前不会执行下一次Lifecycle_TryStart；起跑仅由前台启动入口发生。
   因此这两条前台事件产生至Synchronize之间不存在能提交APPLIED的活动TIM6。
2. TIM6产生事件只有CompleteControl；Select仅发布revision/transition，真正暂停/恢复事件也在
   CompleteControl。TIM15和TIM6同优先级互不抢占，TIM7更低；其它ISR不调用调参事件产生方法。
   前台不能抢占TIM6，故CompleteControl至同ISR中Synchronize之间不存在另一事件生产者。
3. Stop/ResetSession/Switch/PrepareSet不产生事件；Stop还按原逻辑清transition并取消pending。
   极端迟到控制入口在start为0时按原门禁退出，不能让已停前台与完整拍提交并行。
4. 每个调用点保证TakeEvent槽已清后才可能进入下一事件生产方法；若未来新增可运行前台的
   事件生产路径，必须重做规约及并发设计，不能静默依靠单槽覆盖。本组验证包含逐调用点和
   优先级审查，并覆盖停车中断、运行SET在锁外被完整拍抢占和事件队满的原gap语义。

## G4-04 应用协调、结果和事件适配

system/tuning_application.c 提供：

- `void TuningApplication_Set(uint32_t id, const tuning_request_t *request, tuning_reply_t *reply)`：
  Validate在锁外；原临界区捕获context并PrepareSet；退出锁后，仅停车OK依次
  私有`TuningApplication_Merge(uint8_t mode, uint8_t gear, const tuning_values_t *values)`依次
  Param_SetDirection→Param_SetSpeedGains→Param_SetFan(run)→Param_SetFan(idle)，再CommitParked。
  立即Synchronize后，再在锁外以Tuning_GetRevision覆盖reply.position.revision，
  然后返回reply供dispatch编码。保留原Tuning_Set在可能发生TIM6之后的最后一次revision读取，
  RECEIVED及拒绝也执行这次读取（但原JSON仅APPLIED附位置）。不能沿用PrepareSet锁内版本
  或强行扩锁阻止该抢占。参数merge顺序、截断int16_t和每次MarkDirty次数不合并。
- `void TuningApplication_Switch(uint8_t enable, tuning_reply_t *reply)`：原锁内捕获context并
  Switch；退出后立即Synchronize。原运行等待、普通元素和目标条件不改变。
- `void TuningApplication_FinalizeStop(void)`：CopyUnsaved成功才按原Merge顺序合入；不清dirty、
  不Flash。接在日志收尾之后、原Tuning_FinalizeStop位置。
- `void TuningApplication_ServiceSave(void)`：仅原App_IsStopped为真且CopyUnsaved成功时
  Param_TryFlushPending一次，再CompleteSave并Synchronize；失败不重复尝试，不回滚RAM。
- `void TuningApplication_Synchronize(void)`：立即TakeRevision并Telemetry_SetRevision，再
  TakeEvent交CommandDispatch_QueueTuningEvent。不能从核心直接调用该方法。

Synchronize调用点：前台Set/Switch操作返回前；TIM6 Tracking_Observe之后、执行/记录发布之前；
Tuning_CompleteControl之后、随后发布记录之前；CompleteSave之后。TIM15/TIM6同优先级互不抢占，
这两个TIM6调用返回至同步之间不存在采样、记录生产或前台观察者；允许把原函数内部的上传版本
赋值移到该紧邻同步点，但不得越过记录发布/ISR返回，不扩大关中断区。逐拍日志/上传revision
比较必须证明这种模块边界变更没有可观察差异。其它事件时序仍由原执行上下文立即映射。

`void CommandDispatch_QueueTuningEvent(const tuning_event_t *event)` 使用原SCB事件队列：
按原Tuning_Event次序获取Telemetry状态的run_id、BSP_Sampler_GetTick、Tracking完整序号，
随后映射事件kind/code，调用Tuning_CopyEventValues按原位置复制revision及values，
has_values=1后SCB_QueueEvent并忽略返回。不能在业务发布槽时提前冻结effective，避免改变前台
事件在上述读取间被TIM6打断时的原取值位置。
保存事件values来自last_success，其余来自effective；不能因满队列改写final或保留到另一个队列。
事件槽只携带kind/code，revision/values/tick/run_id均在原发生上下文同步复制；停车回复的tick/sequence仍是merge前
捕获，不能用稍后事件时间替代。业务与SCB状态逐枚举switch显式映射，不依赖数值强转。

CommandDispatch_Init注册 `SCB_SetHandlers(CommandDispatch_HandleCommand,
CommandDispatch_ResetSession)`。HandleCommand仍接收scb_command_t，逐字段复制SET的业务请求；
已预留request_id原样传入。SCB解析失败、去重、32请求容量预留、ACK和最终结果淘汰留原可靠层。
不增加二次接收或在业务拒绝前释放请求槽。op和未知命令回显仍由dispatch使用原字符串处理。

CommandDispatch_Service顺序固定为：Datalog_ServiceExport→Tuning_TakeResult及原JSON最终结果
发送→TuningApplication_ServiceSave。它替代G2 Scheduler原Tuning_Service调用，运行/停车均保留。
ResetSession先清Tuning pending/final，再Datalog_CancelExport；SCB已排队的会话清理仍由SCB完成。
原上电Tuning_Init和CommandDispatch_Init连在原初始化位置，不在起跑重复注册/清状态。

原Begin/Send/Position等函数改名CommandDispatch_*，保持参数形状；Query/Log/Handle的旧签名
除前缀外保持（id、scb_command或target），仍为本文件private；公开HandleCommand给SCB注册。
所有JSON键/顺序/数值格式、布尔、phase/save拼写、完整对象及溢出IO_ERROR替代保持。
GET_PARAMS锁内复制bank/镜像与tuning状态，停车effective仍从活动getter取，锁外编码；
GET_STATUS原内存meta、FSM、run_id、计数捕获保持，不扫描Flash；GET_REQUEST_RESULT展开
一层包装及LOG_DATA块offset/length逻辑、s_cached[length]终止位置不改。
GET_LOG_META仍256B头转512字符小写hex；LOG_READ成功直接QueueResult(LOG_DATA)，失败走RESULT；
LOG_BEGIN/END字段、960字节块上限、原导出租期和互斥由现有服务保持。

G4/G5职责边界：G4拥有command_dispatch全部路由/JSON及tuning业务/适配。G5只拆SCB frame、
JSON、command codec及记录/日志/上传；不得把这些分发方法搬回tuning。G4先使用现有Datalog API，
G5把其结果类型改为独立datalog_result_t时，由G5同步更新dispatch显式映射
OK/INVALID_ARG/OUT_OF_RANGE/BUSY/NOT_FOUND/IO_ERROR，不能让日志核心依赖SCB。
G5的ReadExport输出改为const uint8_t **data和uint16_t *length，只借出原缓存+8处原始数据，
失败仍先length=0；dispatch调用scb_command_codec中的
`uint16_t SCB_EncodeLogData(uint8_t *payload, uint32_t offset, const uint8_t *data, uint16_t length)`
将LE32 offset/LE16 length/LE16零及原数据写入既有s_json，返回8+length后以LOG_DATA入原缓存。
原s_export_block[968]仍只保留一份，原始数据从+8借用，不增加第二块，也不改变重复读/ACK/租期。
SCB_ValidateValues对业务validator的适配必须逐字段转换，不保留两份边界规则；
SCB codec独立宿主测试编译真实tuning_values.c，无需链接tuning.c/业务事务/BSP，不能用死代码
丢弃或未定义符号忽略掩盖新依赖。协议codec拆分及
函数声明位置由G5完成，G4先保持原SCB公共签名及全部608项协议断言。

## G4-05 菜单和绘制接口与边界行为

menu.h不包含main.h，使用stdint和真实业务声明。menu_keys.h保留MENU_KEY数值，menu.h保留
MENU_PAGE数值。原四个g_menu_*对象改为各owner的static：page=HOME/arrow=1归menu，
key_adc1/2=0归menu_keys；禁止跨模块extern可写对象或可写状态指针。

接口固定：`uint8_t Menu_GetPage(void)`、`Menu_GetArrow(void)`、`Menu_GetFanEditIdle(void)`；
`void Menu_RestorePosition(uint8_t page, uint8_t arrow)`仅按原六合法page和1..5 arrow恢复，不改
s_mode/fan_edit_idle/显示哨兵/last_key；`uint16_t MenuKeys_GetAdc1(void)`及Adc2只读显示滤波值。
Menu_ReadKeyAverage保持private且原bsp_adc_channel_t形参；Menu_ScanKey保留uint8_t(void)。
Menu_HandleKey保留void(uint8_t)，头中明确声明以便协调和测试，不赋予新活动调用方。

menu_render公开 `void MenuRender_Reset(void)`（LCD_Clear WHITE后把page=0xFFFF/arrow=0xFF）；
`void MenuRender_PreparePage(void)`（当前page变化才Clear，再更新page/arrow哨兵）；
`void MenuRender_Draw(void)`（原DrawContent→DrawCursor）。DrawContent/DrawCursor保持private，
只通过menu/key和业务只读API取值，不导航、不写bank、不拥有last_key。
Menu_Update依次 ScanKey→无键清last_key→RST新沿Reset、记录key、return→新沿HandleKey→
更新last_key→PreparePage→dirty且松键时Param_Save并清menu dirty→Draw。RST不立即重画，下一次
因page哨兵再Clear再绘制；不能“优化”掉这一次清屏或改变早退。

完整保留以下键盘与页面规则：

- 每路三次BSP_ADC_Read，uint32求和除3；先路1再路2；用原未滤波ADC/100的严格开区间
  (3,9)/(9,15)/(15,20)/(20,25)/(25,31)判键，路2可覆盖路1；最后才以alpha=0.7更新两显示值。
  滤波器仅首次扫描初始化，不起跑/页面切换重置；不把注释误写的“十次”当成算法。
- UP/DOWN分别按调参页5项、主页3项、其它4项循环；OK主页page=20+arrow再arrow=1；
  调参第5项只翻转fan_edit_idle。FOUR仍未知default无动作。
- BACK调参页首先Param_Save，此时page仍ADJUST且arrow仍旧值，随后HOME/arrow1/mode1；
  其它三个数据页只返回HOME/arrow1。不能先导航再保存。
- LEFT/RIGHT仅调参且mode=2：前三项±0.001f，第四项±100后显式int16_t，第5项从活动风扇读
  int32再±100并单侧钳位后传Param_SetFan；原五个独立if及末尾menu dirty=1、MarkDirty保持。
  不增加方向数值限制、无效arrow限制，保留设置器和菜单末尾重复MarkDirty。
- ADJUST1/2先写页面，再SelectGear低/高，再fan_edit_idle=0，最后mode=2；原arrow不改。
  Menu_HandleKey入口仍读取当前mode/gear和bank的局部副本，即使本键仅导航也保持顺序。
- DrawContent显示活动PID/电机值而非bank副本；全部字符串/坐标/位宽/颜色/float小数位逐调用保持。
  Cursor仅位置/页面变化时按原范围擦旧光标并画新光标；不新增非法arrow修复。

Dashboard保持100ms无符号差节流，初始化last_refresh=0，全部显示读取及gyro窄化位置保持。
G2电池/距离用Battery/Lifecycle getter，G3方向和kernel用Tracking getter；不把停车绘制改为ISR。
Wireless原d/c、len=2清len、非法len清整个buffer、数字非法清len、len4..7倒序权重浮点加法、
未知命令无回显、命中四字符格式回显和最后MarkDirty全部保持；不接入新命令或改变UART协议。

## G4-06 验证和提交门禁

| 项 | 对应条款 | 唯一通过标准 |
|---|---|---|
| G4-V01 | G4-01/02 | bank/store不包含或调用HMI/control/system；无capture回调；原每个函数/对象有唯一owner，镜像断言/CRC/默认及版本迁移逐字节匹配 |
| G4-V02 | G4-02 | 原param_tests四bank、v1/v2/v3、保存失败门禁全通过；新增vp/vi操作前/后/3000ms前后镜像getter与活动getter对照，证明只在实际flush采集活动PI |
| G4-V03 | G4-03/04 | runtime_tuning原全断言与历史控制等价保持；覆盖baseline/pending/last_success/effective独立、两模式两档/元素、保护取消、revision回绕、停止恢复、禁用再开、会话取消、final未取拒绝新SET |
| G4-V04 | G4-04 | 独立codec宿主入口只增编纯tuning_values.c且无事务/BSP stub；scb_protocol608项、scb_link去重/容量/ACK/事件gap、spi_log_export原断言不减；所有JSON/日志二进制结果与迁移前比对一致，拒绝无参数副作用 |
| G4-V05 | G4-03/04 | 每个业务调用紧接同步，final仍由前台take；记录/上传逐拍revision及事件kind/values/位置一致，ISR无JSON/Flash，无新事件队列、缓存或动态分配 |
| G4-V06 | G4-05 | 原key/menu断言保持；LCD记录型替身按同一ADC/键序比较全部绘图调用，重点BACK在导航前保存、RST早退/下拍清屏、松键保存、第二路覆盖和阈值边界 |
| G4-V07 | G4-01～05 | 双LCD GCC均0警告/0错误，Keil源清单存在/无重复；栈与静态RAM变化列明，CS-01～15/R08逐函数内容及依赖摘要复核通过 |

当前侧测试改为真实拆分源清单与owner API，历史git show路径和期望不得改动。不能在测试头导出
可写产品内部对象，不能以自生成期待值替代原历史比较；现有R07外部样本阻塞单列，完整入口不
伪报通过。新增验证仅针对真实边界风险，不为纯路径迁移复制实现。所有实际结果回填validation。
G4实现不能早于本文件纯规约提交，G2/G3的未完成门禁不被本文件替代；无代码随本文件提交。
AC5、实车、700/900us和吞吐继续待验。同步Phase06/08/13/14/16现行接口引用、README、
CHANGELOG及注释证据，历史验证记录保留。等价失败停止该组提交并保留失败证据，不趁机修参数。
