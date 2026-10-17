#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "zf_common_headfile.h"
#include "pid.h"
#include "motor.h"
#include "menu.h"
#include "adc.h"
#include "filter.h"
#include "control.h"
#include "eeprom.h"
#include "gyroscope.h"
#include "roundabout.h"
#include "element.h"


#define PIT_TR      (TIM3_PIT)      // 循迹控制中断（5ms）
#define PIT_SP      (TIM4_PIT)      // 速度环中断（2ms）


extern int32 time;
extern uint8 uart[32];      // 串口数据发送缓冲区
extern uint8 dat[32];
extern bit key_flag;
extern bit Start_flag;           // 启动标志位
extern bit Run_flag;             // 运行标志位
extern int16 battery;
extern int16 battery_filt;

void All_init(void);        // 系统初始化
void uart_adjust(void);		// 串口无线调参解析
void key_start(void);		// 启动按键检测与电池电压监测
uint8 battery_voltage_get(void);

void pit_speed (void);      // 速度环中断服务函数
void pit_track (void);      // 循迹控制中断服务函数

float float_abs(float a);   // 浮点数绝对值

#endif
