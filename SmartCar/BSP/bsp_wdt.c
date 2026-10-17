/**
 * @file bsp_wdt.c
 * @brief 独立看门狗 IWDG1：约 6s 超时（LSI 32kHz / 64 / (2999+1)）。
 * @details 配置独立看门狗并提供刷新操作；首次起跑启用，停车前台继续按契约喂狗。
 * @note 本工程 HAL 配置（stm32h7xx_hal_conf.h）未使能 HAL_IWDG_MODULE_ENABLED， 且 Drivers 中不含
 *     stm32h7xx_hal_iwdg.c/h，因此此处直接操作寄存器实现。 超时取 6s 而非 1s：给 Flash 存储等阻塞操作留足余量。
 *     *****************************************************************************
 */
#include "bsp_wdt.h"
#include "app_config.h"

#if APP_WDG_ENABLE
#define BSP_WDT_KEY_RELOAD 0x0000AAAAu /* 喂狗 */
#define BSP_WDT_KEY_ENABLE 0x0000CCCCu /* 启动（一旦启动不可关闭） */
#define BSP_WDT_KEY_ACCESS 0x00005555u /* 解锁 PR/RLR/WINR 写访问 */
#define BSP_WDT_PRESCALER_64 4u        /* PR=4 → 64 分频 */
#define BSP_WDT_RELOAD                                                                             \
    2999u                               /* 32000/64=500Hz，3000 计数 ≈ 6s；
                                             * 为 W25Q64 参数保存与日志收尾留出余量 */
#define BSP_WDT_WINDOW_FULL 0x00000FFFu /* 窗口禁用（=满量程） */
/* 句柄静态定义在本文件 */
static IWDG_TypeDef *const s_hiwdg = IWDG1;

#endif /* WDG_ENABLE */
/**
 * @brief 按配置首次启动 IWDG1。
 * @details 按现有调用顺序使用 __HAL_RCC_LSI_ENABLE、__HAL_RCC_GET_FLAG、WRITE_REG、READ_REG
 *     完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @note 首次起跑调用；开启时等待 LSI 和寄存器同步，不能重复作为运行复位使用。
 */
void BSP_WDT_Init(void)
{
#if APP_WDG_ENABLE
    /* IWDG 时钟源为 LSI，需先使能并等待就绪 */
    __HAL_RCC_LSI_ENABLE();
    while (__HAL_RCC_GET_FLAG(RCC_FLAG_LSIRDY) == 0u)
    {
        /* 保持原有等待循环为空，仅轮询硬件就绪位。 */
    }

    WRITE_REG(s_hiwdg->KR, BSP_WDT_KEY_ACCESS); /* 解锁 */
    WRITE_REG(s_hiwdg->PR, BSP_WDT_PRESCALER_64);
    WRITE_REG(s_hiwdg->RLR, BSP_WDT_RELOAD);
    WRITE_REG(s_hiwdg->WINR, BSP_WDT_WINDOW_FULL);
    while ((READ_REG(s_hiwdg->SR) & (IWDG_SR_PVU | IWDG_SR_RVU | IWDG_SR_WVU)) != 0u)
    {
        /* 保持原有等待循环为空，仅轮询硬件就绪位。 */
    }
    WRITE_REG(s_hiwdg->KR, BSP_WDT_KEY_RELOAD); /* 装载计数器 */
    WRITE_REG(s_hiwdg->KR, BSP_WDT_KEY_ENABLE); /* 启动 */
#endif                                          /* WDG_ENABLE */
}

/**
 * @brief 在看门狗启用配置下重装 IWDG1。
 * @note 运行 TIM7 或停车主循环调用；宏关闭时保持空操作。
 */
void BSP_WDT_Feed(void)
{
#if APP_WDG_ENABLE
    WRITE_REG(s_hiwdg->KR, BSP_WDT_KEY_RELOAD);
#endif /* WDG_ENABLE */
}
