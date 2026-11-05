/**
 * @file buffer_pool.h
 * @brief 内部固定池与DMA所有权。
 * @details 声明内部固定池与DMA所有权接口；实现及完整参数/错误语义位于buffer_pool.c。
 *     调用方按接口约束串行访问owner状态，头文件不定义线图像或运行期服务。
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

enum
{
    BRIDGE_DMA_BYTES = 1024, /**< 每块DMA字节数。 */
    BRIDGE_DMA_ALIGNMENT = 64, /**< DMA起始地址对齐字节数。 */
    BRIDGE_UDP_SLOTS = 16, /**< UDP固定池槽数。 */
    BRIDGE_UDP_BYTES = 1016, /**< 每个UDP池槽字节数。 */
    BRIDGE_EVENT_SLOTS = 32, /**< EVENT固定池槽数。 */
    BRIDGE_EVENT_BYTES = 1024, /**< 每个EVENT池槽字节数。 */
    BRIDGE_POOL_DATA_BYTES = 51072 /**< 四块数据总字节数，不含元数据。 */
};

typedef enum
{
    BRIDGE_POOL_UDP, /**< UDP槽类别。 */
    BRIDGE_POOL_EVENT, /**< EVENT槽类别。 */
    BRIDGE_POOL_KIND_COUNT /**< 池类别数哨兵，不是可借类别。 */
} bridge_pool_kind_t;

typedef enum
{
    BRIDGE_DMA_IDLE, /**< DMA在池内空闲。 */
    BRIDGE_DMA_APPLICATION, /**< 唯一应用owner持有，可准备或处理。 */
    BRIDGE_DMA_DRIVER /**< 驱动持有，应用不可访问数据。 */
} bridge_dma_owner_t;

/*
 * 分配回调须返回内部内存；DMA请求还须可达。init/deinit仅在服务停止时执行。
 * 回调不重入，依赖持续到deinit；锁只保护元数据，分配在锁外。
 */
typedef struct
{
    void *(*alloc)(size_t alignment, size_t bytes,
        bool dma); /**< 分配内部内存；dma为true时还须满足DMA能力。 */
    void (*free)(void *ptr); /**< 归还alloc获得的独立块，不在元数据锁内调用。 */
    void (*lock)(void); /**< 进入短元数据临界区，不可重入。 */
    void (*unlock)(void); /**< 与lock成对退出同一临界区。 */
} bridge_pool_ops_t;

typedef struct
{
    unsigned char *tx; /**< 借出的TX字节区，驱动持有期应用不可访问。 */
    unsigned char *rx; /**< 借出的RX字节区，驱动返还后才可读写。 */
    size_t capacity; /**< 每个缓冲的容量，字节。 */
    uint64_t generation; /**< 非零借用代际，校验过期/伪造凭据。 */
} bridge_dma_pair_t;

typedef struct
{
    bridge_pool_kind_t kind; /**< UDP或EVENT池类别。 */
    uint32_t index; /**< 类内槽索引，从零开始。 */
    unsigned char *data; /**< 借出槽起始地址，release成功后失效。 */
    size_t capacity; /**< 该槽可写字节容量。 */
    uint64_t generation; /**< 非零槽代际，防止重复或过期归还。 */
} bridge_pool_slot_t;

typedef struct
{
    size_t allocated_bytes; /**< 数据块容量总和，字节，不含SDK/任务占用。 */
    size_t metadata_bytes; /**< 池状态及代际计数器大小，字节。 */
    uint32_t capacity[BRIDGE_POOL_KIND_COUNT]; /**< 各类池配置槽数。 */
    uint32_t used[BRIDGE_POOL_KIND_COUNT]; /**< 快照时各类借出槽数。 */
    uint32_t peak[BRIDGE_POOL_KIND_COUNT]; /**< 各类历史借出槽数峰值。 */
    bridge_dma_owner_t dma_owner; /**< 快照时成对DMA所有者。 */
} bridge_pool_snapshot_t;

/**
 * @brief 一次性分配并发布完整固定池。
 * @note buffer_pool.c::bridge_pool_init；init/deinit须停服；借出数据随凭据归还而失效。
 */
esp_err_t bridge_pool_init(const bridge_pool_ops_t *ops);
/**
 * @brief 无借出资源时销毁池。
 * @note buffer_pool.c::bridge_pool_deinit；init/deinit须停服；借出数据随凭据归还而失效。
 */
esp_err_t bridge_pool_deinit(void);
/**
 * @brief 独占取得成对DMA缓冲的应用所有权。
 * @note buffer_pool.c::bridge_pool_dma_acquire；init/deinit须停服；借出数据随凭据归还而失效。
 */
esp_err_t bridge_pool_dma_acquire(bridge_dma_pair_t *pair);
/**
 * @brief 把DMA所有权从应用移交驱动。
 * @note buffer_pool.c::bridge_pool_dma_submit；init/deinit须停服；借出数据随凭据归还而失效。
 */
esp_err_t bridge_pool_dma_submit(const bridge_dma_pair_t *pair);
/**
 * @brief 凭驱动返回证据回收DMA到应用。
 * @note buffer_pool.c::bridge_pool_dma_reclaim；init/deinit须停服；借出数据随凭据归还而失效。
 */
esp_err_t bridge_pool_dma_reclaim(const bridge_dma_pair_t *pair);
/**
 * @brief 将应用持有的DMA归还空闲池。
 * @note buffer_pool.c::bridge_pool_dma_release；init/deinit须停服；借出数据随凭据归还而失效。
 */
esp_err_t bridge_pool_dma_release(const bridge_dma_pair_t *pair);
/**
 * @brief 从指定类别取得一个固定槽。
 * @note buffer_pool.c::bridge_pool_claim；init/deinit须停服；借出数据随凭据归还而失效。
 */
esp_err_t bridge_pool_claim(bridge_pool_kind_t kind, bridge_pool_slot_t *slot);
/**
 * @brief 校验代际和地址后归还固定槽。
 * @note buffer_pool.c::bridge_pool_release；init/deinit须停服；借出数据随凭据归还而失效。
 */
esp_err_t bridge_pool_release(const bridge_pool_slot_t *slot);
/**
 * @brief 复制池额度及所有权快照。
 * @note buffer_pool.c::bridge_pool_snapshot；init/deinit须停服；借出数据随凭据归还而失效。
 */
esp_err_t bridge_pool_snapshot(bridge_pool_snapshot_t *out);
