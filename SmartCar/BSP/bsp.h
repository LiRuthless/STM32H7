/**
 * @file bsp.h
 * @brief 固件 BSP 聚合入口，向 App 提供板级硬件访问接口。
 * @details 聚合应用层获准使用的 BSP 接口；只提供依赖入口，不新增运行状态或硬件行为。
 */
#ifndef SMARTCAR_BSP_BSP_H
#define SMARTCAR_BSP_BSP_H

/* 板级支持包汇总头：收拢全部 HAL 依赖，应用层只包含本文件 */
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

#include "bsp_uart.h"
#include "bsp_adc.h"
#include "bsp_time.h"
#include "bsp_gray.h"
#include "bsp_pwm.h"
#include "bsp_encoder.h"
#include "bsp_key.h"
#include "bsp_lcd.h"
#include "bsp_imu660rb.h"
#include "bsp_dl1b.h"
#include "bsp_flash.h"
#include "bsp_wdt.h"
#include "bsp_sampler.h"

#endif /* __BSP_H */
