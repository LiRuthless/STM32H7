/**
 * @file spi_protocol.h
 * @brief SPI握手与可靠业务端口适配。
 * @details 声明SPI握手与可靠业务端口适配接口；实现及完整参数/错误语义位于spi_protocol.c。
 *     调用方按接口约束串行访问owner状态，头文件不定义线图像或运行期服务。
 */
#ifndef BRIDGE_SPI_PROTOCOL_H
#define BRIDGE_SPI_PROTOCOL_H
#include "link_session.h"
#include "reliable_link.h"

/*
 * 端口启动前绑定，SPI owner串行调用且均须有界、无socket。
 * RX视图只借用到回调返回，commit/telemetry必须同步复制到已持有槽；
 * reserve成功必须保证commit可完成，重复/旧帧由cancel归还预留。
 */
typedef struct
{
    void *context; /**< 启动绑定上下文，依赖须存续至服务结束。 */
    bool (*reserve)(
        void *, const scb_frame_view_t *); /**< 为可靠帧取得真实固定槽；不能仅观察空位。 */
    void (*commit)(
        void *, const scb_frame_view_t *); /**< 同步复制借用视图至已预留槽；成功预留后不得失败。 */
    void (*cancel)(void *); /**< 归还本次重复/无效/未交付帧的预留。 */
    bool (*telemetry)(void *, const scb_frame_view_t *); /**< 同步复制遥测视图，false表示丢弃。 */
    bool (*peek_tx)(void *, scb_frame_view_t *); /**< 借出待发COMMAND视图到tx_consumed回调。 */
    void (*tx_consumed)(
        void *, bool encoded); /**< 通知peek图像是否成功编码，调用方结束本次借用。 */
    /**
     * @brief 有界发布SPI会话/ready变化，不操作socket。
     */
    void (*session_changed)(void *, uint32_t session_id, bool ready);
} bridge_spi_port_t;

typedef struct
{
    uint32_t received; /**< 成功完整解码的SPI入帧数。 */
    uint32_t crc_errors; /**< 入帧CRC不符次数。 */
    uint32_t invalid; /**< 其他格式、方向或会话拒绝次数。 */
    uint32_t duplicates; /**< 可靠重复或旧帧累计次数。 */
    uint32_t backpressure; /**< 可靠下游未预留槽导致的背压次数。 */
    uint32_t delivered; /**< 同步commit可靠帧次数。 */
    uint32_t telemetry_packets; /**< 已交付到遥测分支的包数。 */
    uint32_t telemetry_records; /**< 这些遥测包中记录条数总和。 */
    uint32_t telemetry_dropped; /**< 无遥测消费者或消费者拒绝的包数。 */
    uint32_t records_dropped; /**< 上述丢弃包内记录条数。 */
    uint32_t retries; /**< 实际接受的可靠重发完成次数。 */
    uint32_t rebuilds; /**< 逻辑SPI会话重建次数。 */
    uint32_t rng_failures; /**< 握手未取得合格随机候选次数。 */
    uint32_t invalid_outbound; /**< 待发COMMAND方向或编码被拒绝次数。 */
} bridge_spi_protocol_stats_t;

typedef struct
{
    scb_session_t session; /**< SPI任务独占的握手状态。 */
    scb_link_t link; /**< SPI任务独占的可靠窗口及缓存。 */
    bridge_spi_port_t port; /**< 启动时复制的有界消费者表。 */
    bridge_spi_protocol_stats_t stats; /**< SPI任务累计计数，发布时复制到快照。 */
} bridge_spi_protocol_t;

typedef struct
{
    uint32_t seq; /**< prepare可靠图像序号，非可靠时为零。 */
    bool reliable; /**< 该图像是否需向可靠层报告完整发送。 */
} bridge_spi_tx_meta_t;

typedef uint32_t (*bridge_spi_random_fn)(void *context);

/**
 * @brief 校验可选SPI回调是否成组绑定。
 * @note spi_protocol.c::bridge_spi_port_valid；只读端口组合，允许启动前检查，不访问DMA。
 */
bool bridge_spi_port_valid(const bridge_spi_port_t *port);
/**
 * @brief 初始化SPI协议实例及可选端口。
 * @note spi_protocol.c::bridge_spi_protocol_init；仅SPI owner串行调用；驱动持有期仅poll/lost不接触DMA。
 */
void bridge_spi_protocol_init(bridge_spi_protocol_t *state, const bridge_spi_port_t *port);
/**
 * @brief 在不借用DMA的情况下推进重试状态。
 * @note spi_protocol.c::bridge_spi_protocol_poll；仅SPI owner串行调用；驱动持有期仅poll/lost不接触DMA。
 */
void bridge_spi_protocol_poll(bridge_spi_protocol_t *state, uint64_t now_ms);
/**
 * @brief 按当前阶段决定是否撤销SPI逻辑会话。
 * @note spi_protocol.c::bridge_spi_protocol_lost；仅SPI owner串行调用；驱动持有期仅poll/lost不接触DMA。
 */
void bridge_spi_protocol_lost(bridge_spi_protocol_t *state);
/**
 * @brief 向可靠窗口报告完整物理发送。
 * @note spi_protocol.c::bridge_spi_protocol_sent；仅SPI owner串行调用；驱动持有期仅poll/lost不接触DMA。
 */
void bridge_spi_protocol_sent(
    bridge_spi_protocol_t *state, bridge_spi_tx_meta_t meta, bool complete, uint64_t now_ms);
/**
 * @brief 在已回收DMA中验证并接管入站帧。
 * @note spi_protocol.c::bridge_spi_protocol_receive；仅SPI owner串行调用；驱动持有期仅poll/lost不接触DMA。
 */
void bridge_spi_protocol_receive(bridge_spi_protocol_t *state, const uint8_t *rx,
    uint8_t *application_tx, bridge_json_workspace_t *workspace, bridge_spi_random_fn random,
    void *random_context);
/**
 * @brief 在应用持有的缓冲中准备下一SPI图像。
 * @note spi_protocol.c::bridge_spi_protocol_prepare；仅SPI owner串行调用；驱动持有期仅poll/lost不接触DMA。
 */
bool bridge_spi_protocol_prepare(bridge_spi_protocol_t *state, uint64_t now_ms,
    uint8_t *application_tx, bridge_json_workspace_t *workspace, bridge_spi_tx_meta_t *meta);
#endif
