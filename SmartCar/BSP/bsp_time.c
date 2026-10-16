#include "bsp_time.h"
#include "main.h"
void BSP_Time_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    /* 自由运行，任何测量/重启均不得清零 CYCCNT。 */
}
uint32_t BSP_Time_GetCycles(void) { return DWT->CYCCNT; }
