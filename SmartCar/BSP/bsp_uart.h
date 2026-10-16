/**
 * @file bsp_uart.h
 * @brief BSP USART1 中断接收与阻塞发送；供 App 前台调参和日志导出。
 */
#ifndef SMARTCAR_BSP_BSP_UART_H
#define SMARTCAR_BSP_BSP_UART_H

#include "main.h"

/* USART1 (PA9/PA10) 115200：调试输出 + 无线调参共用
 * 接收：单字节中断 + 环形缓冲；发送：阻塞 */

/**
 * @brief 清空 USART1 接收索引并开启单字节接收中断。
 * @note 启动主循环调用，保持原 HAL 返回值忽略行为。
 */
void BSP_UART_Init(void);
/**
 * @brief 使用 USART1 阻塞发送字节。
 * @param[in] buf 输入字节；为空或 len 为零时直接返回 0。
 * @param[in] len 字节数，uint16_t 范围。
 * @return HAL 成功时 0；失败时按原 len - TxXferCount 计算返回值。
 * @note 单次 HAL 超时 100 ms；前台使用，不与其他发送交错。
 */
uint32_t BSP_UART_Write(const uint8_t *buf, uint16_t len);
/**
 * @brief 发送以零结尾的字符串。
 * @param[in] str 字符串；为空时返回 1，否则必须有效且以零结尾。
 * @return 空指针返回 1，否则返回 BSP_UART_Write 的原始结果。
 * @note 前台阻塞发送；长度保持原 uint16_t 转换。
 */
uint32_t BSP_UART_WriteString(const char *str);
/**
 * @brief 从接收环形缓冲消费字节。
 * @param[out] buf 接收区；非空时容量至少 len 字节，逐字节写入，空指针返回 0。
 * @param[in] len 最多读取的字节数。
 * @return 本次实际读取的字节数，0～len。
 * @note 前台读尾索引；UART ISR 写头索引，缓冲满时 ISR 丢最旧字节。
 */
uint16_t BSP_UART_Read(uint8_t *buf, uint16_t len);
/**
 * @brief 查询接收环形缓冲当前可读字节数。
 * @return 环形索引差，范围 0～255。
 */
uint16_t BSP_UART_RxAvailable(void);
/**
 * @brief 将消费索引推进到当前生产索引以丢弃缓存。
 * @note 前台调用，不关闭接收中断。
 */
void BSP_UART_RxFlush(void);

#endif /* __BSP_UART_H */
