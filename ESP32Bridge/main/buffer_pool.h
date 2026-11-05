#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

enum {
    BRIDGE_DMA_BYTES = 1024, BRIDGE_DMA_ALIGNMENT = 64,
    BRIDGE_UDP_SLOTS = 16, BRIDGE_UDP_BYTES = 1016,
    BRIDGE_EVENT_SLOTS = 32, BRIDGE_EVENT_BYTES = 1024,
    BRIDGE_POOL_DATA_BYTES = 51072
};
typedef enum { BRIDGE_POOL_UDP, BRIDGE_POOL_EVENT, BRIDGE_POOL_KIND_COUNT } bridge_pool_kind_t;
typedef enum { BRIDGE_DMA_IDLE, BRIDGE_DMA_APPLICATION, BRIDGE_DMA_DRIVER } bridge_dma_owner_t;

/* Allocator must return internal memory, DMA-capable when requested. init/deinit
 * run only with services stopped. Callbacks cannot reenter and remain valid
 * until deinit; metadata paths use lock, allocations never run under it. */
typedef struct {
    void *(*alloc)(size_t alignment, size_t bytes, bool dma);
    void (*free)(void *ptr);
    void (*lock)(void);
    void (*unlock)(void);
} bridge_pool_ops_t;
typedef struct {
    unsigned char *tx, *rx;
    size_t capacity;
    uint64_t generation;
} bridge_dma_pair_t;
typedef struct {
    bridge_pool_kind_t kind;
    uint32_t index;
    unsigned char *data;
    size_t capacity;
    uint64_t generation;
} bridge_pool_slot_t;
typedef struct {
    size_t allocated_bytes, metadata_bytes;
    uint32_t capacity[BRIDGE_POOL_KIND_COUNT];
    uint32_t used[BRIDGE_POOL_KIND_COUNT], peak[BRIDGE_POOL_KIND_COUNT];
    bridge_dma_owner_t dma_owner;
} bridge_pool_snapshot_t;

esp_err_t bridge_pool_init(const bridge_pool_ops_t *ops);
esp_err_t bridge_pool_deinit(void);
esp_err_t bridge_pool_dma_acquire(bridge_dma_pair_t *pair);
esp_err_t bridge_pool_dma_submit(const bridge_dma_pair_t *pair);
/* Only after receiving the driver's transaction, or a queue error proving
 * it never accepted the buffers. A timer is not reclamation evidence. */
esp_err_t bridge_pool_dma_reclaim(const bridge_dma_pair_t *pair);
esp_err_t bridge_pool_dma_release(const bridge_dma_pair_t *pair);
esp_err_t bridge_pool_claim(bridge_pool_kind_t kind, bridge_pool_slot_t *slot);
esp_err_t bridge_pool_release(const bridge_pool_slot_t *slot);
esp_err_t bridge_pool_snapshot(bridge_pool_snapshot_t *out);
