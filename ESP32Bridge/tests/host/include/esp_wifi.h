#pragma once
/* Host-only Wi-Fi boundary: records configuration for assertions, never
 * touches radios. Field layout covers exactly what production code sets. */
#include "test_sdk.h"

typedef struct { int magic; } wifi_init_config_t;
#define WIFI_INIT_CONFIG_DEFAULT() { 0x1F2F3F4F }
typedef enum { WIFI_MODE_NULL = 0, WIFI_MODE_STA, WIFI_MODE_AP, WIFI_MODE_APSTA } wifi_mode_t;
typedef enum { WIFI_IF_STA = 0, WIFI_IF_AP } wifi_interface_t;
typedef enum { WIFI_AUTH_OPEN = 0, WIFI_AUTH_WEP, WIFI_AUTH_WPA_PSK, WIFI_AUTH_WPA2_PSK } wifi_auth_mode_t;
typedef struct { bool capable, required; } wifi_pmf_config_t;
typedef struct {
    uint8_t ssid[32];
    uint8_t password[64];
    uint8_t ssid_len;
    uint8_t channel;
    wifi_auth_mode_t authmode;
    uint8_t ssid_hidden;
    uint8_t max_connection;
    uint16_t beacon_interval;
    wifi_pmf_config_t pmf_cfg;
} wifi_ap_config_t;
typedef struct { wifi_ap_config_t ap; } wifi_config_t;

esp_err_t esp_wifi_init(const wifi_init_config_t *config);
esp_err_t esp_wifi_deinit(void);
esp_err_t esp_wifi_set_mode(wifi_mode_t mode);
esp_err_t esp_wifi_set_config(wifi_interface_t interface, wifi_config_t *conf);
esp_err_t esp_wifi_start(void);
