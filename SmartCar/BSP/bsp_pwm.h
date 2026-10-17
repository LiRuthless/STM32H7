/**
 * @file bsp_pwm.h
 * @brief BSP 电机和风扇 PWM 输出；万分比换算只在此层执行。
 * @details 声明电机和风扇 PWM 接口；完整限幅、方向及 CCR 换算位于 bsp_pwm.c。
 */
#ifndef SMARTCAR_BSP_BSP_PWM_H
#define SMARTCAR_BSP_BSP_PWM_H

#include "main.h"

/* PWM 输出：占空比统一 0~10000 万分比语义（沿用源工程，PID 限幅/死区参数不变），
 * 内部按各自定时器 ARR 换算 CCR。
 *   左电机  TIM13_CH1 PA6  17kHz（ARR=7059）
 *   右电机  TIM4_CH3  PD14 17kHz（ARR=7059）
 *   负压风扇 TIM14_CH1 PA7  17kHz（ARR=7059） */
#define BSP_PWM_DUTY_MAX 10000u

typedef enum
{
    BSP_PWM_MOTOR_L = 0, /**< TIM1 左电机 PWM 通道。 */
    BSP_PWM_MOTOR_R,     /**< TIM8 右电机 PWM 通道。 */
    BSP_PWM_FAN,         /**< TIM14 负压风扇 PWM 通道。 */
    BSP_PWM_CH_NUM       /**< 可配置 PWM 通道总数，固定为 3。 */
} bsp_pwm_channel_t;

/**
 * @brief 启动左右电机和风扇的硬件 PWM 通道。
 * @note 完整契约见 bsp_pwm.c::BSP_PWM_Init。
 */
void BSP_PWM_Init(void);
/**
 * @brief 将万分比占空比换算并写入指定 PWM 通道。
 * @note duty 占空比 0～10000，超出上限时截为 10000。 控制 ISR、停车路径及前台可调用；乘除与寄存器写入顺序保持。 完整契约见
 *     bsp_pwm.c::BSP_PWM_SetDuty。
 */
void BSP_PWM_SetDuty(bsp_pwm_channel_t ch, uint32_t duty);

#endif /* __BSP_PWM_H */
