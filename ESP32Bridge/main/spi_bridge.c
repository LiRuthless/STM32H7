/**
 * @file spi_bridge.c
 * @brief SPI事务驱动与READY发布。
 * @details 启动创建Core1静态任务；任务持DMA/描述和spi_protocol对象，经提交→驱动→回收处理。
 *     ISR回调只写GPIO READY，不解析/日志/等待；任务发布快照供NET/启动者读取。
 *     驱动可能仍持有内存的错误路径保留资源，不以逻辑超时伪造回收。
 */
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

/* SPI静态栈，字节额度由数组长度和元素宽度约束。 */
static StackType_t s_stack[BRIDGE_SPI_STACK_BYTES / sizeof(StackType_t)];
/* SPI静态任务控制块，启动创建后由RTOS使用。 */
static StaticTask_t s_task_storage;
/* SPI初始化通知信号量的静态存储。 */
static StaticSemaphore_t s_boot_storage;
/* 启动者等待、SPI任务发布初始化通知的句柄。 */
static SemaphoreHandle_t s_boot;
/* 启动时创建的SPI任务句柄。 */
static TaskHandle_t s_task;
/* SPI任务独占凭据；submit后不得解引用TX/RX。 */
static bridge_dma_pair_t s_dma;
/* SPI任务准备、驱动持有、原描述成功返回后才复用的唯一描述。 */
static spi_slave_transaction_t s_transaction;
/* SPI任务写，启动者收到初始化信号量后读取的结果。 */
static esp_err_t s_init_error;
/* 启动上下文单次启动标志，配置入口只在启动前使用。 */
static bool s_started;
/* 启动前复制、SPI任务只读使用的有界端口表。 */
static bridge_spi_port_t s_port;
/* 仅SPI任务推进的协议、窗口和计数状态。 */
static bridge_spi_protocol_t s_protocol;
/* SPI任务发布的快照副本，由s_snapshot_mux保护。 */
static bridge_spi_snapshot_t s_health = {.init_error = ESP_ERR_INVALID_STATE};
/* 只保护快照复制，不跨驱动等待或协议解析。 */
static portMUX_TYPE s_snapshot_mux = portMUX_INITIALIZER_UNLOCKED;
_Static_assert(sizeof(s_stack) == BRIDGE_SPI_STACK_BYTES, "ESP-IDF static stack size in bytes");

/**
 * @brief 通知已装载事务可由主机开始。
 * @details SPI驱动ISR回调只用GPIO低层接口写GPIO14为0，不解析、不打印、不等待或取得任务锁。
 * @param[in] transaction 驱动提供的描述，本回调不解引用，可为空。
 * @note 保持IRAM_ATTR；真实波形和缓存关闭行为仍待实板验证。
 */
static void IRAM_ATTR ready_loaded(spi_slave_transaction_t *transaction)
{
    (void)transaction;
    gpio_ll_set_level(&GPIO, 14, 0);
}

/**
 * @brief 撤销已完成事务的READY。
 * @details SPI驱动ISR回调只用GPIO低层接口写GPIO14为1，不解析、不打印、不等待或取得任务锁。
 * @param[in] transaction 驱动提供的描述，本回调不解引用，可为空。
 * @note 保持IRAM_ATTR；真实波形和缓存关闭行为仍待实板验证。
 */
static void IRAM_ATTR ready_finished(spi_slave_transaction_t *transaction)
{
    (void)transaction;
    gpio_ll_set_level(&GPIO, 14, 1);
}

/**
 * @brief 读取当前单调毫秒时间。
 * @details 将SDK微秒计时除以1000，供任务内重试和诊断使用。
 * @return 当前毫秒值。
 */
static uint64_t now_ms(void)
{
    return (uint64_t)esp_timer_get_time() / 1000u;
}

/**
 * @brief 取得生产会话随机候选。
 * @details 转发esp_random，候选合法性及最多四次尝试由协议层负责。
 * @param[in] context 保留的回调上下文，此绑定忽略，可为空。
 * @return 硬件随机源返回的32位候选，不在本函数回退固定值。
 */
static uint32_t random_session(void *context)
{
    (void)context;
    return esp_random();
}

/**
 * @brief 发布SPI任务持有状态的只读快照。
 * @details 先从协议对象和当前任务补充会话、计数及栈余量，再于短临界区复制；不借出DMA指针。
 * @param[in,out] health 非空SPI任务局部累计状态，补充字段后发布。
 * @note 只由SPI任务调用，其他任务通过bridge_spi_snapshot读取。
 */
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

/**
 * @brief 读取SPI服务发布的快照副本。
 * @details 只在短临界区复制，不操作协议、驱动或数据缓冲。
 * @param[out] snapshot 非空；成功写快照，空时不写。
 * @return 成功ESP_OK，空指针ESP_ERR_INVALID_ARG。
 * @note 任务上下文可读；快照不转移DMA所有权。
 */
esp_err_t bridge_spi_snapshot(bridge_spi_snapshot_t *snapshot)
{
    if (!snapshot)
    {
        return ESP_ERR_INVALID_ARG;
    }
    portENTER_CRITICAL(&s_snapshot_mux);
    *snapshot = s_health;
    portEXIT_CRITICAL(&s_snapshot_mux);
    return ESP_OK;
}

/**
 * @brief 在启动前复制SPI端口表。
 * @details 启动后拒绝替换；reserve/commit/cancel和peek/consumed必须成组提供。NULL清空表，生产无消费者时维持背压/丢弃。
 * @param[in] port 可为空；非空只借用表到调用结束，其context及回调依赖须活到服务结束。
 * @return 成功ESP_OK，已启动ESP_ERR_INVALID_STATE，组合非法ESP_ERR_INVALID_ARG。
 */
esp_err_t bridge_spi_configure(const bridge_spi_port_t *port)
{
    if (s_started)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (!bridge_spi_port_valid(port))
    {
        return ESP_ERR_INVALID_ARG;
    }
    s_port = port ? *port : (bridge_spi_port_t){0};
    return ESP_OK;
}

/**
 * @brief 执行独占DMA事务的提交与回收循环。
 * @details 准备TX并清RX/描述后移交池所有权，再零等待排队。超时只推进逻辑计时和快照；成功取回原描述后才reclaim并读取长度，先通知TX完成再处理完整RX。错误或陌生描述不能证明回
 *     收，退出时保留驱动可能持有的资源。
 * @param[in,out] workspace 非空且由SPI任务独占的JSON工作区。
 * @param[in,out] health 非空任务局部诊断累计值，按实际事件更新。
 * @return 初始化/排队/回收或服务失败返回实际ESP错误；正常循环不返回。
 * @note 仅SPI任务调用，不等待网络；重发图像只能在应用持有DMA时复制。
 */
static esp_err_t run_transactions(bridge_json_workspace_t *workspace, bridge_spi_snapshot_t *health)
{
    uint64_t last_log = now_ms();
    const uint32_t wait_ticks = pdMS_TO_TICKS(10) ? pdMS_TO_TICKS(10) : 1;
    for (;;)
    {
        bridge_spi_tx_meta_t sent;
        if (!bridge_spi_protocol_prepare(&s_protocol, now_ms(), s_dma.tx, workspace, &sent))
        {
            return ESP_ERR_INVALID_STATE;
        }
        for (size_t i = 0; i < BRIDGE_DMA_BYTES; ++i)
        {
            s_dma.rx[i] = 0;
        }
        s_transaction.trans_len = 0;
        esp_err_t error = bridge_pool_dma_submit(&s_dma);
        if (error != ESP_OK)
        {
            return error;
        }
        error = spi_slave_queue_trans(SPI2_HOST, &s_transaction, 0);
        if (error != ESP_OK)
        {
            /* 非OK排队结果证明本描述未被接受，才允许撤回DMA所有权。 */
            const esp_err_t reclaim = bridge_pool_dma_reclaim(&s_dma);
            return reclaim == ESP_OK ? error : reclaim;
        }
        ++health->queued;
        health->driver_owned = true;
        publish_health(health);
        spi_slave_transaction_t *returned = NULL;
        for (;;)
        {
            error = spi_slave_get_trans_result(SPI2_HOST, &returned, wait_ticks);
            if (error != ESP_ERR_TIMEOUT)
            {
                break;
            }
            ++health->wait_timeouts;
            /* 等待分支不访问事务描述或DMA数据。 */
            bridge_spi_protocol_poll(&s_protocol, now_ms());
            publish_health(health);
            const uint64_t now = now_ms();
            if (now - last_log >= 5000u)
            {
                ESP_LOGI("spi",
                    "queued=%u completed=%u short=%u crc=%u retry=%u invalid=%u dup=%u backpressure=%u delivered=%u rebuilds=%u tele_drop=%u invalid_out=%u session=%u ready=%d stack_min=%u internal_free=%u internal_min=%u",
                    (unsigned)health->queued, (unsigned)health->completed,
                    (unsigned)health->truncated, (unsigned)health->protocol.crc_errors,
                    (unsigned)health->protocol.retries, (unsigned)health->protocol.invalid,
                    (unsigned)health->protocol.duplicates, (unsigned)health->protocol.backpressure,
                    (unsigned)health->protocol.delivered, (unsigned)health->protocol.rebuilds,
                    (unsigned)health->protocol.telemetry_dropped,
                    (unsigned)health->protocol.invalid_outbound, (unsigned)health->session_id,
                    health->link_ready, (unsigned)health->stack_free_min,
                    (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                    (unsigned)heap_caps_get_minimum_free_size(
                        MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
                last_log = now;
            }
        }
        /* get错误或陌生描述不构成DMA回收证据。 */
        if (error != ESP_OK)
        {
            return error;
        }
        if (returned != &s_transaction)
        {
            return ESP_ERR_INVALID_STATE;
        }
        error = bridge_pool_dma_reclaim(&s_dma);
        if (error != ESP_OK)
        {
            return error;
        }
        health->driver_owned = false;
        ++health->completed;
        const bool complete = returned->trans_len == BRIDGE_DMA_BYTES * 8u;
        bridge_spi_protocol_sent(&s_protocol, sent, complete, now_ms());
        if (complete)
        {
            bridge_spi_protocol_receive(
                &s_protocol, s_dma.rx, s_dma.tx, workspace, random_session, NULL);
        }
        else if (returned->trans_len < BRIDGE_DMA_BYTES * 8u)
        {
            ++health->truncated;
        }
        else
        {
            ++health->overlength;
        }
        publish_health(health);
    }
}

/**
 * @brief 在指定核初始化SPI并运行事务服务。
 * @details Core1校验后取得DMA并安装驱动/回调；初始化失败且已acquire时回滚应用所有权。发布初始化快照后通知启动者；运行失败撤逻辑会话和READY，保留内存，不复位设备。
 * @param[in] context 任务参数，此任务忽略，可为空。
 * @note s_boot由启动者先创建；协议、描述与DMA仅此任务推进。
 */
static void spi_task(void *context)
{
    (void)context;
    /* SPI驱动在本任务所在核初始化，显式亲和性保持Core1归属。 */
    s_init_error = xPortGetCoreID() == BRIDGE_SPI_CORE ? ESP_OK : ESP_ERR_INVALID_STATE;
    bool acquired = false;
    if (s_init_error == ESP_OK)
    {
        s_init_error = bridge_pool_dma_acquire(&s_dma);
        acquired = s_init_error == ESP_OK;
    }
    if (s_init_error == ESP_OK)
    {
        const spi_bus_config_t bus =
        {
            .mosi_io_num = 11,
            .miso_io_num = 13,
            .sclk_io_num = 12,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .data4_io_num = -1,
            .data5_io_num = -1,
            .data6_io_num = -1,
            .data7_io_num = -1,
            .max_transfer_sz = BRIDGE_DMA_BYTES,
            .flags = 0,
            .isr_cpu_id = ESP_INTR_CPU_AFFINITY_1,
            .intr_flags = ESP_INTR_FLAG_IRAM
        };
        const spi_slave_interface_config_t slave =
        {
            .spics_io_num = 10,
            .flags = 0,
            .queue_size = 1,
            .mode = 0,
            .post_setup_cb = ready_loaded,
            .post_trans_cb = ready_finished
        };
        s_transaction = (spi_slave_transaction_t)
        {
            .length = BRIDGE_DMA_BYTES * 8u, .tx_buffer = s_dma.tx, .rx_buffer = s_dma.rx
        };
        s_init_error = spi_slave_initialize(SPI2_HOST, &bus, &slave, SPI_DMA_CH_AUTO);
    }
    if (s_init_error != ESP_OK && acquired)
    {
        const esp_err_t release_error = bridge_pool_dma_release(&s_dma);
        if (release_error != ESP_OK)
        {
            ESP_LOGE("spi", "DMA init rollback failed=%s", esp_err_to_name(release_error));
        }
    }
    ESP_LOGI("spi",
        "Driver init=%s core=%d priority=%u stack_bytes=%u transaction_bits=%u; callbacks publish READY only for queued transactions",
        esp_err_to_name(s_init_error), xPortGetCoreID(), (unsigned)BRIDGE_SPI_PRIORITY,
        (unsigned)sizeof(s_stack), (unsigned)s_transaction.length);
    bridge_spi_snapshot_t health = {.init_error = s_init_error};
    bridge_spi_protocol_init(&s_protocol, &s_port);
    publish_health(&health);
    xSemaphoreGive(s_boot);
    if (s_init_error == ESP_OK)
    {
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

/**
 * @brief 单次创建SPI静态任务并等待初始化结果。
 * @details 先置启动标志，再创建信号量和Core1/优先级12/6144字节栈任务；失败不重试重建。仅等待初始化通知，不等待网络或SPI流量。
 * @param[in] context 启动协调器参数，此适配忽略，可为空。
 * @return 已启动、无内存、通知失败或实际驱动初始化错误；成功ESP_OK。
 * @note READY保护及池须已由启动协调器准备，运行期不得再次配置端口。
 */
esp_err_t bridge_spi_start(void *context)
{
    (void)context;
    if (s_started)
    {
        return ESP_ERR_INVALID_STATE;
    }
    s_started = true;
    s_boot = xSemaphoreCreateBinaryStatic(&s_boot_storage);
    if (!s_boot)
    {
        portENTER_CRITICAL(&s_snapshot_mux);
        s_health.init_error = ESP_ERR_NO_MEM;
        portEXIT_CRITICAL(&s_snapshot_mux);
        return ESP_ERR_NO_MEM;
    }
    s_task = xTaskCreateStaticPinnedToCore(spi_task, "scb_spi", sizeof(s_stack), NULL,
        BRIDGE_SPI_PRIORITY, s_stack, &s_task_storage, BRIDGE_SPI_CORE);
    if (!s_task)
    {
        portENTER_CRITICAL(&s_snapshot_mux);
        s_health.init_error = ESP_ERR_NO_MEM;
        portEXIT_CRITICAL(&s_snapshot_mux);
        return ESP_ERR_NO_MEM;
    }
    /* 这里只等SPI初始化；运行期由独占驱动事务循环推进，不依赖socket。 */
    if (xSemaphoreTake(s_boot, portMAX_DELAY) != pdTRUE)
    {
        return ESP_ERR_INVALID_STATE;
    }
    return s_init_error;
}
