/**
 * @file dashboard.h
 * @brief App 板载单页显示；只在停车前台按 100 ms 刷新。
 * @details 声明停车前台的运行仪表初始化与刷新接口；完整实现位于 dashboard.c。
 */
#ifndef SMARTCAR_APP_DASHBOARD_H
#define SMARTCAR_APP_DASHBOARD_H

#include <stdint.h>

/* 板载屏单页仪表盘（与 menu.c 的外接屏完整菜单互斥，
 * 由 app_config.h 的 LCD_TARGET_EXTERNAL 编译期选择） */

/**
 * @brief 清屏并绘制板载单页仪表盘标签。
 * @note BSP LCD 初始化后前台调用，可阻塞；运行链不调用，与外接菜单编译期互斥。 完整契约见 dashboard.c::Dashboard_Init。
 */
void Dashboard_Init(void);

/**
 * @brief 按 100 ms 间隔刷新板载数值区。
 * @note 仅停车前台调用，可阻塞；无符号减法处理 HAL tick 回绕，保持各状态读取位置。 完整契约见 dashboard.c::Dashboard_Update。
 */
void Dashboard_Update(void);

#endif /* SMARTCAR_APP_DASHBOARD_H */
