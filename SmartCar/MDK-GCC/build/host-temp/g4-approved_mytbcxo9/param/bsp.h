
#ifndef TEST_BSP_H
#define TEST_BSP_H
#include <stdint.h>
typedef enum {
    BSP_ADC_IND_H_L, BSP_ADC_IND_V_L, BSP_ADC_IND_V_R, BSP_ADC_IND_H_R, BSP_ADC_KEY1, BSP_ADC_KEY2
} bsp_adc_channel_t;
uint16_t BSP_ADC_Read(bsp_adc_channel_t ch);
void BSP_ADC_Init(void);
uint8_t BSP_IMU660RB_Init(void);
void BSP_IMU660RB_GetAcc(int16_t *x, int16_t *y, int16_t *z);
void BSP_IMU660RB_GetGyro(int16_t *x, int16_t *y, int16_t *z);
void BSP_Sampler_GetGyroRaw(int16_t *x, int16_t *y, int16_t *z);
int16_t BSP_Sampler_ConsumeEncL(void);
int16_t BSP_Sampler_ConsumeEncR(void);
void BSP_Encoder_Init(void);
void BSP_PWM_Init(void);
void BSP_PWM_SetDuty(int channel, uint32_t duty);
void BSP_PWM_SetDirection(int channel, uint8_t high);
enum { BSP_PWM_MOTOR_L, BSP_PWM_MOTOR_R, BSP_PWM_FAN };
enum { GPIO_PIN_RESET, GPIO_PIN_SET };
#define MOTOR_L_DIR_GPIO_Port ((void *)1)
#define MOTOR_R_DIR_GPIO_Port ((void *)2)
#define MOTOR_L_DIR_Pin 1u
#define MOTOR_R_DIR_Pin 2u
void HAL_GPIO_WritePin(void *port, uint16_t pin, int state);
#endif

#include "bsp_lcd.h"
void BSP_UART_WriteString(const char*);
uint16_t BSP_UART_Read(uint8_t*,uint16_t);
uint8_t BSP_Flash_Read(uint32_t,uint8_t*,uint32_t);
uint8_t BSP_Flash_Write(uint32_t,const uint8_t*,uint32_t);
uint32_t HAL_GetTick(void);
uint32_t BSP_Time_GetTick(void);
uint32_t BSP_Time_EnterCritical(void);
void BSP_Time_ExitCritical(uint32_t);

#include "bsp_key.h"
#include "bsp_runtime.h"
#include "bsp_dl1b.h"
#include "bsp_sampler.h"
#include "bsp_wdt.h"
void BSP_UART_Init(void);
void BSP_Flash_Init(void);
