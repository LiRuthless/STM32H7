#ifndef __BSP_SAMPLER_H
#define __BSP_SAMPLER_H

#include "main.h"

/* 高速采样器：TIM15 1ms（1kHz）周期中断，纯寄存器配置（CubeMX 未占用 TIM15）。
 * 职责：1ms 读取左右编码器（读即清零）并累计、1ms 读取 IMU 陀螺/加速度原始值。
 * NVIC 优先级 0（与 TIM6 控制环同级，互不抢占，SPI4 访问不会交错）。
 * 控制环（TIM6 2ms）通过 ConsumeEnc 接口取 2ms 累计值，控制周期不变。 */

void     BSP_Sampler_Init(void);    /* 配置 TIM15（不启动） */
void     BSP_Sampler_Start(void);
void     BSP_Sampler_Stop(void);

uint32_t BSP_Sampler_GetTick(void); /* 1ms 采样节拍 */

/* 最近一个 1ms 窗口的编码器计数（符号已按 BSP_ENCODER_*_INVERT 处理，前进为正） */
int16_t  BSP_Sampler_GetEncL_1ms(void);
int16_t  BSP_Sampler_GetEncR_1ms(void);

/* 最近一次的 IMU 原始值（1kHz 更新） */
void     BSP_Sampler_GetGyroRaw(int16_t *x, int16_t *y, int16_t *z);
void     BSP_Sampler_GetAccRaw(int16_t *x, int16_t *y, int16_t *z);

/* 控制环 2ms 消费接口：返回自上次调用以来的累计计数并清零（前进为正） */
int16_t  BSP_Sampler_ConsumeEncL(void);
int16_t  BSP_Sampler_ConsumeEncR(void);

#endif /* __BSP_SAMPLER_H */
