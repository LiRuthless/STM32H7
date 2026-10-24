/**
 * @file speed_controller.h
 * @brief 双轮 PI 与复位。
 * @details 声明左右轮独立PI及统一历史复位；实现见speed_controller.c，不读电机或传感器状态。
 */

#ifndef SMARTCAR_APP_CONTROL_SPEED_CONTROLLER_H
#define SMARTCAR_APP_CONTROL_SPEED_CONTROLLER_H

#include "control/pid.h"
/**
 * @brief 更新左轮独立 PI 历史并写调用方输出。
 * @note 完整契约见 speed_controller.c::SpeedController_UpdateLeft。
 */
int16_t SpeedController_UpdateLeft(int16_t target, int16_t measured,
    float kp, float ki, float *output);

/**
 * @brief 更新右轮独立 PI 历史并写调用方输出。
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
