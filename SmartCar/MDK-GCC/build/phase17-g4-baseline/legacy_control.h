/**
 * @file legacy_control.h
 * @brief 旧执行包装。
 * @details 声明对应实现的模块接口，私有历史不通过本头公开。
 */

#ifndef SMARTCAR_APP_CONTROL_LEGACY_CONTROL_H
#define SMARTCAR_APP_CONTROL_LEGACY_CONTROL_H

#include <stdint.h>
/**
 * @brief 保留完整双传感器检查与控制的旧封装入口。
 * @note 当前 App 调度使用分步接口；本函数调用关系保持，不新增接入或合并错误分支。
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
