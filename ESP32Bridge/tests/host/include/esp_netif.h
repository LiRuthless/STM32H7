#pragma once
#include "test_sdk.h"
/* Host-only netif boundary additions for the Phase 04 AP path. */
typedef struct esp_netif { int host_only; } esp_netif_t;
typedef struct { uint32_t addr; } esp_ip4_addr_t;
typedef struct { esp_ip4_addr_t ip, netmask, gw; } esp_netif_ip_info_t;
/* Little-endian host keeps the same byte order as the target constant. */
#define ESP_IP4TOADDR(a, b, c, d) \
    ((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))
esp_err_t esp_netif_dhcps_stop(esp_netif_t *esp_netif);
esp_err_t esp_netif_dhcps_start(esp_netif_t *esp_netif);
esp_err_t esp_netif_set_ip_info(esp_netif_t *esp_netif, const esp_netif_ip_info_t *ip_info);
