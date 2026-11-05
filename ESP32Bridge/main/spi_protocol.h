#ifndef BRIDGE_SPI_PROTOCOL_H
#define BRIDGE_SPI_PROTOCOL_H
#include "link_session.h"
#include "reliable_link.h"

/* Startup-bound, single SPI owner. Every callback is bounded, nonblocking and
 * does no socket I/O. Views borrow the reclaimed application RX only until
 * callback return; commit/telemetry MUST copy into an already owned fixed slot.
 * A reserve success guarantees commit cannot subsequently fail. cancel releases
 * that reservation for duplicates/stale/invalid frames. Missing consumers mean
 * real backpressure/drop, never synthetic forwarding success. */
typedef struct {
    void *context;
    bool (*reserve)(void *, const scb_frame_view_t *);
    void (*commit)(void *, const scb_frame_view_t *);
    void (*cancel)(void *);
    bool (*telemetry)(void *, const scb_frame_view_t *);
    bool (*peek_tx)(void *, scb_frame_view_t *);
    void (*tx_consumed)(void *, bool encoded);
    void (*session_changed)(void *, uint32_t session_id, bool ready);
} bridge_spi_port_t;

typedef struct {
    uint32_t received, crc_errors, invalid, duplicates, backpressure, delivered;
    uint32_t telemetry_packets, telemetry_records, telemetry_dropped, records_dropped;
    uint32_t retries, rebuilds, rng_failures, invalid_outbound;
} bridge_spi_protocol_stats_t;
typedef struct {
    scb_session_t session;
    scb_link_t link;
    bridge_spi_port_t port;
    bridge_spi_protocol_stats_t stats;
} bridge_spi_protocol_t;
typedef struct { uint32_t seq; bool reliable; } bridge_spi_tx_meta_t;
typedef uint32_t (*bridge_spi_random_fn)(void *context);

bool bridge_spi_port_valid(const bridge_spi_port_t *port);
void bridge_spi_protocol_init(bridge_spi_protocol_t *state, const bridge_spi_port_t *port);
/* Poll never takes a DMA pointer: legal while hardware owns both buffers. */
void bridge_spi_protocol_poll(bridge_spi_protocol_t *state, uint64_t now_ms);
/* Logical invalidation only; never takes or reclaims any DMA buffer. */
void bridge_spi_protocol_lost(bridge_spi_protocol_t *state);
/* The following three operations belong only to the successful-reclaim path. */
void bridge_spi_protocol_sent(bridge_spi_protocol_t *state, bridge_spi_tx_meta_t meta,
                              bool complete, uint64_t now_ms);
void bridge_spi_protocol_receive(bridge_spi_protocol_t *state, const uint8_t *rx, uint8_t *application_tx,
                                 bridge_json_workspace_t *workspace, bridge_spi_random_fn random, void *random_context);
bool bridge_spi_protocol_prepare(bridge_spi_protocol_t *state, uint64_t now_ms, uint8_t *application_tx,
                                 bridge_json_workspace_t *workspace, bridge_spi_tx_meta_t *meta);
#endif
