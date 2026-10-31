/**
 * @file log_store.c
 * @brief 日志Flash字节存储后端。
 * @details 唯一App日志Flash访问点，将前台请求一一转发BSP。
 *     仅持写头回读校验缓冲，不拥有会话、队列、导出或协议状态。
 */
#include "services/log_store.h"
#include "bsp.h"
#include <string.h>

/* 前台写头回读临时副本，不是当前会话头状态。 */
static datalog_header_t s_check;

/**
 * @brief 擦除日志首扇区并复位底层懒擦前沿。
 * @return 底层返回值，0成功，非零失败。
 * @note 仅前台调用，可阻塞；不维护会话、统计或故障。
 */
uint8_t LogStore_Erase(void)
{
    return BSP_Flash_LogErase();
}

/**
 * @brief 写头后回读并逐字节核对。
 * @details 先写offset0的256字节，再读入唯一静态校验缓冲并比较。
 *     写、读、比较沿用短路次序，任一步失败即返回，不重试或修改会话状态。
 * @param[in] header 非空256字节会话头，整个调用期间可读。
 * @return 全部成功0；任一步失败1。
 * @note 仅起跑前台调用，可阻塞，不占256B栈。
 */
uint8_t LogStore_WriteHeader(const datalog_header_t *header)
{
    if (BSP_Flash_LogWrite(0, (const uint8_t *)header, 256)
        || BSP_Flash_LogRead(0, (uint8_t *)&s_check, 256)
        || memcmp(&s_check, header, 256))
    {
        return 1;
    }
    return 0;
}

/**
 * @brief 按字节写入日志区。
 * @param[in] offset 日志区内绝对字节偏移。
 * @param[in] data 非空且至少length字节；底层失败可能已经处理部分字节。
 * @param[in] length 写入字节数，范围由BSP检查。
 * @return 底层返回值，0成功，非零失败。
 * @note 仅前台调用，可阻塞；不缓存、重试或喂狗，不修改日志队列。
 */
uint8_t LogStore_Write(uint32_t offset, const uint8_t *data, uint32_t length)
{
    return BSP_Flash_LogWrite(offset, data, length);
}

/**
 * @brief 按字节读取日志区。
 * @param[in] offset 日志区内绝对字节偏移。
 * @param[out] data 非空且至少length字节；底层失败可能已经处理部分字节。
 * @param[in] length 读取字节数，范围由BSP检查。
 * @return 底层返回值，0成功，非零失败。
 * @note 仅前台调用，可阻塞；不缓存、重试或喂狗，不修改日志队列。
 */
uint8_t LogStore_Read(uint32_t offset, uint8_t *data, uint32_t length)
{
    return BSP_Flash_LogRead(offset, data, length);
}
