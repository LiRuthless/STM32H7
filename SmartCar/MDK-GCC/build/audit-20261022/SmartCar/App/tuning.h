/**
 * @file tuning.h
 * @brief 完整拍调参、状态查询及停车保存接口。
 * @details 前台接收完整请求，TIM6独占控制快照提交；完整约束见tuning.c。
 */
#ifndef SMARTCAR_APP_TUNING_H
#define SMARTCAR_APP_TUNING_H
#include "scb_protocol.h"
#include "pid.h"

typedef enum
{
    TUNING_DISABLED, /**< 未开启覆盖。 */
    TUNING_ARMED, /**< 已开启，等待普通完整拍及成功参数。 */
    TUNING_ACTIVE, /**< 当前使用调试快照。 */
    TUNING_SUSPENDED /**< 元素期间使用原策略。 */
} tuning_phase_t;

typedef struct
{
    scb_values_t effective; /**< 最近完整拍实际值。 */
    uint32_t revision; /**< 有效参数提交版本。 */
    tuning_phase_t phase; /**< 调试生命周期。 */
    uint8_t enabled; /**< 已显式开启。 */
    uint8_t overridden; /**< 本拍使用调试值。 */
    uint8_t save_state; /**< 0空闲、1待保存、2成功、3失败。 */
} tuning_status_t;

/**
 * @brief 初始化调参并注册命令。
 * @note 完整契约见tuning.c::Tuning_Init。
 */
void Tuning_Init(void);
/**
 * @brief 前台分发已预留请求。
 * @note 完整契约见tuning.c::Tuning_HandleCommand。
 */
void Tuning_HandleCommand(uint32_t id, const scb_command_t *command);
/**
 * @brief 清未生效传输请求。
 * @note 完整契约见tuning.c::Tuning_ResetSession。
 */
void Tuning_ResetSession(void);
/**
 * @brief 选择本拍完整控制值。
 * @note 完整契约见tuning.c::Tuning_SelectControl。
 */
void Tuning_SelectControl(uint8_t ordinary, uint8_t allowed,
    const pid_track_gains_t *original, int16_t base_speed);
/**
 * @brief 复制本拍调试值及许可。
 * @note 完整契约见tuning.c::Tuning_GetControl。
 */
uint8_t Tuning_GetControl(scb_values_t *values);
/**
 * @brief 在执行后提交结果。
 * @note 完整契约见tuning.c::Tuning_CompleteControl。
 */
void Tuning_CompleteControl(void);
/**
 * @brief 停车关闭并恢复共享基线。
 * @note 完整契约见tuning.c::Tuning_Stop。
 */
void Tuning_Stop(void);
/**
 * @brief 日志收尾后合入成功参数。
 * @note 完整契约见tuning.c::Tuning_FinalizeStop。
 */
void Tuning_FinalizeStop(void);
/**
 * @brief 服务最终结果及停车保存。
 * @note 完整契约见tuning.c::Tuning_Service。
 */
void Tuning_Service(void);
/**
 * @brief 复制一致状态。
 * @note 完整契约见tuning.c::Tuning_GetStatus。
 */
void Tuning_GetStatus(tuning_status_t *status);
#endif
