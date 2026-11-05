/**
 * @file spi_protocol.c
 * @brief SPI握手与可靠业务端口适配。
 * @details SPI任务输入已回收帧和完成事实，调用codec/session/link及启动绑定端口，准备下一图像。
 *     实例和工作区由SPI任务独占；poll可在驱动持有期运行但不接触DMA。
 *     端口回调有界同步复制，当前无Phase05生产消费者时保持真实背压和丢弃。
 */
#include "spi_protocol.h"

/**
 * @brief 校验可选SPI回调是否成组绑定。
 * @details NULL有效；接管三回调以及出站两回调各自全有或全无，避免半绑定无法提交/撤销。
 * @param[in] p 可为空，只读借用的端口表。
 * @return 组合完整返回true，否则false。
 */
bool bridge_spi_port_valid(const bridge_spi_port_t *p)
{
    return !p
        || (((!p->reserve && !p->commit && !p->cancel) || (p->reserve && p->commit && p->cancel))
            && ((!p->peek_tx && !p->tx_consumed) || (p->peek_tx && p->tx_consumed)));
}

/**
 * @brief 初始化SPI协议实例及可选端口。
 * @details 先整体清零，仅复制合法表，再建立等待HELLO的session及dead可靠窗口；不访问DMA。
 * @param[out] p 非空且由SPI任务独占的协议对象。
 * @param[in] port 可为空；无效组合不复制，context与回调依赖须在服务期有效。
 */
void bridge_spi_protocol_init(bridge_spi_protocol_t *p, const bridge_spi_port_t *port)
{
    *p = (bridge_spi_protocol_t){0};
    if (port && bridge_spi_port_valid(port))
    {
        p->port = *port;
    }
    scb_session_init(&p->session);
    scb_link_reset(&p->link, SCB_SPI, 0);
}

/**
 * @brief 同步通知SPI端口当前逻辑会话状态。
 * @details 仅在绑定回调时调用，不取得网络状态，不等待socket；回调必须有界。
 * @param[in,out] p 非空且由SPI任务独占的实例，回调可更新外部端口状态。
 */
static void publish_session(bridge_spi_protocol_t *p)
{
    if (p->port.session_changed)
    {
        p->port.session_changed(
            p->port.context, p->session.session_id, p->session.phase == SCB_SESSION_READY);
    }
}

/**
 * @brief 撤销SPI逻辑会话并重置独立可靠窗口。
 * @details 先累计重建次数，再失效session、reset link，最后发布会话；不回收DMA、不复位硬件。
 * @param[in,out] p 非空SPI任务独占实例。
 */
static void rebuild(bridge_spi_protocol_t *p)
{
    ++p->stats.rebuilds;
    scb_session_spi_lost(&p->session);
    scb_link_reset(&p->link, SCB_SPI, 0);
    publish_session(p);
}

/**
 * @brief 按当前阶段决定是否撤销SPI逻辑会话。
 * @details 仅在非初始等待状态调用rebuild，避免重复失效增加统计；不操作DMA或socket。
 * @param[in,out] p 非空SPI任务独占实例。
 */
void bridge_spi_protocol_lost(bridge_spi_protocol_t *p)
{
    if (p->session.phase != SCB_WAIT_HELLO || p->session.session_id != 0)
    {
        rebuild(p);
    }
}

/**
 * @brief 在不借用DMA的情况下推进重试状态。
 * @details 仅READY时轮询；REBUILD触发逻辑重建，RESEND只标记link.sending，不立即复制图像。等待期间的新精确ACK仍可取消待重发。
 * @param[in,out] p 非空SPI任务独占实例。
 * @param[in] now 当前单调毫秒时间。
 * @note 允许在驱动持有事务时调用，不传入也不访问DMA。
 */
void bridge_spi_protocol_poll(bridge_spi_protocol_t *p, uint64_t now)
{
    if (p->session.phase != SCB_SESSION_READY)
    {
        return;
    }
    const uint8_t *ignored;
    size_t length;
    if (scb_link_poll(&p->link, now, &ignored, &length) == SCB_LINK_REBUILD)
    {
        rebuild(p);
    }
    /* RESEND仅标记sending，原图像待驱动返还描述后才复制；期间精确ACK可取消待办。 */
}

/**
 * @brief 向可靠窗口报告完整物理发送。
 * @details 仅complete、reliable且READY时转发sent；实际接受重发完成时增加重试统计，截短不启动重试计时。
 * @param[in,out] p 非空SPI任务独占实例。
 * @param[in] meta 上次prepare返回的发送身份。
 * @param[in] complete 回收长度是否恰为8192bit。
 * @param[in] now 完成时的单调毫秒数。
 */
void bridge_spi_protocol_sent(
    bridge_spi_protocol_t *p, bridge_spi_tx_meta_t meta, bool complete, uint64_t now)
{
    if (!complete || !meta.reliable || p->session.phase != SCB_SESSION_READY)
    {
        return;
    }
    const bool retry = p->link.sent;
    if (scb_link_sent(&p->link, meta.seq, now) && retry)
    {
        ++p->stats.retries;
    }
}

/**
 * @brief 在已回收DMA中验证并接管入站帧。
 * @details 先解码统计，再处理握手及至多四次随机候选；READY业务先验证方向/请求身份，再reserve和可靠receive。交付时同步commit，重复/无效撤销预留，遥测由回调同步复
 *     制或计丢弃；无消费者不伪造ACK。
 * @param[in,out] p 非空SPI任务独占实例。
 * @param[in] rx 非空1024字节应用持有输入，回调返回前借用且不变。
 * @param[out] tx 非空1024字节应用持有输出，握手可能编码；失败不承诺整体不变。
 * @param[in,out] w 非空独占JSON工作区。
 * @param[in] random 可为空；非空须有界地提供生产随机候选。
 * @param[in] random_context 原样传给random，可为空，调用期间有效。
 * @note 仅原事务成功回收后调用；端口不得保留RX视图，预留成功须保证commit可完成。
 */
void bridge_spi_protocol_receive(bridge_spi_protocol_t *p, const uint8_t *rx, uint8_t *tx,
    bridge_json_workspace_t *w, bridge_spi_random_fn random, void *random_context)
{
    scb_frame_view_t v;
    const scb_error_t error = scb_decode(rx, SCB_SPI_BYTES, SCB_SPI, w, &v);
    if (error != SCB_OK)
    {
        if (error == SCB_CRC)
        {
            ++p->stats.crc_errors;
        }
        else
        {
            ++p->stats.invalid;
        }
        return;
    }
    ++p->stats.received;
    if (v.header.type == SCB_HELLO || p->session.phase != SCB_SESSION_READY)
    {
        const uint32_t old_session = p->session.session_id;
        const scb_spi_phase_t old_phase = p->session.phase;
        size_t used;
        scb_session_result_t result
            = scb_session_spi(&p->session, rx, SCB_SPI_BYTES, 0, w, tx, SCB_SPI_BYTES, &used);
        for (unsigned attempt = 0; result == SCB_SESSION_NEED_RANDOM && random && attempt < 4u;
            ++attempt)
        {
            result = scb_session_spi(&p->session, rx, SCB_SPI_BYTES, random(random_context), w, tx,
                SCB_SPI_BYTES, &used);
        }
        if (result == SCB_SESSION_NEED_RANDOM)
        {
            ++p->stats.rng_failures;
            return;
        }
        if (result != SCB_SESSION_REPLY && result != SCB_SESSION_ESTABLISHED)
        {
            ++p->stats.invalid;
            return;
        }
        if (p->session.session_id != old_session || result == SCB_SESSION_ESTABLISHED)
        {
            scb_link_reset(&p->link, SCB_SPI, p->session.session_id);
        }
        if (result == SCB_SESSION_ESTABLISHED)
        {
            scb_frame_view_t ignored;
            (void)scb_link_receive(&p->link, rx, SCB_SPI_BYTES, false, w, &ignored);
        }
        if (old_session != p->session.session_id || old_phase != p->session.phase)
        {
            publish_session(p);
        }
        return;
    }
    const uint8_t type = v.header.type;
    if (v.header.session_id != p->session.session_id || !v.header.seq || v.header.seq == UINT32_MAX
        || (type != SCB_IDLE && type != SCB_TELEMETRY && type != SCB_RESULT && type != SCB_EVENT
            && type != SCB_LOG_DATA)
        || ((type == SCB_EVENT || type == SCB_TELEMETRY) && v.header.request_id != 0)
        || ((type == SCB_RESULT || type == SCB_LOG_DATA) && v.header.request_id == 0))
    {
        ++p->stats.invalid;
        return;
    }
    const bool reserved
        = v.header.flags == SCB_RELIABLE && p->port.reserve && p->port.reserve(p->port.context, &v);
    const scb_link_result_t result = scb_link_receive(&p->link, rx, SCB_SPI_BYTES, reserved, w, &v);
    if (result == SCB_LINK_DELIVER)
    {
        if (reserved)
        {
            p->port.commit(p->port.context, &v);
            ++p->stats.delivered;
        }
        else if (type == SCB_TELEMETRY)
        {
            ++p->stats.telemetry_packets;
            p->stats.telemetry_records += v.header.record_count;
            if (!p->port.telemetry || !p->port.telemetry(p->port.context, &v))
            {
                ++p->stats.telemetry_dropped;
                p->stats.records_dropped += v.header.record_count;
            }
        }
    }
    else
    {
        if (reserved)
        {
            p->port.cancel(p->port.context);
        }
        if (result == SCB_LINK_DUPLICATE || result == SCB_LINK_STALE)
        {
            ++p->stats.duplicates;
        }
        else if (result == SCB_LINK_BACKPRESSURE)
        {
            ++p->stats.backpressure;
        }
        else if (result == SCB_LINK_INVALID)
        {
            ++p->stats.invalid;
        }
    }
}

/**
 * @brief 在应用持有的缓冲中准备下一SPI图像。
 * @details 先清发送元信息并poll；握手阶段生成对应图像，重发复制原缓存，否则取可选COMMAND并通知是否编码成功，最后生成IDLE。近回绕重建后递归重新准备，不改变原可靠字节。
 * @param[in,out] p 非空SPI任务独占实例。
 * @param[in] now 当前单调毫秒数。
 * @param[out] tx 非空1024字节应用所有缓冲；失败不承诺原内容，驱动持有时禁止调用。
 * @param[in,out] w 非空独占JSON工作区。
 * @param[out] meta 非空；先清零，可靠图像成功时写seq及reliable。
 * @return 取得可排队图像返回true，否则false。
 */
bool bridge_spi_protocol_prepare(bridge_spi_protocol_t *p, uint64_t now, uint8_t *tx,
    bridge_json_workspace_t *w, bridge_spi_tx_meta_t *meta)
{
    *meta = (bridge_spi_tx_meta_t){0};
    bridge_spi_protocol_poll(p, now);
    size_t used = 0;
    if (p->session.phase == SCB_WAIT_IDLE)
    {
        return scb_session_spi_pending_reply(&p->session, w, tx, SCB_SPI_BYTES, &used);
    }
    if (p->session.phase == SCB_WAIT_HELLO)
    {
        const scb_header_t idle = {.type = SCB_IDLE};
        return scb_encode(&idle, NULL, SCB_SPI, tx, SCB_SPI_BYTES, w, &used) == SCB_OK;
    }
    if (p->link.pending && p->link.sending)
    {
        for (size_t i = 0; i < p->link.tx_length; ++i)
        {
            tx[i] = p->link.tx[i];
        }
        meta->seq = p->link.tx_seq;
        meta->reliable = true;
        return true;
    }
    scb_frame_view_t outbound;
    if (!p->link.pending && p->port.peek_tx && p->port.peek_tx(p->port.context, &outbound))
    {
        const bool direction_ok = outbound.header.type == SCB_COMMAND
            && outbound.header.flags == SCB_RELIABLE && outbound.header.request_id != 0;
        const scb_link_result_t result = direction_ok
            ? scb_link_prepare(
                  &p->link, &outbound.header, outbound.payload, tx, SCB_SPI_BYTES, w, &used)
            : SCB_LINK_INVALID;
        p->port.tx_consumed(p->port.context, result == SCB_LINK_OK);
        if (result == SCB_LINK_OK)
        {
            meta->seq = p->link.tx_seq;
            meta->reliable = true;
            return true;
        }
        if (result == SCB_LINK_REBUILD)
        {
            rebuild(p);
            return bridge_spi_protocol_prepare(p, now, tx, w, meta);
        }
        ++p->stats.invalid_outbound;
    }
    const scb_header_t idle = {.type = SCB_IDLE};
    const scb_link_result_t result
        = scb_link_prepare(&p->link, &idle, NULL, tx, SCB_SPI_BYTES, w, &used);
    if (result == SCB_LINK_REBUILD)
    {
        rebuild(p);
        return bridge_spi_protocol_prepare(p, now, tx, w, meta);
    }
    return result == SCB_LINK_OK;
}
