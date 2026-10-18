#include "spi_bridge.h"
#include "buffer_pool.h"
#include "driver/spi_slave.h"
#include "esp_intr_alloc.h"
#include "esp_log.h"
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
_Static_assert(sizeof(s_stack) == BRIDGE_SPI_STACK_BYTES, "ESP-IDF static stack size in bytes");

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
            .post_setup_cb = NULL, .post_trans_cb = NULL
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
    ESP_LOGI("spi", "Driver init=%s core=%d priority=%u stack_bytes=%u transaction_bits=%u; READY high, transaction loop awaits Group 2",
             esp_err_to_name(s_init_error), xPortGetCoreID(), (unsigned)BRIDGE_SPI_PRIORITY,
             (unsigned)sizeof(s_stack), (unsigned)s_transaction.length);
    xSemaphoreGive(s_boot);
    if (s_init_error != ESP_OK) { vTaskDelete(NULL); return; }
    /* Deliberate group boundary: hardware initialized, no transaction queued.
     * Block without polling. Group 2 replaces this with the real lifecycle. */
    for (;;) (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
}

esp_err_t bridge_spi_start(void *context)
{
    (void)context;
    if (s_started) return ESP_ERR_INVALID_STATE;
    s_started = true;
    s_boot = xSemaphoreCreateBinaryStatic(&s_boot_storage);
    if (!s_boot) return ESP_ERR_NO_MEM;
    s_task = xTaskCreateStaticPinnedToCore(spi_task, "scb_spi", sizeof(s_stack), NULL,
                                           BRIDGE_SPI_PRIORITY, s_stack, &s_task_storage, BRIDGE_SPI_CORE);
    if (!s_task) return ESP_ERR_NO_MEM;
    /* Only initialization waits here. It contains no socket/network dependency.
     * Actual SPI traffic will use a separate driver-owned transaction loop. */
    if (xSemaphoreTake(s_boot, portMAX_DELAY) != pdTRUE) return ESP_ERR_INVALID_STATE;
    return s_init_error;
}
