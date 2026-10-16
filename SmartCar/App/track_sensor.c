/**
 * @file track_sensor.c
 * @brief App 四路电感滤波与误差；TIM6 内存读取，原八次去极值平均不变。
 * @note 移植来源：Sirius20260718/project/code/adc.c，STC AI8051U 省赛快照（2026-07-18）。
 * @note 源算法文件未单列作者及独立许可证；保留原出处，关联 Phase 13/15 规约。
 */
#include "bsp.h"
#include "app_config.h"
#include <stdlib.h>
#include "track_sensor.h"

/* TIM6 持有采样滤波中间量和观察结果；前台按原停车约定读取。 */
static track_state_t s_track = { .weights = { 15, 20, 22, 10 } };
static uint16_t s_adc_sum[4];
static uint16_t s_max[4];
static uint16_t s_min[4];

/* 通道映射：[0]横左 [1]竖左 [2]竖右 [3]横右 */
static const bsp_adc_channel_t s_adc_channel_map[4]
    = { BSP_ADC_IND_H_L, BSP_ADC_IND_V_L, BSP_ADC_IND_V_R, BSP_ADC_IND_H_R };

const track_state_t *Track_GetState(void)
{
    return &s_track;
}

int16_t Track_CalculateError(const int16_t *a, const track_weights_t *w)
{
    int32_t dx = a[0] - a[3];
    int32_t dy = a[1] - a[2];
    int32_t denominator = w->xx * (a[0] + a[3]) + w->abs * labs(dy);
    return denominator ? (int16_t)(100 * (w->x * dx + w->y * dy) / denominator) : 0;
}

void Track_PublishObservation(
    const track_weights_t *weights, int16_t error, int16_t symmetry_x, int16_t symmetry_y)
{
    s_track.weights = *weights;
    s_track.error = error;
    s_track.symmetry_x = symmetry_x;
    s_track.symmetry_y = symmetry_y;
}

void Track_SetWeights(const track_weights_t *weights)
{
    s_track.weights.x = weights->x;
    s_track.weights.xx = weights->xx;
    s_track.weights.y = weights->y;
    s_track.weights.abs = weights->abs;
}

int16_t Track_UpdateError(void)
{
    int16_t diff = 0;   /* 循迹偏差（带符号） */
    int32_t diff_x = 0; /* 横电感差（带符号） */
    int32_t diff_y = 0; /* 竖电感差（带符号） */
    int32_t denominator = 0;
    int32_t sum_x;
    int32_t sum_y;

    diff_x = (s_track.filtered[0] - s_track.filtered[3]); /* 横电感差（带符号） */
    diff_y = (s_track.filtered[1] - s_track.filtered[2]); /* 竖电感差（带符号） */

    sum_x = s_track.filtered[0] + s_track.filtered[3];
    sum_y = s_track.filtered[1] + s_track.filtered[2];
    s_track.symmetry_x = (sum_x == 0) ? 0 : (abs(diff_x) * 100) / sum_x;
    s_track.symmetry_y = (sum_y == 0) ? 0 : (abs(diff_y) * 100) / sum_y;

    denominator = s_track.weights.xx * (s_track.filtered[0] + s_track.filtered[3])
        + s_track.weights.abs * labs(s_track.filtered[1] - s_track.filtered[2]);

    if (denominator == 0)
    {
        s_track.error = 0;
        return 0;
    }

    diff = 100 * (s_track.weights.x * diff_x + s_track.weights.y * diff_y) / denominator;
    s_track.error = diff;

    return diff;
}

void Track_UpdateSymmetry(void)
{
    int32_t minus_x = 0; /* 横电感差（带符号） */
    int32_t minus_y = 0; /* 竖电感差（带符号） */
    int32_t sum_x;
    int32_t sum_y;

    minus_x = (s_track.filtered[0] - s_track.filtered[3]); /* 横电感差（带符号） */
    minus_y = (s_track.filtered[1] - s_track.filtered[2]); /* 竖电感差（带符号） */

    sum_x = s_track.filtered[0] + s_track.filtered[3];
    sum_y = s_track.filtered[1] + s_track.filtered[2];
    s_track.symmetry_x = (sum_x == 0) ? 0 : (abs(minus_x) * 100) / sum_x;
    s_track.symmetry_y = (sum_y == 0) ? 0 : (abs(minus_y) * 100) / sum_y;
}

void Track_ReadAdc(void)
{
    uint8_t i;
    uint8_t j;

    /* 初始化每组的最大值、最小值和累加和 */
    for (i = 0; i < 4; i++)
    {
        s_max[i] = 0;              /* 最大值初始化为0 */
        s_min[i] = APP_ADC_FULL_SCALE; /* 最小值初始化为ADC最大值4095 */
        s_adc_sum[i] = 0;          /* 累加和清零 */
    }

    /* 循环采样8次 */
    for (i = 0; i < 8; i++)
    {
        /* 依次读取四路ADC通道的原始值 */
        s_track.raw[0] = BSP_ADC_Read(s_adc_channel_map[0]);
        s_track.raw[1] = BSP_ADC_Read(s_adc_channel_map[1]);
        s_track.raw[2] = BSP_ADC_Read(s_adc_channel_map[2]);
        s_track.raw[3] = BSP_ADC_Read(s_adc_channel_map[3]);

        /* 对四路数据分别更新累加和、最大值、最小值 */
        for (j = 0; j < 4; j++)
        {
            s_adc_sum[j] += s_track.raw[j]; /* 累加当前采样值 */
            if (s_max[j] < s_track.raw[j])
            {
                s_max[j] = s_track.raw[j];
            }
            if (s_min[j] > s_track.raw[j])
            {
                s_min[j] = s_track.raw[j];
            }
        }
    }

    /* 去掉每组的最大值和最小值后，剩余6次求平均 */
    for (i = 0; i < 4; i++)
    {
        s_adc_sum[i] -= s_max[i];
        s_adc_sum[i] -= s_min[i];
        s_track.filtered[i] = s_adc_sum[i] / 6;
    }
}

void Track_InitAdc(void)
{
    BSP_ADC_Init();
}
