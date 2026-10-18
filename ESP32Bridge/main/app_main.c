#include <stddef.h>

#include "esp_log.h"
#include "sdkconfig.h"
#include "startup.h"
#include "buffer_pool.h"
#include "spi_bridge.h"
#include "net_bridge.h"

#if !CONFIG_IDF_TARGET_ESP32S3
#error "ESP32Bridge requires esp32s3"
#endif
#if defined(CONFIG_SPIRAM) || defined(CONFIG_FREERTOS_UNICORE)
#error "ESP32Bridge requires internal memory and both CPU cores"
#endif

/* Startup states and live service snapshots are reported honestly; nothing
 * here fabricates downstream Phase 05/06 results. */
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
    if (report.steps[BRIDGE_START_SPI].state == BRIDGE_SUCCEEDED) {
        bridge_spi_snapshot_t spi;
        if (bridge_spi_snapshot(&spi) == ESP_OK) {
            ESP_LOGI(tag, "SPI service=%s queued=%u completed=%u session=%u link_ready=%d driver_owned=%d stack_min=%u",
                esp_err_to_name(spi.service_error), (unsigned)spi.queued, (unsigned)spi.completed,
                (unsigned)spi.session_id, spi.link_ready, spi.driver_owned, (unsigned)spi.stack_free_min);
        }
    }
    if (report.steps[BRIDGE_START_SOCKETS].state == BRIDGE_SUCCEEDED) {
        bridge_net_snapshot_t net;
        if (bridge_net_snapshot(&net) == ESP_OK) {
            ESP_LOGI(tag, "NET service=%s listening=%d open=%d connected=%u rejected=%u received=%u crc=%u invalid_close=%u sent=%u short=%u stall=%u subs=%u udp=%u/%u/%u backpressure=%u session_close=%u link_fail=%u stack_min=%u",
                esp_err_to_name(net.service_error), net.listening, net.control_open,
                (unsigned)net.connected, (unsigned)net.rejected, (unsigned)net.received,
                (unsigned)net.crc_errors, (unsigned)net.invalid_closes, (unsigned)net.sent_bytes,
                (unsigned)net.short_writes, (unsigned)net.stalls, (unsigned)net.subscriptions,
                (unsigned)net.udp_sent, (unsigned)net.udp_failed, (unsigned)net.udp_dropped,
                (unsigned)net.backpressure, (unsigned)net.session_closes, (unsigned)net.link_failures,
                (unsigned)net.stack_free_min);
        }
    }
    ESP_LOGW(tag, "SPI/network services run when their initialization succeeds; downstream bridge consumers, export and PC tools await Phase 05/06");
}
