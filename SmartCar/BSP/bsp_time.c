#include "bsp_time.h"
#include "main.h"
void BSP_Time_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    /* 自由运行，任何测量/重启均不得清零 CYCCNT。 */
}
uint32_t BSP_Time_GetCycles(void) { return DWT->CYCCNT; }
uint32_t BSP_EnterCritical(void) { uint32_t old=__get_PRIMASK(); __disable_irq(); return old; }
void BSP_ExitCritical(uint32_t old) { __set_PRIMASK(old); }
void BSP_MemoryBarrier(void) { __DMB(); }
