/**
 * @file menu.h
 * @brief 停车菜单导航与业务编辑。
 * @details 只声明页面、光标只读接口及按键业务处理；不公开可写状态。
 */
#ifndef SMARTCAR_APP_HMI_MENU_H
#define SMARTCAR_APP_HMI_MENU_H
#include <stdint.h>
#include "hmi/menu_keys.h"

/* 页面ID定义 */
#define MENU_PAGE_HOME 0     /* 主页 */
#define MENU_PAGE_ADC_ERR 21 /* ADC实时数据显示 */
#define MENU_PAGE_SPD_DIS 22 /* 速度和距离 */
#define MENU_PAGE_GYRO 23    /* 陀螺仪 */
#define MENU_PAGE_ADJUST1 11 /* 调参页1，慢速 */
#define MENU_PAGE_ADJUST2 12 /* 调参页2，快速 */


/**
 * @brief 处理停车菜单按键沿、保存及显示。
 * @note 完整契约见 menu.c::Menu_Update。
 */
void Menu_Update(void);
/**
 * @brief 执行原菜单导航与调参动作。
 * @note 完整契约见 menu.c::Menu_HandleKey。
 */
void Menu_HandleKey(uint8_t key);
/**
 * @brief 读取当前页面编号。
 * @note 完整契约见 menu.c::Menu_GetPage。
 */
uint8_t Menu_GetPage(void);
/**
 * @brief 读取当前光标编号。
 * @note 完整契约见 menu.c::Menu_GetArrow。
 */
uint8_t Menu_GetArrow(void);
/**
 * @brief 读取当前风扇编辑选择。
 * @note 完整契约见 menu.c::Menu_GetFanEditIdle。
 */
uint8_t Menu_GetFanEditIdle(void);
/**
 * @brief 按镜像合法性恢复页面与光标。
 * @note 完整契约见 menu.c::Menu_RestorePosition。
 */
void Menu_RestorePosition(uint8_t page, uint8_t arrow);

#endif /* SMARTCAR_APP_HMI_MENU_H */
