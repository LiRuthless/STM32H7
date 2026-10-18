/* Production net_bridge.c compiled verbatim. Sockets/Wi-Fi/netif/MAC/RTOS are
 * fixtures driven by an explicit script; builtin jumps model a parked task,
 * never real RTOS/network evidence. One script entry drives one poll iteration. */
#include "net_bridge.c"
#include <stdarg.h>
#include "sdkconfig.h"

#define CHECK(c) do { if (!(c)) return __LINE__; } while (0)
#define SCRIPT_MAX 512u
#define SCRIPT_BYTES 1100u
#define CAPTURE_BYTES 65536u
#define UDP_MAX 16u
#define INJECTED 0x6D1

int bridge_stub_errno;

enum {
    EV_IDLE, EV_ACCEPT, EV_ACCEPT_EAGAIN,
    EV_RECV, EV_RECV_EOF, EV_RECV_EAGAIN, EV_RECV_ERROR,
    EV_SEND, EV_SEND_EAGAIN, EV_SEND_ZERO, EV_SEND_ERROR,
    EV_BUSY_SEND, EV_BUSY_SEND_EAGAIN, EV_BUSY_SEND_ZERO, EV_BUSY_SEND_ERROR,
    EV_OFFER, EV_SET_SPI, EV_UDP_FAIL, EV_UNBIND
};

typedef struct {
    uint32_t kind, arg, length;
    uint8_t bytes[SCRIPT_BYTES];
} script_entry_t;

static script_entry_t s_script[SCRIPT_MAX];
static script_entry_t s_current;
static uint32_t s_script_len, s_script_pos;
static int s_violations, s_parked, s_deleted, s_polls, s_max_timeout;
static uint64_t s_clock_ms;
static intptr_t s_jump[5];
static uint32_t s_spi_session_fixture;
static bool s_spi_ready_fixture;
static int s_socket_calls, s_accept_calls, s_sendto_calls;
static int s_live[64], s_live_count;
static int s_next_accept_fd;
static uint8_t s_capture[CAPTURE_BYTES], s_busy_capture[4096];
static size_t s_capture_len, s_busy_capture_len;
static uint8_t s_udp_bytes[UDP_MAX][BRIDGE_NET_UDP_BYTES];
static size_t s_udp_lengths[UDP_MAX];
static uint32_t s_udp_ips[UDP_MAX];
static uint16_t s_udp_ports[UDP_MAX];
static uint32_t s_udp_count;
static int s_udp_fail;
static int s_offer_results[64], s_offer_count;
static char s_log[8192];
static size_t s_log_used;
static int s_init_fail;
static int s_close_log[64], s_close_count;

/* AP fixture state. */
static const uint8_t s_fixture_mac[6] = { 0x24, 0x6F, 0x28, 0x12, 0xAB, 0xCD };
static esp_netif_t s_fake_netif;
static int s_ap_fail_at, s_ap_order[24], s_ap_order_count;
static int s_ap_destroyed, s_ap_deinit;
static wifi_config_t s_ap_config;
static int s_ap_mode;
static esp_netif_ip_info_t s_ap_ip;
static int s_ap_init_magic;

void *memset(void *p, int c, size_t n) { uint8_t *b = p; for (size_t i = 0; i < n; ++i) b[i] = (uint8_t)c; return p; }
void *memcpy(void *p, const void *s, size_t n) { uint8_t *b = p; const uint8_t *v = s; for (size_t i = 0; i < n; ++i) b[i] = v[i]; return p; }
static bool text_equal(const void *a, const void *b, size_t n)
{ const uint8_t *x = a, *y = b; for (size_t i = 0; i < n; ++i) if (x[i] != y[i]) return false; return true; }

void test_lock(void) {}
void test_unlock(void) {}
static void log_char(char value)
{ if (s_log_used + 1 < sizeof(s_log)) { s_log[s_log_used++] = value; s_log[s_log_used] = 0; } }
static void log_text(const char *value) { while (*value) log_char(*value++); }
static void log_uint(unsigned value)
{
    char digits[10]; int count = 0;
    do { digits[count++] = (char)('0' + value % 10); value /= 10; } while (value);
    while (count) log_char(digits[--count]);
}
void test_log(const char *tag, const char *format, ...)
{
    log_text(tag); log_char(':');
    va_list args; va_start(args, format);
    while (*format) {
        if (*format++ != '%') { log_char(format[-1]); continue; }
        const char conversion = *format++;
        if (conversion == 's') log_text(va_arg(args, const char *));
        else if (conversion == 'u') log_uint(va_arg(args, unsigned));
        else if (conversion == 'd') {
            int value = va_arg(args, int);
            if (value < 0) { log_char('-'); log_uint((unsigned)(-(value + 1)) + 1); }
            else log_uint((unsigned)value);
        } else ++s_violations;
    }
    va_end(args); log_char('\n');
}
const char *esp_err_to_name(esp_err_t error) { return error == ESP_OK ? "ESP_OK" : "INJECTED"; }
int64_t esp_timer_get_time(void) { return (int64_t)s_clock_ms * 1000; }
UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t task) { (void)task; return 4096; }
int xPortGetCoreID(void) { return BRIDGE_NET_CORE; }
SemaphoreHandle_t xSemaphoreCreateBinaryStatic(StaticSemaphore_t *storage) { storage->token = 0; return storage; }
BaseType_t xSemaphoreGive(SemaphoreHandle_t semaphore) { ++semaphore->token; return pdTRUE; }
BaseType_t xSemaphoreTake(SemaphoreHandle_t semaphore, uint32_t ticks)
{ if (ticks != portMAX_DELAY || semaphore->token != 1) { ++s_violations; return 0; } --semaphore->token; return pdTRUE; }
void vTaskDelete(TaskHandle_t task) { if (task) ++s_violations; ++s_deleted; }
TaskHandle_t xTaskCreateStaticPinnedToCore(TaskFunction_t task, const char *name, uint32_t bytes,
    void *arg, UBaseType_t priority, StackType_t *stack, StaticTask_t *storage, BaseType_t core)
{
    if (bytes != BRIDGE_NET_STACK_BYTES || priority != BRIDGE_NET_PRIORITY || core != BRIDGE_NET_CORE ||
        stack != s_stack || storage != &s_task_storage || !name) ++s_violations;
    if (!__builtin_setjmp((void **)s_jump)) task(arg);
    return storage;
}

/* SPI is a separate production owner; here only its published snapshot exists. */
esp_err_t bridge_spi_snapshot(bridge_spi_snapshot_t *snapshot)
{
    if (!snapshot) return ESP_ERR_INVALID_ARG;
    *snapshot = (bridge_spi_snapshot_t){0};
    snapshot->session_id = s_spi_session_fixture;
    snapshot->link_ready = s_spi_ready_fixture;
    return ESP_OK;
}

static bool fd_live(int fd)
{ for (int i = 0; i < s_live_count; ++i) if (s_live[i] == fd) return true; return false; }
static void fd_add(int fd)
{ if (s_live_count < 64) s_live[s_live_count++] = fd; else ++s_violations; }

int socket(int domain, int type, int protocol)
{
    (void)protocol;
    if (domain != AF_INET || (type != SOCK_STREAM && type != SOCK_DGRAM)) ++s_violations;
    const int call = s_socket_calls++;
    if ((s_init_fail == 1 && call == 0) || (s_init_fail == 5 && call == 1)) { bridge_stub_errno = EAGAIN; return -1; }
    const int fd = 100 + call;
    fd_add(fd);
    return fd;
}
int bind(int s, const struct sockaddr *name, socklen_t namelen)
{
    if (s != 100 || namelen != sizeof(struct sockaddr_in)) ++s_violations;
    const struct sockaddr_in *address = (const struct sockaddr_in *)name;
    if (address->sin_family != AF_INET || address->sin_port != htons(BRIDGE_NET_TCP_PORT) ||
        address->sin_addr.s_addr != htonl(0xC0A80401u)) ++s_violations;
    if (s_init_fail == 2) { bridge_stub_errno = EPIPE; return -1; }
    return 0;
}
int listen(int s, int backlog)
{
    if (s != 100 || backlog < 2) ++s_violations;
    if (s_init_fail == 3) { bridge_stub_errno = EPIPE; return -1; }
    return 0;
}
int fcntl(int s, int cmd, ...)
{
    va_list args; va_start(args, cmd);
    const int value = va_arg(args, int);
    va_end(args);
    if (cmd != F_SETFL || !(value & O_NONBLOCK) || !fd_live(s)) ++s_violations;
    if (s_init_fail == 4 && s == 100) { bridge_stub_errno = EPIPE; return -1; }
    return 0;
}
int close(int s)
{
    if (!fd_live(s)) { ++s_violations; return -1; }
    for (int i = 0; i < s_live_count; ++i)
        if (s_live[i] == s) { s_live[i] = s_live[--s_live_count]; break; }
    if (s_close_count < 64) s_close_log[s_close_count] = s;
    ++s_close_count;
    return 0;
}
int accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    ++s_accept_calls;
    if (s != s_listen || !addrlen || *addrlen < (socklen_t)sizeof(struct sockaddr_in)) ++s_violations;
    if (s_current.kind == EV_ACCEPT_EAGAIN) { bridge_stub_errno = EAGAIN; return -1; }
    if (s_current.kind != EV_ACCEPT) { ++s_violations; bridge_stub_errno = EAGAIN; return -1; }
    struct sockaddr_in *peer = (struct sockaddr_in *)addr;
    peer->sin_family = AF_INET;
    peer->sin_addr.s_addr = s_current.arg;
    peer->sin_port = htons(5001);
    *addrlen = sizeof(struct sockaddr_in);
    const int fd = s_next_accept_fd++;
    fd_add(fd);
    return fd;
}
ssize_t recv(int s, void *mem, size_t len, int flags)
{
    if (flags != 0 || s != s_conn || s < 0) { ++s_violations; bridge_stub_errno = EPIPE; return -1; }
    if (s_current.kind == EV_RECV) {
        if (s_current.length > len || s_current.length > SCRIPT_BYTES) { ++s_violations; bridge_stub_errno = EPIPE; return -1; }
        memcpy(mem, s_current.bytes, s_current.length);
        return (ssize_t)s_current.length;
    }
    if (s_current.kind == EV_RECV_EOF) return 0;
    if (s_current.kind == EV_RECV_EAGAIN) { bridge_stub_errno = EAGAIN; return -1; }
    if (s_current.kind == EV_RECV_ERROR) { bridge_stub_errno = EPIPE; return -1; }
    ++s_violations; bridge_stub_errno = EPIPE; return -1;
}
static ssize_t send_capped(int s, const void *data, size_t size, uint8_t *capture, size_t *used, size_t capacity,
                           uint32_t cap_kind, uint32_t eagain_kind, uint32_t zero_kind, uint32_t error_kind)
{
    (void)s;
    if (s_current.kind == cap_kind) {
        size_t n = s_current.arg < size ? s_current.arg : size;
        if (*used + n > capacity) { ++s_violations; n = 0; }
        memcpy(capture + *used, data, n);
        *used += n;
        return (ssize_t)n;
    }
    if (s_current.kind == zero_kind) return 0;
    if (s_current.kind == eagain_kind) { bridge_stub_errno = EAGAIN; return -1; }
    if (s_current.kind == error_kind) { bridge_stub_errno = EPIPE; return -1; }
    ++s_violations; bridge_stub_errno = EPIPE; return -1;
}
ssize_t send(int s, const void *data, size_t size, int flags)
{
    if (flags != 0 || !fd_live(s)) { ++s_violations; bridge_stub_errno = EPIPE; return -1; }
    if (s == s_conn) return send_capped(s, data, size, s_capture, &s_capture_len, sizeof(s_capture),
                                        EV_SEND, EV_SEND_EAGAIN, EV_SEND_ZERO, EV_SEND_ERROR);
    if (s == s_busy) return send_capped(s, data, size, s_busy_capture, &s_busy_capture_len, sizeof(s_busy_capture),
                                        EV_BUSY_SEND, EV_BUSY_SEND_EAGAIN, EV_BUSY_SEND_ZERO, EV_BUSY_SEND_ERROR);
    ++s_violations; bridge_stub_errno = EPIPE; return -1;
}
ssize_t sendto(int s, const void *data, size_t size, int flags, const struct sockaddr *to, socklen_t tolen)
{
    ++s_sendto_calls;
    if (s != s_udp || flags != 0 || tolen != sizeof(struct sockaddr_in) || size > BRIDGE_NET_UDP_BYTES) ++s_violations;
    const struct sockaddr_in *destination = (const struct sockaddr_in *)to;
    if (destination->sin_family != AF_INET) ++s_violations;
    if (s_udp_fail) { bridge_stub_errno = EAGAIN; return -1; }
    if (s_udp_count >= UDP_MAX) { ++s_violations; bridge_stub_errno = EAGAIN; return -1; }
    memcpy(s_udp_bytes[s_udp_count], data, size);
    s_udp_lengths[s_udp_count] = size;
    s_udp_ips[s_udp_count] = destination->sin_addr.s_addr;
    s_udp_ports[s_udp_count] = htons(destination->sin_port);
    ++s_udp_count;
    return (ssize_t)size;
}

int poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
    if (timeout < 0 || timeout > BRIDGE_NET_POLL_LIMIT_MS || nfds > 3) ++s_violations;
    if (timeout > s_max_timeout) s_max_timeout = timeout;
    for (nfds_t i = 0; i < nfds; ++i) {
        if (!fd_live(fds[i].fd)) ++s_violations;
        fds[i].revents = 0;
    }
    ++s_polls;
    if (s_script_pos >= s_script_len) { s_parked = 1; __builtin_longjmp((void **)s_jump, 1); }
    s_current = s_script[s_script_pos++];
    switch (s_current.kind) {
    case EV_IDLE:
        s_clock_ms += (uint32_t)timeout;
        return 0;
    case EV_ACCEPT: case EV_ACCEPT_EAGAIN: {
        int found = 0;
        for (nfds_t i = 0; i < nfds; ++i) if (fds[i].fd == s_listen) { fds[i].revents = POLLIN; ++found; }
        if (!found) ++s_violations;
        return found;
    }
    case EV_RECV: case EV_RECV_EOF: case EV_RECV_EAGAIN: case EV_RECV_ERROR: {
        int found = 0;
        for (nfds_t i = 0; i < nfds; ++i) if (fds[i].fd == s_conn) { fds[i].revents |= POLLIN; ++found; }
        if (!found) ++s_violations;
        return found;
    }
    case EV_SEND: case EV_SEND_EAGAIN: case EV_SEND_ZERO: case EV_SEND_ERROR: {
        /* A scripted send without a pending TX image simply finds no writable
         * interest this iteration; capture assertions catch missing bytes. */
        for (nfds_t i = 0; i < nfds; ++i)
            if (fds[i].fd == s_conn && (fds[i].events & POLLOUT)) { fds[i].revents |= POLLOUT; return 1; }
        return 0;
    }
    case EV_BUSY_SEND: case EV_BUSY_SEND_EAGAIN: case EV_BUSY_SEND_ZERO: case EV_BUSY_SEND_ERROR: {
        for (nfds_t i = 0; i < nfds; ++i)
            if (fds[i].fd == s_busy) { fds[i].revents |= POLLOUT; return 1; }
        return 0;
    }
    case EV_OFFER:
        if (s_offer_count < 64)
            s_offer_results[s_offer_count] = bridge_net_telemetry_offer(s_current.bytes, s_current.length);
        ++s_offer_count;
        return 0;
    case EV_SET_SPI:
        s_spi_session_fixture = s_current.arg;
        s_spi_ready_fixture = s_current.length != 0;
        return 0;
    case EV_UDP_FAIL:
        s_udp_fail = (int)s_current.arg;
        return 0;
    case EV_UNBIND:
        s_port = (bridge_net_port_t){0};
        return 0;
    default:
        ++s_violations;
        return 0;
    }
}

/* AP fixtures: record configuration, inject one failing step. */
static void ap_record(int code)
{ if (s_ap_order_count < 24) s_ap_order[s_ap_order_count++] = code; else ++s_violations; }
esp_netif_t *esp_netif_create_default_wifi_ap(void)
{
    ap_record(1);
    if (s_ap_fail_at == 1) return NULL;
    return &s_fake_netif;
}
void esp_netif_destroy_default_wifi(void *netif)
{
    ap_record(10);
    if (netif != &s_fake_netif) ++s_violations;
    ++s_ap_destroyed;
}
esp_err_t esp_netif_dhcps_stop(esp_netif_t *netif)
{ ap_record(2); if (netif != &s_fake_netif) ++s_violations; return s_ap_fail_at == 2 ? INJECTED : ESP_OK; }
esp_err_t esp_netif_set_ip_info(esp_netif_t *netif, const esp_netif_ip_info_t *info)
{ ap_record(3); if (netif != &s_fake_netif) ++s_violations; s_ap_ip = *info; return s_ap_fail_at == 3 ? INJECTED : ESP_OK; }
esp_err_t esp_netif_dhcps_start(esp_netif_t *netif)
{ ap_record(4); if (netif != &s_fake_netif) ++s_violations; return s_ap_fail_at == 4 ? INJECTED : ESP_OK; }
esp_err_t esp_wifi_init(const wifi_init_config_t *config)
{ ap_record(5); s_ap_init_magic = config ? config->magic : 0; return s_ap_fail_at == 5 ? INJECTED : ESP_OK; }
esp_err_t esp_wifi_deinit(void) { ap_record(11); ++s_ap_deinit; return ESP_OK; }
esp_err_t esp_wifi_set_mode(wifi_mode_t mode)
{ ap_record(6); s_ap_mode = (int)mode; return s_ap_fail_at == 6 ? INJECTED : ESP_OK; }
esp_err_t esp_read_mac(uint8_t *mac, esp_mac_type_t type)
{
    ap_record(7);
    if (type != ESP_MAC_WIFI_SOFTAP) ++s_violations;
    if (s_ap_fail_at == 7) return INJECTED;
    memcpy(mac, s_fixture_mac, 6);
    return ESP_OK;
}
esp_err_t esp_wifi_set_config(wifi_interface_t interface, wifi_config_t *conf)
{
    ap_record(8);
    if (interface != WIFI_IF_AP) ++s_violations;
    s_ap_config = *conf;
    return s_ap_fail_at == 8 ? INJECTED : ESP_OK;
}
esp_err_t esp_wifi_start(void) { ap_record(9); return s_ap_fail_at == 9 ? INJECTED : ESP_OK; }

static bool log_has(const char *needle)
{
    size_t length = 0;
    while (needle[length]) ++length;
    if (!length) return true;
    for (size_t i = 0; i + length <= s_log_used; ++i)
        if (text_equal(s_log + i, needle, length)) return true;
    return false;
}

static void reset_runtime(void)
{
    s_started = false;
    s_init_error = ESP_ERR_INVALID_STATE;
    s_health = (bridge_net_snapshot_t){ .init_error = ESP_ERR_INVALID_STATE };
    s_port = (bridge_net_port_t){0};
    s_boot = NULL; s_task = NULL;
    s_listen = -1; s_conn = -1; s_busy = -1; s_udp = -1;
    s_udp_length = 0; s_udp_generation = 0;
    s_udp_pending = s_udp_in_use = s_udp_overflow = false;
    s_rx_length = 0;
    s_tx_length = s_tx_cursor = 0; s_tx_seq = 0; s_tx_progress = 0;
    s_tx_reliable = s_want_idle = false;
    s_busy_length = s_busy_cursor = 0; s_busy_deadline = 0;
    s_link = (scb_link_t){0};
    s_generation = 0; s_peer_ip = 0; s_sub_port = 0;
    s_subscribed = s_hello_done = false;
    s_spi_session = 0; s_spi_known = s_spi_ready = false;
    s_boot_storage = (StaticSemaphore_t){0};
}

static void reset_fixture(void)
{
    s_script_len = s_script_pos = 0;
    s_violations = s_parked = s_deleted = s_polls = s_max_timeout = 0;
    s_clock_ms = 0;
    s_spi_session_fixture = 0; s_spi_ready_fixture = false;
    s_socket_calls = s_accept_calls = s_sendto_calls = 0;
    s_live_count = 0; s_next_accept_fd = 200;
    s_capture_len = s_busy_capture_len = 0;
    s_udp_count = 0; s_udp_fail = 0;
    s_offer_count = 0;
    s_log_used = 0; s_log[0] = 0;
    s_init_fail = 0;
    s_close_count = 0;
    bridge_stub_errno = 0;
    reset_runtime();
}

void fixture_reset(void) { reset_fixture(); }
void fixture_spi(uint32_t session, int ready)
{ s_spi_session_fixture = session; s_spi_ready_fixture = ready != 0; }
void fixture_push(uint32_t kind, uint32_t arg, const uint8_t *bytes, uint32_t length)
{
    if (s_script_len >= SCRIPT_MAX || length > SCRIPT_BYTES) { ++s_violations; return; }
    script_entry_t *entry = &s_script[s_script_len++];
    entry->kind = kind; entry->arg = arg; entry->length = length;
    if (bytes && length) memcpy(entry->bytes, bytes, length);
}

int bridge_net_run(void)
{
    const bridge_net_port_t port = { NULL, bridge_net_socket_telemetry };
    if (bridge_net_configure(&port) != ESP_OK) return __LINE__;
    const esp_err_t result = bridge_net_start(NULL);
    if (result != ESP_OK) return __LINE__;
    if (!s_parked || s_deleted || s_violations) return __LINE__;
    if (s_listen < 0 || !fd_live(s_listen)) return __LINE__;
    return 0;
}

int bridge_net_start_fail_test(int scenario)
{
    reset_fixture();
    s_init_fail = scenario;
    const bridge_net_port_t port = { NULL, bridge_net_socket_telemetry };
    if (bridge_net_configure(&port) != ESP_OK) return __LINE__;
    const esp_err_t result = bridge_net_start(NULL);
    if (scenario >= 1 && scenario <= 4) {
        CHECK(result == ESP_FAIL && s_deleted == 1 && !s_parked);
        CHECK(s_listen < 0 && s_live_count == (scenario == 1 ? 0 : 0));
        CHECK(s_health.init_error == ESP_FAIL);
    } else if (scenario == 5) {
        /* UDP socket failure is not fatal: the service runs, sends count failed. */
        CHECK(result == ESP_OK && s_parked == 1);
        CHECK(s_udp < 0 && s_listen >= 0);
    } else return __LINE__;
    CHECK(s_violations == 0);
    return 0;
}

static int check_ap_success(void)
{
    static const int expected[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    CHECK(s_ap_order_count == 9);
    for (int i = 0; i < 9; ++i) CHECK(s_ap_order[i] == expected[i]);
    CHECK(s_ap_ip.ip.addr == ESP_IP4TOADDR(192, 168, 4, 1));
    CHECK(s_ap_ip.gw.addr == ESP_IP4TOADDR(192, 168, 4, 1));
    CHECK(s_ap_ip.netmask.addr == ESP_IP4TOADDR(255, 255, 255, 0));
    CHECK(s_ap_init_magic == 0x1F2F3F4F);
    CHECK(s_ap_mode == WIFI_MODE_AP);
    static const char expected_ssid[] = "SmartCar-ABCD";
    for (int i = 0; i < 13; ++i) CHECK(s_ap_config.ap.ssid[i] == (uint8_t)expected_ssid[i]);
    CHECK(s_ap_config.ap.ssid_len == 13 && s_ap_config.ap.channel == 1);
    CHECK(s_ap_config.ap.authmode == WIFI_AUTH_WPA2_PSK && s_ap_config.ap.max_connection == 4);
    static const char password[] = CONFIG_BRIDGE_AP_PASSWORD;
    size_t length = 0;
    while (password[length]) ++length;
    CHECK(length >= 8 && length <= 63);
    CHECK(text_equal(s_ap_config.ap.password, password, length));
    CHECK(s_ap_destroyed == 0 && s_ap_deinit == 0);
    return 0;
}

int bridge_net_ap_test(int scenario)
{
    reset_fixture();
    s_ap_fail_at = scenario;
    s_ap_order_count = 0; s_ap_destroyed = 0; s_ap_deinit = 0;
    s_ap_config = (wifi_config_t){0};
    s_ap_mode = -1; s_ap_ip = (esp_netif_ip_info_t){0}; s_ap_init_magic = 0;
    const esp_err_t result = bridge_net_ap_start(NULL);
    if (scenario == 0) {
        CHECK(result == ESP_OK);
        CHECK(check_ap_success() == 0);
    } else if (scenario >= 1 && scenario <= 9) {
        CHECK(result != ESP_OK);
        CHECK(s_ap_destroyed == (scenario == 1 ? 0 : 1));
        CHECK(s_ap_deinit == (scenario >= 6 ? 1 : 0));
        /* Steps after the failing one never ran; rollback order deinit→destroy. */
        const int expected_total = scenario == 1 ? 1 : (scenario >= 6 ? scenario + 2 : scenario + 1);
        CHECK(s_ap_order_count == expected_total);
        if (scenario >= 6) {
            CHECK(s_ap_order[s_ap_order_count - 2] == 11 && s_ap_order[s_ap_order_count - 1] == 10);
        } else if (scenario >= 2) {
            CHECK(s_ap_order[s_ap_order_count - 1] == 10);
        }
    } else return __LINE__;
    static const char password[] = CONFIG_BRIDGE_AP_PASSWORD;
    CHECK(!log_has(password));
    CHECK(s_violations == 0);
    return 0;
}

/* The generation credential is defense in depth for cross-task timing: a
 * pending image produced under a dead connection must never be delivered to a
 * fresh one. Drive udp_service directly with a stale generation. */
static bool s_fixture_udp_called;
static bool fixture_udp(void *context, uint32_t ip, uint16_t port, const uint8_t *frame, size_t length)
{ (void)context; (void)ip; (void)port; (void)frame; (void)length; s_fixture_udp_called = true; return true; }
int bridge_net_generation_test(void)
{
    reset_fixture();
    const bridge_net_port_t port = { NULL, fixture_udp };
    CHECK(bridge_net_configure(&port) == ESP_OK);
    bridge_net_snapshot_t health = {0};
    s_generation = 7;
    s_conn = 200; s_subscribed = true; s_sub_port = 5001;
    s_udp_generation = 6; s_udp_length = 152; s_udp_pending = true;
    udp_service(&health);
    CHECK(!s_fixture_udp_called && health.udp_dropped == 1 && !s_udp_pending);
    /* Matching generation reaches the bound callback. */
    s_udp_generation = 7; s_udp_length = 152; s_udp_pending = true;
    udp_service(&health);
    CHECK(s_fixture_udp_called && health.udp_sent == 1);
    /* No connection at all: counted as a drop, never a send. */
    s_conn = -1; s_subscribed = false;
    s_udp_generation = 7; s_udp_pending = true;
    udp_service(&health);
    CHECK(health.udp_dropped == 2 && health.udp_sent == 1);
    CHECK(s_violations == 0);
    return 0;
}

size_t bridge_net_snapshot_size(void) { return sizeof(bridge_net_snapshot_t); }
unsigned bridge_net_metric(unsigned id)
{
    switch (id) {
    case 0: return (unsigned)s_violations;
    case 1: return (unsigned)s_parked;
    case 2: return (unsigned)s_polls;
    case 3: return (unsigned)s_accept_calls;
    case 4: return (unsigned)s_close_count;
    case 5: return (unsigned)s_udp_count;
    case 6: return (unsigned)s_deleted;
    case 7: return (unsigned)s_sendto_calls;
    case 8: return (unsigned)s_clock_ms;
    case 9: return (unsigned)s_max_timeout;
    case 10: return (unsigned)s_offer_count;
    case 11: return (unsigned)s_live_count;
    default: return 99;
    }
}
int bridge_net_offer_result(unsigned index)
{ return index < (unsigned)s_offer_count ? s_offer_results[index] : -99; }
int bridge_net_close_event(unsigned index)
{ return index < (unsigned)s_close_count ? s_close_log[index] : -1; }
size_t bridge_net_capture(uint8_t *out, size_t capacity)
{
    if (capacity < s_capture_len) return 0;
    memcpy(out, s_capture, s_capture_len);
    return s_capture_len;
}
size_t bridge_net_busy_capture(uint8_t *out, size_t capacity)
{
    if (capacity < s_busy_capture_len) return 0;
    memcpy(out, s_busy_capture, s_busy_capture_len);
    return s_busy_capture_len;
}
size_t bridge_net_udp_get(unsigned index, uint8_t *out, size_t capacity, uint32_t *ip, uint16_t *port)
{
    if (index >= s_udp_count || capacity < s_udp_lengths[index]) return 0;
    memcpy(out, s_udp_bytes[index], s_udp_lengths[index]);
    *ip = s_udp_ips[index];
    *port = s_udp_ports[index];
    return s_udp_lengths[index];
}
const char *bridge_net_log(void) { return s_log; }
