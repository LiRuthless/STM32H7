/**
 * @file bsp_gray.c
 * @brief BSP 龙邱 16 路复用灰度采集；主循环或 TIM6 发布完整帧。
 */
/* 移植来源：LQ_Track.c V2.0.0 (2026-03-24)，chiusir。
 * 版权所有 (C) 2025-2026 北京龙邱科技有限公司，GPL-3.0。
 * 原库声明：允许使用、修改、分发；修改版本以相同许可证发布，保留版权与
 * 许可证信息；不提供担保。商业用途（包括单位使用）需联系 www.lqist.cn。
 * 完整协议见 https://www.gnu.org/licenses/gpl-3.0.html 。
 * SHA256: 6AB7B2B68AFE7C00C6CF2096E56953D398C1E73C028D792431B90786F792DCEF
 * Tracking_init/Tracking_Value_once/Tracking_Value_Acquire 的 STM32 适配。
 * 每通道只选址一次，丢三均二的是真实 DMA 转换，先均值后缩放；不移植测试循环。
 */
#include "bsp_gray.h"
#include "bsp_time.h"
/* 主循环起跑复位，主循环或 TIM6 完整成功采集后递增；失败不发布半帧。 */
static uint32_t s_sequence;

/**
 * @brief 一次 BSRR 写入选择 0～15 灰度通道。
 * @param[in] ch 选择线编码，调用方保证 0～15。
 * @note 由完整帧扫描串行调用，不在本层执行标定或算法。
 */
static void BSP_Gray_SelectChannel(uint8_t ch)
{
    uint32_t pins = GRAY_IO1_Pin | GRAY_IO2_Pin | GRAY_IO3_Pin | GRAY_IO4_Pin;
    uint32_t set = ((ch & 1u) ? GRAY_IO1_Pin : 0u) | ((ch & 2u) ? GRAY_IO2_Pin : 0u)
        | ((ch & 4u) ? GRAY_IO3_Pin : 0u) | ((ch & 8u) ? GRAY_IO4_Pin : 0u);
    GPIOE->BSRR = set | ((pins & ~set) << 16);
}

void BSP_Gray_ResetSequence(void)
{
    s_sequence = 0;
}

void BSP_Gray_Init(void)
{
    BSP_Time_Init();
    BSP_Gray_SelectChannel(0);
    BSP_Gray_ResetSequence();
}

bsp_gray_status_t BSP_Gray_ReadFrame(bsp_gray_frame_t *out)
{
    bsp_gray_frame_t next;
    uint32_t start;
    uint8_t ch;
    uint8_t sample;
    if (out == 0)
    {
        return BSP_GRAY_ARG;
    }
    start = BSP_Time_GetCycles();
    for (ch = 0; ch < 16; ++ch)
    {
        uint32_t sum = 0;
        BSP_Gray_SelectChannel(ch);
        for (sample = 0; sample < 5; ++sample)
        {
            uint16_t value;
            bsp_gray_status_t status = BSP_ADC_ReadGrayFresh(&value, start);
            if (status != BSP_GRAY_OK)
            {
                return status;
            }
            /* 五次都等待新 DMA 转换；丢前三次，保留后两次，先整数均值后交给 App。 */
            if (sample >= 3)
            {
                sum += value;
            }
        }
        next.raw[ch] = (uint16_t)(sum / 2u);
    }
    /* uint32_t 无符号相减允许 DWT 自然回绕；整帧仍共用同一起始预算。 */
    next.scan_cycles = (uint32_t)(BSP_Time_GetCycles() - start);
    if (next.scan_cycles >= BSP_GRAY_BUDGET_CYCLES)
    {
        return BSP_GRAY_TIMEOUT;
    }
    next.sequence = ++s_sequence;
    *out = next;
    return BSP_GRAY_OK;
}
