/**
 * @file bsp_adc.h
 * @brief BSP ADC1 十通道 DMA 采集；供主循环和 TIM6 读取。
 * @details 声明 ADC DMA、电感通道与灰度单次转换接口；完整硬件和失败语义位于 bsp_adc.c。
 */
#ifndef SMARTCAR_BSP_BSP_ADC_H
#define SMARTCAR_BSP_BSP_ADC_H

#include "main.h"

/* ADC1 十通道扫描 + DMA 循环（16bit 原始，读取时 >>4 归一化到 12bit 0~4095，
 * 与源工程全部阈值标度一致）。
 * 枚举顺序必须与 .ioc 的 rank 顺序一致：
 * rank0=INP10(PC0) rank1=INP11(PC1) rank2=INP18(PA4) rank3=INP8(PC5)
 * rank4=INP4(PC4)  rank5=INP14(PA2) rank6=INP15(PA3) rank7=INP9(PB0)
 * rank8=INP5(PB1)  rank9=INP19(PA5) */
typedef enum
{
    BSP_ADC_IND_H_L = 0, /**< 电感 1，PC0 横电感左，源 adc_filted[0]。 */
    BSP_ADC_IND_V_L,     /**< 电感 2，PC1 竖电感左，源 adc_filted[1]。 */
    BSP_ADC_IND_V_R,     /**< 电感 3，PA4 竖电感右，源 adc_filted[2]。 */
    BSP_ADC_IND_H_R,     /**< 电感 4，PC5 横电感右，源 adc_filted[3]。 */
    BSP_ADC_GRAY,        /**< PC4 灰度模拟输入。 */
    BSP_ADC_KEY1,        /**< PA2 方向及确定分压按键组。 */
    BSP_ADC_KEY2,        /**< PA3 复位及返回分压按键组。 */
    BSP_ADC_VBAT,        /**< PB0 电池电压采样输入。 */
    BSP_ADC_SPARE1,      /**< PB1 备用模拟输入。 */
    BSP_ADC_SPARE2,      /**< PA5 备用模拟输入。 */
    BSP_ADC_CH_NUM       /**< ADC1 循环 DMA 通道总数，固定为 10。 */
} bsp_adc_channel_t;

typedef enum
{
    BSP_GRAY_OK = 0,    /**< 灰度整帧采集完成，输出缓冲区已更新。 */
    BSP_GRAY_ARG,       /**< 输出指针为空，未启动扫描。 */
    BSP_GRAY_TIMEOUT,   /**< 等待 DMA 完成超过单次采集时限。 */
    BSP_GRAY_DMA_ERROR  /**< HAL ADC DMA 启动或停止失败。 */
} bsp_gray_status_t;

/**
 * @brief 校准 ADC1 并启动十通道循环 DMA。
 * @note 启动主循环调用；保持原有 HAL 返回值忽略行为。 完整契约见 bsp_adc.c::BSP_ADC_Init。
 */
void BSP_ADC_Init(void);
/**
 * @brief 读取指定 ADC 通道最近一次转换。
 * @note 完整契约见 bsp_adc.c::BSP_ADC_Read。
 */
uint16_t BSP_ADC_Read(bsp_adc_channel_t ch);


/**
 * @brief 等待并读取选址后一次新的灰度 DMA 转换。
 * @note value 成功时写入 0～4095；空指针返回 BSP_GRAY_ARG，失败不写输出。 主循环或 TIM6 有界等待；先观察 rank5 写入前窗口，再等写入后窗口。
 *     完整契约见 bsp_adc.c::BSP_ADC_ReadGrayFresh。
 */
bsp_gray_status_t BSP_ADC_ReadGrayFresh(uint16_t *value, uint32_t deadline_start);

#endif /* __BSP_ADC_H */
