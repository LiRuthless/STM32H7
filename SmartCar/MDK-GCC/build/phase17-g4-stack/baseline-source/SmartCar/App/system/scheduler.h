/**
 * @file scheduler.h
 * @brief 前台及三线调度接口。
 * @details 接口实现与完整调用约束见scheduler.c；模块状态不对外暴露。
 */
#ifndef SMARTCAR_APP_SYSTEM_SCHEDULER_H
#define SMARTCAR_APP_SYSTEM_SCHEDULER_H

#include <stdint.h>

/**
 * @brief 调度运行日志或停车采样、显示、调参和起跑。
 * @note 完整状态、上下文与副作用见scheduler.c::Scheduler_Loop。
 */
void Scheduler_Loop(void);

/**
 * @brief 执行 2 ms 控制拍及完整预算保护。
 * @note 完整状态、上下文与副作用见scheduler.c::Scheduler_ControlISR。
 */
void Scheduler_ControlISR(void);

/**
 * @brief 执行 5 ms 电池保护、看门狗和运行灯。
 * @note 完整状态、上下文与副作用见scheduler.c::Scheduler_TaskISR。
 */
void Scheduler_TaskISR(void);

/**
 * @brief 运行时向日志环形队列推入 1 ms 记录。
 * @note 完整状态、上下文与副作用见scheduler.c::Scheduler_SampleISR。
 */
void Scheduler_SampleISR(void);

#endif
