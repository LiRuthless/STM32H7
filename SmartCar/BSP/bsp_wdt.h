#ifndef __BSP_WDT_H
#define __BSP_WDT_H

#include "main.h"

/* 独立看门狗 IWDG1（.ioc 未配置，代码内直接初始化）。
 * 由 app_config.h 的 WDG_ENABLE 宏开关；约 1s 超时，TIM7 辅助中断中喂狗。 */

void BSP_WDT_Init(void);
void BSP_WDT_Feed(void);

#endif /* __BSP_WDT_H */
