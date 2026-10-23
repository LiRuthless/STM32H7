/**
 * @file battery.c
 * @brief 电池ADC采样和低通滤波。
 * @details 停车前台或TIM7刷新原始值与滤波值；低压持续保护由安全模块判断，不反向依赖系统编排。
 */
/**
 * @note 移植来源：Sirius20260718/project/user/main.c、user/isr.c、code/config.c。
 * @note STC AI8051U 省赛快照（2026-07-18）；原算法未单列作者或独立许可证。
 * @note 保留源工程出处，关联 Phase 13/14/15 规约。
 */
#include "bsp.h"
#include "sensing/battery.h"
#include "sensing/filter.h"

/* 前台停车采样或运行TIM7更新，显示及记录按原并发契约读取。 */
static int16_t s_raw = 0;
static int16_t s_filtered = 0;
/* 上电初始化alpha=0.65，起跑不清滤波历史。 */
static filter_lowpass_t s_filter;

/**
 * @brief 初始化电池低通滤波器。
 * @note 仅上电原滤波初始化位置调用；不清除其他采样状态。
 */
void Battery_Init(void)
{
    Filter_InitLowpass(&s_filter, 0.65f);
}

/**
 * @brief 读取电池ADC并刷新滤波值。
 * @details 先把ADC缩窄为int16_t，再转float滤波，最后截断为int16_t，保持原标度和计算顺序。
 * @note 停车前台或TIM7调用；不实施低压保护，不复位滤波历史。
 */
void Battery_Update(void)
{
    s_raw = (int16_t)BSP_ADC_Read(BSP_ADC_VBAT);
    s_filtered = (int16_t)Filter_UpdateLowpass(&s_filter, (float)s_raw);
}

/**
 * @brief 读取电池ADC原始标度值。
 * @return 最近采样的int16_t值。
 * @note 单次复制，无锁或额外采样。
 */
int16_t Battery_GetRaw(void)
{
    return s_raw;
}

/**
 * @brief 读取电池滤波标度值。
 * @return 最近采样的int16_t值。
 * @note 单次复制，无锁或额外采样。
 */
int16_t Battery_GetFiltered(void)
{
    return s_filtered;
}
