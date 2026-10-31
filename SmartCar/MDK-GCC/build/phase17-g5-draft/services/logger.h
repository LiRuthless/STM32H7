/**
 * @file logger.h
 * @brief 日志会话状态内部只读与故障接口。
 * @details 完整契约在logger.c；公开Datalog入口仍由datalog.h声明。
 */
#ifndef SMARTCAR_APP_SERVICES_LOGGER_H
#define SMARTCAR_APP_SERVICES_LOGGER_H
#include "services/datalog.h"

/**
 * @brief 锁存首次故障并按原规则停生产。
 * @note 完整契约见 logger.c::Logger_LatchFault。
 */
void Logger_LatchFault(uint8_t fault);

/**
 * @brief 读取当前会话首次停车原因。
 * @note 完整契约见 logger.c::Logger_GetStopReason。
 */
uint8_t Logger_GetStopReason(void);

/**
 * @brief 读取当前会话头已写回校验标志。
 * @note 完整契约见 logger.c::Logger_HasHeader。
 */
uint8_t Logger_HasHeader(void);

#endif /* SMARTCAR_APP_SERVICES_LOGGER_H */
