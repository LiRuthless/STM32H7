/**
 * @file lifecycle.h
 * @brief 运行生命周期接口。
 * @details 接口实现与完整调用约束见lifecycle.c；模块状态不对外暴露。
 */
#ifndef SMARTCAR_APP_SYSTEM_LIFECYCLE_H
#define SMARTCAR_APP_SYSTEM_LIFECYCLE_H

#include <stdint.h>

/**
 * @brief 按既有顺序初始化 BSP、参数和 App 模块。
 * @note 完整状态、上下文与副作用见lifecycle.c::Lifecycle_Init。
 */
void Lifecycle_Init(void);

/**
 * @brief 检查停车起跑条件，并在全部通过后启用定时运行链。
 * @note 完整状态、上下文与副作用见lifecycle.c::Lifecycle_TryStart。
 */
uint8_t Lifecycle_TryStart(void);

/**
 * @brief 检查启动、按键及待收尾门禁。
 * @note 完整状态、上下文与副作用见lifecycle.c::Lifecycle_IsStopped。
 */
uint8_t Lifecycle_IsStopped(void);

/**
 * @brief 读取定时控制启动标志。
 * @note 完整状态、上下文与副作用见lifecycle.c::Lifecycle_IsStarted。
 */
uint8_t Lifecycle_IsStarted(void);

/**
 * @brief 读取前台运行分支标志。
 * @note 完整状态、上下文与副作用见lifecycle.c::Lifecycle_IsKeyActive。
 */
uint8_t Lifecycle_IsKeyActive(void);

/**
 * @brief 读取等待期结束后的运行标志。
 * @note 完整状态、上下文与副作用见lifecycle.c::Lifecycle_IsRunning。
 */
uint8_t Lifecycle_IsRunning(void);

/**
 * @brief 读取本次运行2ms拍数。
 * @note 完整状态、上下文与副作用见lifecycle.c::Lifecycle_GetControlTicks。
 */
int32_t Lifecycle_GetControlTicks(void);

/**
 * @brief 读取最近控制拍的测距毫米值。
 * @note 完整状态、上下文与副作用见lifecycle.c::Lifecycle_GetDistanceMm。
 */
uint16_t Lifecycle_GetDistanceMm(void);

/**
 * @brief 推进一个2ms控制拍。
 * @note 完整状态、上下文与副作用见lifecycle.c::Lifecycle_AdvanceControlTick。
 */
void Lifecycle_AdvanceControlTick(void);

/**
 * @brief 保存本拍的测距结果。
 * @note 完整状态、上下文与副作用见lifecycle.c::Lifecycle_SetDistanceMm。
 */
void Lifecycle_SetDistanceMm(uint16_t distance_mm);

/**
 * @brief 发布等待期已结束的运行标志。
 * @note 完整状态、上下文与副作用见lifecycle.c::Lifecycle_MarkRunning。
 */
void Lifecycle_MarkRunning(void);

/**
 * @brief 按原链式顺序清除运行标志。
 * @note 完整状态、上下文与副作用见lifecycle.c::Lifecycle_ClearRunFlags。
 */
void Lifecycle_ClearRunFlags(void);

/**
 * @brief 读取看门狗已启动标志。
 * @note 完整状态、上下文与副作用见lifecycle.c::Lifecycle_IsWatchdogStarted。
 */
uint8_t Lifecycle_IsWatchdogStarted(void);

#if defined(SMARTCAR_HOST_TEST)
/**
 * @brief 布置宿主运行状态。
 * @note 仅宿主测试编译；见lifecycle.c::Lifecycle_TestSetRunState。
 */
void Lifecycle_TestSetRunState(int32_t ticks, uint8_t key, uint8_t start, uint8_t run);
#endif

#endif
