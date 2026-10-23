/**
 * @file pid.c
 * @brief App 方向、角度及独立双轮 PI；保留休眠增量式控制器。
 * @details 实现方向非线性 PD、左右速度位置式 PI 和保留的增量式 PID；活动控制在 TIM6 更新，前台负责参数设置和起跑/停车复位。
 * @note 移植来源：Sirius20260718/project/code/pid.c，STC AI8051U 省赛快照（2026-07-18）。
 * @note 源算法文件未单列作者及独立许可证；保留原出处，关联 Phase 13/15 规约。
 */
#include "bsp.h"
#include "app_config.h"
#include <stdlib.h>
#include "control/pid.h"
#include "sensing/imu_proc.h"
#include "sensing/track_sensor.h"
#include "control/motor.h"

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

/**
 * @brief 读取模块持有的只读运行状态。
 * @return 非空只读指针，不转移所有权。
 * @note 静态对象寿命为整个程序；控制拍可更新，前台一致性读取须遵守既有停车约定。
 */
const pid_state_t *PID_GetState(void)
{
    return &s_pid;
}

/**
 * @brief 按枚举更新单项增益。
 * @details 按增益枚举只更新对应的一个 PID 参数；未知枚举进入 default 且不修改任何增益，历史状态始终保留。
 * @param[in] id 要更新的 pid_gain_id_t 枚举项。
 * @param[in] value 写入所选增益的单精度数，不作范围钳位。
 * @note 参数切换的既有上下文调用，不重置历史。
 */
void PID_SetGain(pid_gain_id_t id, float value)
{
    /* switch 只写枚举指定的单项增益；default 不写状态，所有分支都保留控制历史。 */
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

/**
 * @brief 更新四项方向增益，保留控制历史。
 * @details 更新方向环的比例、微分和非线性系数；只改变后续控制拍使用的参数，不清方向环历史。
 * @param[in] gains 非空方向增益，不改历史或其他增益。
 * @note 启动、档位切换或状态剖面应用时调用；写 s_pid 方向增益，不访问硬件。
 */
void PID_SetTrackGains(const pid_track_gains_t *gains)
{
    s_pid.kp_x = gains->kp;
    s_pid.k2p_x = gains->k2p;
    s_pid.kd_x = gains->kd;
    s_pid.k2d_x = gains->k2d;
}

/**
 * @brief 更新三项角度增益，保留控制历史。
 * @details 更新角度环比例和微分系数；只改变后续计算参数，不清零偏差历史或输出状态。
 * @param[in] gains 非空角度增益，不改历史或其他增益。
 * @note 状态剖面应用时调用；写 s_pid 角度增益，不访问硬件。
 */
void PID_SetAngleGains(const pid_angle_gains_t *gains)
{
    s_pid.kp_a = gains->kp;
    s_pid.kd_a = gains->kd;
    s_pid.kg_a = gains->kg;
}

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

/**
 * @brief 用旧共享历史计算电磁方向输出。
 * @details 从模块参数构造方向增益，读取电磁偏差和 IMU X 轴加速度，调用方向计算并更新共享历史。
 * @return 转换有效时的方向输出，-1000..1000；越界浮点转换不保证数学饱和结果。
 * @note 保留旧入口和调用边，当前观察器使用各自独立历史。
 */
int16_t PID_UpdateTrack(void)
{
    const pid_track_gains_t gains = { s_pid.kp_x, s_pid.k2p_x, s_pid.kd_x, s_pid.k2d_x };
    return PID_DirectionStep(
        Track_GetState()->error, IMU_GetState()->accel[0], &gains, &s_track_error_last);
}

/**
 * @brief 计算原角度 PD 并分配差速。
 * @details 按目标角度减当前 X 轴角度得到误差，计算比例加拍间差分再减角速度阻尼。
 *     保存误差历史后，把浮点输出取负传给 Motor_DistributeSpeed；本函数没有方向限幅。
 * @param[in] target_angle 目标 X 轴角度，单位为度。
 * @note 保留的角度控制入口；浮点输出向差速接口整数形参转换存在范围约束。
 */
void PID_UpdateAngle(int16_t target_angle)
{
    s_pid.angle_err = target_angle - IMU_GetState()->angle[0]; /* 角度误差 = 目标角度 - 当前角度 */

    s_pid.angle_out = s_pid.kp_a * s_pid.angle_err
        + s_pid.kd_a * (s_pid.angle_err - s_angle_err_last)
        - s_pid.kg_a * IMU_GetState()->gyro[0]; /* 角度PD控制：角度误差比例+微分-陀螺仪阻尼 */

    s_angle_err_last = s_pid.angle_err;

    Motor_DistributeSpeed(-s_pid.angle_out);
}

/**
 * @brief 保留左轮增量式速度控制入口。
 * @details 读取该轮目标和反馈，累计 kp*(本拍误差-前拍误差)+ki*本拍误差；
 *     更新前拍误差后把累计值限制到 ±7500，微分占位保持零。
 * @return 原浮点累计经 int32_t 再转 int16_t，范围 ±7500。
 * @note 当前不接入；局部静态历史保留，微分代码仍注释，ResetAll 不新增复位。
 */
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

/**
 * @brief 保留右轮增量式速度控制入口。
 * @details 读取该轮目标和反馈，累计 kp*(本拍误差-前拍误差)+ki*本拍误差；
 *     更新前拍误差后把累计值限制到 ±7500，微分占位保持零。
 * @return 原浮点累计经 int32_t 再转 int16_t，范围 ±7500。
 * @note 当前不接入；局部静态历史保留，微分代码仍注释，ResetAll 不新增复位。
 */
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
 * @details 目标跨零换向时清积分，先保存目标历史，再计算误差并累加积分；积分限制为
 *     ±(7500-dead_zone)。比例加积分后按目标符号加入死区，输出限制到 ±7500。
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
/**
 * @brief 更新左轮独立活动位置式 PI。
 * @details 读取该轮目标及反馈，以该轮独立积分/目标历史执行位置式 PI，并写入对应输出缓存。
 * @return 原浮点输出经 int32_t 再转 int16_t，范围 ±7500。
 * @note TIM6 调用；目标换向先清积分，保持死区、积分限幅和输出转换顺序。
 */
int16_t PID_UpdateSpeedLeft(void)
{
    const motor_state_t *motor = Motor_GetState();
    return PID_UpdateSpeed(&s_speed_left, motor->left.target_speed, motor->left.real_speed,
        PID_MOTOR_DEAD_ZONE_LEFT, &s_pid.out_l);
}

/* 右轮位置式速度 PI 入口。 */
/**
 * @brief 更新右轮独立活动位置式 PI。
 * @details 读取该轮目标及反馈，以该轮独立积分/目标历史执行位置式 PI，并写入对应输出缓存。
 * @return 原浮点输出经 int32_t 再转 int16_t，范围 ±7500。
 * @note TIM6 调用；目标换向先清积分，保持死区、积分限幅和输出转换顺序。
 */
int16_t PID_UpdateSpeedRight(void)
{
    const motor_state_t *motor = Motor_GetState();
    return PID_UpdateSpeed(&s_speed_right, motor->right.target_speed, motor->right.real_speed,
        PID_MOTOR_DEAD_ZONE_RIGHT, &s_pid.out_r);
}

/**
 * @brief 清两个活动 PI 的积分、目标历史和输出。
 * @details 分别清左右积分、前拍目标和 out_l/out_r，不修改增益、方向或角度历史。
 * @note 既有停车/起跑路径调用，不清休眠增量式局部静态量。
 */
void PID_ResetSpeed(void)
{
    s_speed_left.integral = 0.0f;
    s_speed_right.integral = 0.0f;
    s_speed_left.last_target = 0;
    s_speed_right.last_target = 0;
    s_pid.out_l = 0.0f;
    s_pid.out_r = 0.0f;
}

/**
 * @brief 清活动方向、角度历史并复位速度 PI。
 * @details 清共享电磁方向历史、角度历史及角度输出，再清双轮活动 PI 的积分、目标历史和输出。
 * @note 再次起跑前调用，保留所有增益及休眠控制器历史。
 */
void PID_ResetAll(void)
{
    s_track_error_last = 0;
    s_angle_err_last = 0.0f;
    s_pid.angle_err = 0.0f;
    s_pid.angle_out = 0.0f;
    PID_ResetSpeed();
}
