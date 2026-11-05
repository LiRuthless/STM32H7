#pragma once
#include "test_sdk.h"
#include "esp_netif.h"
esp_netif_t *esp_netif_create_default_wifi_ap(void);
void esp_netif_destroy_default_wifi(void *esp_netif);
