#ifndef _TRACK_SENSOR_H_
#define _TRACK_SENSOR_H_

#include <stdint.h>

typedef struct {
    int8_t x;
    int8_t xx;
    int8_t y;
    int8_t abs;
} track_weights_t;

typedef struct {
    uint16_t raw[4];            // [0]横左 [1]竖左 [2]竖右 [3]横右
    int16_t filtered[4];
    int16_t error;
    int16_t symmetry_x;
    int16_t symmetry_y;
    track_weights_t weights;
} track_state_t;

const track_state_t *Track_GetState(void);
void Track_SetWeights(const track_weights_t *weights);

int16_t get_track_error(void);  // 计算循迹误差
void symmetry_adc(void);        // 计算电感对称度
void front_adc_init(void);      // 前置ADC初始化（薄封装，DMA已自动运行）
void read_adc(void);            // 读取并滤波四路ADC

#endif
