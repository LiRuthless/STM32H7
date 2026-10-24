/**
 * @file tuning.c
 * @brief 完整控制拍调参事务。
 * @details 仅持有独立baseline、pending、last_success、effective与final；上下文由system输入，事件即时取走适配。
 */
#include "bsp.h"
#include "app_config.h"
#include "control/tuning.h"
#include "control/motor.h"
#include <string.h>

typedef struct
{
    uint32_t id; /**< 已预留请求编号。 */
    tuning_result_code_t code; /**< 最终业务状态。 */
    tuning_values_t values; /**< 实际生效值。 */
    uint32_t tick; /**< 提交采样tick。 */
    uint32_t sequence; /**< 完整控制序号。 */
    uint32_t revision; /**< 提交版本。 */
} tuning_result_t;

/* 控制拍持有effective/成功值/待应用状态，前台接收与查询均在短临界区复制。 */
static tuning_status_t s_tuning;
static tuning_values_t s_baseline;
static tuning_values_t s_last_success;
static tuning_request_t s_pending;
static uint32_t s_pending_id;
static uint8_t s_owner_mode;
static uint8_t s_owner_gear;
static uint8_t s_last_mode;
static uint8_t s_last_gear;
static uint8_t s_has_success;
static uint8_t s_dirty;
static uint8_t s_staged;
static uint8_t s_transition; /**< 1暂停、2恢复；Complete固定发生时元数据。 */
/* TIM6写最终结果、前台复制并释放；槽非空时前台拒绝下一SET。 */
static tuning_result_t s_final;

/* 版本发布位与单事件槽只在当前业务调用后立即取走，不作为跨轮队列。 */
static uint8_t s_revision_pending;
static uint8_t s_event_pending;
static tuning_event_t s_event;

/**
 * @brief 发布本次调用的即时业务事件。
 * @details 只保存种类和状态，不提前冻结可能被中断改变的值或版本。
 * @param[in] kind 本次唯一业务事件种类。
 * @param[in] code 业务结果。
 * @note 调用后由原上下文立即Synchronize；停车前台与TIM6生产者互斥。
 */
static void Tuning_Event(tuning_event_kind_t kind, tuning_result_code_t code)
{
    s_event.kind = kind;
    s_event.code = code;
    s_event_pending = 1;
}

/**
 * @brief 固定复制活动共享参数和方向监视值。
 * @param[out] values 非空输出，所有八项均写入。
 * @note 无历史推进；调用方保持控制拍一致性。
 */
void Tuning_ReadActive(tuning_values_t *values)
{
    const pid_state_t *p = PID_GetState();
    const motor_state_t *m = Motor_GetState();
    *values = (tuning_values_t)
    {
        p->kp_x, p->k2p_x, p->kd_x, p->kp_v, p->ki_v,
        m->base_speed, m->fan_duty, m->fan_duty_idle
    };
}

/**
 * @brief 应用共享速度PI和风扇，不改变历史或方向策略。
 * @details 四项共享写入在同一控制拍完成；不调用任何PID复位。
 * @param[in] values 非空完整合法值。
 * @note TIM6或停车调用；PWM仍由原App位置写出。
 */
static void Tuning_ApplyShared(const tuning_values_t *values)
{
    PID_SetGain(PID_GAIN_KP_V, values->kp_v);
    PID_SetGain(PID_GAIN_KI_V, values->ki_v);
    Motor_SetFanDuty((int16_t)values->fan_run);
    Motor_SetFanDutyIdle((int16_t)values->fan_idle);
}

/**
 * @brief 比较八项实际值，避免填充字节影响版本判定。
 * @details 按字段数值比较，零符号不构成调参暂停/恢复的实际变化。
 * @param[in] a 非空值。
 * @param[in] b 非空值。
 * @return 全部数值相等返回1，正负零视为同值。
 * @note 不对结构体做字节比较。
 */
static uint8_t Tuning_Equal(const tuning_values_t *a, const tuning_values_t *b)
{
    return a->kp == b->kp && a->k2p == b->k2p && a->kd == b->kd
        && a->kp_v == b->kp_v && a->ki_v == b->ki_v
        && a->base_speed == b->base_speed && a->fan_run == b->fan_run
        && a->fan_idle == b->fan_idle;
}

/**
 * @brief 终结未生效请求并保留独立最终邮箱。
 * @details 有pending才占用最终邮箱；已有最终结果不会被空取消操作覆盖。
 * @param[in] status 本次拒绝原因。
 * @note 调用方持控制所有权或短临界区；邮箱未排空期间不能接收下一SET。
 */
static void Tuning_RejectPending(tuning_result_code_t status)
{
    if (s_pending_id)
    {
        s_final.id = s_pending_id;
        s_final.code = status;
        s_pending_id = 0;
        s_staged = 0;
    }
}

/**
 * @brief 取得选中模式的本拍调试副本。
 * @param[out] values 非空完整副本。
 * @return 本拍允许覆盖1，否则0且副本仍为当前effective。
 * @note TIM6选择后执行前调用，不推进任何状态。
 */
uint8_t Tuning_GetControl(tuning_values_t *values)
{
    *values = s_tuning.effective;
    return s_tuning.overridden;
}

/**
 * @brief 关闭调试并取消尚未生效的请求。
 * @details 立即恢复共享PI/风扇，方向不再覆盖；保留最后成功值等待停车前台合入。
 * @note 原停车入口调用，无Flash，不增加停车条件或重置控制历史。
 */
void Tuning_Stop(void)
{
    uint32_t mask = BSP_Time_EnterCritical();
    Tuning_RejectPending(TUNING_BUSY);
    if (s_tuning.overridden)
    {
        Tuning_ApplyShared(&s_baseline);
    }
    s_tuning.enabled = s_tuning.overridden = 0;
    s_tuning.phase = TUNING_DISABLED;
    s_transition = 0;
    BSP_Time_ExitCritical(mask);
}

/**
 * @brief 复制完整调参状态。
 * @details 保存进入前PRIMASK，在短临界区内复制effective和全部事务标志，再恢复原屏蔽值；
 *     不把该副本作为新owner，也不推进pending或final。
 * @param[out] status 非空输出。
 * @note 前台/ISR可调用，仅固定长度短临界区，不暴露可写指针。
 */
void Tuning_GetStatus(tuning_status_t *status)
{
    uint32_t mask = BSP_Time_EnterCritical();
    *status = s_tuning;
    BSP_Time_ExitCritical(mask);
}

/**
 * @brief 选择同拍最终参数并应用共享PI/风扇。
 * @details 状态选择已完成，先复核待应用项，再选择成功快照或原方向/共享基线。
 *     仅本拍执行副本接收覆盖，元素私有策略不被修改。
 * @param[in] ordinary 新电磁状态是否普通。
 * @param[in] allowed 是否已越过等待且仍起跑。
 * @param[in] original 非空原策略方向增益。
 * @param[in] base_speed 原策略基础速度，脉冲/2ms。
 * @param[in] context 非空同拍业务输入，读取模式、档位、启动许可和累计保护位。
 * @note TIM6每完整拍一次，不重置PID历史，不在此格式化结果或写Flash。
 */
void Tuning_SelectControl(uint8_t ordinary, uint8_t allowed,
    const pid_track_gains_t *original, int16_t base_speed, const tuning_context_t *context)
{
    tuning_values_t next;
    uint8_t was = s_tuning.overridden;
    uint8_t matching = s_owner_mode == context->mode
        && s_owner_gear == context->gear;
    uint8_t use = s_tuning.enabled && matching && ordinary && allowed;
    /* 新元素状态已确定；先复核pending目标、启动和保护，拒绝仅进入独立final邮箱。 */
    s_staged = s_transition = 0;
    if (s_pending_id)
    {
        tuning_result_code_t error = !ordinary ? TUNING_SPECIAL_ELEMENT : TUNING_BUSY;
        if (use && context->started && !context->protection_flags
            && s_pending.mode == context->mode
            && s_pending.gear == context->gear
            && s_pending.values.fan_idle == Motor_GetState()->fan_duty_idle)
        {
            s_staged = 1;
        }
        else
        {
            Tuning_RejectPending(error);
        }
    }
    /* 从原方向策略构造本拍执行副本，共享量按覆盖历史恢复baseline；不反写成功值。 */
    Tuning_ReadActive(&next);
    next.kp = original->kp;
    next.k2p = original->k2p;
    next.kd = original->kd;
    next.base_speed = base_speed;
    if (was || (s_tuning.enabled && matching))
    {
        next.kp_v = s_baseline.kp_v;
        next.ki_v = s_baseline.ki_v;
        next.fan_run = s_baseline.fan_run;
        next.fan_idle = s_baseline.fan_idle;
    }
    s_tuning.overridden = use && (s_staged || (s_has_success
        && s_last_mode == s_owner_mode && s_last_gear == s_owner_gear));
    if (s_tuning.overridden)
    {
        next = s_staged ? s_pending.values : s_last_success;
    }
    s_tuning.phase = !s_tuning.enabled ? TUNING_DISABLED
        : (!ordinary ? TUNING_SUSPENDED
        : (s_tuning.overridden ? TUNING_ACTIVE : TUNING_ARMED));
    /* 真正变化才推进版本；运行路径跳过零，事件延后到控制完成，版本先在同ISR同步。 */
    if (s_staged || ((was || s_tuning.overridden)
        && !Tuning_Equal(&next, &s_tuning.effective)))
    {
        ++s_tuning.revision;
        if (!s_tuning.revision)
        {
            ++s_tuning.revision;
        }
        s_revision_pending = 1;
        if (!s_staged && was != s_tuning.overridden)
        {
            s_transition = s_tuning.overridden ? 2 : 1;
        }
    }
    s_tuning.effective = next;
    if (was || s_tuning.enabled)
    {
        Tuning_ApplyShared(&next);
    }
}

/**
 * @brief 执行完成后固定APPLIED及切换事件。
 * @details 此时控制序号已增加；成功值独立复制，最终结果不依赖后续元素临时参数。
 * @param[in] context 非空已完成拍输入，成功时固定其中采样tick和完整序号。
 * @note TIM6调用，无Flash/JSON。首次保护停车取消staged，不能在错误路径报APPLIED。
 */
void Tuning_CompleteControl(const tuning_context_t *context)
{
    /* 只有完整拍执行结束才把暂存请求变为成功值及APPLIED，final保持到前台取走。 */
    if (s_staged && s_pending_id)
    {
        s_last_success = s_pending.values;
        s_last_mode = s_pending.mode;
        s_last_gear = s_pending.gear;
        s_has_success = s_dirty = 1;
        s_tuning.save_state = 1;
        s_final.id = s_pending_id;
        s_final.code = TUNING_APPLIED;
        s_final.values = s_last_success;
        s_final.tick = context->sample_tick;
        s_final.sequence = context->control_sequence;
        s_final.revision = s_tuning.revision;
        s_pending_id = s_staged = 0;
        Tuning_Event(TUNING_EVENT_TUNE_APPLIED, TUNING_APPLIED);
    }
    /* 非staged的暂停/恢复与APPLIED互斥，单槽事件在本次调用后立即适配。 */
    if (s_transition)
    {
        Tuning_Event(s_transition == 1 ? TUNING_EVENT_TUNE_SUSPENDED
            : TUNING_EVENT_TUNE_RESUMED, TUNING_OK);
        s_transition = 0;
    }
}

/**
 * @brief 会话变化取消未应用请求而不回滚控制。
 * @details 关闭中断期间清传输所有权，已成功值/有效参数和调试开关不清除。
 * @note 前台持短临界区清pending/最终邮箱，成功快照、enabled及effective保持。
 */
void Tuning_ResetSession(void)
{
    uint32_t mask = BSP_Time_EnterCritical();
    s_pending_id = s_staged = 0;
    memset(&s_final, 0, sizeof(s_final));
    BSP_Time_ExitCritical(mask);
}

/**
 * @brief 上电初始化调参事务。
 * @details 只清调参状态、最终邮箱及pending/成功/脏/暂存/过渡标志，再读取活动值作为effective；
 *     不清baseline、成功值存储或目标历史，不注册通信回调，不在起跑调用。
 * @note 仅App_Init最后调用；不在起跑重置，保留停车预开启，不写Flash。
 */
void Tuning_Init(void)
{
    memset(&s_tuning, 0, sizeof(s_tuning));
    memset(&s_final, 0, sizeof(s_final));
    s_pending_id = 0;
    s_has_success = s_dirty = s_staged = s_transition = 0;
    Tuning_ReadActive(&s_tuning.effective);
    s_revision_pending = s_event_pending = 0;
}

/**
 * @brief 检查SET门禁并暂存运行请求。
 * @details 调用方已在锁外校验数值；锁内保持目标、邮箱、等待和元素检查顺序。
 * @param[in] id 已预留请求编号。
 * @param[in] request 非空完整请求。
 * @param[in] context 原临界区内捕获的业务状态。
 * @param[in] validated 锁外validator结果。
 * @param[out] reply 非空状态与原捕获位置。
 * @note 前台持原短临界区调用，不合入bank。
 */
void Tuning_PrepareSet(uint32_t id, const tuning_request_t *request,
    const tuning_context_t *context, tuning_result_code_t validated, tuning_reply_t *reply)
{
    tuning_result_code_t status = validated;
    uint8_t parked;
    parked = context->stopped;
    if (request->mode > 1 || request->gear > 1)
    {
        status = TUNING_INVALID_ARG;
    }
    if (status == TUNING_OK && (s_pending_id || s_final.id || (!parked
        && (!context->started || context->control_ticks <= APP_RUN_DELAY_COUNT))))
    {
        status = TUNING_BUSY;
    }
    if (status == TUNING_OK && !parked)
    {
        if (request->mode != context->mode
            || request->gear != context->gear
            || request->values.fan_idle != Motor_GetState()->fan_duty_idle)
        {
            status = TUNING_INVALID_ARG;
        }
        else if (!context->ordinary)
        {
            status = TUNING_SPECIAL_ELEMENT;
        }
        else if (!s_tuning.enabled)
        {
            status = TUNING_BUSY;
        }
    }
    if (status == TUNING_OK && !parked)
    {
        s_pending = *request;
        s_pending_id = id;
        status = TUNING_RECEIVED;
    }
    reply->position.tick = context->sample_tick;
    reply->position.control_sequence = context->control_sequence;
    reply->code = status;
    reply->position.revision = s_tuning.revision;
}

/**
 * @brief 合入停车请求后的成功事务。
 * @details system已按原顺序写bank及共享值；独立记录成功目标、有效值和新版本。
 * @param[in] request 非空已通过停车门禁的请求。
 * @param[in,out] reply 保留合入前tick和序号，更新APPLIED和值。
 * @note 停车前台调用后立即同步事件。
 */
void Tuning_CommitParked(const tuning_request_t *request, tuning_reply_t *reply)
{
    s_last_success = request->values;
    s_last_mode = request->mode;
    s_last_gear = request->gear;
    s_has_success = s_dirty = 1;
    s_tuning.save_state = 1;
    Tuning_ReadActive(&s_tuning.effective);
    ++s_tuning.revision;
    s_revision_pending = 1;
    reply->code = TUNING_APPLIED;
    Tuning_Event(TUNING_EVENT_TUNE_APPLIED, reply->code);
    reply->values = request->values;
    reply->position.revision = s_tuning.revision;
}

/**
 * @brief 开启或关闭当前目标覆盖许可。
 * @details 同目标重开沿用基线，停车换目标重捕获；运行禁用在后续拍恢复。
 * @param[in] enable 非零开启，零关闭。
 * @param[in] context 非空锁内状态。
 * @param[out] reply 非空状态及位置。
 * @note 调用方持原临界区，不格式化或Flash。
 */
void Tuning_Switch(uint8_t enable, const tuning_context_t *context, tuning_reply_t *reply)
{
    tuning_result_code_t status = TUNING_OK;
    if (enable && !context->stopped && !context->started)
    {
        status = TUNING_BUSY;
    }
    else if (enable && context->started && !context->ordinary)
    {
        status = TUNING_SPECIAL_ELEMENT;
    }
    else if (enable && s_tuning.enabled && !context->stopped
        && (s_owner_mode != context->mode || s_owner_gear != context->gear))
    {
        status = TUNING_BUSY;
    }
    else if (enable && (!s_tuning.enabled || (context->stopped
        && (s_owner_mode != context->mode
        || s_owner_gear != context->gear))))
    {
        /* 同目标禁用尚未到恢复拍便重开时，不能把仍在执行的调试值重新当成基线。 */
        if (!s_tuning.overridden)
        {
            Tuning_ReadActive(&s_baseline);
        }
        s_owner_mode = context->mode;
        s_owner_gear = context->gear;
        s_tuning.enabled = 1;
        s_tuning.phase = TUNING_ARMED;
    }
    else if (!enable)
    {
        Tuning_RejectPending(TUNING_BUSY);
        s_tuning.enabled = 0;
        s_tuning.phase = TUNING_DISABLED;
        if (context->stopped && s_tuning.overridden)
        {
            Tuning_ApplyShared(&s_baseline);
            s_tuning.overridden = 0;
        }
    }
    reply->position.tick = context->sample_tick;
    reply->position.control_sequence = context->control_sequence;
    reply->position.revision = s_tuning.revision;
    reply->code = status;
}

/**
 * @brief 读取当前参数版本。
 * @return 当前uint32版本，允许既定路径回绕。
 */
uint32_t Tuning_GetRevision(void)
{
    return s_tuning.revision;
}

/**
 * @brief 取走独立最终结果邮箱。
 * @details 复制后只清id；编码与保存留前台锁外。
 * @param[out] reply 非空结果输出。
 * @param[out] id 非空请求编号输出。
 * @return 有结果1，无结果0。
 */
uint8_t Tuning_TakeResult(tuning_reply_t *reply, uint32_t *id)
{
    uint32_t mask = BSP_Time_EnterCritical();
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
    return 1;
}

/**
 * @brief 复制尚待保存的成功值及目标。
 * @details 无脏时不写输出；只复制，不清脏或保存。
 * @param[out] request 非空目标及成功值输出。
 * @return 有待保存成功值1，否则0。
 */
uint8_t Tuning_CopyUnsaved(tuning_request_t *request)
{
    if (!s_dirty)
    {
        return 0;
    }
    request->mode = s_last_mode;
    request->gear = s_last_gear;
    request->values = s_last_success;
    return 1;
}

/**
 * @brief 记录一次前台保存尝试结果。
 * @details 成功或失败都清事务dirty，避免对不可用Flash重复尝试。
 * @param[in] saved 非零成功，零失败。
 */
void Tuning_CompleteSave(uint8_t saved)
{
    s_dirty = 0;
    s_tuning.save_state = saved ? 2 : 3;
    Tuning_Event(saved ? TUNING_EVENT_PARAM_SAVED : TUNING_EVENT_PARAM_SAVE_FAILED,
        saved ? TUNING_SAVED : TUNING_IO_ERROR);
}

/**
 * @brief 在原事件发生上下文读取版本和值。
 * @details 先读取revision；保存事件取成功值，其余取effective，不提前冻结。
 * @param[in] kind 待适配事件种类。
 * @param[out] revision 非空版本输出。
 * @param[out] values 非空完整值输出。
 */
void Tuning_CopyEventValues(tuning_event_kind_t kind, uint32_t *revision, tuning_values_t *values)
{
    *revision = s_tuning.revision;
    *values = (kind == TUNING_EVENT_PARAM_SAVED || kind == TUNING_EVENT_PARAM_SAVE_FAILED)
        ? s_last_success : s_tuning.effective;
}

/**
 * @brief 取出当前调用待发布版本。
 * @details 取出后清发布位，调用方在原记录发布前适配上传。
 * @param[out] revision 非空输出，无待发布时保持。
 * @return 有待发布版本1，否则0。
 */
uint8_t Tuning_TakeRevision(uint32_t *revision)
{
    if (!s_revision_pending)
    {
        return 0;
    }
    *revision = s_tuning.revision;
    s_revision_pending = 0;
    return 1;
}

/**
 * @brief 取出当前调用产生的唯一事件。
 * @details 取出后清槽；final邮箱与该即时描述互相独立。
 * @param[out] event 非空输出，无事件时保持。
 * @return 有事件1，否则0。
 */
uint8_t Tuning_TakeEvent(tuning_event_t *event)
{
    if (!s_event_pending)
    {
        return 0;
    }
    *event = s_event;
    s_event_pending = 0;
    return 1;
}
