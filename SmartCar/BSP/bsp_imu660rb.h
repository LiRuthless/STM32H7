/**
 * @file bsp_imu660rb.h
 * @brief BSP IMU660RB SPI4 驱动；启动校准及 TIM15 原始采样。
 */
#ifndef SMARTCAR_BSP_BSP_IMU660RB_H
#define SMARTCAR_BSP_BSP_IMU660RB_H

#include "main.h"

/* IMU660RB（LSM6DSR 类，WHO_AM_I=0x0F）：硬件 SPI4 共享总线，CS=PD3。
 * 量程：陀螺仪 ±2000dps（原始值/14.3 = °/s），加速度 ±8g（原始值/4098 = g）。
 * 寄存器配置序列移植自逐飞 zf_device_imu660rb.c，轴符号约定保持原样。 */

/**
 * @brief 检查设备标识并按原序列配置 IMU660RB。
 * @return 0 标识检查成功并完成配置；1 标识轮询超时。
 * @note 启动主循环调用，含 HAL_Delay，配置写入保持原 HAL 返回值忽略行为。
 */
uint8_t BSP_IMU660RB_Init(void);
/**
 * @brief 读取三轴加速度原始值。
 * @param[out] x X 轴 int16_t 原始计数；必须非空，调用期间依次写入。
 * @param[out] y Y 轴 int16_t 原始计数；必须非空，调用期间依次写入。
 * @param[out] z Z 轴 int16_t 原始计数；必须非空，调用期间依次写入。
 * @note SPI4 阻塞读取；TIM15 或启动校准调用，不与 LCD 并发；保持原 HAL 错误忽略行为。
 */
void BSP_IMU660RB_GetAcc(int16_t *x, int16_t *y, int16_t *z);
/**
 * @brief 读取三轴陀螺仪原始值。
 * @param[out] x X 轴 int16_t 原始计数；必须非空，调用期间依次写入。
 * @param[out] y Y 轴 int16_t 原始计数；必须非空，调用期间依次写入。
 * @param[out] z Z 轴 int16_t 原始计数；必须非空，调用期间依次写入。
 * @note SPI4 阻塞读取；TIM15 或启动校准调用，不与 LCD 并发；保持原 HAL 错误忽略行为。
 */
void BSP_IMU660RB_GetGyro(int16_t *x, int16_t *y, int16_t *z);

#endif /* __BSP_IMU660RB_H */
