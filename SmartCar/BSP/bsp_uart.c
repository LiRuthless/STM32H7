/**
  ******************************************************************************
  * @file    bsp_uart.c
  * @brief   USART1(115200) 收发：单字节中断接收 + 256 字节环形缓冲，
  *          阻塞发送，printf 重定向（AC6/Keil）。
  ******************************************************************************
  */

/* 包含头文件 ------------------------------------------------------------------*/
#include "bsp_uart.h"
#include "usart.h"

#include <stdio.h>
#include <string.h>

/* 私有定义 -----------------------------------------------------------*/
#define UART_RX_BUF_SIZE    256u    /* 必须为 2 的幂，用于位运算回绕 */

/* 私有变量 ---------------------------------------------------------*/
static uint8_t  s_rx_buf[UART_RX_BUF_SIZE];
static volatile uint16_t s_rx_head;    /* 写指针（中断写入） */
static volatile uint16_t s_rx_tail;    /* 读指针（应用读出） */
static uint8_t  s_rx_byte;             /* 单字节中断接收暂存 */

/* 私有函数原型 ---------------------------------------------*/
static void uart_rx_push(uint8_t ch);

/* 导出函数定义 -------------------------------------------*/

/**
  * @brief  启动首次单字节中断接收
  * @retval 无
  */
void BSP_UART_Init(void)
{
  s_rx_head = 0;
  s_rx_tail = 0;
  (void)HAL_UART_Receive_IT(&huart1, &s_rx_byte, 1);
}

/**
  * @brief  阻塞发送
  * @param  buf 数据缓冲
  * @param  len 数据长度
  * @retval 未发送字节数，0 = 全部发完
  */
uint32_t BSP_UART_Write(const uint8_t *buf, uint16_t len)
{
  if (buf == NULL || len == 0u)
  {
    return 0;
  }
  if (HAL_UART_Transmit(&huart1, (uint8_t *)buf, len, 100) == HAL_OK)
  {
    return 0;
  }
  /* 发送失败：按已发出的字节数折算 */
  return (uint32_t)(len - (uint16_t)huart1.TxXferCount);
}

/**
  * @brief  发送字符串（遇 '\0' 结束）
  * @retval 未发送字节数，0 = 全部发完
  */
uint32_t BSP_UART_WriteString(const char *str)
{
  if (str == NULL)
  {
    return 1;
  }
  return BSP_UART_Write((const uint8_t *)str, (uint16_t)strlen(str));
}

/**
  * @brief  从接收环形缓冲读出数据
  * @retval 实际读出字节数
  */
uint16_t BSP_UART_Read(uint8_t *buf, uint16_t len)
{
  uint16_t cnt = 0;

  if (buf == NULL)
  {
    return 0;
  }
  while (cnt < len && s_rx_tail != s_rx_head)
  {
    buf[cnt++] = s_rx_buf[s_rx_tail];
    s_rx_tail = (uint16_t)((s_rx_tail + 1u) & (UART_RX_BUF_SIZE - 1u));
  }
  return cnt;
}

/**
  * @brief  查询接收缓冲中未读字节数
  */
uint16_t BSP_UART_RxAvailable(void)
{
  return (uint16_t)((s_rx_head - s_rx_tail) & (UART_RX_BUF_SIZE - 1u));
}

/**
  * @brief  清空接收缓冲
  */
void BSP_UART_RxFlush(void)
{
  s_rx_tail = s_rx_head;
}

/* 私有函数定义 -------------------------------------------*/

/**
  * @brief  字节入队；缓冲满时丢弃最旧的一字节为新字节让位
  *         （新数据通常比旧数据更有价值）
  */
static void uart_rx_push(uint8_t ch)
{
  uint16_t next = (uint16_t)((s_rx_head + 1u) & (UART_RX_BUF_SIZE - 1u));

  if (next == s_rx_tail)          /* 满：丢最旧 */
  {
    s_rx_tail = (uint16_t)((s_rx_tail + 1u) & (UART_RX_BUF_SIZE - 1u));
  }
  s_rx_buf[s_rx_head] = ch;
  s_rx_head = next;
}

/* 中断回调 ---------------------------------------------------------*/

/**
  * @brief  UART 接收完成回调（全工程唯一实现）
  *         USART1：字节入队后重新武装下一次接收
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart == &huart1)
  {
    uart_rx_push(s_rx_byte);
    (void)HAL_UART_Receive_IT(&huart1, &s_rx_byte, 1);
  }
}

/**
  * @brief  UART 错误回调： overrun/帧错误等恢复接收，
  *         否则一次总线错误后接收永久停摆
  */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart == &huart1)
  {
    __HAL_UART_CLEAR_OREFLAG(huart);
    __HAL_UART_CLEAR_FEFLAG(huart);
    __HAL_UART_CLEAR_NEFLAG(huart);
    __HAL_UART_CLEAR_PEFLAG(huart);
    (void)HAL_UART_Receive_IT(&huart1, &s_rx_byte, 1);
  }
}

/* printf 重定向 ------------------------------------------------------*/

/**
  * @brief  标准输出重定向到 USART1（AC6/Keil，stdio 模式）
  */
int fputc(int ch, FILE *f)
{
  (void)f;
  (void)BSP_UART_Write((const uint8_t *)&ch, 1);
  return ch;
}
