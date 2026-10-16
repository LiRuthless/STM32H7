/**
 * @file motor.h
 * @brief App 双轮速度闭环与里程；TIM6 执行，停车路径可直接清 PWM。
 * @note 移植来源：Sirius20260718/project/code/motor.h，STC AI8051U 省赛快照（2026-07-18）。
 * @note 源算法文件未单列作者及独立许可证；保留原出处，关联 Phase 13/15 规约。
 */
#ifndef SMARTCAR_APP_MOTOR_H
#define SMARTCAR_APP_MOTOR_H

#include <stdint.h>

typedef struct
{
    int16_t target_speed; /**< 单轮目标，每 2 ms 控制拍的编码器脉冲。 */
    int16_t real_speed;   /**< 低通输出截断到 int16_t 后的每拍脉冲。 */
    int32_t distance;     /**< 滤波截断后的单轮脉冲累计值，不是毫米。 */
} motor_wheel_state_t;

typedef struct
{
    motor_wheel_state_t left;  /**< 左轮独立目标、反馈与累计脉冲。 */
    motor_wheel_state_t right; /**< 右轮独立目标、反馈与累计脉冲。 */
    int32_t distance;          /**< 左右累计脉冲之和除以 2，保留整数除法。 */
    int16_t base_speed;        /**< 差速基础速度，每 2 ms 控制拍脉冲。 */
    int16_t fan_duty;          /**< 运行风扇占空比，万分比 0..10000。 */
    int16_t fan_duty_idle;     /**< 停车风扇占空比，万分比，初值 1100。 */
    float alpha;               /**< 编码器低通系数，初值 0.88。 */
} motor_state_t;

/**
 * @brief 读取模块持有的只读运行状态。
 * @return 非空只读指针，不转移所有权。
 * @note 静态对象寿命为整个程序；控制拍可更新，前台一致性读取须遵守既有停车约定。
 */
const motor_state_t *Motor_GetState(void);

/**
 * @brief 依次设置左右目标速度。
 * @param[in] left 左轮目标，脉冲/2 ms，int16_t。
 * @param[in] right 右轮目标，脉冲/2 ms，int16_t。
 * @note 仅保存，不写 PWM，不新增限幅；控制拍或既有停车路径调用。
 */
void Motor_SetTargets(int16_t left, int16_t right);

/**
 * @brief 保存差速基础速度。
 * @param[in] speed 基础速度，脉冲/2 ms，int16_t。
 */
void Motor_SetBaseSpeed(int16_t speed);

/**
 * @brief 保存运行风扇占空比。
 * @param[in] duty 万分比，约定 0..10000，保持 int16_t。
 * @note 仅保存，保持原调用方写 PWM 的时机；不新增范围检查。
 */
void Motor_SetFanDuty(int16_t duty);

/**
 * @brief 保存停车风扇占空比。
 * @param[in] duty 万分比，约定 0..10000，保持 int16_t。
 * @note 仅保存，保持原调用方写 PWM 的时机；不新增范围检查。
 */
void Motor_SetFanDutyIdle(int16_t duty);

/**
 * @brief 按原顺序清左右累计脉冲及均值。
 * @note 控制拍或起跑准备调用，不改硬件计数、反馈或滤波历史。
 */
void Motor_ResetDistance(void);

/**
 * @brief 按方向输出分配非对称双轮目标。
 * @param[in] pid_out 方向输出，正常 -1000..1000。
 * @note 内侧 3*pid_out 后整数除以 2，外侧倍数 1；保持计算和赋值顺序。
 */
void Motor_DistributeSpeed(int16_t pid_out);

/**
 * @brief 计算两轮 PI 后依次写方向和 PWM。
 * @note TIM6 调用；先左后右计算，再左后右写出；左右正转电平相反，不新增饱和处理。
 */
void Motor_UpdateControl(void);

/**
 * @brief 读取采样累计量并低通后累计里程。
 * @note TIM6 每 2 ms 调用；先左右读清，再分别 float→int16 截断累计，最后整数平均。
 */
void Motor_ReadEncoder(void);

/**
 * @brief 初始化 PWM 并设置原方向电平。
 * @note 上电前台调用，保持两路方向初值和初始化顺序。
 */
void Motor_Init(void);

/**
 * @brief 初始化编码器与两个低通实例。
 * @note 上电前台调用，低通系数取现有 alpha，不改硬件配置。
 */
void Motor_InitEncoder(void);

/**
 * @brief 清目标、反馈、里程与低通历史。
 * @note 再次起跑前调用；不改风扇、alpha 或重复初始化硬件。
 */
void Motor_ResetRunState(void);

/**
 * @brief 清目标与活动 PI 并直接清两路 PWM。
 * @note 有界，无阻塞，既有 ISR/前台停车路径可调用；保持左后右顺序。
 */
void Motor_EmergencyStop(void);

#endif
