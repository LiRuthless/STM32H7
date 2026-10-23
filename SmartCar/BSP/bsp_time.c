/**
 * @file bsp_time.c
 * @brief BSP DWT 周期计数、临界区与内存发布屏障。
 * @details 封装自由运行 DWT 周期计数、PRIMASK 临界区和 DMB 发布屏障，供 ISR 与前台共享。
 */
#include "bsp_time.h"
#include "main.h"

/**
 * @brief 启用自由运行 DWT 周期计数。
 * @details 按函数体既定条件和写入顺序更新模块状态；提前返回分支不执行其后的状态发布。
 * @note 不清零 CYCCNT，保持其他测量与无符号回绕差值有效。
 */
void BSP_Time_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    /* 自由运行，任何测量/重启均不得清零 CYCCNT。 */
}

/**
 * @brief 读取当前 DWT 周期计数。
 * @details 直接读取自由运行的 DWT CYCCNT；调用方以无符号相减计算时间差，从而允许 32 位自然回绕。
 * @return 480 MHz 内核周期计数，按 uint32_t 自然回绕。
 * @pre 已调用 BSP_Time_Init；前台或 ISR 可读。
 * @note 前台和 ISR 均可读取；不清零计数器，也不提供等待或互斥。
 */
uint32_t BSP_Time_GetCycles(void)
{
    return DWT->CYCCNT;
}

/**
 * @brief 保存当前中断屏蔽状态并关闭可屏蔽中断。
 * @details 按现有调用顺序使用 __get_PRIMASK、__disable_irq 完成本函数处理。
 * @return 进入前 PRIMASK，必须交给对应 ExitCritical 恢复。
 * @note 前台或 ISR 调用；不改变调用者已有的嵌套屏蔽状态。
 */
uint32_t BSP_Time_EnterCritical(void)
{
    uint32_t old = __get_PRIMASK();
    __disable_irq();
    return old;
}

/**
 * @brief 恢复配对临界区进入前的中断屏蔽状态。
 * @details 把配对进入函数保存的 PRIMASK 原值写回，恢复调用前的可屏蔽中断状态而非无条件开中断。
 * @param[in] old 配对 BSP_Time_EnterCritical 返回的 PRIMASK 原值。
 * @note 必须与同一路径的 EnterCritical 配对；可从前台或 ISR 恢复原屏蔽状态。
 */
void BSP_Time_ExitCritical(uint32_t old)
{
    __set_PRIMASK(old);
}

/**
 * @brief 执行 DMB，保持共享记录内容与发布索引的可见顺序。
 * @note 前台及 ISR 可用；不替代临界区互斥。
 */
void BSP_Time_MemoryBarrier(void)
{
    __DMB();
}

/**
 * @brief 读取HAL毫秒时基。
 * @return uint32_t毫秒计数，自然回绕。
 * @note 只读一次，不关中断、不等待；由原调用者计算无符号差值。
 */
uint32_t BSP_Time_GetTick(void)
{
    return HAL_GetTick();
}
