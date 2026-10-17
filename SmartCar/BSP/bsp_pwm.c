/**
 * @file bsp_pwm.c
 * @brief PWM 输出：TIM13_CH1 左电机、TIM4_CH3 右电机、TIM14_CH1 风扇。
 * @details 把万分比占空比和方向转换为定时器比较值与方向 GPIO，并提供风扇输出。
 */
#include "bsp_pwm.h"
#include "tim.h"
static TIM_HandleTypeDef *const s_pwm_tim[BSP_PWM_CH_NUM] =
{
    &htim13, /* BSP_PWM_MOTOR_L */
    &htim4,  /* BSP_PWM_MOTOR_R */
    &htim14, /* BSP_PWM_FAN */
};

static const uint32_t s_pwm_chan[BSP_PWM_CH_NUM] =
{
    TIM_CHANNEL_1, /* BSP_PWM_MOTOR_L */
    TIM_CHANNEL_3, /* BSP_PWM_MOTOR_R */
    TIM_CHANNEL_1, /* BSP_PWM_FAN */
};
/**
 * @brief 启动左右电机和风扇的硬件 PWM 通道。
 * @details 启动左右电机和风扇 PWM 通道，并把方向与比较值置为既定初始状态；不启动应用控制环。
 * @pre CubeMX 定时器已初始化；启动主循环调用。
 * @note 上电前台调用；写定时器和方向 GPIO，不改变应用运行标志。
 */
void BSP_PWM_Init(void)
{
    (void)HAL_TIM_PWM_Start(&htim13, TIM_CHANNEL_1); /* 左电机 */
    (void)HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);  /* 右电机 */
    (void)HAL_TIM_PWM_Start(&htim14, TIM_CHANNEL_1); /* 负压风扇 */
}

/**
 * @brief 将万分比占空比换算并写入指定 PWM 通道。
 * @details 按现有调用顺序使用 __HAL_TIM_GET_AUTORELOAD、__HAL_TIM_SET_COMPARE 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @param[in] ch BSP_PWM_* 通道；索引达到通道数时不写寄存器。
 * @param[in] duty 占空比 0～10000，超出上限时截为 10000。
 * @note 控制 ISR、停车路径及前台可调用；乘除与寄存器写入顺序保持。
 */
void BSP_PWM_SetDuty(bsp_pwm_channel_t ch, uint32_t duty)
{
    /* 先把万分比限制到 0..10000，再按通道查表换算 CCR；电机通道同时依据符号设置方向，风扇只写占空比。 */
    uint32_t arr;
    uint32_t ccr;

    if (ch >= BSP_PWM_CH_NUM)
    {
        return;
    }
    if (duty > BSP_PWM_DUTY_MAX)
    {
        duty = BSP_PWM_DUTY_MAX;
    }
    arr = __HAL_TIM_GET_AUTORELOAD(s_pwm_tim[ch]);
    ccr = (duty * (arr + 1u)) / BSP_PWM_DUTY_MAX;
    __HAL_TIM_SET_COMPARE(s_pwm_tim[ch], s_pwm_chan[ch], ccr);
}
