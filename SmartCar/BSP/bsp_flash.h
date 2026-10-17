/**
 * @file bsp_flash.h
 * @brief BSP 参数和日志 Flash 服务；前台串行访问 W25Q64。
 * @details 声明参数区与日志区服务接口；完整地址、擦写和失败语义位于 bsp_flash.c。
 */
#ifndef SMARTCAR_BSP_BSP_FLASH_H
#define SMARTCAR_BSP_BSP_FLASH_H

#include "main.h"
#include "bsp_w25q64.h"

/* 参数/日志存储服务：后端为板载 SPI Flash W25Q64（8MB，SPI1），不使用片内 Flash。
 * 布局：日志区 0x000000 ~ 0x7FEFFF（8MB 减去末尾 4KB），
 *       参数区 0x7FF000 起最后一个 4KB 扇区。
 * 参数：4KB 扇区读-改-擦-写（约几十 ms，无需关中断）。
 * 日志：「边写边擦」——LogErase 只擦首个 4KB 扇区（起跑前几乎无延时），
 *       之后 LogWrite 每跨入一个新 4KB 扇区自动先擦除（几十 ms）。 */

#define BSP_FLASH_PARAM_SECTOR_SIZE 4096u /* 参数逻辑区大小 */
#define BSP_FLASH_LOG_SIZE (BSP_W25Q64_TOTAL_SIZE - BSP_W25Q64_SECTOR_SIZE) /* 8MB-4KB */

/**
 * @brief 初始化 Flash 后端并复位日志擦除前沿。
 * @note 启动主循环调用；成功状态保存在模块中，后续接口报告不可用。 完整契约见 bsp_flash.c::BSP_Flash_Init。
 */
void BSP_Flash_Init(void);
/**
 * @brief 读取末尾参数扇区。
 * @note offset 字节偏移；范围 0～4095。 len 非零字节数，整个范围必须落在目标区域。 前台串行阻塞调用；不得在 ISR 执行 Flash 等待。 完整契约见
 *     bsp_flash.c::BSP_Flash_Read。
 */
uint8_t BSP_Flash_Read(uint32_t offset, uint8_t *buf, uint32_t len);
/**
 * @brief 写入末尾参数扇区。
 * @note offset 字节偏移；范围 0～4095。 buf 输入缓冲，必须非空并可读 len 字节。 读入静态影子后比较；相同内容不擦写，失败不回滚已擦写的数据。 完整契约见
 *     bsp_flash.c::BSP_Flash_Write。
 */
uint8_t BSP_Flash_Write(uint32_t offset, const uint8_t *buf, uint32_t len);

/* 日志区（追加写模式） */

/**
 * @brief 擦除日志首扇区并在成功后重置擦除前沿。
 * @note 起跑前主循环阻塞调用，只擦首个 4 KiB 扇区。 完整契约见 bsp_flash.c::BSP_Flash_LogErase。
 */
uint8_t BSP_Flash_LogErase(void);
/**
 * @brief 写入日志存储区。
 * @note buf 输入缓冲，必须非空并可读 len 字节。 len 非零字节数，整个范围必须落在目标区域。 完整契约见 bsp_flash.c::BSP_Flash_LogWrite。
 */
uint8_t BSP_Flash_LogWrite(uint32_t offset, const uint8_t *buf, uint32_t len);
/**
 * @brief 读取日志存储区。
 * @note len 非零字节数，整个范围必须落在目标区域。 buf 输出缓冲，必须非空且可写 len 字节；底层传输失败可能已部分写入。 前台串行阻塞调用；不得在 ISR 执行 Flash
 *     等待。 完整契约见 bsp_flash.c::BSP_Flash_LogRead。
 */
uint8_t BSP_Flash_LogRead(uint32_t offset, uint8_t *buf, uint32_t len);

#endif /* __BSP_FLASH_H */
