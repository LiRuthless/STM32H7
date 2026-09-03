/**
  ******************************************************************************
  * @file    bsp_wdt.c
  * @brief   独立看门狗 IWDG1：约 6s 超时（LSI 32kHz / 64 / (2999+1)）。
  *          由 app_config.h 的 WDG_ENABLE 宏开关，关闭时为空实现。
  * @note    本工程 HAL 配置（stm32h7xx_hal_conf.h）未使能 HAL_IWDG_MODULE_ENABLED，
  *          且 Drivers 中不含 stm32h7xx_hal_iwdg.c/h，因此此处直接操作寄存器实现。
  *          超时取 6s 而非 1s：菜单保存参数时 Flash 整扇区擦除关中断约 2s。
  ******************************************************************************
  */

/* 包含头文件 ------------------------------------------------------------------*/
#include "bsp_wdt.h"
#include "app_config.h"

#if WDG_ENABLE

/* 私有定义 -----------------------------------------------------------*/
#define IWDG_KEY_RELOAD     0x0000AAAAu     /* 喂狗 */
#define IWDG_KEY_ENABLE     0x0000CCCCu     /* 启动（一旦启动不可关闭） */
#define IWDG_KEY_ACCESS     0x00005555u     /* 解锁 PR/RLR/WINR 写访问 */
#define IWDG_PRESCALER_64   4u              /* PR=4 → 64 分频 */
#define IWDG_RELOAD         2999u           /* 32000/64=500Hz，3000 计数 ≈ 6s。
                                             * 不能取 1s：菜单保存参数时 Flash 整扇区擦除
                                             * 关中断可达约 2s，超时过短会误复位 */
#define IWDG_WINDOW_FULL    0x00000FFFu     /* 窗口禁用（=满量程） */

/* 私有变量 ---------------------------------------------------------*/
static IWDG_TypeDef *const s_hiwdg = IWDG1; /* 句柄静态定义在本文件 */

#endif /* WDG_ENABLE */

/* 导出函数定义 -------------------------------------------*/

/**
  * @brief  初始化并启动 IWDG1（启动后无法关闭）
  * @retval 无
  */
void BSP_WDT_Init(void)
{
#if WDG_ENABLE
  /* IWDG 时钟源为 LSI，需先使能并等待就绪 */
  __HAL_RCC_LSI_ENABLE();
  while (__HAL_RCC_GET_FLAG(RCC_FLAG_LSIRDY) == 0u)
  {
  }

  WRITE_REG(s_hiwdg->KR,   IWDG_KEY_ACCESS);    /* 解锁 */
  WRITE_REG(s_hiwdg->PR,   IWDG_PRESCALER_64);
  WRITE_REG(s_hiwdg->RLR,  IWDG_RELOAD);
  WRITE_REG(s_hiwdg->WINR, IWDG_WINDOW_FULL);
  while ((READ_REG(s_hiwdg->SR) & (IWDG_SR_PVU | IWDG_SR_RVU | IWDG_SR_WVU)) != 0u)
  {
  }
  WRITE_REG(s_hiwdg->KR, IWDG_KEY_RELOAD);      /* 装载计数器 */
  WRITE_REG(s_hiwdg->KR, IWDG_KEY_ENABLE);      /* 启动 */
#endif /* WDG_ENABLE */
}

/**
  * @brief  喂狗
  * @retval 无
  */
void BSP_WDT_Feed(void)
{
#if WDG_ENABLE
  WRITE_REG(s_hiwdg->KR, IWDG_KEY_RELOAD);
#endif /* WDG_ENABLE */
}
