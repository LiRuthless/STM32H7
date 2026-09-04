#ifndef __APP_H
#define __APP_H

#include "main.h"

/* 从源 config.c 搬入的全局（bit → uint8_t，命名与源工程一致） */
extern uint8_t  uart[32];           /* 串口发送缓冲区 */
extern uint8_t  dat[32];            /* 串口接收缓冲区 */
extern int16_t  battery;            /* 电池电压ADC原始值 */
extern int16_t  battery_filt;       /* 电池电压滤波值 */
extern int32_t  time;               /* 控制节拍计数（2ms） */
extern uint8_t  key_flag;           /* 启动键已按下 */
extern uint8_t  Start_flag;         /* 启动标志位 */
extern uint8_t  Run_flag;           /* 运行标志位（赛道检测控制） */

/* DL1B 激光测距（mm），无效值 8192；control/roundabout 模块 extern 引用 */
extern uint16_t dl1b_distance_mm;

void  App_Init(void);         /* 系统初始化（对应源 All_init + main 前半段） */
void  App_Loop(void);         /* 主循环（对应源 while 结构） */
void  App_ControlISR(void);   /* TIM6 2ms 控制中断（对应源 pit_track） */
void  App_TaskISR(void);      /* TIM7 5ms 辅助中断（日志刷写/电池/喂狗/状态灯） */
void  App_SampleISR(void);    /* TIM15 1ms 高速采样钩子（数据记录） */

float float_abs(float a);     /* 浮点数绝对值 */

#endif /* __APP_H */
