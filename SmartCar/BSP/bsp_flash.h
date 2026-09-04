#ifndef __BSP_FLASH_H
#define __BSP_FLASH_H

#include "main.h"
#include "bsp_w25q64.h"

/* 参数/日志存储服务：后端为板载 SPI Flash W25Q64（8MB，SPI1），不使用片内 Flash。
 * 布局：日志区 0x000000 ~ 0x7FEFFF（8MB 减去末尾 4KB），
 *       参数区 0x7FF000 起最后一个 4KB 扇区。
 * 参数：4KB 扇区读-改-擦-写（约几十 ms，无需关中断）。
 * 日志：「边写边擦」——LogErase 只擦首个 4KB 扇区（起跑前几乎无延时），
 *       之后 LogWrite 每跨入一个新 4KB 扇区自动先擦除（几十 ms）。 */

#define BSP_FLASH_PARAM_SECTOR_SIZE   4096u                       /* 参数逻辑区大小 */

void    BSP_Flash_Init(void);                                    /* 初始化 W25Q64 */
uint8_t BSP_Flash_Read(uint32_t offset, uint8_t *buf, uint32_t len);        /* 参数区读，0=成功 */
uint8_t BSP_Flash_Write(uint32_t offset, const uint8_t *buf, uint32_t len); /* 参数区写（读-改-擦-写），0=成功 */

/* 日志区（追加写模式） */
#define BSP_FLASH_LOG_SIZE      (W25Q64_TOTAL_SIZE - W25Q64_SECTOR_SIZE)    /* 8MB-4KB */

uint8_t BSP_Flash_LogErase(void);        /* 复位日志：擦首个扇区+重置擦除前沿（约几十 ms），0=成功 */
uint8_t BSP_Flash_LogWrite(uint32_t offset, const uint8_t *buf, uint32_t len); /* 32B 对齐追加写（自动跨扇区擦除），0=成功 */
uint8_t BSP_Flash_LogRead(uint32_t offset, uint8_t *buf, uint32_t len);          /* 0=成功 */

#endif /* __BSP_FLASH_H */
