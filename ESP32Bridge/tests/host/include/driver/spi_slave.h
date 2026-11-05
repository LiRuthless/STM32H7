/**
 * @file spi_slave.h
 * @brief 定义虚拟SPI总线、原描述与回调接口，保持固定字段。
 * @details 仅为tests/host自有夹具提供driver/spi_slave.h所需的声明、固定常量或内联替身。
 *     实现由宿主夹具或本头内联提供，不链接真实SDK；不证明DMA、调度或硬件能力。
 */
#pragma once
#include "test_sdk.h"

enum
{
    SPI2_HOST = 1,
    SPI_DMA_CH_AUTO = 3,
    ESP_INTR_CPU_AFFINITY_1 = 2
};

typedef struct
{
    int mosi_io_num; /**< 配置的MOSI引脚值。 */
    int miso_io_num; /**< 配置的MISO引脚值。 */
    int sclk_io_num; /**< 配置的SCK引脚值。 */
    int quadwp_io_num; /**< 配置的WP引脚或-1。 */
    int quadhd_io_num; /**< 配置的HD引脚或-1。 */
    int data4_io_num; /**< 扩展数据4引脚或-1。 */
    int data5_io_num; /**< 扩展数据5引脚或-1。 */
    int data6_io_num; /**< 扩展数据6引脚或-1。 */
    int data7_io_num; /**< 扩展数据7引脚或-1。 */
    int max_transfer_sz; /**< 最大事务字节容量。 */
    uint32_t flags; /**< SDK固定标志字段，按原值保留。 */
    int isr_cpu_id; /**< 驱动初始化请求的ISR亲和性。 */
    int intr_flags; /**< 驱动初始化请求的中断属性。 */
} spi_bus_config_t;

typedef struct spi_slave_transaction_t
{
    uint32_t flags; /**< SDK固定标志字段，按原值保留。 */
    size_t length; /**< 描述请求位数，夹具按8192bit核对。 */
    size_t trans_len; /**< 虚拟驱动完成后报告的位数。 */
    const void *tx_buffer; /**< 排队后虚拟驱动借用的只读TX地址。 */
    void *rx_buffer, *user; /**< 保留SDK调用者上下文字段。 */
} spi_slave_transaction_t;

typedef struct
{
    int spics_io_num; /**< 配置的片选引脚值。 */
    uint32_t flags; /**< SDK固定标志字段，按原值保留。 */
    int queue_size; /**< 请求驱动队列深度。 */
    uint8_t mode; /**< SPI模式字段，夹具核对Mode0。 */
    void (*post_setup_cb)(spi_slave_transaction_t *); /**< 虚拟装载后调用的生产READY回调。 */
    void (*post_trans_cb)(spi_slave_transaction_t *); /**< 虚拟完成后调用的生产READY回调。 */
} spi_slave_interface_config_t;

esp_err_t spi_slave_initialize(
    int host, const spi_bus_config_t *bus, const spi_slave_interface_config_t *slave, int dma);
esp_err_t spi_slave_queue_trans(
    int host, const spi_slave_transaction_t *transaction, uint32_t ticks);
esp_err_t spi_slave_get_trans_result(
    int host, spi_slave_transaction_t **transaction, uint32_t ticks);
