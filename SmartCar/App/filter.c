/**
 * @file filter.c
 * @brief App 一阶低通与高通状态更新；控制拍调用，保留原浮点顺序。
 * @note 移植来源：Sirius20260718/project/code/filter.c，STC AI8051U 省赛快照（2026-07-18）。
 * @note 源算法文件未单列作者及独立许可证；保留原出处，关联 Phase 13/15 规约。
 */
#include "filter.h"

void Filter_InitLowpass(filter_lowpass_t *filt, float alpha)
{
    filt->last_output = 0.0f; /* 初始输出为0 */
    filt->alpha = alpha;      /* 设置滤波系数 */
}

float Filter_UpdateLowpass(filter_lowpass_t *filt, float input)
{
    /* 一阶IIR滤波计算：新输出 = alpha * 输入 + (1-alpha) * 上次的输出 */
    filt->last_output = filt->alpha * input + (1.0f - filt->alpha) * filt->last_output;
    return filt->last_output; /* 返回滤波结果 */
}

/* 一阶高通滤波器初始化 */
void Filter_InitHighpass(filter_highpass_t *hpf, float cutoff_hz, float dt_s)
{
    float rc = 1.0f / (2.0f * 3.1415926f * cutoff_hz);
    hpf->alpha = rc / (rc + dt_s);
    hpf->last_input = 0.0f;
    hpf->output = 0.0f;
}

/* 高通滤波器更新，每次采样调用一次 */
float Filter_UpdateHighpass(filter_highpass_t *hpf, float input)
{
    hpf->output = hpf->alpha * (hpf->output + input - hpf->last_input);
    hpf->last_input = input;
    return hpf->output;
}
