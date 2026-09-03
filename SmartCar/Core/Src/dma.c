/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    dma.c
  * @brief   本文件提供所有已请求的存储器到存储器
  *          DMA 传输的配置代码。
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
#include "dma.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* 配置 DMA                                                              */
/*----------------------------------------------------------------------------*/

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/**
  * 使能 DMA 控制器时钟
  */
void MX_DMA_Init(void)
{

  /* DMA 控制器时钟使能 */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA 中断初始化 */
  /* DMA1_Stream0_IRQn 中断配置 */
  HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */

