/**
 * @file net_harness.c
 * @brief 生产C的宿主边界夹具。
 * @details 包含真实net_bridge并链接codec/session/link；用可控socket/Wi-Fi/时钟边界驱动网络任务。
 *     脚本状态与断言属于宿主用例，固定接口名及布局保持；不证明实机网络性能。
 */
/*
 * 直接包含生产net_bridge，socket/Wi-Fi/netif/MAC/RTOS由显式脚本驱动。
 * 每项脚本对应一次poll，跳转仅模拟任务停驻，不代表真实调度或网络。
 */
#include "net_bridge.c"
#include <stdarg.h>
#include "sdkconfig.h"
#include "buffer_pool.h"

#define CHECK(c)                                                                                   \
    do                                                                                             \
    {                                                                                              \
        if (!(c))                                                                                  \
        {                                                                                          \
            return __LINE__;                                                                       \
        }                                                                                          \
    } while (0)
#define SCRIPT_MAX 512u
#define SCRIPT_BYTES 1100u
#define CAPTURE_BYTES 65536u
#define UDP_MAX 16u
#define INJECTED 0x6D1

int bridge_stub_errno;

enum
{
    EV_IDLE,
    EV_ACCEPT,
    EV_ACCEPT_EAGAIN,
    EV_RECV,
    EV_RECV_EOF,
    EV_RECV_EAGAIN,
    EV_RECV_ERROR,
    EV_SEND,
    EV_SEND_EAGAIN,
    EV_SEND_ZERO,
    EV_SEND_ERROR,
    EV_BUSY_SEND,
    EV_BUSY_SEND_EAGAIN,
    EV_BUSY_SEND_ZERO,
    EV_BUSY_SEND_ERROR,
    EV_OFFER,
    EV_SET_SPI,
    EV_UDP_FAIL,
    EV_UNBIND,
    EV_BRIDGE_SPI,
    EV_BRIDGE_TIME,
    EV_BRIDGE_SPI_BURST
};

typedef struct
{
    uint32_t kind; /**< 脚本事件码，每项驱动一轮poll。 */
    uint32_t arg; /**< 该事件的fd/IP/短写上限等参数，语义由kind决定。 */
    uint32_t length; /**< bytes内有效输入字节数。 */
    uint8_t bytes[SCRIPT_BYTES]; /**< 本事件输入字节副本，不保留Python指针。 */
} script_entry_t;

static script_entry_t s_script[SCRIPT_MAX];
static script_entry_t s_current;
static uint32_t s_script_len;
static uint32_t s_script_pos;
static int s_violations;
static int s_parked;
static int s_deleted;
static int s_polls;
static int s_max_timeout;
static uint64_t s_clock_ms;
static intptr_t s_jump[5];
static uint32_t s_spi_session_fixture;
static bool s_spi_ready_fixture;
static int s_socket_calls;
static int s_accept_calls;
static int s_sendto_calls;
static int s_live[64];
static int s_live_count;
static int s_next_accept_fd;
static uint8_t s_capture[CAPTURE_BYTES];
static uint8_t s_busy_capture[4096];
static size_t s_capture_len;
static size_t s_busy_capture_len;
static uint8_t s_udp_bytes[UDP_MAX][BRIDGE_NET_UDP_BYTES];
static size_t s_udp_lengths[UDP_MAX];
static uint32_t s_udp_ips[UDP_MAX];
static uint16_t s_udp_ports[UDP_MAX];
static uint32_t s_udp_count;
static int s_udp_fail;
static int s_offer_results[64];
static int s_offer_count;
static char s_log[8192];
static size_t s_log_used;
static int s_init_fail;
static int s_close_log[64];
static int s_close_count;
/* Phase05复用本网络夹具的生产SPI协议与应用所有图像，非真实DMA或RTOS。 */
static bridge_spi_protocol_t s_bridge_protocol;
static bridge_spi_tx_meta_t s_bridge_meta;
static bridge_json_workspace_t s_bridge_workspace;
static uint8_t s_bridge_tx[SCB_SPI_BYTES];
static bool s_bridge_fixture;
_Alignas(64) static uint8_t s_bridge_arena[BRIDGE_POOL_DATA_BYTES + 256];
static size_t s_bridge_arena_used;

/**
 * @brief 为生产固定池提供一次性宿主内存，不替代池额度或归还算法。
 * @param[in] alignment 池请求对齐。
 * @param[in] bytes 池请求字节数。
 * @param[in] dma 本夹具没有真实DMA，仅记录池语义。
 * @return 返回静态arena切片，耗尽返回NULL。
 */
static void *bridge_fixture_alloc(size_t alignment, size_t bytes, bool dma)
{
    (void)dma;
    const size_t start = (s_bridge_arena_used + alignment - 1) & ~(alignment - 1);
    if (start + bytes > sizeof(s_bridge_arena))
    {
        return NULL;
    }
    s_bridge_arena_used = start + bytes;
    return s_bridge_arena + start;
}

/**
 * @brief 静态arena无系统资源需释放，池仍实际校验所有归还凭据。
 * @param[in] ptr arena内已归还的块。
 */
static void bridge_fixture_free(void *ptr)
{
    (void)ptr;
}

/**
 * @brief 宿主固定会话候选，仅用于独立可重现输入。
 * @param[in] context 不使用。
 * @return 夹具指定非零会话。
 */
static uint32_t bridge_fixture_random(void *context)
{
    (void)context;
    return 0x12345678u;
}

/**
 * @brief 输入完整SPI事务，遵循上一TX完成→RX接管→准备下一TX顺序。
 * @param[in] bytes 独立生成的1024B图像。
 */
static void bridge_fixture_transaction(const uint8_t *bytes)
{
    bridge_spi_protocol_sent(&s_bridge_protocol, s_bridge_meta, true, s_clock_ms);
    bridge_spi_protocol_receive(&s_bridge_protocol, bytes, s_bridge_tx,
        &s_bridge_workspace, bridge_fixture_random, NULL);
    (void)bridge_spi_protocol_prepare(&s_bridge_protocol, s_clock_ms, s_bridge_tx,
        &s_bridge_workspace, &s_bridge_meta);
    s_spi_session_fixture = s_bridge_protocol.session.session_id;
    s_spi_ready_fixture = s_bridge_protocol.session.phase == SCB_SESSION_READY;
}

/* 仅用于记录AP配置、步骤及回滚的宿主状态。 */
static const uint8_t s_fixture_mac[6] = {0x24, 0x6F, 0x28, 0x12, 0xAB, 0xCD};
static esp_netif_t s_fake_netif;
static int s_ap_fail_at;
static int s_ap_order[24];
static int s_ap_order_count;
static int s_ap_destroyed;
static int s_ap_deinit;
static wifi_config_t s_ap_config;
static int s_ap_mode;
static esp_netif_ip_info_t s_ap_ip;
static int s_ap_init_magic;

/**
 * @brief 按显式字节数填充夹具缓冲，返回原目标；调用者保证可写范围。
 */
void *memset(void *p, int c, size_t n)
{
    uint8_t *b = p;
    for (size_t i = 0; i < n; ++i)
    {
        b[i] = (uint8_t)c;
    }
    return p;
}

/**
 * @brief 逐字节复制不重叠范围并返回目标，不提供memmove语义；借用仅到返回。
 */
void *memcpy(void *p, const void *s, size_t n)
{
    uint8_t *b = p;
    const uint8_t *v = s;
    for (size_t i = 0; i < n; ++i)
    {
        b[i] = v[i];
    }
    return p;
}

/**
 * @brief 逐字节比较两段借用输入，等长内容全同返回true。
 */
static bool text_equal(const void *a, const void *b, size_t n)
{
    const uint8_t *x = a;
    const uint8_t *y = b;
    for (size_t i = 0; i < n; ++i)
    {
        if (x[i] != y[i])
        {
            return false;
        }
    }
    return true;
}

/**
 * @brief 串行脚本夹具不做实际锁操作；空体不证明跨线程互斥。
 */
void test_lock(void)
{
    /* 串行宿主脚本无真实并发锁；空体保留边界调用。 */
}

/**
 * @brief 串行脚本夹具不做实际解锁；保持SDK边界调用形式。
 */
void test_unlock(void)
{
    /* 串行宿主脚本无真实并发锁；空体保留边界调用。 */
}

/**
 * @brief 有界追加日志单字节并维护尾NUL，满时忽略后续字节。
 */
static void log_char(char value)
{
    if (s_log_used + 1 < sizeof(s_log))
    {
        s_log[s_log_used++] = value;
        s_log[s_log_used] = 0;
    }
}

/**
 * @brief 追加非空NUL结尾文本到夹具日志，不借出输入。
 */
static void log_text(const char *value)
{
    while (*value)
    {
        log_char(*value++);
    }
}

/**
 * @brief 以十进制追加无符号值到有界日志。
 */
static void log_uint(unsigned value)
{
    char digits[10];
    int count = 0;
    do
    {
        digits[count++] = (char)('0' + value % 10);
        value /= 10;
    } while (value);
    while (count)
    {
        log_char(digits[--count]);
    }
}

/**
 * @brief 按夹具需要收集或忽略格式化诊断，不把日志送到业务socket。
 */
void test_log(const char *tag, const char *format, ...)
{
    log_text(tag);
    log_char(':');
    va_list args;
    va_start(args, format);
    while (*format)
    {
        if (*format++ != '%')
        {
            log_char(format[-1]);
            continue;
        }
        const char conversion = *format++;
        if (conversion == 's')
        {
            log_text(va_arg(args, const char *));
        }
        else if (conversion == 'u')
        {
            log_uint(va_arg(args, unsigned));
        }
        else if (conversion == 'l' && *format == 'u')
        {
            ++format;
            log_uint((unsigned)va_arg(args, unsigned long));
        }
        else if (conversion == 'd')
        {
            int value = va_arg(args, int);
            if (value < 0)
            {
                log_char('-');
                log_uint((unsigned)(-(value + 1)) + 1);
            }
            else
            {
                log_uint((unsigned)value);
            }
        }
        else
        {
            ++s_violations;
        }
    }
    va_end(args);
    log_char('\n');
}

/**
 * @brief 将零/非零注入错误转为固定静态字面量，返回指针无需释放。
 */
const char *esp_err_to_name(esp_err_t error)
{
    return error == ESP_OK ? "ESP_OK" : "INJECTED";
}

/**
 * @brief 把本夹具毫秒时钟转换成微秒返回，不读取真实时钟。
 */
int64_t esp_timer_get_time(void)
{
    return (int64_t)s_clock_ms * 1000;
}

/**
 * @brief 忽略任务参数并返回固定栈余量夹具值，不作为真实栈测量。
 */
UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t task)
{
    (void)task;
    return 4096;
}

/**
 * @brief 返回本夹具模拟核号，用于任务配置断言，不是实测CPU亲和性。
 */
int xPortGetCoreID(void)
{
    return BRIDGE_NET_CORE;
}

/**
 * @brief 初始化调用者提供的静态信号量token，按夹具失败设置返回句柄或NULL。
 */
SemaphoreHandle_t xSemaphoreCreateBinaryStatic(StaticSemaphore_t *storage)
{
    storage->token = 0;
    return storage;
}

/**
 * @brief 记录初始化完成token，SPI夹具还核对发布结果；只模拟通知顺序。
 */
BaseType_t xSemaphoreGive(SemaphoreHandle_t semaphore)
{
    ++semaphore->token;
    return pdTRUE;
}

/**
 * @brief 核对预期等待参数和token后消费通知；不执行真实阻塞，返回夹具接受结果。
 */
BaseType_t xSemaphoreTake(SemaphoreHandle_t semaphore, uint32_t ticks)
{
    if (ticks != portMAX_DELAY || semaphore->token != 1)
    {
        ++s_violations;
        return 0;
    }
    --semaphore->token;
    return pdTRUE;
}

/**
 * @brief 记录任务结束并核对空任务参数，不释放真实RTOS资源。
 */
void vTaskDelete(TaskHandle_t task)
{
    if (task)
    {
        ++s_violations;
    }
    ++s_deleted;
}

/**
 * @brief 核对任务栈/优先级/核号等固定参数，按注入失败或同步执行任务；跳转模拟停驻。
 */
TaskHandle_t xTaskCreateStaticPinnedToCore(TaskFunction_t task, const char *name, uint32_t bytes,
    void *arg, UBaseType_t priority, StackType_t *stack, StaticTask_t *storage, BaseType_t core)
{
    if (bytes != BRIDGE_NET_STACK_BYTES || priority != BRIDGE_NET_PRIORITY
        || core != BRIDGE_NET_CORE || stack != s_stack || storage != &s_task_storage || !name)
    {
        ++s_violations;
    }
    if (!__builtin_setjmp((void **)s_jump))
    {
        task(arg);
    }
    return storage;
}

/* SPI是独立生产owner；网络夹具仅提供其发布快照，不运行SPI驱动。 */
/**
 * @brief 返回可控SPI session/ready副本，空输出拒绝，不执行SPI生产任务。
 */
esp_err_t bridge_spi_snapshot(bridge_spi_snapshot_t *snapshot)
{
    if (!snapshot)
    {
        return ESP_ERR_INVALID_ARG;
    }
    *snapshot = (bridge_spi_snapshot_t){0};
    snapshot->session_id = s_spi_session_fixture;
    snapshot->link_ready = s_spi_ready_fixture;
    return ESP_OK;
}

/**
 * @brief 扫描活fd表，找到返回true；只反映本夹具资源表。
 */
static bool fd_live(int fd)
{
    for (int i = 0; i < s_live_count; ++i)
    {
        if (s_live[i] == fd)
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief 向活fd表登记句柄，表满记违规而不越界。
 */
static void fd_add(int fd)
{
    if (s_live_count < 64)
    {
        s_live[s_live_count++] = fd;
    }
    else
    {
        ++s_violations;
    }
}

/**
 * @brief 核对域/类型并按创建序号返回虚拟fd或注入EAGAIN，不创建真实socket。
 */
int socket(int domain, int type, int protocol)
{
    (void)protocol;
    if (domain != AF_INET || (type != SOCK_STREAM && type != SOCK_DGRAM))
    {
        ++s_violations;
    }
    const int call = s_socket_calls++;
    if ((s_init_fail == 1 && call == 0) || (s_init_fail == 5 && call == 1))
    {
        bridge_stub_errno = EAGAIN;
        return -1;
    }
    const int fd = 100 + call;
    fd_add(fd);
    return fd;
}

/**
 * @brief 核对192.168.4.1及TCP端口/布局，按失败步骤返回零或-1/EPIPE。
 */
int bind(int s, const struct sockaddr *name, socklen_t namelen)
{
    if (s != 100 || namelen != sizeof(struct sockaddr_in))
    {
        ++s_violations;
    }
    const struct sockaddr_in *address = (const struct sockaddr_in *)name;
    if (address->sin_family != AF_INET || address->sin_port != htons(BRIDGE_NET_TCP_PORT)
        || address->sin_addr.s_addr != htonl(0xC0A80401u))
    {
        ++s_violations;
    }
    if (s_init_fail == 2)
    {
        bridge_stub_errno = EPIPE;
        return -1;
    }
    return 0;
}

/**
 * @brief 核对监听fd与backlog，按失败步骤返回零或-1/EPIPE。
 */
int listen(int s, int backlog)
{
    if (s != 100 || backlog < 2)
    {
        ++s_violations;
    }
    if (s_init_fail == 3)
    {
        bridge_stub_errno = EPIPE;
        return -1;
    }
    return 0;
}

/**
 * @brief 核对活fd及F_SETFL/O_NONBLOCK参数，注入指定失败；不修改真实描述符。
 */
int fcntl(int s, int cmd, ...)
{
    va_list args;
    va_start(args, cmd);
    const int value = va_arg(args, int);
    va_end(args);
    if (cmd != F_SETFL || !(value & O_NONBLOCK) || !fd_live(s))
    {
        ++s_violations;
    }
    if (s_init_fail == 4 && s == 100)
    {
        bridge_stub_errno = EPIPE;
        return -1;
    }
    return 0;
}

/**
 * @brief 从活表删除fd并记录关闭顺序；无效fd返回-1并记违规。
 */
int close(int s)
{
    if (!fd_live(s))
    {
        ++s_violations;
        return -1;
    }
    for (int i = 0; i < s_live_count; ++i)
    {
        if (s_live[i] == s)
        {
            s_live[i] = s_live[--s_live_count];
            break;
        }
    }
    if (s_close_count < 64)
    {
        s_close_log[s_close_count] = s;
    }
    ++s_close_count;
    return 0;
}

/**
 * @brief 按本轮脚本写固定布局对端IP/端口并登记新fd，非accept事件返回-1/EAGAIN。
 */
int accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    ++s_accept_calls;
    if (s != s_listen || !addrlen || *addrlen < (socklen_t)sizeof(struct sockaddr_in))
    {
        ++s_violations;
    }
    if (s_current.kind == EV_ACCEPT_EAGAIN)
    {
        bridge_stub_errno = EAGAIN;
        return -1;
    }
    if (s_current.kind != EV_ACCEPT)
    {
        ++s_violations;
        bridge_stub_errno = EAGAIN;
        return -1;
    }
    struct sockaddr_in *peer = (struct sockaddr_in *)addr;
    peer->sin_family = AF_INET;
    peer->sin_addr.s_addr = s_current.arg;
    peer->sin_port = htons(5001);
    *addrlen = sizeof(struct sockaddr_in);
    const int fd = s_next_accept_fd++;
    fd_add(fd);
    return fd;
}

/**
 * @brief 按脚本复制指定切片或返回EOF/EAGAIN/EPIPE，验证目标容量；错误不发布部分字节。
 */
ssize_t recv(int s, void *mem, size_t len, int flags)
{
    if (flags != 0 || s != s_conn || s < 0)
    {
        ++s_violations;
        bridge_stub_errno = EPIPE;
        return -1;
    }
    if (s_current.kind == EV_RECV)
    {
        if (s_current.length > len || s_current.length > SCRIPT_BYTES)
        {
            ++s_violations;
            bridge_stub_errno = EPIPE;
            return -1;
        }
        memcpy(mem, s_current.bytes, s_current.length);
        return (ssize_t)s_current.length;
    }
    if (s_current.kind == EV_RECV_EOF)
    {
        return 0;
    }
    if (s_current.kind == EV_RECV_EAGAIN)
    {
        bridge_stub_errno = EAGAIN;
        return -1;
    }
    if (s_current.kind == EV_RECV_ERROR)
    {
        bridge_stub_errno = EPIPE;
        return -1;
    }
    ++s_violations;
    bridge_stub_errno = EPIPE;
    return -1;
}

/**
 * @brief 按脚本限制正数发送字节并追加捕获，或返回零/暂时/硬错误；不修改原图像。
 */
static ssize_t send_capped(int s, const void *data, size_t size, uint8_t *capture, size_t *used,
    size_t capacity, uint32_t cap_kind, uint32_t eagain_kind, uint32_t zero_kind,
    uint32_t error_kind)
{
    (void)s;
    if (s_current.kind == cap_kind)
    {
        size_t n = s_current.arg < size ? s_current.arg : size;
        if (*used + n > capacity)
        {
            ++s_violations;
            n = 0;
        }
        memcpy(capture + *used, data, n);
        *used += n;
        return (ssize_t)n;
    }
    if (s_current.kind == zero_kind)
    {
        return 0;
    }
    if (s_current.kind == eagain_kind)
    {
        bridge_stub_errno = EAGAIN;
        return -1;
    }
    if (s_current.kind == error_kind)
    {
        bridge_stub_errno = EPIPE;
        return -1;
    }
    ++s_violations;
    bridge_stub_errno = EPIPE;
    return -1;
}

/**
 * @brief 只为活控制/BUSY fd选择独立捕获区及脚本事件，其他情况-1/EPIPE并记违规。
 */
ssize_t send(int s, const void *data, size_t size, int flags)
{
    if (flags != 0 || !fd_live(s))
    {
        ++s_violations;
        bridge_stub_errno = EPIPE;
        return -1;
    }
    if (s == s_conn)
    {
        return send_capped(s, data, size, s_capture, &s_capture_len, sizeof(s_capture), EV_SEND,
            EV_SEND_EAGAIN, EV_SEND_ZERO, EV_SEND_ERROR);
    }
    if (s == s_busy)
    {
        return send_capped(s, data, size, s_busy_capture, &s_busy_capture_len,
            sizeof(s_busy_capture), EV_BUSY_SEND, EV_BUSY_SEND_EAGAIN, EV_BUSY_SEND_ZERO,
            EV_BUSY_SEND_ERROR);
    }
    ++s_violations;
    bridge_stub_errno = EPIPE;
    return -1;
}

/**
 * @brief 按脚本捕获完整UDP图像及目的IP/端口；失败或容量满返回-1，不发送真实网络。
 */
ssize_t sendto(
    int s, const void *data, size_t size, int flags, const struct sockaddr *to, socklen_t tolen)
{
    ++s_sendto_calls;
    if (s != s_udp || flags != 0 || tolen != sizeof(struct sockaddr_in)
        || size > BRIDGE_NET_UDP_BYTES)
    {
        ++s_violations;
    }
    const struct sockaddr_in *destination = (const struct sockaddr_in *)to;
    if (destination->sin_family != AF_INET)
    {
        ++s_violations;
    }
    if (s_udp_fail)
    {
        bridge_stub_errno = EAGAIN;
        return -1;
    }
    if (s_udp_count >= UDP_MAX)
    {
        ++s_violations;
        bridge_stub_errno = EAGAIN;
        return -1;
    }
    memcpy(s_udp_bytes[s_udp_count], data, size);
    s_udp_lengths[s_udp_count] = size;
    s_udp_ips[s_udp_count] = destination->sin_addr.s_addr;
    s_udp_ports[s_udp_count] = htons(destination->sin_port);
    ++s_udp_count;
    return (ssize_t)size;
}

/**
 * @brief 逐轮读取脚本设置revents并推进虚拟毫秒时间；核对等待上限，脚本耗尽跳回停驻点。
 */
int poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
    if (timeout < 0 || timeout > BRIDGE_NET_POLL_LIMIT_MS || nfds > 3)
    {
        ++s_violations;
    }
    if (timeout > s_max_timeout)
    {
        s_max_timeout = timeout;
    }
    for (nfds_t i = 0; i < nfds; ++i)
    {
        if (!fd_live(fds[i].fd))
        {
            ++s_violations;
        }
        fds[i].revents = 0;
    }
    ++s_polls;
    if (s_script_pos >= s_script_len)
    {
        s_parked = 1;
        __builtin_longjmp((void **)s_jump, 1);
    }
    s_current = s_script[s_script_pos++];
    switch (s_current.kind)
    {
        case EV_IDLE:
            s_clock_ms += (uint32_t)timeout;
            return 0;
        case EV_ACCEPT:
        case EV_ACCEPT_EAGAIN:
        {
            int found = 0;
            for (nfds_t i = 0; i < nfds; ++i)
            {
                if (fds[i].fd == s_listen)
                {
                    fds[i].revents = POLLIN;
                    ++found;
                }
            }
            if (!found)
            {
                ++s_violations;
            }
            return found;
        }
        case EV_RECV:
        case EV_RECV_EOF:
        case EV_RECV_EAGAIN:
        case EV_RECV_ERROR:
        {
            int found = 0;
            for (nfds_t i = 0; i < nfds; ++i)
            {
                if (fds[i].fd == s_conn)
                {
                    fds[i].revents |= POLLIN;
                    ++found;
                }
            }
            if (!found)
            {
                ++s_violations;
            }
            return found;
        }
        case EV_SEND:
        case EV_SEND_EAGAIN:
        case EV_SEND_ZERO:
        case EV_SEND_ERROR:
        {
            /* 本轮没有待发图像时，不报告可写事件；捕获字节断言核对遗漏。 */
            for (nfds_t i = 0; i < nfds; ++i)
            {
                if (fds[i].fd == s_conn && (fds[i].events & POLLOUT))
                {
                    fds[i].revents |= POLLOUT;
                    return 1;
                }
            }
            return 0;
        }
        case EV_BUSY_SEND:
        case EV_BUSY_SEND_EAGAIN:
        case EV_BUSY_SEND_ZERO:
        case EV_BUSY_SEND_ERROR:
        {
            for (nfds_t i = 0; i < nfds; ++i)
            {
                if (fds[i].fd == s_busy)
                {
                    fds[i].revents |= POLLOUT;
                    return 1;
                }
            }
            return 0;
        }
        case EV_OFFER:
            if (s_offer_count < 64)
            {
                s_offer_results[s_offer_count]
                    = bridge_net_telemetry_offer(s_current.bytes, s_current.length);
            }
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
        case EV_BRIDGE_SPI:
            bridge_fixture_transaction(s_current.bytes);
            return 0;
        case EV_BRIDGE_TIME:
            s_clock_ms = s_current.arg;
            bridge_spi_protocol_poll(&s_bridge_protocol, s_clock_ms);
            return 0;
        case EV_BRIDGE_SPI_BURST:
            for (uint32_t i = 0; i < s_current.arg && s_script_pos < s_script_len; ++i)
            {
                const script_entry_t *entry = &s_script[s_script_pos++];
                if (entry->kind != EV_BRIDGE_SPI || entry->length != SCB_SPI_BYTES)
                {
                    ++s_violations;
                    break;
                }
                bridge_fixture_transaction(entry->bytes);
            }
            return 0;
        default:
            ++s_violations;
            return 0;
    }
}

/* 记录AP步骤并注入指定失败，不创建真实热点。 */
/**
 * @brief 有界记录AP步骤码，溢出记违规。
 */
static void ap_record(int code)
{
    if (s_ap_order_count < 24)
    {
        s_ap_order[s_ap_order_count++] = code;
    }
    else
    {
        ++s_violations;
    }
}

/**
 * @brief 记录AP创建步骤，按注入返回静态netif或NULL。
 */
esp_netif_t *esp_netif_create_default_wifi_ap(void)
{
    ap_record(1);
    if (s_ap_fail_at == 1)
    {
        return NULL;
    }
    return &s_fake_netif;
}

/**
 * @brief 核对并记录静态AP接口销毁，不释放真实SDK资源。
 */
void esp_netif_destroy_default_wifi(void *netif)
{
    ap_record(10);
    if (netif != &s_fake_netif)
    {
        ++s_violations;
    }
    ++s_ap_destroyed;
}

/**
 * @brief 记录停止DHCP步骤并核对接口，返回对应注入结果。
 */
esp_err_t esp_netif_dhcps_stop(esp_netif_t *netif)
{
    ap_record(2);
    if (netif != &s_fake_netif)
    {
        ++s_violations;
    }
    return s_ap_fail_at == 2 ? INJECTED : ESP_OK;
}

/**
 * @brief 复制待设置IP信息供断言并记录步骤，返回对应注入结果。
 */
esp_err_t esp_netif_set_ip_info(esp_netif_t *netif, const esp_netif_ip_info_t *info)
{
    ap_record(3);
    if (netif != &s_fake_netif)
    {
        ++s_violations;
    }
    s_ap_ip = *info;
    return s_ap_fail_at == 3 ? INJECTED : ESP_OK;
}

/**
 * @brief 记录开启DHCP步骤并核对接口，返回对应注入结果。
 */
esp_err_t esp_netif_dhcps_start(esp_netif_t *netif)
{
    ap_record(4);
    if (netif != &s_fake_netif)
    {
        ++s_violations;
    }
    return s_ap_fail_at == 4 ? INJECTED : ESP_OK;
}

/**
 * @brief 记录并核对默认初始化magic，按步骤注入返回值，不使用无线硬件。
 */
esp_err_t esp_wifi_init(const wifi_init_config_t *config)
{
    ap_record(5);
    s_ap_init_magic = config ? config->magic : 0;
    return s_ap_fail_at == 5 ? INJECTED : ESP_OK;
}

/**
 * @brief 记录Wi-Fi回滚，核对其确已初始化。
 */
esp_err_t esp_wifi_deinit(void)
{
    ap_record(11);
    ++s_ap_deinit;
    return ESP_OK;
}

/**
 * @brief 记录AP模式参数并返回对应注入结果。
 */
esp_err_t esp_wifi_set_mode(wifi_mode_t mode)
{
    ap_record(6);
    s_ap_mode = (int)mode;
    return s_ap_fail_at == 6 ? INJECTED : ESP_OK;
}

/**
 * @brief 核对AP MAC类型，成功复制固定六字节夹具MAC，失败不提供真实地址。
 */
esp_err_t esp_read_mac(uint8_t *mac, esp_mac_type_t type)
{
    ap_record(7);
    if (type != ESP_MAC_WIFI_SOFTAP)
    {
        ++s_violations;
    }
    if (s_ap_fail_at == 7)
    {
        return INJECTED;
    }
    memcpy(mac, s_fixture_mac, 6);
    return ESP_OK;
}

/**
 * @brief 核对AP接口并复制配置供断言，按步骤返回注入结果。
 */
esp_err_t esp_wifi_set_config(wifi_interface_t interface, wifi_config_t *conf)
{
    ap_record(8);
    if (interface != WIFI_IF_AP)
    {
        ++s_violations;
    }
    s_ap_config = *conf;
    return s_ap_fail_at == 8 ? INJECTED : ESP_OK;
}

/**
 * @brief 记录Wi-Fi启动步骤并返回注入错误或ESP_OK。
 */
esp_err_t esp_wifi_start(void)
{
    ap_record(9);
    return s_ap_fail_at == 9 ? INJECTED : ESP_OK;
}

/**
 * @brief 查找夹具日志是否包含NUL结尾needle，供诊断与密码不泄漏断言。
 */
static bool log_has(const char *needle)
{
    size_t length = 0;
    while (needle[length])
    {
        ++length;
    }
    if (!length)
    {
        return true;
    }
    for (size_t i = 0; i + length <= s_log_used; ++i)
    {
        if (text_equal(s_log + i, needle, length))
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief 重置生产网络静态运行状态与脚本捕获，供本夹具用例隔离，不提供产品复位接口。
 */
static void reset_runtime(void)
{
    s_bridge_bound = false;
    s_bridge_fixture = false;
    s_generation_high = 0;
    s_subscription_generation = 0;
    s_bridge_udp_seq = 1;
    s_started = false;
    s_init_error = ESP_ERR_INVALID_STATE;
    s_health = (bridge_net_snapshot_t){.init_error = ESP_ERR_INVALID_STATE};
    s_port = (bridge_net_port_t){0};
    s_boot = NULL;
    s_task = NULL;
    s_listen = -1;
    s_conn = -1;
    s_busy = -1;
    s_udp = -1;
    s_udp_length = 0;
    s_udp_generation = 0;
    s_udp_pending = s_udp_in_use = s_udp_overflow = false;
    s_rx_length = 0;
    s_tx_length = s_tx_cursor = 0;
    s_tx_seq = 0;
    s_tx_progress = 0;
    s_tx_reliable = s_want_idle = false;
    s_busy_length = s_busy_cursor = 0;
    s_busy_deadline = 0;
    s_link = (scb_link_t){0};
    s_generation = 0;
    s_peer_ip = 0;
    s_sub_port = 0;
    s_subscribed = s_hello_done = false;
    s_spi_session = 0;
    s_spi_known = s_spi_ready = false;
    s_boot_storage = (StaticSemaphore_t){0};
}

/**
 * @brief 先重置运行状态，再重置AP注入及绑定出口记录。
 */
static void reset_fixture(void)
{
    s_script_len = s_script_pos = 0;
    s_violations = s_parked = s_deleted = s_polls = s_max_timeout = 0;
    s_clock_ms = 0;
    s_spi_session_fixture = 0;
    s_spi_ready_fixture = false;
    s_socket_calls = s_accept_calls = s_sendto_calls = 0;
    s_live_count = 0;
    s_next_accept_fd = 200;
    s_capture_len = s_busy_capture_len = 0;
    s_udp_count = 0;
    s_udp_fail = 0;
    s_offer_count = 0;
    s_log_used = 0;
    s_log[0] = 0;
    s_init_fail = 0;
    s_close_count = 0;
    bridge_stub_errno = 0;
    reset_runtime();
}

/**
 * @brief 供Python入口重置本夹具全部状态。
 */
void fixture_reset(void)
{
    reset_fixture();
}

/**
 * @brief 复用生产池、SPI端口和NET绑定启用Phase05贯通模式。
 * @return 零成功，否则实际池或绑定错误。
 */
int fixture_bridge_enable(void)
{
    const bridge_spi_port_t port = bridge_state_spi_port();
    port.session_changed(port.context, 0, false);
    const esp_err_t deinit = bridge_pool_deinit();
    if (deinit != ESP_OK)
    {
        return deinit;
    }
    s_bridge_arena_used = 0;
    const bridge_pool_ops_t ops =
    {
        bridge_fixture_alloc, bridge_fixture_free, test_lock, test_unlock
    };
    const esp_err_t init = bridge_pool_init(&ops);
    if (init != ESP_OK)
    {
        return init;
    }
    bridge_spi_protocol_init(&s_bridge_protocol, &port);
    s_bridge_meta = (bridge_spi_tx_meta_t){0};
    s_bridge_fixture = true;
    return bridge_net_bind_bridge();
}

/**
 * @brief 返回生产状态快照的单个验证字段，不重算桥接行为。
 * @param[in] index 0信用、1墓碑、2丢弃、3峰值、4超时、5孤儿、6取消、7背压、8事件峰值。
 * @return 对应实际累计计数或状态，未知索引返回0。
 */
uint32_t fixture_bridge_metric(unsigned index)
{
    bridge_state_snapshot_t snapshot;
    bridge_state_snapshot(&snapshot);
    switch (index)
    {
        case 0: return snapshot.credit_mask;
        case 1: return snapshot.tombstone_mask;
        case 2: return snapshot.telemetry_dropped;
        case 3: return snapshot.telemetry_peak;
        case 4: return snapshot.export_timeouts;
        case 5: return snapshot.orphan_results;
        case 6: return snapshot.canceled_results;
        case 7: return snapshot.command_backpressure;
        case 8: return snapshot.event_peak;
        default: return 0;
    }
}

/**
 * @brief 读取生产SPI协议准备的下次事务图像，不重编码。
 * @param[out] bytes 非空至少1024B。
 */
void fixture_bridge_spi_get(uint8_t *bytes)
{
    memcpy(bytes, s_bridge_tx, SCB_SPI_BYTES);
}

/**
 * @brief 把显式session及ready值设为下一次SPI快照输入。
 */
void fixture_spi(uint32_t session, int ready)
{
    s_spi_session_fixture = session;
    s_spi_ready_fixture = ready != 0;
}

/**
 * @brief 有容量才复制一条显式脚本；长度/条数越界记违规，不无限追加。
 */
void fixture_push(uint32_t kind, uint32_t arg, const uint8_t *bytes, uint32_t length)
{
    if (s_script_len >= SCRIPT_MAX || length > SCRIPT_BYTES)
    {
        ++s_violations;
        return;
    }
    script_entry_t *entry = &s_script[s_script_len++];
    entry->kind = kind;
    entry->arg = arg;
    entry->length = length;
    if (bytes && length)
    {
        memcpy(entry->bytes, bytes, length);
    }
}

/**
 * @brief 创建生产网络任务并运行至脚本停驻，返回零或违规结果，不真实调度任务。
 */
int bridge_net_run(void)
{
    const bridge_net_port_t port = {NULL, bridge_net_socket_telemetry};
    if (bridge_net_configure(&port) != ESP_OK)
    {
        return __LINE__;
    }
    const esp_err_t result = bridge_net_start(NULL);
    if (result != ESP_OK)
    {
        return __LINE__;
    }
    if (!s_parked || s_deleted || s_violations)
    {
        return __LINE__;
    }
    if (s_listen < 0 || !fd_live(s_listen))
    {
        return __LINE__;
    }
    return 0;
}

/**
 * @brief 按scenario注入网络初始化失败并核对资源/快照；零通过，否则失败行号。
 */
int bridge_net_start_fail_test(int scenario)
{
    reset_fixture();
    s_init_fail = scenario;
    const bridge_net_port_t port = {NULL, bridge_net_socket_telemetry};
    if (bridge_net_configure(&port) != ESP_OK)
    {
        return __LINE__;
    }
    const esp_err_t result = bridge_net_start(NULL);
    if (scenario >= 1 && scenario <= 4)
    {
        CHECK(result == ESP_FAIL && s_deleted == 1 && !s_parked);
        CHECK(s_listen < 0 && s_live_count == (scenario == 1 ? 0 : 0));
        CHECK(s_health.init_error == ESP_FAIL);
    }
    else if (scenario == 5)
    {
        /* UDP socket失败不停止服务，后续报文只增加发送失败计数。 */
        CHECK(result == ESP_OK && s_parked == 1);
        CHECK(s_udp < 0 && s_listen >= 0);
    }
    else
    {
        return __LINE__;
    }
    CHECK(s_violations == 0);
    return 0;
}

/**
 * @brief 核对已捕获WPA2、SSID、地址、密码配置及步骤；零通过，否则失败行号。
 */
static int check_ap_success(void)
{
    static const int expected[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    CHECK(s_ap_order_count == 9);
    for (int i = 0; i < 9; ++i)
    {
        CHECK(s_ap_order[i] == expected[i]);
    }
    CHECK(s_ap_ip.ip.addr == ESP_IP4TOADDR(192, 168, 4, 1));
    CHECK(s_ap_ip.gw.addr == ESP_IP4TOADDR(192, 168, 4, 1));
    CHECK(s_ap_ip.netmask.addr == ESP_IP4TOADDR(255, 255, 255, 0));
    CHECK(s_ap_init_magic == 0x1F2F3F4F);
    CHECK(s_ap_mode == WIFI_MODE_AP);
    static const char expected_ssid[] = "SmartCar-ABCD";
    for (int i = 0; i < 13; ++i)
    {
        CHECK(s_ap_config.ap.ssid[i] == (uint8_t)expected_ssid[i]);
    }
    CHECK(s_ap_config.ap.ssid_len == 13 && s_ap_config.ap.channel == 1);
    CHECK(s_ap_config.ap.authmode == WIFI_AUTH_WPA2_PSK && s_ap_config.ap.max_connection == 4);
    static const char password[] = CONFIG_BRIDGE_AP_PASSWORD;
    size_t length = 0;
    while (password[length])
    {
        ++length;
    }
    CHECK(length >= 8 && length <= 63);
    CHECK(text_equal(s_ap_config.ap.password, password, length));
    CHECK(s_ap_destroyed == 0 && s_ap_deinit == 0);
    return 0;
}

/**
 * @brief 按scenario运行真实AP启动对替身的调用，验证成功或失败回滚；零通过，否则失败行号。
 */
int bridge_net_ap_test(int scenario)
{
    reset_fixture();
    s_ap_fail_at = scenario;
    s_ap_order_count = 0;
    s_ap_destroyed = 0;
    s_ap_deinit = 0;
    s_ap_config = (wifi_config_t){0};
    s_ap_mode = -1;
    s_ap_ip = (esp_netif_ip_info_t){0};
    s_ap_init_magic = 0;
    const esp_err_t result = bridge_net_ap_start(NULL);
    if (scenario == 0)
    {
        CHECK(result == ESP_OK);
        CHECK(check_ap_success() == 0);
    }
    else if (scenario >= 1 && scenario <= 9)
    {
        CHECK(result != ESP_OK);
        CHECK(s_ap_destroyed == (scenario == 1 ? 0 : 1));
        CHECK(s_ap_deinit == (scenario >= 6 ? 1 : 0));
        /* 失败后的步骤不得执行，回滚顺序保持deinit后destroy。 */
        const int expected_total
            = scenario == 1 ? 1 : (scenario >= 6 ? scenario + 2 : scenario + 1);
        CHECK(s_ap_order_count == expected_total);
        if (scenario >= 6)
        {
            CHECK(s_ap_order[s_ap_order_count - 2] == 11 && s_ap_order[s_ap_order_count - 1] == 10);
        }
        else if (scenario >= 2)
        {
            CHECK(s_ap_order[s_ap_order_count - 1] == 10);
        }
    }
    else
    {
        return __LINE__;
    }
    static const char password[] = CONFIG_BRIDGE_AP_PASSWORD;
    CHECK(!log_has(password));
    CHECK(s_violations == 0);
    return 0;
}

/*
 * 直接构造旧连接代际驱动udp_service，核对延迟图像不流入新连接。
 * 此检查不证明跨核调度与数据竞争的硬件行为。
 */
static bool s_fixture_udp_called;

/**
 * @brief 只标记本次UDP回调已调用并返回true，不读取图像或发送网络。
 */
static bool fixture_udp(
    void *context, uint32_t ip, uint16_t port, const uint8_t *frame, size_t length)
{
    (void)context;
    (void)ip;
    (void)port;
    (void)frame;
    (void)length;
    s_fixture_udp_called = true;
    return true;
}

/**
 * @brief 直接构造旧代际、匹配代际及无连接场景，验证消费归因；零通过，否则失败行号。
 */
int bridge_net_generation_test(void)
{
    reset_fixture();
    const bridge_net_port_t port = {NULL, fixture_udp};
    CHECK(bridge_net_configure(&port) == ESP_OK);
    bridge_net_snapshot_t health = {0};
    s_generation = 7;
    s_conn = 200;
    s_subscribed = true;
    s_sub_port = 5001;
    s_udp_generation = 6;
    s_udp_length = 152;
    s_udp_pending = true;
    udp_service(&health);
    CHECK(!s_fixture_udp_called && health.udp_dropped == 1 && !s_udp_pending);
    /* 匹配代际才可到达已绑定的出口回调。 */
    s_udp_generation = 7;
    s_udp_length = 152;
    s_udp_pending = true;
    udp_service(&health);
    CHECK(s_fixture_udp_called && health.udp_sent == 1);
    /* 无控制连接时只计丢弃，不调用出口。 */
    s_conn = -1;
    s_subscribed = false;
    s_udp_generation = 7;
    s_udp_pending = true;
    udp_service(&health);
    CHECK(health.udp_dropped == 2 && health.udp_sent == 1);
    CHECK(s_violations == 0);
    return 0;
}

/**
 * @brief 返回生产网络快照类型大小供Python布局核对。
 */
size_t bridge_net_snapshot_size(void)
{
    return sizeof(bridge_net_snapshot_t);
}

/**
 * @brief 按既有id读取违规、资源、时钟及脚本计数，未知id返回99。
 */
unsigned bridge_net_metric(unsigned id)
{
    switch (id)
    {
        case 0:
            return (unsigned)s_violations;
        case 1:
            return (unsigned)s_parked;
        case 2:
            return (unsigned)s_polls;
        case 3:
            return (unsigned)s_accept_calls;
        case 4:
            return (unsigned)s_close_count;
        case 5:
            return (unsigned)s_udp_count;
        case 6:
            return (unsigned)s_deleted;
        case 7:
            return (unsigned)s_sendto_calls;
        case 8:
            return (unsigned)s_clock_ms;
        case 9:
            return (unsigned)s_max_timeout;
        case 10:
            return (unsigned)s_offer_count;
        case 11:
            return (unsigned)s_live_count;
        default:
            return 99;
    }
}

/**
 * @brief 按索引返回offer结果，越界返回-99。
 */
int bridge_net_offer_result(unsigned index)
{
    return index < (unsigned)s_offer_count ? s_offer_results[index] : -99;
}

/**
 * @brief 按索引返回关闭fd，越界返回-1。
 */
int bridge_net_close_event(unsigned index)
{
    return index < (unsigned)s_close_count ? s_close_log[index] : -1;
}

/**
 * @brief 容量足够才复制控制发送捕获并返回长度，否则返回零且不写。
 */
size_t bridge_net_capture(uint8_t *out, size_t capacity)
{
    if (capacity < s_capture_len)
    {
        return 0;
    }
    memcpy(out, s_capture, s_capture_len);
    return s_capture_len;
}

/**
 * @brief 容量足够才复制BUSY捕获并返回长度，否则返回零且不写。
 */
size_t bridge_net_busy_capture(uint8_t *out, size_t capacity)
{
    if (capacity < s_busy_capture_len)
    {
        return 0;
    }
    memcpy(out, s_busy_capture, s_busy_capture_len);
    return s_busy_capture_len;
}

/**
 * @brief 索引及容量有效才复制UDP并写IP/端口；失败返回零且保留所有输出。
 */
size_t bridge_net_udp_get(
    unsigned index, uint8_t *out, size_t capacity, uint32_t *ip, uint16_t *port)
{
    if (index >= s_udp_count || capacity < s_udp_lengths[index])
    {
        return 0;
    }
    memcpy(out, s_udp_bytes[index], s_udp_lengths[index]);
    *ip = s_udp_ips[index];
    *port = s_udp_ports[index];
    return s_udp_lengths[index];
}

/**
 * @brief 返回静态日志借用指针，在下次写入或重置前有效。
 */
const char *bridge_net_log(void)
{
    return s_log;
}
