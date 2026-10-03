// ============================================================
// 文件名: pid.c
// 功能说明: PID控制器实现模块（移植自 8051 源工程，算法原样照搬）
// 包含位置式循迹PID（带非线性P2项与accel_x阻尼）、增量式速度PID
// 和位置式速度PID（左右轮独立），用于电磁循迹方向控制与速度闭环。
// ============================================================

#include "bsp.h"
#include "app_config.h"
#include <stdlib.h>
#include "pid.h"
#include "imu_proc.h"
#include "track_sensor.h"
#include "motor.h"

static pid_state_t s_pid;

typedef struct {
    float integral;
    int16_t last_target;
} speed_pi_state_t;

static speed_pi_state_t s_speed_left;
static speed_pi_state_t s_speed_right;

const pid_state_t *PID_GetState(void)
{
    return &s_pid;
}

void PID_SetGain(pid_gain_id_t id, float value)
{
    switch(id)
    {
        case PID_GAIN_KP_V: s_pid.KP_v = value; break;
        case PID_GAIN_KI_V: s_pid.KI_v = value; break;
        case PID_GAIN_KD_V: s_pid.KD_v = value; break;
        case PID_GAIN_KP_X: s_pid.KP_x = value; break;
        case PID_GAIN_K2P_X: s_pid.K2P_x = value; break;
        case PID_GAIN_KI_X: s_pid.KI_x = value; break;
        case PID_GAIN_KD_X: s_pid.KD_x = value; break;
        case PID_GAIN_K2D_X: s_pid.K2D_x = value; break;
        case PID_GAIN_KP_A: s_pid.KP_a = value; break;
        case PID_GAIN_KD_A: s_pid.KD_a = value; break;
        case PID_GAIN_KG_A: s_pid.KG_a = value; break;
        default: break;
    }
}

void PID_SetTrackGains(const pid_track_gains_t *gains)
{
    s_pid.KP_x = gains->kp;
    s_pid.K2P_x = gains->k2p;
    s_pid.KD_x = gains->kd;
    s_pid.K2D_x = gains->k2d;
}

void PID_SetAngleGains(const pid_angle_gains_t *gains)
{
    s_pid.KP_a = gains->kp;
    s_pid.KD_a = gains->kd;
    s_pid.KG_a = gains->kg;
}

/* 活动控制器历史状态集中保存，允许安全停车与再次起跑显式复位。 */
static int32_t s_track_error_last = 0;
static float s_angle_err_last = 0.0f;


// 函数名: PID_track
// 功能: 位置式循迹PID控制器
// 返回值: 方向控制输出，范围[-MAX_DIR_OUT, MAX_DIR_OUT]
// 说明: 采用改进型位置式PID，增加了非线性P2项（K2P_x * error * |error|），
//       在小偏差时响应柔和，大偏差时快速修正；K2D_x*accel_x 为前向加速度阻尼项。
int16_t PID_track(void)
{
	const track_state_t *track = Track_GetState();
	const imu_state_t *imu = IMU_GetState();
	int32_t error = 0.0;

    float P_out = 0.0;       // P环节输出
    float P2_out = 0.0;      // P2环节输出（非线性项）
    float D_out = 0.0;       // D环节输出
	float D2_out = 0.0;

    int16_t PID_out = 0.0;   // PID总输出

    error = track->error;            // 更新当前偏差

    P_out  = s_pid.KP_x  * (float) error;                       // 计算P环节输出
    P2_out = s_pid.K2P_x * (float) error * abs(error);          // 计算非线性P2项
    D_out  = s_pid.KD_x  * (float)(error - s_track_error_last); // 计算D环节输出（微分项）
	D2_out = s_pid.K2D_x * imu->accel[0];

    s_track_error_last = error;      // 更新上次偏差，供下次微分计算使用

    PID_out = (P_out + P2_out/*I_out*/ + D_out - D2_out);        // 汇总PID各环节输出

    if(PID_out >  MAX_DIR_OUT) PID_out =  MAX_DIR_OUT;      // 输出限幅上限
    if(PID_out < -MAX_DIR_OUT) PID_out = -MAX_DIR_OUT;      // 输出限幅下限

    return PID_out;                 // 返回PID控制输出
}


// 函数名: PID_angle
// 功能: 角度环PD控制器（用于环岛等需要固定角度转向的场景）
// 参数: target_angle - 目标角度（单位：度）
// 说明: 根据目标角度与当前角度的偏差计算方向输出，并叠加陀螺仪阻尼项。
//       输出通过 speed_control() 分配左右轮速度，实现按角度转向。
void PID_angle(int16_t target_angle)
{
	s_pid.angle_err = target_angle - IMU_GetState()->angle[0];  // 角度误差 = 目标角度 - 当前角度

	s_pid.angle_out = s_pid.KP_a * s_pid.angle_err + s_pid.KD_a * (s_pid.angle_err - s_angle_err_last) - s_pid.KG_a * IMU_GetState()->gyro[0];  // 角度PD控制：角度误差比例+微分-陀螺仪阻尼

	s_angle_err_last = s_pid.angle_err;

	speed_control( -s_pid.angle_out );
}



// 函数名: PID_L
// 功能: 左轮增量式速度PID控制器（本工程默认未用，保留照搬）
// 返回值: 左轮电机PWM控制增量输出，范围[-MAX_SPD_OUT, MAX_SPD_OUT]
// 说明: 采用增量式PID算法，输出为PWM增量，抗积分饱和。
int16_t PID_L(void)
{
    static float P_outL = 0.0,      // P环节输出
                 I_outL = 0.0,      // I环节输出
                 D_outL = 0.0,      // D环节输出
				 PID_sumL = 0.0;    // PID累计输出

    static int32_t errorL = 0,          // 当前偏差
                 Last_errorL = 0;       // 上次偏差

    errorL = Motor_GetState()->left.target_speed - Motor_GetState()->left.real_speed;     // 计算速度偏差 = 目标速度 - 实际速度

    P_outL = s_pid.KP_v * (float)(errorL - Last_errorL);     // P环节：比例控制（基于偏差变化率）
    I_outL = s_pid.KI_v * (float) errorL;                    // I环节：积分控制（累积偏差）
//    D_outL = s_pid.KD_v * (float)(errorL - 2 * Last_errorL + Previous_errorL); // D环节：微分控制（未启用）

    PID_sumL += (P_outL + I_outL + D_outL);     // PID总输出累加

    Last_errorL = errorL;                       // 更新上次偏差

    if(PID_sumL >  MAX_SPD_OUT) PID_sumL =  MAX_SPD_OUT;      // 输出限幅上限
    if(PID_sumL < -MAX_SPD_OUT) PID_sumL = -MAX_SPD_OUT;      // 输出限幅下限

    return (int32_t)PID_sumL;                            // 返回左轮电机PWM控制值
}

// 函数名: PID_R
// 功能: 右轮增量式速度PID控制器（本工程默认未用，保留照搬）
// 说明: 与PID_L对称，实现右轮独立速度闭环控制。
int16_t PID_R(void)
{
    static float P_outR = 0.0,      // P环节输出
                 I_outR = 0.0,      // I环节输出
                 D_outR = 0.0,      // D环节输出
				 PID_sumR = 0.0;    // PID累计输出

    static int32_t errorR = 0,          // 当前偏差
                 Last_errorR = 0;       // 上次偏差

    errorR = Motor_GetState()->right.target_speed - Motor_GetState()->right.real_speed;     // 计算速度偏差 = 目标速度 - 实际速度

    P_outR = s_pid.KP_v * (float)(errorR - Last_errorR);     // P环节：比例控制
    I_outR = s_pid.KI_v * (float) errorR;                    // I环节：积分控制
//    D_outR = s_pid.KD_v * (float)(errorR - 2 * Last_errorR + Previous_errorR); // D环节：微分控制（未启用）

    PID_sumR += (P_outR + I_outR + D_outR);     // PID总输出累加

    Last_errorR = errorR;                       // 更新上次偏差

    if(PID_sumR >  MAX_SPD_OUT) PID_sumR =  MAX_SPD_OUT;      // 输出限幅上限
    if(PID_sumR < -MAX_SPD_OUT) PID_sumR = -MAX_SPD_OUT;      // 输出限幅下限

    return (int32_t)PID_sumR;                            // 返回右轮电机PWM控制值
}

// 左右轮共用位置式 PI 计算，历史状态分别由两个实例持有。
static int16_t PID_speed(speed_pi_state_t *state, int16_t target,
                             int16_t measured, int16_t dead_zone, float *output)
{
    int32_t error = 0;
    float p_out = 0.0f;
    float d_out = 0.0f;

    if ((target > 0 && state->last_target < 0) ||
        (target < 0 && state->last_target > 0))
    {
        state->integral = 0.0f;
    }
    state->last_target = target;

    error = target - measured;
    p_out = s_pid.KP_v * (float)error;
    state->integral += s_pid.KI_v * (float)error;

    if(state->integral >  (MAX_SPD_OUT - dead_zone)) state->integral =  (MAX_SPD_OUT - dead_zone);
    if(state->integral < -(MAX_SPD_OUT - dead_zone)) state->integral = -(MAX_SPD_OUT - dead_zone);

    *output = p_out + state->integral + d_out;
    if      (target > 0) *output += dead_zone;
    else if (target < 0) *output -= dead_zone;

    if(*output >  MAX_SPD_OUT) *output =  MAX_SPD_OUT;
    if(*output < -MAX_SPD_OUT) *output = -MAX_SPD_OUT;

    return (int32_t)*output;
}

// 左轮位置式速度 PI 入口。
int16_t PID_L_pos(void)
{
    const motor_state_t *motor = Motor_GetState();
    return PID_speed(&s_speed_left, motor->left.target_speed,
                         motor->left.real_speed, MOTOR_DEAD_ZONE_L, &s_pid.out_l);
}

// 右轮位置式速度 PI 入口。
int16_t PID_R_pos(void)
{
    const motor_state_t *motor = Motor_GetState();
    return PID_speed(&s_speed_right, motor->right.target_speed,
                         motor->right.real_speed, MOTOR_DEAD_ZONE_R, &s_pid.out_r);
}

// 函数名: PID_ResetSpeed
// 功能: 清除活动位置式速度 PI 的全部历史状态与可观测输出
void PID_ResetSpeed(void)
{
    s_speed_left.integral = 0.0f;
    s_speed_right.integral = 0.0f;
    s_speed_left.last_target = 0;
    s_speed_right.last_target = 0;
    s_pid.out_l = 0.0f;
    s_pid.out_r = 0.0f;
}

// 函数名: PID_ResetAll
// 功能: 再次起跑前复位当前实际使用的方向、角度和速度控制器历史
void PID_ResetAll(void)
{
    s_track_error_last = 0;
    s_angle_err_last = 0.0f;
    s_pid.angle_err = 0.0f;
    s_pid.angle_out = 0.0f;
    PID_ResetSpeed();
}
