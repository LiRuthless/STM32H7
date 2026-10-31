/**
 * @file bsp_encoder.c
 * @brief 编码器接口：TIM5(PA0/PA1)=左轮、TIM3(PC6/PC7)=右轮， TI12 四倍频正交模式。
 * @details 读取并清零左右定时器编码器计数，按编译期方向配置发布有符号脉冲。
 */
#include "bsp_encoder.h"
#include "tim.h"
static int16_t BSP_Encoder_ReadClear(TIM_HandleTypeDef *htim, uint8_t invert);
/**
 * @brief 清零并启动左右轮正交编码器定时器。
 * @details 按现有调用顺序使用 __HAL_TIM_SET_COUNTER、HAL_TIM_Encoder_Start 完成本函数处理。
 * @note 起跑前主循环调用，保持左轮 TIM5、右轮 TIM3 次序。
 */
void BSP_Encoder_Init(void)
{
    __HAL_TIM_SET_COUNTER(&htim5, 0);
    __HAL_TIM_SET_COUNTER(&htim3, 0);
    (void)HAL_TIM_Encoder_Start(&htim5, TIM_CHANNEL_ALL); /* 左轮 */
    (void)HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL); /* 右轮 */
}

/**
 * @brief 读取左轮编码器并清零硬件计数。
 * @return 转为 int16_t 并按配置反向后的脉冲计数；前进为正。
 * @note TIM15 采样调用；读取具有清零副作用。
 */
int16_t BSP_Encoder_GetLeft(void)
{
    return BSP_Encoder_ReadClear(&htim5, BSP_ENCODER_L_INVERT);
}

/**
 * @brief 读取右轮编码器并清零硬件计数。
 * @return 转为 int16_t 并按配置反向后的脉冲计数；前进为正。
 * @note TIM15 采样调用；读取具有清零副作用。
 */
int16_t BSP_Encoder_GetRight(void)
{
    return BSP_Encoder_ReadClear(&htim3, BSP_ENCODER_R_INVERT);
}
/**
 * @brief 按原 int16_t 截断与方向配置读取并清零轮计数。
 * @details 按现有调用顺序使用 __HAL_TIM_GET_COUNTER、__HAL_TIM_SET_COUNTER 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
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
