/**
 * @file legacy_pid.h
 * @brief 旧算法显式输入接口。
 * @details 声明旧共享方向预览及休眠增量控制；实现见legacy_pid.c，活动双模式历史由各FSM持有。
 */

#ifndef SMARTCAR_APP_CONTROL_LEGACY_PID_H
#define SMARTCAR_APP_CONTROL_LEGACY_PID_H

#include "control/pid.h"
/**
 * @brief 用旧共享历史计算电磁方向输出。
 * @note 停车前台预览仍调用本入口，旧环岛保留原调用；活动双模式观察器使用各自独立历史。
 *     完整契约见 legacy_pid.c::PID_UpdateTrack。
 */
int16_t PID_UpdateTrack(int16_t error, float accel);

/**
 * @brief 保留左轮增量式速度控制入口。
 * @note 当前不接入；局部静态历史保留，微分代码仍注释，ResetAll 不新增复位。
 *     完整契约见 legacy_pid.c::PID_UpdateIncrementalLeft。
 */
int16_t PID_UpdateIncrementalLeft(int16_t target, int16_t measured);

/**
 * @brief 保留右轮增量式速度控制入口。
 * @note 当前不接入；局部静态历史保留，微分代码仍注释，ResetAll 不新增复位。
 *     完整契约见 legacy_pid.c::PID_UpdateIncrementalRight。
 */
int16_t PID_UpdateIncrementalRight(int16_t target, int16_t measured);

/**
 * @brief 只清旧共享方向历史，不清休眠增量式局部状态。
 * @note 完整契约见 legacy_pid.c::LegacyPID_ResetTrackHistory。
 */
void LegacyPID_ResetTrackHistory(void);

#endif
