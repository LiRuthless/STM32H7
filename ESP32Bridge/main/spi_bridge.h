#ifndef BRIDGE_SPI_BRIDGE_H
#define BRIDGE_SPI_BRIDGE_H
#include "esp_err.h"

enum { BRIDGE_SPI_CORE = 1, BRIDGE_SPI_PRIORITY = 12, BRIDGE_SPI_STACK_BYTES = 6144 };
/* Startup-only, once, after the pool and READY guard. Creates a static Core1
 * task and returns the actual driver initialization result. This Group 1
 * entry initializes hardware only; it never publishes READY or claims a
 * protocol session. Transaction lifecycle is the next implementation group. */
esp_err_t bridge_spi_start(void *context);
#endif
