#ifndef __BSP_ADC_H
#define __BSP_ADC_H

#include "main.h"

/* ADC1 十通道扫描 + DMA 循环（16bit 原始，读取时 >>4 归一化到 12bit 0~4095，
 * 与源工程全部阈值标度一致）。
 * 枚举顺序必须与 .ioc 的 rank 顺序一致：
 * rank0=INP10(PC0) rank1=INP11(PC1) rank2=INP18(PA4) rank3=INP8(PC5)
 * rank4=INP4(PC4)  rank5=INP14(PA2) rank6=INP15(PA3) rank7=INP9(PB0)
 * rank8=INP5(PB1)  rank9=INP19(PA5) */
typedef enum
{
    BSP_ADC_IND_H_L = 0,    /* 电感1 PC0 横电感·左（源 adc_filted[0]） */
    BSP_ADC_IND_V_L,        /* 电感2 PC1 竖电感·左（源 adc_filted[1]） */
    BSP_ADC_IND_V_R,        /* 电感3 PA4 竖电感·右（源 adc_filted[2]） */
    BSP_ADC_IND_H_R,        /* 电感4 PC5 横电感·右（源 adc_filted[3]） */
    BSP_ADC_GRAY,           /* 灰度   PC4 */
    BSP_ADC_KEY1,           /* 按键组1 PA2（方向+确定） */
    BSP_ADC_KEY2,           /* 按键组2 PA3（复位+返回） */
    BSP_ADC_VBAT,           /* 电池   PB0 */
    BSP_ADC_SPARE1,         /* 备用   PB1 */
    BSP_ADC_SPARE2,         /* 备用   PA5 */
    BSP_ADC_CH_NUM
} bsp_adc_channel_t;

void     BSP_ADC_Init(void);                            /* 校准并启动 DMA 循环采集 */
uint16_t BSP_ADC_Read(bsp_adc_channel_t ch);            /* 12bit 值 0~4095 */

#endif /* __BSP_ADC_H */
