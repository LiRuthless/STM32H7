/**
  ******************************************************************************
  * @file    bsp_adc.c
  * @brief   ADC1 十通道扫描 + DMA 循环采集。
  *          原始 16bit，读取时 >>4 归一化到 12bit（0~4095）。
  *          rank 顺序已核对与 bsp_adc.h 枚举一致：
  *          rank1=INP10 rank2=INP11 rank3=INP18 rank4=INP8 rank5=INP4
  *          rank6=INP14 rank7=INP15 rank8=INP9 rank9=INP5 rank10=INP19
  ******************************************************************************
  */

/* 包含头文件 ------------------------------------------------------------------*/
#include "bsp_adc.h"
#include "adc.h"

/* 私有变量 ---------------------------------------------------------*/
/* DMA 目标缓冲必须放在 D2 SRAM1（0x30000000，DMA1 可直接访问；该区域不在链接
 * 脚本任何 RW 段内，绝对定位不会与变量重叠），DTCM(0x20000000) 不可被 DMA1 访问。
 * D-Cache 未开启，无需 Cache 维护。 */
#if defined(__CC_ARM)
__attribute__((at(0x30000000)))
#else
__attribute__((section(".ARM.__at_0x30000000")))
#endif
static uint16_t s_adc_buf[BSP_ADC_CH_NUM];

/* 导出函数定义 -------------------------------------------*/

/**
  * @brief  校准 ADC1 并启动 DMA 循环采集
  * @retval 无
  */
void BSP_ADC_Init(void)
{
  /* 单端模式偏移校准 */
  (void)HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);
  (void)HAL_ADC_Start_DMA(&hadc1, (uint32_t *)s_adc_buf, BSP_ADC_CH_NUM);
}

/**
  * @brief  读取指定通道，16bit 原始值右移 4 位归一化到 12bit
  * @param  ch 通道枚举（顺序 = DMA 缓冲下标 = rank 顺序）
  * @retval 12bit 转换值 0~4095
  */
uint16_t BSP_ADC_Read(bsp_adc_channel_t ch)
{
  if (ch >= BSP_ADC_CH_NUM)
  {
    return 0;
  }
  return (uint16_t)(s_adc_buf[ch] >> 4);
}
