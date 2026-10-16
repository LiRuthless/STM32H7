/**
 * @file control.c
 * @brief App 双模式观察与唯一控制执行口；TIM6 调用，旧整体入口保留。
 * @note 移植来源：Sirius20260718/project/code/control.c，STC AI8051U 省赛快照（2026-07-18）。
 * @note 源算法文件未单列作者及独立许可证；保留原出处，关联 Phase 13/15 规约。
 */
#include "bsp.h"
#include "app_config.h"
#include "app.h"
#include "control.h"
#include "motor.h"
#include "track_sensor.h"
#include "pid.h"
#include "element.h"
#include "roundabout.h"
#include "gray_sensor.h"
#include "optical_fsm.h"
#include "electromagnetic_fsm.h"
#include "imu_proc.h"
#include "param.h"

/* 共享监视量由既有控制/复位路径写入；双模式观察使用各自私有上下文，只有选中模式执行。 */
int16_t g_control_track_output = 0; /* 方向控制输出（由循迹PID计算） */
uint8_t g_control_kernel_state = CONTROL_KERNEL_TRACKING;
uint8_t g_control_cask_flag = 0;

/* 只覆盖旧分支实际赋值的字段；未出现的字段沿用上一拍。 */
static const control_profile_t s_kernel_profiles[7] =
{
    [CONTROL_KERNEL_TRACKING]
    =
    {
        CONTROL_PROFILE_WEIGHTS | CONTROL_PROFILE_BASE | CONTROL_PROFILE_TRACK,
        { 15, 20, 22, 10 },
        245,
        { 2.0f, 0.008f, 15.0f, 1.0f },
        { 0 }
    },
    [CONTROL_KERNEL_ISLAND_L] =
    {
        CONTROL_PROFILE_WEIGHTS | CONTROL_PROFILE_ANGLE,
        { 15, 20, 22, 10 },
        0,
        { 0 },
        { 2.0f, 1.2f, 0.0f }
    },
    [CONTROL_KERNEL_ISLAND_R]
    =
    {
        CONTROL_PROFILE_WEIGHTS | CONTROL_PROFILE_BASE | CONTROL_PROFILE_ANGLE,
        { 15, 20, 22, 10 },
        150,
        { 0 },
        { 2.0f, 1.2f, 0.0f }
    },
    [CONTROL_KERNEL_TEETERBOARD] =
    {
        CONTROL_PROFILE_BASE | CONTROL_PROFILE_TRACK,
        { 0 },
        100,
        { 4.5f, 0.0f, 6.2f, 0.9f },
        { 0 }
    },
    [CONTROL_KERNEL_CROSSROADS]
    =
    {
        CONTROL_PROFILE_WEIGHTS | CONTROL_PROFILE_BASE | CONTROL_PROFILE_TRACK,
        { 15, 20, 2, 10 },
        240,
        { 2.0f, 0.008f, 15.0f, 1.0f },
        { 0 }
    },
    [CONTROL_KERNEL_CASK]
    =
    {
        CONTROL_PROFILE_WEIGHTS | CONTROL_PROFILE_BASE | CONTROL_PROFILE_TRACK,
        { 15, 20, 2, 5 },
        210,
        { 2.0f, 0.008f, 15.0f, 1.0f },
        { 0 }
    },
    [CONTROL_KERNEL_REISLAND]
    =
    {
        CONTROL_PROFILE_WEIGHTS | CONTROL_PROFILE_BASE | CONTROL_PROFILE_TRACK,
        { 20, 10, 1, 10 },
        180,
        { 2.0f, 0.008f, 15.0f, 1.0f },
        { 0 }
    }
};

static tracking_state_t s_tracking = { .mode = TRACKING_MODE_OPTICAL };

void Control_ApplyProfile(const control_profile_t *profile)
{
    if (profile->fields & CONTROL_PROFILE_WEIGHTS)
    {
        Track_SetWeights(&profile->weights);
    }
    if (profile->fields & CONTROL_PROFILE_BASE)
    {
        Motor_SetBaseSpeed(profile->base_speed);
    }
    if (profile->fields & CONTROL_PROFILE_TRACK)
    {
        PID_SetTrackGains(&profile->track);
    }
    if (profile->fields & CONTROL_PROFILE_ANGLE)
    {
        PID_SetAngleGains(&profile->angle);
    }
}

const tracking_state_t *Tracking_GetState(void)
{
    return &s_tracking;
}

tracking_result_t Tracking_TrySetMode(uint8_t mode)
{
    if (mode > 1)
    {
        return TRACKING_ARG;
    }
    if (!App_IsStopped())
    {
        return TRACKING_BUSY;
    }
    if (!Param_TryFlushPending())
    {
        BSP_UART_WriteString("track mode save failed\r\n");
        return TRACKING_SAVE_FAILED;
    }
    s_tracking.mode = mode;
    Control_ResetRunState();
    Motor_ResetRunState();
    PID_ResetAll();
    IMU_ResetRunState();
    Param_ApplyCurrentDirection();
    BSP_UART_WriteString(
        mode == TRACKING_MODE_OPTICAL ? "track mode: optical\r\n" : "track mode: em\r\n");
    return TRACKING_OK;
}

void Tracking_SetGear(uint8_t gear)
{
    if (gear < 2)
    {
        s_tracking.gear = gear;
    }
}

void Tracking_CompleteControl(void)
{
    ++s_tracking.control_sequence;
}

void Tracking_Stopped(uint8_t reason)
{
    const gray_state_t *g = Gray_GetState();
    const optical_inputs_t in
        =
        {
            0,
            (uint8_t)!(s_tracking.protection_flags & TRACKING_PROTECT_GRAY_LOST),
            0,
            reason,
            g_app_control_ticks,
            g->error,
            0,
            0,
            { 0 }
        };
    OpticalFSM_Update(&in);
    g_control_track_output = 0;
    s_tracking.track_out = 0;
}

void Tracking_AddProtection(uint16_t flags)
{
    uint32_t mask = BSP_Time_EnterCritical();
    s_tracking.protection_flags |= flags;
    BSP_Time_ExitCritical(mask);
}

const control_profile_t *Control_GetKernelProfile(uint8_t state)
{
    return &s_kernel_profiles[state < 7 ? state : CONTROL_KERNEL_TRACKING];
}

void Control_MergeProfile(control_profile_t *dst, const control_profile_t *src)
{
    if (src->fields & CONTROL_PROFILE_WEIGHTS)
    {
        dst->weights = src->weights;
    }
    if (src->fields & CONTROL_PROFILE_BASE)
    {
        dst->base_speed = src->base_speed;
    }
    if (src->fields & CONTROL_PROFILE_TRACK)
    {
        dst->track = src->track;
    }
    if (src->fields & CONTROL_PROFILE_ANGLE)
    {
        dst->angle = src->angle;
    }
    dst->fields |= src->fields;
}

void Tracking_Observe(uint8_t running, uint8_t stop_reason)
{
    const track_state_t *t = Track_GetState();
    const gray_state_t *g = Gray_GetState();
    const param_direction_t *p = Param_GetDirection(TRACKING_MODE_OPTICAL, s_tracking.gear);
    const electromagnetic_inputs_t em
        =
        {
            t->filtered,
            g_app_dl1b_distance_mm,
            IMU_GetState()->accel[0],
            IMU_GetAngleDeltaX(),
            (uint8_t)(running && g_app_control_ticks > RUN_DELAY_COUNT)
        };
    const optical_inputs_t opt =
    {
        running,
        g->line_valid,
        (uint8_t)(t->filtered[0] + t->filtered[1] + t->filtered[2] + t->filtered[3] > 300),
        stop_reason,
        g_app_control_ticks,
        g->error,
        p->base_speed,
        IMU_GetState()->accel[0],
        { p->kp, p->k2p, p->kd, 1.0f }
    };
    const electromagnetic_state_t *e;
    ElectromagneticFSM_Update(&em);
    OpticalFSM_Update(&opt);
    e = ElectromagneticFSM_GetState();
    Track_PublishObservation(&e->configuration.weights, e->error, e->symmetry_x, e->symmetry_y);
    /* 兼容旧监视字段；所有活动判断仅写私有电磁上下文。 */
    g_control_kernel_state = e->kernel_state;
    g_roundabout_state = e->roundabout_state;
    s_tracking.active_error = (s_tracking.mode == TRACKING_MODE_OPTICAL) ? g->error : e->error;
}

void Tracking_Execute(void)
{
    if (s_tracking.mode == TRACKING_MODE_OPTICAL)
    {
        const optical_state_t *o = OpticalFSM_GetState();
        if (o->state != OPTICAL_TRACKING)
        {
            return;
        }
        Motor_SetBaseSpeed(o->base_speed);
        g_control_track_output = o->output;
        Motor_DistributeSpeed(g_control_track_output);
    }
    else
    {
        const electromagnetic_state_t *e = ElectromagneticFSM_GetState();
        Control_ApplyProfile(&e->configuration);
        Motor_DistributeSpeed(e->steering);
        if (e->reset_distance)
        {
            Motor_ResetDistance();
        }
        if (e->zero_angle)
        {
            IMU_ZeroAngleX();
        }
        g_control_track_output = e->output;
    }
    s_tracking.track_out = g_control_track_output;
}

void Tracking_AccumulateMotion(void)
{
    ElectromagneticFSM_Accumulate(
        Motor_GetState()->left.real_speed, Motor_GetState()->right.real_speed);
}

void Control_UpdateLegacy(void)
{
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
    Run_flag = 1;
    Tracking_Execute();
}

void Control_ResetRunState(void)
{
    g_control_kernel_state = CONTROL_KERNEL_TRACKING;
    g_control_cask_flag = 0;
    g_control_track_output = 0;
    s_tracking.active_error = 0;
    s_tracking.track_out = 0;
    s_tracking.control_sequence = 0;
    s_tracking.protection_flags = 0;
    ElectromagneticFSM_Reset();
    OpticalFSM_Reset();
}
