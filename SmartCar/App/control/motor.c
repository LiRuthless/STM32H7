/**
 * @file motor.c
 * @brief App 双轮速度闭环与里程；TIM6 执行，停车路径可直接清 PWM。
 * @details 把方向与速度控制量合成为左右轮目标，经速度 PI 计算后向 BSP 发布 PWM，并提供紧急清零。
 * @note 移植来源：Sirius20260718/project/code/motor.c，STC AI8051U 省赛快照（2026-07-18）。
 * @note 源算法文件未单列作者及独立许可证；保留原出处，关联 Phase 13/15 规约。
 */
#include "bsp.h"
#include "app_config.h"
#include "control/motor.h"
#include "control/pid.h"
#include "sensing/filter.h"

/* TIM6 更新双轮状态和滤波；停车路径可清目标/PWM，前台参数按原门禁写入。 */
static motor_state_t s_motor = { .fan_duty_idle = 1100, .alpha = 0.88f };
static filter_lowpass_t s_filter_left;
static filter_lowpass_t s_filter_right;

/**
 * @brief 读取模块持有的只读运行状态。
 * @return 非空只读指针，不转移所有权。
 * @note 静态对象寿命为整个程序；控制拍可更新，前台一致性读取须遵守既有停车约定。
 */
const motor_state_t *Motor_GetState(void)
{
    return &s_motor;
}

/**
 * @brief 依次设置左右目标速度。
 * @details 依次写左右轮目标，不读取反馈、不运行PI，也不向BSP发布PWM。
 * @param[in] left 左轮目标，脉冲/2 ms，int16_t。
 * @param[in] right 右轮目标，脉冲/2 ms，int16_t。
 * @note 仅保存，不写 PWM，不新增限幅；控制拍或既有停车路径调用。
 */
void Motor_SetTargets(int16_t left, int16_t right)
{
    s_motor.left.target_speed = left;
    s_motor.right.target_speed = right;
}

/**
 * @brief 保存差速基础速度。
 * @details 只写差速分配所用基础速度；当前左右目标和PWM保留，下一次分配时才使用新值。
 * @param[in] speed 基础速度，脉冲/2 ms，int16_t。
 * @note TIM6 剖面应用或停车参数路径调用；写 s_motor.base_speed，不直接访问 PWM。
 */
void Motor_SetBaseSpeed(int16_t speed)
{
    s_motor.base_speed = speed;
}

/**
 * @brief 保存运行风扇占空比。
 * @details 只更新运行风扇设定；实际PWM仍由运行生命周期和控制拍的既有调用点写出。
 * @param[in] duty 万分比，约定 0..10000，保持 int16_t。
 * @note 仅保存，保持原调用方写 PWM 的时机；不新增范围检查。
 */
void Motor_SetFanDuty(int16_t duty)
{
    s_motor.fan_duty = duty;
}

/**
 * @brief 保存停车风扇占空比。
 * @details 只更新停车风扇设定，不隐式切换运行状态或写风扇PWM。
 * @param[in] duty 万分比，约定 0..10000，保持 int16_t。
 * @note 仅保存，保持原调用方写 PWM 的时机；不新增范围检查。
 */
void Motor_SetFanDutyIdle(int16_t duty)
{
    s_motor.fan_duty_idle = duty;
}

/**
 * @brief 按原顺序清左右累计脉冲及均值。
 * @details 依次清左轮、右轮及平均累计距离，不清反馈、目标或采样器累计计数。
 * @note 控制拍或起跑准备调用，不改硬件计数、反馈或滤波历史。
 */
void Motor_ResetDistance(void)
{
    s_motor.left.distance = 0;
    s_motor.right.distance = 0;
    s_motor.distance = 0;
}

/**
 * @brief 按方向输出分配非对称双轮目标。
 * @details 非负方向量使左轮减速、右轮加速；负值时反向分配。内侧用3/2倍、
 *     外侧用1倍，先乘后整数除法并写int16_t目标，不额外限幅或执行速度PI。
 * @param[in] pid_out 方向输出，正常 -1000..1000。
 * @note 内侧 3*pid_out 后整数除以 2，外侧倍数 1；保持计算和赋值顺序。
 */
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

/**
 * @brief 计算两轮 PI 后依次写方向和 PWM。
 * @details 先用两轮各自目标与反馈计算左右PI，再依次发布左轮、右轮的方向和绝对PWM；
 *     左轮非负输出置高、右轮非负输出置低，负输出取反后转uint32_t，不交换硬件写入顺序。
 * @note TIM6 调用；先左后右计算，再左后右写出；左右正转电平相反，不新增饱和处理。
 */
void Motor_UpdateControl(void)
{
    int16_t speed_out_left = 0;  /* 左轮速度PID输出 */
    int16_t speed_out_right = 0; /* 右轮速度PID输出 */

    speed_out_left = PID_UpdateSpeedLeft(
        s_motor.left.target_speed, s_motor.left.real_speed);
    speed_out_right = PID_UpdateSpeedRight(
        s_motor.right.target_speed, s_motor.right.real_speed);

    /* 左轮控制 */
    if (speed_out_left >= 0) /* 正转 */
    {
        BSP_PWM_SetDirection(BSP_PWM_MOTOR_L, 1);
        BSP_PWM_SetDuty(BSP_PWM_MOTOR_L, (uint32_t)speed_out_left);
    }
    else /* 反转 */
    {
        BSP_PWM_SetDirection(BSP_PWM_MOTOR_L, 0);
        BSP_PWM_SetDuty(BSP_PWM_MOTOR_L, (uint32_t)(-speed_out_left));
    }

    /* 右轮控制 */
    if (speed_out_right >= 0) /* 正转 */
    {
        BSP_PWM_SetDirection(BSP_PWM_MOTOR_R, 0);
        BSP_PWM_SetDuty(BSP_PWM_MOTOR_R, (uint32_t)speed_out_right);
    }
    else /* 反转 */
    {
        BSP_PWM_SetDirection(BSP_PWM_MOTOR_R, 1);
        BSP_PWM_SetDuty(BSP_PWM_MOTOR_R, (uint32_t)(-speed_out_right));
    }
}

/**
 * @brief 读取采样累计量并低通后累计里程。
 * @details 先读清左右采样累计量，再分别经过独立低通并窄化为int16_t反馈；
 *     将各反馈累计到各自距离，最后以两侧累计和的整数平均发布总距离。
 * @note TIM6 每 2 ms 调用；先左右读清，再分别 float→int16 截断累计，最后整数平均。
 */
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

/**
 * @brief 初始化 PWM 并设置原方向电平。
 * @details 先初始化电机和风扇PWM，再依次将左右方向脚置高；不运行PI或恢复轮速目标。
 * @note 上电前台调用，保持两路方向初值和初始化顺序。
 */
void Motor_Init(void)
{
    BSP_PWM_Init(); /* 左/右电机及负压风扇PWM（17kHz，占空比0） */

    BSP_PWM_SetDirection(BSP_PWM_MOTOR_L, 1); /* 源工程默认高电平 */
    BSP_PWM_SetDirection(BSP_PWM_MOTOR_R, 1);
}

/**
 * @brief 初始化编码器与两个低通实例。
 * @details 先初始化编码器，再以当前alpha分别清左右低通历史；不重写基础速度或风扇参数。
 * @note 上电前台调用，低通系数取现有 alpha，不改硬件配置。
 */
void Motor_InitEncoder(void)
{
    BSP_Encoder_Init();
    Filter_InitLowpass(&s_filter_left, s_motor.alpha);
    Filter_InitLowpass(&s_filter_right, s_motor.alpha);
}

/**
 * @brief 清目标、反馈、里程与低通历史。
 * @details 先清两轮目标和反馈，再清两侧及平均距离，最后分别清低通历史；
 *     不重复初始化编码器，不清PI历史，PI由生命周期相邻的专属复位入口处理。
 * @note 再次起跑前调用；不改风扇、alpha 或重复初始化硬件。
 */
void Motor_ResetRunState(void)
{
    Motor_SetTargets(0, 0);
    s_motor.left.real_speed = 0;
    s_motor.right.real_speed = 0;
    Motor_ResetDistance();
    Filter_InitLowpass(&s_filter_left, s_motor.alpha);
    Filter_InitLowpass(&s_filter_right, s_motor.alpha);
}

/**
 * @brief 清目标与活动 PI 并直接清两路 PWM。
 * @details 先清两轮目标并复位活动速度PI，随后左、右PWM置零；
 *     不等待下个控制拍，不改变方向脚、反馈距离或风扇，风扇由安全协调器处理。
 * @note 有界，无阻塞，既有 ISR/前台停车路径可调用；保持左后右顺序。
 */
void Motor_EmergencyStop(void)
{
    Motor_SetTargets(0, 0);
    PID_ResetSpeed();
    BSP_PWM_SetDuty(BSP_PWM_MOTOR_L, 0);
    BSP_PWM_SetDuty(BSP_PWM_MOTOR_R, 0);
}
