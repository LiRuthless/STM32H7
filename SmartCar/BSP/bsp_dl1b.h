/**
 * @file bsp_dl1b.h
 * @brief BSP DL1B 测距驱动；初始化由主循环、更新由 TIM7 调用。
 * @details 声明 DL1B 初始化与显示接口；完整位时序和缓存行为位于 bsp_dl1b.c。
 */
#ifndef SMARTCAR_BSP_BSP_DL1B_H
#define SMARTCAR_BSP_BSP_DL1B_H

#include "main.h"

/* DL1B 激光测距（VL53 内核，7 位地址 0x29）：硬件 I2C2（PB10/PB11，≤400kHz），XS=PE8。
 * 寄存器流程移植自逐飞 zf_device_dl1b.c。
 * 距离约定：单位 mm；无效/超量程返回 8192。 */
#define BSP_DL1B_INVALID 8192u

/**
 * @brief 复位 DL1B、校验固件状态并写入原配置。
 * @note 启动主循环阻塞调用；失败不主动清除原有初始化标志。 完整契约见 bsp_dl1b.c::BSP_DL1B_Init。
 */
uint8_t BSP_DL1B_Init(void);
/**
 * @brief 读取 DL1B 就绪状态并更新距离缓存。
 * @note TIM7 调用且有 I2C 阻塞；未初始化直接返回，未就绪或读取失败发布 INVALID。 完整契约见 bsp_dl1b.c::BSP_DL1B_Update。
 */
void BSP_DL1B_Update(void);
/**
 * @brief 读取最近一次测距缓存。
 * @note 完整契约见 bsp_dl1b.c::BSP_DL1B_GetDistanceMm。
 */
uint16_t BSP_DL1B_GetDistanceMm(void);

#endif /* __BSP_DL1B_H */
