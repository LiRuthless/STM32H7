/**
 * @file pid.h
 * @brief App 方向、角度及独立双轮 PI；保留休眠增量式控制器。
 * @details 声明 PID 参数、状态与更新接口；完整公式、限幅和休眠入口说明位于 pid.c。
 * @note 移植来源：Sirius20260718/project/code/pid.h，STC AI8051U 省赛快照（2026-07-18）。
 * @note 源算法文件未单列作者及独立许可证；保留原出处，关联 Phase 13/15 规约。
 */
#ifndef SMARTCAR_APP_PID_H
#define SMARTCAR_APP_PID_H

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
 * @note 静态对象寿命为整个程序；控制拍可更新，前台一致性读取须遵守既有停车约定。 完整契约见 pid.c::PID_GetState。
 */
const pid_state_t *PID_GetState(void);

/**
 * @brief 按枚举更新单项增益。
 * @note 参数切换的既有上下文调用，不重置历史。 完整契约见 pid.c::PID_SetGain。
 */
void PID_SetGain(pid_gain_id_t id, float value);

/**
 * @brief 按原顺序更新四项方向增益。
 * @note gains 非空方向增益，不改历史或其他增益。 完整契约见 pid.c::PID_SetTrackGains。
 */
void PID_SetTrackGains(const pid_track_gains_t *gains);

/**
 * @brief 按原顺序更新三项角度增益。
 * @note gains 非空角度增益，不改历史或其他增益。 完整契约见 pid.c::PID_SetAngleGains。
 */
void PID_SetAngleGains(const pid_angle_gains_t *gains);

/**
 * @brief 用调用方独立历史计算方向控制。
 * @note gains 非空四项增益。 history 非空独立前拍偏差，计算差分后更新。 TIM6 调用；浮点求和先转换 int16_t，再限幅 ±1000，不调整顺序。 完整契约见
 *     pid.c::PID_DirectionStep。
 */
int16_t PID_DirectionStep(
    int16_t error, float accel, const pid_track_gains_t *gains, int32_t *history);

/**
 * @brief 用旧共享历史计算电磁方向输出。
 * @note 保留旧入口和调用边，当前观察器使用各自独立历史。 完整契约见 pid.c::PID_UpdateTrack。
 */
int16_t PID_UpdateTrack(void);

/**
 * @brief 计算原角度 PD 并分配差速。
 * @note 保持角误差、浮点输出、历史再差速顺序；旧入口保留。 完整契约见 pid.c::PID_UpdateAngle。
 */
void PID_UpdateAngle(int16_t target_angle);

/**
 * @brief 保留左轮增量式速度控制入口。
 * @note 当前不接入；局部静态历史保留，微分代码仍注释，ResetAll 不新增复位。 完整契约见 pid.c::PID_UpdateIncrementalLeft。
 */
int16_t PID_UpdateIncrementalLeft(void);

/**
 * @brief 保留右轮增量式速度控制入口。
 * @note 当前不接入；局部静态历史保留，微分代码仍注释，ResetAll 不新增复位。 完整契约见 pid.c::PID_UpdateIncrementalRight。
 */
int16_t PID_UpdateIncrementalRight(void);

/**
 * @brief 更新左轮独立活动位置式 PI。
 * @note TIM6 调用；目标换向先清积分，保持死区、积分限幅和输出转换顺序。 完整契约见 pid.c::PID_UpdateSpeedLeft。
 */
int16_t PID_UpdateSpeedLeft(void);

/**
 * @brief 更新右轮独立活动位置式 PI。
 * @note TIM6 调用；目标换向先清积分，保持死区、积分限幅和输出转换顺序。 完整契约见 pid.c::PID_UpdateSpeedRight。
 */
int16_t PID_UpdateSpeedRight(void);

/**
 * @brief 清两个活动 PI 的积分、目标历史和输出。
 * @note 既有停车/起跑路径调用，不清休眠增量式局部静态量。 完整契约见 pid.c::PID_ResetSpeed。
 */
void PID_ResetSpeed(void);

/**
 * @brief 清活动方向、角度历史并复位速度 PI。
 * @note 再次起跑前调用，保留所有增益及休眠控制器历史。 完整契约见 pid.c::PID_ResetAll。
 */
void PID_ResetAll(void);

#endif
