/**
 * @file legacy_control.h
 * @brief 旧执行包装。
 * @details 声明当前活动调度未接入的双传感器整段控制及角度差速包装；
 *     实现位于legacy_control.c，保留内部调用与既有副作用，不作为新调度入口。
 */

#ifndef SMARTCAR_APP_CONTROL_LEGACY_CONTROL_H
#define SMARTCAR_APP_CONTROL_LEGACY_CONTROL_H

#include <stdint.h>
/**
 * @brief 保留完整双传感器检查与控制的旧封装入口。
 * @note 当前App调度使用分步接口；本入口仍休眠，保留原观察、版本发布和执行副作用。
 *     完整契约见 legacy_control.c::Control_UpdateLegacy。
 */
void Control_UpdateLegacy(void);

/**
 * @brief 保留角度控制的电机执行包装。
 * @note 当前活动链不调用，不新增角度环接入。
 *     完整契约见 legacy_control.c::PID_UpdateAngle。
 */
void PID_UpdateAngle(int16_t target_angle, float angle, float gyro);

#endif
