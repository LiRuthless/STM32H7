#ifndef BRIDGE_RELIABLE_LINK_H
#define BRIDGE_RELIABLE_LINK_H
#include "protocol.h"

#define SCB_RETRY_MS 200u
#define SCB_MAX_RETRIES 3u
typedef enum {
    SCB_LINK_OK, SCB_LINK_DELIVER, SCB_LINK_DUPLICATE, SCB_LINK_BACKPRESSURE,
    SCB_LINK_STALE, SCB_LINK_INVALID, SCB_LINK_BUSY, SCB_LINK_RESEND, SCB_LINK_REBUILD
} scb_link_result_t;

/* Single-owner state, no DMA/socket pointers, no allocation. Each transport
 * owns its instance. tx bytes remain immutable until exact ACK or reset.
 * last_rx stores the full reliable identity, independently of unreliable seq. */
typedef struct {
    scb_transport_t transport;
    uint32_t session_id, next_seq, rx_high, rx_reliable_seq, ack_seq, tx_seq;
    uint64_t last_send_ms;
    size_t tx_length, rx_length;
    unsigned retries;
    bool pending, sending, sent, dead;
    uint8_t tx[SCB_SPI_BYTES], last_rx[SCB_SPI_BYTES];
} scb_link_t;

void scb_link_reset(scb_link_t *link, scb_transport_t transport, uint32_t session);
/* reserved means an owned downstream slot, not a free-space observation.
 * On DELIVER the caller must commit the borrowed view into that slot before
 * another call/ACK emission. Failed/duplicate calls clear view. Codec errors,
 * session mismatch and changed duplicate identity never advance ACK. */
scb_link_result_t scb_link_receive(scb_link_t *link, const uint8_t *bytes, size_t length,
                                  bool reserved, bridge_json_workspace_t *workspace,
                                  scb_frame_view_t *view);
/* header carries only business fields; session/seq/ack are replaced. Ordinary
 * IDLE may pass an occupied reliable window. Handshake uses link_session.
 * Output must be a separate application-owned buffer, never driver-owned. */
scb_link_result_t scb_link_prepare(scb_link_t *link, const scb_header_t *header,
                                  const uint8_t *payload, uint8_t *output, size_t capacity,
                                  bridge_json_workspace_t *workspace, size_t *used);
/* For the reliable image only, after ALL bytes/transaction actually sent.
 * False/partial/error sends must not call this; poll waits until completion.
 * Stale completions after ACK are harmless. */
bool scb_link_sent(scb_link_t *link, uint32_t seq, uint64_t now_ms);
/* RESEND lends immutable bytes until next owner call; copy into application
 * DMA before queueing, never edit a driver-held buffer. Calls do not wait. */
scb_link_result_t scb_link_poll(scb_link_t *link, uint64_t now_ms,
                               const uint8_t **bytes, size_t *length);
#endif
