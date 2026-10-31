/**
 * @file bsp_wdt.h
 * @brief BSP IWDG1；首次起跑初始化，TIM7 或停车主循环喂狗。
 * @details 声明独立看门狗初始化和刷新接口；完整调用限制位于 bsp_wdt.c。
 */
#ifndef SMARTCAR_BSP_BSP_WDT_H
#define SMARTCAR_BSP_BSP_WDT_H

#include "main.h"

/* 独立看门狗 IWDG1（.ioc 未配置，代码内直接初始化）。
 * 由 app_config.h 的 APP_WDG_ENABLE 宏开关；约 6s 超时，运行时由 TIM7 喂狗，停车后由主循环喂狗。 */

/**
 * @brief 按配置首次启动 IWDG1。
 * @note 首次起跑调用；开启时等待 LSI 和寄存器同步，不能重复作为运行复位使用。 完整契约见 bsp_wdt.c::BSP_WDT_Init。
 */
void BSP_WDT_Init(void);
/**
 * @brief 在看门狗启用配置下重装 IWDG1。
 * @note 运行 TIM7 或停车主循环调用；宏关闭时保持空操作。 完整契约见 bsp_wdt.c::BSP_WDT_Feed。
 */
void BSP_WDT_Feed(void);

#endif /* __BSP_WDT_H */
