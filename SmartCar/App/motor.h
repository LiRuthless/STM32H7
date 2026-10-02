// ============================================================
// 文件名: motor.h
// 功能说明: 电机控制与编码器读取模块头文件
// ============================================================

#ifndef _MOTOR_H_
#define _MOTOR_H_

#include <stdint.h>

typedef struct {
    int16_t target_speed;
    int16_t real_speed;
    int32_t distance;
} motor_wheel_state_t;

typedef struct {
    motor_wheel_state_t left;
    motor_wheel_state_t right;
    int32_t distance;
    int16_t base_speed;
    int16_t fan_duty;
    int16_t fan_duty_idle;
    float alpha;
} motor_state_t;

const motor_state_t *Motor_GetState(void);
void Motor_SetTargets(int16_t left, int16_t right);
void Motor_SetBaseSpeed(int16_t speed);
void Motor_SetFanDuty(int16_t duty);
void Motor_SetFanDutyIdle(int16_t duty);
void Motor_ResetDistance(void);

void speed_control(int16_t pid_out);
void motor_control(void);       // 电机控制
void read_encoder(void);        // 读取编码器

void motor_init(void);          // 电机初始化
void encoder_init(void);        // 编码器初始化
void Motor_ResetRunState(void); // 再次起跑前复位速度、滤波和里程
void Motor_EmergencyStop(void); // 立即清速度 PI 与两路电机 PWM

#endif
