/**
 * @file speed_controller.c
 * @brief 独立双轮位置式 PI。
 * @details 显式目标反馈与增益输入；换向清积分、积分限幅、死区补偿、输出限幅依次执行。
 * @note 移植来源：Sirius20260718/project/code/pid.c，STC AI8051U 省赛快照（2026-07-18）。
 * @note 源文件未单列作者及独立许可证；保留出处，关联 Phase 13/15。
 */
#include "control/speed_controller.h"
typedef struct
{
    float integral;      /**< 该轮独立积分，限幅 ±(7500-dead_zone)。 */
    int16_t last_target; /**< 上一拍该轮目标脉冲，换向时先清积分。 */
} pid_speed_pi_state_t;

/* 左右积分与目标历史各自独立，只有原 reset 路径清零。 */
static pid_speed_pi_state_t s_speed_left;
static pid_speed_pi_state_t s_speed_right;
/**
 * @brief 以独立状态执行单轮位置式 PI。
 * @details 目标跨零换向时清积分，先保存目标历史，再计算误差并累加积分；积分限制为
 *     ±(7500-dead_zone)。比例加积分后按目标符号加入死区，输出限制到 ±7500。
 * @param[in,out] state 非空本轮积分和目标历史，不与另一轮共享。
 * @param[in] target 目标，脉冲/2 ms，int16_t。
 * @param[in] measured 反馈，脉冲/2 ms，int16_t。
 * @param[in] dead_zone 原 PWM 死区，当前为 500。
 * @param[in] kp 速度比例增益。
 * @param[in] ki 速度积分增益。
 * @param[out] output 非空浮点输出，限幅 ±7500。
 * @return float→int32_t→int16_t 的原输出值。
 * @note 控制拍调用，无阻塞；换向、历史、积分、死区、限幅顺序不变。
 */
static int16_t SpeedController_Update(
    pid_speed_pi_state_t *state, int16_t target, int16_t measured, int16_t dead_zone,
    float kp, float ki, float *output)
{
    /* 先处理目标换向并更新历史，再计算 PI、限制积分、补偿死区，最后限幅并返回输出。 */
    int32_t error = 0;
    float p_out = 0.0f;
    float d_out = 0.0f;

    if ((target > 0 && state->last_target < 0) || (target < 0 && state->last_target > 0))
    {
        state->integral = 0.0f;
    }
    state->last_target = target;

    error = target - measured;
    p_out = kp * (float)error;
    state->integral += ki * (float)error;

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

/**
 * @brief 更新Left轮独立 PI 历史与 owner 输出。
 * @param[in] target 目标脉冲/2 ms。
 * @param[in] measured 反馈脉冲/2 ms。
 * @param[in] kp 比例增益。
 * @param[in] ki 积分增益。
 * @param[out] output 非空 owner 输出，写入限幅结果。
 * @return 输出先转 int32_t 再转 int16_t，范围 ±7500。
 */
int16_t SpeedController_UpdateLeft(int16_t target, int16_t measured,
    float kp, float ki, float *output)
{
    return SpeedController_Update(&s_speed_left, target, measured,
        PID_MOTOR_DEAD_ZONE_LEFT, kp, ki, output);
}

/**
 * @brief 更新Right轮独立 PI 历史与 owner 输出。
 * @param[in] target 目标脉冲/2 ms。
 * @param[in] measured 反馈脉冲/2 ms。
 * @param[in] kp 比例增益。
 * @param[in] ki 积分增益。
 * @param[out] output 非空 owner 输出，写入限幅结果。
 * @return 输出先转 int32_t 再转 int16_t，范围 ±7500。
 */
int16_t SpeedController_UpdateRight(int16_t target, int16_t measured,
    float kp, float ki, float *output)
{
    return SpeedController_Update(&s_speed_right, target, measured,
        PID_MOTOR_DEAD_ZONE_RIGHT, kp, ki, output);
}

/**
 * @brief 按左积分、右积分、左目标、右目标顺序清活动 PI 历史。
 */
void SpeedController_Reset(void)
{
    s_speed_left.integral = 0.0f;
    s_speed_right.integral = 0.0f;
    s_speed_left.last_target = 0;
    s_speed_right.last_target = 0;
}
