/**
  ******************************************************************************
  * @file    bsp_pwm.c
  * @brief   PWM 输出：TIM13_CH1 左电机、TIM4_CH3 右电机、TIM14_CH1 风扇。
  *          占空比统一 0~10000 万分比，内部按各定时器 ARR 换算 CCR。
  ******************************************************************************
  */

/* 包含头文件 ------------------------------------------------------------------*/
#include "bsp_pwm.h"
#include "tim.h"

/* 私有变量 ---------------------------------------------------------*/
static TIM_HandleTypeDef *const s_pwm_tim[BSP_PWM_CH_NUM] =
{
  &htim13,    /* BSP_PWM_MOTOR_L */
  &htim4,     /* BSP_PWM_MOTOR_R */
  &htim14,    /* BSP_PWM_FAN */
};

static const uint32_t s_pwm_chan[BSP_PWM_CH_NUM] =
{
  TIM_CHANNEL_1,    /* BSP_PWM_MOTOR_L */
  TIM_CHANNEL_3,    /* BSP_PWM_MOTOR_R */
  TIM_CHANNEL_1,    /* BSP_PWM_FAN */
};

/* 导出函数定义 -------------------------------------------*/

/**
  * @brief  启动三路 PWM 输出
  * @retval 无
  */
void BSP_PWM_Init(void)
{
  (void)HAL_TIM_PWM_Start(&htim13, TIM_CHANNEL_1);    /* 左电机 */
  (void)HAL_TIM_PWM_Start(&htim4,  TIM_CHANNEL_3);    /* 右电机 */
  (void)HAL_TIM_PWM_Start(&htim14, TIM_CHANNEL_1);    /* 负压风扇 */
}

/**
  * @brief  设置占空比（0~10000），超限时内部截断
  *         CCR = duty * (ARR + 1) / 10000
  */
void BSP_PWM_SetDuty(bsp_pwm_channel_t ch, uint32_t duty)
{
  uint32_t arr;
  uint32_t ccr;

  if (ch >= BSP_PWM_CH_NUM)
  {
    return;
  }
  if (duty > BSP_PWM_DUTY_MAX)
  {
    duty = BSP_PWM_DUTY_MAX;
  }
  arr = __HAL_TIM_GET_AUTORELOAD(s_pwm_tim[ch]);
  ccr = (duty * (arr + 1u)) / BSP_PWM_DUTY_MAX;
  __HAL_TIM_SET_COMPARE(s_pwm_tim[ch], s_pwm_chan[ch], ccr);
}
