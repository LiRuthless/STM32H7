/**
 * @file legacy_control.c
 * @brief 旧控制执行包装。
 * @details 保留双传感器检查和角度差速副作用，活动调度不新增调用。
 * @note 移植来源：Sirius20260718/project/code/control.c / pid.c，STC AI8051U 省赛快照（2026-07-18）。
 * @note 源文件未单列作者及独立许可证；保留出处，关联 Phase 13/15。
 */
#include "bsp.h"
#include "app_config.h"
#include "app.h"
#include "system/lifecycle.h"
#include "control/tracking.h"
#include "control/motor.h"
#include "sensing/track_sensor.h"
#include "control/pid.h"
#include "sensing/gray_sensor.h"
#include "control/optical_fsm.h"
#include "control/electromagnetic_fsm.h"
#include "sensing/imu_proc.h"
#include "services/param.h"
#include "control/tuning.h"

/**
 * @brief 保留完整双传感器检查与控制的旧封装入口。
 * @details 按现有调用顺序使用 Track_ReadAdc、Gray_Update、Gray_GetState、Track_GetState
 *     完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @note 当前 App 调度使用分步接口；本函数调用关系保持，不新增接入或合并错误分支。
 */
void Control_UpdateLegacy(void)
{
    /* 保留入口先刷新双传感器并累计全部保护位；存在故障时观察停车并请求首因，否则依次观察、置运行标志和执行控制。 */
    uint16_t flags = 0;
    Track_ReadAdc();
    Gray_Update();
    if (Gray_GetState()->acquisition_status)
    {
        flags |= TRACKING_PROTECT_ACQUIRE;
    }
    if (!Gray_GetState()->calibration_valid)
    {
        flags |= TRACKING_PROTECT_CALIBRATION;
    }
    if (!Gray_GetState()->line_valid)
    {
        flags |= TRACKING_PROTECT_GRAY_LOST;
    }
    if (Track_GetState()->filtered[0] + Track_GetState()->filtered[1]
            + Track_GetState()->filtered[2] + Track_GetState()->filtered[3]
        <= 300)
    {
        flags |= TRACKING_PROTECT_EM_LOST;
    }
    Tracking_AddProtection(flags);
    if (flags)
    {
        app_stop_reason_t reason
            = (flags & (TRACKING_PROTECT_ACQUIRE | TRACKING_PROTECT_CALIBRATION))
            ? APP_STOP_GRAY_ACQUIRE
            : ((flags & TRACKING_PROTECT_GRAY_LOST) ? APP_STOP_GRAY_LOST : APP_STOP_OFF_TRACK);
        Tracking_Observe(0, (uint8_t)reason);
        App_RequestStop(reason);
        return;
    }
    Tracking_Observe(1, 0);
    Lifecycle_MarkRunning();
    Tracking_Execute();
}

/**
 * @brief 保留角度控制的电机执行包装。
 * @details 先完成误差、输出和历史写入，再将输出取负并转换为差速接口整数参数。
 * @param[in] target_angle 目标 X 角度，度。
 * @param[in] angle 当前 X 角度，度。
 * @param[in] gyro 当前 X 角速度，度/秒。
 * @note 当前活动链不调用，不新增角度环接入。
 */
void PID_UpdateAngle(int16_t target_angle, float angle, float gyro)
{
    Motor_DistributeSpeed(-PID_CalculateAngle(target_angle, angle, gyro));
}
