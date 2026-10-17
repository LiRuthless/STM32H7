/**
 * @file bsp_uart.c
 * @brief USART1(115200) 收发：单字节中断接收 + 256 字节环形缓冲， 阻塞发送，printf 重定向（AC6/Keil）。
 * @details 封装阻塞 UART 收发、字符串输出和运行库字符重定向，供菜单、日志与故障报告使用。
 */
#include "bsp_uart.h"
#include "usart.h"

#include <stdio.h>
#include <string.h>
#define BSP_UART_RX_BUF_SIZE 256u /* 必须为 2 的幂，用于位运算回绕 */
/* USART1 单生产者/单消费者接收状态：ISR 写 head/byte，前台写 tail 并读取缓冲。 */
static uint8_t s_rx_buf[BSP_UART_RX_BUF_SIZE];
static volatile uint16_t s_rx_head;
static volatile uint16_t s_rx_tail;
static uint8_t s_rx_byte;
static void BSP_UART_PushRx(uint8_t ch);
/**
 * @brief 清空 USART1 接收索引并开启单字节接收中断。
 * @details 按现有调用顺序使用 HAL_UART_Receive_IT 完成本函数处理。
 * @note 启动主循环调用，保持原 HAL 返回值忽略行为。
 */
void BSP_UART_Init(void)
{
    s_rx_head = 0;
    s_rx_tail = 0;
    (void)HAL_UART_Receive_IT(&huart1, &s_rx_byte, 1);
}

/**
 * @brief 使用 USART1 阻塞发送字节。
 * @details 检查输入后以阻塞 HAL 发送指定字节数；空指针或零长度直接返回，不修改接收环形缓冲。
 * @param[in] buf 输入字节；为空或 len 为零时直接返回 0。
 * @param[in] len 发送字节数；零长度不调用 HAL。
 * @return HAL 成功时 0；失败时按原 len - TxXferCount 计算返回值。
 * @note 单次 HAL 超时 100 ms；前台使用，不与其他发送交错。
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
 * @brief 发送以零结尾的字符串。
 * @details 按现有调用顺序使用 BSP_UART_Write、strlen 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @param[in] str 字符串；为空时返回 1，否则必须有效且以零结尾。
 * @return 空指针返回 1，否则返回 BSP_UART_Write 的原始结果。
 * @note 前台阻塞发送；长度保持原 uint16_t 转换。
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
 * @brief 从接收环形缓冲消费字节。
 * @details 从当前 tail 起逐字节复制，直到达到请求长度或追上 ISR 发布的 head；每消费一个字节推进一次 tail，空指针返回零。
 * @param[out] buf 接收缓冲，非空且容量至少为 len 字节。
 * @param[in] len 最多消费的字节数；可大于当前可读数量。
 * @return 本次实际读取的字节数，0～len。
 * @note 前台读尾索引；UART ISR 写头索引，缓冲满时 ISR 丢最旧字节。
 */
uint16_t BSP_UART_Read(uint8_t *buf, uint16_t len)
{
    /* 按 tail 到 head 的当前可读范围逐字节复制并推进 tail；达到 len 或缓存暂空即停止。 */
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

/**
 * @brief 查询接收环形缓冲当前可读字节数。
 * @details 读取 UART 接收状态并返回当前是否已有可取字节；不消费接收数据。
 * @return 环形索引差，范围 0～255。
 * @note 前台轮询读取，接收中断更新环形索引；返回值只表示调用瞬间的缓存数量。
 */
uint16_t BSP_UART_RxAvailable(void)
{
    return (uint16_t)((s_rx_head - s_rx_tail) & (BSP_UART_RX_BUF_SIZE - 1u));
}

/**
 * @brief 将消费索引推进到当前生产索引以丢弃缓存。
 * @details 按函数体既定条件和写入顺序更新模块状态；提前返回分支不执行其后的状态发布。
 * @note 前台调用，不关闭接收中断。
 */
void BSP_UART_RxFlush(void)
{
    s_rx_tail = s_rx_head;
}
/**
 * @brief 将 UART ISR 收到的字节发布到环形缓冲，满时丢最旧字节。
 * @details 按函数体既定条件和写入顺序更新模块状态；提前返回分支不执行其后的状态发布。
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
/**
 * @brief USART1 单字节接收完成回调。
 * @details 按现有调用顺序使用 BSP_UART_PushRx、HAL_UART_Receive_IT 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
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
 * @details 按现有调用顺序使用
 *     __HAL_UART_CLEAR_OREFLAG、__HAL_UART_CLEAR_FEFLAG、__HAL_UART_CLEAR_NEFLAG、
 *     __HAL_UART_CLEAR_PEFLAG
 *     完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
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
/* 运行库字符输出重定向到 USART1 阻塞发送。 */

/**
 * @brief 将运行库字符输出重定向到 USART1。
 * @details 按现有调用顺序使用 BSP_UART_Write 完成本函数处理。
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
