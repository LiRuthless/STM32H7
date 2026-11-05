#include "startup.h"
#include <stdbool.h>

esp_err_t bridge_password_validate(const unsigned char *data, size_t length)
{
    if (data == NULL || length < 8 || length > 63) {
        return ESP_ERR_INVALID_ARG;
    }
    for (size_t i = 0; i < length; ++i) {
        if (data[i] == 0 || data[i] > 127) {
            return ESP_ERR_INVALID_ARG;
        }
    }
    return ESP_OK;
}

esp_err_t bridge_startup_run(const bridge_startup_ops_t *ops, bridge_startup_report_t *report)
{
    if (ops == NULL || report == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    *report = (bridge_startup_report_t){
        .ready_error = ops->ready_error, .password_error = ops->password_error, .pool_error = ops->pool_error
    };
    const bridge_start_step_fn callbacks[BRIDGE_START_COUNT] = {
        ops->nvs, ops->netif, ops->event_loop, ops->ap, ops->sockets, ops->spi
    };
    const int dependencies[BRIDGE_START_COUNT] = {
        BRIDGE_DEP_NONE, BRIDGE_START_NVS, BRIDGE_START_NETIF,
        BRIDGE_START_EVENT_LOOP, BRIDGE_START_AP, BRIDGE_DEP_NONE
    };
    esp_err_t first_error = ops->ready_error != ESP_OK ? ops->ready_error : ops->password_error;
    bool incomplete = false;
    for (int i = 0; i < BRIDGE_START_COUNT; ++i) {
        bridge_start_result_t *step = &report->steps[i];
        step->dependency = BRIDGE_DEP_NONE;
        if (ops->pool_error != ESP_OK) {
            *step = (bridge_start_result_t){ BRIDGE_BLOCKED, ops->pool_error, BRIDGE_DEP_POOL };
        } else if (dependencies[i] >= 0 && report->steps[dependencies[i]].state != BRIDGE_SUCCEEDED) {
            *step = (bridge_start_result_t){ BRIDGE_BLOCKED, report->steps[dependencies[i]].error, dependencies[i] };
        } else if (i == BRIDGE_START_AP && ops->password_error != ESP_OK) {
            *step = (bridge_start_result_t){ BRIDGE_BLOCKED, ops->password_error, BRIDGE_DEP_PASSWORD };
        } else if (i == BRIDGE_START_SPI && ops->ready_error != ESP_OK) {
            *step = (bridge_start_result_t){ BRIDGE_BLOCKED, ops->ready_error, BRIDGE_DEP_READY };
        } else if (callbacks[i] == NULL) {
            *step = (bridge_start_result_t){ BRIDGE_UNIMPLEMENTED, ESP_ERR_NOT_SUPPORTED, BRIDGE_DEP_NONE };
        } else {
            step->error = callbacks[i](ops->context);
            step->state = step->error == ESP_OK ? BRIDGE_SUCCEEDED : BRIDGE_FAILED;
            if (first_error == ESP_OK && step->error != ESP_OK) {
                first_error = step->error;
            }
        }
        incomplete = incomplete || step->state != BRIDGE_SUCCEEDED;
    }
    if (ops->pool_error != ESP_OK) {
        return first_error != ESP_OK ? first_error : ops->pool_error;
    }
    return first_error != ESP_OK ? first_error : (incomplete ? ESP_ERR_NOT_SUPPORTED : ESP_OK);
}
