#ifndef __BSP_KEY_H
#define __BSP_KEY_H

#include "main.h"

/* 启动按键：板载 K1 = PC13（下拉输入，按下为高），主循环轮询 + 消抖。
 * ADC 分压键盘（PA2/PA3）由 App 层 menu 直接经 BSP_ADC_Read 读取，不在此模块。 */

void    BSP_Key_Init(void);
uint8_t BSP_Key_StartPressed(void);   /* 检测到一次有效按下（下降沿/按下沿，带消抖）返回 1，否则 0 */

#endif /* __BSP_KEY_H */
