/**
 * @file esp_wifi_default.h
 * @brief 声明静态AP接口创建与销毁边界。
 * @details 仅为tests/host自有夹具提供esp_wifi_default.h所需的声明、固定常量或内联替身。
 *     实现由宿主夹具或本头内联提供，不链接真实SDK；不证明DMA、调度或硬件能力。
 */
#pragma once
#include "test_sdk.h"
#include "esp_netif.h"
esp_netif_t *esp_netif_create_default_wifi_ap(void);
void esp_netif_destroy_default_wifi(void *esp_netif);
