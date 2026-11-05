#include "reliable_link.h"

void scb_link_reset(scb_link_t *l, scb_transport_t transport, uint32_t session)
{
    if (!l) return;
    *l = (scb_link_t){ .transport = transport, .session_id = session, .next_seq = 1,
        .dead = !session || (transport != SCB_SPI && transport != SCB_TCP) };
}

scb_link_result_t scb_link_receive(scb_link_t *l, const uint8_t *b, size_t n,
                                  bool reserved, bridge_json_workspace_t *w, scb_frame_view_t *out)
{
    if (!out) return SCB_LINK_INVALID;
    *out = (scb_frame_view_t){0};
    if (!l || l->dead) return SCB_LINK_REBUILD;
    scb_frame_view_t v;
    if (scb_decode(b, n, l->transport, w, &v) != SCB_OK) return SCB_LINK_INVALID;
    const scb_header_t *h = &v.header;
    if (h->session_id != l->session_id || h->seq == 0 || h->seq == UINT32_MAX ||
        h->type == SCB_HELLO || h->type == SCB_HELLO_ACK) return SCB_LINK_INVALID;
    const size_t identity = SCB_HEADER_BYTES + h->payload_len;
    scb_link_result_t result = SCB_LINK_DELIVER;
    if (h->flags == SCB_RELIABLE) {
        if (h->seq < l->rx_reliable_seq) return SCB_LINK_STALE;
        if (h->seq == l->rx_reliable_seq) {
            if (identity != l->rx_length) return SCB_LINK_INVALID;
            for (size_t i = 0; i < identity; ++i) if (b[i] != l->last_rx[i]) return SCB_LINK_INVALID;
            result = SCB_LINK_DUPLICATE;
        } else if (!reserved) result = SCB_LINK_BACKPRESSURE;
    } else if (h->seq <= l->rx_high) result = SCB_LINK_STALE;

    /* ACK is independent of downstream backpressure, but can only come from
     * a valid current frame and a reliable image actually sent before. */
    if (l->pending && l->sent && h->ack_seq == l->tx_seq) {
        l->pending = false; l->sending = false; l->tx_length = 0;
    }
    if (result != SCB_LINK_DELIVER) return result;
    if (h->flags == SCB_RELIABLE) {
        for (size_t i = 0; i < identity; ++i) l->last_rx[i] = b[i];
        l->rx_length = identity; l->rx_reliable_seq = h->seq; l->ack_seq = h->seq;
    }
    if (h->seq > l->rx_high) l->rx_high = h->seq;
    *out = v;
    return result;
}

scb_link_result_t scb_link_prepare(scb_link_t *l, const scb_header_t *header,
                                  const uint8_t *p, uint8_t *out, size_t capacity,
                                  bridge_json_workspace_t *w, size_t *used)
{
    if (!used) return SCB_LINK_INVALID;
    *used = 0;
    if (!l || l->dead) return SCB_LINK_REBUILD;
    if (!header || !out || header->type == SCB_HELLO || header->type == SCB_HELLO_ACK) return SCB_LINK_INVALID;
    if (l->next_seq == UINT32_MAX) { l->dead = true; return SCB_LINK_REBUILD; }
    if (header->flags == SCB_RELIABLE && l->pending) return SCB_LINK_BUSY;
    /* Do not let an application alias/rewrite the immutable retransmit image. */
    const uintptr_t d = (uintptr_t)out, start = (uintptr_t)l;
    if (d > UINTPTR_MAX - capacity || (d < start + sizeof(*l) && start < d + capacity)) return SCB_LINK_INVALID;
    scb_header_t h = *header;
    h.session_id = l->session_id; h.seq = l->next_seq; h.ack_seq = l->ack_seq;
    if (scb_encode(&h, p, l->transport, out, capacity, w, used) != SCB_OK) return SCB_LINK_INVALID;
    ++l->next_seq;
    if (h.flags == SCB_RELIABLE) {
        for (size_t i = 0; i < *used; ++i) l->tx[i] = out[i];
        l->tx_length = *used; l->tx_seq = h.seq; l->retries = 0;
        l->pending = true; l->sending = true; l->sent = false;
    }
    return SCB_LINK_OK;
}

bool scb_link_sent(scb_link_t *l, uint32_t seq, uint64_t now)
{
    if (!l || l->dead || !l->pending || !l->sending || l->tx_seq != seq) return false;
    if (l->sent) ++l->retries;
    l->sent = true; l->sending = false; l->last_send_ms = now;
    return true;
}

scb_link_result_t scb_link_poll(scb_link_t *l, uint64_t now, const uint8_t **b, size_t *n)
{
    if (!b || !n) return SCB_LINK_INVALID;
    *b = NULL; *n = 0;
    if (!l || l->dead) return SCB_LINK_REBUILD;
    if (!l->pending || l->sending || !l->sent || now < l->last_send_ms || now - l->last_send_ms < SCB_RETRY_MS)
        return SCB_LINK_OK;
    if (l->retries == SCB_MAX_RETRIES) { l->dead = true; return SCB_LINK_REBUILD; }
    l->sending = true;
    *b = l->tx; *n = l->tx_length;
    return SCB_LINK_RESEND;
}
