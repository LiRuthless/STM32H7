/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   本文件提供所有已使用 GPIO 引脚的
  *          配置代码。
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

/* 包含头文件 ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* 配置 GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** 配置引脚
     PC14-OSC32_IN (OSC32_IN)   ------> RCC_OSC32_IN
     PC15-OSC32_OUT (OSC32_OUT)   ------> RCC_OSC32_OUT
     PH0-OSC_IN (PH0)   ------> RCC_OSC_IN
     PH1-OSC_OUT (PH1)   ------> RCC_OSC_OUT
     PA13 (JTMS/SWDIO)   ------> DEBUG_JTMS-SWDIO
     PA14 (JTCK/SWCLK)   ------> DEBUG_JTCK-SWCLK
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO 端口时钟使能 */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*配置 GPIO 引脚输出电平 */
  HAL_GPIO_WritePin(GPIOE, LED_BLUE_Pin|MOTOR_L_DIR_Pin|DL1B_XS_Pin|MOTOR_R_DIR_Pin, GPIO_PIN_RESET);

  /*配置 GPIO 引脚输出电平 */
  HAL_GPIO_WritePin(GPIOE, LCD2_CS_Pin|LCD_CS_Pin|LCD_WR_RS_Pin, GPIO_PIN_SET);

  /*配置 GPIO 引脚输出电平 */
  HAL_GPIO_WritePin(WIFI_CS_GPIO_Port, WIFI_CS_Pin, GPIO_PIN_SET);

  /*配置 GPIO 引脚输出电平 */
  HAL_GPIO_WritePin(GPIOD, LCD2_RST_Pin|LCD2_BLK_Pin|WIFI_RST_Pin|IMU_CS_Pin, GPIO_PIN_SET);

  /*配置 GPIO 引脚输出电平 */
  HAL_GPIO_WritePin(LED_EXT_GPIO_Port, LED_EXT_Pin, GPIO_PIN_RESET);

  /*配置 GPIO 引脚 : PEPin PEPin PEPin PEPin
                           PEPin */
  GPIO_InitStruct.Pin = LED_BLUE_Pin|MOTOR_L_DIR_Pin|DL1B_XS_Pin|LCD2_CS_Pin
                          |MOTOR_R_DIR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*配置 GPIO 引脚 : PEPin PEPin PEPin PEPin */
  GPIO_InitStruct.Pin = GRAY_IO1_Pin|GRAY_IO2_Pin|GRAY_IO3_Pin|GRAY_IO4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*配置 GPIO 引脚 : PtPin */
  GPIO_InitStruct.Pin = KEY_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(KEY_GPIO_Port, &GPIO_InitStruct);

  /*配置 GPIO 引脚 : PEPin PEPin */
  GPIO_InitStruct.Pin = LCD_CS_Pin|LCD_WR_RS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*配置 GPIO 引脚 : PBPin PBPin */
  GPIO_InitStruct.Pin = WIFI_CS_Pin|LED_EXT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*配置 GPIO 引脚 : PDPin PDPin PDPin PDPin */
  GPIO_InitStruct.Pin = LCD2_RST_Pin|LCD2_BLK_Pin|WIFI_RST_Pin|IMU_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*配置 GPIO 引脚 : PtPin */
  GPIO_InitStruct.Pin = WIFI_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(WIFI_INT_GPIO_Port, &GPIO_InitStruct);

  /* EXTI 中断初始化*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 4, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
