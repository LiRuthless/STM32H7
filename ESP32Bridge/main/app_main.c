#include <stddef.h>

#include "esp_log.h"
#include "sdkconfig.h"
#include "startup.h"
#include "buffer_pool.h"

#if !CONFIG_IDF_TARGET_ESP32S3
#error "ESP32Bridge requires esp32s3"
#endif
#if defined(CONFIG_SPIRAM) || defined(CONFIG_FREERTOS_UNICORE)
#error "ESP32Bridge requires internal memory and both CPU cores"
#endif

/* Missing service callbacks are reported honestly until their phases land. */
void app_main(void)
{
    static const char tag[] = "foundation";
    static const char *const steps[] = { "NVS", "NETIF", "EVENT_LOOP", "AP", "SOCKETS", "SPI" };
    static const char *const states[] = { "NOT_ATTEMPTED", "SUCCEEDED", "FAILED", "BLOCKED", "UNIMPLEMENTED" };
    bridge_startup_report_t report;
    const esp_err_t result = bridge_startup_idf_run(&report);
    ESP_LOGI(tag, "Startup result=%s READY_guard=%s password_check=%s pool=%s",
             esp_err_to_name(result), esp_err_to_name(report.ready_error),
             esp_err_to_name(report.password_error), esp_err_to_name(report.pool_error));
    for (int i = 0; i < BRIDGE_START_COUNT; ++i) {
        ESP_LOGI(tag, "Stage=%s state=%s error=%s dependency=%d", steps[i], states[report.steps[i].state],
                 esp_err_to_name(report.steps[i].error), report.steps[i].dependency);
    }
    bridge_pool_snapshot_t pool;
    if (bridge_pool_snapshot(&pool) == ESP_OK) {
        ESP_LOGI(tag, "Pool data=%u metadata=%u UDP=%u EVENT=%u DMA_owner=%d",
                 (unsigned)pool.allocated_bytes, (unsigned)pool.metadata_bytes,
                 (unsigned)pool.capacity[BRIDGE_POOL_UDP], (unsigned)pool.capacity[BRIDGE_POOL_EVENT], pool.dma_owner);
    }
    ESP_LOGW(tag, "AP/socket/SPI services await their phases; no bridge traffic is enabled");
}
