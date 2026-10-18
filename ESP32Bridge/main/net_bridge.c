#include "net_bridge.h"
#include "spi_bridge.h"
#include "link_session.h"
#include "reliable_link.h"
#include <stdint.h>
#include "lwip/sockets.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "esp_mac.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "sdkconfig.h"

/* All network buffers are static and owned by this module; the DMA pool and
 * driver-held buffers are never touched. The single network task does every
 * socket call; the only cross-context entry is bridge_net_telemetry_offer,
 * which hands the UDP image over under s_udp_mux. */
static StackType_t s_stack[BRIDGE_NET_STACK_BYTES / sizeof(StackType_t)];
static StaticTask_t s_task_storage;
static StaticSemaphore_t s_boot_storage;
static SemaphoreHandle_t s_boot;
static TaskHandle_t s_task;
static esp_err_t s_init_error = ESP_ERR_INVALID_STATE;
static bool s_started;
static bridge_net_port_t s_port;
static bridge_net_snapshot_t s_health = { .init_error = ESP_ERR_INVALID_STATE };
static portMUX_TYPE s_snapshot_mux = portMUX_INITIALIZER_UNLOCKED;
static portMUX_TYPE s_udp_mux = portMUX_INITIALIZER_UNLOCKED;
_Static_assert(sizeof(s_stack) == BRIDGE_NET_STACK_BYTES, "ESP-IDF static stack size in bytes");

static int s_listen = -1, s_conn = -1, s_busy = -1, s_udp = -1;
/* Cross-context UDP image handoff. Writers take the image only when neither
 * pending nor in use; overflow is folded into the drop counter by the task. */
static uint8_t s_udp_image[BRIDGE_NET_UDP_BYTES];
static size_t s_udp_length;
static uint32_t s_udp_generation;
static bool s_udp_pending, s_udp_in_use, s_udp_overflow;
/* Task-only connection state. */
static uint8_t s_rx[BRIDGE_NET_RX_BYTES];
static size_t s_rx_length;
static uint8_t s_tx[BRIDGE_NET_TX_BYTES];
static size_t s_tx_length, s_tx_cursor;
static uint32_t s_tx_seq;
static uint64_t s_tx_progress;
static bool s_tx_reliable, s_want_idle;
static uint8_t s_busy_image[BRIDGE_NET_BUSY_BYTES];
static size_t s_busy_length, s_busy_cursor;
static uint64_t s_busy_deadline;
static scb_link_t s_link;
static uint32_t s_generation, s_peer_ip;
static uint16_t s_sub_port;
static bool s_subscribed, s_hello_done;
static uint32_t s_spi_session;
static bool s_spi_known, s_spi_ready;

static uint64_t now_ms(void) { return (uint64_t)esp_timer_get_time() / 1000u; }
static uint16_t load16(const uint8_t *b) { return (uint16_t)((uint16_t)b[0] | ((uint16_t)b[1] << 8)); }

static void publish(bridge_net_snapshot_t *health)
{
    health->listening = s_listen >= 0;
    health->control_open = s_conn >= 0;
    health->subscribed = s_subscribed;
    health->udp_port = s_subscribed ? s_sub_port : 0;
    health->peer_ipv4 = s_peer_ip;
    health->stack_free_min = uxTaskGetStackHighWaterMark(NULL);
    portENTER_CRITICAL(&s_snapshot_mux);
    s_health = *health;
    portEXIT_CRITICAL(&s_snapshot_mux);
}

esp_err_t bridge_net_snapshot(bridge_net_snapshot_t *snapshot)
{
    if (!snapshot) return ESP_ERR_INVALID_ARG;
    portENTER_CRITICAL(&s_snapshot_mux);
    *snapshot = s_health;
    portEXIT_CRITICAL(&s_snapshot_mux);
    return ESP_OK;
}

esp_err_t bridge_net_configure(const bridge_net_port_t *port)
{
    if (s_started) return ESP_ERR_INVALID_STATE;
    if (port && !port->telemetry) return ESP_ERR_INVALID_ARG;
    s_port = port ? *port : (bridge_net_port_t){0};
    return ESP_OK;
}

esp_err_t bridge_net_telemetry_offer(const uint8_t *frame, size_t length)
{
    if (!frame || length < SCB_HEADER_BYTES || length > BRIDGE_NET_UDP_BYTES) return ESP_ERR_INVALID_ARG;
    scb_frame_view_t view;
    if (scb_decode(frame, length, SCB_UDP, NULL, &view) != SCB_OK) return ESP_ERR_INVALID_ARG;
    portENTER_CRITICAL(&s_udp_mux);
    if (s_udp_pending || s_udp_in_use) {
        s_udp_overflow = true;
        portEXIT_CRITICAL(&s_udp_mux);
        return ESP_ERR_INVALID_STATE;
    }
    for (size_t i = 0; i < length; ++i) s_udp_image[i] = frame[i];
    s_udp_length = length;
    s_udp_generation = s_generation;
    s_udp_pending = true;
    portEXIT_CRITICAL(&s_udp_mux);
    return ESP_OK;
}

/* Logical close only: subscription, pending TX and the pending UDP image die
 * with the connection. SPI state is never reset from here and nothing is
 * replayed; a reconnect must send a fresh HELLO and subscribe again. */
static void close_control(bridge_net_snapshot_t *health)
{
    (void)health; /* counters are incremented by the call sites */
    if (s_conn >= 0) close(s_conn);
    s_conn = -1;
    s_rx_length = 0;
    s_tx_length = 0; s_tx_cursor = 0; s_tx_reliable = false; s_want_idle = false;
    s_hello_done = false; s_subscribed = false;
    scb_link_reset(&s_link, SCB_TCP, 0);
    portENTER_CRITICAL(&s_udp_mux);
    s_udp_pending = false;
    portEXIT_CRITICAL(&s_udp_mux);
}

static void session_check(bridge_net_snapshot_t *health)
{
    bridge_spi_snapshot_t spi;
    if (bridge_spi_snapshot(&spi) != ESP_OK) return;
    /* G-02: an SPI session change immediately invalidates the business
     * permission and closes the control TCP; the PC reconnects and learns the
     * current state through a fresh HELLO/HELLO_ACK. */
    if (s_spi_known && spi.session_id != s_spi_session && s_conn >= 0) {
        close_control(health);
        ++health->session_closes;
    }
    s_spi_session = spi.session_id;
    s_spi_ready = spi.link_ready;
    s_spi_known = true;
}

static void link_service(bridge_net_snapshot_t *health, uint64_t now)
{
    if (s_conn < 0 || !s_hello_done || s_tx_length != 0) return;
    const uint8_t *bytes;
    size_t length;
    const scb_link_result_t result = scb_link_poll(&s_link, now, &bytes, &length);
    /* Exhausted retries close only this TCP connection, never the SPI link. */
    if (result == SCB_LINK_REBUILD) { ++health->link_failures; close_control(health); return; }
    if (result == SCB_LINK_RESEND && length <= sizeof(s_tx)) {
        for (size_t i = 0; i < length; ++i) s_tx[i] = bytes[i];
        s_tx_length = length; s_tx_cursor = 0;
        s_tx_seq = s_link.tx_seq; s_tx_reliable = true; s_tx_progress = now;
    }
}

static void idle_service(bridge_json_workspace_t *workspace, uint64_t now)
{
    if (s_conn < 0 || !s_want_idle || s_tx_length != 0 || s_link.dead) return;
    const scb_header_t idle = { .type = SCB_IDLE };
    size_t used;
    if (scb_link_prepare(&s_link, &idle, NULL, s_tx, sizeof(s_tx), workspace, &used) != SCB_LINK_OK) return;
    s_want_idle = false;
    s_tx_length = used; s_tx_cursor = 0; s_tx_reliable = false; s_tx_progress = now;
}

static void handle_subscribe(bridge_net_snapshot_t *health, bridge_json_workspace_t *workspace,
                             const scb_frame_view_t *view, uint64_t now)
{
    uint32_t port_value = 0;
    bool legal = view->header.request_id != 0;
    if (legal) {
        bridge_json_view_t args, port;
        legal = bridge_json_object_get(view->payload, view->header.payload_len, "args", workspace, &args) &&
                bridge_json_object_get(args.bytes, args.length, "udp_port", workspace, &port) &&
                bridge_json_u32(port, &port_value) && port_value >= 1u && port_value <= 65535u;
    }
    uint8_t payload[48];
    size_t n = 0;
    static const char prefix[] = "{\"op\":\"SUBSCRIBE\",\"status\":\"";
    static const char okay[] = "OK";
    static const char invalid[] = "INVALID_ARG";
    const char *status = legal ? okay : invalid;
    while (prefix[n]) { payload[n] = (uint8_t)prefix[n]; ++n; }
    for (size_t i = 0; status[i]; ++i) payload[n++] = (uint8_t)status[i];
    payload[n++] = (uint8_t)'"'; payload[n++] = (uint8_t)'}';
    if (legal) {
        /* The destination IP is always the TCP peer recorded at accept; an
         * address inside the frame is never honored. */
        s_subscribed = true;
        s_sub_port = (uint16_t)port_value;
        ++health->subscriptions;
    }
    const scb_header_t header = { .type = SCB_RESULT, .flags = SCB_RELIABLE,
        .payload_len = (uint16_t)n, .request_id = view->header.request_id };
    size_t used;
    if (scb_link_prepare(&s_link, &header, payload, s_tx, sizeof(s_tx), workspace, &used) != SCB_LINK_OK) {
        ++health->invalid_closes;
        close_control(health);
        return;
    }
    s_tx_length = used; s_tx_cursor = 0;
    s_tx_seq = s_link.tx_seq; s_tx_reliable = true; s_tx_progress = now;
}

static void dispatch(bridge_net_snapshot_t *health, bridge_json_workspace_t *workspace,
                     size_t frame_length, bool *consumed, uint64_t now)
{
    *consumed = false;
    scb_frame_view_t view;
    const scb_error_t error = scb_decode(s_rx, frame_length, SCB_TCP, workspace, &view);
    if (error != SCB_OK) {
        if (error == SCB_CRC) ++health->crc_errors; else ++health->invalid_closes;
        close_control(health);
        return;
    }
    if (!s_hello_done) {
        /* Before HELLO every other frame closes the connection. */
        if (view.header.type != SCB_HELLO) { ++health->invalid_closes; close_control(health); return; }
        size_t used;
        if (scb_session_tcp_hello_explicit(s_spi_known ? s_spi_session : 0, s_spi_known && s_spi_ready,
                                           s_rx, frame_length, workspace, s_tx, sizeof(s_tx), &used)
                != SCB_SESSION_REPLY) {
            ++health->invalid_closes; close_control(health); return;
        }
        ++health->received;
        s_tx_length = used; s_tx_cursor = 0; s_tx_reliable = false; s_tx_progress = now;
        s_hello_done = true;
        scb_link_reset(&s_link, SCB_TCP, s_spi_known ? s_spi_session : 0);
        *consumed = true;
        return;
    }
    if (view.header.type == SCB_COMMAND) {
        bridge_json_view_t op;
        const bool subscribe = bridge_json_object_get(view.payload, view.header.payload_len, "op", workspace, &op) &&
                               bridge_json_string_equal(op, "SUBSCRIBE");
        /* A SUBSCRIBE reply needs the TX image and a free reliable window, so a
         * fresh frame stays buffered until both are available; a duplicate is
         * still confirmed by IDLE even while our own reply awaits its ACK. The
         * borrow call applies a carried ACK exactly once in both cases. Other
         * reliable COMMANDs have no Phase 05 consumer: real backpressure,
         * never an ACK and never a fabricated RESULT. */
        const bool reserve = subscribe && s_tx_length == 0 && !s_link.pending;
        const scb_link_result_t result = scb_link_receive(&s_link, s_rx, frame_length, reserve, workspace, &view);
        if (result == SCB_LINK_BACKPRESSURE && subscribe) return; /* deferred until TX/window drain */
        ++health->received;
        if (result == SCB_LINK_DELIVER) {
            handle_subscribe(health, workspace, &view, now);
            if (s_conn < 0) return;
        } else if (result == SCB_LINK_DUPLICATE) {
            s_want_idle = true;
        } else if (result == SCB_LINK_BACKPRESSURE) {
            ++health->backpressure;
        } else if (result == SCB_LINK_STALE) {
            /* Older reliable identity: not confirmed again. */
        } else {
            ++health->invalid_closes; close_control(health); return;
        }
        *consumed = true;
        return;
    }
    if (view.header.type == SCB_IDLE) {
        scb_frame_view_t ignored;
        const scb_link_result_t result = scb_link_receive(&s_link, s_rx, frame_length, false, workspace, &ignored);
        if (result == SCB_LINK_INVALID || result == SCB_LINK_REBUILD) {
            ++health->invalid_closes; close_control(health); return;
        }
        ++health->received;
        *consumed = true;
        return;
    }
    ++health->invalid_closes;
    close_control(health);
}

static void rx_recv(bridge_net_snapshot_t *health)
{
    /* A complete-but-deferred frame keeps the buffer occupied; never recv 0
     * bytes, which would be indistinguishable from a peer close. */
    if (s_rx_length == sizeof(s_rx)) return;
    const ssize_t n = recv(s_conn, s_rx + s_rx_length, sizeof(s_rx) - s_rx_length, 0);
    if (n == 0) { close_control(health); return; }
    if (n < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) close_control(health);
        return;
    }
    s_rx_length += (size_t)n;
}

/* Consumes complete frames from the assembly buffer. Runs every iteration, so
 * a frame deferred behind an occupied TX image is retried once TX drains. */
static void rx_consume(bridge_net_snapshot_t *health, bridge_json_workspace_t *workspace, uint64_t now)
{
    for (;;) {
        if (s_conn < 0 || s_rx_length < SCB_HEADER_BYTES) return;
        /* Early header validation before the payload is awaited. */
        if (s_rx[0] != 'S' || s_rx[1] != 'C' || s_rx[2] != 'B' || s_rx[3] != '1' || s_rx[4] != 1u ||
            (s_rx[5] != SCB_HELLO && s_rx[5] != SCB_HELLO_ACK && s_rx[5] != SCB_IDLE &&
             s_rx[5] != SCB_COMMAND && s_rx[5] != SCB_RESULT && s_rx[5] != SCB_EVENT &&
             s_rx[5] != SCB_TELEMETRY && s_rx[5] != SCB_LOG_DATA) ||
            load16(s_rx + 8) > SCB_PAYLOAD_BYTES) {
            ++health->invalid_closes;
            close_control(health);
            return;
        }
        const size_t frame_length = SCB_HEADER_BYTES + (size_t)load16(s_rx + 8);
        if (s_rx_length < frame_length) return;
        bool consumed;
        dispatch(health, workspace, frame_length, &consumed, now);
        if (s_conn < 0 || !consumed) return;
        for (size_t i = frame_length; i < s_rx_length; ++i) s_rx[i - frame_length] = s_rx[i];
        s_rx_length -= frame_length;
    }
}

static void tx_service(bridge_net_snapshot_t *health, bool writable, uint64_t now)
{
    if (s_conn < 0 || s_tx_length == s_tx_cursor) return;
    if (writable) {
        const size_t remaining = s_tx_length - s_tx_cursor;
        const ssize_t n = send(s_conn, s_tx + s_tx_cursor, remaining, 0);
        if (n > 0) {
            s_tx_cursor += (size_t)n;
            health->sent_bytes += (uint32_t)n;
            s_tx_progress = now;
            if ((size_t)n < remaining) ++health->short_writes;
            if (s_tx_cursor == s_tx_length) {
                if (s_tx_reliable) scb_link_sent(&s_link, s_tx_seq, now);
                s_tx_length = 0; s_tx_cursor = 0; s_tx_reliable = false;
            }
        } else if (n == 0) {
            ++health->short_writes;
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            close_control(health);
            return;
        }
    }
    /* A single frame that makes no progress for the stall limit is a dead
     * connection: close and reclaim, never blocking SPI. */
    if (s_tx_length > s_tx_cursor && now - s_tx_progress > BRIDGE_NET_STALL_LIMIT_MS) {
        ++health->stalls;
        close_control(health);
    }
}

static void busy_service(bridge_net_snapshot_t *health, bool writable, uint64_t now)
{
    if (s_busy < 0) return;
    if (writable && s_busy_cursor < s_busy_length) {
        const size_t remaining = s_busy_length - s_busy_cursor;
        const ssize_t n = send(s_busy, s_busy_image + s_busy_cursor, remaining, 0);
        if (n > 0) {
            s_busy_cursor += (size_t)n;
            health->sent_bytes += (uint32_t)n;
            if ((size_t)n < remaining) ++health->short_writes;
        } else if (n == 0) {
            ++health->short_writes;
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            close(s_busy); s_busy = -1; return;
        }
    }
    if (s_busy_cursor >= s_busy_length || now >= s_busy_deadline) {
        close(s_busy);
        s_busy = -1;
    }
}

static void accept_service(bridge_net_snapshot_t *health, bridge_json_workspace_t *workspace, uint64_t now)
{
    struct sockaddr_in peer = {0};
    socklen_t peer_length = sizeof(peer);
    const int fd = accept(s_listen, (struct sockaddr *)&peer, &peer_length);
    if (fd < 0) return;
    if (s_conn < 0) {
        /* Empty control slot: take over and wait for HELLO. */
        s_conn = fd;
        ++s_generation;
        ++health->connected;
        s_peer_ip = peer.sin_addr.s_addr;
        s_rx_length = 0; s_tx_length = 0; s_tx_cursor = 0; s_tx_reliable = false;
        s_hello_done = false; s_subscribed = false; s_want_idle = false;
        scb_link_reset(&s_link, SCB_TCP, 0);
        return;
    }
    ++health->rejected;
    if (s_busy >= 0) { close(fd); return; }
    /* G-01: an occupied slot is answered immediately with a HELLO_ACK/BUSY
     * encoded by the production codec, on an independent buffer and cursor,
     * with a bounded deadline; the first connection is never touched. */
    size_t used;
    if (scb_session_tcp_busy(s_spi_known ? s_spi_session : 0, s_spi_known && s_spi_ready,
                             workspace, s_busy_image, sizeof(s_busy_image), &used) != SCB_SESSION_REPLY) {
        close(fd);
        return;
    }
    s_busy = fd;
    s_busy_length = used; s_busy_cursor = 0;
    s_busy_deadline = now + BRIDGE_NET_BUSY_LIMIT_MS;
}

static void udp_service(bridge_net_snapshot_t *health)
{
    portENTER_CRITICAL(&s_udp_mux);
    if (s_udp_overflow) { s_udp_overflow = false; ++health->udp_dropped; }
    if (!s_udp_pending) { portEXIT_CRITICAL(&s_udp_mux); return; }
    const size_t length = s_udp_length;
    const uint32_t generation = s_udp_generation;
    s_udp_pending = false;
    s_udp_in_use = true;
    portEXIT_CRITICAL(&s_udp_mux);
    /* The generation credential keeps delayed work of a dead connection away
     * from a fresh one; unbound outlet, no subscription and disconnection all
     * count as drops. */
    const bool deliver = s_conn >= 0 && s_subscribed && generation == s_generation && s_port.telemetry != NULL;
    if (!deliver) ++health->udp_dropped;
    else if (s_port.telemetry(s_port.context, s_peer_ip, s_sub_port, s_udp_image, length)) ++health->udp_sent;
    else ++health->udp_failed;
    portENTER_CRITICAL(&s_udp_mux);
    s_udp_in_use = false;
    portEXIT_CRITICAL(&s_udp_mux);
}

static struct pollfd s_fds[3];
static int s_listen_index, s_conn_index, s_busy_index;

/* One bounded poll per iteration covers listen/control/BUSY. The writable wait
 * never crosses the next reliable timing deadline of this end. */
static void poll_wait(uint64_t now)
{
    nfds_t count = 0;
    s_listen_index = s_conn_index = s_busy_index = -1;
    if (s_listen >= 0) {
        s_listen_index = (int)count;
        s_fds[count++] = (struct pollfd){ .fd = s_listen, .events = POLLIN, .revents = 0 };
    }
    if (s_conn >= 0) {
        s_conn_index = (int)count;
        short events = POLLIN;
        if (s_tx_length > s_tx_cursor) events |= POLLOUT;
        s_fds[count++] = (struct pollfd){ .fd = s_conn, .events = events, .revents = 0 };
    }
    if (s_busy >= 0) {
        s_busy_index = (int)count;
        s_fds[count++] = (struct pollfd){ .fd = s_busy, .events = POLLOUT, .revents = 0 };
    }
    int timeout = BRIDGE_NET_POLL_LIMIT_MS;
    if (s_conn >= 0 && s_hello_done && !s_link.dead && s_link.pending && s_link.sent && !s_link.sending) {
        const uint64_t due = s_link.last_send_ms + SCB_RETRY_MS;
        const int remaining = due > now ? (int)(due - now) : 0;
        if (remaining < timeout) timeout = remaining;
    }
    if (s_busy >= 0) {
        const int remaining = s_busy_deadline > now ? (int)(s_busy_deadline - now) : 0;
        if (remaining < timeout) timeout = remaining;
    }
    (void)poll(s_fds, count, timeout);
}

static esp_err_t run_service(bridge_net_snapshot_t *health, bridge_json_workspace_t *workspace)
{
    for (;;) {
        uint64_t now = now_ms();
        session_check(health);
        link_service(health, now);
        idle_service(workspace, now);
        poll_wait(now);
        now = now_ms();
        const short listen_events = s_listen_index >= 0 ? s_fds[s_listen_index].revents : 0;
        const short conn_events = s_conn_index >= 0 ? s_fds[s_conn_index].revents : 0;
        const short busy_events = s_busy_index >= 0 ? s_fds[s_busy_index].revents : 0;
        if (busy_events & (POLLERR | POLLHUP | POLLNVAL)) { close(s_busy); s_busy = -1; }
        else busy_service(health, (busy_events & POLLOUT) != 0, now);
        if (conn_events & (POLLERR | POLLHUP | POLLNVAL)) close_control(health);
        else if (s_conn >= 0) {
            tx_service(health, (conn_events & POLLOUT) != 0, now);
            if (s_conn >= 0 && (conn_events & POLLIN)) rx_recv(health);
            if (s_conn >= 0) rx_consume(health, workspace, now);
        }
        if (s_listen >= 0 && (listen_events & POLLIN)) accept_service(health, workspace, now);
        udp_service(health);
        publish(health);
    }
    /* Unreachable: the loop above never exits; keeps the signature honest. */
    return ESP_OK;
}

/* Production UDP outlet: the task-owned nonblocking socket. Bound once by
 * startup through bridge_net_configure; a failed or short datagram is only
 * counted by the caller and never blocks the network task. */
bool bridge_net_socket_telemetry(void *context, uint32_t ipv4, uint16_t port, const uint8_t *frame, size_t length)
{
    (void)context;
    if (s_udp < 0 || !frame || length > BRIDGE_NET_UDP_BYTES) return false;
    struct sockaddr_in destination = {0};
    destination.sin_family = AF_INET;
    destination.sin_port = htons(port);
    destination.sin_addr.s_addr = ipv4;
    const ssize_t sent = sendto(s_udp, frame, length, 0, (const struct sockaddr *)&destination, sizeof(destination));
    return sent == (ssize_t)length;
}

static esp_err_t init_sockets(void)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { ESP_LOGE("net", "listen socket failed errno=%d", errno); return ESP_FAIL; }
    esp_err_t error = ESP_FAIL;
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons(BRIDGE_NET_TCP_PORT);
    address.sin_addr.s_addr = htonl(0xC0A80401u); /* 192.168.4.1 */
    if (bind(fd, (const struct sockaddr *)&address, sizeof(address)) != 0) {
        ESP_LOGE("net", "bind %u failed errno=%d", (unsigned)BRIDGE_NET_TCP_PORT, errno);
    } else if (listen(fd, BRIDGE_NET_BACKLOG) != 0) {
        ESP_LOGE("net", "listen failed errno=%d", errno);
    } else if (fcntl(fd, F_SETFL, O_NONBLOCK) != 0) {
        ESP_LOGE("net", "nonblocking listen failed errno=%d", errno);
    } else {
        error = ESP_OK;
    }
    if (error != ESP_OK) { close(fd); return error; }
    s_listen = fd;
    /* UDP socket failure is not fatal: every datagram is then counted failed. */
    s_udp = socket(AF_INET, SOCK_DGRAM, 0);
    if (s_udp >= 0 && fcntl(s_udp, F_SETFL, O_NONBLOCK) != 0) {
        close(s_udp);
        s_udp = -1;
    }
    if (s_udp < 0) ESP_LOGW("net", "UDP socket unavailable; telemetry will be counted failed");
    ESP_LOGI("net", "Listening on 192.168.4.1:%u backlog=%u core=%d priority=%u stack_bytes=%u",
             (unsigned)BRIDGE_NET_TCP_PORT, (unsigned)BRIDGE_NET_BACKLOG, xPortGetCoreID(),
             (unsigned)BRIDGE_NET_PRIORITY, (unsigned)sizeof(s_stack));
    return ESP_OK;
}

static void net_task(void *context)
{
    (void)context;
    s_init_error = init_sockets();
    bridge_net_snapshot_t health = { .init_error = s_init_error };
    publish(&health);
    xSemaphoreGive(s_boot);
    if (s_init_error == ESP_OK) {
        bridge_json_workspace_t workspace;
        health.service_error = run_service(&health, &workspace);
        if (s_conn >= 0) { close(s_conn); s_conn = -1; }
        if (s_busy >= 0) { close(s_busy); s_busy = -1; }
        if (s_listen >= 0) { close(s_listen); s_listen = -1; }
        if (s_udp >= 0) { close(s_udp); s_udp = -1; }
        publish(&health);
        ESP_LOGE("net", "Network service stopped error=%s; no reset", esp_err_to_name(health.service_error));
    }
    vTaskDelete(NULL);
}

esp_err_t bridge_net_start(void *context)
{
    (void)context;
    if (s_started) return ESP_ERR_INVALID_STATE;
    s_started = true;
    s_boot = xSemaphoreCreateBinaryStatic(&s_boot_storage);
    if (!s_boot) {
        portENTER_CRITICAL(&s_snapshot_mux); s_health.init_error = ESP_ERR_NO_MEM; portEXIT_CRITICAL(&s_snapshot_mux);
        return ESP_ERR_NO_MEM;
    }
    s_task = xTaskCreateStaticPinnedToCore(net_task, "scb_net", sizeof(s_stack), NULL,
                                           BRIDGE_NET_PRIORITY, s_stack, &s_task_storage, BRIDGE_NET_CORE);
    if (!s_task) {
        portENTER_CRITICAL(&s_snapshot_mux); s_health.init_error = ESP_ERR_NO_MEM; portEXIT_CRITICAL(&s_snapshot_mux);
        return ESP_ERR_NO_MEM;
    }
    /* Only task initialization is awaited here; it has no SPI dependency. */
    if (xSemaphoreTake(s_boot, portMAX_DELAY) != pdTRUE) return ESP_ERR_INVALID_STATE;
    return s_init_error;
}

esp_err_t bridge_net_ap_start(void *context)
{
    (void)context;
    esp_err_t error;
    esp_netif_t *ap = esp_netif_create_default_wifi_ap();
    if (!ap) {
        ESP_LOGE("net", "AP netif creation failed");
        return ESP_ERR_NO_MEM;
    }
    error = esp_netif_dhcps_stop(ap);
    if (error == ESP_OK) {
        const esp_netif_ip_info_t info = {
            .ip = { .addr = ESP_IP4TOADDR(192, 168, 4, 1) },
            .gw = { .addr = ESP_IP4TOADDR(192, 168, 4, 1) },
            .netmask = { .addr = ESP_IP4TOADDR(255, 255, 255, 0) }
        };
        error = esp_netif_set_ip_info(ap, &info);
    }
    if (error == ESP_OK) error = esp_netif_dhcps_start(ap);
    bool wifi_initialized = false;
    if (error == ESP_OK) {
        wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
        error = esp_wifi_init(&init);
        wifi_initialized = error == ESP_OK;
    }
    if (error == ESP_OK) error = esp_wifi_set_mode(WIFI_MODE_AP);
    uint8_t mac[6] = {0};
    if (error == ESP_OK) error = esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
    if (error == ESP_OK) {
        static const char digits[] = "0123456789ABCDEF";
        char ssid[14] = "SmartCar-";
        ssid[9] = digits[mac[4] >> 4]; ssid[10] = digits[mac[4] & 0xF];
        ssid[11] = digits[mac[5] >> 4]; ssid[12] = digits[mac[5] & 0xF];
        ssid[13] = 0;
        wifi_config_t config = { .ap = {
            .ssid_len = 13, .channel = 1, .authmode = WIFI_AUTH_WPA2_PSK, .max_connection = 4
        } };
        for (size_t i = 0; i < 13u; ++i) config.ap.ssid[i] = (uint8_t)ssid[i];
        static const unsigned char password[] = CONFIG_BRIDGE_AP_PASSWORD;
        for (size_t i = 0; i < sizeof(password) - 1u; ++i) config.ap.password[i] = password[i];
        /* WPA2-PSK threshold only; an invalid password was already blocked at
         * startup and this never falls back to an open AP. */
        error = esp_wifi_set_config(WIFI_IF_AP, &config);
    }
    if (error == ESP_OK) error = esp_wifi_start();
    if (error != ESP_OK) {
        ESP_LOGE("net", "AP startup failed error=%s; resources released, no open fallback",
                 esp_err_to_name(error));
        if (wifi_initialized) esp_wifi_deinit();
        esp_netif_destroy_default_wifi(ap);
    }
    return error;
}
