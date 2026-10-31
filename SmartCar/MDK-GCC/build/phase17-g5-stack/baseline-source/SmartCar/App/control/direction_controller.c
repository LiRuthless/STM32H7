/**
 * @file direction_controller.c
 * @brief 无静态历史的方向计算。
 * @details 调用方拥有历史；原四项求和先窄化，再限幅。
 * @note 移植来源：Sirius20260718/project/code/pid.c，STC AI8051U 省赛快照（2026-07-18）。
 * @note 源文件未单列作者及独立许可证；保留出处，关联 Phase 13/15。
 */
#include <stdlib.h>
#include "control/direction_controller.h"
/**
 * @brief 用调用方独立历史计算方向控制。
 * @details 依次计算 kp*e、k2p*e*abs(e)、kd*(e-前拍偏差) 和 k2d*accel。
 *     非线性项为带符号二次项；先保存本拍偏差，再将前三项之和减阻尼项转换为 int16_t 并限幅。
 * @param[in] input 当前循迹偏差，单位为控制算法整数标度。
 * @param[in] accel IMU X 轴加速度的现有算法标度，用作阻尼项输入。
 * @param[in] gains 非空四项增益。
 * @param[in,out] history 非空独立前拍偏差，计算差分后更新。
 * @return 转换有效时的方向输出，-1000..1000；越界浮点转换不保证数学饱和结果。
 * @note TIM6 调用；浮点求和先转换 int16_t，再限幅 ±1000；大值存在转换风险，尚未实施 R01 修复。
 */
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
