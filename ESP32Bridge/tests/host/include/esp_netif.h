/**
 * @file esp_netif.h
 * @brief 声明AP地址与DHCP边界，由网络夹具记录和注入错误。
 * @details 仅为tests/host自有夹具提供esp_netif.h所需的声明、固定常量或内联替身。
 *     实现由宿主夹具或本头内联提供，不链接真实SDK；不证明DMA、调度或硬件能力。
 */
#pragma once
#include "test_sdk.h"

/* 仅宿主网络接口替身，支持生产AP初始化路径的参数/步骤核对。 */
typedef struct esp_netif
{
    int host_only; /**< 静态网络接口占位，不代表真实SDK对象。 */
} esp_netif_t;

typedef struct
{
    uint32_t addr; /**< 夹具IPv4字节值，生产按网络序使用。 */
} esp_ip4_addr_t;

typedef struct
{
    esp_ip4_addr_t ip; /**< AP地址配置副本。 */
    esp_ip4_addr_t netmask; /**< 子网掩码配置副本。 */
    esp_ip4_addr_t gw; /**< 网关地址配置副本。 */
} esp_netif_ip_info_t;

/* 小端宿主采用与目标常量相同的字节排列。 */
#define ESP_IP4TOADDR(a, b, c, d)                                                                  \
    ((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))
esp_err_t esp_netif_dhcps_stop(esp_netif_t *esp_netif);
esp_err_t esp_netif_dhcps_start(esp_netif_t *esp_netif);
esp_err_t esp_netif_set_ip_info(esp_netif_t *esp_netif, const esp_netif_ip_info_t *ip_info);
