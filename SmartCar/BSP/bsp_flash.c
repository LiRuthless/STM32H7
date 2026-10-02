/**
  ******************************************************************************
  * @file    bsp_flash.c
  * @brief   参数/日志存储服务，后端为板载 W25Q64（8MB，SPI1）。片内 Flash 不再使用。
  *          参数区：芯片末尾 4KB 扇区（0x7FF000），读-改-擦-写。
  *          日志区：0x000000 起约 8MB，「边写边擦」追加写（LogWrite 跨入新 4KB
  *          扇区时自动先擦除），无需起跑前整片擦除。
  ******************************************************************************
  */

/* 包含头文件 ------------------------------------------------------------------*/
#include "bsp_flash.h"
#include "bsp_w25q64.h"
#include <string.h>

/* 私有定义 -----------------------------------------------------------*/
#define PARAM_ADDR          (W25Q64_TOTAL_SIZE - W25Q64_SECTOR_SIZE)  /* 0x7FF000 */
#define FLASH_WORD_SIZE     32u                                       /* 日志记录粒度（沿用原 32B 约束） */

/* 私有变量 ---------------------------------------------------------*/
static uint8_t  s_ready;                    /* W25Q64 自检通过标志 */
static uint32_t s_log_frontier;             /* 日志区已擦除到的偏移（边写边擦前沿） */
static uint8_t  s_param_shadow[W25Q64_SECTOR_SIZE];   /* 参数扇区影子缓冲（4KB） */

/* 导出函数定义 -------------------------------------------*/

/**
  * @brief  初始化 W25Q64（JEDEC ID 自检），失败时后续读写全部返回错误
  */
void BSP_Flash_Init(void)
{
  s_ready = (BSP_W25Q64_Init() == 0u) ? 1u : 0u;
  s_log_frontier = 0;
}

/* ==================== 参数区（末尾 4KB 扇区，读-改-擦-写） ==================== */

/**
  * @brief  参数区按逻辑偏移读取
  * @retval 0=成功，1=失败/越界
  */
uint8_t BSP_Flash_Read(uint32_t offset, uint8_t *buf, uint32_t len)
{
  if ((0u == s_ready) || (0 == buf) || (0u == len) ||
      (offset >= W25Q64_SECTOR_SIZE) ||
      (len > (W25Q64_SECTOR_SIZE - offset)))
  {
    return 1;
  }
  return BSP_W25Q64_Read(PARAM_ADDR + offset, buf, len);
}

/**
  * @brief  参数区按逻辑偏移写入：4KB 扇区读-改-擦-写（约几十 ms，无需关中断）
  * @retval 0=成功，1=失败/越界
  */
uint8_t BSP_Flash_Write(uint32_t offset, const uint8_t *buf, uint32_t len)
{
  uint32_t addr;

  if ((0u == s_ready) || (0 == buf) || (0u == len) ||
      (offset >= W25Q64_SECTOR_SIZE) ||
      (len > (W25Q64_SECTOR_SIZE - offset)))
  {
    return 1;
  }

  /* 读整扇区，先比较待修改区域，避免额外 4KB 栈缓冲和重复读取 */
  if (BSP_W25Q64_Read(PARAM_ADDR, s_param_shadow, W25Q64_SECTOR_SIZE) != 0u)
  {
    return 1;
  }
  if (memcmp(&s_param_shadow[offset], buf, len) == 0)
  {
    return 0;
  }
  memcpy(&s_param_shadow[offset], buf, len);

  /* 擦除并回写 16 页 */
  if (BSP_W25Q64_EraseSector(PARAM_ADDR) != 0u)
  {
    return 1;
  }
  for (addr = 0; addr < W25Q64_SECTOR_SIZE; addr += W25Q64_PAGE_SIZE)
  {
    if (BSP_W25Q64_WritePage(PARAM_ADDR + addr, &s_param_shadow[addr],
                             W25Q64_PAGE_SIZE) != 0u)
    {
      return 1;
    }
  }
  return 0;
}

/* ==================== 日志区（追加写，边写边擦） ==================== */

/**
  * @brief  复位日志：擦除首个 4KB 扇区并把擦除前沿置到 4KB（约几十 ms）
  * @retval 0=成功，1=失败
  */
uint8_t BSP_Flash_LogErase(void)
{
  if (0u == s_ready)
  {
    return 1;
  }
  if (BSP_W25Q64_EraseSector(0) != 0u)
  {
    return 1;
  }
  s_log_frontier = W25Q64_SECTOR_SIZE;
  return 0;
}

/**
  * @brief  日志追加写：跨入未擦除的 4KB 扇区时自动先擦除（几十 ms），
  *         写操作按 256B 页拆分（页编程典型 0.7ms）
  * @param  offset 日志区字节偏移，必须 32 对齐
  * @param  len    必须 32 的整数倍
  * @retval 0=成功，1=失败/越界
  */
uint8_t BSP_Flash_LogWrite(uint32_t offset, const uint8_t *buf, uint32_t len)
{
  if ((0u == s_ready) || (0 == buf) || (0u == len) ||
      (0u != (offset % FLASH_WORD_SIZE)) ||
      (0u != (len % FLASH_WORD_SIZE)) ||
      (offset >= BSP_FLASH_LOG_SIZE) ||
      (len > (BSP_FLASH_LOG_SIZE - offset)))
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
    s_log_frontier += W25Q64_SECTOR_SIZE;
  }

  /* 按 256B 页拆分编程（页内可任意 32B 对齐写入，不跨页） */
  while (len > 0u)
  {
    uint16_t chunk = (uint16_t)(W25Q64_PAGE_SIZE - (offset % W25Q64_PAGE_SIZE));
    if (chunk > len)
    {
      chunk = (uint16_t)len;
    }
    if (BSP_W25Q64_WritePage(offset, buf, chunk) != 0u)
    {
      return 1;
    }
    offset += chunk;
    buf    += chunk;
    len    -= chunk;
  }
  return 0;
}

/**
  * @brief  日志区读取
  * @retval 0=成功，1=失败/越界
  */
uint8_t BSP_Flash_LogRead(uint32_t offset, uint8_t *buf, uint32_t len)
{
  if ((0u == s_ready) || (0 == buf) || (0u == len) ||
      (offset >= BSP_FLASH_LOG_SIZE) ||
      (len > (BSP_FLASH_LOG_SIZE - offset)))
  {
    return 1;
  }
  return BSP_W25Q64_Read(offset, buf, len);
}
