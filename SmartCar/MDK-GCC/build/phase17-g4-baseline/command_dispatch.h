/**
 * @file command_dispatch.h
 * @brief 业务通信适配的运行入口。
 * @details 公开接口仅使用业务类型；SCB命令handler在实现内注册，不向system传递协议头。
 */
#ifndef SMARTCAR_APP_SERVICES_COMMAND_DISPATCH_H
#define SMARTCAR_APP_SERVICES_COMMAND_DISPATCH_H
#include "control/tuning.h"

/**
 * @brief 执行通信业务适配
 * @note 完整契约见 command_dispatch.c::CommandDispatch_Init。
 */
void CommandDispatch_Init(void);
/**
 * @brief 执行通信业务适配
 * @note 完整契约见 command_dispatch.c::CommandDispatch_InitLink。
 */
void CommandDispatch_InitLink(void);
/**
 * @brief 执行通信业务适配
 * @note 完整契约见 command_dispatch.c::CommandDispatch_Service。
 */
void CommandDispatch_Service(void);
/**
 * @brief 执行通信业务适配
 * @note 完整契约见 command_dispatch.c::CommandDispatch_ServiceLink。
 */
void CommandDispatch_ServiceLink(void);
/**
 * @brief 执行通信业务适配
 * @note 完整契约见 command_dispatch.c::CommandDispatch_ResetSession。
 */
void CommandDispatch_ResetSession(void);
/**
 * @brief 执行通信业务适配
 * @note 完整契约见 command_dispatch.c::CommandDispatch_QueueTuningEvent。
 */
void CommandDispatch_QueueTuningEvent(const tuning_event_t *event);
/**
 * @brief 执行通信业务适配
 * @note 完整契约见 command_dispatch.c::CommandDispatch_PublishRunStarted。
 */
void CommandDispatch_PublishRunStarted(void);
/**
 * @brief 执行通信业务适配
 * @note 完整契约见 command_dispatch.c::CommandDispatch_PublishRunStopped。
 */
void CommandDispatch_PublishRunStopped(uint8_t reason);

#endif /* SMARTCAR_APP_SERVICES_COMMAND_DISPATCH_H */
