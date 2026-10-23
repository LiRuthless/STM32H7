/**
 * @file bsp_pwm.c
 * @brief PWM 输出：TIM13_CH1 左电机、TIM4_CH3 右电机、TIM14_CH1 风扇。
 * @details 启动三个 PWM 通道，将无符号万分比占空比转换为 CCR；不保存应用控制状态。
 *     电机方向 GPIO 由 App 的 Motor_UpdateControl 处理，本模块不设置方向。
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
 * @details 依次启动 TIM13_CH1、TIM4_CH3、TIM14_CH1，忽略 HAL 启动返回值；
 *     使用定时器既有比较值，不写方向 GPIO，不启动应用控制环。
 * @pre CubeMX 定时器已初始化；启动主循环调用。
 * @note 上电前台调用；不返回 PWM 启动失败状态。
 */
void BSP_PWM_Init(void)
{
    (void)HAL_TIM_PWM_Start(&htim13, TIM_CHANNEL_1); /* 左电机 */
    (void)HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);  /* 右电机 */
    (void)HAL_TIM_PWM_Start(&htim14, TIM_CHANNEL_1); /* 负压风扇 */
}

/**
 * @brief 将万分比占空比换算并写入指定 PWM 通道。
 * @details 拒绝达到通道数的索引，将 duty 截至 10000，再以 duty*(ARR+1)/10000
 *     计算比较值并写入所选通道；不判断正反转。
 * @param[in] ch BSP_PWM_* 通道；索引达到通道数时不写寄存器。
 * @param[in] duty 占空比 0～10000，超出上限时截为 10000。
 * @note 控制 ISR、停车路径及前台可调用；调用方提供有效通道，方向由电机控制层设置。
 */
void BSP_PWM_SetDuty(bsp_pwm_channel_t ch, uint32_t duty)
{
    /* 检查通道并限制占空比，再按所选定时器 ARR 换算 CCR；三个通道均只更新占空比。 */
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
