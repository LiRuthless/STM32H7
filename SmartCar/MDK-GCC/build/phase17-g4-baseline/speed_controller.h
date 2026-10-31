/**
 * @file speed_controller.h
 * @brief 双轮 PI 与复位。
 * @details 声明对应实现的模块接口，私有历史不通过本头公开。
 */

#ifndef SMARTCAR_APP_CONTROL_SPEED_CONTROLLER_H
#define SMARTCAR_APP_CONTROL_SPEED_CONTROLLER_H

#include "control/pid.h"
/**
 * @brief 更新Left轮独立 PI 历史与 owner 输出。
 * @note 完整契约见 speed_controller.c::SpeedController_UpdateLeft。
 */
int16_t SpeedController_UpdateLeft(int16_t target, int16_t measured,
    float kp, float ki, float *output);

/**
 * @brief 更新Right轮独立 PI 历史与 owner 输出。
 * @note 完整契约见 speed_controller.c::SpeedController_UpdateRight。
 */
int16_t SpeedController_UpdateRight(int16_t target, int16_t measured,
    float kp, float ki, float *output);

/**
 * @brief 按左积分、右积分、左目标、右目标顺序清活动 PI 历史。
 * @note 完整契约见 speed_controller.c::SpeedController_Reset。
 */
void SpeedController_Reset(void);

#endif
