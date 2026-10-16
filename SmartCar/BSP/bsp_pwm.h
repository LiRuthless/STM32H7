/**
 * @file bsp_pwm.h
 * @brief BSP 电机和风扇 PWM 输出；万分比换算只在此层执行。
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
    BSP_PWM_MOTOR_L = 0,
    BSP_PWM_MOTOR_R,
    BSP_PWM_FAN,
    BSP_PWM_CH_NUM
} bsp_pwm_channel_t;

/**
 * @brief 启动左右电机和风扇的硬件 PWM 通道。
 * @pre CubeMX 定时器已初始化；启动主循环调用。
 */
void BSP_PWM_Init(void);
/**
 * @brief 将万分比占空比换算并写入指定 PWM 通道。
 * @param[in] ch BSP_PWM_* 通道；索引达到通道数时不写寄存器。
 * @param[in] duty 占空比 0～10000，超出上限时截为 10000。
 * @note 控制 ISR、停车路径及前台可调用；乘除与寄存器写入顺序保持。
 */
void BSP_PWM_SetDuty(bsp_pwm_channel_t ch, uint32_t duty);

#endif /* __BSP_PWM_H */
