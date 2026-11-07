/**
 * @file reliable_link.c
 * @brief 独立可靠窗口与重试状态。
 * @details 传输owner输入完整帧、预留事实、完成发送及毫秒时间，取得接管/重发/重建动作。
 *     SPI/NET分别持有实例并串行调用；本模块不访问驱动、ISR或socket，ACK不表示业务生效。
 *
 * @note Phase02 G4：receive_validated复用入帧验证；prepare_validated复用生产队列负载验证。
 */
#include "reliable_link.h"

/**
 * @brief 重置一个传输独占的可靠窗口。
 * @details 整体重置序号/缓存/计时；seq从1开始，无会话或非SPI/TCP时标记dead，不操作socket或DMA。
 * @param[in,out] l 可为空，空时无操作；其他情况必须由所属任务独占。
 * @param[in] transport SPI或TCP，其他值生成dead实例。
 * @param[in] session 会话标识，零表示无有效链路。
 */
void scb_link_reset(scb_link_t *l, scb_transport_t transport, uint32_t session)
{
    if (!l)
    {
        return;
    }
    *l = (scb_link_t)
    {
        .transport = transport,
        .session_id = session,
        .next_seq = 1,
        .dead = !session || (transport != SCB_SPI && transport != SCB_TCP)
    };
}

/**
 * @brief 校验入帧并推进可靠接管与精确ACK。
 * @details 先清空有效out，完整解码并检查会话和序号；可靠重复按完整身份比较。合法帧携带的精确ACK可独立于下游背压释放已发送TX；新可靠帧只有reserved成立才更新接管ACK并发布
 *     视图。
 * @param[in,out] l 独占链路状态；空或dead返回重建，不处理帧。
 * @param[in] b 输入完整帧；交付视图使用完前保持有效且不变。
 * @param[in] n 输入字节数，按实例传输校验。
 * @param[in] reserved 表示已取得真实下游槽所有权，而非仅观察空位。
 * @param[in,out] w JSON帧所需独占工作区，其他类型可为空。
 * @param[out] out 非空；进入清零，只有DELIVER写借用视图。
 * @return 返回DELIVER、重复、旧帧、背压、非法或REBUILD；未交付不发布视图。
 * @note DELIVER后先提交复制再允许发出ACK；失败/重复时由调用方撤销预留。
 */
scb_link_result_t scb_link_receive(scb_link_t *l, const uint8_t *b, size_t n, bool reserved,
    bridge_json_workspace_t *w, scb_frame_view_t *out)
{
    if (!out)
    {
        return SCB_LINK_INVALID;
    }
    *out = (scb_frame_view_t){0};
    if (!l || l->dead)
    {
        return SCB_LINK_REBUILD;
    }
    scb_frame_view_t v;
    if (scb_decode(b, n, l->transport, w, &v) != SCB_OK)
    {
        return SCB_LINK_INVALID;
    }
    return scb_link_receive_validated(l, b, &v, reserved, out);
}

/**
 * @brief 使用已验证帧推进可靠状态，不重复解码。
 * @details 仍检查会话/序号、精确重复身份及真实预留，合法帧才处理反向ACK。
 * @param[in,out] l 所属任务独占可靠窗口。
 * @param[in] b 与validated对应的完整原始帧，调用期间有效且不变。
 * @param[in] validated 完整scb_decode成功的视图，不能为空且不得修改。
 * @param[in] reserved 下游真实预留事实。
 * @param[out] out 非空交付视图；先清空，仅DELIVER发布。
 * @return 实际接管、重复、背压、非法或重建结果。
 * @pre b与validated必须对应同一帧及l的传输，原始字节不能被重用。
 */
scb_link_result_t scb_link_receive_validated(scb_link_t *l, const uint8_t *b,
    const scb_frame_view_t *validated, bool reserved, scb_frame_view_t *out)
{
    if (!out)
    {
        return SCB_LINK_INVALID;
    }
    const scb_frame_view_t v = validated ? *validated : (scb_frame_view_t){0};
    *out = (scb_frame_view_t){0};
    if (!l || l->dead)
    {
        return SCB_LINK_REBUILD;
    }
    if (!b || !validated)
    {
        return SCB_LINK_INVALID;
    }
    const scb_header_t *h = &v.header;
    if (h->session_id != l->session_id || h->seq == 0 || h->seq == UINT32_MAX
        || h->type == SCB_HELLO || h->type == SCB_HELLO_ACK)
    {
        return SCB_LINK_INVALID;
    }
    const size_t identity = SCB_HEADER_BYTES + h->payload_len;
    scb_link_result_t result = SCB_LINK_DELIVER;
    if (h->flags == SCB_RELIABLE)
    {
        if (h->seq < l->rx_reliable_seq)
        {
            return SCB_LINK_STALE;
        }
        if (h->seq == l->rx_reliable_seq)
        {
            if (identity != l->rx_length)
            {
                return SCB_LINK_INVALID;
            }
            for (size_t i = 0; i < identity; ++i)
            {
                if (b[i] != l->last_rx[i])
                {
                    return SCB_LINK_INVALID;
                }
            }
            result = SCB_LINK_DUPLICATE;
        }
        else if (!reserved)
        {
            result = SCB_LINK_BACKPRESSURE;
        }
    }
    else if (h->seq <= l->rx_high)
    {
        result = SCB_LINK_STALE;
    }

    /* ACK处理独立于下游背压，但必须来自合法当前帧，且对应可靠图像已完整发送。 */
    if (l->pending && l->sent && h->ack_seq == l->tx_seq)
    {
        l->pending = false;
        l->sending = false;
        l->tx_length = 0;
    }
    if (result != SCB_LINK_DELIVER)
    {
        return result;
    }
    if (h->flags == SCB_RELIABLE)
    {
        for (size_t i = 0; i < identity; ++i)
        {
            l->last_rx[i] = b[i];
        }
        l->rx_length = identity;
        l->rx_reliable_seq = h->seq;
        l->ack_seq = h->seq;
    }
    if (h->seq > l->rx_high)
    {
        l->rx_high = h->seq;
    }
    *out = v;
    return result;
}

/**
 * @brief 检查可靠发送窗口及输出范围，并准备新的传输身份。
 * @details 不消耗序号；近回绕时失效窗口，编码成功后才由commit_prepared提交。
 * @param[in,out] l 所属任务独占窗口。
 * @param[in] header 待发业务头，调用期间借用。
 * @param[out] out 独立输出，不得覆盖窗口。
 * @param[in] capacity 输出可写容量。
 * @param[out] used 非空，进入清零。
 * @param[out] prepared 非空，成功写新身份头。
 * @return OK、INVALID、BUSY或REBUILD。
 */
static scb_link_result_t prepare_header(scb_link_t *l, const scb_header_t *header,
    uint8_t *out, size_t capacity, size_t *used, scb_header_t *prepared)
{
    if (!used)
    {
        return SCB_LINK_INVALID;
    }
    *used = 0;
    if (!l || l->dead)
    {
        return SCB_LINK_REBUILD;
    }
    if (!header || !out || header->type == SCB_HELLO || header->type == SCB_HELLO_ACK)
    {
        return SCB_LINK_INVALID;
    }
    if (l->next_seq == UINT32_MAX)
    {
        l->dead = true;
        return SCB_LINK_REBUILD;
    }
    if (header->flags == SCB_RELIABLE && l->pending)
    {
        return SCB_LINK_BUSY;
    }
    /* 输出不得别名改写可靠层的不可变重发图像。 */
    const uintptr_t d = (uintptr_t)out;
    const uintptr_t start = (uintptr_t)l;
    if (d > UINTPTR_MAX - capacity || (d < start + sizeof(*l) && start < d + capacity))
    {
        return SCB_LINK_INVALID;
    }
    *prepared = *header;
    prepared->session_id = l->session_id;
    prepared->seq = l->next_seq;
    prepared->ack_seq = l->ack_seq;
    return SCB_LINK_OK;
}

/**
 * @brief 提交成功编码的序号和不可变可靠重发图像。
 * @details 仅编码成功后调用；不触发发送或启动重试计时。
 * @param[in,out] l 所属任务独占窗口。
 * @param[in] h 本次编码使用的传输头。
 * @param[in] out 已成功编码的独立图像。
 * @param[in] used 本次图像的有效长度，非空。
 * @return SCB_LINK_OK。
 */
static scb_link_result_t commit_prepared(scb_link_t *l, const scb_header_t *h,
    const uint8_t *out, const size_t *used)
{
    ++l->next_seq;
    if (h->flags == SCB_RELIABLE)
    {
        for (size_t i = 0; i < *used; ++i)
        {
            l->tx[i] = out[i];
        }
        l->tx_length = *used;
        l->tx_seq = h->seq;
        l->retries = 0;
        l->pending = true;
        l->sending = true;
        l->sent = false;
    }
    return SCB_LINK_OK;
}

/**
 * @brief 编码新出帧并在需要时保留不可变重发图像。
 * @details 覆盖session/seq/ack，拒绝握手类型和窗口冲突；成功编码后才递增seq并缓存可靠图像。近回绕标记dead，IDLE可通过被占可靠窗口。
 * @param[in,out] l 所属任务独占状态；空或dead返回REBUILD。
 * @param[in] header 非空业务字段，调用期间借用。
 * @param[in] p 负载指针，按codec要求有效且不与输出重叠。
 * @param[out] out 非空独立应用缓冲，不可与链路对象重叠或由驱动持有。
 * @param[in] capacity 可写容量，字节。
 * @param[in,out] w JSON负载所需独占工作区。
 * @param[out] used 非空；进入清零，成功写实际编码字节数。
 * @return 成功SCB_LINK_OK，非法、窗口忙或需要重建返回对应结果。
 */
scb_link_result_t scb_link_prepare(scb_link_t *l, const scb_header_t *header, const uint8_t *p,
    uint8_t *out, size_t capacity, bridge_json_workspace_t *w, size_t *used)
{
    scb_header_t h;
    const scb_link_result_t result = prepare_header(l, header, out, capacity, used, &h);
    if (result != SCB_LINK_OK)
    {
        return result;
    }
    if (scb_encode(&h, p, l->transport, out, capacity, w, used) != SCB_OK)
    {
        return SCB_LINK_INVALID;
    }
    return commit_prepared(l, &h, out, used);
}

/**
 * @brief 为已验证内部负载分配新传输身份并编码。
 * @details 共用窗口检查和提交路径，省略负载全文重验。
 * @param[in,out] l 所属任务独占窗口。
 * @param[in] frame 完整验证且类型/负载结构未变的内部帧。
 * @param[out] out 独立可写目标，不能覆盖窗口或负载。
 * @param[in] capacity 输出容量，字节。
 * @param[out] used 非空，失败清零，成功提交长度。
 * @return OK、INVALID、BUSY或REBUILD。
 * @pre 仅用于已验证且借用期间保持有效的帧，不用于本地新生成负载。
 */
scb_link_result_t scb_link_prepare_validated(scb_link_t *l, const scb_frame_view_t *frame,
    uint8_t *out, size_t capacity, size_t *used)
{
    scb_header_t h;
    const scb_link_result_t result
        = prepare_header(l, frame ? &frame->header : NULL, out, capacity, used, &h);
    if (result != SCB_LINK_OK)
    {
        return result;
    }
    const scb_frame_view_t prepared = {h, frame->payload};
    if (scb_encode_validated(&prepared, l->transport, out, capacity, used) != SCB_OK)
    {
        return SCB_LINK_INVALID;
    }
    return commit_prepared(l, &h, out, used);
}

/**
 * @brief 确认可靠图像已经全部发送完成。
 * @details 匹配当前待发seq才清sending并记录毫秒起点；已有一次完成发送则增加重试次数，首次不计重发。ACK后的旧完成通知不产生作用。
 * @param[in,out] l 所属任务独占链路，空或失效拒绝。
 * @param[in] seq 已实际完成发送的可靠帧序号。
 * @param[in] now 单调毫秒时间，完整发送结束时采样。
 * @return 接受完成通知返回true，过期、不匹配或不在发送中返回false。
 * @note 部分发送或失败不得调用本接口。
 */
bool scb_link_sent(scb_link_t *l, uint32_t seq, uint64_t now)
{
    if (!l || l->dead || !l->pending || !l->sending || l->tx_seq != seq)
    {
        return false;
    }
    if (l->sent)
    {
        ++l->retries;
    }
    l->sent = true;
    l->sending = false;
    l->last_send_ms = now;
    return true;
}

/**
 * @brief 轮询重试截止并借出原可靠图像。
 * @details 只有pending、已完整发送且不在发送时才检查200ms；回退时间不触发重试。达到三次重发后再到期输出REBUILD，否则置sending并借出不可变缓存。
 * @param[in,out] l 所属任务独占状态，空或dead要求重建。
 * @param[in] now 当前单调毫秒数。
 * @param[out] b 非空指针槽；两个输出都有效才先清NULL，仅RESEND借出图像到下次owner调用前。
 * @param[out] n 非空长度槽；两个输出都有效才先清零，仅RESEND写字节数。
 * @return 返回OK、RESEND、REBUILD或参数非法。
 * @note 借出图像须复制到应用所有的发送缓冲，不可覆写驱动持有数据。
 */
scb_link_result_t scb_link_poll(scb_link_t *l, uint64_t now, const uint8_t **b, size_t *n)
{
    if (!b || !n)
    {
        return SCB_LINK_INVALID;
    }
    *b = NULL;
    *n = 0;
    if (!l || l->dead)
    {
        return SCB_LINK_REBUILD;
    }
    if (!l->pending || l->sending || !l->sent || now < l->last_send_ms
        || now - l->last_send_ms < SCB_RETRY_MS)
    {
        return SCB_LINK_OK;
    }
    if (l->retries == SCB_MAX_RETRIES)
    {
        l->dead = true;
        return SCB_LINK_REBUILD;
    }
    l->sending = true;
    *b = l->tx;
    *n = l->tx_length;
    return SCB_LINK_RESEND;
}
