/**
 * @file bsp_flash.c
 * @brief 参数/日志存储服务，后端为板载 W25Q64（8MB，SPI1）。
 * @details 在 W25Q64 上划分参数和日志区域，封装跨页写入、擦除、会话扫描及容量边界。
 */
#include "bsp_flash.h"
#include "bsp_w25q64.h"
#include <string.h>
#define BSP_FLASH_PARAM_ADDR (BSP_W25Q64_TOTAL_SIZE - BSP_W25Q64_SECTOR_SIZE) /* 0x7FF000 */
#define BSP_FLASH_WORD_SIZE 32u /* 日志记录粒度（沿用原 32B 约束） */
/* Flash 可用标志，初始化前台写、其余前台接口只读；0 不可用，1 可用。 */
static uint8_t s_ready;
/* 日志区边写边擦前沿，单位字节、范围 0..BSP_FLASH_LOG_SIZE，仅前台读写。 */
static uint32_t s_log_frontier;
/* 参数末扇区 4 KiB 读改写影子，仅前台参数写接口串行使用。 */
static uint8_t s_param_shadow[BSP_W25Q64_SECTOR_SIZE];
/**
 * @brief 初始化 Flash 后端并复位日志擦除前沿。
 * @details 按现有调用顺序使用 BSP_W25Q64_Init 完成本函数处理。
 * @note 启动主循环调用；成功状态保存在模块中，后续接口报告不可用。
 */
void BSP_Flash_Init(void)
{
    s_ready = (BSP_W25Q64_Init() == 0u) ? 1u : 0u;
    s_log_frontier = 0;
}
/* 参数区位于末尾 4 KiB 扇区，更新采用读-改-擦-写。 */

/**
 * @brief 读取末尾参数扇区。
 * @details 按现有调用顺序使用 BSP_W25Q64_Read 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @param[in] offset 字节偏移；范围 0～4095。
 * @param[out] buf 输出缓冲，必须非空且可写 len 字节；底层传输失败可能已部分写入。
 * @param[in] len 非零字节数，整个范围必须落在目标区域。
 * @return 0 成功；1 参数不合法、后端未就绪或 SPI/等待失败。
 * @note 前台串行阻塞调用；不得在 ISR 执行 Flash 等待。
 */
uint8_t BSP_Flash_Read(uint32_t offset, uint8_t *buf, uint32_t len)
{
    if ((0u == s_ready) || (0 == buf) || (0u == len) || (offset >= BSP_W25Q64_SECTOR_SIZE)
        || (len > (BSP_W25Q64_SECTOR_SIZE - offset)))
    {
        return 1;
    }
    return BSP_W25Q64_Read(BSP_FLASH_PARAM_ADDR + offset, buf, len);
}

/**
 * @brief 写入末尾参数扇区。
 * @details 按现有调用顺序使用 BSP_W25Q64_Read、memcmp、memcpy、BSP_W25Q64_EraseSector
 *     完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @param[in] offset 字节偏移；范围 0～4095。
 * @param[in] buf 输入缓冲，必须非空并可读 len 字节。
 * @param[in] len 非零字节数，整个范围必须落在目标区域。
 * @return 0 成功；1 参数不合法、后端未就绪或 SPI/等待失败。
 * @note 读入静态影子后比较；相同内容不擦写，失败不回滚已擦写的数据。
 */
uint8_t BSP_Flash_Write(uint32_t offset, const uint8_t *buf, uint32_t len)
{
    uint32_t addr;

    if ((0u == s_ready) || (0 == buf) || (0u == len) || (offset >= BSP_W25Q64_SECTOR_SIZE)
        || (len > (BSP_W25Q64_SECTOR_SIZE - offset)))
    {
        return 1;
    }

    /* 读整扇区，先比较待修改区域，避免额外 4KB 栈缓冲和重复读取 */
    if (BSP_W25Q64_Read(BSP_FLASH_PARAM_ADDR, s_param_shadow, BSP_W25Q64_SECTOR_SIZE) != 0u)
    {
        return 1;
    }
    if (memcmp(&s_param_shadow[offset], buf, len) == 0)
    {
        return 0;
    }
    memcpy(&s_param_shadow[offset], buf, len);

    /* 擦除并回写 16 页 */
    if (BSP_W25Q64_EraseSector(BSP_FLASH_PARAM_ADDR) != 0u)
    {
        return 1;
    }
    for (addr = 0; addr < BSP_W25Q64_SECTOR_SIZE; addr += BSP_W25Q64_PAGE_SIZE)
    {
        if (BSP_W25Q64_WritePage(
                BSP_FLASH_PARAM_ADDR + addr, &s_param_shadow[addr], BSP_W25Q64_PAGE_SIZE)
            != 0u)
        {
            return 1;
        }
    }
    return 0;
}
/* 日志区从零地址追加写入，并按写入前沿预先擦除。 */

/**
 * @brief 擦除日志首扇区并在成功后重置擦除前沿。
 * @details 按现有调用顺序使用 BSP_W25Q64_EraseSector 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @return 0 成功；1 后端不可用或擦除失败。
 * @note 起跑前主循环阻塞调用，只擦首个 4 KiB 扇区。
 */
uint8_t BSP_Flash_LogErase(void)
{
    /* 先检查后端就绪，再从日志首地址按 64 KiB 块顺序擦除全区；任一块失败立即返回且不更新会话前沿。 */
    if (0u == s_ready)
    {
        return 1;
    }
    if (BSP_W25Q64_EraseSector(0) != 0u)
    {
        return 1;
    }
    s_log_frontier = BSP_W25Q64_SECTOR_SIZE;
    return 0;
}

/**
 * @brief 写入日志存储区。
 * @details 检查日志区相对范围后，把请求拆分为不跨 W25Q64 页边界的片段顺序写入；任一页失败立即返回并保留此前已写入的数据。
 * @param[in] offset 字节偏移；范围限于日志区。
 * @param[in] buf 输入缓冲，必须非空并可读 len 字节。
 * @param[in] len 非零字节数，整个范围必须落在目标区域。
 * @return 0 成功；1 参数不合法、后端未就绪或 SPI/等待失败。
 * @pre 偏移和长度须为 32 字节倍数；按既有前沿追加，跨扇区先擦再写。
 * @note 仅由主循环日志消费者调用，可因擦除、页编程和 WIP 等待而阻塞；不在 ISR 调用。
 */
uint8_t BSP_Flash_LogWrite(uint32_t offset, const uint8_t *buf, uint32_t len)
{
    if ((0u == s_ready) || (0 == buf) || (0u == len) || (0u != (offset % BSP_FLASH_WORD_SIZE))
        || (0u != (len % BSP_FLASH_WORD_SIZE)) || (offset >= BSP_FLASH_LOG_SIZE)
        || (len > (BSP_FLASH_LOG_SIZE - offset)))
    {
        return 1;
    }

    /* 边写边擦：保证 [offset, offset+len) 落在已擦除区域内 */
    while (offset + len > s_log_frontier)
    {
        if (BSP_W25Q64_EraseSector(s_log_frontier) != 0u)
        {
            return 1;
        }
        s_log_frontier += BSP_W25Q64_SECTOR_SIZE;
    }

    /* 按 256B 页拆分编程（页内可任意 32B 对齐写入，不跨页） */
    while (len > 0u)
    {
        uint16_t chunk = (uint16_t)(BSP_W25Q64_PAGE_SIZE - (offset % BSP_W25Q64_PAGE_SIZE));
        if (chunk > len)
        {
            chunk = (uint16_t)len;
        }
        if (BSP_W25Q64_WritePage(offset, buf, chunk) != 0u)
        {
            return 1;
        }
        offset += chunk;
        buf += chunk;
        len -= chunk;
    }
    return 0;
}

/**
 * @brief 读取日志存储区。
 * @details 按现有调用顺序使用 BSP_W25Q64_Read 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @param[in] offset 字节偏移；范围限于日志区。
 * @param[out] buf 输出缓冲，必须非空且可写 len 字节；底层传输失败可能已部分写入。
 * @param[in] len 非零字节数，整个范围必须落在目标区域。
 * @return 0 成功；1 参数不合法、后端未就绪或 SPI/等待失败。
 * @note 前台串行阻塞调用；不得在 ISR 执行 Flash 等待。
 */
uint8_t BSP_Flash_LogRead(uint32_t offset, uint8_t *buf, uint32_t len)
{
    if ((0u == s_ready) || (0 == buf) || (0u == len) || (offset >= BSP_FLASH_LOG_SIZE)
        || (len > (BSP_FLASH_LOG_SIZE - offset)))
    {
        return 1;
    }
    return BSP_W25Q64_Read(offset, buf, len);
}
