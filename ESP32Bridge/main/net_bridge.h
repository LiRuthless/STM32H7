#ifndef BRIDGE_NET_BRIDGE_H
#define BRIDGE_NET_BRIDGE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "protocol.h"

enum {
    BRIDGE_NET_CORE = 0, BRIDGE_NET_PRIORITY = 5, BRIDGE_NET_STACK_BYTES = 8192,
    BRIDGE_NET_TCP_PORT = 5000, BRIDGE_NET_BACKLOG = 2,
    BRIDGE_NET_RX_BYTES = 1024, BRIDGE_NET_TX_BYTES = 1024,
    BRIDGE_NET_BUSY_BYTES = 256, BRIDGE_NET_UDP_BYTES = 1016,
    BRIDGE_NET_BUSY_LIMIT_MS = 500, BRIDGE_NET_STALL_LIMIT_MS = 2000,
    BRIDGE_NET_POLL_LIMIT_MS = 10
};

/* UDP telemetry egress, bound once before the service starts, never at runtime.
 * The callback receives one complete encoded TELEMETRY frame (32B header plus
 * payload, no SPI padding, <= 1016B) and the destination: the current TCP peer
 * IPv4 in network byte order and the subscribed port. It must be nonblocking;
 * false means the datagram was not sent and is only counted. */
typedef struct {
    void *context;
    bool (*telemetry)(void *context, uint32_t ipv4, uint16_t port, const uint8_t *frame, size_t length);
} bridge_net_port_t;

/* Startup-only, once. NULL clears the binding; with no bound outlet every
 * offered telemetry frame is dropped and counted. */
esp_err_t bridge_net_configure(const bridge_net_port_t *port);
/* Production UDP outlet implementation, bound by startup before the service
 * starts; uses the task-owned nonblocking socket. */
bool bridge_net_socket_telemetry(void *context, uint32_t ipv4, uint16_t port, const uint8_t *frame, size_t length);
/* Real WPA2 SoftAP: default AP netif, explicit 192.168.4.1/24 with the DHCP
 * server, WIFI_INIT_CONFIG_DEFAULT, AP mode, SSID SmartCar-XXXX from
 * esp_read_mac(ESP_MAC_WIFI_SOFTAP) last two bytes, WPA2-PSK authmode with no
 * open fallback, password from CONFIG_BRIDGE_AP_PASSWORD, channel 1,
 * max_connection 4. Any failing step returns its error and releases the
 * resources it acquired. */
esp_err_t bridge_net_ap_start(void *context);
/* Startup-only, once. Creates the static Core0/priority-5 network task with an
 * 8192B static stack and TCB. Listen setup failure stops only the network
 * service and reports the error; AP/SPI are unaffected. */
esp_err_t bridge_net_start(void *context);
/* Offer one complete encoded TELEMETRY frame for UDP delivery; takes an owned
 * copy into the static 1016B image. Callable from another task context. The
 * frame is dropped and counted when the outlet is unbound, no subscription is
 * active, the control connection is gone or the image is still occupied. */
esp_err_t bridge_net_telemetry_offer(const uint8_t *frame, size_t length);

typedef struct {
    esp_err_t init_error, service_error;
    uint32_t connected, rejected, received, crc_errors, invalid_closes;
    uint32_t sent_bytes, short_writes, stalls, link_failures;
    uint32_t subscriptions, udp_sent, udp_failed, udp_dropped;
    uint32_t backpressure, session_closes, stack_free_min;
    uint32_t peer_ipv4;
    uint16_t udp_port;
    bool listening, control_open, subscribed;
} bridge_net_snapshot_t;
/* Task-safe published copy; read-only and never carries the password. */
esp_err_t bridge_net_snapshot(bridge_net_snapshot_t *snapshot);
#endif
