#ifndef _PID_H_
#define _PID_H_

#include <stdint.h>

typedef struct {
    float kp;
    float k2p;
    float kd;
    float k2d;
} pid_track_gains_t;

typedef struct {
    float kp;
    float kd;
    float kg;
} pid_angle_gains_t;

typedef enum {
    PID_GAIN_KP_V, PID_GAIN_KI_V, PID_GAIN_KD_V,
    PID_GAIN_KP_X, PID_GAIN_K2P_X, PID_GAIN_KI_X,
    PID_GAIN_KD_X, PID_GAIN_K2D_X,
    PID_GAIN_KP_A, PID_GAIN_KD_A, PID_GAIN_KG_A
} pid_gain_id_t;

typedef struct {
    float KP_v, KI_v, KD_v;
    float KP_x, K2P_x, KI_x, KD_x, K2D_x;
    float KP_a, KD_a, KG_a;
    float angle_err, angle_out;
    float out_l, out_r;
} pid_state_t;

const pid_state_t *PID_GetState(void);
void PID_SetGain(pid_gain_id_t id, float value);
void PID_SetTrackGains(const pid_track_gains_t *gains);
void PID_SetAngleGains(const pid_angle_gains_t *gains);

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
