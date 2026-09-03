#ifndef __PARAM_H
#define __PARAM_H

#include "main.h"

/* 速度档位（对应源 eeprom.c 的 speed_low / speed_high 两块存储区） */
#define PARAM_GEAR_LOW      0   /* 慢速档（菜单 ADJUST1） */
#define PARAM_GEAR_HIGH     1   /* 快速档（菜单 ADJUST2） */

void Param_Load(void);                  /* 上电加载：读 Flash，校验失败用默认并回写 */
void Param_Save(void);                  /* 收集当前全局参数写回 Flash（菜单翻页/退出时调用） */
void Param_SelectGear(uint8_t gear);    /* 切换活动档位，并把该档参数分发到全局 */

#endif /* __PARAM_H */
