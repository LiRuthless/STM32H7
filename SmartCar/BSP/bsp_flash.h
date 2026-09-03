#ifndef __BSP_FLASH_H
#define __BSP_FLASH_H

#include "main.h"

/* 片内 Flash 模拟 EEPROM：使用 Bank2 Sector7（0x081E0000，128KB）。
 * H743 最小擦除单位 128KB 扇区、最小写入单位 32 字节 Flash Word，
 * 实现为「RAM 影子缓冲 + 整扇区读-改-擦-写」，对上层提供按逻辑偏移的随机读写。
 * offset 为扇区内逻辑偏移（0 起），掉电只丢失未 Write 的影子修改。 */
#define BSP_FLASH_BASE_ADDR     0x081E0000u
#define BSP_FLASH_SECTOR_SIZE   (128u * 1024u)

void    BSP_Flash_Init(void);                                    /* 加载扇区到影子缓冲 */
uint8_t BSP_Flash_Read(uint32_t offset, uint8_t *buf, uint32_t len);        /* 0=成功 */
uint8_t BSP_Flash_Write(uint32_t offset, const uint8_t *buf, uint32_t len); /* 0=成功（含擦除+回写） */

#endif /* __BSP_FLASH_H */
