#ifndef __BSP_IMU660RB_H
#define __BSP_IMU660RB_H

#include "main.h"

/* IMU660RB（LSM6DSR 类，WHO_AM_I=0x0F）：硬件 SPI4 共享总线，CS=PD3。
 * 量程：陀螺仪 ±2000dps（原始值/14.3 = °/s），加速度 ±8g（原始值/4098 = g）。
 * 寄存器配置序列移植自逐飞 zf_device_imu660rb.c，轴符号约定保持原样。 */

uint8_t BSP_IMU660RB_Init(void);    /* 0=成功，1=失败（自检不通过） */
void    BSP_IMU660RB_GetAcc(int16_t *x, int16_t *y, int16_t *z);
void    BSP_IMU660RB_GetGyro(int16_t *x, int16_t *y, int16_t *z);

#endif /* __BSP_IMU660RB_H */
