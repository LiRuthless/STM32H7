#ifndef BRIDGE_LINK_SESSION_H
#define BRIDGE_LINK_SESSION_H
#include "protocol.h"

typedef enum { SCB_WAIT_HELLO, SCB_WAIT_IDLE, SCB_SESSION_READY } scb_spi_phase_t;
typedef enum {
    SCB_SESSION_REJECT, SCB_SESSION_REPLY, SCB_SESSION_ESTABLISHED,
    SCB_SESSION_PASS, SCB_SESSION_NEED_RANDOM
} scb_session_result_t;
/* Single lifecycle owner serializes access with transport owners. A sticky
 * close_tcp request invalidates business permission before socket close.
 * No socket, clock, RNG or DMA access occurs inside this pure state machine. */
typedef struct {
    scb_spi_phase_t phase;
    uint32_t session_id, previous_session, tcp_session;
    bool tcp_open, tcp_hello, close_tcp;
} scb_session_t;

void scb_session_init(scb_session_t *session);
/* candidate must come from hardware RNG in production. NEED_RANDOM changes
 * nothing; retry with a nonzero value different from old sessions. REPLY is
 * encoded SPI HELLO_ACK for a subsequent transaction, never current MISO. */
scb_session_result_t scb_session_spi(scb_session_t *session, const uint8_t *bytes, size_t length,
                                    uint32_t candidate, bridge_json_workspace_t *workspace,
                                    uint8_t *reply, size_t capacity, size_t *used);
/* Timeout/restart revokes ready, invalidates TCP permission immediately and
 * asks its owner to close. Callers reset their independent reliable windows. */
void scb_session_spi_lost(scb_session_t *session);
/* Re-encode the existing pending handshake, without changing its session. */
bool scb_session_spi_pending_reply(const scb_session_t *session, bridge_json_workspace_t *workspace,
                                   uint8_t *reply, size_t capacity, size_t *used);
/* Does not decide how a second connection is rejected (G-01). */
bool scb_session_tcp_open(scb_session_t *session);
void scb_session_tcp_close(scb_session_t *session);
scb_session_result_t scb_session_tcp_hello(scb_session_t *session, const uint8_t *bytes, size_t length,
                                         bridge_json_workspace_t *workspace,
                                         uint8_t *reply, size_t capacity, size_t *used);
bool scb_session_tcp_permitted(const scb_session_t *session, uint32_t frame_session);
#endif
