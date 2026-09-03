#ifndef __MENU_H
#define __MENU_H

#include "main.h"

/* 按键值枚举（与源 menu.h 一致） */
#define OK      1       /* 确定键 */
#define UP      2       /* 上键 */
#define DOWN    3       /* 下键 */
#define BACK    4       /* 返回键 */
#define LEFT    5       /* 左键（参数减） */
#define RIGHT   6       /* 右键（参数加） */
#define RST     7       /* 复位键（清屏刷新，保持当前页面） */
#define ADJUST1 8       /* 调参1（慢速档） */
#define ADJUST2 9       /* 调参2（快速档） */
#define FOUR    10      /* 预留 */

/* 页面ID定义 */
#define PAGE_HOME       0   /* 主页 */
#define PAGE_ADC_ERR    21  /* ADC实时数据显示 */
#define PAGE_SPD_DIS    22  /* 速度和距离 */
#define PAGE_GYRO       23  /* 陀螺仪 */
#define PAGE_ADJUST1    11  /* 调参页1，慢速 */
#define PAGE_ADJUST2    12  /* 调参页2，快速 */

extern uint8_t  page;       /* 当前页面编号 */
extern uint8_t  arrow;      /* 当前光标位置（从1开始计数） */
extern uint16_t key_adc1;   /* 按键ADC通道1采样值（方向+确定） */
extern uint16_t key_adc2;   /* 按键ADC通道2采样值（复位+返回） */

uint8_t key_scan(void);     /* ADC按键扫描 */
void    menu(void);         /* 主菜单显示与处理（主循环周期调用，仅停车时运行） */

#endif /* __MENU_H */
