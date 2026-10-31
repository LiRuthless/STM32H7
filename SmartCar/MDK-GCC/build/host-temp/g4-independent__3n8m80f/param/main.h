#include <stdint.h>
#define KEY_GPIO_Port ((void*)3)
#define KEY_Pin 13
int HAL_GPIO_ReadPin(void*,uint16_t);
uint32_t HAL_GetTick(void);
uint32_t BSP_Time_GetTick(void);
