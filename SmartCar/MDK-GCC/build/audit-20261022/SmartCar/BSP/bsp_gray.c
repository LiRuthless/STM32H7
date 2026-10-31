/**
 * @file bsp_gray.c
 * @brief BSP 龙邱 16 路复用灰度采集；主循环或 TIM6 发布完整帧。
 * @details 驱动十六选一灰度复用器，每通道等待五个新转换并丢三均二，预算内成功后发布完整帧。
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
 * @details 按函数体既定条件和写入顺序更新模块状态；提前返回分支不执行其后的状态发布。
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

/**
 * @brief 将后续灰度完整帧序号从零重新累计。
 * @details 按函数体既定条件和写入顺序更新模块状态；提前返回分支不执行其后的状态发布。
 * @note 起跑复位时调用；不得与扫描并发。
 */
void BSP_Gray_ResetSequence(void)
{
    s_sequence = 0;
}

/**
 * @brief 启用 DWT 并选择通道零，复位帧序号。
 * @details 按现有调用顺序使用 BSP_Time_Init、BSP_Gray_SelectChannel、BSP_Gray_ResetSequence 完成本函数处理。
 * @note 起跑前或停车主循环调用，不清零自由运行 DWT 计数器。
 */
void BSP_Gray_Init(void)
{
    BSP_Time_Init();
    BSP_Gray_SelectChannel(0);
    BSP_Gray_ResetSequence();
}

/**
 * @brief 依次采集 16 路灰度并在完整成功后发布。
 * @details 依次选择 0 到 15 通道；每路等待五次新的 ADC 转换，丢弃前三次并对后两次取整数均值。任一转换错误或整帧超预算立即返回且不写调用方输出，全部成功后才递增序号并发布帧。
 * @param[out] out 完整帧；空指针返回 ARG；任一失败均保持原帧内容。
 * @return BSP_GRAY_OK 成功；ARG 参数空；TIMEOUT 达到 700 us；DMA_ERROR 转换异常。
 * @pre 已初始化 ADC DMA 和灰度驱动。
 * @note 主循环或 TIM6 调用；每通道选址一次，五个新转换丢前三个、后两个取整均值。
 */
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

    /* 整帧共用一次 DWT 起点；无符号差值允许 CYCCNT 在扫描期间自然回绕。 */
    start = BSP_Time_GetCycles();
    for (ch = 0; ch < 16; ++ch)
    {
        uint32_t sum = 0;

        /* 每个地址只选址一次，随后等待五个新 DMA 转换，避免复用切换前的旧样本。 */
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
    /* 全部通道完成后再核对整帧预算；超时仍不发布调用方输出或递增成功序号。 */
    next.scan_cycles = (uint32_t)(BSP_Time_GetCycles() - start);
    if (next.scan_cycles >= BSP_GRAY_BUDGET_CYCLES)
    {
        return BSP_GRAY_TIMEOUT;
    }
    /* 仅成功帧递增序号并一次性复制给调用方，任何失败都保留原输出。 */
    next.sequence = ++s_sequence;
    *out = next;
    return BSP_GRAY_OK;
}
