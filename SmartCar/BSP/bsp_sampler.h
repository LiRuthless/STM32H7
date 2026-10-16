/**
 * @file bsp_sampler.h
 * @brief BSP TIM15 一毫秒采样调度；与 TIM6 同优先级且互不抢占。
 */
#ifndef SMARTCAR_BSP_BSP_SAMPLER_H
#define SMARTCAR_BSP_BSP_SAMPLER_H

#include "main.h"

/* 高速采样器：TIM15 1ms（1kHz）周期中断，纯寄存器配置（CubeMX 未占用 TIM15）。
 * 职责：1ms 读取左右编码器（读即清零）并累计、1ms 读取 IMU 陀螺/加速度原始值。
 * NVIC 优先级 0（与 TIM6 控制环同级，互不抢占，SPI4 访问不会交错）。
 * 控制环（TIM6 2ms）通过 ConsumeEnc 接口取 2ms 累计值，控制周期不变。 */

/**
 * @brief 配置 TIM15 一毫秒周期与优先级零，保持未启动。
 * @note 启动主循环调用；TIM15 与 TIM6 同级，保证 SPI4 及累计量访问不抢占。
 */
void BSP_Sampler_Init(void);
/**
 * @brief 清零采样拍与轮脉冲累计并启动 TIM15。
 * @note 起跑前调用；保留计数器、状态标志及使能的写入次序。
 */
void BSP_Sampler_Start(void);
/**
 * @brief 关闭 TIM15 计数使采样停止。
 * @note 安全停车路径可调用，不改已有采样值。
 */
void BSP_Sampler_Stop(void);

/**
 * @brief 读取最近采样节拍。
 * @return 自启动以来一毫秒拍数，uint32_t 自然回绕。
 */
uint32_t BSP_Sampler_GetTick(void);

/* 最近一个 1ms 窗口的编码器计数（符号已按 BSP_ENCODER_*_INVERT 处理，前进为正） */
/**
 * @brief 读取左轮最近一毫秒脉冲。
 * @return 已按编码器方向配置处理的 int16_t 脉冲计数。
 * @note TIM15 写入；TIM6 同级读取，前进为正。
 */
int16_t BSP_Sampler_GetEncL_1ms(void);
/**
 * @brief 读取右轮最近一毫秒脉冲。
 * @return 已按编码器方向配置处理的 int16_t 脉冲计数。
 * @note TIM15 写入；TIM6 同级读取，前进为正。
 */
int16_t BSP_Sampler_GetEncR_1ms(void);

/* 最近一次的 IMU 原始值（1kHz 更新） */
/**
 * @brief 读取三轴陀螺仪原始值。
 * @param[out] x X 轴 int16_t 原始计数；必须非空，调用期间依次写入。
 * @param[out] y Y 轴 int16_t 原始计数；必须非空，调用期间依次写入。
 * @param[out] z Z 轴 int16_t 原始计数；必须非空，调用期间依次写入。
 * @note TIM15 发布、TIM6 同级读取；前台应在采样停止后读取一致快照。
 */
void BSP_Sampler_GetGyroRaw(int16_t *x, int16_t *y, int16_t *z);
/**
 * @brief 读取三轴加速度原始值。
 * @param[out] x X 轴 int16_t 原始计数；必须非空，调用期间依次写入。
 * @param[out] y Y 轴 int16_t 原始计数；必须非空，调用期间依次写入。
 * @param[out] z Z 轴 int16_t 原始计数；必须非空，调用期间依次写入。
 * @note TIM15 发布、TIM6 同级读取；前台应在采样停止后读取一致快照。
 */
void BSP_Sampler_GetAccRaw(int16_t *x, int16_t *y, int16_t *z);

/* 控制环 2ms 消费接口：返回自上次调用以来的累计计数并清零（前进为正） */
/**
 * @brief 取出并清零左轮累计脉冲。
 * @return 自上次消费起累计的 int16_t 脉冲计数，前进为正。
 * @note TIM6 控制拍调用；与 TIM15 同级保证读清过程不被采样打断。
 */
int16_t BSP_Sampler_ConsumeEncL(void);
/**
 * @brief 取出并清零右轮累计脉冲。
 * @return 自上次消费起累计的 int16_t 脉冲计数，前进为正。
 * @note TIM6 控制拍调用；与 TIM15 同级保证读清过程不被采样打断。
 */
int16_t BSP_Sampler_ConsumeEncR(void);

#endif /* __BSP_SAMPLER_H */
