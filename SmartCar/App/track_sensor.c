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

uint16_t adc_raw[4] = {0};       // 四路ADC原始采样值数组
int16_t adc_filted[4] = {0};     // 四路ADC滤波后的输出值数组

int16_t track_error = 0; 				// 循迹偏差值（由get_track_error计算）
int16_t symmetry_x = 0;
int16_t symmetry_y = 0;

int8_t weight_x   = 15;	//15
int8_t weight_xx  = 20;	//20
int8_t weight_y   = 22;	//28
int8_t weight_abs = 10;

uint16_t adc_sum[4] = {0};        // 每路ADC累加和，用于平均滤波
uint16_t max[4] = {0};            // 每路ADC采样最大值，用于去极值
uint16_t min[4] = {ADC_FULL_SCALE, ADC_FULL_SCALE, ADC_FULL_SCALE, ADC_FULL_SCALE}; // 每路ADC采样最小值

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

	static int16_t diff_last = 0;   // 上次偏差

    diff_x = (adc_filted[0] - adc_filted[3]);  // 横电感差（带符号）
    diff_y = (adc_filted[1] - adc_filted[2]);  // 竖电感差（带符号）

	symmetry_x = (abs(diff_x) * 100) / (adc_filted[0] + adc_filted[3]);
	symmetry_y = (abs(diff_y) * 100) / (adc_filted[1] + adc_filted[2]);

    denominator = weight_xx * (adc_filted[0] + adc_filted[3]) + weight_abs * labs(adc_filted[1] - adc_filted[2]);

    if (denominator == 0)	return 0;  		// 避免除零：电感值全为0时返回0偏差

	diff = 100 * (weight_x * diff_x + weight_y * diff_y) / denominator;

	diff_last = diff;

    return diff;
}


// 函数名: symmetry_adc
// 功能: 计算横/竖电感对称度（0~100，越小越对称）
void symmetry_adc(void)
{
	int32_t minus_x = 0;   // 横电感差（带符号）
    int32_t minus_y = 0;   // 竖电感差（带符号）

    minus_x = (adc_filted[0] - adc_filted[3]);  // 横电感差（带符号）
    minus_y = (adc_filted[1] - adc_filted[2]);  // 竖电感差（带符号）

	symmetry_x = (abs(minus_x) * 100) / (adc_filted[0] + adc_filted[3]);
	symmetry_y = (abs(minus_y) * 100) / (adc_filted[1] + adc_filted[2]);
}



// 函数名: read_adc
// 功能: 读取四路ADC并进行去极值平均滤波
// 说明: 采样8次，去掉每组的最大值和最小值，剩余6次取平均。
//       本工程 ADC 由 DMA 连续采集，这里仅对 BSP_ADC_Read 做 8 次内存读取。
void read_adc(void)
{
    static uint8_t i = 0;
    static uint8_t j = 0;

    // 初始化每组的最大值、最小值和累加和
    for( i = 0; i < 4; i++ )
    {
        max[i] = 0;                     // 最大值初始化为0
        min[i] = ADC_FULL_SCALE;        // 最小值初始化为ADC最大值4095
        adc_sum[i] = 0;                 // 累加和清零
    }

    // 循环采样8次
    for( i = 0; i < 8; i++ )
    {
        // 依次读取四路ADC通道的原始值
        adc_raw[0] = BSP_ADC_Read(adc_ch_map[0]);
        adc_raw[1] = BSP_ADC_Read(adc_ch_map[1]);
        adc_raw[2] = BSP_ADC_Read(adc_ch_map[2]);
        adc_raw[3] = BSP_ADC_Read(adc_ch_map[3]);

        // 对四路数据分别更新累加和、最大值、最小值
        for( j = 0; j < 4; j++ )
        {
            adc_sum[j] += adc_raw[j];                       // 累加当前采样值
            if(max[j] < adc_raw[j]) max[j] = adc_raw[j];    // 更新最大值
            if(min[j] > adc_raw[j]) min[j] = adc_raw[j];    // 更新最小值
        }
    }

    // 去掉每组的最大值和最小值后，剩余6次求平均
    for( i = 0; i < 4; i++ )
    {
        adc_sum[i] -= max[i];            // 减去最大值
        adc_sum[i] -= min[i];            // 减去最小值
        adc_filted[i] = adc_sum[i] / 6;  // 8-2=6次平均作为滤波结果
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
