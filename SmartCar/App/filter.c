// ============================================================
// 文件名: filter.c
// 功能说明: 一阶低通/高通滤波器实现（移植自 8051 源工程，算法原样照搬）
// 低通滤波公式：y[n] = alpha * x[n] + (1-alpha) * y[n-1]
// ============================================================

#include "filter.h"

// 函数名: lowpass_init
// 功能: 初始化低通滤波器
// 参数: filt  - 低通滤波器结构体指针
//       alpha - 滤波系数，范围(0,1)。越大响应越快，越小平滑效果越好
void lowpass_init(LowPassFilter *filt, float alpha) {
    filt->last_output = 0.0f;   // 初始输出为0
    filt->alpha = alpha;        // 设置滤波系数
}

// 函数名: lowpass_update
// 功能: 低通滤波器更新（单步计算）
// 参数: filt  - 低通滤波器结构体指针
//       input - 当前输入值
// 返回值: 滤波后的输出值
float lowpass_update(LowPassFilter *filt, float input) {
    // 一阶IIR滤波计算：新输出 = alpha * 输入 + (1-alpha) * 上次的输出
    filt->last_output = filt->alpha * input + (1.0f - filt->alpha) * filt->last_output;
    return filt->last_output;   // 返回滤波结果
}

// 一阶高通滤波器初始化
void gyro_hpf_init(gyro_hpf_t *hpf, float cutoff_hz, float dt_s)
{
    float rc = 1.0f / (2.0f * 3.1415926f * cutoff_hz);
    hpf->alpha      = rc / (rc + dt_s);
    hpf->last_input = 0.0f;
    hpf->output     = 0.0f;
}

// 高通滤波器更新，每次采样调用一次
float gyro_hpf_update(gyro_hpf_t *hpf, float input)
{
    hpf->output     = hpf->alpha * (hpf->output + input - hpf->last_input);
    hpf->last_input = input;
    return hpf->output;
}
