/**
 * @file bsp_uart.h
 * @brief BSP USART1 中断接收与阻塞发送；供 App 前台调参和日志导出。
 * @details 声明 UART 初始化、收发与字符串接口；完整阻塞和错误语义位于 bsp_uart.c。
 */
#ifndef SMARTCAR_BSP_BSP_UART_H
#define SMARTCAR_BSP_BSP_UART_H

#include "main.h"

/* USART1 (PA9/PA10) 115200：调试输出 + 无线调参共用
 * 接收：单字节中断 + 环形缓冲；发送：阻塞 */

/**
 * @brief 清空 USART1 接收索引并开启单字节接收中断。
 * @note 启动主循环调用，保持原 HAL 返回值忽略行为。 完整契约见 bsp_uart.c::BSP_UART_Init。
 */
void BSP_UART_Init(void);
/**
 * @brief 使用 USART1 阻塞发送字节。
 * @note 单次 HAL 超时 100 ms；前台使用，不与其他发送交错。 完整契约见 bsp_uart.c::BSP_UART_Write。
 */
uint32_t BSP_UART_Write(const uint8_t *buf, uint16_t len);
/**
 * @brief 发送以零结尾的字符串。
 * @note str 字符串；为空时返回 1，否则必须有效且以零结尾。 前台阻塞发送；长度保持原 uint16_t 转换。 完整契约见
 *     bsp_uart.c::BSP_UART_WriteString。
 */
uint32_t BSP_UART_WriteString(const char *str);
/**
 * @brief 从接收环形缓冲消费字节。
 * @note buf 接收区；非空时容量至少 len 字节，逐字节写入，空指针返回 0。 前台读尾索引；UART ISR 写头索引，缓冲满时 ISR 丢最旧字节。 完整契约见
 *     bsp_uart.c::BSP_UART_Read。
 */
uint16_t BSP_UART_Read(uint8_t *buf, uint16_t len);
/**
 * @brief 查询接收环形缓冲当前可读字节数。
 * @note 完整契约见 bsp_uart.c::BSP_UART_RxAvailable。
 */
uint16_t BSP_UART_RxAvailable(void);
/**
 * @brief 将消费索引推进到当前生产索引以丢弃缓存。
 * @note 前台调用，不关闭接收中断。 完整契约见 bsp_uart.c::BSP_UART_RxFlush。
 */
void BSP_UART_RxFlush(void);

#endif /* __BSP_UART_H */
