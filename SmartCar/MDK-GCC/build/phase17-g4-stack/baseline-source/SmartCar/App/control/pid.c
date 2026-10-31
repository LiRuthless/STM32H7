/**
 * @file pid.c
 * @brief 控制参数和输出协调接口。
 * @details 唯一持有增益和可见输出，算法及历史委托专属模块，不读取传感器或硬件。
 * @note 移植来源：Sirius20260718/project/code/pid.c，STC AI8051U 省赛快照（2026-07-18）。
 * @note 源文件未单列作者及独立许可证；保留出处，关联 Phase 13/15。
 */
#include "control/pid.h"
#include "control/angle_controller.h"
#include "control/speed_controller.h"
#include "control/legacy_pid.h"
/* 增益与可见输出唯一 owner，前台设置、控制拍更新。 */
static pid_state_t s_pid;
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
 * @brief 用模块增益计算纯角度输出。
 * @param[in] target_angle 目标 X 角度，度。
 * @param[in] angle 当前 X 角度，度。
 * @param[in] gyro 当前 X 角速度，度/秒。
 * @return 原角度输出，未取负或限幅。
 * @note 只更新角度历史及 s_pid 误差输出，无电机动作。
 */
float PID_CalculateAngle(int16_t target_angle, float angle, float gyro)
{
    const pid_angle_gains_t gains = { s_pid.kp_a, s_pid.kd_a, s_pid.kg_a };
    AngleController_Update(target_angle, angle, gyro, &gains, &s_pid.angle_err, &s_pid.angle_out);
    return s_pid.angle_out;
}

/**
 * @brief 以模块速度增益计算Left轮 PI。
 * @param[in] target 目标脉冲/2 ms。
 * @param[in] measured 反馈脉冲/2 ms。
 * @return 限幅的整数 PI 输出，范围 ±7500。
 * @note 双轮历史由 speed_controller 独立持有。
 */
int16_t PID_UpdateSpeedLeft(int16_t target, int16_t measured)
{
    return SpeedController_UpdateLeft(target, measured, s_pid.kp_v, s_pid.ki_v, &s_pid.out_l);
}

/**
 * @brief 以模块速度增益计算Right轮 PI。
 * @param[in] target 目标脉冲/2 ms。
 * @param[in] measured 反馈脉冲/2 ms。
 * @return 限幅的整数 PI 输出，范围 ±7500。
 * @note 双轮历史由 speed_controller 独立持有。
 */
int16_t PID_UpdateSpeedRight(int16_t target, int16_t measured)
{
    return SpeedController_UpdateRight(target, measured, s_pid.kp_v, s_pid.ki_v, &s_pid.out_r);
}

/**
 * @brief 清两个活动 PI 的积分、目标历史和输出。
 * @details 分别清左右积分、前拍目标和 out_l/out_r，不修改增益、方向或角度历史。
 * @note 既有停车/起跑路径调用，不清休眠增量式局部静态量。
 */
void PID_ResetSpeed(void)
{
    SpeedController_Reset();
    s_pid.out_l = 0.0f;
    s_pid.out_r = 0.0f;
}

/**
 * @brief 清旧预览方向及角度历史，再复位活动速度 PI。
 * @details 先清前台预览与旧环岛共用的方向历史，再清角度历史和误差/输出，最后清双轮 PI。
 *     两模式影子方向历史由各自 FSM reset 处理，本函数不访问它们。
 * @note 再次起跑前调用，保留所有增益及休眠增量式控制器历史。
 */
void PID_ResetAll(void)
{
    LegacyPID_ResetTrackHistory();
    AngleController_Reset();
    s_pid.angle_err = 0.0f;
    s_pid.angle_out = 0.0f;
    PID_ResetSpeed();
}
