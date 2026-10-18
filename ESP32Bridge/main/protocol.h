#ifndef BRIDGE_PROTOCOL_H
#define BRIDGE_PROTOCOL_H

#include "json_guard.h"

#define SCB_HEADER_BYTES 32u
#define SCB_PAYLOAD_BYTES 992u
#define SCB_SPI_BYTES 1024u
#define SCB_RECORD_BYTES 96u
#define SCB_TELEMETRY_PREFIX 24u
#define SCB_RELIABLE 1u

enum {
    SCB_HELLO = 0x01, SCB_HELLO_ACK = 0x02, SCB_IDLE = 0x03,
    SCB_COMMAND = 0x10, SCB_RESULT = 0x11, SCB_EVENT = 0x12,
    SCB_TELEMETRY = 0x20, SCB_LOG_DATA = 0x30
};
typedef enum { SCB_SPI, SCB_TCP, SCB_UDP } scb_transport_t;
typedef enum {
    SCB_OK = 0, SCB_ARGUMENT, SCB_LENGTH, SCB_MAGIC, SCB_VERSION,
    SCB_TYPE, SCB_FLAGS, SCB_RECORD_COUNT, SCB_CRC, SCB_PAYLOAD,
    SCB_JSON, SCB_TRANSPORT, SCB_ALIAS
} scb_error_t;

/* Field values only. This ordinary C structure is NEVER a wire image. */
typedef struct {
    uint8_t type;
    uint16_t flags, payload_len, record_count;
    uint32_t session_id, seq, ack_seq, request_id;
} scb_header_t;
typedef struct {
    scb_header_t header;
    const uint8_t *payload;
} scb_frame_view_t;

/* Pure codec: no ACK, execution, I/O or allocation. The caller must keep input
 * alive for the returned view, and reserve an owned slot before accepting it.
 * Failure clears view/used. json_workspace is mandatory for JSON types only.
 * SPI decoder ignores padding; encoder always emits exactly 1024 zero-tailed
 * bytes. TCP/UDP require exactly one complete frame, without padding.
 * Stream assembly, invalid-frame transport actions and session/seq/ack policy
 * belong to their subsequent stages, never to this decoder. */
scb_error_t scb_decode(const uint8_t *bytes, size_t length, scb_transport_t transport,
                       bridge_json_workspace_t *json_workspace, scb_frame_view_t *view);
scb_error_t scb_encode(const scb_header_t *header, const uint8_t *payload,
                       scb_transport_t transport, uint8_t *output, size_t capacity,
                       bridge_json_workspace_t *json_workspace, size_t *used);

#endif
