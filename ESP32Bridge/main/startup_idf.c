/**
 * @file startup_idf.c
 * @brief 真实IDF启动与固定池适配。
 * @details 启动任务先保护READY、检查配置和创建池，再调用startup协调NET/SPI。
 *     本模块持池元数据锁；池回调供任务使用，不在锁内分配。服务端口启动前绑定。
 */
#include "startup.h"
#include "buffer_pool.h"
#include "spi_bridge.h"
#include "net_bridge.h"
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

/* 启动时建立，运行期池元数据操作共用的短临界区锁。 */
static portMUX_TYPE s_pool_mux = portMUX_INITIALIZER_UNLOCKED;

/**
 * @brief 进入固定池元数据临界区。
 * @details 使用本模块静态portMUX；与pool_unlock成对，锁内不分配内存。
 * @note 供池接口在任务上下文调用，不可重入。
 */
static void pool_lock(void)
{
    portENTER_CRITICAL(&s_pool_mux);
}

/**
 * @brief 退出固定池元数据临界区。
 * @details 释放pool_lock取得的同一portMUX。
 * @note 调用方必须已持有该锁。
 */
static void pool_unlock(void)
{
    portEXIT_CRITICAL(&s_pool_mux);
}

/**
 * @brief 分配并核验内部可达的池内存。
 * @details 按DMA标志选择能力；核验首尾地址的内部/DMA可达性，失败释放本次分配，不发布不合格指针。
 * @param[in] alignment 由池传入的对齐字节数。
 * @param[in] bytes 非零分配字节数，由固定池容量表保证。
 * @param[in] dma true要求内部DMA能力，false仅要求内部8位可达。
 * @return 返回独占分配块，失败返回NULL；由池通过heap_caps_free归还。
 */
static void *pool_alloc(size_t alignment, size_t bytes, bool dma)
{
    uint32_t caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
    if (dma)
    {
        caps |= MALLOC_CAP_DMA;
    }
    unsigned char *ptr = heap_caps_aligned_alloc(alignment, bytes, caps);
    if (ptr != NULL
        && (!esp_ptr_internal(ptr) || !esp_ptr_internal(ptr + bytes - 1)
            || (dma && (!esp_ptr_dma_capable(ptr) || !esp_ptr_dma_capable(ptr + bytes - 1)))))
    {
        heap_caps_free(ptr);
        return NULL;
    }
    return ptr;
}

/**
 * @brief 初始化NVS且保留原错误。
 * @details 转发nvs_flash_init，不以擦除Flash或重启掩盖失败。
 * @param[in] context 启动表上下文，此适配不使用，可为空。
 * @return 返回SDK初始化结果。
 */
static esp_err_t init_nvs(void *context)
{
    (void)context;
    /* 不以擦除Flash或重启掩盖初始化错误。 */
    return nvs_flash_init();
}

/**
 * @brief 初始化网络接口运行时。
 * @details 转发esp_netif_init，不以擦除Flash或重启掩盖失败。
 * @param[in] context 启动表上下文，此适配不使用，可为空。
 * @return 返回SDK初始化结果。
 */
static esp_err_t init_netif(void *context)
{
    (void)context;
    return esp_netif_init();
}

/**
 * @brief 建立默认事件循环。
 * @details 转发esp_event_loop_create_default，不以擦除Flash或重启掩盖失败。
 * @param[in] context 启动表上下文，此适配不使用，可为空。
 * @return 返回SDK初始化结果。
 */
static esp_err_t init_event_loop(void *context)
{
    (void)context;
    return esp_event_loop_create_default();
}

/**
 * @brief 将启动协调器绑定到真实IDF资源与服务。
 * @details 先锁存READY高电平再配置输出，随后校验密码、创建固定池并绑定UDP出口，最后由协调器按依赖启动。网络失败不阻断独立SPI；本接口不擦Flash、不复位。
 * @param[out] report 不能为空；空指针直接拒绝，其他路径由bridge_startup_run完整填充。
 * @return 返回参数错误或协调器的实际启动结果。
 * @note 仅服务未启动时调用；池锁和端口表在服务运行期间保持有效。
 */
esp_err_t bridge_startup_idf_run(bridge_startup_report_t *report)
{
    if (report == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    /* 先锁存高电平再启用输出；实测波形仍待验证。 */
    esp_err_t ready_error = gpio_set_level(GPIO_NUM_14, 1);
    if (ready_error == ESP_OK)
    {
        const gpio_config_t ready =
        {
            .pin_bit_mask = UINT64_C(1) << GPIO_NUM_14,
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        ready_error = gpio_config(&ready);
    }
    static const unsigned char password[] = CONFIG_BRIDGE_AP_PASSWORD;
    const esp_err_t password_error = bridge_password_validate(password, sizeof(password) - 1);
    if (password_error != ESP_OK)
    {
        ESP_LOGE("startup", "Invalid local AP configuration; AP/socket startup blocked");
    }
    const bridge_pool_ops_t memory_ops = {pool_alloc, heap_caps_free, pool_lock, pool_unlock};
    const size_t free_before = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    const esp_err_t pool_error = bridge_pool_init(&memory_ops);
    /* 服务启动前绑定已知非空UDP回调，此时s_started尚未设置；不等待网络流量。 */
    (void)bridge_net_configure(&(const bridge_net_port_t){NULL, bridge_net_socket_telemetry});
    ESP_LOGI("startup", "Pool init=%s; internal free before=%u after=%u minimum=%u",
        esp_err_to_name(pool_error), (unsigned)free_before,
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
        (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    const bridge_startup_ops_t ops =
    {
        .nvs = init_nvs,
        .netif = init_netif,
        .event_loop = init_event_loop,
        .ap = bridge_net_ap_start,
        .sockets = bridge_net_start,
        .spi = bridge_spi_start,
        .ready_error = ready_error,
        .password_error = password_error,
        .pool_error = pool_error
    };
    return bridge_startup_run(&ops, report);
}
