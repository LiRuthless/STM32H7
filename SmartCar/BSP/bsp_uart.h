#ifndef __BSP_UART_H
#define __BSP_UART_H

#include "main.h"

/* USART1 (PA9/PA10) 115200：调试输出 + 无线调参共用
 * 接收：单字节中断 + 环形缓冲；发送：阻塞 */

void     BSP_UART_Init(void);
uint32_t BSP_UART_Write(const uint8_t *buf, uint16_t len);   /* 返回未发送字节数，0=全部发完 */
uint32_t BSP_UART_WriteString(const char *str);
uint16_t BSP_UART_Read(uint8_t *buf, uint16_t len);          /* 从接收缓冲读出，返回实际字节数 */
uint16_t BSP_UART_RxAvailable(void);
void     BSP_UART_RxFlush(void);

#endif /* __BSP_UART_H */
