// ============================================================
// 文件名: control.h
// 功能说明: 控制策略模块头文件
// 声明循迹测试、速度测试、环岛测试及ADC/陀螺仪测试等函数。
// ============================================================

#ifndef _CONTROL_H_
#define _CONTROL_H_

#define KERNEL_TRACKING		0
#define KERNEL_ISLAND_L		1
#define KERNEL_ISLAND_R		2
#define KERNEL_TEETERBOARD	3
#define KERNEL_CROSSROADS	4
#define KERNEL_CASK			5
#define KERNEL_REISLAND     6


extern int16 track_out;     // 方向控制输出
extern uint8 kernel_state;
extern uint8 cask_flag;


void whole_test(void);
void roundabout_test(void); // 环岛状态机测试与控制（基于角度环）
void track_test(void);      // 电磁循迹主控制函数
void speed_test2(void);     // 速度环调参测试（固定周期切换速度）
void speed_test(void);      // 速度环与循迹综合控制
void gyro_test(void);       // 陀螺仪测试（预留）
void adc_test(void);        // ADC采集与串口输出测试

#endif
