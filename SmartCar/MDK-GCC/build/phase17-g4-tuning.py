exec(open(__file__.replace('tuning.py','refactor.py'),encoding='utf-8').read().split('# Four banks')[0])
t=read('tuning.c');th=read('tuning.h');proto=read('scb_protocol.c')
codes=['OK','RECEIVED','APPLIED','SAVED','INVALID_ARG','OUT_OF_RANGE','BUSY','SPECIAL_ELEMENT','NOT_FOUND','IO_ERROR']
def biz(s):
    s=s.replace('scb_values_t','tuning_values_t').replace('scb_status_t','tuning_result_code_t').replace('scb_command_t','tuning_request_t')
    for c in codes:s=s.replace('SCB_'+c,'TUNING_'+c)
    return s
enum='typedef enum\n{\n'+''.join('    TUNING_'+c+', /**< '+d+' */\n' for c,d in zip(codes,['参数合法或请求成功。','请求等待完整拍。','已在完整拍应用。','保存成功。','参数形状无效。','参数越界或非有限。','当前状态不允许。','赛道元素禁止覆盖。','目标不存在。','保存或服务错误。']))+'} tuning_result_code_t;\n'
defs=enum+'''typedef struct
{
    uint8_t mode; /**< 目标模式，0电磁、1光电。 */
    uint8_t gear; /**< 目标档位，0低、1高。 */
    tuning_values_t values; /**< 完整八项请求值。 */
} tuning_request_t;

typedef struct
{
    uint32_t tick; /**< 原捕获采样tick，ms。 */
    uint32_t control_sequence; /**< 完整控制序号。 */
    uint32_t revision; /**< 本次有效参数版本。 */
} tuning_position_t;

typedef struct
{
    tuning_result_code_t code; /**< 业务状态；仅APPLIED携带values。 */
    tuning_values_t values; /**< 已应用请求或最终成功值。 */
    tuning_position_t position; /**< 原发生点捕获的位置。 */
} tuning_reply_t;
'''
header('control/tuning_values.h','调参业务值、请求及结果。','纯数值validator与业务类型不依赖事务、BSP或通信。',inc('app_types.h'),defs+decl('tuning_result_code_t Tuning_ValidateValues(const tuning_values_t *values)','校验完整八项参数','tuning_values.c::Tuning_ValidateValues'))
validator=biz(extract(proto,'SCB_ValidateValues')).replace('SCB_ValidateValues','Tuning_ValidateValues')
write('control/tuning_values.c',filedoc('tuning_values.c','完整调参值的唯一范围校验。','仅按同源配置限值执行有序比较；NaN、无穷及整数越界失败，不钳位。')+inc('control/tuning_values.h','app_config.h')+'\n'+validator)
adapter=extract(proto,'SCB_ValidateValues');adapter=adapter[:adapter.index('\n{')]+'''\n{
    tuning_values_t business;
    if (!values)
    {
        return SCB_INVALID_ARG;
    }
'''+''.join(f'    business.{n} = values->{n};\n' for n in ['kp','k2p','kd','kp_v','ki_v','base_speed','fan_run','fan_idle'])+'''    switch (Tuning_ValidateValues(&business))
    {
        case TUNING_OK:
            return SCB_OK;
        case TUNING_INVALID_ARG:
            return SCB_INVALID_ARG;
        default:
            return SCB_OUT_OF_RANGE;
    }
}
'''
write('services/scb_protocol.c',proto.replace(extract(proto,'SCB_ValidateValues').strip(),adapter.strip()).replace('#include "services/scb_protocol.h"','#include "services/scb_protocol.h"\n#include "control/tuning_values.h"'))
context='''typedef struct
{
    uint8_t stopped; /**< 原停车三条件结果。 */
    uint8_t started; /**< 已启动控制链。 */
    uint8_t mode; /**< 当前所选模式。 */
    uint8_t gear; /**< 当前所选档位。 */
    uint8_t ordinary; /**< 电磁普通主态且环岛NORMAL。 */
    uint16_t protection_flags; /**< 同拍累计保护位。 */
    uint32_t control_ticks; /**< 原2ms等待拍计数。 */
    uint32_t sample_tick; /**< 采样时基，ms。 */
    uint32_t control_sequence; /**< 完整控制拍序号。 */
} tuning_context_t;

typedef enum
{
    TUNING_EVENT_TUNE_APPLIED, /**< 本次成功值应用。 */
    TUNING_EVENT_TUNE_SUSPENDED, /**< 元素暂停覆盖。 */
    TUNING_EVENT_TUNE_RESUMED, /**< 普通拍恢复覆盖。 */
    TUNING_EVENT_PARAM_SAVED, /**< 前台保存成功。 */
    TUNING_EVENT_PARAM_SAVE_FAILED /**< 前台保存失败。 */
} tuning_event_kind_t;

typedef struct
{
    tuning_event_kind_t kind; /**< 待即时适配的业务事件种类。 */
    tuning_result_code_t code; /**< 业务状态，显式映射至协议。 */
} tuning_event_t;
'''
status=th[th.index('typedef enum'):th.index('/**\n * @brief 初始化')].replace('scb_values_t','tuning_values_t')
sigs=['void Tuning_Init(void)','uint32_t Tuning_GetRevision(void)','void Tuning_ReadActive(tuning_values_t *values)','void Tuning_PrepareSet(uint32_t id, const tuning_request_t *request, const tuning_context_t *context, tuning_result_code_t validated, tuning_reply_t *reply)','void Tuning_CommitParked(const tuning_request_t *request, tuning_reply_t *reply)','void Tuning_Switch(uint8_t enable, const tuning_context_t *context, tuning_reply_t *reply)','void Tuning_SelectControl(uint8_t ordinary, uint8_t allowed, const pid_track_gains_t *original, int16_t base_speed, const tuning_context_t *context)','uint8_t Tuning_GetControl(tuning_values_t *values)','void Tuning_CompleteControl(const tuning_context_t *context)','void Tuning_Stop(void)','void Tuning_ResetSession(void)','void Tuning_GetStatus(tuning_status_t *out)','uint8_t Tuning_TakeResult(tuning_reply_t *reply, uint32_t *id)','uint8_t Tuning_CopyUnsaved(tuning_request_t *request)','void Tuning_CompleteSave(uint8_t saved)','void Tuning_CopyEventValues(tuning_event_kind_t kind, uint32_t *revision, tuning_values_t *values)','uint8_t Tuning_TakeRevision(uint32_t *revision)','uint8_t Tuning_TakeEvent(tuning_event_t *event)']
header('control/tuning.h','完整拍调参事务与业务状态。','事务拥有基线、待应用、成功值、有效值和最终邮箱；前台用短临界区，TIM6独占提交。',inc('control/tuning_values.h','control/pid.h'),status+context+''.join(decl(s,'访问完整拍调参事务','tuning.c::'+re.search(r'(\w+)\(',s)[1]) for s in sigs))
core=filedoc('tuning.c','完整控制拍调参事务。','仅持有独立baseline、pending、last_success、effective与final；上下文由system输入，事件即时取走适配。')+inc('bsp.h','app_config.h','control/tuning.h','control/motor.h')+'#include <string.h>\n\n'
state=t[t.index('typedef struct'):t.index('/* 仅前台使用')]
state=biz(state).replace('status; /**< 最终','code; /**< 最终');core+=state+'\n/* 版本发布位与单事件槽只在当前业务调用后立即取走，不作为跨轮队列。 */\nstatic uint8_t s_revision_pending;\nstatic uint8_t s_event_pending;\nstatic tuning_event_t s_event;\n\n'
event=fn('static void Tuning_Event(tuning_event_kind_t kind, tuning_result_code_t code)','发布本次调用的即时业务事件。','只保存种类和状态，不提前冻结可能被中断改变的值或版本。',[('in','kind 本次唯一业务事件种类。'),('in','code 业务结果。')],'''    s_event.kind = kind;
    s_event.code = code;
    s_event_pending = 1;''','调用后由原上下文立即Synchronize；停车前台与TIM6生产者互斥。');core+=event+'\n'
for n in ['Tuning_ReadActive','Tuning_ApplyShared','Tuning_Equal','Tuning_RejectPending','Tuning_GetControl','Tuning_Stop','Tuning_GetStatus']:
    f=biz(extract(t,n)).replace('s_final.status','s_final.code')
    if n=='Tuning_ReadActive':f=f.replace('static void','void',1)
    core+=f+'\n'
f=biz(extract(t,'Tuning_SelectControl')).replace('int16_t base_speed)\n','int16_t base_speed, const tuning_context_t *context)\n')
for old,new in [('Tracking_GetState()->mode','context->mode'),('Tracking_GetState()->gear','context->gear'),('Lifecycle_IsStarted()','context->started'),('Tracking_GetState()->protection_flags','context->protection_flags')]:f=f.replace(old,new)
f=f.replace('Telemetry_SetRevision(s_tuning.revision);','s_revision_pending = 1;');core+=f+'\n'
f=biz(extract(t,'Tuning_CompleteControl')).replace('Tuning_CompleteControl(void)','Tuning_CompleteControl(const tuning_context_t *context)').replace('BSP_Sampler_GetTick()','context->sample_tick').replace('Tracking_GetState()->control_sequence','context->control_sequence').replace('s_final.status','s_final.code')
f=f.replace('SCB_EVENT_','TUNING_EVENT_');core+=f+'\n'
f=biz(extract(t,'Tuning_ResetSession')).replace('    Datalog_CancelExport();\n','');core+=f+'\n'
f=biz(extract(t,'Tuning_Init')).replace('    SCB_SetHandlers(Tuning_HandleCommand, Tuning_ResetSession);','    s_revision_pending = s_event_pending = 0;');core+=f+'\n'
# Gate and capture remain inside the old caller critical section.
f=biz(extract(t,'Tuning_Set'));b=body(f);b=b[b.index('    parked = App_IsStopped();'):b.index('    BSP_Time_ExitCritical(mask);')]
b=b.replace('parked = App_IsStopped();','parked = context->stopped;').replace('command->','request->').replace('s_pending = *command;','s_pending = *request;')
for old,new in [('Lifecycle_IsStarted()','context->started'),('Lifecycle_GetControlTicks()','context->control_ticks'),('Tracking_GetState()->mode','context->mode'),('Tracking_GetState()->gear','context->gear'),('Tuning_IsOrdinary()','context->ordinary'),('tick = BSP_Sampler_GetTick();','reply->position.tick = context->sample_tick;'),('sequence = Tracking_GetState()->control_sequence;','reply->position.control_sequence = context->control_sequence;')]:b=b.replace(old,new)
core+=fn(sigs[3],'检查SET门禁并暂存运行请求。','调用方已在锁外校验数值；锁内保持目标、邮箱、等待和元素检查顺序。',[('in','id 已预留请求编号。'),('in','request 非空完整请求。'),('in','context 原临界区内捕获的业务状态。'),('in','validated 锁外validator结果。'),('out','reply 非空状态与原捕获位置。')],'    tuning_result_code_t status = validated;\n    uint8_t parked;\n'+b+'    reply->code = status;\n    reply->position.revision = s_tuning.revision;','前台持原短临界区调用，不合入bank。')+'\n'
b=body(biz(extract(t,'Tuning_Set')));b=b[b.index('        s_last_success ='):b.index('    revision = s_tuning.revision;')];b=b[:b.rfind('    }')];b='\n'.join(line[4:] if line.startswith('    ') else line for line in b.splitlines());b=b.replace('command->','request->').replace('Telemetry_SetRevision(s_tuning.revision);','s_revision_pending = 1;').replace('status = TUNING_APPLIED;','reply->code = TUNING_APPLIED;').replace('SCB_EVENT_TUNE_APPLIED, status','TUNING_EVENT_TUNE_APPLIED, reply->code');b+='\n    reply->values = request->values;\n    reply->position.revision = s_tuning.revision;'
core+=fn(sigs[4],'合入停车请求后的成功事务。','system已按原顺序写bank及共享值；独立记录成功目标、有效值和新版本。',[('in','request 非空已通过停车门禁的请求。'),('in,out','reply 保留合入前tick和序号，更新APPLIED和值。')],b,'停车前台调用后立即同步事件。')+'\n'
f=biz(extract(t,'Tuning_Switch'));b=body(f);b=b[b.index('    if (enable'):b.index('    BSP_Time_ExitCritical(mask);')]
for old,new in [('App_IsStopped()','context->stopped'),('Lifecycle_IsStarted()','context->started'),('Tuning_IsOrdinary()','context->ordinary'),('Tracking_GetState()->mode','context->mode'),('Tracking_GetState()->gear','context->gear'),('tick = BSP_Sampler_GetTick();','reply->position.tick = context->sample_tick;'),('sequence = Tracking_GetState()->control_sequence;','reply->position.control_sequence = context->control_sequence;'),('revision = s_tuning.revision;','reply->position.revision = s_tuning.revision;')]:b=b.replace(old,new)
core+=fn(sigs[5],'开启或关闭当前目标覆盖许可。','同目标重开沿用基线，停车换目标重捕获；运行禁用在后续拍恢复。',[('in','enable 非零开启，零关闭。'),('in','context 非空锁内状态。'),('out','reply 非空状态及位置。')],'    tuning_result_code_t status = TUNING_OK;\n'+b+'    reply->code = status;','调用方持原临界区，不格式化或Flash。')+'\n'
extra=[('uint32_t Tuning_GetRevision(void)','读取当前参数版本。','',[],'    return s_tuning.revision;','当前uint32版本，允许既定路径回绕。'),('uint8_t Tuning_TakeResult(tuning_reply_t *reply, uint32_t *id)','取走独立最终结果邮箱。','复制后只清id；编码与保存留前台锁外。',[('out','reply 非空结果输出。'),('out','id 非空请求编号输出。')],'''    uint32_t mask = BSP_Time_EnterCritical();
    tuning_result_t result = s_final;
    s_final.id = 0;
    BSP_Time_ExitCritical(mask);
    *id = result.id;
    if (!result.id)
    {
        return 0;
    }
    reply->code = result.code;
    reply->values = result.values;
    reply->position.tick = result.tick;
    reply->position.control_sequence = result.sequence;
    reply->position.revision = result.revision;
    return 1;''','有结果1，无结果0。'),('uint8_t Tuning_CopyUnsaved(tuning_request_t *request)','复制尚待保存的成功值及目标。','无脏时不写输出；只复制，不清脏或保存。',[('out','request 非空目标及成功值输出。')],'''    if (!s_dirty)
    {
        return 0;
    }
    request->mode = s_last_mode;
    request->gear = s_last_gear;
    request->values = s_last_success;
    return 1;''','有待保存成功值1，否则0。'),('void Tuning_CompleteSave(uint8_t saved)','记录一次前台保存尝试结果。','成功或失败都清事务dirty，避免对不可用Flash重复尝试。',[('in','saved 非零成功，零失败。')],'''    s_dirty = 0;
    s_tuning.save_state = saved ? 2 : 3;
    Tuning_Event(saved ? TUNING_EVENT_PARAM_SAVED : TUNING_EVENT_PARAM_SAVE_FAILED,
        saved ? TUNING_SAVED : TUNING_IO_ERROR);''',None),('void Tuning_CopyEventValues(tuning_event_kind_t kind, uint32_t *revision, tuning_values_t *values)','在原事件发生上下文读取版本和值。','先读取revision；保存事件取成功值，其余取effective，不提前冻结。',[('in','kind 待适配事件种类。'),('out','revision 非空版本输出。'),('out','values 非空完整值输出。')],'''    *revision = s_tuning.revision;
    *values = (kind == TUNING_EVENT_PARAM_SAVED || kind == TUNING_EVENT_PARAM_SAVE_FAILED)
        ? s_last_success : s_tuning.effective;''',None),('uint8_t Tuning_TakeRevision(uint32_t *revision)','取出当前调用待发布版本。','取出后清发布位，调用方在原记录发布前适配上传。',[('out','revision 非空输出，无待发布时保持。')],'''    if (!s_revision_pending)
    {
        return 0;
    }
    *revision = s_tuning.revision;
    s_revision_pending = 0;
    return 1;''','有待发布版本1，否则0。'),('uint8_t Tuning_TakeEvent(tuning_event_t *event)','取出当前调用产生的唯一事件。','取出后清槽；final邮箱与该即时描述互相独立。',[('out','event 非空输出，无事件时保持。')],'''    if (!s_event_pending)
    {
        return 0;
    }
    *event = s_event;
    s_event_pending = 0;
    return 1;''','有事件1，否则0。')]
for sig,b,d,ps,code,ret in extra:
    f=fn(sig,b,d,ps,code)
    if ret:f=f.replace(' */\n'+sig,' * @return '+ret+'\n */\n'+sig)
    core+=f+'\n'
write('control/tuning.c',core)

# Explicit context and parameter application, without protocol types.
app=filedoc('tuning_application.c','调参上下文与参数保存协调。','前台在原临界区捕获业务上下文；TIM6按同拍状态输入，事务产生的版本和事件即时适配。')+inc('bsp.h','app.h','system/lifecycle.h','system/tuning_application.h','system/parameter_application.h','control/tracking.h','control/electromagnetic_fsm.h','services/command_dispatch.h','services/telemetry.h')+'\n'
f=extract(t,'Tuning_IsOrdinary').replace('Tuning_IsOrdinary','TuningApplication_IsOrdinary');app+=f+'\n'
app+=fn('void TuningApplication_CaptureContext(tuning_context_t *context)','捕获本次业务调用的瞬时上下文。','不缓存为长期状态，不采集传感器；前台由调用方持原短临界区，TIM6同级不可抢占。',[('out','context 非空上下文输出。')],'''    context->stopped = App_IsStopped();
    context->started = Lifecycle_IsStarted();
    context->mode = Tracking_GetState()->mode;
    context->gear = Tracking_GetState()->gear;
    context->ordinary = TuningApplication_IsOrdinary();
    context->protection_flags = Tracking_GetState()->protection_flags;
    context->control_ticks = (uint32_t)Lifecycle_GetControlTicks();
    context->sample_tick = BSP_Sampler_GetTick();
    context->control_sequence = Tracking_GetState()->control_sequence;''')+'\n'
app+=biz(extract(t,'Tuning_Merge')).replace('Tuning_Merge','TuningApplication_Merge')+'\n'
app+=fn('void TuningApplication_Synchronize(void)','即时适配当前业务调用的版本和事件。','先发布上传版本，再交通信适配事件；不得跨记录生产、ISR返回或下一业务调用。',[],'''    uint32_t revision;
    tuning_event_t event;
    if (Tuning_TakeRevision(&revision))
    {
        Telemetry_SetRevision(revision);
    }
    if (Tuning_TakeEvent(&event))
    {
        CommandDispatch_QueueTuningEvent(&event);
    }''')+'\n'
app+=fn('void TuningApplication_Set(uint32_t id, const tuning_request_t *request, tuning_reply_t *reply)','协调完整设置请求。','validator在锁外执行，锁内只门禁和pending；停车成功在锁外合入，再取最终版本。',[('in','id 已预留请求编号。'),('in','request 非空完整业务请求。'),('out','reply 非空业务回复。')],'''    tuning_result_code_t validated = Tuning_ValidateValues(&request->values);
    tuning_context_t context;
    uint32_t mask = BSP_Time_EnterCritical();
    TuningApplication_CaptureContext(&context);
    Tuning_PrepareSet(id, request, &context, validated, reply);
    BSP_Time_ExitCritical(mask);
    if (reply->code == TUNING_OK)
    {
        TuningApplication_Merge(request->mode, request->gear, &request->values);
        Tuning_CommitParked(request, reply);
    }
    TuningApplication_Synchronize();
    reply->position.revision = Tuning_GetRevision();''','锁外末尾版本读取允许运行完整拍先提交，不能移入锁。')+'\n'
app+=fn('void TuningApplication_Switch(uint8_t enable, tuning_reply_t *reply)','协调覆盖许可切换。','原锁内捕获上下文并修改事务，锁外立即同步。',[('in','enable 非零开启、零关闭。'),('out','reply 非空结果输出。')],'''    tuning_context_t context;
    uint32_t mask = BSP_Time_EnterCritical();
    TuningApplication_CaptureContext(&context);
    Tuning_Switch(enable, &context, reply);
    BSP_Time_ExitCritical(mask);
    TuningApplication_Synchronize();''')+'\n'
app+=fn('void TuningApplication_FinalizeStop(void)','日志收尾后合入未保存成功值。','只合入明确目标，不清事务dirty，不访问Flash。',[],'''    tuning_request_t request;
    if (Tuning_CopyUnsaved(&request))
    {
        TuningApplication_Merge(request.mode, request.gear, &request.values);
    }''')+'\n'
app+=fn('void TuningApplication_ServiceSave(void)','停车前台尝试保存一次事务参数。','停车门禁通过且有成功脏值才尝试；成功失败均完成事务保存状态并即时适配事件。',[],'''    tuning_request_t request;
    if (App_IsStopped() && Tuning_CopyUnsaved(&request))
    {
        uint8_t saved = Param_TryFlushPending();
        Tuning_CompleteSave(saved);
        TuningApplication_Synchronize();
    }''')
write('system/tuning_application.c',app)
appsigs=['void TuningApplication_CaptureContext(tuning_context_t *context)','void TuningApplication_Set(uint32_t id, const tuning_request_t *request, tuning_reply_t *reply)','void TuningApplication_Switch(uint8_t enable, tuning_reply_t *reply)','void TuningApplication_FinalizeStop(void)','void TuningApplication_ServiceSave(void)','void TuningApplication_Synchronize(void)']
header('system/tuning_application.h','调参上下文与保存协调入口。','业务请求与结果独立于通信；即时同步接口在记录发布前执行。',inc('control/tuning.h'),''.join(decl(s,'协调调参业务与应用状态','tuning_application.c::'+re.search(r'(\w+)\(',s)[1]) for s in appsigs))

# Dispatch retains the original wire serializers and static workspaces.
dispatch=filedoc('command_dispatch.c','通信业务路由、结果编码与事件适配。','前台拥有原固定JSON工作区；ISR事件仅复制描述入原SCB队列，业务事务保持在control。')
dispatch+=inc('bsp.h','app.h','app_config.h','system/lifecycle.h','system/safety.h','system/tuning_application.h','system/parameter_application.h','control/tracking.h','control/tuning.h','control/electromagnetic_fsm.h','control/optical_fsm.h','control/motor.h','services/datalog.h','services/scb_link.h','services/telemetry.h','services/command_dispatch.h')+'#include <string.h>\n\n'
dispatch+=t[t.index('/* 仅前台使用'):t.index('/**\n * @brief 读取当前电磁')].replace('static datalog_meta_t s_log_meta; /**< 前台独占缓存头副本，避免256B进入查询栈。 */','static datalog_meta_t s_log_meta;')+'\n'
dispatch+=fn('static scb_status_t CommandDispatch_MapResult(tuning_result_code_t code)','将业务状态显式映射为协议状态。','逐枚举映射，不依赖两种枚举的数值相同。',[('in','code 调参业务状态。')],'    switch (code)\n    {\n'+''.join('        case TUNING_'+c+':\n            return SCB_'+c+';\n' for c in codes)+'        default:\n            return SCB_IO_ERROR;\n    }').replace(' */\nstatic scb_status_t',' * @return 相应SCB状态；未知业务值映射IO_ERROR。\n */\nstatic scb_status_t')+'\n'
for target,source in [('CommandDispatch_ToWire','business'),('CommandDispatch_ToBusiness','wire')]:
    towire=source=='business';sig=('static void '+target+'('+('scb_values_t *wire, const tuning_values_t *business' if towire else 'tuning_values_t *business, const scb_values_t *wire')+')')
    dst,src=('wire','business') if towire else ('business','wire')
    dispatch+=fn(sig,'逐字段复制业务与协议的八项参数。','两类型各自拥有接口，禁止强转或别名共享布局。',[('out',dst+' 非空完整输出。'),('in',src+' 非空完整输入。')],''.join(f'    {dst}->{n} = {src}->{n};\n' for n in ['kp','k2p','kd','kp_v','ki_v','base_speed','fan_run','fan_idle']))+'\n'
for n in ['Tuning_Begin','Tuning_Send','Tuning_Position','Tuning_ReadBank','Tuning_QueryParams','Tuning_QueryStatus','Tuning_QueryResult','Tuning_QueryLogMeta','Tuning_LogCommand','Tuning_HandleCommand']:
    f=extract(t,n)
    # rename all dispatch references including private Set/Switch below
    for q in ['Begin','Send','Position','ReadBank','QueryParams','QueryStatus','QueryResult','QueryLogMeta','LogCommand','HandleCommand','Set','Switch']:f=f.replace('Tuning_'+q,'CommandDispatch_'+q)
    if n=='Tuning_HandleCommand':f=f.replace('void CommandDispatch_HandleCommand','static void CommandDispatch_HandleCommand',1)
    if n=='Tuning_QueryParams':
        f=f.replace('    tuning_status_t status;', '    tuning_status_t status;\n    scb_values_t effective;')
        f=f.replace('Tuning_ReadActive(&status.effective);','Tuning_ReadActive(&status.effective);')
        f=f.replace('SCB_JsonValues(&writer, &status.effective);','CommandDispatch_ToWire(&effective, &status.effective);\n    SCB_JsonValues(&writer, &effective);')
    dispatch+=f+'\n'
for name in ['Set','Switch']:
    f=extract(t,'Tuning_'+name);b=f[:f.index('\n{')].replace('Tuning_'+name,'CommandDispatch_'+name)
    if name=='Set':c='''    tuning_request_t request;
    tuning_reply_t reply;
    scb_writer_t writer;
    scb_status_t status;
    request.mode = command->mode;
    request.gear = command->gear;
    CommandDispatch_ToBusiness(&request.values, &command->values);
    TuningApplication_Set(id, &request, &reply);
    status = CommandDispatch_MapResult(reply.code);
    CommandDispatch_Begin(&writer, command->op, status);
    if (status == SCB_APPLIED)
    {
        SCB_JsonAppend(&writer, ",\\\"values\\\":");
        SCB_JsonValues(&writer, &command->values);
        CommandDispatch_Position(&writer, reply.position.tick,
            reply.position.control_sequence, reply.position.revision);
    }
    CommandDispatch_Send(id, command->op, &writer, status != SCB_RECEIVED);'''
    else:c='''    tuning_reply_t reply;
    scb_writer_t writer;
    TuningApplication_Switch((uint8_t)!strcmp(command->op, "TUNE_ENABLE"), &reply);
    CommandDispatch_Begin(&writer, command->op, CommandDispatch_MapResult(reply.code));
    CommandDispatch_Position(&writer, reply.position.tick,
        reply.position.control_sequence, reply.position.revision);
    CommandDispatch_Send(id, command->op, &writer, 1);'''
    # Place before handler to avoid undeclared private references.
    pos=dispatch.index(extract(t,'Tuning_HandleCommand').split('\n')[0]) if False else dispatch.index('/**\n * @brief 分发') if '/**\n * @brief 分发' in dispatch else -1
    # forward declarations below handle ordering without relocating documentation
    dispatch+='\n'+b+'\n{\n'+c+'\n}\n'
dispatch=dispatch.replace('static uint8_t s_json[SCB_PAYLOAD_MAX];','static void CommandDispatch_Set(uint32_t id, const scb_command_t *command);\nstatic void CommandDispatch_Switch(uint32_t id, const scb_command_t *command);\n\nstatic uint8_t s_json[SCB_PAYLOAD_MAX];')
dispatch+=fn('void CommandDispatch_ResetSession(void)','取消会话尚未完成请求和导出。','先清事务pending/final，再取消导出；已成功参数和开关不清。',[],'''    Tuning_ResetSession();
    Datalog_CancelExport();''')+'\n'
dispatch+=fn('void CommandDispatch_Init(void)','注册业务请求及会话回调。','在InitLink和Tuning_Init之后执行，不再次初始化链路或事务。',[],'''    SCB_SetHandlers(CommandDispatch_HandleCommand, CommandDispatch_ResetSession);''')+'\n'
dispatch+=fn('void CommandDispatch_InitLink(void)','初始化原可靠通信链路。','仅在原初始化调用点转发一次SCB_Init。',[],'    SCB_Init();')+'\n'
dispatch+=fn('void CommandDispatch_ServiceLink(void)','执行原前台链路服务。','在业务Service之后单独调用；不得合并检查点或增加重试。',[],'    SCB_Service();')+'\n'
dispatch+=fn('void CommandDispatch_Service(void)','前台输出事务结果并尝试停车保存。','先服务导出租期，后取最终结果编码，再处理停车保存。',[],'''    tuning_reply_t reply;
    uint32_t id;
    scb_writer_t writer;
    scb_values_t values;
    Datalog_ServiceExport();
    if (Tuning_TakeResult(&reply, &id))
    {
        CommandDispatch_Begin(&writer, "SET_PARAMS", CommandDispatch_MapResult(reply.code));
        if (reply.code == TUNING_APPLIED)
        {
            SCB_JsonAppend(&writer, ",\\\"values\\\":");
            CommandDispatch_ToWire(&values, &reply.values);
            SCB_JsonValues(&writer, &values);
            CommandDispatch_Position(&writer, reply.position.tick,
                reply.position.control_sequence, reply.position.revision);
        }
        CommandDispatch_Send(id, "SET_PARAMS", &writer, 1);
    }
    TuningApplication_ServiceSave();''')+'\n'
f=extract(t,'Tuning_Event');f=f[:f.index('\n{')].replace('static void Tuning_Event(scb_event_kind_t kind, scb_status_t status)','void CommandDispatch_QueueTuningEvent(const tuning_event_t *business)')+'''\n{
    telemetry_status_t upload;
    scb_event_t event = { 0 };
    tuning_values_t values;
    Telemetry_GetStatus(&upload);
    event.run_id = upload.run_id;
    event.tick = BSP_Sampler_GetTick();
    event.control_sequence = Tracking_GetState()->control_sequence;
    switch (business->kind)
    {
'''+''.join('        case TUNING_EVENT_'+k+':\n            event.kind = SCB_EVENT_'+k+';\n            break;\n' for k in ['TUNE_APPLIED','TUNE_SUSPENDED','TUNE_RESUMED','PARAM_SAVED','PARAM_SAVE_FAILED'])+'''    }
    event.status = CommandDispatch_MapResult(business->code);
    Tuning_CopyEventValues(business->kind, &event.revision, &values);
    event.has_values = 1;
    CommandDispatch_ToWire(&event.values, &values);
    (void)SCB_QueueEvent(&event);
}
''';dispatch+=f+'\n'
for n,source in [('Started','lifecycle.c'),('Stopped','safety.c')]:
    src=read(source);start=src.index('        telemetry_status_t upload;',src.index('/* 实际定时链') if n=='Started' else src.index('/* 首次停车事件'))
    end=src.index('    }',start);b=src[start:end];b='\n'.join(line[4:] for line in b.splitlines()).replace('(uint8_t)s_stop_reason','reason')
    dispatch+=fn('void CommandDispatch_PublishRun'+n+'('+('void' if n=='Started' else 'uint8_t reason')+')','将原运行'+('起跑' if n=='Started' else '停车')+'事件入通信队列。','按原顺序复制上传身份、采样tick和完整序号，入队失败仍忽略。',[] if n=='Started' else [('in','reason 已锁存且本次运行不变的首次停车原因。')],b,'原运行位置调用，不格式化JSON，不增加事件缓存。')+'\n'
dispatch=dispatch.replace('\\\"','\\"') if False else dispatch
write('services/command_dispatch.c',dispatch)
ds=['void CommandDispatch_Init(void)','void CommandDispatch_InitLink(void)','void CommandDispatch_Service(void)','void CommandDispatch_ServiceLink(void)','void CommandDispatch_ResetSession(void)','void CommandDispatch_QueueTuningEvent(const tuning_event_t *event)','void CommandDispatch_PublishRunStarted(void)','void CommandDispatch_PublishRunStopped(uint8_t reason)']
header('services/command_dispatch.h','业务通信适配的运行入口。','公开接口仅使用业务类型；SCB命令handler在实现内注册，不向system传递协议头。',inc('control/tuning.h'),''.join(decl(s,'执行通信业务适配','command_dispatch.c::'+re.search(r'(\w+)\(',s)[1]) for s in ds))

# Original runtime call sites and event locations.
for name in ['lifecycle.c','safety.c','scheduler.c']:
    path='system/'+name;s=(A/path).read_text(encoding='utf-8')
    s=s.replace('#include "services/scb_link.h"','#include "services/command_dispatch.h"\n#include "system/tuning_application.h"')
    if name in ['lifecycle.c','safety.c']:
        start=s.index('    {\n        telemetry_status_t upload;',s.index('/* 实际定时链') if name=='lifecycle.c' else s.index('/* 首次停车事件'))
        end=s.index('    }',start)+5
        s=s[:start]+('    CommandDispatch_PublishRunStarted();' if name=='lifecycle.c' else '    CommandDispatch_PublishRunStopped((uint8_t)s_stop_reason);')+s[end:]
    s=s.replace('    SCB_Init();\n    Tuning_Init();','    CommandDispatch_InitLink();\n    Tuning_Init();\n    CommandDispatch_Init();').replace('Tuning_Service();','CommandDispatch_Service();').replace('SCB_Service();','CommandDispatch_ServiceLink();').replace('Tuning_FinalizeStop();','TuningApplication_FinalizeStop();')
    if name=='scheduler.c':s=s.replace('    Tracking_Observe(1, 0);','    Tracking_Observe(1, 0);\n    TuningApplication_Synchronize();').replace('    Tuning_CompleteControl();','    tuning_context_t tuning_context;\n    TuningApplication_CaptureContext(&tuning_context);\n    Tuning_CompleteControl(&tuning_context);\n    TuningApplication_Synchronize();')
    write(path,s)
s=(A/'control/tracking.c').read_text(encoding='utf-8').replace('scb_values_t','tuning_values_t').replace('#include "control/tuning.h"','#include "control/tuning.h"\n#include "system/tuning_application.h"')
s=s.replace('    Tuning_SelectControl(e->kernel_state', '    tuning_context_t tuning_context;\n    TuningApplication_CaptureContext(&tuning_context);\n    Tuning_SelectControl(e->kernel_state').replace(': (em.enabled ? e->configuration.base_speed : Motor_GetState()->base_speed));',': (em.enabled ? e->configuration.base_speed : Motor_GetState()->base_speed), &tuning_context);')
# CaptureContext is a system dependency introduced only for context assembly; replace with direct same-tick input.
s=s.replace('#include "system/tuning_application.h"\n','').replace('    TuningApplication_CaptureContext(&tuning_context);','''    tuning_context = (tuning_context_t)
    {
        0, Lifecycle_IsStarted(), s_tracking.mode, s_tracking.gear,
        (uint8_t)(e->kernel_state == CONTROL_KERNEL_TRACKING
            && e->roundabout_state == ROUNDABOUT_STATE_NORMAL),
        s_tracking.protection_flags, (uint32_t)Lifecycle_GetControlTicks(),
        0, s_tracking.control_sequence
    };''')
write('control/tracking.c',s)
