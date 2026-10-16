/**
 * @file bsp_time.h
 * @brief BSP DWT 周期计数、临界区与内存发布屏障。
 */
#ifndef SMARTCAR_BSP_BSP_TIME_H
#define SMARTCAR_BSP_BSP_TIME_H
#include <stdint.h>
#define BSP_GRAY_BUDGET_CYCLES 336000u
#define BSP_CONTROL_BUDGET_CYCLES 432000u
/**
 * @brief 启用自由运行 DWT 周期计数。
 * @note 不清零 CYCCNT，保持其他测量与无符号回绕差值有效。
 */
void BSP_Time_Init(void);
/**
 * @brief 读取当前 DWT 周期计数。
 * @return 480 MHz 内核周期计数，按 uint32_t 自然回绕。
 * @pre 已调用 BSP_Time_Init；前台或 ISR 可读。
 */
uint32_t BSP_Time_GetCycles(void);
/**
 * @brief 保存当前中断屏蔽状态并关闭可屏蔽中断。
 * @return 进入前 PRIMASK，必须交给对应 ExitCritical 恢复。
 * @note 前台或 ISR 调用；不改变调用者已有的嵌套屏蔽状态。
 */
uint32_t BSP_Time_EnterCritical(void);
/**
 * @brief 恢复配对临界区进入前的中断屏蔽状态。
 * @param[in] previous_mask 配对 EnterCritical 返回的 PRIMASK 原值。
 */
void BSP_Time_ExitCritical(uint32_t previous_mask);
/**
 * @brief 执行 DMB，保持共享记录内容与发布索引的可见顺序。
 * @note 前台及 ISR 可用；不替代临界区互斥。
 */
void BSP_Time_MemoryBarrier(void);
#endif
