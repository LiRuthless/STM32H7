/**
 * @file tuning_application.c
 * @brief 调参上下文与参数保存协调。
 * @details 前台在原临界区捕获业务上下文；TIM6按同拍状态输入，事务产生的版本和事件即时适配。
 */
#include "bsp.h"
#include "app.h"
#include "system/lifecycle.h"
#include "system/tuning_application.h"
#include "system/parameter_application.h"
#include "control/tracking.h"
#include "control/electromagnetic_fsm.h"
#include "services/command_dispatch.h"
#include "services/telemetry.h"

/**
 * @brief 读取当前电磁观察是否处于普通赛道。
 * @return 主状态普通且环岛NORMAL返回1。
 * @note 调用方在前台需持短临界区，TIM6选择阶段可直接读取。
 */
static uint8_t TuningApplication_IsOrdinary(void)
{
    const electromagnetic_state_t *e = ElectromagneticFSM_GetState();
    return e->kernel_state == CONTROL_KERNEL_TRACKING
        && e->roundabout_state == ROUNDABOUT_STATE_NORMAL;
}

/**
 * @brief 捕获本次业务调用的瞬时上下文。
 * @details 不缓存为长期状态，不采集传感器；前台由调用方持原短临界区，TIM6同级不可抢占。
 * @param[out] context 非空上下文输出。
 */
void TuningApplication_CaptureContext(tuning_context_t *context)
{
    context->stopped = App_IsStopped();
    context->started = Lifecycle_IsStarted();
    context->mode = Tracking_GetState()->mode;
    context->gear = Tracking_GetState()->gear;
    context->ordinary = TuningApplication_IsOrdinary();
    context->protection_flags = Tracking_GetState()->protection_flags;
    context->control_ticks = (uint32_t)Lifecycle_GetControlTicks();
    context->sample_tick = BSP_Sampler_GetTick();
    context->control_sequence = Tracking_GetState()->control_sequence;
}

/**
 * @brief 将成功调试值写入显式参数bank，不从活动元素推导。
 * @details 方向写目标mode/gear，共享PI及档位风扇分发到原镜像，暂不写Flash。
 * @param[in] mode 合法目标模式。
 * @param[in] gear 合法目标档位。
 * @param[in] values 非空完整合法值。
 * @note 仅停车前台，无Flash；原88B镜像由原保存函数写出。
 */
static void TuningApplication_Merge(uint8_t mode, uint8_t gear, const tuning_values_t *values)
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
 * @brief 即时适配当前业务调用的版本和事件。
 * @details 先发布上传版本，再交通信适配事件；不得跨记录生产、ISR返回或下一业务调用。
 */
void TuningApplication_Synchronize(void)
{
    uint32_t revision;
    tuning_event_t event;
    if (Tuning_TakeRevision(&revision))
    {
        Telemetry_SetRevision(revision);
    }
    if (Tuning_TakeEvent(&event))
    {
        CommandDispatch_QueueTuningEvent(&event);
    }
}

/**
 * @brief 协调完整设置请求。
 * @details validator在锁外执行，锁内只门禁和pending；停车成功在锁外合入，再取最终版本。
 * @param[in] id 已预留请求编号。
 * @param[in] request 非空完整业务请求。
 * @param[out] reply 非空业务回复。
 * @note 锁外末尾版本读取允许运行完整拍先提交，不能移入锁。
 */
void TuningApplication_Set(uint32_t id, const tuning_request_t *request, tuning_reply_t *reply)
{
    tuning_result_code_t validated = Tuning_ValidateValues(&request->values);
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
    reply->position.revision = Tuning_GetRevision();
}

/**
 * @brief 协调覆盖许可切换。
 * @details 原锁内捕获上下文并修改事务，锁外立即同步。
 * @param[in] enable 非零开启、零关闭。
 * @param[out] reply 非空结果输出。
 */
void TuningApplication_Switch(uint8_t enable, tuning_reply_t *reply)
{
    tuning_context_t context;
    uint32_t mask = BSP_Time_EnterCritical();
    TuningApplication_CaptureContext(&context);
    Tuning_Switch(enable, &context, reply);
    BSP_Time_ExitCritical(mask);
    TuningApplication_Synchronize();
}

/**
 * @brief 日志收尾后合入未保存成功值。
 * @details 只合入明确目标，不清事务dirty，不访问Flash。
 */
void TuningApplication_FinalizeStop(void)
{
    tuning_request_t request;
    if (Tuning_CopyUnsaved(&request))
    {
        TuningApplication_Merge(request.mode, request.gear, &request.values);
    }
}

/**
 * @brief 停车前台尝试保存一次事务参数。
 * @details 停车门禁通过且有成功脏值才尝试；成功失败均完成事务保存状态并即时适配事件。
 */
void TuningApplication_ServiceSave(void)
{
    tuning_request_t request;
    if (App_IsStopped() && Tuning_CopyUnsaved(&request))
    {
        uint8_t saved = Param_TryFlushPending();
        Tuning_CompleteSave(saved);
        TuningApplication_Synchronize();
    }
}
