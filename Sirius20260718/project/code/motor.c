// ============================================================
// 文件名: motor.c
// 功能说明: 电机控制与编码器读取模块
// 实现直流电机PID速度闭环控制、编码器脉冲读取与低通滤波，
// 以及电机PWM和方向的初始化配置。
// ============================================================

#include "config.h"

int16 target_speed_L = 0;       // 左轮目标速度
int16 target_speed_R = 0;       // 右轮目标速度
int16 real_speed_L = 0;         // 左轮实际速度（编码器滤波后）
int16 real_speed_R = 0;         // 右轮实际速度（编码器滤波后）

int16 base_speed = 0;      		// 基础目标速度
int16 fan_duty = 0;				// 负压电机PWM占空比

int32 distance_L = 0;				// 左轮累计行驶距离
int32 distance_R = 0;				// 右轮累计行驶距离
int32 Distance = 0;					// 累计行驶距离（左右轮平均）

float alpha = 0.88 ;              // 编码器速度低通滤波系数

LowPassFilter filt_encoder_L;   // 左轮编码器低通滤波器
LowPassFilter filt_encoder_R;   // 右轮编码器低通滤波器



// 函数名: speed_control
// 功能: 根据方向PID输出分配左右轮目标速度（差速控制）
// 参数: pid_out - 方向PID输出（正值偏右需左转，负值偏左需右转）
// 说明: 采用非对称差速策略：转向内侧轮减速倍数为2，外侧轮加速倍数为1，
//       可在过弯时降低内侧轮速度、防止冲出赛道。
void speed_control(int16 pid_out)
{
	if( pid_out >= 0 )
	{
		target_speed_L = base_speed - (3 * pid_out)/2;    // 左轮减速（内侧轮）
		target_speed_R = base_speed + 1 * pid_out;    // 右轮加速（外侧轮）
	}
	else
	{
		target_speed_L = base_speed - 1 * pid_out;    // 左轮加速（外侧轮）
		target_speed_R = base_speed + (3 * pid_out)/2;    // 右轮减速（内侧轮）
	}
}

// 函数名: motor_control
// 功能: 电机控制函数（速度环输出执行）
// 说明: 调用左右轮PID控制器计算输出，通过DIR引脚控制方向，
//       PWM引脚控制占空比，实现电机正反转调速。
void motor_control(void)
{
    int16 speed_outL = 0,    // 左轮速度PID输出
          speed_outR = 0;    // 右轮速度PID输出
           
    // 根据宏定义选择使用增量式PID或位置式PID
    speed_outL = PID_L_pos();   // 左轮位置式速度PID计算（含快速制动与坡道保持）
    speed_outR = PID_R_pos();   // 右轮位置式速度PID计算（含快速制动与坡道保持）
    
    // 串口打印调试数据：目标速度、实际速度、PID输出
//    sprintf(uart, "%d,%d,%d\r\n", target_speed_L, real_speed_L, speed_outL);
//    uart_write_buffer(UART_1, uart, strlen(uart));
    
            
    // 左轮控制
    if(speed_outL >= 0)             // 正转（速度为正）
    {
        gpio_set_level(MOTOR_DIR_L, GPIO_HIGH);      // DIR置高，左轮正转
        pwm_set_duty(MOTOR_PWM_L, speed_outL);      // 设置正占空比
    }
    else                            // 反转（速度为负）
    {
        gpio_set_level(MOTOR_DIR_L, GPIO_LOW);     // DIR置低，左轮反转
        pwm_set_duty(MOTOR_PWM_L, -speed_outL);     // 设置正占空比（取绝对值）
    }
    
    // 右轮控制
    if(speed_outR >= 0)             // 正转（速度为正）
    {
        gpio_set_level(MOTOR_DIR_R, GPIO_LOW);      // DIR置低，右轮正转（左右轮DIR逻辑相反）
        pwm_set_duty(MOTOR_PWM_R, speed_outR);      // 设置正占空比
    }
    else                            // 反转（速度为负）
    {
        gpio_set_level(MOTOR_DIR_R, GPIO_HIGH);     // DIR置高，右轮反转（左右轮DIR逻辑相反）
        pwm_set_duty(MOTOR_PWM_R, -speed_outR);     // 设置正占空比（取绝对值）
    }

}

// 函数名: read_encoder
// 功能: 读取编码器并计算实际速度
// 说明: 读取左右编码器脉冲数，经过低通滤波得到平滑速度值，
//       然后清零编码器计数，为下一次采样做准备。
void read_encoder(void)
{
    int16 encoder_L = 0,     // 左轮编码器累计值
          encoder_R = 0;     // 右轮编码器累计值		 
    
    encoder_L = -encoder_get_count(ENCODER_DIR_L);      // 读取左轮编码器计数值（取反）
    encoder_R =  encoder_get_count(ENCODER_DIR_R);      // 读取右轮编码器计数值
	
	encoder_clear_count(ENCODER_DIR_L);     // 清除左轮编码器计数值
    encoder_clear_count(ENCODER_DIR_R);     // 清除右轮编码器计数值

//    if (gpio_get_level(ENCODER_DIR_DIR_L) == 1)       // 方向检测（已禁用）
//    {
//        encoder_L = -encoder_L;
//    }
//    if (gpio_get_level(ENCODER_DIR_DIR_R) == 1)
//    {
//        encoder_R = -encoder_R;
//    }

    // 低通滤波，得到平滑速度
    real_speed_L = (int16)lowpass_update(&filt_encoder_L, (float)encoder_L);  // 左轮速度低通滤波
    real_speed_R = (int16)lowpass_update(&filt_encoder_R, (float)encoder_R);  // 右轮速度低通滤波
    
    distance_L += real_speed_L;     
    distance_R += real_speed_R;            
    Distance = (distance_L + distance_R) / 2;
}

// 函数名: motor_init
// 功能: 电机初始化函数
// 说明: 初始化左右轮DIR方向GPIO和PWM输出，以及中舵机PWM。
//       PWM频率统一设置为17KHz。
void motor_init(void)
{
    gpio_init(MOTOR_DIR_L, GPO, GPIO_HIGH, GPO_PUSH_PULL);  // 左轮DIR GPIO初始化
    pwm_init(MOTOR_PWM_L, 17000, 0);                        // 左轮PWM初始化：17KHz、占空比0
    gpio_init(MOTOR_DIR_R, GPO, GPIO_HIGH, GPO_PUSH_PULL);  // 右轮DIR GPIO初始化
    pwm_init(MOTOR_PWM_R, 17000, 0);                        // 右轮PWM初始化：17KHz、占空比0
	
//	pwm_init(MOTOR_PWM_M, 17000, 0);                        // 负压电机PWM初始化：17KHz、占空比0
//	gpio_init(MOTOR_PWM_M, GPO, GPIO_LOW, GPO_PUSH_PULL);
	pwm_init(MOTOR_PWM_M, 100, 0);                          // 负压电机PWM初始化：17KHz、占空比0
	
}

// 函数名: encoder_init
// 功能: 编码器初始化函数
// 说明: 初始化左右轮带方向编码器接口，并配置低通滤波器。
void encoder_init(void)
{
    encoder_dir_init(ENCODER_DIR_L, ENCODER_DIR_DIR_L, ENCODER_DIR_PULSE_L);      
    encoder_dir_init(ENCODER_DIR_R, ENCODER_DIR_DIR_R, ENCODER_DIR_PULSE_R);   
	encoder_clear_count(ENCODER_DIR_L);     // 清除左轮编码器计数值
    encoder_clear_count(ENCODER_DIR_R);     // 清除右轮编码器计数值
    lowpass_init(&filt_encoder_L, alpha);   // 初始化左轮速度低通滤波
    lowpass_init(&filt_encoder_R, alpha);   // 初始化右轮速度低通滤波
}
