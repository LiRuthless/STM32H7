/**
  ******************************************************************************
  * @file    bsp_encoder.c
  * @brief   编码器接口：TIM5(PA0/PA1)=左轮、TIM3(PC6/PC7)=右轮，
  *          TI12 四倍频正交模式。每次读取后清零，符号由 INVERT 宏决定。
  ******************************************************************************
  */

/* 包含头文件 ------------------------------------------------------------------*/
#include "bsp_encoder.h"
#include "tim.h"

/* 私有函数原型 ---------------------------------------------*/
static int16_t encoder_read_clear(TIM_HandleTypeDef *htim, uint8_t invert);

/* 导出函数定义 -------------------------------------------*/

/**
  * @brief  启动两路编码器计数
  * @retval 无
  */
void BSP_Encoder_Init(void)
{
  __HAL_TIM_SET_COUNTER(&htim5, 0);
  __HAL_TIM_SET_COUNTER(&htim3, 0);
  (void)HAL_TIM_Encoder_Start(&htim5, TIM_CHANNEL_ALL);   /* 左轮 */
  (void)HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);   /* 右轮 */
}

/**
  * @brief  读左轮计数并清零，前进为正
  */
int16_t BSP_Encoder_GetLeft(void)
{
  return encoder_read_clear(&htim5, BSP_ENCODER_L_INVERT);
}

/**
  * @brief  读右轮计数并清零，前进为正
  */
int16_t BSP_Encoder_GetRight(void)
{
  return encoder_read_clear(&htim3, BSP_ENCODER_R_INVERT);
}

/* 私有函数定义 -------------------------------------------*/

/**
  * @brief  读取计数器并清零；invert 非 0 时取反
  */
static int16_t encoder_read_clear(TIM_HandleTypeDef *htim, uint8_t invert)
{
  int16_t cnt = (int16_t)__HAL_TIM_GET_COUNTER(htim);

  __HAL_TIM_SET_COUNTER(htim, 0);
  if (invert != 0u)
  {
    cnt = (int16_t)(-cnt);
  }
  return cnt;
}
