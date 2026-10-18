#include "spi_protocol.h"

bool bridge_spi_port_valid(const bridge_spi_port_t *p)
{
    return !p || (((!p->reserve && !p->commit && !p->cancel) || (p->reserve && p->commit && p->cancel)) &&
                  ((!p->peek_tx && !p->tx_consumed) || (p->peek_tx && p->tx_consumed)));
}

void bridge_spi_protocol_init(bridge_spi_protocol_t *p, const bridge_spi_port_t *port)
{
    *p = (bridge_spi_protocol_t){0};
    if (port && bridge_spi_port_valid(port)) p->port = *port;
    scb_session_init(&p->session);
    scb_link_reset(&p->link, SCB_SPI, 0);
}

static void publish_session(bridge_spi_protocol_t *p)
{
    if (p->port.session_changed)
        p->port.session_changed(p->port.context, p->session.session_id, p->session.phase == SCB_SESSION_READY);
}

static void rebuild(bridge_spi_protocol_t *p)
{
    ++p->stats.rebuilds;
    scb_session_spi_lost(&p->session);
    scb_link_reset(&p->link, SCB_SPI, 0);
    publish_session(p);
}

void bridge_spi_protocol_lost(bridge_spi_protocol_t *p)
{
    if (p->session.phase != SCB_WAIT_HELLO || p->session.session_id != 0) rebuild(p);
}

void bridge_spi_protocol_poll(bridge_spi_protocol_t *p, uint64_t now)
{
    if (p->session.phase != SCB_SESSION_READY) return;
    const uint8_t *ignored;
    size_t length;
    if (scb_link_poll(&p->link, now, &ignored, &length) == SCB_LINK_REBUILD) rebuild(p);
    /* RESEND marks link.sending. The immutable cache is copied only after the
     * driver returns its descriptor. A new exact RX ACK can cancel meanwhile. */
}

void bridge_spi_protocol_sent(bridge_spi_protocol_t *p, bridge_spi_tx_meta_t meta, bool complete, uint64_t now)
{
    if (!complete || !meta.reliable || p->session.phase != SCB_SESSION_READY) return;
    const bool retry = p->link.sent;
    if (scb_link_sent(&p->link, meta.seq, now) && retry) ++p->stats.retries;
}

void bridge_spi_protocol_receive(bridge_spi_protocol_t *p, const uint8_t *rx, uint8_t *tx,
                                 bridge_json_workspace_t *w, bridge_spi_random_fn random, void *random_context)
{
    scb_frame_view_t v;
    const scb_error_t error = scb_decode(rx, SCB_SPI_BYTES, SCB_SPI, w, &v);
    if (error != SCB_OK) {
        if (error == SCB_CRC) ++p->stats.crc_errors;
        else ++p->stats.invalid;
        return;
    }
    ++p->stats.received;
    if (v.header.type == SCB_HELLO || p->session.phase != SCB_SESSION_READY) {
        const uint32_t old_session = p->session.session_id;
        const scb_spi_phase_t old_phase = p->session.phase;
        size_t used;
        scb_session_result_t result = scb_session_spi(&p->session, rx, SCB_SPI_BYTES, 0, w, tx, SCB_SPI_BYTES, &used);
        for (unsigned attempt = 0; result == SCB_SESSION_NEED_RANDOM && random && attempt < 4u; ++attempt)
            result = scb_session_spi(&p->session, rx, SCB_SPI_BYTES, random(random_context), w, tx, SCB_SPI_BYTES, &used);
        if (result == SCB_SESSION_NEED_RANDOM) { ++p->stats.rng_failures; return; }
        if (result != SCB_SESSION_REPLY && result != SCB_SESSION_ESTABLISHED) { ++p->stats.invalid; return; }
        if (p->session.session_id != old_session || result == SCB_SESSION_ESTABLISHED)
            scb_link_reset(&p->link, SCB_SPI, p->session.session_id);
        if (result == SCB_SESSION_ESTABLISHED) {
            scb_frame_view_t ignored;
            (void)scb_link_receive(&p->link, rx, SCB_SPI_BYTES, false, w, &ignored);
        }
        if (old_session != p->session.session_id || old_phase != p->session.phase) publish_session(p);
        return;
    }
    const uint8_t type = v.header.type;
    if (v.header.session_id != p->session.session_id || !v.header.seq || v.header.seq == UINT32_MAX ||
        (type != SCB_IDLE && type != SCB_TELEMETRY && type != SCB_RESULT && type != SCB_EVENT && type != SCB_LOG_DATA) ||
        ((type == SCB_EVENT || type == SCB_TELEMETRY) && v.header.request_id != 0) ||
        ((type == SCB_RESULT || type == SCB_LOG_DATA) && v.header.request_id == 0)) {
        ++p->stats.invalid; return;
    }
    const bool reserved = v.header.flags == SCB_RELIABLE && p->port.reserve && p->port.reserve(p->port.context, &v);
    const scb_link_result_t result = scb_link_receive(&p->link, rx, SCB_SPI_BYTES, reserved, w, &v);
    if (result == SCB_LINK_DELIVER) {
        if (reserved) { p->port.commit(p->port.context, &v); ++p->stats.delivered; }
        else if (type == SCB_TELEMETRY) {
            ++p->stats.telemetry_packets; p->stats.telemetry_records += v.header.record_count;
            if (!p->port.telemetry || !p->port.telemetry(p->port.context, &v)) {
                ++p->stats.telemetry_dropped; p->stats.records_dropped += v.header.record_count;
            }
        }
    } else {
        if (reserved) p->port.cancel(p->port.context);
        if (result == SCB_LINK_DUPLICATE || result == SCB_LINK_STALE) ++p->stats.duplicates;
        else if (result == SCB_LINK_BACKPRESSURE) ++p->stats.backpressure;
        else if (result == SCB_LINK_INVALID) ++p->stats.invalid;
    }
}

bool bridge_spi_protocol_prepare(bridge_spi_protocol_t *p, uint64_t now, uint8_t *tx,
                                 bridge_json_workspace_t *w, bridge_spi_tx_meta_t *meta)
{
    *meta = (bridge_spi_tx_meta_t){0};
    bridge_spi_protocol_poll(p, now);
    size_t used = 0;
    if (p->session.phase == SCB_WAIT_IDLE)
        return scb_session_spi_pending_reply(&p->session, w, tx, SCB_SPI_BYTES, &used);
    if (p->session.phase == SCB_WAIT_HELLO) {
        const scb_header_t idle = {.type = SCB_IDLE};
        return scb_encode(&idle, NULL, SCB_SPI, tx, SCB_SPI_BYTES, w, &used) == SCB_OK;
    }
    if (p->link.pending && p->link.sending) {
        for (size_t i = 0; i < p->link.tx_length; ++i) tx[i] = p->link.tx[i];
        meta->seq = p->link.tx_seq; meta->reliable = true;
        return true;
    }
    scb_frame_view_t outbound;
    if (!p->link.pending && p->port.peek_tx && p->port.peek_tx(p->port.context, &outbound)) {
        const bool direction_ok = outbound.header.type == SCB_COMMAND && outbound.header.flags == SCB_RELIABLE &&
                                  outbound.header.request_id != 0;
        const scb_link_result_t result = direction_ok ? scb_link_prepare(&p->link, &outbound.header, outbound.payload,
            tx, SCB_SPI_BYTES, w, &used) : SCB_LINK_INVALID;
        p->port.tx_consumed(p->port.context, result == SCB_LINK_OK);
        if (result == SCB_LINK_OK) { meta->seq = p->link.tx_seq; meta->reliable = true; return true; }
        if (result == SCB_LINK_REBUILD) { rebuild(p); return bridge_spi_protocol_prepare(p, now, tx, w, meta); }
        ++p->stats.invalid_outbound;
    }
    const scb_header_t idle = {.type = SCB_IDLE};
    const scb_link_result_t result = scb_link_prepare(&p->link, &idle, NULL, tx, SCB_SPI_BYTES, w, &used);
    if (result == SCB_LINK_REBUILD) { rebuild(p); return bridge_spi_protocol_prepare(p, now, tx, w, meta); }
    return result == SCB_LINK_OK;
}
