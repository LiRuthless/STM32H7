// ============================================================
// 文件名: motor.c
// 功能说明: 电机控制与编码器读取模块
// 实现直流电机PID速度闭环控制、编码器脉冲读取与低通滤波，
// 以及电机PWM和方向的初始化配置。
// ============================================================

#include "bsp.h"
#include "app_config.h"
#include "motor.h"
#include "pid.h"
#include "filter.h"

int16_t target_speed_L = 0;     // 左轮目标速度
int16_t target_speed_R = 0;     // 右轮目标速度
int16_t real_speed_L = 0;       // 左轮实际速度（编码器滤波后）
int16_t real_speed_R = 0;       // 右轮实际速度（编码器滤波后）

int16_t base_speed = 0;         // 基础目标速度
int16_t fan_duty = 0;           // 负压电机PWM占空比

int32_t distance_L = 0;         // 左轮累计行驶距离
int32_t distance_R = 0;         // 右轮累计行驶距离
int32_t Distance = 0;           // 累计行驶距离（左右轮平均）

float alpha = 0.88f;            // 编码器速度低通滤波系数

LowPassFilter filt_encoder_L;   // 左轮编码器低通滤波器
LowPassFilter filt_encoder_R;   // 右轮编码器低通滤波器


// 函数名: speed_control
// 功能: 根据方向PID输出分配左右轮目标速度（差速控制）
// 说明: 非对称差速：转向内侧轮减速倍数为3/2，外侧轮加速倍数为1
void speed_control(int16_t pid_out)
{
    if( pid_out >= 0 )
    {
        target_speed_L = base_speed - (3 * pid_out) / 2;    // 左轮减速（内侧轮）
        target_speed_R = base_speed + 1 * pid_out;          // 右轮加速（外侧轮）
    }
    else
    {
        target_speed_L = base_speed - 1 * pid_out;          // 左轮加速（外侧轮）
        target_speed_R = base_speed + (3 * pid_out) / 2;    // 右轮减速（内侧轮）
    }
}

// 函数名: motor_control
// 功能: 电机控制函数（速度环输出执行）
// 说明: 位置式速度PID输出，按符号设置DIR方向脚，PWM输出占空比绝对值。
//       方向约定：左轮正转DIR=高电平，右轮正转DIR=低电平（左右轮相反）。
void motor_control(void)
{
    int16_t speed_outL = 0,     // 左轮速度PID输出
            speed_outR = 0;     // 右轮速度PID输出

    speed_outL = PID_L_pos();   // 左轮位置式速度PID（含快速制动与坡道保持）
    speed_outR = PID_R_pos();   // 右轮位置式速度PID（含快速制动与坡道保持）

    // 左轮控制
    if( speed_outL >= 0 )       // 正转
    {
        HAL_GPIO_WritePin(MOTOR_L_DIR_GPIO_Port, MOTOR_L_DIR_Pin, GPIO_PIN_SET);
        BSP_PWM_SetDuty(BSP_PWM_MOTOR_L, (uint32_t)speed_outL);
    }
    else                        // 反转
    {
        HAL_GPIO_WritePin(MOTOR_L_DIR_GPIO_Port, MOTOR_L_DIR_Pin, GPIO_PIN_RESET);
        BSP_PWM_SetDuty(BSP_PWM_MOTOR_L, (uint32_t)(-speed_outL));
    }

    // 右轮控制
    if( speed_outR >= 0 )       // 正转
    {
        HAL_GPIO_WritePin(MOTOR_R_DIR_GPIO_Port, MOTOR_R_DIR_Pin, GPIO_PIN_RESET);
        BSP_PWM_SetDuty(BSP_PWM_MOTOR_R, (uint32_t)speed_outR);
    }
    else                        // 反转
    {
        HAL_GPIO_WritePin(MOTOR_R_DIR_GPIO_Port, MOTOR_R_DIR_Pin, GPIO_PIN_SET);
        BSP_PWM_SetDuty(BSP_PWM_MOTOR_R, (uint32_t)(-speed_outR));
    }
}

// 函数名: read_encoder
// 功能: 读取编码器并计算实际速度（2ms 周期调用，控制环语义不变）
// 说明: 原始计数由 TIM15 采样器以 1ms 读取并累计，此处取 2ms 累计值
//       （BSP_Sampler_Consume 返回累计并清零，符号已按源约定处理），
//       一阶低通滤波后累加距离。
void read_encoder(void)
{
    int16_t encoder_L = 0,      // 左轮编码器计数值
            encoder_R = 0;      // 右轮编码器计数值

    encoder_L = BSP_Sampler_ConsumeEncL();  // 取 2ms 累计计数并清零（符号已处理）
    encoder_R = BSP_Sampler_ConsumeEncR();

    // 低通滤波，得到平滑速度
    real_speed_L = (int16_t)lowpass_update(&filt_encoder_L, (float)encoder_L);
    real_speed_R = (int16_t)lowpass_update(&filt_encoder_R, (float)encoder_R);

    distance_L += real_speed_L;
    distance_R += real_speed_R;
    Distance = (distance_L + distance_R) / 2;
}

// 函数名: motor_init
// 功能: 电机初始化（PWM + DIR默认电平）
void motor_init(void)
{
    BSP_PWM_Init();     // 左/右电机及负压风扇PWM（17kHz，占空比0）

    HAL_GPIO_WritePin(MOTOR_L_DIR_GPIO_Port, MOTOR_L_DIR_Pin, GPIO_PIN_SET);    // 源工程默认高电平
    HAL_GPIO_WritePin(MOTOR_R_DIR_GPIO_Port, MOTOR_R_DIR_Pin, GPIO_PIN_SET);
}

// 函数名: encoder_init
// 功能: 编码器初始化并配置低通滤波器
void encoder_init(void)
{
    BSP_Encoder_Init();
    lowpass_init(&filt_encoder_L, alpha);
    lowpass_init(&filt_encoder_R, alpha);
}
