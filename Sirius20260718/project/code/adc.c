// ============================================================
// 文件名: adc.c
// 功能说明: ADC采集与循迹误差计算模块
// 本文件实现四路ADC通道的采集、去极值平均滤波，
// 以及基于差分放大原理的电磁循迹偏差计算。
// ============================================================

#include "config.h"

//uint16 adc1 = 0;
//uint16 adc2 = 0;
//uint16 adc3 = 0;
//uint16 adc4 = 0;

uint16 adc_raw[4] = {0};        // 四路ADC原始采样值数组
int16 adc_filted[4] = {0};     // 四路ADC滤波后的输出值数组

int16 track_error = 0; 				// 循迹偏差值（由get_track_error计算）
int16 symmetry_x = 0;
int16 symmetry_y = 0;

int8 weight_x   = 15;	//15
int8 weight_xx  = 20;	//20
int8 weight_y   = 22;	//28
int8 weight_abs = 10;

uint16 adc_sum[4] = {0};        // 每路ADC累加和，用于平均滤波
uint16 max[4] = {0};            // 每路ADC采样最大值，用于去极值
uint16 min[4] = {4095,4095,4095,4095}; // 每路ADC采样最小值，用于去极值


// 函数名: track_error
// 功能: 计算循迹误差
// 返回值: 归一化的循迹偏差值，范围约[-1, 1]
// 说明: 采用差分放大公式：
//       diff = ((adc0 - adc3) + (adc1 - adc2)) / ((adc0 + adc3) + |adc1 - adc2|)
//       其中adc0/adc3为外侧电感，adc1/adc2为内侧电感。
int16 get_track_error(void)
{
	int16 diff = 0;  // 外侧电感差（带符号）
	int32 diff_x = 0;  // 外侧电感差（带符号）
    int32 diff_y = 0;  // 内侧电感差（带符号）
    int32 denominator = 0;
	
	static int16 diff_last = 0;   // 上次偏差
	
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


void symmetry_adc(void)
{
	int32 minus_x = 0;  // 外侧电感差（带符号）
    int32 minus_y = 0;  // 内侧电感差（带符号）
	
    minus_x = (adc_filted[0] - adc_filted[3]);  // 横电感差（带符号）
    minus_y = (adc_filted[1] - adc_filted[2]);  // 竖电感差（带符号）
	
	symmetry_x = (abs(minus_x) * 100) / (adc_filted[0] + adc_filted[3]);
	symmetry_y = (abs(minus_y) * 100) / (adc_filted[1] + adc_filted[2]);
}



// 函数名: read_adc
// 功能: 读取四路ADC并进行去极值平均滤波
// 说明: 采样8次，去掉每组的最大值和最小值，剩余6次取平均。
//       这样可以有效消除偶然噪声干扰。
void read_adc(void)
{
    static uint8 i = 0;
    static uint8 j = 0;
    
    // 初始化每组的最大值、最小值和累加和
    for( i = 0; i < 4; i++ )
    {
        max[i] = 0;           // 最大值初始化为0
        min[i] = 4095;        // 最小值初始化为ADC最大值4095
        adc_sum[i] = 0;       // 累加和清零
    }
    
    // 循环采样8次
    for( i = 0; i < 8; i++ ) 
    {
        // 依次读取四路ADC通道的原始值
        adc_raw[0] = adc_convert(ADC_CHANNEL1); 
        adc_raw[1] = adc_convert(ADC_CHANNEL2);    
        adc_raw[2] = adc_convert(ADC_CHANNEL3);    
        adc_raw[3] = adc_convert(ADC_CHANNEL4);    
        
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
// 功能: 初始化四路前置ADC通道
// 说明: 配置ADC_CHANNEL1~4为12位分辨率模式。
void front_adc_init(void)
{
    adc_init(ADC_CHANNEL1, ADC_12BIT); // 初始化通道1，12位精度
    adc_init(ADC_CHANNEL2, ADC_12BIT); // 初始化通道2，12位精度
    adc_init(ADC_CHANNEL3, ADC_12BIT); // 初始化通道3，12位精度
    adc_init(ADC_CHANNEL4, ADC_12BIT); // 初始化通道4，12位精度
}

// 以下为早期版本的备用代码，已注释停用

//void read_adc(void)
//{
//    adc1 = adc_convert(ADC_CHANNEL1);     
//    adc2 = adc_convert(ADC_CHANNEL2);    
//    adc3 = adc_convert(ADC_CHANNEL3);
//    adc4 = adc_convert(ADC_CHANNEL4);
//}
