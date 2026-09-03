#ifndef __BSP_H
#define __BSP_H

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
#include "bsp_pwm.h"
#include "bsp_encoder.h"
#include "bsp_key.h"
#include "bsp_lcd.h"
#include "bsp_imu660rb.h"
#include "bsp_dl1b.h"
#include "bsp_flash.h"
#include "bsp_wdt.h"

#endif /* __BSP_H */
