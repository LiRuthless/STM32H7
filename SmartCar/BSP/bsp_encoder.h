/**
 * @file bsp_encoder.h
 * @brief BSP 左右轮编码器驱动；TIM15 每毫秒读取并清零。
 */
#ifndef SMARTCAR_BSP_BSP_ENCODER_H
#define SMARTCAR_BSP_BSP_ENCODER_H

#include "main.h"

/* 编码器：TIM5(PA0/PA1)=左轮（32bit 定时器），TIM3(PC6/PC7)=右轮（16bit），
 * 均为 TI12 四倍频正交模式。TIM15 采样器每 1ms 读一次计数并清零，
 * TIM6 控制环每 2ms 消费累计值；正常轮速下读取值不会溢出。
 * 符号约定沿用源工程：左轮取反、右轮不取反（前进方向为正）。
 * 若实际接线相反，翻转下方宏即可。 */
#define BSP_ENCODER_L_INVERT 1
#define BSP_ENCODER_R_INVERT 0

/**
 * @brief 清零并启动左右轮正交编码器定时器。
 * @note 起跑前主循环调用，保持左轮 TIM5、右轮 TIM3 次序。
 */
void BSP_Encoder_Init(void);
/**
 * @brief 读取左轮编码器并清零硬件计数。
 * @return 转为 int16_t 并按配置反向后的脉冲计数；前进为正。
 * @note TIM15 采样调用；读取具有清零副作用。
 */
int16_t BSP_Encoder_GetLeft(void);
/**
 * @brief 读取右轮编码器并清零硬件计数。
 * @return 转为 int16_t 并按配置反向后的脉冲计数；前进为正。
 * @note TIM15 采样调用；读取具有清零副作用。
 */
int16_t BSP_Encoder_GetRight(void);

#endif /* __BSP_ENCODER_H */
