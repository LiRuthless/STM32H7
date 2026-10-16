/**
 * @file bsp_gray.h
 * @brief BSP 龙邱 16 路复用灰度采集；主循环或 TIM6 发布完整帧。
 */
#ifndef SMARTCAR_BSP_BSP_GRAY_H
#define SMARTCAR_BSP_BSP_GRAY_H
#include "bsp_adc.h"

typedef struct
{
    uint16_t raw[16];     /**< 通道 0～15 的 12 位原始值，0～4095。 */
    uint32_t sequence;    /**< 完整成功帧序号，复位后从 1 累计并自然回绕。 */
    uint32_t scan_cycles; /**< 整帧 DWT 周期数，成功时小于 336000。 */
} bsp_gray_frame_t;

/**
 * @brief 启用 DWT 并选择通道零，复位帧序号。
 * @note 起跑前或停车主循环调用，不清零自由运行 DWT 计数器。
 */
void BSP_Gray_Init(void);
/**
 * @brief 将后续灰度完整帧序号从零重新累计。
 * @note 起跑复位时调用；不得与扫描并发。
 */
void BSP_Gray_ResetSequence(void);
/**
 * @brief 依次采集 16 路灰度并在完整成功后发布。
 * @param[out] out 完整帧；空指针返回 ARG；任一失败均保持原帧内容。
 * @return BSP_GRAY_OK 成功；ARG 参数空；TIMEOUT 达到 700 us；DMA_ERROR 转换异常。
 * @pre 已初始化 ADC DMA 和灰度驱动。
 * @note 主循环或 TIM6 调用；每通道选址一次，五个新转换丢前三个、后两个取整均值。
 */
bsp_gray_status_t BSP_Gray_ReadFrame(bsp_gray_frame_t *out);
#endif
