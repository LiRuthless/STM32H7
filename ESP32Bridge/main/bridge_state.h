/**
 * @file bridge_state.h
 * @brief 生产桥接的固定信用与租借接口。
 * @details SPI和NET各自串行调用本端接口；实现位于bridge_state.c。
 *     共享状态由短临界区保护，不借用驱动DMA，不执行socket。
 */
#ifndef BRIDGE_STATE_H
#define BRIDGE_STATE_H
#include "spi_protocol.h"

typedef struct
{
    uint64_t generation; /**< 本连接的非零代际；由NET owner发布。 */
    uint32_t session; /**< HELLO绑定的SPI会话。 */
    bool open; /**< 控制连接存在且完成HELLO。 */
    bool subscribed; /**< 本连接具备遥测订阅。 */
    uint32_t subscription; /**< NET在每次合法SUBSCRIBE时递增的订阅身份。 */
} bridge_identity_t;

typedef struct
{
    uint32_t session; /**< 当前SPI逻辑会话。 */
    uint32_t commands; /**< 已提交COMMAND包数。 */
    uint32_t results; /**< 已提交RESULT/LOG_DATA包数。 */
    uint32_t orphan_results; /**< 无归属结果包数。 */
    uint32_t canceled_results; /**< 墓碑命中或失去连接归属的结果包数。 */
    uint32_t command_backpressure; /**< COMMAND预留被拒次数。 */
    uint32_t result_backpressure; /**< RESULT预留被拒次数。 */
    uint32_t event_backpressure; /**< EVENT预留被拒次数。 */
    uint32_t telemetry_dropped; /**< ESP32累计遥测丢弃包数。 */
    uint32_t telemetry_offline; /**< 无有效连接或订阅的丢弃包数。 */
    uint32_t telemetry_overflow; /**< 固定槽满时丢旧包数。 */
    uint32_t telemetry_cleared; /**< 生命周期清队列包数。 */
    uint32_t telemetry_failed; /**< 出口失败或发送时身份过期的包数。 */
    uint32_t telemetry_peak; /**< 遥测FIFO历史包数峰值。 */
    uint32_t event_peak; /**< EVENT FIFO历史条数峰值。 */
    uint32_t export_timeouts; /**< 独立10秒导出取消次数。 */
    uint32_t credit_mask; /**< 单请求活动信用位图，仅bit0。 */
    uint32_t tombstone_mask; /**< 单请求墓碑位图，仅bit0。 */
    bool ready; /**< 当前SPI已握手就绪。 */
    bool close_requested; /**< 缺少可信源锚点，NET须关闭并要求查询。 */
} bridge_state_snapshot_t;

/** @brief 启动前取得完整生产SPI端口表；bridge_state.c::bridge_state_spi_port。 */
bridge_spi_port_t bridge_state_spi_port(void);
/** @brief NET发布一致连接资格；bridge_state.c::bridge_state_identity。 */
void bridge_state_identity(bridge_identity_t identity);
/** @brief NET在可靠receive前取得真实信用；bridge_state.c::bridge_state_net_reserve。 */
bool bridge_state_net_reserve(const scb_frame_view_t *view, bridge_json_view_t op);
/** @brief NET在DELIVER后同步复制预留帧；bridge_state.c::bridge_state_net_commit。 */
void bridge_state_net_commit(const scb_frame_view_t *view);
/** @brief NET归还未提交预留；bridge_state.c::bridge_state_net_cancel。 */
void bridge_state_net_cancel(void);
/** @brief NET借出可靠上行；bridge_state.c::bridge_state_net_peek。 */
bool bridge_state_net_peek(scb_frame_view_t *view, bridge_json_workspace_t *workspace);
/** @brief NET报告编码及可靠序号并归还借用；bridge_state.c::bridge_state_net_consumed。 */
void bridge_state_net_consumed(bool encoded, uint32_t seq);
/** @brief NET仅在可靠层精确ACK后通知；bridge_state.c::bridge_state_net_acked。 */
void bridge_state_net_acked(uint32_t seq);
/** @brief NET每轮推进独立计时；bridge_state.c::bridge_state_poll。 */
void bridge_state_poll(uint64_t now_ms);
/** @brief 复制桥接统计；bridge_state.c::bridge_state_snapshot。 */
void bridge_state_snapshot(bridge_state_snapshot_t *snapshot);
/** @brief NET借出遥测payload；bridge_state.c::bridge_state_udp_peek。 */
bool bridge_state_udp_peek(scb_frame_view_t *view);
/** @brief NET发送前复核借用身份；bridge_state.c::bridge_state_udp_valid。 */
bool bridge_state_udp_valid(void);
/** @brief NET结束UDP发送借用并归因；bridge_state.c::bridge_state_udp_consumed。 */
void bridge_state_udp_consumed(bool sent);
/** @brief NET取走一次关闭待办；bridge_state.c::bridge_state_take_close。 */
bool bridge_state_take_close(void);
#endif
