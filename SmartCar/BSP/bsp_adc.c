/**
 * @file bsp_adc.c
 * @brief ADC1 十通道扫描 + DMA 循环采集。
 * @details ADC1 的十个扫描结果由循环 DMA 写入 D2 SRAM1，读取时右移四位到 12 位标度。
 *     PC4 灰度位于 rank5；前台或 TIM6 可观察 NDTR 等待新样本，失败不写输出。
 */
#include "bsp_adc.h"
#include "adc.h"
#include "bsp_time.h"
/* DMA 目标缓冲必须放在 D2 SRAM1（0x30000000，DMA1 可直接访问；该区域不在链接
 * 脚本任何 RW 段内，绝对定位不会与变量重叠），DTCM(0x20000000) 不可被 DMA1 访问。
 * D-Cache 未开启，无需 Cache 维护。 */
#if defined(__CC_ARM)
__attribute__((at(0x30000000)))
#else
__attribute__((section(".ARM.__at_0x30000000")))
#endif
static volatile uint16_t s_adc_buf[BSP_ADC_CH_NUM];
/**
 * @brief 校准 ADC1 并启动十通道循环 DMA。
 * @details 先执行单端偏移校准，再把模块缓冲交给 ADC1 循环 DMA；两个 HAL 返回值均未检查。
 * @note 启动主循环调用；不返回校准或 DMA 启动失败状态。
 */
void BSP_ADC_Init(void)
{
    /* 单端模式偏移校准 */
    (void)HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);
    (void)HAL_ADC_Start_DMA(&hadc1, (uint32_t *)s_adc_buf, BSP_ADC_CH_NUM);
}

/**
 * @brief 读取指定 ADC 通道最近一次转换。
 * @details 先验证通道枚举范围，越界返回零；有效通道读取 DMA 缓存并右移四位，转换为 12 位原始值。
 * @param[in] ch DMA 通道索引；应为 BSP_ADC_* 枚举有效通道。
 * @return 右移四位后的 0～4095 原始读数；索引达到通道数时返回 0。
 * @pre 已启动 DMA；D-Cache 保持关闭。
 * @note 前台菜单、电池读取及 TIM6 电感采样可调用；读取单个半字，不锁定整组快照。
 */
uint16_t BSP_ADC_Read(bsp_adc_channel_t ch)
{
    if (ch >= BSP_ADC_CH_NUM)
    {
        return 0;
    }
    return (uint16_t)(s_adc_buf[ch] >> 4);
}

/* 不依赖低优先级 DMA 回调；NDTR=10..6 表示本轮 rank5 尚未写入。 */
/**
 * @brief 等待并读取选址后一次新的灰度 DMA 转换。
 * @details 先检查输出指针和 DMA 句柄，再在整帧 DWT 期限内轮询。NDTR 大于 5 时记录
 *     rank5 写入前窗口，随后小于等于 5 才读取缓存；无 DMA 使能、错误标志或异常计数均失败。
 * @param[out] value 成功时写入 0～4095；空指针返回 BSP_GRAY_ARG，失败不写输出。
 * @param[in] deadline_start 整帧起始 DWT 周期；与当前计数作无符号差处理回绕。
 * @return BSP_GRAY_OK 成功；ARG 参数空；TIMEOUT 超过整帧预算；DMA_ERROR 状态异常。
 * @note 主循环或 TIM6 有界等待；先观察 rank5 写入前窗口，再等写入后窗口。
 */
bsp_gray_status_t BSP_ADC_ReadGrayFresh(uint16_t *value, uint32_t deadline_start)
{
    /* 先验证输出和 DMA 句柄，再在整帧共同期限内观察 NDTR 跨过灰度 rank；等待期间持续检查 DMA 错误和超时。 */
    uint8_t before = 0;
    DMA_HandleTypeDef *dma = hadc1.DMA_Handle;
    if (value == 0)
    {
        return BSP_GRAY_ARG;
    }
    if (dma == 0 || dma->Instance == 0)
    {
        return BSP_GRAY_DMA_ERROR;
    }
    for (;;)
    {
        uint32_t remaining;
        if ((uint32_t)(BSP_Time_GetCycles() - deadline_start) >= BSP_GRAY_BUDGET_CYCLES)
        {
            return BSP_GRAY_TIMEOUT;
        }
        if ((((DMA_Stream_TypeDef *)dma->Instance)->CR & DMA_SxCR_EN) == 0u
            || __HAL_DMA_GET_FLAG(dma, __HAL_DMA_GET_TE_FLAG_INDEX(dma))
            || __HAL_DMA_GET_FLAG(dma, __HAL_DMA_GET_DME_FLAG_INDEX(dma))
            || __HAL_DMA_GET_FLAG(dma, __HAL_DMA_GET_FE_FLAG_INDEX(dma)))
        {
            return BSP_GRAY_DMA_ERROR;
        }
        remaining = __HAL_DMA_GET_COUNTER(dma);
        if (remaining > BSP_ADC_CH_NUM)
        {
            return BSP_GRAY_DMA_ERROR;
        }
        if (remaining > BSP_ADC_CH_NUM - BSP_ADC_GRAY - 1u)
        {
            before = 1;
        }
        else if (before)
        {
            *value = (uint16_t)(s_adc_buf[BSP_ADC_GRAY] >> 4);
            return BSP_GRAY_OK;
        }
    }
}
