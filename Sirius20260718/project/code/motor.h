#ifndef _MOTOR_H_
#define _MOTOR_H_

// 电机方向与PWM引脚定义
#define MOTOR_DIR_L                 (IO_P26)            // 左轮方向
#define MOTOR_PWM_L                 (PWMB_CH4_P27)      // 左轮PWM
#define MOTOR_DIR_R                 (IO_P51)            // 右轮方向
#define MOTOR_PWM_R                 (PWMB_CH3_P25)      // 右轮PWM
#define MOTOR_PWM_M                 (PWMA_CH2P_P22)      // 中舵机PWM

// 编码器定义
#define ENCODER_DIR_L               (TIM1_ENCOEDER)     // 左轮编码器
#define ENCODER_DIR_DIR_L           (IO_P24)            // 左轮编码器方向
#define ENCODER_DIR_PULSE_L         (TIM1_ENCOEDER_P35) // 左轮编码器脉冲

#define ENCODER_DIR_R               (TIM0_ENCOEDER)     // 右轮编码器
#define ENCODER_DIR_DIR_R           (IO_P11)            // 右轮编码器方向
#define ENCODER_DIR_PULSE_R         (TIM0_ENCOEDER_P34) // 右轮编码器脉冲


extern int16 target_speed_L;    // 左轮目标速度
extern int16 target_speed_R;    // 右轮目标速度
extern int16 real_speed_L;      // 左轮实际速度
extern int16 real_speed_R;      // 右轮实际速度
extern int16 base_speed;	    // 基础目标速度
extern int16 fan_duty;				// 负压电机PWM占空比

extern int32 Distance;				// 累计行驶距离（左右轮平均）
extern int32 distance_L;			// 左轮累计行驶距离
extern int32 distance_R;			// 右轮累计行驶距离

extern float alpha;             // 低通滤波系数

void speed_control(int16 pid_out);
void motor_control(void);       // 电机控制
void read_encoder(void);        // 读取编码器

void motor_init(void);          // 电机初始化
void encoder_init(void);        // 编码器初始化

#endif
