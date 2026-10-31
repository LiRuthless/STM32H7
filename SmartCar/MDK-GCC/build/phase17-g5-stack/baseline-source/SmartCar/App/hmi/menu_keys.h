/**
 * @file menu_keys.h
 * @brief 按键分类和只读ADC显示值。
 * @details 停车前台扫描的实现和滤波状态位于menu_keys.c。
 */
#ifndef SMARTCAR_APP_HMI_MENU_KEYS_H
#define SMARTCAR_APP_HMI_MENU_KEYS_H
#include <stdint.h>

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

/**
 * @brief 按原两路 ADC 区间判定按键。
 * @note 完整契约见 menu_keys.c::Menu_ScanKey。
 */
uint8_t Menu_ScanKey(void);
/**
 * @brief 读取第1路按键显示滤波值。
 * @note 完整契约见 menu_keys.c::MenuKeys_GetAdc1。
 */
uint16_t MenuKeys_GetAdc1(void);
/**
 * @brief 读取第2路按键显示滤波值。
 * @note 完整契约见 menu_keys.c::MenuKeys_GetAdc2。
 */
uint16_t MenuKeys_GetAdc2(void);

#endif /* SMARTCAR_APP_HMI_MENU_KEYS_H */
