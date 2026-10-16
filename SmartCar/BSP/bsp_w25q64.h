/**
 * @file bsp_w25q64.h
 * @brief BSP W25Q64 SPI1 驱动；前台执行有界等待的存储读写。
 */
#ifndef SMARTCAR_BSP_BSP_W25Q64_H
#define SMARTCAR_BSP_BSP_W25Q64_H

#include "main.h"

/* 板载 SPI Flash W25Q64（8MB）：SPI1（SCK=PB3、MISO=PB4、MOSI=PD7、CS=PD6，
 * 引脚板载焊死，CubeMX 未配置 SPI1，此处纯代码配置，30MHz，MODE0）。
 * 页编程 256B；擦除粒度 4KB 扇区（约几十 ms，远快于片内 Flash 的 128KB 扇区）。 */
#define BSP_W25Q64_TOTAL_SIZE (8u * 1024u * 1024u)
#define BSP_W25Q64_PAGE_SIZE 256u
#define BSP_W25Q64_SECTOR_SIZE 4096u

/* 以下接口均为 0=成功、1=参数/SPI/超时失败；写入与擦除完成前等待 WIP 清零。 */
/**
 * @brief 初始化 SPI1 并释放掉电模式、检查 JEDEC ID。
 * @return 0 标识为 0xEF4017；1 初始化、传输失败或标识不符。
 * @note 启动主循环阻塞调用；含 HAL_Delay，保持原有 GPIO/SPI 配置。
 */
uint8_t BSP_W25Q64_Init(void);
/**
 * @brief 读取W25Q64 数据。
 * @param[in] addr 字节地址；必须小于 8 MiB。
 * @param[out] buf 输出缓冲，必须非空且可写 len 字节；底层传输失败可能已部分写入。
 * @param[in] len 非零字节数，整个范围必须落在目标区域。
 * @return 0 成功；1 参数不合法、后端未就绪或 SPI/等待失败。
 * @note 长度仍按原逻辑转为 uint16_t 传给 HAL，不新增长度限制。
 */
uint8_t BSP_W25Q64_Read(uint32_t addr, uint8_t *buf, uint32_t len);
/**
 * @brief 写入W25Q64 数据。
 * @param[in] addr 字节地址；必须小于 8 MiB。
 * @param[in] buf 输入缓冲，必须非空并可读 len 字节。
 * @param[in] len 非零字节数，整个范围必须落在目标区域。
 * @return 0 成功；1 参数不合法、后端未就绪或 SPI/等待失败。
 * @pre 长度最多 256 字节且不能跨页；写后 WIP 等待上限 10 ms。
 */
uint8_t BSP_W25Q64_WritePage(uint32_t addr, const uint8_t *buf, uint16_t len);
/**
 * @brief 擦除4 KiB 扇区并等待 WIP 清零。
 * @param[in] addr 目标所在块的字节地址，小于 8 MiB；低位由芯片指令处理。
 * @return 0 完成；1 地址不合法、SPI 失败或 WIP 轮询超时。
 * @note 仅前台阻塞调用，WIP 等待上限 500 ms。
 */
uint8_t BSP_W25Q64_EraseSector(uint32_t addr);
/**
 * @brief 擦除64 KiB 块并等待 WIP 清零。
 * @param[in] addr 目标所在块的字节地址，小于 8 MiB；低位由芯片指令处理。
 * @return 0 完成；1 地址不合法、SPI 失败或 WIP 轮询超时。
 * @note 仅前台阻塞调用，WIP 等待上限 2500 ms。
 */
uint8_t BSP_W25Q64_EraseBlock64K(uint32_t addr);
/**
 * @brief 擦除整片并等待 WIP 清零。
 * @return 0 完成；1 地址不合法、SPI 失败或 WIP 轮询超时。
 * @note 仅前台阻塞调用，WIP 等待上限 110 s。仅看门狗启动前维护使用，当前运行链不调用。
 */
uint8_t BSP_W25Q64_EraseChip(void);

#endif /* __BSP_W25Q64_H */
