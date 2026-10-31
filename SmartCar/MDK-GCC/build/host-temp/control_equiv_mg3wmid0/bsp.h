
#ifndef TEST_BSP_H
#define TEST_BSP_H
#include <stdint.h>
typedef enum {
    BSP_ADC_IND_H_L, BSP_ADC_IND_V_L, BSP_ADC_IND_V_R, BSP_ADC_IND_H_R
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
enum { BSP_PWM_MOTOR_L, BSP_PWM_MOTOR_R, BSP_PWM_FAN };
enum { GPIO_PIN_RESET, GPIO_PIN_SET };
#define MOTOR_L_DIR_GPIO_Port ((void *)1)
#define MOTOR_R_DIR_GPIO_Port ((void *)2)
#define MOTOR_L_DIR_Pin 1u
#define MOTOR_R_DIR_Pin 2u
void HAL_GPIO_WritePin(void *port, uint16_t pin, int state);
#endif
