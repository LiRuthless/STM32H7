#pragma once
#include "test_sdk.h"
enum { SPI2_HOST = 1, SPI_DMA_CH_AUTO = 3, ESP_INTR_CPU_AFFINITY_1 = 2 };
typedef struct {
    int mosi_io_num, miso_io_num, sclk_io_num, quadwp_io_num, quadhd_io_num;
    int data4_io_num, data5_io_num, data6_io_num, data7_io_num;
    int max_transfer_sz;
    uint32_t flags;
    int isr_cpu_id, intr_flags;
} spi_bus_config_t;
typedef struct spi_slave_transaction_t {
    uint32_t flags;
    size_t length, trans_len;
    const void *tx_buffer;
    void *rx_buffer, *user;
} spi_slave_transaction_t;
typedef struct {
    int spics_io_num;
    uint32_t flags;
    int queue_size;
    uint8_t mode;
    void (*post_setup_cb)(spi_slave_transaction_t *);
    void (*post_trans_cb)(spi_slave_transaction_t *);
} spi_slave_interface_config_t;
esp_err_t spi_slave_initialize(int host, const spi_bus_config_t *bus, const spi_slave_interface_config_t *slave, int dma);
esp_err_t spi_slave_queue_trans(int host, const spi_slave_transaction_t *transaction, uint32_t ticks);
esp_err_t spi_slave_get_trans_result(int host, spi_slave_transaction_t **transaction, uint32_t ticks);
