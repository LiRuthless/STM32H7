#ifndef _FILTER_H_
#define _FILTER_H_

// 一阶低通滤波器结构体
typedef struct {
    float last_output;  // 上一次滤波输出值
    float alpha;        // 滤波系数 0~1
} LowPassFilter;

void lowpass_init(LowPassFilter *filt, float alpha);    // 初始化低通滤波器
float lowpass_update(LowPassFilter *filt, float input); // 低通滤波器单步更新



/* 一阶高通滤波器结构体 */
typedef struct {
    float alpha;        /* 滤波系数 α = RC/(RC+dt) */
    float last_input;   /* 上一次输入 x[n-1] */
    float output;       /* 上一次输出 y[n-1] */
} gyro_hpf_t;

/* 对外接口函数 */
void  gyro_hpf_init(gyro_hpf_t *hpf, float cutoff_hz, float dt_s);
float gyro_hpf_update(gyro_hpf_t *hpf, float input);

/* y轴滤波器实例，定义在 .c 中，这里用 extern 供其他文件使用 */
extern gyro_hpf_t gyro_hpf_x;
extern gyro_hpf_t gyro_hpf_y;
#endif
