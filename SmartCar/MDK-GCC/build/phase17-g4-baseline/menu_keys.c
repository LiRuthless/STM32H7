/**
 * @file menu_keys.c
 * @brief 两路ADC按键扫描与显示滤波。
 * @details 停车前台依次读取两路各三次，按未滤波值判键后更新显示值；第二路可覆盖第一路。
 */
#include "bsp.h"
#include "hmi/menu_keys.h"
#include "sensing/filter.h"

/* 仅停车扫描更新的两路显示值和首次初始化的滤波历史。 */
static uint16_t s_key_adc1 = 0;
static uint16_t s_key_adc2 = 0;
static filter_lowpass_t s_filt_key1;
static filter_lowpass_t s_filt_key2;
static uint8_t s_filt_inited = 0;

/**
 * @brief 对指定按键 ADC 内存读三次求整数均值。
 * @details 按现有调用顺序使用 BSP_ADC_Read 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @param[in] ch BSP_ADC_KEY1 或 BSP_ADC_KEY2。
 * @return ADC 0..4095 的三读均值。
 * @note 停车前台调用，无额外延时或新转换要求。
 */
static uint16_t Menu_ReadKeyAverage(bsp_adc_channel_t ch)
{
    /* 连续读取三次 ADC 键值并累加，最后做整数平均；调用期间不改变菜单状态。 */
    uint32_t sum = 0;
    for (uint8_t i = 0; i < 3; i++)
    {
        sum += BSP_ADC_Read(ch);
    }
    return (uint16_t)(sum / 3);
}

/**
 * @brief 按原两路 ADC 区间判定按键。
 * @details 按现有调用顺序使用 Filter_InitLowpass、Menu_ReadKeyAverage、Filter_UpdateLowpass
 *     完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @return MENU_KEY_* 或 0 无键；第二路仍可覆盖第一路结果。
 * @note 仅停车前台调用；每路三次内存均值，先判键再更新显示滤波，不改判定阈值。
 */
uint8_t Menu_ScanKey(void)
{
    uint8_t key_status = 0;

    if (!s_filt_inited) /* 按键低通滤波器（源 key_init，α=0.7） */
    {
        Filter_InitLowpass(&s_filt_key1, 0.7f);
        Filter_InitLowpass(&s_filt_key2, 0.7f);
        s_filt_inited = 1;
    }

    s_key_adc1 = Menu_ReadKeyAverage(BSP_ADC_KEY1); /* 方向键+确定 */
    s_key_adc2 = Menu_ReadKeyAverage(BSP_ADC_KEY2); /* 复位+返回+调参 */

    if (s_key_adc1 / 100 > 3 && s_key_adc1 / 100 < 9)
    {
        key_status = MENU_KEY_UP; /* 约542 */
    }
    else if (s_key_adc1 / 100 > 9 && s_key_adc1 / 100 < 15)
    {
        key_status = MENU_KEY_DOWN; /* 约1262 */
    }
    else if (s_key_adc1 / 100 > 15 && s_key_adc1 / 100 < 20)
    {
        key_status = MENU_KEY_OK; /* 约1840 */
    }
    else if (s_key_adc1 / 100 > 20 && s_key_adc1 / 100 < 25)
    {
        key_status = MENU_KEY_LEFT; /* 约2467 */
    }
    else if (s_key_adc1 / 100 > 25 && s_key_adc1 / 100 < 31)
    {
        key_status = MENU_KEY_RIGHT; /* 约3082 */
    }

    if (s_key_adc2 / 100 > 3 && s_key_adc2 / 100 < 9)
    {
        key_status = MENU_KEY_RST; /* 约542 */
    }
    else if (s_key_adc2 / 100 > 9 && s_key_adc2 / 100 < 15)
    {
        key_status = MENU_KEY_ADJUST1; /* 约1262 */
    }
    else if (s_key_adc2 / 100 > 15 && s_key_adc2 / 100 < 20)
    {
        key_status = MENU_KEY_ADJUST2; /* 约1840 */
    }
    else if (s_key_adc2 / 100 > 20 && s_key_adc2 / 100 < 25)
    {
        key_status = MENU_KEY_FOUR; /* 约2467（预留） */
    }
    else if (s_key_adc2 / 100 > 25 && s_key_adc2 / 100 < 31)
    {
        key_status = MENU_KEY_BACK; /* 约3082 */
    }

    s_key_adc1 = (uint16_t)Filter_UpdateLowpass(&s_filt_key1, (float)s_key_adc1);
    s_key_adc2 = (uint16_t)Filter_UpdateLowpass(&s_filt_key2, (float)s_key_adc2);
    return key_status;
}

/**
 * @brief 读取第1路按键显示滤波值。
 * @return 最近扫描得到的滤波ADC值。
 */
uint16_t MenuKeys_GetAdc1(void)
{
    return s_key_adc1;
}

/**
 * @brief 读取第2路按键显示滤波值。
 * @return 最近扫描得到的滤波ADC值。
 */
uint16_t MenuKeys_GetAdc2(void)
{
    return s_key_adc2;
}
