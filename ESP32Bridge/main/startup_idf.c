#include "startup.h"
#include "buffer_pool.h"
#include <stdint.h>
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_memory_utils.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

static portMUX_TYPE s_pool_mux = portMUX_INITIALIZER_UNLOCKED;
static void pool_lock(void) { portENTER_CRITICAL(&s_pool_mux); }
static void pool_unlock(void) { portEXIT_CRITICAL(&s_pool_mux); }

static void *pool_alloc(size_t alignment, size_t bytes, bool dma)
{
    uint32_t caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
    if (dma) {
        caps |= MALLOC_CAP_DMA;
    }
    unsigned char *ptr = heap_caps_aligned_alloc(alignment, bytes, caps);
    if (ptr != NULL && (!esp_ptr_internal(ptr) || !esp_ptr_internal(ptr + bytes - 1) ||
        (dma && (!esp_ptr_dma_capable(ptr) || !esp_ptr_dma_capable(ptr + bytes - 1))))) {
        heap_caps_free(ptr);
        return NULL;
    }
    return ptr;
}

static esp_err_t init_nvs(void *context)
{
    (void)context;
    /* Do not erase Flash or reboot to conceal an initialization error. */
    return nvs_flash_init();
}
static esp_err_t init_netif(void *context)
{
    (void)context;
    return esp_netif_init();
}
static esp_err_t init_event_loop(void *context)
{
    (void)context;
    return esp_event_loop_create_default();
}

esp_err_t bridge_startup_idf_run(bridge_startup_report_t *report)
{
    if (report == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    /* Set latch before enabling the pad. Real waveforms remain unmeasured. */
    esp_err_t ready_error = gpio_set_level(GPIO_NUM_14, 1);
    if (ready_error == ESP_OK) {
        const gpio_config_t ready = {
            .pin_bit_mask = UINT64_C(1) << GPIO_NUM_14, .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE, .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        ready_error = gpio_config(&ready);
    }
    static const unsigned char password[] = CONFIG_BRIDGE_AP_PASSWORD;
    const esp_err_t password_error = bridge_password_validate(password, sizeof(password) - 1);
    if (password_error != ESP_OK) {
        ESP_LOGE("startup", "Invalid local AP configuration; AP/socket startup blocked");
    }
    const bridge_pool_ops_t memory_ops = { pool_alloc, heap_caps_free, pool_lock, pool_unlock };
    const size_t free_before = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    const esp_err_t pool_error = bridge_pool_init(&memory_ops);
    ESP_LOGI("startup", "Pool init=%s; internal free before=%u after=%u minimum=%u",
             esp_err_to_name(pool_error), (unsigned)free_before,
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    const bridge_startup_ops_t ops = {
        .nvs = init_nvs, .netif = init_netif, .event_loop = init_event_loop,
        .ap = NULL, .sockets = NULL, .spi = NULL,
        .ready_error = ready_error, .password_error = password_error, .pool_error = pool_error
    };
    return bridge_startup_run(&ops, report);
}
