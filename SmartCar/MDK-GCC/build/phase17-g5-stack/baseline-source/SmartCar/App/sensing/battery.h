/**
 * @file battery.h
 * @brief 电池采样与滤波只读接口。
 * @details 接口实现与完整调用约束见battery.c；模块状态不对外暴露。
 */
#ifndef SMARTCAR_APP_SENSING_BATTERY_H
#define SMARTCAR_APP_SENSING_BATTERY_H

#include <stdint.h>

/**
 * @brief 初始化电池低通滤波器。
 * @note 完整状态、上下文与副作用见battery.c::Battery_Init。
 */
void Battery_Init(void);

/**
 * @brief 读取电池ADC并刷新滤波值。
 * @note 完整状态、上下文与副作用见battery.c::Battery_Update。
 */
void Battery_Update(void);

/**
 * @brief 读取电池ADC原始标度值。
 * @note 完整状态、上下文与副作用见battery.c::Battery_GetRaw。
 */
int16_t Battery_GetRaw(void);

/**
 * @brief 读取电池滤波标度值。
 * @note 完整状态、上下文与副作用见battery.c::Battery_GetFiltered。
 */
int16_t Battery_GetFiltered(void);

#endif
