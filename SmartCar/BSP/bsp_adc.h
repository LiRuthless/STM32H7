/**
 * @file bsp_adc.h
 * @brief BSP ADC1 十通道 DMA 采集；供主循环和 TIM6 读取。
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
    BSP_ADC_IND_H_L = 0, /* 电感1 PC0 横电感·左（源 adc_filted[0]） */
    BSP_ADC_IND_V_L,     /* 电感2 PC1 竖电感·左（源 adc_filted[1]） */
    BSP_ADC_IND_V_R,     /* 电感3 PA4 竖电感·右（源 adc_filted[2]） */
    BSP_ADC_IND_H_R,     /* 电感4 PC5 横电感·右（源 adc_filted[3]） */
    BSP_ADC_GRAY,        /* 灰度   PC4 */
    BSP_ADC_KEY1,        /* 按键组1 PA2（方向+确定） */
    BSP_ADC_KEY2,        /* 按键组2 PA3（复位+返回） */
    BSP_ADC_VBAT,        /* 电池   PB0 */
    BSP_ADC_SPARE1,      /* 备用   PB1 */
    BSP_ADC_SPARE2,      /* 备用   PA5 */
    BSP_ADC_CH_NUM
} bsp_adc_channel_t;

typedef enum
{
    BSP_GRAY_OK = 0,
    BSP_GRAY_ARG,
    BSP_GRAY_TIMEOUT,
    BSP_GRAY_DMA_ERROR
} bsp_gray_status_t;

/**
 * @brief 校准 ADC1 并启动十通道循环 DMA。
 * @note 启动主循环调用；保持原有 HAL 返回值忽略行为。
 */
void BSP_ADC_Init(void);
/**
 * @brief 读取指定 ADC 通道最近一次转换。
 * @param[in] ch DMA 通道索引；应为 BSP_ADC_* 枚举有效通道。
 * @return 右移四位后的 0～4095 原始读数；索引达到通道数时返回 0。
 * @pre 已启动 DMA；D-Cache 保持关闭。
 */
uint16_t BSP_ADC_Read(bsp_adc_channel_t ch);


/**
 * @brief 等待并读取选址后一次新的灰度 DMA 转换。
 * @param[out] value 成功时写入 0～4095；空指针返回 BSP_GRAY_ARG，失败不写输出。
 * @param[in] deadline_start 整帧起始 DWT 周期；与当前计数作无符号差处理回绕。
 * @return BSP_GRAY_OK 成功；ARG 参数空；TIMEOUT 超过整帧预算；DMA_ERROR 状态异常。
 * @note 主循环或 TIM6 有界等待；先观察 rank5 写入前窗口，再等写入后窗口。
 */
bsp_gray_status_t BSP_ADC_ReadGrayFresh(uint16_t *value, uint32_t deadline_start);

#endif /* __BSP_ADC_H */
