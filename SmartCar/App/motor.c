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

static motor_state_t s_motor = {
    .fan_duty_idle = 1100,
    .alpha = 0.88f
};
static LowPassFilter s_filter_left;
static LowPassFilter s_filter_right;

const motor_state_t *Motor_GetState(void)
{
    return &s_motor;
}

void Motor_SetTargets(int16_t left, int16_t right)
{
    s_motor.left.target_speed = left;
    s_motor.right.target_speed = right;
}

void Motor_SetBaseSpeed(int16_t speed)
{
    s_motor.base_speed = speed;
}

void Motor_SetFanDuty(int16_t duty)
{
    s_motor.fan_duty = duty;
}

void Motor_SetFanDutyIdle(int16_t duty)
{
    s_motor.fan_duty_idle = duty;
}

void Motor_ResetDistance(void)
{
    s_motor.left.distance = 0;
    s_motor.right.distance = 0;
    s_motor.distance = 0;
}


// 函数名: speed_control
// 功能: 根据方向PID输出分配左右轮目标速度（差速控制）
// 说明: 非对称差速：转向内侧轮减速倍数为3/2，外侧轮加速倍数为1
void speed_control(int16_t pid_out)
{
    if( pid_out >= 0 )
    {
        s_motor.left.target_speed = s_motor.base_speed - (3 * pid_out) / 2;    // 左轮减速（内侧轮）
        s_motor.right.target_speed = s_motor.base_speed + 1 * pid_out;          // 右轮加速（外侧轮）
    }
    else
    {
        s_motor.left.target_speed = s_motor.base_speed - 1 * pid_out;          // 左轮加速（外侧轮）
        s_motor.right.target_speed = s_motor.base_speed + (3 * pid_out) / 2;    // 右轮减速（内侧轮）
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
    s_motor.left.real_speed = (int16_t)lowpass_update(&s_filter_left, (float)encoder_L);
    s_motor.right.real_speed = (int16_t)lowpass_update(&s_filter_right, (float)encoder_R);

    s_motor.left.distance += s_motor.left.real_speed;
    s_motor.right.distance += s_motor.right.real_speed;
    s_motor.distance = (s_motor.left.distance + s_motor.right.distance) / 2;
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
    lowpass_init(&s_filter_left, s_motor.alpha);
    lowpass_init(&s_filter_right, s_motor.alpha);
}

// 函数名: Motor_ResetRunState
// 功能: 再次起跑前清除速度、滤波历史和里程；不重复启动编码器硬件
void Motor_ResetRunState(void)
{
    Motor_SetTargets(0, 0);
    s_motor.left.real_speed = 0;
    s_motor.right.real_speed = 0;
    Motor_ResetDistance();
    lowpass_init(&s_filter_left, s_motor.alpha);
    lowpass_init(&s_filter_right, s_motor.alpha);
}

// 函数名: Motor_EmergencyStop
// 功能: 有界安全停车；不经过速度 PI，直接切断左右电机 PWM
void Motor_EmergencyStop(void)
{
    Motor_SetTargets(0, 0);
    PID_ResetSpeed();
    BSP_PWM_SetDuty(BSP_PWM_MOTOR_L, 0);
    BSP_PWM_SetDuty(BSP_PWM_MOTOR_R, 0);
}
