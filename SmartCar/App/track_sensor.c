// ============================================================
// 文件名: track_sensor.c  （移植自源工程 adc.c）
// 功能说明: 四路电感ADC采集与循迹误差计算模块
// 实现四路ADC通道的读取、去极值平均滤波，
// 以及基于加权差分归一化的电磁循迹偏差计算。
// 本工程 ADC 由 DMA 连续采集，read_adc() 改为对 BSP_ADC_Read
// 做 8 次读取去极值平均（纯内存读，开销可忽略）。
// ============================================================

#include "bsp.h"
#include "app_config.h"
#include <stdlib.h>
#include "track_sensor.h"

static track_state_t s_track = {
    .weights = {15, 20, 22, 10}
};
static uint16_t s_adc_sum[4];
static uint16_t s_max[4];
static uint16_t s_min[4];

const track_state_t *Track_GetState(void)
{
    return &s_track;
}

void Track_SetWeights(const track_weights_t *weights)
{
    s_track.weights.x = weights->x;
    s_track.weights.xx = weights->xx;
    s_track.weights.y = weights->y;
    s_track.weights.abs = weights->abs;
}

// 通道映射：[0]横左 [1]竖左 [2]竖右 [3]横右
static const bsp_adc_channel_t adc_ch_map[4] =
{
    BSP_ADC_IND_H_L,
    BSP_ADC_IND_V_L,
    BSP_ADC_IND_V_R,
    BSP_ADC_IND_H_R
};


// 函数名: get_track_error
// 功能: 计算循迹误差
// 返回值: 归一化的循迹偏差值，范围约[-100, 100]
// 说明: 采用加权差分归一化公式：
//       diff = 100*(weight_x*横差 + weight_y*竖差) / (weight_xx*横和 + weight_abs*|竖差|)
int16_t get_track_error(void)
{
	int16_t diff = 0;      // 循迹偏差（带符号）
	int32_t diff_x = 0;    // 横电感差（带符号）
    int32_t diff_y = 0;    // 竖电感差（带符号）
    int32_t denominator = 0;
    int32_t sum_x;
    int32_t sum_y;

    diff_x = (s_track.filtered[0] - s_track.filtered[3]);  // 横电感差（带符号）
    diff_y = (s_track.filtered[1] - s_track.filtered[2]);  // 竖电感差（带符号）

    sum_x = s_track.filtered[0] + s_track.filtered[3];
    sum_y = s_track.filtered[1] + s_track.filtered[2];
    s_track.symmetry_x = (sum_x == 0) ? 0 : (abs(diff_x) * 100) / sum_x;
    s_track.symmetry_y = (sum_y == 0) ? 0 : (abs(diff_y) * 100) / sum_y;

    denominator = s_track.weights.xx * (s_track.filtered[0] + s_track.filtered[3]) + s_track.weights.abs * labs(s_track.filtered[1] - s_track.filtered[2]);

    if (denominator == 0)
    {
        s_track.error = 0;
        return 0;
    }

	diff = 100 * (s_track.weights.x * diff_x + s_track.weights.y * diff_y) / denominator;
	s_track.error = diff;

    return diff;
}


// 函数名: symmetry_adc
// 功能: 计算横/竖电感对称度（0~100，越小越对称）
void symmetry_adc(void)
{
	int32_t minus_x = 0;   // 横电感差（带符号）
    int32_t minus_y = 0;   // 竖电感差（带符号）
    int32_t sum_x;
    int32_t sum_y;

    minus_x = (s_track.filtered[0] - s_track.filtered[3]);  // 横电感差（带符号）
    minus_y = (s_track.filtered[1] - s_track.filtered[2]);  // 竖电感差（带符号）

    sum_x = s_track.filtered[0] + s_track.filtered[3];
    sum_y = s_track.filtered[1] + s_track.filtered[2];
    s_track.symmetry_x = (sum_x == 0) ? 0 : (abs(minus_x) * 100) / sum_x;
    s_track.symmetry_y = (sum_y == 0) ? 0 : (abs(minus_y) * 100) / sum_y;
}



// 函数名: read_adc
// 功能: 读取四路ADC并进行去极值平均滤波
// 说明: 采样8次，去掉每组的最大值和最小值，剩余6次取平均。
//       本工程 ADC 由 DMA 连续采集，这里仅对 BSP_ADC_Read 做 8 次内存读取。
void read_adc(void)
{
    uint8_t i;
    uint8_t j;

    // 初始化每组的最大值、最小值和累加和
    for( i = 0; i < 4; i++ )
    {
        s_max[i] = 0;                     // 最大值初始化为0
        s_min[i] = ADC_FULL_SCALE;        // 最小值初始化为ADC最大值4095
        s_adc_sum[i] = 0;                 // 累加和清零
    }

    // 循环采样8次
    for( i = 0; i < 8; i++ )
    {
        // 依次读取四路ADC通道的原始值
        s_track.raw[0] = BSP_ADC_Read(adc_ch_map[0]);
        s_track.raw[1] = BSP_ADC_Read(adc_ch_map[1]);
        s_track.raw[2] = BSP_ADC_Read(adc_ch_map[2]);
        s_track.raw[3] = BSP_ADC_Read(adc_ch_map[3]);

        // 对四路数据分别更新累加和、最大值、最小值
        for( j = 0; j < 4; j++ )
        {
            s_adc_sum[j] += s_track.raw[j];                  // 累加当前采样值
            if(s_max[j] < s_track.raw[j]) s_max[j] = s_track.raw[j];
            if(s_min[j] > s_track.raw[j]) s_min[j] = s_track.raw[j];
        }
    }

    // 去掉每组的最大值和最小值后，剩余6次求平均
    for( i = 0; i < 4; i++ )
    {
        s_adc_sum[i] -= s_max[i];
        s_adc_sum[i] -= s_min[i];
        s_track.filtered[i] = s_adc_sum[i] / 6;
    }
}

// 函数名: front_adc_init
// 功能: 前置ADC初始化
// 说明: 本工程 ADC 校准与 DMA 循环采集由 BSP_ADC_Init 完成并在启动后自动运行，
//       此处仅作薄封装以保持源工程接口不变。
void front_adc_init(void)
{
    BSP_ADC_Init();
}
