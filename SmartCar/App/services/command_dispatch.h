/**
 * @file command_dispatch.h
 * @brief 业务通信适配的运行入口。
 * @details 公开接口仅使用业务类型；SCB命令handler在实现内注册，不向system传递协议头。
 */
#ifndef SMARTCAR_APP_SERVICES_COMMAND_DISPATCH_H
#define SMARTCAR_APP_SERVICES_COMMAND_DISPATCH_H
#include "control/tuning.h"

/**
 * @brief 注册业务请求及会话回调。
 * @note 完整契约见 command_dispatch.c::CommandDispatch_Init。
 */
void CommandDispatch_Init(void);
/**
 * @brief 初始化原可靠通信链路。
 * @note 完整契约见 command_dispatch.c::CommandDispatch_InitLink。
 */
void CommandDispatch_InitLink(void);
/**
 * @brief 前台输出事务结果并尝试停车保存。
 * @note 完整契约见 command_dispatch.c::CommandDispatch_Service。
 */
void CommandDispatch_Service(void);
/**
 * @brief 执行原前台链路服务。
 * @note 完整契约见 command_dispatch.c::CommandDispatch_ServiceLink。
 */
void CommandDispatch_ServiceLink(void);
/**
 * @brief 取消会话尚未完成请求和导出。
 * @note 完整契约见 command_dispatch.c::CommandDispatch_ResetSession。
 */
void CommandDispatch_ResetSession(void);
/**
 * @brief 把即时业务事件映射为SCB事件描述。
 * @note 完整契约见 command_dispatch.c::CommandDispatch_QueueTuningEvent。
 */
void CommandDispatch_QueueTuningEvent(const tuning_event_t *event);
/**
 * @brief 将原运行起跑事件入通信队列。
 * @note 完整契约见 command_dispatch.c::CommandDispatch_PublishRunStarted。
 */
void CommandDispatch_PublishRunStarted(void);
/**
 * @brief 将原运行停车事件入通信队列。
 * @note 完整契约见 command_dispatch.c::CommandDispatch_PublishRunStopped。
 */
void CommandDispatch_PublishRunStopped(uint8_t reason);

#endif /* SMARTCAR_APP_SERVICES_COMMAND_DISPATCH_H */
