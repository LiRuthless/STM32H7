/**
 * @file direction_controller.h
 * @brief 显式输入方向计算。
 * @details 声明对应实现的模块接口，私有历史不通过本头公开。
 */

#ifndef SMARTCAR_APP_CONTROL_DIRECTION_CONTROLLER_H
#define SMARTCAR_APP_CONTROL_DIRECTION_CONTROLLER_H

#include "control/pid.h"
/**
 * @brief 用调用方独立历史计算方向控制。
 * @note TIM6 调用；浮点求和先转换 int16_t，再限幅 ±1000；大值存在转换风险，尚未实施 R01 修复。
 *     完整契约见 direction_controller.c::PID_DirectionStep。
 */
int16_t PID_DirectionStep(
    int16_t input, float accel, const pid_track_gains_t *gains, int32_t *history);

#endif
