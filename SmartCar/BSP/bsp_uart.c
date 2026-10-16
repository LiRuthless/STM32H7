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
#define BSP_UART_RX_BUF_SIZE 256u /* 必须为 2 的幂，用于位运算回绕 */

/* 私有变量 ---------------------------------------------------------*/
static uint8_t s_rx_buf[BSP_UART_RX_BUF_SIZE];
static volatile uint16_t s_rx_head; /* 写指针（中断写入） */
static volatile uint16_t s_rx_tail; /* 读指针（应用读出） */
static uint8_t s_rx_byte;           /* 单字节中断接收暂存 */

/* 私有函数原型 ---------------------------------------------*/
static void BSP_UART_PushRx(uint8_t ch);

/* 导出函数定义 -------------------------------------------*/

void BSP_UART_Init(void)
{
    s_rx_head = 0;
    s_rx_tail = 0;
    (void)HAL_UART_Receive_IT(&huart1, &s_rx_byte, 1);
}

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

uint32_t BSP_UART_WriteString(const char *str)
{
    if (str == NULL)
    {
        return 1;
    }
    return BSP_UART_Write((const uint8_t *)str, (uint16_t)strlen(str));
}

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
        s_rx_tail = (uint16_t)((s_rx_tail + 1u) & (BSP_UART_RX_BUF_SIZE - 1u));
    }
    return cnt;
}

uint16_t BSP_UART_RxAvailable(void)
{
    return (uint16_t)((s_rx_head - s_rx_tail) & (BSP_UART_RX_BUF_SIZE - 1u));
}

void BSP_UART_RxFlush(void)
{
    s_rx_tail = s_rx_head;
}

/* 私有函数定义 -------------------------------------------*/

/**
 * @brief 将 UART ISR 收到的字节发布到环形缓冲，满时丢最旧字节。
 * @param[in] ch 待发布的原始接收字节，0～255。
 * @note 仅 UART ISR 写入，保留原满缓冲处理。
 */
static void BSP_UART_PushRx(uint8_t ch)
{
    uint16_t next = (uint16_t)((s_rx_head + 1u) & (BSP_UART_RX_BUF_SIZE - 1u));

    if (next == s_rx_tail) /* 满：丢最旧 */
    {
        s_rx_tail = (uint16_t)((s_rx_tail + 1u) & (BSP_UART_RX_BUF_SIZE - 1u));
    }
    s_rx_buf[s_rx_head] = ch;
    s_rx_head = next;
}

/* 中断回调 ---------------------------------------------------------*/

/**
 * @brief USART1 单字节接收完成回调。
 * @param[in] huart HAL 句柄，须有效；仅处理 huart1。
 * @note UART ISR 先发布接收字节，再重新挂接下一字节接收。
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart1)
    {
        BSP_UART_PushRx(s_rx_byte);
        (void)HAL_UART_Receive_IT(&huart1, &s_rx_byte, 1);
    }
}

/**
 * @brief 清除 USART1 接收错误并重新挂接接收。
 * @param[in] huart HAL 句柄，须有效；仅处理 huart1。
 * @note HAL 固定入口，保持原清标志顺序。
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
 * @brief 将运行库字符输出重定向到 USART1。
 * @param[in] ch 原字符值，只发送其内存中的一个字节。
 * @param[in] f 运行库流指针，本实现忽略，允许为空。
 * @return 原 ch 值；保持串口发送错误忽略行为。
 * @note 阻塞发送；运行库固定名称。
 */
int fputc(int ch, FILE *f)
{
    (void)f;
    (void)BSP_UART_Write((const uint8_t *)&ch, 1);
    return ch;
}
