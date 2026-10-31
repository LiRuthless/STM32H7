/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : main.c 文件的头文件。
  *                   本文件包含应用程序的通用定义。
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* 防止递归包含的宏定义 -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* 包含头文件 ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* 私有包含 ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* 导出类型 ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* 导出常量 --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* 导出宏 ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* 导出函数原型 ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* 私有定义 -----------------------------------------------------------*/
#define LED_BLUE_Pin GPIO_PIN_3
#define LED_BLUE_GPIO_Port GPIOE
#define GRAY_IO1_Pin GPIO_PIN_4
#define GRAY_IO1_GPIO_Port GPIOE
#define GRAY_IO2_Pin GPIO_PIN_6
#define GRAY_IO2_GPIO_Port GPIOE
#define KEY_Pin GPIO_PIN_13
#define KEY_GPIO_Port GPIOC
#define INDUCTOR1_Pin GPIO_PIN_0
#define INDUCTOR1_GPIO_Port GPIOC
#define INDUCTOR2_Pin GPIO_PIN_1
#define INDUCTOR2_GPIO_Port GPIOC
#define ENC_L_A_Pin GPIO_PIN_0
#define ENC_L_A_GPIO_Port GPIOA
#define ENC_L_B_Pin GPIO_PIN_1
#define ENC_L_B_GPIO_Port GPIOA
#define KEY_ADC1_Pin GPIO_PIN_2
#define KEY_ADC1_GPIO_Port GPIOA
#define KEY_ADC2_Pin GPIO_PIN_3
#define KEY_ADC2_GPIO_Port GPIOA
#define INDUCTOR3_Pin GPIO_PIN_4
#define INDUCTOR3_GPIO_Port GPIOA
#define ADC_SPARE2_Pin GPIO_PIN_5
#define ADC_SPARE2_GPIO_Port GPIOA
#define MOTOR_L_PWM_Pin GPIO_PIN_6
#define MOTOR_L_PWM_GPIO_Port GPIOA
#define FAN_PWM_Pin GPIO_PIN_7
#define FAN_PWM_GPIO_Port GPIOA
#define GRAY_ADC_Pin GPIO_PIN_4
#define GRAY_ADC_GPIO_Port GPIOC
#define INDUCTOR4_Pin GPIO_PIN_5
#define INDUCTOR4_GPIO_Port GPIOC
#define VBAT_ADC_Pin GPIO_PIN_0
#define VBAT_ADC_GPIO_Port GPIOB
#define ADC_SPARE1_Pin GPIO_PIN_1
#define ADC_SPARE1_GPIO_Port GPIOB
#define MOTOR_L_DIR_Pin GPIO_PIN_7
#define MOTOR_L_DIR_GPIO_Port GPIOE
#define DL1B_XS_Pin GPIO_PIN_8
#define DL1B_XS_GPIO_Port GPIOE
#define LCD2_CS_Pin GPIO_PIN_9
#define LCD2_CS_GPIO_Port GPIOE
#define LCD_BLK_Pin GPIO_PIN_10
#define LCD_BLK_GPIO_Port GPIOE
#define LCD_CS_Pin GPIO_PIN_11
#define LCD_CS_GPIO_Port GPIOE
#define LCD_WR_RS_Pin GPIO_PIN_13
#define LCD_WR_RS_GPIO_Port GPIOE
#define MOTOR_R_DIR_Pin GPIO_PIN_15
#define MOTOR_R_DIR_GPIO_Port GPIOE
#define WIFI_CS_Pin GPIO_PIN_12
#define WIFI_CS_GPIO_Port GPIOB
#define LCD2_RST_Pin GPIO_PIN_9
#define LCD2_RST_GPIO_Port GPIOD
#define LCD2_BLK_Pin GPIO_PIN_10
#define LCD2_BLK_GPIO_Port GPIOD
#define MOTOR_R_PWM_Pin GPIO_PIN_14
#define MOTOR_R_PWM_GPIO_Port GPIOD
#define WIFI_RST_Pin GPIO_PIN_15
#define WIFI_RST_GPIO_Port GPIOD
#define ENC_R_A_Pin GPIO_PIN_6
#define ENC_R_A_GPIO_Port GPIOC
#define ENC_R_B_Pin GPIO_PIN_7
#define ENC_R_B_GPIO_Port GPIOC
#define WIFI_INT_Pin GPIO_PIN_15
#define WIFI_INT_GPIO_Port GPIOA
#define WIFI_INT_EXTI_IRQn EXTI15_10_IRQn
#define IMU_CS_Pin GPIO_PIN_3
#define IMU_CS_GPIO_Port GPIOD
#define LED_EXT_Pin GPIO_PIN_5
#define LED_EXT_GPIO_Port GPIOB
#define SPARE_PWM2_Pin GPIO_PIN_8
#define SPARE_PWM2_GPIO_Port GPIOB
#define SPARE_PWM1_Pin GPIO_PIN_9
#define SPARE_PWM1_GPIO_Port GPIOB
#define GRAY_IO3_Pin GPIO_PIN_0
#define GRAY_IO3_GPIO_Port GPIOE
#define GRAY_IO4_Pin GPIO_PIN_1
#define GRAY_IO4_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
