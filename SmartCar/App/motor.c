/**
 * @file motor.c
 * @brief App 双轮速度闭环与里程；TIM6 执行，停车路径可直接清 PWM。
 * @note 移植来源：Sirius20260718/project/code/motor.c，STC AI8051U 省赛快照（2026-07-18）。
 * @note 源算法文件未单列作者及独立许可证；保留原出处，关联 Phase 13/15 规约。
 */
#include "bsp.h"
#include "app_config.h"
#include "motor.h"
#include "pid.h"
#include "filter.h"

/* TIM6 更新双轮状态和滤波；停车路径可清目标/PWM，前台参数按原门禁写入。 */
static motor_state_t s_motor = { .fan_duty_idle = 1100, .alpha = 0.88f };
static filter_lowpass_t s_filter_left;
static filter_lowpass_t s_filter_right;

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

void Motor_DistributeSpeed(int16_t pid_out)
{
    if (pid_out >= 0)
    {
        s_motor.left.target_speed = s_motor.base_speed - (3 * pid_out) / 2; /* 左轮减速（内侧轮） */
        s_motor.right.target_speed = s_motor.base_speed + 1 * pid_out;      /* 右轮加速（外侧轮） */
    }
    else
    {
        s_motor.left.target_speed = s_motor.base_speed - 1 * pid_out; /* 左轮加速（外侧轮） */
        s_motor.right.target_speed
            = s_motor.base_speed + (3 * pid_out) / 2; /* 右轮减速（内侧轮） */
    }
}

void Motor_UpdateControl(void)
{
    int16_t speed_out_left = 0;  /* 左轮速度PID输出 */
    int16_t speed_out_right = 0; /* 右轮速度PID输出 */

    speed_out_left = PID_UpdateSpeedLeft();   /* 左轮位置式速度PID（含快速制动与坡道保持） */
    speed_out_right = PID_UpdateSpeedRight(); /* 右轮位置式速度PID（含快速制动与坡道保持） */

    /* 左轮控制 */
    if (speed_out_left >= 0) /* 正转 */
    {
        HAL_GPIO_WritePin(MOTOR_L_DIR_GPIO_Port, MOTOR_L_DIR_Pin, GPIO_PIN_SET);
        BSP_PWM_SetDuty(BSP_PWM_MOTOR_L, (uint32_t)speed_out_left);
    }
    else /* 反转 */
    {
        HAL_GPIO_WritePin(MOTOR_L_DIR_GPIO_Port, MOTOR_L_DIR_Pin, GPIO_PIN_RESET);
        BSP_PWM_SetDuty(BSP_PWM_MOTOR_L, (uint32_t)(-speed_out_left));
    }

    /* 右轮控制 */
    if (speed_out_right >= 0) /* 正转 */
    {
        HAL_GPIO_WritePin(MOTOR_R_DIR_GPIO_Port, MOTOR_R_DIR_Pin, GPIO_PIN_RESET);
        BSP_PWM_SetDuty(BSP_PWM_MOTOR_R, (uint32_t)speed_out_right);
    }
    else /* 反转 */
    {
        HAL_GPIO_WritePin(MOTOR_R_DIR_GPIO_Port, MOTOR_R_DIR_Pin, GPIO_PIN_SET);
        BSP_PWM_SetDuty(BSP_PWM_MOTOR_R, (uint32_t)(-speed_out_right));
    }
}

void Motor_ReadEncoder(void)
{
    int16_t encoder_left = 0;  /* 左轮编码器计数值 */
    int16_t encoder_right = 0; /* 右轮编码器计数值 */

    encoder_left = BSP_Sampler_ConsumeEncL(); /* 取 2ms 累计计数并清零（符号已处理） */
    encoder_right = BSP_Sampler_ConsumeEncR();

    /* 低通滤波，得到平滑速度 */
    s_motor.left.real_speed = (int16_t)Filter_UpdateLowpass(&s_filter_left, (float)encoder_left);
    s_motor.right.real_speed = (int16_t)Filter_UpdateLowpass(&s_filter_right, (float)encoder_right);

    s_motor.left.distance += s_motor.left.real_speed;
    s_motor.right.distance += s_motor.right.real_speed;
    s_motor.distance = (s_motor.left.distance + s_motor.right.distance) / 2;
}

void Motor_Init(void)
{
    BSP_PWM_Init(); /* 左/右电机及负压风扇PWM（17kHz，占空比0） */

    HAL_GPIO_WritePin(MOTOR_L_DIR_GPIO_Port, MOTOR_L_DIR_Pin, GPIO_PIN_SET); /* 源工程默认高电平 */
    HAL_GPIO_WritePin(MOTOR_R_DIR_GPIO_Port, MOTOR_R_DIR_Pin, GPIO_PIN_SET);
}

void Motor_InitEncoder(void)
{
    BSP_Encoder_Init();
    Filter_InitLowpass(&s_filter_left, s_motor.alpha);
    Filter_InitLowpass(&s_filter_right, s_motor.alpha);
}

void Motor_ResetRunState(void)
{
    Motor_SetTargets(0, 0);
    s_motor.left.real_speed = 0;
    s_motor.right.real_speed = 0;
    Motor_ResetDistance();
    Filter_InitLowpass(&s_filter_left, s_motor.alpha);
    Filter_InitLowpass(&s_filter_right, s_motor.alpha);
}

void Motor_EmergencyStop(void)
{
    Motor_SetTargets(0, 0);
    PID_ResetSpeed();
    BSP_PWM_SetDuty(BSP_PWM_MOTOR_L, 0);
    BSP_PWM_SetDuty(BSP_PWM_MOTOR_R, 0);
}
