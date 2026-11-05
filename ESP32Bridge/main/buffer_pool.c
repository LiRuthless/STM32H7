/**
 * @file buffer_pool.c
 * @brief 内部固定池与DMA所有权。
 * @details 启动时取得四个数据块，运行期经凭据借出/归还并输出额度快照。
 *     本模块持元数据及代际；调用方锁保护运行期转换，数据由唯一持有者访问。
 *     init/deinit要求停服且无并发，池接口不代替驱动回收证据。
 */
#include "buffer_pool.h"
#include <string.h>

typedef struct
{
    bool initialized; /**< 完整池是否已发布；init/deinit须无并发。 */
    bridge_pool_ops_t ops; /**< 初始化复制的分配/锁回调，持续到deinit。 */
    unsigned char *blocks[4]; /**< 四个独立分配块，依次TX、RX、UDP池、EVENT池。 */
    /**
     * @brief 各固定槽当前代际；零为空闲，元数据锁保护。
     */
    uint64_t slots[BRIDGE_POOL_KIND_COUNT][BRIDGE_EVENT_SLOTS];
    uint32_t used[BRIDGE_POOL_KIND_COUNT]; /**< 各池当前借出槽数，锁内更新。 */
    uint32_t peak[BRIDGE_POOL_KIND_COUNT]; /**< 各池借出槽峰值，锁内更新。 */
    bridge_dma_owner_t dma_owner; /**< 成对DMA当前所有者，锁内转换。 */
    uint64_t dma_generation; /**< 当前DMA凭据代际，IDLE时为零。 */
} pool_state_t;

/* 池状态和额度元数据；运行期由ops锁保护，数据由借用者持有。 */
static pool_state_t s_pool;
/* 代际计数跨deinit保留，避免旧池凭据在新池再次有效。 */
/* 持久于deinit的单调代际，运行期仅在池锁内推进。 */
static uint64_t s_next_generation = 1;
/* 启动四块分配大小表，字节，只读。 */
static const size_t s_bytes[4] =
{
    BRIDGE_DMA_BYTES, BRIDGE_DMA_BYTES,
    BRIDGE_UDP_SLOTS * BRIDGE_UDP_BYTES, BRIDGE_EVENT_SLOTS * BRIDGE_EVENT_BYTES
};
/* 各类固定池槽数，只读。 */
static const uint32_t s_capacity[BRIDGE_POOL_KIND_COUNT] = {BRIDGE_UDP_SLOTS, BRIDGE_EVENT_SLOTS};
/* 各类固定池单槽字节数，只读。 */
static const size_t s_slot_bytes[BRIDGE_POOL_KIND_COUNT] = {BRIDGE_UDP_BYTES, BRIDGE_EVENT_BYTES};
_Static_assert(2 * BRIDGE_DMA_BYTES + BRIDGE_UDP_SLOTS * BRIDGE_UDP_BYTES
            + BRIDGE_EVENT_SLOTS * BRIDGE_EVENT_BYTES
        == BRIDGE_POOL_DATA_BYTES,
    "Pool capacity mismatch");

/**
 * @brief 归还本轮已获得且不重复的分配块。
 * @details 按原分配顺序扫描，跳过NULL和与前项相同的地址，避免失败回滚重复释放。
 * @param[in] ops 不能为空且free有效；仅在本次回滚/销毁期间借用。
 * @param[in] blocks 不能为空，四个槽的数组；块在释放后不再可访问。
 * @note 不持元数据锁调用；地址相同去重不等于任意重叠块可安全释放。
 */
static void free_blocks(const bridge_pool_ops_t *ops, unsigned char *const blocks[4])
{
    for (size_t i = 0; i < 4; ++i)
    {
        bool duplicate = false;
        for (size_t j = 0; j < i; ++j)
        {
            duplicate = duplicate || blocks[i] == blocks[j];
        }
        if (blocks[i] != NULL && !duplicate)
        {
            ops->free(blocks[i]);
        }
    }
}

/* 调用方已持元数据锁；零永久表示代际耗尽。 */
/**
 * @brief 取得不回绕的槽位代际。
 * @details 零表示永久耗尽；成功写出当前代际并递增，UINT64_MAX后将下次值置零。
 * @param[out] out 不能为空；成功写入非零代际，耗尽时保持原输出。
 * @return 成功ESP_OK，代际耗尽ESP_ERR_INVALID_STATE。
 * @note 调用方已持池元数据锁。
 */
static esp_err_t next_generation(uint64_t *out)
{
    if (s_next_generation == 0)
    {
        return ESP_ERR_INVALID_STATE;
    }
    *out = s_next_generation;
    s_next_generation = s_next_generation == UINT64_MAX ? 0 : s_next_generation + 1;
    return ESP_OK;
}

/**
 * @brief 一次性分配并发布完整固定池。
 * @details 校验回调、四块内存的容量边界、对齐与重叠；任一步失败回滚，不发布半池。全部检查通过后清零块并发布initialized。
 * @param[in] ops 不能为空，各回调必须有效；复制表，回调及其依赖须持续至deinit。
 * @return 成功ESP_OK；参数错误、重复初始化、分配失败或无效内存分别返回对应ESP错误。
 * @note 只在服务停止、无并发使用时调用；分配不在元数据锁内。
 */
esp_err_t bridge_pool_init(const bridge_pool_ops_t *ops)
{
    if (ops == NULL || ops->alloc == NULL || ops->free == NULL || ops->lock == NULL
        || ops->unlock == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_pool.initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }
    unsigned char *blocks[4] = {NULL, NULL, NULL, NULL};
    esp_err_t result = ESP_OK;
    for (size_t i = 0; i < 4; ++i)
    {
        const size_t alignment = i < 2 ? BRIDGE_DMA_ALIGNMENT : 4;
        blocks[i] = ops->alloc(alignment, s_bytes[i], i < 2);
        if (blocks[i] == NULL)
        {
            result = ESP_ERR_NO_MEM;
            break;
        }
        if ((uintptr_t)blocks[i] % alignment != 0
            || (uintptr_t)blocks[i] > UINTPTR_MAX - s_bytes[i])
        {
            result = ESP_ERR_INVALID_STATE;
            break;
        }
        for (size_t j = 0; j < i; ++j)
        {
            const uintptr_t start = (uintptr_t)blocks[i];
            const uintptr_t other = (uintptr_t)blocks[j];
            if (start < other + s_bytes[j] && other < start + s_bytes[i])
            {
                result = ESP_ERR_INVALID_STATE;
                break;
            }
        }
        if (result != ESP_OK)
        {
            break;
        }
    }
    if (result != ESP_OK)
    {
        free_blocks(ops, blocks);
        return result;
    }
    /* 所有分配检查完成后才清零并发布完整池。 */
    for (size_t i = 0; i < 4; ++i)
    {
        memset(blocks[i], 0, s_bytes[i]);
    }
    s_pool = (pool_state_t){.ops = *ops};
    for (size_t i = 0; i < 4; ++i)
    {
        s_pool.blocks[i] = blocks[i];
    }
    s_pool.initialized = true;
    return ESP_OK;
}

/**
 * @brief 无借出资源时销毁池。
 * @details 未初始化时幂等返回；锁内检查DMA和两类槽是否空闲，忙时拒绝。撤销initialized后锁外释放，保留独立代际计数器。
 * @return 成功或未初始化返回ESP_OK；仍有持有者返回ESP_ERR_INVALID_STATE。
 * @note 调用方须停止服务并保证没有并发池操作。
 */
esp_err_t bridge_pool_deinit(void)
{
    if (!s_pool.initialized)
    {
        return ESP_OK;
    }
    bridge_pool_ops_t ops = s_pool.ops;
    ops.lock();
    if (s_pool.dma_owner != BRIDGE_DMA_IDLE || s_pool.used[0] != 0 || s_pool.used[1] != 0)
    {
        ops.unlock();
        return ESP_ERR_INVALID_STATE;
    }
    s_pool.initialized = false;
    ops.unlock();
    free_blocks(&ops, s_pool.blocks);
    s_pool = (pool_state_t){0};
    return ESP_OK;
}

/**
 * @brief 独占取得成对DMA缓冲的应用所有权。
 * @details 锁内仅从IDLE分配新代际并切换为APPLICATION；不分配数据内存。
 * @param[out] pair 不能为空；仅成功时写TX/RX、容量及代际，失败保留原值。
 * @return 成功ESP_OK；空指针ESP_ERR_INVALID_ARG，未初始化、忙或代际耗尽ESP_ERR_INVALID_STATE。
 * @note 由唯一SPI任务使用；取得后至submit前可访问数据。
 */
esp_err_t bridge_pool_dma_acquire(bridge_dma_pair_t *pair)
{
    if (pair == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_pool.initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }
    s_pool.ops.lock();
    esp_err_t result = ESP_ERR_INVALID_STATE;
    if (s_pool.dma_owner == BRIDGE_DMA_IDLE)
    {
        result = next_generation(&s_pool.dma_generation);
        if (result == ESP_OK)
        {
            s_pool.dma_owner = BRIDGE_DMA_APPLICATION;
            *pair = (bridge_dma_pair_t)
            {
                s_pool.blocks[0], s_pool.blocks[1], BRIDGE_DMA_BYTES, s_pool.dma_generation
            };
        }
    }
    s_pool.ops.unlock();
    return result;
}

/**
 * @brief 核验凭据后转换DMA所有权。
 * @details 在池锁内同时检查源状态、非零代际、TX/RX和容量，全部匹配才转换；归还IDLE时清除当前代际。
 * @param[in] pair 不能为空，调用期间借用；不访问其指向的数据。
 * @param[in] from 必须匹配的当前所有者。
 * @param[in] to 成功后写入的所有者。
 * @return 成功ESP_OK；参数错误或状态/凭据不符返回对应ESP错误。
 */
static esp_err_t dma_transition(
    const bridge_dma_pair_t *pair, bridge_dma_owner_t from, bridge_dma_owner_t to)
{
    if (pair == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_pool.initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }
    s_pool.ops.lock();
    esp_err_t result = ESP_ERR_INVALID_STATE;
    if (s_pool.dma_owner == from && pair->generation != 0
        && pair->generation == s_pool.dma_generation && pair->tx == s_pool.blocks[0]
        && pair->rx == s_pool.blocks[1] && pair->capacity == BRIDGE_DMA_BYTES)
    {
        s_pool.dma_owner = to;
        if (to == BRIDGE_DMA_IDLE)
        {
            s_pool.dma_generation = 0;
        }
        result = ESP_OK;
    }
    s_pool.ops.unlock();
    return result;
}

/**
 * @brief 把DMA所有权从应用移交驱动。
 * @details 成功后应用不得访问缓冲或驱动描述；实际排队由SPI任务完成。
 * @param[in] pair 非空的原始DMA凭据；调用期间借用，成功/失败均不改凭据对象。
 * @return 转发dma_transition的ESP_OK、参数错误或状态错误。
 * @note 唯一SPI owner串行调用。
 */
esp_err_t bridge_pool_dma_submit(const bridge_dma_pair_t *pair)
{
    return dma_transition(pair, BRIDGE_DMA_APPLICATION, BRIDGE_DMA_DRIVER);
}

/**
 * @brief 凭驱动返回证据回收DMA到应用。
 * @details 仅当原描述成功返回，或排队失败证明未接受该描述时调用；超时和会话失效均不是回收证据。
 * @param[in] pair 非空的原始DMA凭据；调用期间借用，成功/失败均不改凭据对象。
 * @return 转发dma_transition的ESP_OK、参数错误或状态错误。
 * @note 唯一SPI owner串行调用。
 */
esp_err_t bridge_pool_dma_reclaim(const bridge_dma_pair_t *pair)
{
    return dma_transition(pair, BRIDGE_DMA_DRIVER, BRIDGE_DMA_APPLICATION);
}

/**
 * @brief 将应用持有的DMA归还空闲池。
 * @details 仅允许APPLICATION到IDLE，不可释放驱动仍持有的缓冲。
 * @param[in] pair 非空的原始DMA凭据；调用期间借用，成功/失败均不改凭据对象。
 * @return 转发dma_transition的ESP_OK、参数错误或状态错误。
 * @note 唯一SPI owner串行调用。
 */
esp_err_t bridge_pool_dma_release(const bridge_dma_pair_t *pair)
{
    return dma_transition(pair, BRIDGE_DMA_APPLICATION, BRIDGE_DMA_IDLE);
}

/**
 * @brief 从指定类别取得一个固定槽。
 * @details 锁内按索引找首个空槽，取得新代际后更新使用量和峰值，再发布凭据；满池不接管数据。
 * @param[in] kind UDP或EVENT类别，不接受计数哨兵或越界值。
 * @param[out] slot 不能为空；成功写入槽凭据及借用数据指针，失败保留原值。
 * @return 成功ESP_OK；参数错误、未初始化、代际耗尽或满池分别返回对应ESP错误。
 * @note 借出数据仅由凭据持有者访问，直到成功release。
 */
esp_err_t bridge_pool_claim(bridge_pool_kind_t kind, bridge_pool_slot_t *slot)
{
    if ((unsigned)kind >= BRIDGE_POOL_KIND_COUNT || slot == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_pool.initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }
    s_pool.ops.lock();
    esp_err_t result = ESP_ERR_NO_MEM;
    for (uint32_t i = 0; i < s_capacity[kind]; ++i)
    {
        if (s_pool.slots[kind][i] == 0)
        {
            result = next_generation(&s_pool.slots[kind][i]);
            if (result == ESP_OK)
            {
                ++s_pool.used[kind];
                if (s_pool.used[kind] > s_pool.peak[kind])
                {
                    s_pool.peak[kind] = s_pool.used[kind];
                }
                *slot = (bridge_pool_slot_t)
                {
                    kind, i,
                    s_pool.blocks[2 + kind] + i * s_slot_bytes[kind], s_slot_bytes[kind],
                    s_pool.slots[kind][i]
                };
            }
            break;
        }
    }
    s_pool.ops.unlock();
    return result;
}

/**
 * @brief 校验代际和地址后归还固定槽。
 * @details 锁内核验类别、索引、非零代际、容量及数据指针，重复或过期归还拒绝，不减使用量。
 * @param[in] slot 不能为空；只借用凭据，成功后数据不可再使用。
 * @return 成功ESP_OK，参数错误ESP_ERR_INVALID_ARG，未初始化或无效凭据ESP_ERR_INVALID_STATE。
 */
esp_err_t bridge_pool_release(const bridge_pool_slot_t *slot)
{
    if (slot == NULL || (unsigned)slot->kind >= BRIDGE_POOL_KIND_COUNT
        || slot->index >= s_capacity[slot->kind])
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_pool.initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }
    const bridge_pool_kind_t kind = slot->kind;
    s_pool.ops.lock();
    esp_err_t result = ESP_ERR_INVALID_STATE;
    if (slot->generation != 0 && slot->generation == s_pool.slots[kind][slot->index]
        && slot->capacity == s_slot_bytes[kind]
        && slot->data == s_pool.blocks[2 + kind] + slot->index * s_slot_bytes[kind])
    {
        s_pool.slots[kind][slot->index] = 0;
        --s_pool.used[kind];
        result = ESP_OK;
    }
    s_pool.ops.unlock();
    return result;
}

/**
 * @brief 复制池额度及所有权快照。
 * @details 锁内复制容量、使用/峰值、元数据大小和DMA状态；不转移任何数据指针。
 * @param[out] out 不能为空；成功填充，失败不修改。
 * @return 成功ESP_OK，空指针或未初始化返回对应ESP错误。
 */
esp_err_t bridge_pool_snapshot(bridge_pool_snapshot_t *out)
{
    if (out == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_pool.initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }
    s_pool.ops.lock();
    out->allocated_bytes = BRIDGE_POOL_DATA_BYTES;
    out->metadata_bytes = sizeof(s_pool) + sizeof(s_next_generation);
    out->dma_owner = s_pool.dma_owner;
    for (size_t i = 0; i < BRIDGE_POOL_KIND_COUNT; ++i)
    {
        out->capacity[i] = s_capacity[i];
        out->used[i] = s_pool.used[i];
        out->peak[i] = s_pool.peak[i];
    }
    s_pool.ops.unlock();
    return ESP_OK;
}
