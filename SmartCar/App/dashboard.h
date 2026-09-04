#ifndef __DASHBOARD_H
#define __DASHBOARD_H

#include "main.h"

/* 板载屏单页仪表盘（与 menu.c 的外接屏完整菜单互斥，
 * 由 app_config.h 的 LCD_TARGET_EXTERNAL 编译期选择） */

void Dashboard_Init(void);      /* 清屏并绘制固定标签 */
void Dashboard_Update(void);    /* 周期调用（内部按 100ms 节流刷新数值） */

#endif /* __DASHBOARD_H */
