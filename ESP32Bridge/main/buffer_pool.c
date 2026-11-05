#include "buffer_pool.h"
#include <string.h>

typedef struct {
    bool initialized;
    bridge_pool_ops_t ops;
    unsigned char *blocks[4];
    uint64_t slots[BRIDGE_POOL_KIND_COUNT][BRIDGE_EVENT_SLOTS];
    uint32_t used[BRIDGE_POOL_KIND_COUNT], peak[BRIDGE_POOL_KIND_COUNT];
    bridge_dma_owner_t dma_owner;
    uint64_t dma_generation;
} pool_state_t;
static pool_state_t s_pool;
/* Survives deinit so copied handles from an earlier pool stay invalid. */
static uint64_t s_next_generation = 1;
static const size_t s_bytes[4] = {
    BRIDGE_DMA_BYTES, BRIDGE_DMA_BYTES,
    BRIDGE_UDP_SLOTS * BRIDGE_UDP_BYTES, BRIDGE_EVENT_SLOTS * BRIDGE_EVENT_BYTES
};
static const uint32_t s_capacity[BRIDGE_POOL_KIND_COUNT] = { BRIDGE_UDP_SLOTS, BRIDGE_EVENT_SLOTS };
static const size_t s_slot_bytes[BRIDGE_POOL_KIND_COUNT] = { BRIDGE_UDP_BYTES, BRIDGE_EVENT_BYTES };
_Static_assert(2 * BRIDGE_DMA_BYTES + BRIDGE_UDP_SLOTS * BRIDGE_UDP_BYTES +
               BRIDGE_EVENT_SLOTS * BRIDGE_EVENT_BYTES == BRIDGE_POOL_DATA_BYTES, "Pool capacity mismatch");

static void free_blocks(const bridge_pool_ops_t *ops, unsigned char *const blocks[4])
{
    for (size_t i = 0; i < 4; ++i) {
        bool duplicate = false;
        for (size_t j = 0; j < i; ++j) {
            duplicate = duplicate || blocks[i] == blocks[j];
        }
        if (blocks[i] != NULL && !duplicate) {
            ops->free(blocks[i]);
        }
    }
}

/* Metadata lock held. Zero permanently denotes generation exhaustion. */
static esp_err_t next_generation(uint64_t *out)
{
    if (s_next_generation == 0) {
        return ESP_ERR_INVALID_STATE;
    }
    *out = s_next_generation;
    s_next_generation = s_next_generation == UINT64_MAX ? 0 : s_next_generation + 1;
    return ESP_OK;
}

esp_err_t bridge_pool_init(const bridge_pool_ops_t *ops)
{
    if (ops == NULL || ops->alloc == NULL || ops->free == NULL || ops->lock == NULL || ops->unlock == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_pool.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    unsigned char *blocks[4] = { NULL, NULL, NULL, NULL };
    esp_err_t result = ESP_OK;
    for (size_t i = 0; i < 4; ++i) {
        const size_t alignment = i < 2 ? BRIDGE_DMA_ALIGNMENT : 4;
        blocks[i] = ops->alloc(alignment, s_bytes[i], i < 2);
        if (blocks[i] == NULL) {
            result = ESP_ERR_NO_MEM;
            break;
        }
        if ((uintptr_t)blocks[i] % alignment != 0 || (uintptr_t)blocks[i] > UINTPTR_MAX - s_bytes[i]) {
            result = ESP_ERR_INVALID_STATE;
            break;
        }
        for (size_t j = 0; j < i; ++j) {
            const uintptr_t start = (uintptr_t)blocks[i], other = (uintptr_t)blocks[j];
            if (start < other + s_bytes[j] && other < start + s_bytes[i]) {
                result = ESP_ERR_INVALID_STATE;
                break;
            }
        }
        if (result != ESP_OK) {
            break;
        }
    }
    if (result != ESP_OK) {
        free_blocks(ops, blocks);
        return result;
    }
    /* Publish only after every allocation has passed checks. */
    for (size_t i = 0; i < 4; ++i) {
        memset(blocks[i], 0, s_bytes[i]);
    }
    s_pool = (pool_state_t){ .ops = *ops };
    for (size_t i = 0; i < 4; ++i) {
        s_pool.blocks[i] = blocks[i];
    }
    s_pool.initialized = true;
    return ESP_OK;
}

esp_err_t bridge_pool_deinit(void)
{
    if (!s_pool.initialized) {
        return ESP_OK;
    }
    bridge_pool_ops_t ops = s_pool.ops;
    ops.lock();
    if (s_pool.dma_owner != BRIDGE_DMA_IDLE || s_pool.used[0] != 0 || s_pool.used[1] != 0) {
        ops.unlock();
        return ESP_ERR_INVALID_STATE;
    }
    s_pool.initialized = false;
    ops.unlock();
    free_blocks(&ops, s_pool.blocks);
    s_pool = (pool_state_t){0};
    return ESP_OK;
}

esp_err_t bridge_pool_dma_acquire(bridge_dma_pair_t *pair)
{
    if (pair == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_pool.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    s_pool.ops.lock();
    esp_err_t result = ESP_ERR_INVALID_STATE;
    if (s_pool.dma_owner == BRIDGE_DMA_IDLE) {
        result = next_generation(&s_pool.dma_generation);
        if (result == ESP_OK) {
            s_pool.dma_owner = BRIDGE_DMA_APPLICATION;
            *pair = (bridge_dma_pair_t){ s_pool.blocks[0], s_pool.blocks[1], BRIDGE_DMA_BYTES, s_pool.dma_generation };
        }
    }
    s_pool.ops.unlock();
    return result;
}

static esp_err_t dma_transition(const bridge_dma_pair_t *pair, bridge_dma_owner_t from, bridge_dma_owner_t to)
{
    if (pair == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_pool.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    s_pool.ops.lock();
    esp_err_t result = ESP_ERR_INVALID_STATE;
    if (s_pool.dma_owner == from && pair->generation != 0 && pair->generation == s_pool.dma_generation &&
        pair->tx == s_pool.blocks[0] && pair->rx == s_pool.blocks[1] && pair->capacity == BRIDGE_DMA_BYTES) {
        s_pool.dma_owner = to;
        if (to == BRIDGE_DMA_IDLE) {
            s_pool.dma_generation = 0;
        }
        result = ESP_OK;
    }
    s_pool.ops.unlock();
    return result;
}

esp_err_t bridge_pool_dma_submit(const bridge_dma_pair_t *pair)
{
    return dma_transition(pair, BRIDGE_DMA_APPLICATION, BRIDGE_DMA_DRIVER);
}
esp_err_t bridge_pool_dma_reclaim(const bridge_dma_pair_t *pair)
{
    return dma_transition(pair, BRIDGE_DMA_DRIVER, BRIDGE_DMA_APPLICATION);
}
esp_err_t bridge_pool_dma_release(const bridge_dma_pair_t *pair)
{
    return dma_transition(pair, BRIDGE_DMA_APPLICATION, BRIDGE_DMA_IDLE);
}

esp_err_t bridge_pool_claim(bridge_pool_kind_t kind, bridge_pool_slot_t *slot)
{
    if ((unsigned)kind >= BRIDGE_POOL_KIND_COUNT || slot == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_pool.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    s_pool.ops.lock();
    esp_err_t result = ESP_ERR_NO_MEM;
    for (uint32_t i = 0; i < s_capacity[kind]; ++i) {
        if (s_pool.slots[kind][i] == 0) {
            result = next_generation(&s_pool.slots[kind][i]);
            if (result == ESP_OK) {
                ++s_pool.used[kind];
                if (s_pool.used[kind] > s_pool.peak[kind]) {
                    s_pool.peak[kind] = s_pool.used[kind];
                }
                *slot = (bridge_pool_slot_t){ kind, i, s_pool.blocks[2 + kind] + i * s_slot_bytes[kind],
                                             s_slot_bytes[kind], s_pool.slots[kind][i] };
            }
            break;
        }
    }
    s_pool.ops.unlock();
    return result;
}

esp_err_t bridge_pool_release(const bridge_pool_slot_t *slot)
{
    if (slot == NULL || (unsigned)slot->kind >= BRIDGE_POOL_KIND_COUNT || slot->index >= s_capacity[slot->kind]) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_pool.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    const bridge_pool_kind_t kind = slot->kind;
    s_pool.ops.lock();
    esp_err_t result = ESP_ERR_INVALID_STATE;
    if (slot->generation != 0 && slot->generation == s_pool.slots[kind][slot->index] &&
        slot->capacity == s_slot_bytes[kind] &&
        slot->data == s_pool.blocks[2 + kind] + slot->index * s_slot_bytes[kind]) {
        s_pool.slots[kind][slot->index] = 0;
        --s_pool.used[kind];
        result = ESP_OK;
    }
    s_pool.ops.unlock();
    return result;
}

esp_err_t bridge_pool_snapshot(bridge_pool_snapshot_t *out)
{
    if (out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_pool.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    s_pool.ops.lock();
    out->allocated_bytes = BRIDGE_POOL_DATA_BYTES;
    out->metadata_bytes = sizeof(s_pool) + sizeof(s_next_generation);
    out->dma_owner = s_pool.dma_owner;
    for (size_t i = 0; i < BRIDGE_POOL_KIND_COUNT; ++i) {
        out->capacity[i] = s_capacity[i];
        out->used[i] = s_pool.used[i];
        out->peak[i] = s_pool.peak[i];
    }
    s_pool.ops.unlock();
    return ESP_OK;
}
