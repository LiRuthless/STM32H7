/**
 * @file pid.h
 * @brief 控制参数与只读输出。
 * @details 声明增益与可见输出访问及算法协调接口；pid.c持有参数，方向、角度、轮速历史各归算法模块。
 */

#ifndef SMARTCAR_APP_CONTROL_PID_H
#define SMARTCAR_APP_CONTROL_PID_H

#include <stdint.h>

typedef struct
{
    float kp;  /**< 方向比例增益，保留原控制拍标度。 */
    float k2p; /**< 方向非线性比例增益，保留原标度。 */
    float kd;  /**< 方向拍间差分增益，未除以周期。 */
    float k2d; /**< 加速度阻尼增益，保留原输入标度。 */
} pid_track_gains_t;

typedef struct
{
    float kp; /**< 角度比例增益，输入单位为度。 */
    float kd; /**< 角度拍间差分增益，未除以周期。 */
    float kg; /**< 角速度阻尼增益，输入为度/秒。 */
} pid_angle_gains_t;

typedef enum
{
    PID_GAIN_KP_V,  /**< 双轮共用速度比例增益。 */
    PID_GAIN_KI_V,  /**< 双轮共用速度积分增益。 */
    PID_GAIN_KD_V,  /**< 保留的速度微分增益，当前位置式 PI 不读取。 */
    PID_GAIN_KP_X,  /**< 当前参数 bank 的方向比例增益。 */
    PID_GAIN_K2P_X, /**< 当前参数 bank 的方向非线性比例增益。 */
    PID_GAIN_KI_X,  /**< 保留的方向积分增益，当前方向环不读取。 */
    PID_GAIN_KD_X,  /**< 当前参数 bank 的方向拍间差分增益。 */
    PID_GAIN_K2D_X, /**< 当前参数 bank 的方向加速度阻尼增益。 */
    PID_GAIN_KP_A,  /**< 角度环比例增益。 */
    PID_GAIN_KD_A,  /**< 角度环拍间差分增益。 */
    PID_GAIN_KG_A   /**< 角度环角速度阻尼增益。 */
} pid_gain_id_t;

typedef struct
{
    float kp_v;      /**< 双轮速度比例增益，每控制拍标度。 */
    float ki_v;      /**< 双轮速度积分增益，每控制拍标度。 */
    float kd_v;      /**< 保留速度微分增益，活动位置式 PI 不使用。 */
    float kp_x;      /**< 方向比例增益。 */
    float k2p_x;     /**< 方向非线性比例增益。 */
    float ki_x;      /**< 保留方向积分增益，当前方向计算不使用。 */
    float kd_x;      /**< 方向拍间差分增益。 */
    float k2d_x;     /**< 方向加速度阻尼增益。 */
    float kp_a;      /**< 角度比例增益。 */
    float kd_a;      /**< 角度拍间差分增益。 */
    float kg_a;      /**< 角速度阻尼增益。 */
    float angle_err; /**< 最近角度误差，度。 */
    float angle_out; /**< 原角度环浮点输出，交由差速入口转换。 */
    float out_l;     /**< 活动左轮 PI 浮点输出，限幅 ±7500。 */
    float out_r;     /**< 活动右轮 PI 浮点输出，限幅 ±7500。 */
} pid_state_t;

#define PID_MAX_DIR_OUT (1000)
#define PID_MAX_SPD_OUT (7500)

#define PID_MOTOR_DEAD_ZONE_LEFT (500)
#define PID_MOTOR_DEAD_ZONE_RIGHT (500)

/**
 * @brief 读取模块持有的只读运行状态。
 * @note 静态对象寿命为整个程序；控制拍可更新，前台一致性读取须遵守既有停车约定。
 *     完整契约见 pid.c::PID_GetState。
 */
const pid_state_t *PID_GetState(void);

/**
 * @brief 按枚举更新单项增益。
 * @note 参数切换的既有上下文调用，不重置历史。
 *     完整契约见 pid.c::PID_SetGain。
 */
void PID_SetGain(pid_gain_id_t id, float value);

/**
 * @brief 更新四项方向增益，保留控制历史。
 * @note 启动、档位切换或状态剖面应用时调用；写 s_pid 方向增益，不访问硬件。
 *     完整契约见 pid.c::PID_SetTrackGains。
 */
void PID_SetTrackGains(const pid_track_gains_t *gains);

/**
 * @brief 更新三项角度增益，保留控制历史。
 * @note 状态剖面应用时调用；写 s_pid 角度增益，不访问硬件。
 *     完整契约见 pid.c::PID_SetAngleGains。
 */
void PID_SetAngleGains(const pid_angle_gains_t *gains);

/**
 * @brief 用模块增益计算纯角度输出。
 * @note 只更新角度历史及 s_pid 误差输出，无电机动作。
 *     完整契约见 pid.c::PID_CalculateAngle。
 */
float PID_CalculateAngle(int16_t target_angle, float angle, float gyro);

/**
 * @brief 以模块速度增益计算Left轮 PI。
 * @note 双轮历史由 speed_controller 独立持有。
 *     完整契约见 pid.c::PID_UpdateSpeedLeft。
 */
int16_t PID_UpdateSpeedLeft(int16_t target, int16_t measured);

/**
 * @brief 以模块速度增益计算Right轮 PI。
 * @note 双轮历史由 speed_controller 独立持有。
 *     完整契约见 pid.c::PID_UpdateSpeedRight。
 */
int16_t PID_UpdateSpeedRight(int16_t target, int16_t measured);

/**
 * @brief 清两个活动 PI 的积分、目标历史和输出。
 * @note 既有停车/起跑路径调用，不清休眠增量式局部静态量。
 *     完整契约见 pid.c::PID_ResetSpeed。
 */
void PID_ResetSpeed(void);

/**
 * @brief 清旧预览方向及角度历史，再复位活动速度 PI。
 * @note 两模式影子方向历史由各自 FSM reset 处理；保留增益及休眠增量式历史。
 *     完整契约见 pid.c::PID_ResetAll。
 */
void PID_ResetAll(void);

#endif
