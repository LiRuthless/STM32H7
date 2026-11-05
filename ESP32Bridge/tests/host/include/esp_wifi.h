/**
 * @file esp_wifi.h
 * @brief 保存生产AP配置用于宿主断言，不访问射频硬件。
 * @details 仅为tests/host自有夹具提供esp_wifi.h所需的声明、固定常量或内联替身。
 *     实现由宿主夹具或本头内联提供，不链接真实SDK；不证明DMA、调度或硬件能力。
 */
#pragma once
/* 仅宿主Wi-Fi边界，记录生产设置的字段供断言，不访问无线硬件。 */
#include "test_sdk.h"

typedef struct
{
    int magic; /**< 夹具核对的默认Wi-Fi初始化标记。 */
} wifi_init_config_t;

#define WIFI_INIT_CONFIG_DEFAULT() {0x1F2F3F4F}

typedef enum
{
    WIFI_MODE_NULL = 0,
    WIFI_MODE_STA,
    WIFI_MODE_AP,
    WIFI_MODE_APSTA
} wifi_mode_t;

typedef enum
{
    WIFI_IF_STA = 0,
    WIFI_IF_AP
} wifi_interface_t;

typedef enum
{
    WIFI_AUTH_OPEN = 0,
    WIFI_AUTH_WEP,
    WIFI_AUTH_WPA_PSK,
    WIFI_AUTH_WPA2_PSK
} wifi_auth_mode_t;

typedef struct
{
    bool capable; /**< 保留SDK的PMF可用字段，供配置副本核对。 */
    bool required; /**< 保留SDK的PMF必需字段。 */
} wifi_pmf_config_t;

typedef struct
{
    uint8_t ssid[32]; /**< AP名称字节数组，夹具只记录配置。 */
    uint8_t password[64]; /**< 测试配置密码副本，不属于真实部署密码。 */
    uint8_t ssid_len; /**< SSID有效字节数。 */
    uint8_t channel; /**< 配置的AP信道值，夹具不操作射频。 */
    wifi_auth_mode_t authmode; /**< 配置认证模式，夹具断言WPA2。 */
    uint8_t ssid_hidden; /**< 保留SDK隐藏SSID字段。 */
    uint8_t max_connection; /**< 配置关联容量，不代替控制连接槽。 */
    uint16_t beacon_interval; /**< 保留SDK信标间隔字段，夹具不计真实无线时序。 */
    wifi_pmf_config_t pmf_cfg; /**< 保留SDK的PMF配置子对象。 */
} wifi_ap_config_t;

typedef struct
{
    wifi_ap_config_t ap; /**< 记录的AP配置副本。 */
} wifi_config_t;

esp_err_t esp_wifi_init(const wifi_init_config_t *config);
esp_err_t esp_wifi_deinit(void);
esp_err_t esp_wifi_set_mode(wifi_mode_t mode);
esp_err_t esp_wifi_set_config(wifi_interface_t interface, wifi_config_t *conf);
esp_err_t esp_wifi_start(void);
