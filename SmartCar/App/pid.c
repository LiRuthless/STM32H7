/**
 * @file pid.c
 * @brief App 方向、角度及独立双轮 PI；保留休眠增量式控制器。
 * @note 移植来源：Sirius20260718/project/code/pid.c，STC AI8051U 省赛快照（2026-07-18）。
 * @note 源算法文件未单列作者及独立许可证；保留原出处，关联 Phase 13/15 规约。
 */
#include "bsp.h"
#include "app_config.h"
#include <stdlib.h>
#include "pid.h"
#include "imu_proc.h"
#include "track_sensor.h"
#include "motor.h"

typedef struct
{
    float integral;      /**< 该轮独立积分，限幅 ±(7500-dead_zone)。 */
    int16_t last_target; /**< 上一拍该轮目标脉冲，换向时先清积分。 */
} pid_speed_pi_state_t;

/* TIM6 更新活动输出和独立双轮历史；停车复位不改增益或休眠局部静态量。 */
static pid_state_t s_pid;

static pid_speed_pi_state_t s_speed_left;
static pid_speed_pi_state_t s_speed_right;

/* 活动控制器历史状态集中保存，允许安全停车与再次起跑显式复位。 */
static int32_t s_track_error_last = 0;
static float s_angle_err_last = 0.0f;

static int16_t PID_UpdateSpeed(pid_speed_pi_state_t *state,
    int16_t target,
    int16_t measured,
    int16_t dead_zone,
    float *output);

/* 左右轮共用位置式 PI 计算，历史状态分别由两个实例持有。 */

const pid_state_t *PID_GetState(void)
{
    return &s_pid;
}

void PID_SetGain(pid_gain_id_t id, float value)
{
    switch (id)
    {
        case PID_GAIN_KP_V:
            s_pid.kp_v = value;
            break;
        case PID_GAIN_KI_V:
            s_pid.ki_v = value;
            break;
        case PID_GAIN_KD_V:
            s_pid.kd_v = value;
            break;
        case PID_GAIN_KP_X:
            s_pid.kp_x = value;
            break;
        case PID_GAIN_K2P_X:
            s_pid.k2p_x = value;
            break;
        case PID_GAIN_KI_X:
            s_pid.ki_x = value;
            break;
        case PID_GAIN_KD_X:
            s_pid.kd_x = value;
            break;
        case PID_GAIN_K2D_X:
            s_pid.k2d_x = value;
            break;
        case PID_GAIN_KP_A:
            s_pid.kp_a = value;
            break;
        case PID_GAIN_KD_A:
            s_pid.kd_a = value;
            break;
        case PID_GAIN_KG_A:
            s_pid.kg_a = value;
            break;
        default:
            break;
    }
}

void PID_SetTrackGains(const pid_track_gains_t *gains)
{
    s_pid.kp_x = gains->kp;
    s_pid.k2p_x = gains->k2p;
    s_pid.kd_x = gains->kd;
    s_pid.k2d_x = gains->k2d;
}

void PID_SetAngleGains(const pid_angle_gains_t *gains)
{
    s_pid.kp_a = gains->kp;
    s_pid.kd_a = gains->kd;
    s_pid.kg_a = gains->kg;
}

int16_t PID_DirectionStep(
    int16_t input, float accel, const pid_track_gains_t *gains, int32_t *history)
{
    int32_t error = 0.0;

    float p_out = 0.0;  /* P环节输出 */
    float p2_out = 0.0; /* P2环节输出（非线性项） */
    float d_out = 0.0;  /* D环节输出 */
    float d2_out = 0.0;

    int16_t pid_out = 0.0; /* PID总输出 */

    error = input;

    p_out = gains->kp * (float)error;
    p2_out = gains->k2p * (float)error * abs(error);
    d_out = gains->kd * (float)(error - *history);
    d2_out = gains->k2d * accel;

    *history = error;

    pid_out = (p_out + p2_out /*I_out*/ + d_out - d2_out); /* 汇总PID各环节输出 */

    if (pid_out > PID_MAX_DIR_OUT)
    {
        pid_out = PID_MAX_DIR_OUT; /* 输出限幅上限 */
    }
    if (pid_out < -PID_MAX_DIR_OUT)
    {
        pid_out = -PID_MAX_DIR_OUT; /* 输出限幅下限 */
    }

    return pid_out; /* 返回PID控制输出 */
}

int16_t PID_UpdateTrack(void)
{
    const pid_track_gains_t gains = { s_pid.kp_x, s_pid.k2p_x, s_pid.kd_x, s_pid.k2d_x };
    return PID_DirectionStep(
        Track_GetState()->error, IMU_GetState()->accel[0], &gains, &s_track_error_last);
}

void PID_UpdateAngle(int16_t target_angle)
{
    s_pid.angle_err = target_angle - IMU_GetState()->angle[0]; /* 角度误差 = 目标角度 - 当前角度 */

    s_pid.angle_out = s_pid.kp_a * s_pid.angle_err
        + s_pid.kd_a * (s_pid.angle_err - s_angle_err_last)
        - s_pid.kg_a * IMU_GetState()->gyro[0]; /* 角度PD控制：角度误差比例+微分-陀螺仪阻尼 */

    s_angle_err_last = s_pid.angle_err;

    Motor_DistributeSpeed(-s_pid.angle_out);
}

int16_t PID_UpdateIncrementalLeft(void)
{
    static float s_p_out_left = 0.0;   /* P环节输出 */
    static float s_i_out_left = 0.0;   /* I环节输出 */
    static float s_d_out_left = 0.0;   /* D环节输出 */
    static float s_pid_sum_left = 0.0; /* PID累计输出 */

    static int32_t s_error_left = 0;      /* 当前偏差 */
    static int32_t s_last_error_left = 0; /* 上次偏差 */

    s_error_left = Motor_GetState()->left.target_speed
        - Motor_GetState()->left.real_speed; /* 计算速度偏差 = 目标速度 - 实际速度 */

    s_p_out_left = s_pid.kp_v
        * (float)(s_error_left - s_last_error_left); /* P环节：比例控制（基于偏差变化率） */
    s_i_out_left = s_pid.ki_v * (float)s_error_left; /* I环节：积分控制（累积偏差） */
    /* 当前不接入；源占位 Previous_errorL 本模块未定义。
     * s_d_out_left = s_pid.kd_v * (float)(s_error_left - 2 * s_last_error_left
     *     + Previous_errorL);
     */

    s_pid_sum_left += (s_p_out_left + s_i_out_left + s_d_out_left); /* PID总输出累加 */

    s_last_error_left = s_error_left; /* 更新上次偏差 */

    if (s_pid_sum_left > PID_MAX_SPD_OUT)
    {
        s_pid_sum_left = PID_MAX_SPD_OUT; /* 输出限幅上限 */
    }
    if (s_pid_sum_left < -PID_MAX_SPD_OUT)
    {
        s_pid_sum_left = -PID_MAX_SPD_OUT; /* 输出限幅下限 */
    }

    return (int32_t)s_pid_sum_left; /* 返回左轮电机PWM控制值 */
}

int16_t PID_UpdateIncrementalRight(void)
{
    static float s_p_out_right = 0.0;   /* P环节输出 */
    static float s_i_out_right = 0.0;   /* I环节输出 */
    static float s_d_out_right = 0.0;   /* D环节输出 */
    static float s_pid_sum_right = 0.0; /* PID累计输出 */

    static int32_t s_error_right = 0;      /* 当前偏差 */
    static int32_t s_last_error_right = 0; /* 上次偏差 */

    s_error_right = Motor_GetState()->right.target_speed
        - Motor_GetState()->right.real_speed; /* 计算速度偏差 = 目标速度 - 实际速度 */

    s_p_out_right = s_pid.kp_v * (float)(s_error_right - s_last_error_right); /* P环节：比例控制 */
    s_i_out_right = s_pid.ki_v * (float)s_error_right;                        /* I环节：积分控制 */
    /* 当前不接入；源占位 Previous_errorR 本模块未定义。
     * s_d_out_right = s_pid.kd_v * (float)(s_error_right - 2 * s_last_error_right
     *     + Previous_errorR);
     */

    s_pid_sum_right += (s_p_out_right + s_i_out_right + s_d_out_right); /* PID总输出累加 */

    s_last_error_right = s_error_right; /* 更新上次偏差 */

    if (s_pid_sum_right > PID_MAX_SPD_OUT)
    {
        s_pid_sum_right = PID_MAX_SPD_OUT; /* 输出限幅上限 */
    }
    if (s_pid_sum_right < -PID_MAX_SPD_OUT)
    {
        s_pid_sum_right = -PID_MAX_SPD_OUT; /* 输出限幅下限 */
    }

    return (int32_t)s_pid_sum_right; /* 返回右轮电机PWM控制值 */
}

/**
 * @brief 以独立状态执行单轮位置式 PI。
 * @param[in,out] state 非空本轮积分和目标历史，不与另一轮共享。
 * @param[in] target 目标，脉冲/2 ms，int16_t。
 * @param[in] measured 反馈，脉冲/2 ms，int16_t。
 * @param[in] dead_zone 原 PWM 死区，当前为 500。
 * @param[out] output 非空浮点输出，限幅 ±7500。
 * @return float→int32_t→int16_t 的原输出值。
 * @note 控制拍调用，无阻塞；换向、历史、积分、死区、限幅顺序不变。
 */
static int16_t PID_UpdateSpeed(
    pid_speed_pi_state_t *state, int16_t target, int16_t measured, int16_t dead_zone, float *output)
{
    int32_t error = 0;
    float p_out = 0.0f;
    float d_out = 0.0f;

    if ((target > 0 && state->last_target < 0) || (target < 0 && state->last_target > 0))
    {
        state->integral = 0.0f;
    }
    state->last_target = target;

    error = target - measured;
    p_out = s_pid.kp_v * (float)error;
    state->integral += s_pid.ki_v * (float)error;

    if (state->integral > (PID_MAX_SPD_OUT - dead_zone))
    {
        state->integral = (PID_MAX_SPD_OUT - dead_zone);
    }
    if (state->integral < -(PID_MAX_SPD_OUT - dead_zone))
    {
        state->integral = -(PID_MAX_SPD_OUT - dead_zone);
    }

    *output = p_out + state->integral + d_out;
    if (target > 0)
    {
        *output += dead_zone;
    }
    else if (target < 0)
    {
        *output -= dead_zone;
    }

    if (*output > PID_MAX_SPD_OUT)
    {
        *output = PID_MAX_SPD_OUT;
    }
    if (*output < -PID_MAX_SPD_OUT)
    {
        *output = -PID_MAX_SPD_OUT;
    }

    return (int32_t)*output;
}

/* 左轮位置式速度 PI 入口。 */
int16_t PID_UpdateSpeedLeft(void)
{
    const motor_state_t *motor = Motor_GetState();
    return PID_UpdateSpeed(&s_speed_left, motor->left.target_speed, motor->left.real_speed,
        PID_MOTOR_DEAD_ZONE_LEFT, &s_pid.out_l);
}

/* 右轮位置式速度 PI 入口。 */
int16_t PID_UpdateSpeedRight(void)
{
    const motor_state_t *motor = Motor_GetState();
    return PID_UpdateSpeed(&s_speed_right, motor->right.target_speed, motor->right.real_speed,
        PID_MOTOR_DEAD_ZONE_RIGHT, &s_pid.out_r);
}

void PID_ResetSpeed(void)
{
    s_speed_left.integral = 0.0f;
    s_speed_right.integral = 0.0f;
    s_speed_left.last_target = 0;
    s_speed_right.last_target = 0;
    s_pid.out_l = 0.0f;
    s_pid.out_r = 0.0f;
}

void PID_ResetAll(void)
{
    s_track_error_last = 0;
    s_angle_err_last = 0.0f;
    s_pid.angle_err = 0.0f;
    s_pid.angle_out = 0.0f;
    PID_ResetSpeed();
}
