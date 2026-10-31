/**
 * @file menu.h
 * @brief App 外接屏菜单与 ADC 按键调参；只在停车前台调用。
 * @details 声明停车菜单初始化、按键处理和刷新接口；完整状态机与副作用位于 menu.c。
 * @note 移植来源：Sirius20260718/project/code/menu.c。
 * @note STC AI8051U 省赛快照（2026-07-18）；原算法未单列作者或独立许可证。
 * @note 保留源工程出处，关联 Phase 13/14/15 规约。
 */
#ifndef SMARTCAR_APP_MENU_H
#define SMARTCAR_APP_MENU_H

#include "main.h"

/* 按键值枚举（与源 menu.h 一致） */
#define MENU_KEY_OK 1      /* 确定键 */
#define MENU_KEY_UP 2      /* 上键 */
#define MENU_KEY_DOWN 3    /* 下键 */
#define MENU_KEY_BACK 4    /* 返回键 */
#define MENU_KEY_LEFT 5    /* 左键（参数减） */
#define MENU_KEY_RIGHT 6   /* 右键（参数加） */
#define MENU_KEY_RST 7     /* 复位键（清屏刷新，保持当前页面） */
#define MENU_KEY_ADJUST1 8 /* 调参1（慢速档） */
#define MENU_KEY_ADJUST2 9 /* 调参2（快速档） */
#define MENU_KEY_FOUR 10   /* 预留 */

/* 页面ID定义 */
#define MENU_PAGE_HOME 0     /* 主页 */
#define MENU_PAGE_ADC_ERR 21 /* ADC实时数据显示 */
#define MENU_PAGE_SPD_DIS 22 /* 速度和距离 */
#define MENU_PAGE_GYRO 23    /* 陀螺仪 */
#define MENU_PAGE_ADJUST1 11 /* 调参页1，慢速 */
#define MENU_PAGE_ADJUST2 12 /* 调参页2，快速 */


/* 当前页面编号 */
extern uint8_t g_menu_page;
/* 当前光标位置（从1开始计数） */
extern uint8_t g_menu_arrow;
/* 按键ADC通道1采样值（方向+确定） */
extern uint16_t g_menu_key_adc1;
/* 按键ADC通道2采样值（复位+返回） */
extern uint16_t g_menu_key_adc2;

/**
 * @brief 按原两路 ADC 区间判定按键。
 * @note 仅停车前台调用；每路三次内存均值，先判键再更新显示滤波，不改判定阈值。 完整契约见 menu.c::Menu_ScanKey。
 */
uint8_t Menu_ScanKey(void);

/**
 * @brief 处理停车菜单按键沿、保存及显示。
 * @note 仅外接屏停车前台调用，可阻塞；保留无键沿、复位键早退、页面刷新与松键保存顺序。 完整契约见 menu.c::Menu_Update。
 */
void Menu_Update(void);

#endif /* SMARTCAR_APP_MENU_H */
