#ifndef BSP_TIME_H
#define BSP_TIME_H
#include <stdint.h>
#define BSP_GRAY_BUDGET_CYCLES 336000u
#define BSP_CONTROL_BUDGET_CYCLES 432000u
void BSP_Time_Init(void);
uint32_t BSP_Time_GetCycles(void);
uint32_t BSP_EnterCritical(void);
void BSP_ExitCritical(uint32_t previous_mask);
void BSP_MemoryBarrier(void);
#endif
