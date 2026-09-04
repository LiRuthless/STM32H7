#ifndef __BSP_W25Q64_H
#define __BSP_W25Q64_H

#include "main.h"

/* 板载 SPI Flash W25Q64（8MB）：SPI1（SCK=PB3、MISO=PB4、MOSI=PD7、CS=PD6，
 * 引脚板载焊死，CubeMX 未配置 SPI1，此处纯代码配置，30MHz，MODE0）。
 * 页编程 256B；擦除粒度 4KB 扇区（约几十 ms，远快于片内 Flash 的 128KB 扇区）。 */
#define W25Q64_TOTAL_SIZE       (8u * 1024u * 1024u)
#define W25Q64_PAGE_SIZE        256u
#define W25Q64_SECTOR_SIZE      4096u

uint8_t BSP_W25Q64_Init(void);      /* 0=成功（JEDEC ID 0xEF4017 校验） */
uint8_t BSP_W25Q64_Read(uint32_t addr, uint8_t *buf, uint32_t len);
uint8_t BSP_W25Q64_WritePage(uint32_t addr, const uint8_t *buf, uint16_t len); /* 不跨页，调用方保证 */
uint8_t BSP_W25Q64_EraseSector(uint32_t addr);   /* 4KB 扇区擦除（含等待完成，几十 ms） */
uint8_t BSP_W25Q64_EraseBlock64K(uint32_t addr);
uint8_t BSP_W25Q64_EraseChip(void);

#endif /* __BSP_W25Q64_H */
