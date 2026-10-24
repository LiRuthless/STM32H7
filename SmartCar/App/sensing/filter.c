/**
 * @file filter.c
 * @brief App 一阶低通与高通状态更新；控制拍调用，保留原浮点顺序。
 * @details 实现控制链使用的一阶低通和高通滤波器，状态由调用方提供并在固定采样周期内更新。
 * @note 移植来源：Sirius20260718/project/code/filter.c，STC AI8051U 省赛快照（2026-07-18）。
 * @note 源算法文件未单列作者及独立许可证；保留原出处，关联 Phase 13/15 规约。
 */
#include "sensing/filter.h"

/**
 * @brief 清零低通历史并保存原系数。
 * @details 保存调用方直接给定的 alpha，并清零历史输出；本入口不从截止频率推导系数。
 * @param[in,out] filt 非空低通状态；调用方独占。
 * @param[in] alpha 无量纲系数 0..1，不作范围检查。
 * @note 调用方持有 filt；初始化和再次起跑复位均可调用，不访问共享模块状态。
 */
void Filter_InitLowpass(filter_lowpass_t *filt, float alpha)
{
    filt->last_output = 0.0f; /* 初始输出为0 */
    filt->alpha = alpha;      /* 设置滤波系数 */
}

/**
 * @brief 按原乘加顺序更新单步低通。
 * @details 先以 alpha*input+(1-alpha)*last_output 覆盖历史，再返回同一输出；只写调用方状态。
 * @param[in,out] filt 非空已初始化状态。
 * @param[in] input 当前样本，与历史输出同单位。
 * @return 本拍输出，与 input 同单位。
 * @note 控制拍调用，无阻塞；不得与其他写入并发。
 */
float Filter_UpdateLowpass(filter_lowpass_t *filt, float input)
{
    /* 一阶IIR滤波计算：新输出 = alpha * 输入 + (1-alpha) * 上次的输出 */
    filt->last_output = filt->alpha * input + (1.0f - filt->alpha) * filt->last_output;
    return filt->last_output; /* 返回滤波结果 */
}

/* 一阶高通滤波器初始化 */
/**
 * @brief 按截止频率和采样周期初始化高通。
 * @details 先按 rc=1/(2*pi*cutoff_hz) 求时间常数，再取 rc/(rc+dt_s)；输入及输出历史均清零。
 * @param[in,out] hpf 非空状态，清零输入和输出历史。
 * @param[in] cutoff_hz 截止频率 Hz，须大于 0。
 * @param[in] dt_s 采样周期秒，须大于 0。
 * @note 不作参数检查；当前 IMU 使用 0.2 Hz、2 ms。
 */
void Filter_InitHighpass(filter_highpass_t *hpf, float cutoff_hz, float dt_s)
{
    float rc = 1.0f / (2.0f * 3.1415926f * cutoff_hz);
    hpf->alpha = rc / (rc + dt_s);
    hpf->last_input = 0.0f;
    hpf->output = 0.0f;
}

/* 高通滤波器更新，每次采样调用一次 */
/**
 * @brief 更新一阶高通并保存本拍输入。
 * @details 先计算 alpha*(上拍输出+本拍输入-上拍输入)，再保存本拍输入；返回新输出。
 * @param[in,out] hpf 非空已初始化状态。
 * @param[in] input 当前样本，与输出同单位。
 * @return 本拍高通输出。
 * @note 控制拍调用，无阻塞，保持输出后输入历史的写入顺序。
 */
float Filter_UpdateHighpass(filter_highpass_t *hpf, float input)
{
    hpf->output = hpf->alpha * (hpf->output + input - hpf->last_input);
    hpf->last_input = input;
    return hpf->output;
}
