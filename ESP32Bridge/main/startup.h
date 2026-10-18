#pragma once
#include <stddef.h>
#include "esp_err.h"

typedef enum {
    BRIDGE_START_NVS, BRIDGE_START_NETIF, BRIDGE_START_EVENT_LOOP,
    BRIDGE_START_AP, BRIDGE_START_SOCKETS, BRIDGE_START_SPI, BRIDGE_START_COUNT
} bridge_start_step_t;
typedef enum {
    BRIDGE_NOT_ATTEMPTED, BRIDGE_SUCCEEDED, BRIDGE_FAILED, BRIDGE_BLOCKED, BRIDGE_UNIMPLEMENTED
} bridge_start_state_t;
enum { BRIDGE_DEP_NONE = -1, BRIDGE_DEP_POOL = -2, BRIDGE_DEP_PASSWORD = -3, BRIDGE_DEP_READY = -4 };
typedef struct {
    bridge_start_state_t state;
    esp_err_t error;
    int dependency;
} bridge_start_result_t;
typedef esp_err_t (*bridge_start_step_fn)(void *context);
typedef struct {
    void *context;
    bridge_start_step_fn nvs, netif, event_loop, ap, sockets, spi;
    esp_err_t ready_error, password_error, pool_error;
} bridge_startup_ops_t;
typedef struct {
    bridge_start_result_t steps[BRIDGE_START_COUNT];
    esp_err_t ready_error, password_error, pool_error;
} bridge_startup_report_t;

esp_err_t bridge_password_validate(const unsigned char *data, size_t length);
esp_err_t bridge_startup_run(const bridge_startup_ops_t *ops, bridge_startup_report_t *report);
/* READY protection, credential check, pool and actual SDK NVS/netif/event/SPI
 * initialization. SPI transaction service and AP/socket land in later groups. */
esp_err_t bridge_startup_idf_run(bridge_startup_report_t *report);
