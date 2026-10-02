#ifndef __BSP_DL1B_H
#define __BSP_DL1B_H

#include "main.h"

/* DL1B 激光测距（VL53 内核，7 位地址 0x29）：硬件 I2C2（PB10/PB11，≤400kHz），XS=PE8。
 * 寄存器流程移植自逐飞 zf_device_dl1b.c。
 * 距离约定：单位 mm；无效/超量程返回 8192。 */
#define BSP_DL1B_INVALID    8192u

uint8_t  BSP_DL1B_Init(void);            /* 0=成功 */
void     BSP_DL1B_Update(void);          /* TIM6 控制环每 2ms 调用，非阻塞轮询读取 */
uint16_t BSP_DL1B_GetDistanceMm(void);

#endif /* __BSP_DL1B_H */
