#ifndef TEST_MAIN_H
#define TEST_MAIN_H
#include <stdint.h>
#define MODIFY_REG(a,b,c) ((void)0)
#define GPIO_PIN_RESET 0
#define GPIO_PIN_SET 1
#define KEY_GPIO_Port ((void*)1)
#define KEY_Pin 13
#define LED_BLUE_GPIO_Port ((void*)2)
#define LED_BLUE_Pin 3
#define MOTOR_L_DIR_GPIO_Port ((void*)3)
#define MOTOR_L_DIR_Pin 1
#define MOTOR_R_DIR_GPIO_Port ((void*)4)
#define MOTOR_R_DIR_Pin 2
#define TIM6 ((void*)6)
#define TIM7 ((void*)7)
typedef struct {void* Instance;} TIM_HandleTypeDef;
extern TIM_HandleTypeDef htim6,htim7;
uint32_t HAL_GetTick(void);
uint32_t BSP_Time_GetTick(void);
int HAL_GPIO_ReadPin(void*,uint16_t);
void HAL_GPIO_WritePin(void*,uint16_t,int);
void HAL_GPIO_TogglePin(void*,uint16_t);
int HAL_TIM_Base_Start_IT(TIM_HandleTypeDef*);
int HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef*);
void Test_InjectStop(void);
#endif
