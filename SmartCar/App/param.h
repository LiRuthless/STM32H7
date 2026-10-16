#ifndef __PARAM_H
#define __PARAM_H

#include <stdint.h>
typedef struct { float kp, k2p, kd; int16_t base_speed; } param_direction_t;
const param_direction_t *Param_GetDirection(uint8_t mode, uint8_t gear);
uint8_t Param_SetDirection(uint8_t mode, uint8_t gear, const param_direction_t *profile);
void Param_ApplyCurrentDirection(void);
int16_t Param_GetFan(uint8_t gear, uint8_t idle);
void Param_SetFan(uint8_t gear, uint8_t idle, int16_t duty);
uint8_t Param_TryFlushPending(void);

/* 速度档位（对应源 eeprom.c 的 speed_low / speed_high 两块存储区） */
#define PARAM_GEAR_LOW      0   /* 慢速档（菜单 ADJUST1） */
#define PARAM_GEAR_HIGH     1   /* 快速档（菜单 ADJUST2） */

uint8_t Param_Load(void);               /* 上电加载；返回1表示参数Flash可用 */
void Param_Save(void);                  /* 收集当前全局参数写回 Flash（菜单翻页/退出时调用） */
void Param_SelectGear(uint8_t gear);    /* 切换活动档位，并把该档参数分发到全局 */
void Param_MarkDirty(void);             /* 串口参数修改后开始3秒静默保存计时 */
void Param_ServiceSave(void);            /* 静默满3秒后保存，停车主循环调用 */
void Param_FlushPending(void);           /* 起跑前立即保存待写参数 */

#endif /* __PARAM_H */
