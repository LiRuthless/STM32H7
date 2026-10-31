/**
 * @file legacy_pid.c
 * @brief 旧共享方向与休眠增量式算法。
 * @details 显式输入避免硬件依赖；增量式历史保留且不参与活动 reset。
 * @note 移植来源：Sirius20260718/project/code/pid.c，STC AI8051U 省赛快照（2026-07-18）。
 * @note 源文件未单列作者及独立许可证；保留出处，关联 Phase 13/15。
 */
#include "control/legacy_pid.h"
#include "control/direction_controller.h"
/* 旧共享方向历史，前台预览与旧环岛共用，ResetAll 单独清零。 */
static int32_t s_track_error_last;
/**
 * @brief 用旧共享历史计算电磁方向输出。
 * @details 从模块参数构造方向增益，使用显式电磁偏差和 IMU X 轴加速度，调用方向计算并更新共享历史。
 * @param[in] error 当前电磁偏差。
 * @param[in] accel 当前 IMU X 加速度原标度。
 * @return 转换有效时的方向输出，-1000..1000；越界浮点转换不保证数学饱和结果。
 * @note 停车前台预览仍调用本入口，旧环岛保留原调用；活动双模式观察器使用各自独立历史。
 */
int16_t PID_UpdateTrack(int16_t error, float accel)
{
    const pid_state_t *pid = PID_GetState();
    const pid_track_gains_t gains = { pid->kp_x, pid->k2p_x, pid->kd_x, pid->k2d_x };
    return PID_DirectionStep(
        error, accel, &gains, &s_track_error_last);
}

/**
 * @brief 保留左轮增量式速度控制入口。
 * @details 读取该轮目标和反馈，累计 kp*(本拍误差-前拍误差)+ki*本拍误差；
 *     更新前拍误差后把累计值限制到 ±7500，微分占位保持零。
 * @param[in] target 目标脉冲/2 ms。
 * @param[in] measured 反馈脉冲/2 ms。
 * @return 原浮点累计经 int32_t 再转 int16_t，范围 ±7500。
 * @note 当前不接入；局部静态历史保留，微分代码仍注释，ResetAll 不新增复位。
 */
int16_t PID_UpdateIncrementalLeft(int16_t target, int16_t measured)
{
    const pid_state_t *pid = PID_GetState();
    static float s_p_out_left = 0.0;   /* P环节输出 */
    static float s_i_out_left = 0.0;   /* I环节输出 */
    static float s_d_out_left = 0.0;   /* D环节输出 */
    static float s_pid_sum_left = 0.0; /* PID累计输出 */

    static int32_t s_error_left = 0;      /* 当前偏差 */
    static int32_t s_last_error_left = 0; /* 上次偏差 */

    s_error_left = target - measured; /* 计算速度偏差 = 目标速度 - 实际速度 */

    s_p_out_left = pid->kp_v
        * (float)(s_error_left - s_last_error_left); /* P环节：比例控制（基于偏差变化率） */
    s_i_out_left = pid->ki_v * (float)s_error_left; /* I环节：积分控制（累积偏差） */
    /* 当前不接入；源占位 Previous_errorL 本模块未定义。
     * s_d_out_left = pid->kd_v * (float)(s_error_left - 2 * s_last_error_left
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

/**
 * @brief 保留右轮增量式速度控制入口。
 * @details 读取该轮目标和反馈，累计 kp*(本拍误差-前拍误差)+ki*本拍误差；
 *     更新前拍误差后把累计值限制到 ±7500，微分占位保持零。
 * @param[in] target 目标脉冲/2 ms。
 * @param[in] measured 反馈脉冲/2 ms。
 * @return 原浮点累计经 int32_t 再转 int16_t，范围 ±7500。
 * @note 当前不接入；局部静态历史保留，微分代码仍注释，ResetAll 不新增复位。
 */
int16_t PID_UpdateIncrementalRight(int16_t target, int16_t measured)
{
    const pid_state_t *pid = PID_GetState();
    static float s_p_out_right = 0.0;   /* P环节输出 */
    static float s_i_out_right = 0.0;   /* I环节输出 */
    static float s_d_out_right = 0.0;   /* D环节输出 */
    static float s_pid_sum_right = 0.0; /* PID累计输出 */

    static int32_t s_error_right = 0;      /* 当前偏差 */
    static int32_t s_last_error_right = 0; /* 上次偏差 */

    s_error_right = target - measured; /* 计算速度偏差 = 目标速度 - 实际速度 */

    s_p_out_right = pid->kp_v * (float)(s_error_right - s_last_error_right); /* P环节：比例控制 */
    s_i_out_right = pid->ki_v * (float)s_error_right;                        /* I环节：积分控制 */
    /* 当前不接入；源占位 Previous_errorR 本模块未定义。
     * s_d_out_right = pid->kd_v * (float)(s_error_right - 2 * s_last_error_right
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
 * @brief 只清旧共享方向历史，不清休眠增量式局部状态。
 */
void LegacyPID_ResetTrackHistory(void)
{
    s_track_error_last = 0;
}
