#ifndef _PID_H_
#define _PID_H_

extern float KP_v;      // 速度环比例系数
extern float KI_v;      // 速度环积分系数
extern float KD_v;      // 速度环微分系数
                 
extern float KP_x;      // 方向环比例系数
extern float K2P_x;     // 方向环非线性二次比例系数
extern float KI_x;      // 方向环积分系数
extern float KD_x;      // 方向环微分系数
extern float K2D_x;      // 方向环微分系数

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

int16 PID_track(void);                             // 循迹PID
void PID_angle(int16 target_angle);
int16 PID_L(void);          // 左轮增量式速度PID
int16 PID_R(void);          // 右轮增量式速度PID
int16 PID_L_pos(void);      // 左轮位置式速度PID（含快速制动与坡道保持）
int16 PID_R_pos(void);      // 右轮位置式速度PID（含快速制动与坡道保持）

#endif
