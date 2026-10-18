#include "link_session.h"

void scb_session_init(scb_session_t *s)
{
    if (s) *s = (scb_session_t){0};
}

static bool hello(const scb_frame_view_t *v, const char *role, bridge_json_workspace_t *w)
{
    const scb_header_t *h = &v->header;
    if (h->type != SCB_HELLO || h->session_id || h->seq || h->ack_seq || h->request_id) return false;
    bridge_json_view_t value;
    uint32_t protocol;
    return bridge_json_object_get(v->payload, h->payload_len, "role", w, &value) &&
           bridge_json_string_equal(value, role) &&
           bridge_json_object_get(v->payload, h->payload_len, "protocol", w, &value) &&
           bridge_json_u32(value, &protocol) && protocol == 1;
}

static size_t append(uint8_t *b, size_t n, const char *literal)
{
    while (*literal) b[n++] = (uint8_t)*literal++;
    return n;
}

static bool reply(uint32_t id, bool ready, scb_transport_t transport,
                   bridge_json_workspace_t *w, uint8_t *out, size_t capacity, size_t *used)
{
    uint8_t payload[96], digits[10];
    size_t n = append(payload, 0, "{\"protocol\":1,\"session_id\":");
    size_t count = 0;
    uint32_t value = id;
    do { digits[count++] = (uint8_t)('0' + value % 10u); value /= 10u; } while (value);
    while (count) payload[n++] = digits[--count];
    n = append(payload, n, ready ? ",\"link_ready\":true}" : ",\"link_ready\":false}");
    const scb_header_t h = { .type = SCB_HELLO_ACK, .payload_len = (uint16_t)n, .session_id = id };
    return scb_encode(&h, payload, transport, out, capacity, w, used) == SCB_OK;
}

static void invalidate_tcp(scb_session_t *s)
{
    if (s->tcp_open) s->close_tcp = true;
    s->tcp_hello = false;
}

scb_session_result_t scb_session_spi(scb_session_t *s, const uint8_t *b, size_t n,
                                    uint32_t candidate, bridge_json_workspace_t *w,
                                    uint8_t *out, size_t capacity, size_t *used)
{
    if (!used) return SCB_SESSION_REJECT;
    *used = 0;
    if (!s) return SCB_SESSION_REJECT;
    scb_frame_view_t v;
    if (scb_decode(b, n, SCB_SPI, w, &v) != SCB_OK) return SCB_SESSION_REJECT;
    if (v.header.type == SCB_HELLO) {
        if (!hello(&v, "stm32", w)) return SCB_SESSION_REJECT;
        bool fresh = s->phase != SCB_WAIT_IDLE;
        uint32_t id = fresh ? candidate : s->session_id;
        if (fresh && (!id || id == s->session_id || id == s->previous_session)) return SCB_SESSION_NEED_RANDOM;
        if (!reply(id, false, SCB_SPI, w, out, capacity, used)) return SCB_SESSION_REJECT;
        if (fresh) {
            s->previous_session = s->session_id ? s->session_id : s->previous_session;
            s->session_id = id;
            invalidate_tcp(s);
        }
        s->phase = SCB_WAIT_IDLE;
        return SCB_SESSION_REPLY;
    }
    if (s->phase == SCB_WAIT_HELLO || v.header.session_id != s->session_id ||
        !v.header.seq || v.header.seq == UINT32_MAX) return SCB_SESSION_REJECT;
    if (s->phase == SCB_WAIT_IDLE) {
        if (v.header.type != SCB_IDLE || v.header.ack_seq || v.header.request_id) return SCB_SESSION_REJECT;
        s->phase = SCB_SESSION_READY;
        return SCB_SESSION_ESTABLISHED;
    }
    return v.header.type == SCB_HELLO_ACK ? SCB_SESSION_REJECT : SCB_SESSION_PASS;
}

void scb_session_spi_lost(scb_session_t *s)
{
    if (!s) return;
    if (s->session_id) s->previous_session = s->session_id;
    s->session_id = 0; s->phase = SCB_WAIT_HELLO;
    invalidate_tcp(s);
}

bool scb_session_tcp_open(scb_session_t *s)
{
    if (!s || s->tcp_open) return false;
    s->tcp_open = true; s->tcp_hello = false; s->close_tcp = false; s->tcp_session = 0;
    return true;
}

void scb_session_tcp_close(scb_session_t *s)
{
    if (!s) return;
    s->tcp_open = false; s->tcp_hello = false; s->close_tcp = false; s->tcp_session = 0;
}

scb_session_result_t scb_session_tcp_hello(scb_session_t *s, const uint8_t *b, size_t n,
                                         bridge_json_workspace_t *w,
                                         uint8_t *out, size_t capacity, size_t *used)
{
    if (!used) return SCB_SESSION_REJECT;
    *used = 0;
    if (!s || !s->tcp_open || s->tcp_hello || s->close_tcp) return SCB_SESSION_REJECT;
    scb_frame_view_t v;
    if (scb_decode(b, n, SCB_TCP, w, &v) != SCB_OK || !hello(&v, "pc", w)) return SCB_SESSION_REJECT;
    if (!reply(s->session_id, s->phase == SCB_SESSION_READY, SCB_TCP, w, out, capacity, used))
        return SCB_SESSION_REJECT;
    s->tcp_session = s->session_id; s->tcp_hello = true;
    return SCB_SESSION_REPLY;
}

bool scb_session_tcp_permitted(const scb_session_t *s, uint32_t id)
{
    return s && s->tcp_open && s->tcp_hello && !s->close_tcp && s->phase == SCB_SESSION_READY &&
           id && id == s->session_id && id == s->tcp_session;
}
