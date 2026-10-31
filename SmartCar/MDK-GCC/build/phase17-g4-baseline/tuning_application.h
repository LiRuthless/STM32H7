/**
 * @file tuning_application.h
 * @brief 调参上下文与保存协调入口。
 * @details 业务请求与结果独立于通信；即时同步接口在记录发布前执行。
 */
#ifndef SMARTCAR_APP_SYSTEM_TUNING_APPLICATION_H
#define SMARTCAR_APP_SYSTEM_TUNING_APPLICATION_H
#include "control/tuning.h"

/**
 * @brief 协调调参业务与应用状态
 * @note 完整契约见 tuning_application.c::TuningApplication_CaptureContext。
 */
void TuningApplication_CaptureContext(tuning_context_t *context);
/**
 * @brief 协调调参业务与应用状态
 * @note 完整契约见 tuning_application.c::TuningApplication_Set。
 */
void TuningApplication_Set(uint32_t id, const tuning_request_t *request, tuning_reply_t *reply);
/**
 * @brief 协调调参业务与应用状态
 * @note 完整契约见 tuning_application.c::TuningApplication_Switch。
 */
void TuningApplication_Switch(uint8_t enable, tuning_reply_t *reply);
/**
 * @brief 协调调参业务与应用状态
 * @note 完整契约见 tuning_application.c::TuningApplication_FinalizeStop。
 */
void TuningApplication_FinalizeStop(void);
/**
 * @brief 协调调参业务与应用状态
 * @note 完整契约见 tuning_application.c::TuningApplication_ServiceSave。
 */
void TuningApplication_ServiceSave(void);
/**
 * @brief 协调调参业务与应用状态
 * @note 完整契约见 tuning_application.c::TuningApplication_Synchronize。
 */
void TuningApplication_Synchronize(void);

#endif /* SMARTCAR_APP_SYSTEM_TUNING_APPLICATION_H */
