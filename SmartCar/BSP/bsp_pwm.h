#ifndef __BSP_PWM_H
#define __BSP_PWM_H

#include "main.h"

/* PWM 输出：占空比统一 0~10000 万分比语义（沿用源工程，PID 限幅/死区参数不变），
 * 内部按各自定时器 ARR 换算 CCR。
 *   左电机  TIM13_CH1 PA6  17kHz（ARR=7059）
 *   右电机  TIM4_CH3  PD14 17kHz（ARR=7059）
 *   负压风扇 TIM14_CH1 PA7  17kHz（ARR=7059） */
#define BSP_PWM_DUTY_MAX    10000u

typedef enum
{
    BSP_PWM_MOTOR_L = 0,
    BSP_PWM_MOTOR_R,
    BSP_PWM_FAN,
    BSP_PWM_CH_NUM
} bsp_pwm_channel_t;

void BSP_PWM_Init(void);
void BSP_PWM_SetDuty(bsp_pwm_channel_t ch, uint32_t duty);  /* duty 0~10000，超限时内部截断 */

#endif /* __BSP_PWM_H */
