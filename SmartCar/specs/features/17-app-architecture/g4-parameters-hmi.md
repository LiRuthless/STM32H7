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
| lifecycle/safety 的 RUN_STARTED/RUN_STOPPED 事件组装及 runtime 的 SCB_Init/Service 调用 | command_dispatch.c 的 PublishRunStarted/PublishRunStopped/InitLink/ServiceLink；精确签名、调用点及验证见 G4-07 |

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

## G4-07 运行事件与链路运行时适配增量（2026-10-23）

本增量落实既定通信类型边界：G2为保持调度语义而留在 lifecycle/safety 的 SCB 事件组装，
在 G4 迁入 services/command_dispatch.c。system 中不保留 SCB 类型、枚举、直接函数调用，
也不直接包含 scb_link/scb_protocol 头。G4先验收运行模块自身与新公共接口的直接边界；
现有datalog.h的传递协议依赖由已提交G5规约消除，完整传递闭包在G5/G6最终验收，不能在G4提前报通过。command_dispatch.h 只依赖标准类型及公开业务事件类型，
不得传递包含 SCB 头；需要 scb_command_t 的 HandleCommand 在 command_dispatch.c 内私有，
由本文件的 CommandDispatch_Init 注册，不再从无协议的公共头公开 HandleCommand。
这条可见性决定覆盖 G4-04 中“公开HandleCommand”的旧表述；函数签名仍为
`static void CommandDispatch_HandleCommand(uint32_t id, const scb_command_t *command)`。
通信业务测试通过真实 Init 注册后保存的 handler 调用它，不为测试扩大产品头的协议依赖。

新增接口全部在 services/command_dispatch.h 声明，唯一实现位于 command_dispatch.c：

| 精确声明 | 输入、输出及执行约束 |
|---|---|
| `void CommandDispatch_InitLink(void)` | 只调用一次 SCB_Init，无其他状态、注册、事件、调参初始化或返回值 |
| `void CommandDispatch_ServiceLink(void)` | 只调用一次 SCB_Service，不与 CommandDispatch_Service 合并，不增加门禁或重试 |
| `void CommandDispatch_PublishRunStarted(void)` | 原起跑事件完整组装，前台原位置调用；只借原 SCB 事件队列，满队返回仍忽略 |
| `void CommandDispatch_PublishRunStopped(uint8_t reason)` | reason 为 Safety 锁存的首次停车原因；前台/ISR 原首次停车位置调用，不清原因、不再次停车、不缓存事件或重试 |

PublishRunStarted 保留局部 `telemetry_status_t upload` 与清零的 `scb_event_t event`，
严格依次执行 Telemetry_GetStatus(&upload)、kind=RUN_STARTED、run_id=upload.run_id、
revision=upload.revision、tick=BSP_Sampler_GetTick()、
control_sequence=Tracking_GetState()->control_sequence、忽略返回的 SCB_QueueEvent(&event)。
其余字段来自原清零初始化，不添加 status、values 或新的采样。调用点固定在
Lifecycle_TryStart 的 BSP_Sampler_Start 之后、看门狗首次初始化/后续喂狗判断之前。
失败起跑和拒绝路径不得调用，采样 tick 仍在 Sampler_Start 后读取。

PublishRunStopped 同样保留 upload/event，严格依次执行 Telemetry_GetStatus、
kind=RUN_STOPPED、run_id、revision、BSP_Sampler_GetTick、Tracking完整序号、
stop_reason=reason、忽略返回的 SCB_QueueEvent。调用点为 Safety_RequestStop 首次路径中
Tracking_Stopped((uint8_t)s_stop_reason) 之后；以 `(uint8_t)s_stop_reason` 传参。
首因在此路径已经锁存，到下次前台成功起跑前不变；把该固定值作为参数不会改变事件首因，
字段填充顺序仍在适配器内保持。NONE 和重复停车路径都不得调用该接口；不得移至前台收尾、
提早到停止定时链之前，或把入队失败变为阻塞/重试。原事件队满 gap 行为由 SCB owner 保持。

Lifecycle_Init 原 SCB_Init/Tuning_Init 位置精确改为
CommandDispatch_InitLink → Tuning_Init → CommandDispatch_Init；第三步只注册 handler/reset，
不能再次初始化链路或事务。Tuning_Init 与 handler 注册不在起跑复位时调用。
Scheduler_Loop 运行分支精确保持 Datalog_Service → CommandDispatch_Service →
CommandDispatch_ServiceLink → pending收尾检查 → return；停车分支二次 pending 收尾后精确
保持 CommandDispatch_Service → CommandDispatch_ServiceLink → 原传感器/界面/无线/保存流程。
顶部和两个分支中的原检查点及独立 if 不合并，不把 ServiceLink 藏到 Service 末尾，
不以整体链路快照替代原 key/pending 重读。

本增量仅修改上述内部归属，不修改 SCB ABI、消息字节、队列容量/事件 gap、上传或日志状态。
Telemetry_BeginRun 等现存独立业务调用仍保持各自原运行位置；不借本增量改变上传算法。

### 真实测试与替身适配

- `tests/dual_mode_tracking.py::app_tests` 的当前侧源清单链接真实 command_dispatch 及其
  tuning_application 依赖。APP_MAIN 原 SCB_Init/Service/QueueEvent 替身继续位于链路边界；
  原 start_events/stop_events、stop_reason、link_services、运行/停车收尾断言保留。
  非调参夹具的 TUNING_DISABLED_STUB 按新业务签名适配；不能另造 PublishRun* 替身绕过
  真实事件装配，也不能复制第二份 SCB 事件组装公式。
- `tests/runtime_tuning.py` 保留原链路/结果缓存/事件替身及所有断言；SCB_SetHandlers 替身
  保存真实 CommandDispatch_Init 注册的两个指针。旧 Tuning_HandleCommand 调用改为该真实
  handler，原服务调用改为 CommandDispatch_Service；会话场景调用注册的 reset，保留
  Tuning_Init 仅重置业务的独立场景。InitLink/ServiceLink 必须经真实转发到原链路替身。
- `tests/g2_system.py::SAFETY` 的纯阈值/NONE 独立夹具把不应调用的 SCB_QueueEvent 替身
  改成 `void CommandDispatch_PublishRunStopped(uint8_t reason)` 且进入即 assert(0)；
  相应依赖改真实业务头。该夹具只检查 NONE 无副作用；首次/重复停车的真实适配集成覆盖
  仍由上述 app_tests 完成，不以阈值测试替代事件测试。
- `tests/phase16_check.py` 当前源清单和静态调用图同步真实 owner；原 SCB_ReceiveFrame 的
  已注册业务分发目标更新为 CommandDispatch_HandleCommand。历史 git show 源路径、
  协议期望和所有原判定不改；独立 scb_link/scb_protocol 入口仍检查真实链路/codec。

新增验收 G4-V08（FR-09/11/12）：system 全部 C/H 无 scb_* 类型、SCB_* 调用/枚举或
协议头直接依赖；G5/G6追加完整传递依赖闭包验收。上述四个适配入口真实链接，记录型替身核对初始化、双分支服务、
成功起跑和首次停车的调用顺序。RUN_STARTED/STOPPED 全字段与拆分前逐项一致，失败起跑、
NONE、重复停车无新增事件；队满仍仅原 gap，临界区及二次停车收尾断言保持。
该门禁随 G4-V04/05/07 一起执行，实际结果回填 validation，不在本增量预报通过。

## G4-08 模式切换由运行时协调（2026-10-23）

本增量落实既定运行时协调边界，不改变模式切换行为。原 control/tracking.c 的
`tracking_result_t Tracking_TrySetMode(uint8_t mode)` 完整迁至 system/lifecycle.c，
改名 `tracking_result_t Lifecycle_TrySetMode(uint8_t mode)`，声明位于 lifecycle.h。
tracking_mode_t、tracking_result_t 原枚举移到 app_types.h，名称、顺序、数值保持；
lifecycle.h 使用业务类型，不为该返回值包含 control 头。tracking.c 不再依赖参数应用或
生命周期初始化/reset协调，不保留旧Try入口、转发函数或兼容宏。

新增 `void Tracking_SetMode(uint8_t mode)`，在 tracking.h 声明，tracking.c 唯一定义，
函数体仅 `s_tracking.mode = mode;`。输入由 Lifecycle_TrySetMode 验证；setter本身不校验、
不应用参数、不清控制历史、不做保存，只由此协调入口调用。

Lifecycle_TrySetMode 的语句顺序逐项保持：mode>1返回TRACKING_ARG；原App_IsStopped门禁
失败返回TRACKING_BUSY；Param_TryFlushPending失败原样输出 `track mode save failed\r\n`
并返回TRACKING_SAVE_FAILED；成功执行 Tracking_SetMode(mode)，随后按原顺序
Control_ResetRunState → Motor_ResetRunState → PID_ResetAll → IMU_ResetRunState →
Param_ApplyCurrentDirection → 原二选一模式UART文本 → 返回TRACKING_OK。
同模式仍执行保存门禁、全部复位及回显，不新增同值早退；拒绝不执行后续副作用。
仅搬运归属与替换该单次赋值，不修改状态、参数、协议、Flash布局或实际运行时序。

所有当前调用方（scheduler/menu及仓库实际搜索命中的其余调用）改为 Lifecycle_TrySetMode，
显式包含 lifecycle.h；current侧测试和符号映射使用新入口，不改变历史 git show 路径、
历史输出或期望。dual_mode_tracking 参数独立夹具以真实 lifecycle.c 模式入口链接，按
现有函数节裁剪方式提供必要业务依赖；不能用复制实现或stub替换该入口。完整App与
runtime_tuning仍链接真实lifecycle，原四bank、运行BUSY、保存失败及状态保留断言保持。

新增G4-V09（FR-07/09/11/12）：源码归属检查证明control无Param_TryFlushPending/
Param_ApplyCurrentDirection调用且只有生命周期协调器调用Tracking_SetMode；用真实入口
验证非法模式/运行/保存失败/同模式成功及跨模式成功，逐项比较返回码、UART文本、保存次数、
mode写入和各reset/应用顺序。失败前已发生的保存副作用不回滚；原断言完整保留。
实际结果随G4门禁回填，本增量须纯文档独立提交后才能修改该模式切换实现。

## G4-09 休眠包装原版本发布副作用的同步闭包（2026-10-24）

此处澄清既定行为保持和即时同步契约，先提交纯文档，再作实现。用户已授权完整Phase17，
FR-10要求“保持编译、内部调用和原状态；不新增活动调用”，不允许删除旧入口已有副作用。
App/Core/BSP当前只存在Control_UpdateLegacy的定义和声明，没有活动调用；本增量不向任何
活动调度、回调或调用表新增该入口，不改变其休眠状态，也不接入赛道元素。

已从已提交G3源码6125495核对原路径：

1. control/legacy_control.c::Control_UpdateLegacy故障路径在Tracking_AddProtection之后调用
   `Tracking_Observe(0, (uint8_t)reason)`，随后App_RequestStop；成功路径调用
   `Tracking_Observe(1, 0)`，随后Lifecycle_MarkRunning、Tracking_Execute。
2. control/tracking.c::Tracking_Observe内部调用Tuning_SelectControl。
3. 旧control/tuning.c::Tuning_SelectControl在既定版本变化条件下递增revision并直接
   `Telemetry_SetRevision(s_tuning.revision)`，因此两条旧包装路径都已经具有版本发布副作用。

G4已批准将该同一副作用拆为：Tuning_SelectControl只置s_revision_pending，
system/tuning_application.c::TuningApplication_Synchronize通过Tuning_TakeRevision取出后
在原上下文调用Telemetry_SetRevision。缺少同步会丢失旧入口原有的可观察副作用，
不能以入口未接入活动调度为理由删除该语义。

唯一实现：legacy_control.c显式包含system/tuning_application.h，两个Tracking_Observe后
各紧邻调用现有 `void TuningApplication_Synchronize(void)`。故障路径顺序固定为
Tracking_Observe(0,reason) → Synchronize → App_RequestStop；成功路径固定为
Tracking_Observe(1,0) → Synchronize → Lifecycle_MarkRunning → Tracking_Execute。
不新增函数、状态、协议事件生产者或Flash操作，不改变Observe、原因优先级、保护累计、
运行标志或执行条件。两处同步只消费该次Observe按既有条件产生的待发布版本；
Select不产生APPLIED/暂停/恢复事件，单事件槽仍遵守前述即时消费及互斥契约。
无待发布值时同步无发布动作，不额外递增revision，不新增遥测记录或运行身份。

验收使用tests/g4_legacy_boundary.py编译真实legacy_control.c，记录模块边界调用，
分别覆盖正常、采集错、无标定、灰度失线、电感和等于300及多保护并存；
断言两条路径的Observe/同步/停车或运行执行精确顺序、完整保护位与原首因优先级。
禁止以复制Control_UpdateLegacy函数体当替身；非目标角度包装依赖可使用进入即失败的替身。
结合g4_transactions验证只有原版本变化才发布且只取一次，并复验原runtime_tuning、
control_equivalence、g3_control及独立FSM/参数/App子项，不删除原断言。
只含关闭调参的独立FSM/参数夹具给新增适配依赖提供禁用同步替身；
完整App/调参集成继续链接真实tuning_application。G4-V05/G4-V07据此补充事实，
不能把这两处旧语义适配称为新增活动调用或宣称启用休眠包装。

用户确认事实（2026-10-24）：自动审批对本项两次拒绝后，向用户明确询问是否允许在休眠
Control_UpdateLegacy的两处Tracking_Observe后调用既有TuningApplication_Synchronize，
且入口仍不接入运行调度；用户在选择弹窗回答“允许，保持原有副作用”。
该具体批准覆盖本节既定两处同步及其原版本发布副作用，不扩大休眠入口或赛道元素范围。
