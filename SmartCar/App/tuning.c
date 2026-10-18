/**
 * @file tuning.c
 * @brief SCB业务分发、完整控制拍调参及停车持久化。
 * @details 前台拥有JSON/Flash，TIM6拥有待应用快照；所有跨上下文发布使用短临界区。
 *     baseline和last_success互相独立，元素配置只作为effective来源，不能反写成功参数。
 */
#include "bsp.h"
#include "app.h"
#include "app_config.h"
#include "control.h"
#include "electromagnetic_fsm.h"
#include "optical_fsm.h"
#include "roundabout.h"
#include "motor.h"
#include "param.h"
#include "datalog.h"
#include "scb_link.h"
#include "telemetry.h"
#include "tuning.h"
#include <string.h>

typedef struct
{
    uint32_t id; /**< 已预留请求编号。 */
    scb_status_t status; /**< 最终业务状态。 */
    scb_values_t values; /**< 实际生效值。 */
    uint32_t tick; /**< 提交采样tick。 */
    uint32_t sequence; /**< 完整控制序号。 */
    uint32_t revision; /**< 提交版本。 */
} tuning_result_t;

/* 控制拍持有effective/成功值/待应用状态，前台接收与查询均在短临界区复制。 */
static tuning_status_t s_tuning;
static scb_values_t s_baseline;
static scb_values_t s_last_success;
static scb_command_t s_pending;
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
/* 仅前台使用的编码和缓存工作区，不放控制栈。 */
static uint8_t s_json[SCB_PAYLOAD_MAX];
static uint8_t s_cached[SCB_PAYLOAD_MAX + 1u];
static scb_json_t s_query_json;
static datalog_meta_t s_log_meta; /**< 前台独占缓存头副本，避免256B进入查询栈。 */

/**
 * @brief 读取当前电磁观察是否处于普通赛道。
 * @return 主状态普通且环岛NORMAL返回1。
 * @note 调用方在前台需持短临界区，TIM6选择阶段可直接读取。
 */
static uint8_t Tuning_IsOrdinary(void)
{
    const electromagnetic_state_t *e = ElectromagneticFSM_GetState();
    return e->kernel_state == CONTROL_KERNEL_TRACKING
        && e->roundabout_state == ROUNDABOUT_STATE_NORMAL;
}

/**
 * @brief 固定复制活动共享参数和方向监视值。
 * @param[out] values 非空输出，所有八项均写入。
 * @note 无历史推进；调用方保持控制拍一致性。
 */
static void Tuning_ReadActive(scb_values_t *values)
{
    const pid_state_t *p = PID_GetState();
    const motor_state_t *m = Motor_GetState();
    *values = (scb_values_t)
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
static void Tuning_ApplyShared(const scb_values_t *values)
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
static uint8_t Tuning_Equal(const scb_values_t *a, const scb_values_t *b)
{
    return a->kp == b->kp && a->k2p == b->k2p && a->kd == b->kd
        && a->kp_v == b->kp_v && a->ki_v == b->ki_v
        && a->base_speed == b->base_speed && a->fan_run == b->fan_run
        && a->fan_idle == b->fan_idle;
}

/**
 * @brief 将当前有效快照固定为异步事件描述。
 * @details 在发生上下文复制运行/参数身份；事件队列只保留描述，前台才编码。
 * @param[in] kind 公共事件种类。
 * @param[in] status 公共状态。
 * @note 允许ISR，仅复制固定对象；队满由SCB发布gap，不挤占最终结果邮箱。
 */
static void Tuning_Event(scb_event_kind_t kind, scb_status_t status)
{
    telemetry_status_t upload;
    scb_event_t event = { 0 };
    Telemetry_GetStatus(&upload);
    event.kind = kind;
    event.status = status;
    event.run_id = upload.run_id;
    event.tick = BSP_Sampler_GetTick();
    event.control_sequence = Tracking_GetState()->control_sequence;
    event.revision = s_tuning.revision;
    event.has_values = 1;
    event.values = (kind == SCB_EVENT_PARAM_SAVED || kind == SCB_EVENT_PARAM_SAVE_FAILED)
        ? s_last_success : s_tuning.effective;
    (void)SCB_QueueEvent(&event);
}

/**
 * @brief 初始化前台结果的固定JSON前缀。
 * @details 清writer绑定固定容量缓冲，操作名只来自解析后的已知枚举字符串。
 * @param[out] writer 非空writer，绑定静态s_json。
 * @param[in] op 固定合法操作名，无转义字符。
 * @param[in] status 公共状态枚举。
 * @note 前台独占；调用方添加字段后以Tuning_Send结束对象。
 */
static void Tuning_Begin(scb_writer_t *writer, const char *op, scb_status_t status)
{
    SCB_JsonBegin(writer, s_json, sizeof(s_json));
    SCB_JsonAppend(writer, "{\"op\":\"");
    SCB_JsonAppend(writer, op);
    SCB_JsonAppend(writer, "\",\"status\":\"");
    SCB_JsonAppend(writer, SCB_StatusName(status));
    SCB_JsonAppend(writer, "\"");
}

/**
 * @brief 结束JSON并更新已预留缓存。
 * @details 整条成功才交给可靠层；溢出以错误对象取代，不能发送部分JSON。
 * @param[in] id 非零已预留请求。
 * @param[in,out] writer 前台writer。
 * @param[in] final 零保留请求未完成，否则允许最终ACK后淘汰。
 * @note 溢出不发送截断内容，改为有界IO_ERROR；本模块所有成功形状须由宿主验长度。
 */
static void Tuning_Send(uint32_t id, scb_writer_t *writer, uint8_t final)
{
    SCB_JsonAppend(writer, "}");
    if (writer->failed)
    {
        Tuning_Begin(writer, "GET_REQUEST_RESULT", SCB_IO_ERROR);
        SCB_JsonAppend(writer, "}");
        final = 1;
    }
    (void)SCB_QueueResult(id, SCB_RESULT, s_json, writer->length, final);
}

/**
 * @brief 附加实际发生位置和版本。
 * @details 使用发生处已捕获的三个标量，不从后续控制状态重建时间。
 * @param[in,out] writer 非空前台writer。
 * @param[in] tick 采样tick。
 * @param[in] sequence 完整控制拍。
 * @param[in] revision 有效版本。
 * @note 不重新读取可能已经改变的活动对象。
 */
static void Tuning_Position(scb_writer_t *writer, uint32_t tick,
    uint32_t sequence, uint32_t revision)
{
    SCB_JsonAppend(writer, ",\"tick\":");
    SCB_JsonU32(writer, tick);
    SCB_JsonAppend(writer, ",\"control_sequence\":");
    SCB_JsonU32(writer, sequence);
    SCB_JsonAppend(writer, ",\"revision\":");
    SCB_JsonU32(writer, revision);
}

/**
 * @brief 将成功调试值写入显式参数bank，不从活动元素推导。
 * @details 方向写目标mode/gear，共享PI及档位风扇分发到原镜像，暂不写Flash。
 * @param[in] mode 合法目标模式。
 * @param[in] gear 合法目标档位。
 * @param[in] values 非空完整合法值。
 * @note 仅停车前台，无Flash；原88B镜像由原保存函数写出。
 */
static void Tuning_Merge(uint8_t mode, uint8_t gear, const scb_values_t *values)
{
    param_direction_t direction =
    {
        values->kp, values->k2p, values->kd, (int16_t)values->base_speed
    };
    (void)Param_SetDirection(mode, gear, &direction);
    Param_SetSpeedGains(values->kp_v, values->ki_v);
    Param_SetFan(gear, 0, (int16_t)values->fan_run);
    Param_SetFan(gear, 1, (int16_t)values->fan_idle);
}

/**
 * @brief 终结未生效请求并保留独立最终邮箱。
 * @details 有pending才占用最终邮箱；已有最终结果不会被空取消操作覆盖。
 * @param[in] status 本次拒绝原因。
 * @note 调用方持控制所有权或短临界区；邮箱未排空期间不能接收下一SET。
 */
static void Tuning_RejectPending(scb_status_t status)
{
    if (s_pending_id)
    {
        s_final.id = s_pending_id;
        s_final.status = status;
        s_pending_id = 0;
        s_staged = 0;
    }
}

/**
 * @brief 选择同拍最终参数并应用共享PI/风扇。
 * @details 状态选择已完成，先复核待应用项，再选择成功快照或原方向/共享基线。
 *     仅本拍执行副本接收覆盖，元素私有策略不被修改。
 * @param[in] ordinary 新电磁状态是否普通。
 * @param[in] allowed 是否已越过等待且仍起跑。
 * @param[in] original 非空原策略方向增益。
 * @param[in] base_speed 原策略基础速度，脉冲/2ms。
 * @note TIM6每完整拍一次，不重置PID历史，不在此格式化结果或写Flash。
 */
void Tuning_SelectControl(uint8_t ordinary, uint8_t allowed,
    const pid_track_gains_t *original, int16_t base_speed)
{
    scb_values_t next;
    uint8_t was = s_tuning.overridden;
    uint8_t matching = s_owner_mode == Tracking_GetState()->mode
        && s_owner_gear == Tracking_GetState()->gear;
    uint8_t use = s_tuning.enabled && matching && ordinary && allowed;
    s_staged = s_transition = 0;
    if (s_pending_id)
    {
        scb_status_t error = !ordinary ? SCB_SPECIAL_ELEMENT : SCB_BUSY;
        if (use && g_app_start_flag && !Tracking_GetState()->protection_flags
            && s_pending.mode == Tracking_GetState()->mode
            && s_pending.gear == Tracking_GetState()->gear
            && s_pending.values.fan_idle == Motor_GetState()->fan_duty_idle)
        {
            s_staged = 1;
        }
        else
        {
            Tuning_RejectPending(error);
        }
    }
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
    if (s_staged || ((was || s_tuning.overridden)
        && !Tuning_Equal(&next, &s_tuning.effective)))
    {
        ++s_tuning.revision;
        if (!s_tuning.revision)
        {
            ++s_tuning.revision;
        }
        Telemetry_SetRevision(s_tuning.revision);
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
 * @brief 取得选中模式的本拍调试副本。
 * @param[out] values 非空完整副本。
 * @return 本拍允许覆盖1，否则0且副本仍为当前effective。
 * @note TIM6选择后执行前调用，不推进任何状态。
 */
uint8_t Tuning_GetControl(scb_values_t *values)
{
    *values = s_tuning.effective;
    return s_tuning.overridden;
}

/**
 * @brief 执行完成后固定APPLIED及切换事件。
 * @details 此时控制序号已增加；成功值独立复制，最终结果不依赖后续元素临时参数。
 * @note TIM6调用，无Flash/JSON。首次保护停车取消staged，不能在错误路径报APPLIED。
 */
void Tuning_CompleteControl(void)
{
    if (s_staged && s_pending_id)
    {
        s_last_success = s_pending.values;
        s_last_mode = s_pending.mode;
        s_last_gear = s_pending.gear;
        s_has_success = s_dirty = 1;
        s_tuning.save_state = 1;
        s_final.id = s_pending_id;
        s_final.status = SCB_APPLIED;
        s_final.values = s_last_success;
        s_final.tick = BSP_Sampler_GetTick();
        s_final.sequence = Tracking_GetState()->control_sequence;
        s_final.revision = s_tuning.revision;
        s_pending_id = s_staged = 0;
        Tuning_Event(SCB_EVENT_TUNE_APPLIED, SCB_APPLIED);
    }
    if (s_transition)
    {
        Tuning_Event(s_transition == 1 ? SCB_EVENT_TUNE_SUSPENDED
            : SCB_EVENT_TUNE_RESUMED, SCB_OK);
        s_transition = 0;
    }
}

/**
 * @brief 关闭调试并取消尚未生效的请求。
 * @details 立即恢复共享PI/风扇，方向不再覆盖；保留最后成功值等待停车前台合入。
 * @note 原停车入口调用，无Flash，不增加停车条件或重置控制历史。
 */
void Tuning_Stop(void)
{
    uint32_t mask = BSP_Time_EnterCritical();
    Tuning_RejectPending(SCB_BUSY);
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
 * @brief 日志收尾后把最后成功值合入其明确目标。
 * @details 只在有成功且尚未保存的快照时合入，元素临时值不参与持久化。
 * @note 停车前台调用，无成功调参不改bank；合入仅标脏，Flash由Service执行。
 */
void Tuning_FinalizeStop(void)
{
    if (s_dirty)
    {
        Tuning_Merge(s_last_mode, s_last_gear, &s_last_success);
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
    Datalog_CancelExport();
}

/**
 * @brief 复制完整调参状态。
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
 * @brief 前台输出最终结果并在停车时尝试一次保存。
 * @details 邮箱复制并清空在同一短临界区；编码及原Flash写均在中断开放后执行。
 * @note 运行态只服务结果；保存失败锁存原Flash不可用，不反复重试或伪报成功。
 */
void Tuning_Service(void)
{
    tuning_result_t result;
    scb_writer_t writer;
    Datalog_ServiceExport();
    uint32_t mask = BSP_Time_EnterCritical();
    result = s_final;
    s_final.id = 0;
    BSP_Time_ExitCritical(mask);
    if (result.id)
    {
        Tuning_Begin(&writer, "SET_PARAMS", result.status);
        if (result.status == SCB_APPLIED)
        {
            SCB_JsonAppend(&writer, ",\"values\":");
            SCB_JsonValues(&writer, &result.values);
            Tuning_Position(&writer, result.tick, result.sequence, result.revision);
        }
        Tuning_Send(result.id, &writer, 1);
    }
    if (App_IsStopped() && s_dirty)
    {
        uint8_t saved = Param_TryFlushPending();
        s_dirty = 0;
        s_tuning.save_state = saved ? 2 : 3;
        Tuning_Event(saved ? SCB_EVENT_PARAM_SAVED : SCB_EVENT_PARAM_SAVE_FAILED,
            saved ? SCB_SAVED : SCB_IO_ERROR);
    }
}

/**
 * @brief 取得目标bank的完整存储值。
 * @param[in] mode 合法模式。
 * @param[in] gear 合法档位。
 * @param[out] values 非空八项副本。
 * @note 仅前台；不把运行调试值回收到存储查询。
 */
static void Tuning_ReadBank(uint8_t mode, uint8_t gear, scb_values_t *values)
{
    const param_direction_t *p = Param_GetDirection(mode, gear);
    values->kp = p->kp;
    values->k2p = p->k2p;
    values->kd = p->kd;
    values->base_speed = p->base_speed;
    Param_GetSpeedGains(&values->kp_v, &values->ki_v);
    values->fan_run = Param_GetFan(gear, 0);
    values->fan_idle = Param_GetFan(gear, 1);
}

/**
 * @brief 输出完整bank、当前有效快照和同源限值。
 * @details 先复制存储/有效对象，再逐字段编码八组min/max，不在临界区格式化。
 * @param[in] id 已预留请求。
 * @param[in] command 合法GET_PARAMS目标。
 * @note 先短临界区复制，格式化不屏蔽中断。
 */
static void Tuning_QueryParams(uint32_t id, const scb_command_t *command)
{
    static const char *const s_keys[] =
    {
        "kp", "k2p", "kd", "kp_v", "ki_v", "base_speed", "fan_run", "fan_idle"
    };
    scb_values_t values;
    tuning_status_t status;
    scb_writer_t writer;
    uint8_t i;
    uint32_t mask = BSP_Time_EnterCritical();
    Tuning_ReadBank(command->mode, command->gear, &values);
    status = s_tuning;
    if (App_IsStopped())
    {
        Tuning_ReadActive(&status.effective);
    }
    BSP_Time_ExitCritical(mask);
    Tuning_Begin(&writer, command->op, SCB_OK);
    SCB_JsonAppend(&writer, ",\"values\":");
    SCB_JsonValues(&writer, &values);
    SCB_JsonAppend(&writer, ",\"effective\":");
    SCB_JsonValues(&writer, &status.effective);
    SCB_JsonAppend(&writer, ",\"overridden\":");
    SCB_JsonAppend(&writer, status.overridden ? "true" : "false");
    SCB_JsonAppend(&writer, ",\"limits\":{");
    for (i = 0; i < 8; ++i)
    {
        SCB_JsonAppend(&writer, i ? ",\"" : "\"");
        SCB_JsonAppend(&writer, s_keys[i]);
        SCB_JsonAppend(&writer, "\":{\"min\":");
        SCB_JsonI32(&writer, i < 5 ? TUNING_GAIN_MIN
            : (i == 5 ? TUNING_SPEED_MIN : TUNING_FAN_MIN));
        SCB_JsonAppend(&writer, ",\"max\":");
        SCB_JsonI32(&writer, i < 5 ? TUNING_GAIN_MAX
            : (i == 5 ? TUNING_SPEED_MAX : TUNING_FAN_MAX));
        SCB_JsonAppend(&writer, "}");
    }
    SCB_JsonAppend(&writer, "}");
    Tuning_Send(id, &writer, 1);
}

/**
 * @brief 输出一致运行/调参/上传/日志摘要。
 * @details 在短临界区固定两套元素及运行版本，再用有界writer构造前台结果。
 * @param[in] id 已预留请求。
 * @note 只复制内存状态，日志有效位来自已确认缓存，不在查询时扫描Flash。
 */
static void Tuning_QueryStatus(uint32_t id)
{
    static const char *const s_phases[] = { "DISABLED", "ARMED", "ACTIVE", "SUSPENDED" };
    static const char *const s_saves[] = { "idle", "pending", "saved", "failed" };
    tuning_status_t tune;
    telemetry_status_t upload;
    scb_writer_t writer;
    tracking_state_t track;
    uint8_t kernel;
    uint8_t round;
    uint8_t optical;
    uint8_t running;
    uint32_t tick;
    uint32_t written;
    uint32_t mask = BSP_Time_EnterCritical();
    tune = s_tuning;
    track = *Tracking_GetState();
    kernel = ElectromagneticFSM_GetState()->kernel_state;
    round = ElectromagneticFSM_GetState()->roundabout_state;
    optical = (uint8_t)OpticalFSM_GetState()->state;
    running = g_app_start_flag;
    tick = BSP_Sampler_GetTick();
    written = Datalog_GetCount();
    Telemetry_GetStatus(&upload);
    Datalog_GetMeta(&s_log_meta);
    BSP_Time_ExitCritical(mask);
    Tuning_Begin(&writer, "GET_STATUS", SCB_OK);
    SCB_JsonAppend(&writer, ",\"running\":");
    SCB_JsonAppend(&writer, running ? "true" : "false");
    SCB_JsonAppend(&writer, ",\"mode\":");
    SCB_JsonU32(&writer, track.mode);
    SCB_JsonAppend(&writer, ",\"gear\":");
    SCB_JsonU32(&writer, track.gear);
    SCB_JsonAppend(&writer, ",\"kernel_state\":");
    SCB_JsonU32(&writer, kernel);
    SCB_JsonAppend(&writer, ",\"roundabout_state\":");
    SCB_JsonU32(&writer, round);
    SCB_JsonAppend(&writer, ",\"optical_state\":");
    SCB_JsonU32(&writer, optical);
    SCB_JsonAppend(&writer, ",\"tune_state\":\"");
    SCB_JsonAppend(&writer, s_phases[tune.phase]);
    SCB_JsonAppend(&writer, "\",\"save_state\":\"");
    SCB_JsonAppend(&writer, s_saves[tune.save_state]);
    SCB_JsonAppend(&writer, "\",\"run_id\":");
    SCB_JsonU32(&writer, upload.run_id);
    SCB_JsonAppend(&writer, ",\"dropped_records\":");
    SCB_JsonU32(&writer, upload.dropped);
    SCB_JsonAppend(&writer, ",\"written_records\":");
    SCB_JsonU32(&writer, written);
    SCB_JsonAppend(&writer, ",\"log_valid\":");
    SCB_JsonAppend(&writer, s_log_meta.valid ? "true" : "false");
    SCB_JsonAppend(&writer, ",\"export_active\":");
    SCB_JsonAppend(&writer, Datalog_IsExporting() ? "true" : "false");
    Tuning_Position(&writer, tick, track.control_sequence, tune.revision);
    Tuning_Send(id, &writer, 1);
}

/**
 * @brief 查询直接业务结果，避免查询自身产生递归包装。
 * @details 命中复制原缓存；JSON包装展开一层自身结果，二进制块返回独立定位信息。
 * @param[in] id 已预留查询请求。
 * @param[in] target 被查询缓存编号。
 * @note JSON保留完整对象；LOG_DATA仅回块定位信息，原始块由原LOG_READ去重重取。
 */
static void Tuning_QueryResult(uint32_t id, uint32_t target)
{
    uint8_t type;
    uint16_t length;
    scb_writer_t writer;
    if (!SCB_GetRequestResult(target, &type, s_cached, &length))
    {
        Tuning_Begin(&writer, "GET_REQUEST_RESULT", SCB_NOT_FOUND);
    }
    else
    {
        Tuning_Begin(&writer, "GET_REQUEST_RESULT", SCB_OK);
        SCB_JsonAppend(&writer, ",\"target_request_id\":");
        SCB_JsonU32(&writer, target);
        if (type == SCB_LOG_DATA && length >= 8)
        {
            SCB_JsonAppend(&writer, ",\"offset\":");
            SCB_JsonU32(&writer, SCB_ReadU32(s_cached));
            SCB_JsonAppend(&writer, ",\"length\":");
            SCB_JsonU32(&writer, SCB_ReadU16(s_cached + 4));
        }
        else if (type == SCB_RESULT && SCB_ParseJson(s_cached, length, &s_query_json))
        {
            const char *op = SCB_GetString(&s_query_json,
                SCB_FindMember(&s_query_json, 0, "op"));
            char *result;
            s_cached[length] = 0;
            /* 仅展开本模块生成的查询包装；result始终最后一项且原对象完整。 */
            result = op && !strcmp(op, "GET_REQUEST_RESULT")
                ? strstr((char *)s_cached, ",\"result\":") : 0;
            if (result)
            {
                s_cached[length - 1u] = 0;
                result += 10;
            }
            SCB_JsonAppend(&writer, ",\"result\":");
            SCB_JsonAppend(&writer, result ? result : (char *)s_cached);
        }
        else
        {
            Tuning_Begin(&writer, "GET_REQUEST_RESULT", SCB_IO_ERROR);
        }
    }
    Tuning_Send(id, &writer, 1);
}

/**
 * @brief 校验并接收完整设置，运行态只发布一个待应用快照。
 * @param[in] id 已完成缓存容量预留的非零编号。
 * @param[in] command 完整设置，调用方仍由本函数二次校验。
 * @note 前台；停车配置先APPLIED后Service保存，所有失败不修改任一参数。
 */
static void Tuning_Set(uint32_t id, const scb_command_t *command)
{
    scb_status_t status = SCB_ValidateValues(&command->values);
    scb_writer_t writer;
    uint8_t parked;
    uint32_t tick;
    uint32_t sequence;
    uint32_t revision;
    uint32_t mask = BSP_Time_EnterCritical();
    parked = App_IsStopped();
    if (command->mode > 1 || command->gear > 1)
    {
        status = SCB_INVALID_ARG;
    }
    if (status == SCB_OK && (s_pending_id || s_final.id || (!parked
        && (!g_app_start_flag || g_app_control_ticks <= APP_RUN_DELAY_COUNT))))
    {
        status = SCB_BUSY;
    }
    if (status == SCB_OK && !parked)
    {
        if (command->mode != Tracking_GetState()->mode
            || command->gear != Tracking_GetState()->gear
            || command->values.fan_idle != Motor_GetState()->fan_duty_idle)
        {
            status = SCB_INVALID_ARG;
        }
        else if (!Tuning_IsOrdinary())
        {
            status = SCB_SPECIAL_ELEMENT;
        }
        else if (!s_tuning.enabled)
        {
            status = SCB_BUSY;
        }
    }
    if (status == SCB_OK && !parked)
    {
        s_pending = *command;
        s_pending_id = id;
        status = SCB_RECEIVED;
    }
    tick = BSP_Sampler_GetTick();
    sequence = Tracking_GetState()->control_sequence;
    BSP_Time_ExitCritical(mask);
    if (status == SCB_OK)
    {
        Tuning_Merge(command->mode, command->gear, &command->values);
        s_last_success = command->values;
        s_last_mode = command->mode;
        s_last_gear = command->gear;
        s_has_success = s_dirty = 1;
        s_tuning.save_state = 1;
        Tuning_ReadActive(&s_tuning.effective);
        ++s_tuning.revision;
        Telemetry_SetRevision(s_tuning.revision);
        status = SCB_APPLIED;
        Tuning_Event(SCB_EVENT_TUNE_APPLIED, status);
    }
    revision = s_tuning.revision;
    Tuning_Begin(&writer, command->op, status);
    if (status == SCB_APPLIED)
    {
        SCB_JsonAppend(&writer, ",\"values\":");
        SCB_JsonValues(&writer, &command->values);
        Tuning_Position(&writer, tick, sequence, revision);
    }
    Tuning_Send(id, &writer, status != SCB_RECEIVED);
}

/**
 * @brief 开启或关闭当前目标覆盖。
 * @details 开启只首次捕获基线，重复开启不把调试值变成基线；禁用取消尚未生效请求。
 * @param[in] id 已预留请求编号。
 * @param[in] command TUNE_ENABLE或TUNE_DISABLE。
 * @note 前台只复制基线/改许可；运行恢复在下个完整拍，停车立即恢复共享值。
 */
static void Tuning_Switch(uint32_t id, const scb_command_t *command)
{
    uint8_t enable = !strcmp(command->op, "TUNE_ENABLE");
    scb_status_t status = SCB_OK;
    scb_writer_t writer;
    uint32_t tick;
    uint32_t sequence;
    uint32_t revision;
    uint32_t mask = BSP_Time_EnterCritical();
    if (enable && !App_IsStopped() && !g_app_start_flag)
    {
        status = SCB_BUSY;
    }
    else if (enable && g_app_start_flag && !Tuning_IsOrdinary())
    {
        status = SCB_SPECIAL_ELEMENT;
    }
    else if (enable && !s_tuning.enabled)
    {
        Tuning_ReadActive(&s_baseline);
        s_owner_mode = Tracking_GetState()->mode;
        s_owner_gear = Tracking_GetState()->gear;
        s_tuning.enabled = 1;
        s_tuning.phase = TUNING_ARMED;
    }
    else if (!enable)
    {
        Tuning_RejectPending(SCB_BUSY);
        s_tuning.enabled = 0;
        s_tuning.phase = TUNING_DISABLED;
        if (App_IsStopped() && s_tuning.overridden)
        {
            Tuning_ApplyShared(&s_baseline);
            s_tuning.overridden = 0;
        }
    }
    tick = BSP_Sampler_GetTick();
    sequence = Tracking_GetState()->control_sequence;
    revision = s_tuning.revision;
    BSP_Time_ExitCritical(mask);
    Tuning_Begin(&writer, command->op, status);
    Tuning_Position(&writer, tick, sequence, revision);
    Tuning_Send(id, &writer, 1);
}

/**
 * @brief 输出内存元信息和完整512位头文本。
 * @details 先复制已确认缓存，再逐字节转小写十六进制；未建会话输出空串，不扫描Flash。
 * @param[in] id 已预留请求编号。
 * @note 前台独占静态meta及JSON工作区，不修改日志和导出租期。
 */
static void Tuning_QueryLogMeta(uint32_t id)
{
    static const char s_hex[] = "0123456789abcdef";
    scb_writer_t writer;
    uint16_t i;
    Datalog_GetMeta(&s_log_meta);
    Tuning_Begin(&writer, "GET_LOG_META", SCB_OK);
    SCB_JsonAppend(&writer, ",\"log_valid\":");
    SCB_JsonAppend(&writer, s_log_meta.valid ? "true" : "false");
    SCB_JsonAppend(&writer, ",\"written_records\":");
    SCB_JsonU32(&writer, s_log_meta.written_records);
    SCB_JsonAppend(&writer, ",\"header_hex\":\"");
    for (i = 0; s_log_meta.valid && i < DATALOG_HEADER_SIZE; ++i)
    {
        char hex[3];
        hex[0] = s_hex[s_log_meta.header[i] >> 4];
        hex[1] = s_hex[s_log_meta.header[i] & 15u];
        hex[2] = 0;
        SCB_JsonAppend(&writer, hex);
    }
    SCB_JsonAppend(&writer, "\"");
    Tuning_Send(id, &writer, 1);
}

/**
 * @brief 在前台执行单窗口日志导出命令。
 * @details Begin冻结前缀，Read成功直接缓存二进制块，End只有完整交付才释放；
 *     错误使用RESULT公共状态，容量已经由SCB在分发前预留。
 * @param[in] id 已预留请求编号。
 * @param[in] command 非空已校验的LOG_BEGIN/READ/END命令。
 * @note 前台可能进行有界Flash读；不改调试开关或车辆停车，不放整份日志到RAM。
 */
static void Tuning_LogCommand(uint32_t id, const scb_command_t *command)
{
    scb_writer_t writer;
    scb_status_t status;
    datalog_export_status_t export;
    uint32_t total;
    uint32_t crc;
    uint16_t length;
    if (!strcmp(command->op, "LOG_READ"))
    {
        status = Datalog_ReadExport(command->transfer_id, command->offset, command->length,
            s_json, &length);
        if (status == SCB_OK)
        {
            (void)SCB_QueueResult(id, SCB_LOG_DATA, s_json, length, 1);
            return;
        }
    }
    else if (!strcmp(command->op, "LOG_BEGIN"))
    {
        status = Datalog_BeginExport(&export);
    }
    else
    {
        status = Datalog_EndExport(command->transfer_id, &total, &crc);
    }
    Tuning_Begin(&writer, command->op, status);
    if (status == SCB_OK && !strcmp(command->op, "LOG_BEGIN"))
    {
        SCB_JsonAppend(&writer, ",\"transfer_id\":");
        SCB_JsonU32(&writer, export.transfer_id);
        SCB_JsonAppend(&writer, ",\"total_bytes\":");
        SCB_JsonU32(&writer, export.total_bytes);
        SCB_JsonAppend(&writer, ",\"record_count\":");
        SCB_JsonU32(&writer, export.record_count);
        SCB_JsonAppend(&writer, ",\"header_size\":256,\"record_size\":96");
    }
    else if (status == SCB_OK && !strcmp(command->op, "LOG_END"))
    {
        SCB_JsonAppend(&writer, ",\"total_bytes\":");
        SCB_JsonU32(&writer, total);
        SCB_JsonAppend(&writer, ",\"crc32\":");
        SCB_JsonU32(&writer, crc);
    }
    Tuning_Send(id, &writer, 1);
}

/**
 * @brief 分发已被SCB接管并预留最终结果的业务命令。
 * @details 根据完整操作名转给查询/调参路径，未知操作最终INVALID_ARG，不做部分更新。
 * @param[in] id 非零已预留请求。
 * @param[in] command 非空已完整解析命令。
 * @note 前台串行；日志读取同样只停车执行，不在ISR执行JSON或Flash。
 */
void Tuning_HandleCommand(uint32_t id, const scb_command_t *command)
{
    scb_writer_t writer;
    if (!strcmp(command->op, "SET_PARAMS"))
    {
        Tuning_Set(id, command);
    }
    else if (!strcmp(command->op, "TUNE_ENABLE") || !strcmp(command->op, "TUNE_DISABLE"))
    {
        Tuning_Switch(id, command);
    }
    else if (!strcmp(command->op, "GET_PARAMS"))
    {
        Tuning_QueryParams(id, command);
    }
    else if (!strcmp(command->op, "GET_STATUS"))
    {
        Tuning_QueryStatus(id);
    }
    else if (!strcmp(command->op, "GET_REQUEST_RESULT"))
    {
        Tuning_QueryResult(id, command->target_request_id);
    }
    else if (!strcmp(command->op, "GET_LOG_META"))
    {
        Tuning_QueryLogMeta(id);
    }
    else if (!strcmp(command->op, "LOG_BEGIN") || !strcmp(command->op, "LOG_READ")
        || !strcmp(command->op, "LOG_END"))
    {
        Tuning_LogCommand(id, command);
    }
    else
    {
        Tuning_Begin(&writer, command->op, SCB_INVALID_ARG);
        Tuning_Send(id, &writer, 1);
    }
}

/**
 * @brief 上电清静态调参对象并注册SCB业务/会话钩子。
 * @details 初始effective取活动参数，注册在SCB初始化之后，不清参数bank或控制历史。
 * @note 仅App_Init最后调用；不在起跑重置，保留停车预开启，不写Flash。
 */
void Tuning_Init(void)
{
    memset(&s_tuning, 0, sizeof(s_tuning));
    memset(&s_final, 0, sizeof(s_final));
    s_pending_id = 0;
    s_has_success = s_dirty = s_staged = s_transition = 0;
    Tuning_ReadActive(&s_tuning.effective);
    SCB_SetHandlers(Tuning_HandleCommand, Tuning_ResetSession);
}
