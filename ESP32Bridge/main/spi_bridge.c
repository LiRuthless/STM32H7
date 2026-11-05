#include "spi_bridge.h"
#include "buffer_pool.h"
#include "driver/spi_slave.h"
#include "esp_intr_alloc.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "hal/gpio_ll.h"
#include "soc/gpio_struct.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static StackType_t s_stack[BRIDGE_SPI_STACK_BYTES / sizeof(StackType_t)];
static StaticTask_t s_task_storage;
static StaticSemaphore_t s_boot_storage;
static SemaphoreHandle_t s_boot;
static TaskHandle_t s_task;
static bridge_dma_pair_t s_dma;
static spi_slave_transaction_t s_transaction;
static esp_err_t s_init_error;
static bool s_started;
static bridge_spi_port_t s_port;
static bridge_spi_protocol_t s_protocol;
static bridge_spi_snapshot_t s_health = {.init_error = ESP_ERR_INVALID_STATE};
static portMUX_TYPE s_snapshot_mux = portMUX_INITIALIZER_UNLOCKED;
_Static_assert(sizeof(s_stack) == BRIDGE_SPI_STACK_BYTES, "ESP-IDF static stack size in bytes");

static void IRAM_ATTR ready_loaded(spi_slave_transaction_t *transaction)
{
    (void)transaction;
    gpio_ll_set_level(&GPIO, 14, 0);
}
static void IRAM_ATTR ready_finished(spi_slave_transaction_t *transaction)
{
    (void)transaction;
    gpio_ll_set_level(&GPIO, 14, 1);
}
static uint64_t now_ms(void) { return (uint64_t)esp_timer_get_time() / 1000u; }
static uint32_t random_session(void *context) { (void)context; return esp_random(); }

static void publish_health(bridge_spi_snapshot_t *health)
{
    health->session_id = s_protocol.session.session_id;
    health->link_ready = s_protocol.session.phase == SCB_SESSION_READY;
    health->protocol = s_protocol.stats;
    health->stack_free_min = uxTaskGetStackHighWaterMark(NULL);
    portENTER_CRITICAL(&s_snapshot_mux);
    s_health = *health;
    portEXIT_CRITICAL(&s_snapshot_mux);
}

esp_err_t bridge_spi_snapshot(bridge_spi_snapshot_t *snapshot)
{
    if (!snapshot) return ESP_ERR_INVALID_ARG;
    portENTER_CRITICAL(&s_snapshot_mux);
    *snapshot = s_health;
    portEXIT_CRITICAL(&s_snapshot_mux);
    return ESP_OK;
}

esp_err_t bridge_spi_configure(const bridge_spi_port_t *port)
{
    if (s_started) return ESP_ERR_INVALID_STATE;
    if (!bridge_spi_port_valid(port)) return ESP_ERR_INVALID_ARG;
    s_port = port ? *port : (bridge_spi_port_t){0};
    return ESP_OK;
}

static esp_err_t run_transactions(bridge_json_workspace_t *workspace, bridge_spi_snapshot_t *health)
{
    uint64_t last_log = now_ms();
    const uint32_t wait_ticks = pdMS_TO_TICKS(10) ? pdMS_TO_TICKS(10) : 1;
    for (;;) {
        bridge_spi_tx_meta_t sent;
        if (!bridge_spi_protocol_prepare(&s_protocol, now_ms(), s_dma.tx, workspace, &sent)) return ESP_ERR_INVALID_STATE;
        for (size_t i = 0; i < BRIDGE_DMA_BYTES; ++i) s_dma.rx[i] = 0;
        s_transaction.trans_len = 0;
        esp_err_t error = bridge_pool_dma_submit(&s_dma);
        if (error != ESP_OK) return error;
        error = spi_slave_queue_trans(SPI2_HOST, &s_transaction, 0);
        if (error != ESP_OK) {
            /* A non-OK queue result proves THIS descriptor wasn't accepted. */
            const esp_err_t reclaim = bridge_pool_dma_reclaim(&s_dma);
            return reclaim == ESP_OK ? error : reclaim;
        }
        ++health->queued; health->driver_owned = true;
        publish_health(health);
        spi_slave_transaction_t *returned = NULL;
        for (;;) {
            error = spi_slave_get_trans_result(SPI2_HOST, &returned, wait_ticks);
            if (error != ESP_ERR_TIMEOUT) break;
            ++health->wait_timeouts;
            /* No descriptor or DMA access is permitted in this wait branch. */
            bridge_spi_protocol_poll(&s_protocol, now_ms());
            publish_health(health);
            const uint64_t now = now_ms();
            if (now - last_log >= 5000u) {
                ESP_LOGI("spi", "queued=%u completed=%u short=%u crc=%u retry=%u invalid=%u dup=%u backpressure=%u delivered=%u rebuilds=%u tele_drop=%u invalid_out=%u session=%u ready=%d stack_min=%u internal_free=%u internal_min=%u",
                    (unsigned)health->queued, (unsigned)health->completed, (unsigned)health->truncated,
                    (unsigned)health->protocol.crc_errors, (unsigned)health->protocol.retries,
                    (unsigned)health->protocol.invalid, (unsigned)health->protocol.duplicates,
                    (unsigned)health->protocol.backpressure, (unsigned)health->protocol.delivered,
                    (unsigned)health->protocol.rebuilds, (unsigned)health->protocol.telemetry_dropped,
                    (unsigned)health->protocol.invalid_outbound,
                    (unsigned)health->session_id, health->link_ready, (unsigned)health->stack_free_min,
                    (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                    (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
                last_log = now;
            }
        }
        /* An error or alien descriptor is NOT proof of DMA reclamation. */
        if (error != ESP_OK) return error;
        if (returned != &s_transaction) return ESP_ERR_INVALID_STATE;
        error = bridge_pool_dma_reclaim(&s_dma);
        if (error != ESP_OK) return error;
        health->driver_owned = false;
        ++health->completed;
        const bool complete = returned->trans_len == BRIDGE_DMA_BYTES * 8u;
        bridge_spi_protocol_sent(&s_protocol, sent, complete, now_ms());
        if (complete) bridge_spi_protocol_receive(&s_protocol, s_dma.rx, s_dma.tx, workspace, random_session, NULL);
        else if (returned->trans_len < BRIDGE_DMA_BYTES * 8u) ++health->truncated;
        else ++health->overlength;
        publish_health(health);
    }
}

static void spi_task(void *context)
{
    (void)context;
    /* The driver ISR is installed on this same core. Explicit affinity also
     * prevents accidental future default-core changes in the SDK adapter. */
    s_init_error = xPortGetCoreID() == BRIDGE_SPI_CORE ? ESP_OK : ESP_ERR_INVALID_STATE;
    bool acquired = false;
    if (s_init_error == ESP_OK) {
        s_init_error = bridge_pool_dma_acquire(&s_dma);
        acquired = s_init_error == ESP_OK;
    }
    if (s_init_error == ESP_OK) {
        const spi_bus_config_t bus = {
            .mosi_io_num = 11, .miso_io_num = 13, .sclk_io_num = 12,
            .quadwp_io_num = -1, .quadhd_io_num = -1,
            .data4_io_num = -1, .data5_io_num = -1, .data6_io_num = -1, .data7_io_num = -1,
            .max_transfer_sz = BRIDGE_DMA_BYTES, .flags = 0,
            .isr_cpu_id = ESP_INTR_CPU_AFFINITY_1, .intr_flags = ESP_INTR_FLAG_IRAM
        };
        const spi_slave_interface_config_t slave = {
            .spics_io_num = 10, .flags = 0, .queue_size = 1, .mode = 0,
            .post_setup_cb = ready_loaded, .post_trans_cb = ready_finished
        };
        s_transaction = (spi_slave_transaction_t){
            .length = BRIDGE_DMA_BYTES * 8u, .tx_buffer = s_dma.tx, .rx_buffer = s_dma.rx
        };
        s_init_error = spi_slave_initialize(SPI2_HOST, &bus, &slave, SPI_DMA_CH_AUTO);
    }
    if (s_init_error != ESP_OK && acquired) {
        const esp_err_t release_error = bridge_pool_dma_release(&s_dma);
        if (release_error != ESP_OK) ESP_LOGE("spi", "DMA init rollback failed=%s", esp_err_to_name(release_error));
    }
    ESP_LOGI("spi", "Driver init=%s core=%d priority=%u stack_bytes=%u transaction_bits=%u; callbacks publish READY only for queued transactions",
             esp_err_to_name(s_init_error), xPortGetCoreID(), (unsigned)BRIDGE_SPI_PRIORITY,
             (unsigned)sizeof(s_stack), (unsigned)s_transaction.length);
    bridge_spi_snapshot_t health = {.init_error = s_init_error};
    bridge_spi_protocol_init(&s_protocol, &s_port);
    publish_health(&health);
    xSemaphoreGive(s_boot);
    if (s_init_error == ESP_OK) {
        bridge_json_workspace_t workspace;
        health.service_error = run_transactions(&workspace, &health);
        bridge_spi_protocol_lost(&s_protocol);
        gpio_ll_set_level(&GPIO, 14, 1);
        publish_health(&health);
        ESP_LOGE("spi", "Service stopped error=%s driver_owned=%d; buffers retained, no reset",
                 esp_err_to_name(health.service_error), health.driver_owned);
    }
    vTaskDelete(NULL);
}

esp_err_t bridge_spi_start(void *context)
{
    (void)context;
    if (s_started) return ESP_ERR_INVALID_STATE;
    s_started = true;
    s_boot = xSemaphoreCreateBinaryStatic(&s_boot_storage);
    if (!s_boot) {
        portENTER_CRITICAL(&s_snapshot_mux); s_health.init_error = ESP_ERR_NO_MEM; portEXIT_CRITICAL(&s_snapshot_mux);
        return ESP_ERR_NO_MEM;
    }
    s_task = xTaskCreateStaticPinnedToCore(spi_task, "scb_spi", sizeof(s_stack), NULL,
                                           BRIDGE_SPI_PRIORITY, s_stack, &s_task_storage, BRIDGE_SPI_CORE);
    if (!s_task) {
        portENTER_CRITICAL(&s_snapshot_mux); s_health.init_error = ESP_ERR_NO_MEM; portEXIT_CRITICAL(&s_snapshot_mux);
        return ESP_ERR_NO_MEM;
    }
    /* Only initialization waits here. It contains no socket/network dependency.
     * Actual SPI traffic will use a separate driver-owned transaction loop. */
    if (xSemaphoreTake(s_boot, portMAX_DELAY) != pdTRUE) return ESP_ERR_INVALID_STATE;
    return s_init_error;
}
