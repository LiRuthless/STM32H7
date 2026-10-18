/**
 * @file bsp_wifi.h
 * @brief SPI2 Wi-Fi 非阻塞事务接口。
 * @details 暴露静态DMA事务缓冲和复制状态；实现及IRQ所有权见 bsp_wifi.c。
 */
#ifndef SMARTCAR_BSP_BSP_WIFI_H
#define SMARTCAR_BSP_BSP_WIFI_H
#include <stdint.h>

#define BSP_WIFI_TRANSACTION_SIZE 1024u

typedef struct
{
    uint32_t permits; /**< READY累计许可，IRQ发布。 */
    uint32_t consumed; /**< 前台消费位置。 */
    uint32_t completed; /**< 成功事务累计数。 */
    uint32_t errors; /**< 启动或传输错误累计数。 */
    uint32_t timeouts; /**< 达到10ms的中止数。 */
    uint8_t state; /**< 0空闲/1在途/2待读取/3错误，BSP独占。 */
} bsp_wifi_status_t;

/**
 * @brief 初始化片选与READY许可。
 * @note 完整契约见 bsp_wifi.c::BSP_WiFi_Init。
 */
void BSP_WiFi_Init(void);
/**
 * @brief 取得空闲期可写TX；在途不得写。
 * @note 完整契约见 bsp_wifi.c::BSP_WiFi_GetTx。
 */
uint8_t *BSP_WiFi_GetTx(void);
/**
 * @brief 成功完成后取得RX，否则NULL。
 * @note 完整契约见 bsp_wifi.c::BSP_WiFi_GetRx。
 */
const uint8_t *BSP_WiFi_GetRx(void);
/**
 * @brief 释放已完成RX并允许下一事务。
 * @note 完整契约见 bsp_wifi.c::BSP_WiFi_ReleaseRx。
 */
void BSP_WiFi_ReleaseRx(void);
/**
 * @brief 有许可且空闲时启动1024B DMA，返回1启动/0忙/2错。
 * @note 完整契约见 bsp_wifi.c::BSP_WiFi_TryExchange。
 */
uint8_t BSP_WiFi_TryExchange(void);
/**
 * @brief 前台处理完成、错误及10ms中止。
 * @note 完整契约见 bsp_wifi.c::BSP_WiFi_Service。
 */
void BSP_WiFi_Service(void);
/**
 * @brief 复制事务状态到调用方。
 * @note 完整契约见 bsp_wifi.c::BSP_WiFi_GetStatus。
 */
void BSP_WiFi_GetStatus(bsp_wifi_status_t *status);
/**
 * @brief 空闲时显式选择分频64/256，返回1成功。
 * @note 完整契约见 bsp_wifi.c::BSP_WiFi_SetFastClock。
 */
uint8_t BSP_WiFi_SetFastClock(uint8_t fast);

#endif
