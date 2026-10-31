#ifndef _PID_H_
#define _PID_H_

#include <stdint.h>

extern float KP_v;      // 速度环比例系数
extern float KI_v;      // 速度环积分系数
extern float KD_v;      // 速度环微分系数

extern float KP_x;      // 方向环比例系数
extern float K2P_x;     // 方向环非线性二次比例系数
extern float KI_x;      // 方向环积分系数
extern float KD_x;      // 方向环微分系数
extern float K2D_x;     // 方向环微分系数（accel_x 阻尼项）

extern float KP_a;            // 角度环比例系数
extern float KD_a;            // 角度环微分系数
extern float KG_a;            // 角度环陀螺仪阻尼系数

extern float angle_err;       // 角度环当前误差
extern float angle_out;       // 角度环输出
extern float PID_outL;        // 左轮速度PID总输出
extern float PID_outR;        // 右轮速度PID总输出

#define MAX_DIR_OUT                 (1000)
#define MAX_SPD_OUT                 (7500)

#define MOTOR_DEAD_ZONE_L           (500)
#define MOTOR_DEAD_ZONE_R           (500)

int16_t PID_track(void);                        // 循迹PID
void PID_angle(int16_t target_angle);
int16_t PID_L(void);          // 左轮增量式速度PID
int16_t PID_R(void);          // 右轮增量式速度PID
int16_t PID_L_pos(void);      // 左轮位置式速度PID（含快速制动与坡道保持）
int16_t PID_R_pos(void);      // 右轮位置式速度PID（含快速制动与坡道保持）
void PID_ResetSpeed(void);    // 清活动速度 PI 的积分、目标历史与输出
void PID_ResetAll(void);      // 再次起跑前清方向/角度历史并调用 PID_ResetSpeed

#endif
