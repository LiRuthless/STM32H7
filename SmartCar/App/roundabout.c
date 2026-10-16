/**
 * @file roundabout.c
 * @brief App 环岛配置与旧入口；配置供电磁影子机使用，旧判断函数当前不接入。
 * @note 移植来源：Sirius20260718/project/code/roundabout.c，STC AI8051U 省赛快照（2026-07-18）。
 * @note 源算法文件未单列作者及独立许可证；保留原出处，关联 Phase 13/15 规约。
 */
#include "bsp.h"
#include "app_config.h"
#include "roundabout.h"
#include "control.h"
#include "element.h"
#include "motor.h"
#include "track_sensor.h"
#include "pid.h"
#include "imu_proc.h"

enum
{
    ROUNDABOUT_ROUND_PROFILE_PREENTER,
    ROUNDABOUT_ROUND_PROFILE_TURN,
    ROUNDABOUT_ROUND_PROFILE_IN,
    ROUNDABOUT_ROUND_PROFILE_OUT
};

extern uint16_t g_app_dl1b_distance_mm; /* DL1B激光测距（mm），由app.c定义并周期更新 */
extern float float_abs(float a);        /* config模块 */

/* 旧入口的状态及参数保留；当前 App 只复位旧状态并读取配置，不接入旧判断。 */
uint8_t g_roundabout_state = ROUNDABOUT_STATE_NORMAL; /* 当前赛道元素状态机状态 */
uint8_t g_roundabout_left_flag = 0;                   /* 左环岛处理标志（1表示正在处理环岛） */
uint8_t g_roundabout_right_flag = 0;                  /* 右环岛处理标志（1表示正在处理环岛） */

int8_t g_roundabout_sign_round = 1;

uint16_t g_roundabout_enter_distance1 = 8000; /* 环岛1入环前直行距离 */
uint16_t g_roundabout_out_distance1 = 10000;  /* 环岛1出环后直行距离 */

int16_t g_roundabout_enter_angle1 = 30; /* 环岛1入环初始打角角度 */
int16_t g_roundabout_enter_angle2 = 60;
int16_t g_roundabout_out_angle1 = 200; /* 环岛1出环目标角度 */

static const control_profile_t s_round_profiles[4] =
{
    [ROUNDABOUT_ROUND_PROFILE_PREENTER]
    =
    {
        CONTROL_PROFILE_WEIGHTS | CONTROL_PROFILE_BASE | CONTROL_PROFILE_TRACK,
        { 20, 10, 1, 10 },
        150,
        { 2.0f, 0.008f, 15.0f, 1.0f },
        { 0 }
    },
    [ROUNDABOUT_ROUND_PROFILE_TURN] = { 0 },
    [ROUNDABOUT_ROUND_PROFILE_IN] =
    {
        CONTROL_PROFILE_BASE | CONTROL_PROFILE_TRACK,
        { 0 },
        150,
        { 2.0f, 0.008f, 15.0f, 1.0f },
        { 0 }
    },
    [ROUNDABOUT_ROUND_PROFILE_OUT]
    =
    {
        CONTROL_PROFILE_WEIGHTS | CONTROL_PROFILE_BASE | CONTROL_PROFILE_TRACK,
        { 20, 10, 1, 10 },
        150,
        { 2.0f, 0.008f, 15.0f, 1.0f },
        { 0 }
    }
};

const control_profile_t *Roundabout_GetProfile(uint8_t state)
{
    switch (state)
    {
        case ROUNDABOUT_ISLAND_LPREENTER:
            return &s_round_profiles[ROUNDABOUT_ROUND_PROFILE_PREENTER];
        case ROUNDABOUT_ISLAND_TURN_LEFT:
            return &s_round_profiles[ROUNDABOUT_ROUND_PROFILE_TURN];
        case ROUNDABOUT_ISLAND_IN:
            return &s_round_profiles[ROUNDABOUT_ROUND_PROFILE_IN];
        case ROUNDABOUT_ISLAND_OUT:
            return &s_round_profiles[ROUNDABOUT_ROUND_PROFILE_OUT];
        default:
            return &s_round_profiles[ROUNDABOUT_ROUND_PROFILE_TURN];
    }
}

void Roundabout_Update(void)
{
    switch (g_roundabout_state)
    {
        case ROUNDABOUT_ISLAND_LPREENTER:

            Control_ApplyProfile(&s_round_profiles[ROUNDABOUT_ROUND_PROFILE_PREENTER]);

            (void)Track_UpdateError();
            g_control_track_output = PID_UpdateTrack();

            Motor_DistributeSpeed(g_control_track_output);

            Roundabout_JudgeAhead(); /* 是否满足距离 */
            break;

        case ROUNDABOUT_ISLAND_TURN_LEFT:

            Control_ApplyProfile(&s_round_profiles[ROUNDABOUT_ROUND_PROFILE_TURN]);

            Motor_DistributeSpeed(-30);

            Roundabout_JudgeEntered(); /* 检测距离是否完成 */
            break;

        case ROUNDABOUT_ISLAND_IN:

            Control_ApplyProfile(&s_round_profiles[ROUNDABOUT_ROUND_PROFILE_IN]);

            (void)Track_UpdateError();
            g_control_track_output = PID_UpdateTrack();
            Motor_DistributeSpeed(g_control_track_output);

            Roundabout_JudgeExit();
            break;

        case ROUNDABOUT_ISLAND_OUT:

            Control_ApplyProfile(&s_round_profiles[ROUNDABOUT_ROUND_PROFILE_OUT]);

            (void)Track_UpdateError();
            g_control_track_output = PID_UpdateTrack();

            Motor_DistributeSpeed(g_control_track_output);

            Roundabout_JudgeExited();
            break;
    }
}

void Roundabout_JudgePreentryLeft(void)
{
    const track_state_t *track = Track_GetState();
    if ((track->filtered[0] + track->filtered[3] > 2800))
    {
        if (track->filtered[0] > track->filtered[3])
        {
            g_roundabout_left_flag = 1;
        }
        else
        {
            g_roundabout_right_flag = 1;
        }

        g_control_cask_flag = 0;
        IMU_ZeroAngleX();
        Motor_ResetDistance();
        g_control_kernel_state = CONTROL_KERNEL_REISLAND;
    }

    if (g_app_dl1b_distance_mm < 100) /* DL1B检测到近距障碍，置路障标志 */
    {
        g_control_cask_flag = 1;
    }
}

void Roundabout_JudgePreentryRight(void)
{
    const track_state_t *track = Track_GetState();
    if ((track->filtered[0] + track->filtered[1] + track->filtered[2] + track->filtered[3] > 4500)
        || (track->filtered[0] + track->filtered[3] > 3000))
    {
        IMU_ZeroAngleX();
        g_control_kernel_state = CONTROL_KERNEL_REISLAND;
    }
}

void Roundabout_JudgePreentryExit(void)
{
    if (Motor_GetState()->distance > 5000)
    {
        g_control_kernel_state = CONTROL_KERNEL_TRACKING;
        g_roundabout_left_flag = 0;
        g_roundabout_right_flag = 0;

        g_control_track_output = 0;
        IMU_ZeroAngleX();
    }
}

void Roundabout_JudgeEntryLeft(void)
{
    /* 源工程保留空壳，当前不接入；保持原空操作。 */
}

void Roundabout_JudgeEntryRight(void)
{
    const track_state_t *track = Track_GetState();
    if ((track->filtered[1] + track->filtered[2] < 300) && g_roundabout_right_flag == 1
        && !g_control_cask_flag)
    {
        g_roundabout_state = ROUNDABOUT_ISLAND_LPREENTER;
        g_control_kernel_state = CONTROL_KERNEL_ISLAND_R;

        g_control_track_output = 0;
        IMU_ZeroAngleX();
        Motor_ResetDistance();
    }
}

void Roundabout_JudgeAhead(void)
{
    if (Motor_GetState()->distance >= g_roundabout_enter_distance1)
    {
        g_roundabout_state = ROUNDABOUT_ISLAND_TURN_LEFT;
        Motor_ResetDistance();
    }
}

void Roundabout_JudgeEntered(void)
{
    if (Motor_GetState()->distance > 13000)
    {
        g_roundabout_state = ROUNDABOUT_ISLAND_IN;
        Motor_ResetDistance();
    }
}

void Roundabout_JudgeEnteredAgain(void)
{
    if (float_abs(PID_GetState()->angle_err) < 5.0f) /* 角度误差小于5度，认为入环姿态已调整好 */
    {
        g_roundabout_state = ROUNDABOUT_ISLAND_IN;
    }
}

void Roundabout_JudgePreExit(void)
{
    /* 源工程保留空壳，当前不接入；保持原空操作。 */
}

void Roundabout_JudgeExit(void)
{
    if (Motor_GetState()->distance > 32000)
    {
        g_roundabout_state = ROUNDABOUT_ISLAND_OUT;
        Motor_ResetDistance();
    }
}

void Roundabout_JudgeExited(void)
{
    if (Motor_GetState()->distance > g_roundabout_out_distance1) /* 行驶足够距离，确认出环完成 */
    {
        g_roundabout_left_flag = 0;
        g_roundabout_right_flag = 0;

        g_roundabout_state = ROUNDABOUT_STATE_NORMAL;
        g_control_kernel_state = CONTROL_KERNEL_TRACKING;
    }
}

void Roundabout_Judge(void)
{
    /* 源工程保留空壳，当前不接入；保持原空操作。 */
}

void Roundabout_ResetRunState(void)
{
    g_roundabout_state = ROUNDABOUT_STATE_NORMAL;
    g_roundabout_left_flag = 0;
    g_roundabout_right_flag = 0;
    g_roundabout_sign_round = 1;
}
