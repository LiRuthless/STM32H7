// ============================================================
// 文件名: motor.h
// 功能说明: 电机控制与编码器读取模块头文件
// ============================================================

#ifndef _MOTOR_H_
#define _MOTOR_H_

#include <stdint.h>

extern int16_t target_speed_L;  // 左轮目标速度
extern int16_t target_speed_R;  // 右轮目标速度
extern int16_t real_speed_L;    // 左轮实际速度（编码器滤波后）
extern int16_t real_speed_R;    // 右轮实际速度（编码器滤波后）
extern int16_t base_speed;      // 基础目标速度
extern int16_t fan_duty;        // 负压电机PWM占空比

extern int32_t Distance;        // 累计行驶距离（左右轮平均）
extern int32_t distance_L;      // 左轮累计行驶距离
extern int32_t distance_R;      // 右轮累计行驶距离

extern float alpha;             // 低通滤波系数

void speed_control(int16_t pid_out);
void motor_control(void);       // 电机控制
void read_encoder(void);        // 读取编码器

void motor_init(void);          // 电机初始化
void encoder_init(void);        // 编码器初始化
void Motor_ResetRunState(void); // 再次起跑前复位速度、滤波和里程
void Motor_EmergencyStop(void); // 立即清速度 PI 与两路电机 PWM

#endif
