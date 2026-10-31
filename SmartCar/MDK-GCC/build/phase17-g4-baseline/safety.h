/**
 * @file safety.h
 * @brief 安全保护与收尾接口。
 * @details 接口实现与完整调用约束见safety.c；模块状态不对外暴露。
 */
#ifndef SMARTCAR_APP_SYSTEM_SAFETY_H
#define SMARTCAR_APP_SYSTEM_SAFETY_H

#include <stdint.h>
#include "app_types.h"

/**
 * @brief 锁存首次原因并执行有界安全停车。
 * @note 完整状态、上下文与副作用见safety.c::Safety_RequestStop。
 */
void Safety_RequestStop(app_stop_reason_t reason);

/**
 * @brief 在前台完成日志尾包并报告首次停车。
 * @note 完整状态、上下文与副作用见safety.c::Safety_FinalizeStop。
 */
void Safety_FinalizeStop(void);

/**
 * @brief 读取待前台收尾标志。
 * @note 完整状态、上下文与副作用见safety.c::Safety_IsStopPending。
 */
uint8_t Safety_IsStopPending(void);

/**
 * @brief 读取已锁存的首次停车原因。
 * @note 完整状态、上下文与副作用见safety.c::Safety_GetStopReason。
 */
app_stop_reason_t Safety_GetStopReason(void);

/**
 * @brief 在成功起跑前清除安全历史。
 * @note 完整状态、上下文与副作用见safety.c::Safety_Reset。
 */
void Safety_Reset(void);

/**
 * @brief 检查本次滤波值的低压持续条件。
 * @note 完整状态、上下文与副作用见safety.c::Safety_CheckLowVoltage。
 */
uint8_t Safety_CheckLowVoltage(void);

#endif
