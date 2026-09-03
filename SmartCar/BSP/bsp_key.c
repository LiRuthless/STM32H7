/**
  ******************************************************************************
  * @file    bsp_key.c
  * @brief   启动按键 K1 = PC13（下拉输入，按下为高电平）。
  *          主循环轮询，按下沿检测 + 30ms 消抖。
  ******************************************************************************
  */

/* 包含头文件 ------------------------------------------------------------------*/
#include "bsp_key.h"

/* 私有定义 -----------------------------------------------------------*/
#define KEY_DEBOUNCE_MS     30u     /* 消抖时间 */

/* 私有变量 ---------------------------------------------------------*/
static uint8_t  s_last_level = 0;       /* 上次电平（按下=1） */
static uint32_t s_last_tick  = 0;       /* 上次有效按下的时刻 */

/* 导出函数定义 -------------------------------------------*/

/**
  * @brief  初始化（GPIO 已由 CubeMX 配置，此处复位内部状态）
  * @retval 无
  */
void BSP_Key_Init(void)
{
  s_last_level = (HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin) == GPIO_PIN_SET) ? 1u : 0u;
  s_last_tick  = HAL_GetTick();
}

/**
  * @brief  轮询检测一次有效按下（按下沿 + 消抖）
  * @retval 1 = 检测到有效按下，0 = 无
  */
uint8_t BSP_Key_StartPressed(void)
{
  uint8_t  level = (HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin) == GPIO_PIN_SET) ? 1u : 0u;
  uint8_t  pressed = 0;
  uint32_t now;

  if (level != 0u && s_last_level == 0u)        /* 按下沿 */
  {
    now = HAL_GetTick();
    if ((now - s_last_tick) >= KEY_DEBOUNCE_MS)
    {
      s_last_tick = now;
      pressed = 1;
    }
  }
  s_last_level = level;
  return pressed;
}
