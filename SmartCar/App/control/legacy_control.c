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
#include "control/tuning.h"
#include "system/tuning_application.h"

/**
 * @brief 保留完整双传感器检查与控制的旧封装入口。
 * @details 先更新电感与灰度，累计采集错误、无标定、灰度失线和电感和不大于300的保护位；
 *     故障时按采集/标定、灰度、电感优先级观察停车并请求首因，成功才观察、置运行标志和执行。
 *     两条观察返回后立即同步其原版本发布副作用，先于停车或运行执行，不接入活动调度。
 * @note 当前App调度使用分步接口；本入口仍休眠，保留原观察、版本发布和执行副作用。
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
        TuningApplication_Synchronize();
        App_RequestStop(reason);
        return;
    }
    Tracking_Observe(1, 0);
    TuningApplication_Synchronize();
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
