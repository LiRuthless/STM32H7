#ifndef _TRACK_SENSOR_H_
#define _TRACK_SENSOR_H_

#include <stdint.h>

extern uint16_t adc_raw[4];     // 四路ADC原始采样值数组
extern int16_t adc_filted[4];   // 四路ADC滤波后的输出值数组（[0]横左 [1]竖左 [2]竖右 [3]横右）
extern int16_t track_error;     // 循迹偏差值
extern int16_t symmetry_x;      // 横电感对称度（%）
extern int16_t symmetry_y;      // 竖电感对称度（%）

extern int8_t weight_x;         // 横电感差权重
extern int8_t weight_xx;        // 横电感和权重
extern int8_t weight_y;         // 竖电感差权重
extern int8_t weight_abs;       // 竖电感差绝对值权重

int16_t get_track_error(void);  // 计算循迹误差
void symmetry_adc(void);        // 计算电感对称度
void front_adc_init(void);      // 前置ADC初始化（薄封装，DMA已自动运行）
void read_adc(void);            // 读取并滤波四路ADC

#endif
