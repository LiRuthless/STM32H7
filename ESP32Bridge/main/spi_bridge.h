#ifndef BRIDGE_SPI_BRIDGE_H
#define BRIDGE_SPI_BRIDGE_H
#include "esp_err.h"
#include "spi_protocol.h"

enum { BRIDGE_SPI_CORE = 1, BRIDGE_SPI_PRIORITY = 12, BRIDGE_SPI_STACK_BYTES = 6144 };
/* Bind nonblocking fixed-resource consumers before startup, never at runtime.
 * NULL selects the honest offline behavior: reliable backpressure, telemetry
 * drop accounting. Caller context must outlive the service. */
esp_err_t bridge_spi_configure(const bridge_spi_port_t *port);
/* Startup-only, once, after the pool and READY guard. Creates the static Core1
 * task, initializes the actual driver and starts the transaction lifecycle. */
esp_err_t bridge_spi_start(void *context);
typedef struct {
    esp_err_t init_error, service_error;
    uint32_t queued, completed, truncated, overlength, wait_timeouts;
    uint32_t session_id, stack_free_min;
    bool link_ready, driver_owned;
    bridge_spi_protocol_stats_t protocol;
} bridge_spi_snapshot_t;
/* Task-safe published copy; never exposes DMA pointers or mutable session. */
esp_err_t bridge_spi_snapshot(bridge_spi_snapshot_t *snapshot);
#endif
