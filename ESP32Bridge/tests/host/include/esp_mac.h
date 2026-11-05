/**
 * @file esp_mac.h
 * @brief 提供AP MAC读取固定接口，由网络夹具注入地址。
 * @details 仅为tests/host自有夹具提供esp_mac.h所需的声明、固定常量或内联替身。
 *     实现由宿主夹具或本头内联提供，不链接真实SDK；不证明DMA、调度或硬件能力。
 */
#pragma once
#include "test_sdk.h"

typedef enum
{
    ESP_MAC_WIFI_STA,
    ESP_MAC_WIFI_SOFTAP
} esp_mac_type_t;

esp_err_t esp_read_mac(uint8_t *mac, esp_mac_type_t type);
