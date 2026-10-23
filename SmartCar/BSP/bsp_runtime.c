/**
 * @file bsp_runtime.c
 * @brief 运行定时器、状态灯和SPI时钟的板级适配。
 * @details 在原时点执行HAL动作；TIM6/7回调分派应用入口，TIM15仍由采样器处理。
 */
/**
 * @note 移植来源：Sirius20260718/project/user/main.c、user/isr.c、code/config.c。
 * @note STC AI8051U 省赛快照（2026-07-18）；原算法未单列作者或独立许可证。
 * @note 保留源工程出处，关联 Phase 13/14/15 规约。
 */
#include "bsp_runtime.h"
#include "main.h"
#include "tim.h"
#include "app.h"

/**
 * @brief 恢复SPI4/5内核到PLL3Q时钟。
 * @note 所有MX外设初始化完成后、应用初始化第一步调用。
 */
void BSP_Runtime_ConfigureSpiClock(void)
{
    MODIFY_REG(RCC->D2CCIP1R, RCC_D2CCIP1R_SPI45SEL, RCC_SPI45CLKSOURCE_PLL3);
}

/**
 * @brief 依次启动TIM6控制与TIM7辅助中断。
 * @details 先启动htim6再启动htim7，两次HAL返回值均忽略，不加入回滚或重试。
 * @note 前台成功起跑时调用，保持原HAL失败不回滚行为。
 */
void BSP_Runtime_StartTimers(void)
{
    (void)HAL_TIM_Base_Start_IT(&htim6);
    (void)HAL_TIM_Base_Start_IT(&htim7);
}

/**
 * @brief 依次停止TIM6与TIM7中断。
 * @details 先停止htim6再停止htim7，两次HAL返回值均忽略，不改变调用者中断屏蔽状态。
 * @note 首次停车的前台或ISR调用，忽略原HAL返回值。
 */
void BSP_Runtime_StopTimers(void)
{
    (void)HAL_TIM_Base_Stop_IT(&htim6);
    (void)HAL_TIM_Base_Stop_IT(&htim7);
}

/**
 * @brief 点亮PE3蓝色运行灯。
 * @note NPN驱动高电平点亮；起跑或TIM7原位置调用。
 */
void BSP_Runtime_SetRunLed(void)
{
    HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_SET);
}

/**
 * @brief 翻转PE3蓝色状态灯。
 * @note 仅停车前台250ms到期后调用；不保存软件时基。
 */
void BSP_Runtime_ToggleLed(void)
{
    HAL_GPIO_TogglePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin);
}

/**
 * @brief 保留 HAL 强符号回调分派 TIM6/TIM7。
 * @details 读取句柄实例，TIM6调用2ms控制入口、TIM7调用5ms辅助入口；其他实例无动作。
 * @param[in] htim HAL 提供的非空定时器句柄；其他实例不操作。
 * @note 固定名称，不新增声明或回调注册；周期仍 2/5 ms。
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    /* 按定时器实例把更新事件分派给 TIM6 控制拍或 TIM7 辅助拍；其他实例不处理，也不改变应用标志。 */
    if (htim->Instance == TIM6)
    {
        App_ControlISR();
    }
    else if (htim->Instance == TIM7)
    {
        App_TaskISR();
    }
}
