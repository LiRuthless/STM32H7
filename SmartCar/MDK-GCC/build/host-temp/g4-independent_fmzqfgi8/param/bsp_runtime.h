/**
 * @file bsp_runtime.h
 * @brief 运行时硬件动作接口。
 * @details 不暴露HAL类型；具体时钟、灯和定时器动作见bsp_runtime.c。
 */
#ifndef SMARTCAR_BSP_BSP_RUNTIME_H
#define SMARTCAR_BSP_BSP_RUNTIME_H

/**
 * @brief 恢复SPI4/5内核到PLL3Q时钟。
 * @note 所有MX外设初始化完成后、应用初始化第一步调用。见bsp_runtime.c::BSP_Runtime_ConfigureSpiClock。
 */
void BSP_Runtime_ConfigureSpiClock(void);

/**
 * @brief 依次启动TIM6控制与TIM7辅助中断。
 * @note 前台成功起跑时调用，保持原HAL失败不回滚行为。见bsp_runtime.c::BSP_Runtime_StartTimers。
 */
void BSP_Runtime_StartTimers(void);

/**
 * @brief 依次停止TIM6与TIM7中断。
 * @note 首次停车的前台或ISR调用，忽略原HAL返回值。见bsp_runtime.c::BSP_Runtime_StopTimers。
 */
void BSP_Runtime_StopTimers(void);

/**
 * @brief 点亮PE3蓝色运行灯。
 * @note NPN驱动高电平点亮；起跑或TIM7原位置调用。见bsp_runtime.c::BSP_Runtime_SetRunLed。
 */
void BSP_Runtime_SetRunLed(void);

/**
 * @brief 翻转PE3蓝色状态灯。
 * @note 仅停车前台250ms到期后调用；不保存软件时基。见bsp_runtime.c::BSP_Runtime_ToggleLed。
 */
void BSP_Runtime_ToggleLed(void);

#endif
