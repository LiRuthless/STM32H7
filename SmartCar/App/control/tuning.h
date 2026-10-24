/**
 * @file tuning.h
 * @brief 完整拍调参事务与业务状态。
 * @details 事务拥有基线、待应用、成功值、有效值和最终邮箱；前台用短临界区，TIM6独占提交。
 */
#ifndef SMARTCAR_APP_CONTROL_TUNING_H
#define SMARTCAR_APP_CONTROL_TUNING_H
#include "control/tuning_values.h"
#include "control/pid.h"

typedef enum
{
    TUNING_DISABLED, /**< 未开启覆盖。 */
    TUNING_ARMED, /**< 已开启，等待普通完整拍及成功参数。 */
    TUNING_ACTIVE, /**< 当前使用调试快照。 */
    TUNING_SUSPENDED /**< 元素期间使用原策略。 */
} tuning_phase_t;

typedef struct
{
    tuning_values_t effective; /**< 最近完整拍实际值。 */
    uint32_t revision; /**< 有效参数提交版本。 */
    tuning_phase_t phase; /**< 调试生命周期。 */
    uint8_t enabled; /**< 已显式开启。 */
    uint8_t overridden; /**< 本拍使用调试值。 */
    uint8_t save_state; /**< 0空闲、1待保存、2成功、3失败。 */
} tuning_status_t;

typedef struct
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
/**
 * @brief 上电初始化调参事务。
 * @note 完整契约见 tuning.c::Tuning_Init。
 */
void Tuning_Init(void);
/**
 * @brief 读取当前参数版本。
 * @note 完整契约见 tuning.c::Tuning_GetRevision。
 */
uint32_t Tuning_GetRevision(void);
/**
 * @brief 固定复制活动共享参数和方向监视值。
 * @note 完整契约见 tuning.c::Tuning_ReadActive。
 */
void Tuning_ReadActive(tuning_values_t *values);
/**
 * @brief 检查SET门禁并暂存运行请求。
 * @note 调用方持原短临界区；数值校验已在锁外执行，非空请求/上下文/回复。
 *     完整契约见 tuning.c::Tuning_PrepareSet。
 */
void Tuning_PrepareSet(uint32_t id, const tuning_request_t *request,
    const tuning_context_t *context, tuning_result_code_t validated, tuning_reply_t *reply);
/**
 * @brief 合入停车请求后的成功事务。
 * @note 仅PrepareSet停车成功且应用层merge完成后调用；随后立即Synchronize。
 *     完整契约见 tuning.c::Tuning_CommitParked。
 */
void Tuning_CommitParked(const tuning_request_t *request, tuning_reply_t *reply);
/**
 * @brief 开启或关闭当前目标覆盖许可。
 * @note 前台持原短临界区，传入同次捕获的非空上下文及回复。
 *     完整契约见 tuning.c::Tuning_Switch。
 */
void Tuning_Switch(uint8_t enable, const tuning_context_t *context, tuning_reply_t *reply);
/**
 * @brief 选择同拍最终参数并应用共享PI/风扇。
 * @note TIM6选择阶段传入同拍上下文，不推进控制器历史；Observe返回后立即同步版本。
 *     完整契约见 tuning.c::Tuning_SelectControl。
 */
void Tuning_SelectControl(uint8_t ordinary, uint8_t allowed, const pid_track_gains_t *original,
    int16_t base_speed, const tuning_context_t *context);
/**
 * @brief 取得选中模式的本拍调试副本。
 * @note 完整契约见 tuning.c::Tuning_GetControl。
 */
uint8_t Tuning_GetControl(tuning_values_t *values);
/**
 * @brief 执行完成后固定APPLIED及切换事件。
 * @note 仅执行成功且完整序号递增后调用，随后同ISR立即同步事件再发布记录。
 *     完整契约见 tuning.c::Tuning_CompleteControl。
 */
void Tuning_CompleteControl(const tuning_context_t *context);
/**
 * @brief 关闭调试并取消尚未生效的请求。
 * @note 完整契约见 tuning.c::Tuning_Stop。
 */
void Tuning_Stop(void);
/**
 * @brief 会话变化取消未应用请求而不回滚控制。
 * @note 完整契约见 tuning.c::Tuning_ResetSession。
 */
void Tuning_ResetSession(void);
/**
 * @brief 复制完整调参状态。
 * @note 完整契约见 tuning.c::Tuning_GetStatus。
 */
void Tuning_GetStatus(tuning_status_t *out);
/**
 * @brief 取走独立最终结果邮箱。
 * @note 完整契约见 tuning.c::Tuning_TakeResult。
 */
uint8_t Tuning_TakeResult(tuning_reply_t *reply, uint32_t *id);
/**
 * @brief 复制尚待保存的成功值及目标。
 * @note 停车前台复制，不清脏或访问Flash，输出非空；无待保存值不写输出。
 *     完整契约见 tuning.c::Tuning_CopyUnsaved。
 */
uint8_t Tuning_CopyUnsaved(tuning_request_t *request);
/**
 * @brief 记录一次前台保存尝试结果。
 * @note 只在停车保存尝试后调用，失败也清事务dirty；随后立即同步保存事件。
 *     完整契约见 tuning.c::Tuning_CompleteSave。
 */
void Tuning_CompleteSave(uint8_t saved);
/**
 * @brief 在原事件发生上下文读取版本和值。
 * @note 完整契约见 tuning.c::Tuning_CopyEventValues。
 */
void Tuning_CopyEventValues(tuning_event_kind_t kind, uint32_t *revision, tuning_values_t *values);
/**
 * @brief 取出当前调用待发布版本。
 * @note 由当前业务调用的原上下文立即取走，不能越过记录发布或下一业务调用。
 *     完整契约见 tuning.c::Tuning_TakeRevision。
 */
uint8_t Tuning_TakeRevision(uint32_t *revision);
/**
 * @brief 取出当前调用产生的唯一事件。
 * @note 由当前业务调用的原上下文立即清槽，不是跨轮事件队列；输出非空。
 *     完整契约见 tuning.c::Tuning_TakeEvent。
 */
uint8_t Tuning_TakeEvent(tuning_event_t *event);

#endif /* SMARTCAR_APP_CONTROL_TUNING_H */
