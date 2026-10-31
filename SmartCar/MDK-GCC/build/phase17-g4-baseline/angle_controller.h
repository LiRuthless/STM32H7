/**
 * @file angle_controller.h
 * @brief 角度算法及历史复位。
 * @details 声明对应实现的模块接口，私有历史不通过本头公开。
 */

#ifndef SMARTCAR_APP_CONTROL_ANGLE_CONTROLLER_H
#define SMARTCAR_APP_CONTROL_ANGLE_CONTROLLER_H

#include "control/pid.h"
/**
 * @brief 计算角度 PD 并更新 owner 输出。
 * @note 本模块独占角度前拍历史，不执行电机动作。
 *     完整契约见 angle_controller.c::AngleController_Update。
 */
void AngleController_Update(int16_t target_angle, float angle, float gyro,
    const pid_angle_gains_t *gains, float *error, float *output);

/**
 * @brief 只清角度前拍历史。
 * @note 完整契约见 angle_controller.c::AngleController_Reset。
 */
void AngleController_Reset(void);

#endif
