/**
 * @file legacy_imu.h
 * @brief 仅供 IMU owner 转发的旧算法。
 * @details 声明对应实现的模块接口，私有历史不通过本头公开。
 */

#ifndef SMARTCAR_APP_SENSING_LEGACY_IMU_H
#define SMARTCAR_APP_SENSING_LEGACY_IMU_H

#include "sensing/imu_proc.h"
/**
 * @brief 用旧公式更新加速度与速度积分。
 * @note 仅由 imu_proc 同名入口组参转发，不新增活动调用。
 *     完整契约见 legacy_imu.c::LegacyIMU_UpdateAccelVelocity。
 */
void LegacyIMU_UpdateAccelVelocity(imu_state_t *state, const float offsets[3],
    float *const history[3]);

/**
 * @brief 采一百次加速度，写三轴偏置并清速度。
 * @note 仅由 imu_proc 同名入口组参转发，不新增活动调用。
 *     完整契约见 legacy_imu.c::LegacyIMU_CalibrateAccel。
 */
void LegacyIMU_CalibrateAccel(imu_state_t *state, float *const offsets[3]);

#endif
