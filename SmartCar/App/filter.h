/**
 * @file filter.h
 * @brief App 一阶低通与高通状态更新；控制拍调用，保留原浮点顺序。
 * @note 移植来源：Sirius20260718/project/code/filter.h，STC AI8051U 省赛快照（2026-07-18）。
 * @note 源算法文件未单列作者及独立许可证；保留原出处，关联 Phase 13/15 规约。
 */
#ifndef SMARTCAR_APP_FILTER_H
#define SMARTCAR_APP_FILTER_H

/* 一阶低通滤波器结构体 */
typedef struct
{
    float last_output; /**< 上一拍输出，与输入同单位；初始化清零。 */
    float alpha;       /**< 无量纲系数，调用方给定 0..1，更新不作范围检查。 */
} filter_lowpass_t;

/* 一阶高通滤波器结构体 */
typedef struct
{
    float alpha;      /**< RC/(RC+dt) 无量纲系数，初始化计算。 */
    float last_input; /**< 上一拍输入，与输入同单位。 */
    float output;     /**< 上一拍高通输出，与输入同单位。 */
} filter_highpass_t;

/* 对外接口函数 */

/**
 * @brief 清零低通历史并保存原系数。
 * @param[in,out] filt 非空低通状态；调用方独占。
 * @param[in] alpha 无量纲系数 0..1，不作范围检查。
 */
void Filter_InitLowpass(filter_lowpass_t *filt, float alpha);

/**
 * @brief 按原乘加顺序更新单步低通。
 * @param[in,out] filt 非空已初始化状态。
 * @param[in] input 当前样本，与历史输出同单位。
 * @return 本拍输出，与 input 同单位。
 * @note 控制拍调用，无阻塞；不得与其他写入并发。
 */
float Filter_UpdateLowpass(filter_lowpass_t *filt, float input);

/**
 * @brief 按截止频率和采样周期初始化高通。
 * @param[in,out] hpf 非空状态，清零输入和输出历史。
 * @param[in] cutoff_hz 截止频率 Hz，须大于 0。
 * @param[in] dt_s 采样周期秒，须大于 0。
 * @note 不作参数检查；当前 IMU 使用 0.2 Hz、2 ms。
 */
void Filter_InitHighpass(filter_highpass_t *hpf, float cutoff_hz, float dt_s);

/**
 * @brief 更新一阶高通并保存本拍输入。
 * @param[in,out] hpf 非空已初始化状态。
 * @param[in] input 当前样本，与输出同单位。
 * @return 本拍高通输出。
 * @note 控制拍调用，无阻塞，保持输出后输入历史的写入顺序。
 */
float Filter_UpdateHighpass(filter_highpass_t *hpf, float input);

#endif
