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
static int16_t BSP_Encoder_ReadClear(TIM_HandleTypeDef *htim, uint8_t invert);

/* 导出函数定义 -------------------------------------------*/

void BSP_Encoder_Init(void)
{
    __HAL_TIM_SET_COUNTER(&htim5, 0);
    __HAL_TIM_SET_COUNTER(&htim3, 0);
    (void)HAL_TIM_Encoder_Start(&htim5, TIM_CHANNEL_ALL); /* 左轮 */
    (void)HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL); /* 右轮 */
}

int16_t BSP_Encoder_GetLeft(void)
{
    return BSP_Encoder_ReadClear(&htim5, BSP_ENCODER_L_INVERT);
}

int16_t BSP_Encoder_GetRight(void)
{
    return BSP_Encoder_ReadClear(&htim3, BSP_ENCODER_R_INVERT);
}

/* 私有函数定义 -------------------------------------------*/

/**
 * @brief 按原 int16_t 截断与方向配置读取并清零轮计数。
 * @param[in,out] htim 定时器句柄，必须非空；计数读取后立即清零。
 * @param[in] invert 方向配置，0 保持，非零取反。
 * @return 处理后的 int16_t 脉冲计数，前进为正。
 * @note 仅 TIM15 读清轮计数，保持硬件访问顺序。
 */
static int16_t BSP_Encoder_ReadClear(TIM_HandleTypeDef *htim, uint8_t invert)
{
    int16_t cnt = (int16_t)__HAL_TIM_GET_COUNTER(htim);

    __HAL_TIM_SET_COUNTER(htim, 0);
    if (invert != 0u)
    {
        cnt = (int16_t)(-cnt);
    }
    return cnt;
}
