/**
 * @file spi_driver_harness.c
 * @brief 生产C的宿主边界夹具。
 * @details 包含真实pool/spi_bridge，以脚本化驱动、调度、时钟和GPIO替身核对事务所有权及READY调用。
 *     状态仅属本宿主夹具；跳转模拟任务停驻，不证明RTOS、ISR或电气行为。
 */
/* 直接包含生产任务和池；调度及驱动是宿主替身，跳转只模拟任务停驻。 */
#include "buffer_pool.c"
#include "spi_bridge.c"
#include <stdarg.h>
#define CHECK(c)                                                                                   \
    do                                                                                             \
    {                                                                                              \
        if (!(c))                                                                                  \
        {                                                                                          \
            return __LINE__;                                                                       \
        }                                                                                          \
    } while (0)
#define TRACE_MAX 32u
static _Alignas(64) uint8_t storage[4][BRIDGE_EVENT_SLOTS * BRIDGE_EVENT_BYTES];
static int alloc_count;
static int driver_count;
static int task_count;
static int violations;
static int core_value;
static int driver_error;
static int create_fail;
static int semaphore_fail;
static int parked;
static int deleted;
static int queue_error_at;
static int get_error_at;
static int alien_at;
static int ready_level;
static int lock_depth;
static spi_bus_config_t observed_bus;
static spi_slave_interface_config_t observed_slave;
static intptr_t jump_buffer[5];
static uint8_t input[TRACE_MAX][1024];
static uint8_t captured[TRACE_MAX][1024];
static uint8_t saved_tx[1024];
static uint8_t saved_rx[1024];
static uint8_t saved_desc[sizeof(spi_slave_transaction_t)];
static uint32_t bits[TRACE_MAX];
static uint32_t waits[TRACE_MAX + 1];
static uint32_t input_count;
static uint32_t completed_count;
static uint32_t queued_count;
static uint64_t clock_ms;
static uint32_t rng_value;
static uint32_t rng_calls;
static uint32_t ready_events[TRACE_MAX * 2 + 4];
static uint32_t ready_count;
static uint32_t mode;
static uint32_t reserve_after;
static uint32_t commits;
static uint32_t cancels;
static uint32_t consumed;
static uint32_t session_notifications;
static bool reservation;
static bool outbound;
static uint8_t committed[TRACE_MAX][992];
static uint16_t committed_length[TRACE_MAX];
static uint32_t committed_request[TRACE_MAX];
static uint8_t telemetry_copy[992];
gpio_dev_t GPIO;

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
 * @brief 比较两段n字节借用输入，全部相同true，不修改输入。
 */
static bool equal(const void *a, const void *b, size_t n)
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
 * @brief 核对固定四块分配大小和对齐，返回静态存储；不分配宿主堆。
 */
static void *allocate(size_t alignment, size_t n, bool dma)
{
    if (alloc_count >= 4 || n != s_bytes[alloc_count] || alignment != (dma ? 64u : 4u))
    {
        ++violations;
        return NULL;
    }
    return storage[alloc_count++];
}

/**
 * @brief 静态存储无需释放；保留free固定回调签名而不产生内存操作。
 */
static void release(void *p)
{
    (void)p;
}

/**
 * @brief 检查元数据锁深度，重入记violations；不实现真实并发锁。
 */
static void lock(void)
{
    if (lock_depth++)
    {
        ++violations;
    }
}

/**
 * @brief 核对锁退出配对，深度异常记violations。
 */
static void unlock(void)
{
    if (--lock_depth)
    {
        ++violations;
    }
}

/**
 * @brief 转发夹具lock，供生产portENTER_CRITICAL替身调用。
 */
void test_lock(void)
{
    lock();
}

/**
 * @brief 转发夹具unlock，供生产portEXIT_CRITICAL替身调用。
 */
void test_unlock(void)
{
    unlock();
}

/**
 * @brief 按夹具需要收集或忽略格式化诊断，不把日志送到业务socket。
 */
void test_log(const char *tag, const char *format, ...)
{
    (void)tag;
    (void)format;
}

/**
 * @brief 将零/非零注入错误转为固定静态字面量，返回指针无需释放。
 */
const char *esp_err_to_name(esp_err_t e)
{
    return e ? "injected" : "ESP_OK";
}

/**
 * @brief 返回本夹具模拟核号，用于任务配置断言，不是实测CPU亲和性。
 */
int xPortGetCoreID(void)
{
    return core_value;
}

/**
 * @brief 把本夹具毫秒时钟转换成微秒返回，不读取真实时钟。
 */
int64_t esp_timer_get_time(void)
{
    return (int64_t)clock_ms * 1000;
}

/**
 * @brief 递增调用计数并按模式生成可重复候选，用于重试/拒绝断言，不是硬件随机源。
 */
uint32_t esp_random(void)
{
    ++rng_calls;
    return rng_value + ((mode & 8) && rng_calls > 1 ? 1u : 0u);
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
 * @brief 返回固定128000字节夹具额度，不作真实堆证据。
 */
size_t heap_caps_get_free_size(uint32_t caps)
{
    (void)caps;
    return 128000;
}

/**
 * @brief 返回固定120000字节夹具最低额度，不作真实堆证据。
 */
size_t heap_caps_get_minimum_free_size(uint32_t caps)
{
    (void)caps;
    return 120000;
}

/**
 * @brief 初始化调用者提供的静态信号量token，按夹具失败设置返回句柄或NULL。
 */
SemaphoreHandle_t xSemaphoreCreateBinaryStatic(StaticSemaphore_t *s)
{
    s->token = 0;
    return semaphore_fail ? NULL : s;
}

/**
 * @brief 记录初始化完成token，SPI夹具还核对发布结果；只模拟通知顺序。
 */
BaseType_t xSemaphoreGive(SemaphoreHandle_t s)
{
    if (s_health.init_error != s_init_error)
    {
        ++violations;
    }
    ++s->token;
    return pdTRUE;
}

/**
 * @brief 核对预期等待参数和token后消费通知；不执行真实阻塞，返回夹具接受结果。
 */
BaseType_t xSemaphoreTake(SemaphoreHandle_t s, uint32_t ticks)
{
    if (ticks != portMAX_DELAY || s->token != 1)
    {
        ++violations;
        return 0;
    }
    --s->token;
    return pdTRUE;
}

/**
 * @brief 记录任务结束并核对空任务参数，不释放真实RTOS资源。
 */
void vTaskDelete(TaskHandle_t task)
{
    if (task)
    {
        ++violations;
    }
    ++deleted;
}

/**
 * @brief 此路径不应被事务循环使用；记录违规后跳回停驻点，不真实等待。
 */
uint32_t ulTaskNotifyTake(BaseType_t clear, uint32_t ticks)
{
    (void)clear;
    (void)ticks;
    ++violations;
    __builtin_longjmp((void **)jump_buffer, 1);
}

/**
 * @brief 核对任务栈/优先级/核号等固定参数，按注入失败或同步执行任务；跳转模拟停驻。
 */
TaskHandle_t xTaskCreateStaticPinnedToCore(TaskFunction_t task, const char *name, uint32_t bytes,
    void *arg, UBaseType_t priority, StackType_t *stack, StaticTask_t *tcb, BaseType_t core)
{
    ++task_count;
    if (bytes != 6144 || priority != 12 || core != 1 || stack != s_stack || tcb != &s_task_storage
        || !name)
    {
        ++violations;
    }
    if (create_fail)
    {
        return NULL;
    }
    if (!__builtin_setjmp((void **)jump_buffer))
    {
        task(arg);
    }
    return tcb;
}

/**
 * @brief 核对GPIO14及READY事件序列，保存电平/事件计数，不产生实际波形。
 */
void test_ready_level(unsigned pin, unsigned value)
{
    if (pin != 14 || value > 1)
    {
        ++violations;
    }
    if (value == 0 && s_pool.dma_owner != BRIDGE_DMA_DRIVER)
    {
        ++violations;
    }
    ready_level = (int)value;
    if (ready_count < TRACE_MAX * 2 + 4)
    {
        ready_events[ready_count++] = value;
    }
}

/**
 * @brief 核对SPI配置、核和描述/DMA所有权，复制总线及回调供虚拟驱动调用，返回注入错误。
 */
esp_err_t spi_slave_initialize(
    int host, const spi_bus_config_t *bus, const spi_slave_interface_config_t *slave, int dma)
{
    ++driver_count;
    if (host != SPI2_HOST || dma != SPI_DMA_CH_AUTO || core_value != 1)
    {
        ++violations;
    }
    observed_bus = *bus;
    observed_slave = *slave;
    return driver_error;
}

/**
 * @brief 只接受原描述及驱动所有权，捕获TX/RX/描述哨兵并调用post_setup；注入失败不接管。
 */
esp_err_t spi_slave_queue_trans(
    int host, const spi_slave_transaction_t *transaction, uint32_t ticks)
{
    if (host != SPI2_HOST || ticks != 0 || transaction != &s_transaction
        || s_pool.dma_owner != BRIDGE_DMA_DRIVER || ready_level != 1)
    {
        ++violations;
    }
    if ((int)queued_count == queue_error_at)
    {
        return 0x602;
    }
    if (queued_count >= TRACE_MAX)
    {
        ++violations;
        return 0x603;
    }
    memcpy(captured[queued_count++], transaction->tx_buffer, 1024);
    memcpy(saved_tx, transaction->tx_buffer, 1024);
    memcpy(saved_rx, transaction->rx_buffer, 1024);
    memcpy(saved_desc, transaction, sizeof(*transaction));
    observed_slave.post_setup_cb((spi_slave_transaction_t *)transaction);
    return ESP_OK;
}

/**
 * @brief 先核对驱动持有期哨兵未变，再按脚本超时、失败、返回陌生描述或完成原事务；输入耗尽时跳回停驻点。
 */
esp_err_t spi_slave_get_trans_result(
    int host, spi_slave_transaction_t **transaction, uint32_t ticks)
{
    if (host != SPI2_HOST || ticks != 10 || s_pool.dma_owner != BRIDGE_DMA_DRIVER
        || ready_level != 0)
    {
        ++violations;
    }
    /* 仅虚拟驱动可在持有期读取这些字节；应用等待分支不得改变描述、TX或RX。 */
    if (!equal(saved_tx, s_transaction.tx_buffer, 1024)
        || !equal(saved_rx, s_transaction.rx_buffer, 1024)
        || !equal(saved_desc, &s_transaction, sizeof(s_transaction)))
    {
        ++violations;
    }
    if ((int)completed_count == get_error_at)
    {
        return 0x604;
    }
    if ((int)completed_count == alien_at)
    {
        *transaction = (spi_slave_transaction_t *)storage[3];
        return ESP_OK;
    }
    if (waits[completed_count])
    {
        --waits[completed_count];
        clock_ms += 10;
        return ESP_ERR_TIMEOUT;
    }
    if (completed_count == input_count)
    {
        ++parked;
        __builtin_longjmp((void **)jump_buffer, 1);
    }
    memcpy(s_transaction.rx_buffer, input[completed_count], 1024);
    s_transaction.trans_len = bits[completed_count++];
    observed_slave.post_trans_cb(&s_transaction);
    *transaction = &s_transaction;
    return ESP_OK;
}

/**
 * @brief 核对已回收DMA及预留时点，取得本夹具唯一reservation；未到允许时点返回false。
 */
static bool reserve_frame(void *ctx, const scb_frame_view_t *view)
{
    (void)ctx;
    (void)view;
    if (s_pool.dma_owner != BRIDGE_DMA_APPLICATION)
    {
        ++violations;
    }
    if (completed_count < reserve_after)
    {
        return false;
    }
    if (reservation)
    {
        ++violations;
    }
    reservation = true;
    return true;
}

/**
 * @brief 必须已有预留且应用持DMA，同步复制payload和request_id，计提交并释放预留。
 */
static void commit_frame(void *ctx, const scb_frame_view_t *view)
{
    (void)ctx;
    if (!reservation || commits >= TRACE_MAX || s_pool.dma_owner != BRIDGE_DMA_APPLICATION)
    {
        ++violations;
        return;
    }
    memcpy(committed[commits], view->payload, view->header.payload_len);
    committed_length[commits] = view->header.payload_len;
    committed_request[commits] = view->header.request_id;
    ++commits;
    reservation = false;
}

/**
 * @brief 核对预留存在后释放并计cancel，不改变生产ACK规则。
 */
static void cancel_frame(void *ctx)
{
    (void)ctx;
    if (!reservation)
    {
        ++violations;
    }
    reservation = false;
    ++cancels;
}

/**
 * @brief 仅应用持DMA时按模式接受并同步复制payload，否则拒绝或记违规。
 */
static bool take_telemetry(void *ctx, const scb_frame_view_t *view)
{
    (void)ctx;
    if (s_pool.dma_owner != BRIDGE_DMA_APPLICATION)
    {
        ++violations;
    }
    if (!(mode & 2))
    {
        return false;
    }
    memcpy(telemetry_copy, view->payload, view->header.payload_len);
    return true;
}

/**
 * @brief 有待办时借出固定GET_STATUS图像，无待办返回false且不写视图。
 */
static bool peek_outbound(void *ctx, scb_frame_view_t *view)
{
    (void)ctx;
    static const uint8_t payload[] = "{\"op\":\"GET_STATUS\",\"args\":{}}";
    if (!outbound)
    {
        return false;
    }
    *view = (scb_frame_view_t)
    {
        .header =
        {
            .type = SCB_COMMAND,
            .flags = SCB_RELIABLE,
            .payload_len = sizeof(payload) - 1,
            .request_id = 77
        },
        .payload = payload
    };
    return true;
}

/**
 * @brief 核对编码成功，清出站待办并累计消费次数。
 */
static void consumed_frame(void *ctx, bool encoded)
{
    (void)ctx;
    if (!encoded)
    {
        ++violations;
    }
    outbound = false;
    ++consumed;
}

/**
 * @brief 忽略通知字段，只累计回调次数，不模拟对端或socket。
 */
static void session_notice(void *ctx, uint32_t id, bool ready)
{
    (void)ctx;
    (void)id;
    (void)ready;
    ++session_notifications;
}

/**
 * @brief 隔离用例并重置注入状态，虚拟停驻DMA只在夹具清理中回收；零成功，否则失败行号。
 */
static int setup(int scenario)
{
    /* 仅夹具清理将停驻虚拟DMA标记可归还，以隔离用例；生产无超时取消路径。 */
    if (s_pool.initialized)
    {
        if (s_pool.dma_owner == BRIDGE_DMA_DRIVER)
        {
            CHECK(bridge_pool_dma_reclaim(&s_dma) == ESP_OK);
        }
        if (s_pool.dma_owner == BRIDGE_DMA_APPLICATION)
        {
            CHECK(bridge_pool_dma_release(&s_dma) == ESP_OK);
        }
        CHECK(bridge_pool_deinit() == ESP_OK);
    }
    s_started = false;
    s_init_error = 0;
    s_task = NULL;
    s_port = (bridge_spi_port_t){0};
    s_health = (bridge_spi_snapshot_t){0};
    s_transaction = (spi_slave_transaction_t){0};
    s_dma = (bridge_dma_pair_t){0};
    alloc_count = driver_count = task_count = violations = parked = deleted = lock_depth = 0;
    core_value = 1;
    create_fail = semaphore_fail = driver_error = 0;
    ready_level = 1;
    queued_count = completed_count = input_count = 0;
    clock_ms = 0;
    rng_value = 0x12345678;
    rng_calls = 0;
    mode = reserve_after = commits = cancels = consumed = session_notifications = ready_count = 0;
    reservation = outbound = false;
    memset(waits, 0, sizeof(waits));
    queue_error_at = get_error_at = alien_at = -1;
    const bridge_pool_ops_t ops = {allocate, release, lock, unlock};
    if (scenario != 4)
    {
        CHECK(bridge_pool_init(&ops) == ESP_OK);
    }
    if (scenario == 1)
    {
        create_fail = 1;
    }
    if (scenario == 2)
    {
        semaphore_fail = 1;
    }
    if (scenario == 3)
    {
        core_value = 0;
    }
    if (scenario == 6)
    {
        driver_error = 0x601;
    }
    return 0;
}

/**
 * @brief 按scenario核对真实SPI任务初始化参数、错误回滚及单次启动；零通过，否则失败行号。
 */
int bridge_spi_driver_test(int scenario)
{
    int error = setup(scenario);
    if (error)
    {
        return error;
    }
    if (scenario == 5)
    {
        CHECK(bridge_pool_dma_acquire(&s_dma) == ESP_OK);
    }
    const esp_err_t result = bridge_spi_start(NULL);
    if (scenario == 1 || scenario == 2)
    {
        CHECK(result == ESP_ERR_NO_MEM && !driver_count && !parked);
    }
    else if (scenario >= 3 && scenario <= 5)
    {
        CHECK(result == ESP_ERR_INVALID_STATE && !driver_count && deleted == 1 && !parked);
    }
    else if (scenario == 6)
    {
        CHECK(result == 0x601 && driver_count == 1 && deleted == 1
            && s_pool.dma_owner == BRIDGE_DMA_IDLE);
    }
    else
    {
        CHECK(result == ESP_OK && driver_count == 1 && task_count == 1 && parked == 1 && !deleted
            && queued_count == 1);
        CHECK(observed_bus.mosi_io_num == 11 && observed_bus.miso_io_num == 13
            && observed_bus.sclk_io_num == 12);
        CHECK(observed_bus.quadwp_io_num == -1 && observed_bus.quadhd_io_num == -1
            && observed_bus.data4_io_num == -1 && observed_bus.data5_io_num == -1
            && observed_bus.data6_io_num == -1 && observed_bus.data7_io_num == -1);
        CHECK(observed_bus.max_transfer_sz == 1024 && observed_bus.intr_flags == ESP_INTR_FLAG_IRAM
            && observed_bus.isr_cpu_id == ESP_INTR_CPU_AFFINITY_1);
        CHECK(observed_slave.spics_io_num == 10 && observed_slave.queue_size == 1
            && !observed_slave.mode && !observed_slave.flags);
        CHECK(observed_slave.post_setup_cb && observed_slave.post_trans_cb);
        CHECK(s_transaction.length == 8192 && !s_transaction.flags
            && s_transaction.tx_buffer == s_dma.tx && s_transaction.rx_buffer == s_dma.rx);
        CHECK(s_pool.dma_owner == BRIDGE_DMA_DRIVER && ready_level == 0);
        if (scenario == 7)
        {
            CHECK(bridge_spi_start(NULL) == ESP_ERR_INVALID_STATE && driver_count == 1);
        }
    }
    CHECK(violations == 0 && lock_depth == 0);
    return 0;
}

/**
 * @brief 复制输入帧/bit长度/超时脚本后运行生产任务，核对所有权与配置；零通过，否则失败行号。
 */
int bridge_spi_run_trace(unsigned flags, const uint8_t *frames, unsigned count,
    const uint32_t *lengths, const uint32_t *timeouts, unsigned reserve_from, uint32_t random_value,
    int failure)
{
    CHECK(count < TRACE_MAX);
    int error = setup(0);
    if (error)
    {
        return error;
    }
    mode = flags;
    outbound = (flags & 4) != 0;
    reserve_after = reserve_from;
    rng_value = random_value;
    input_count = count;
    for (unsigned i = 0; i < count; ++i)
    {
        memcpy(input[i], frames + i * 1024, 1024);
        bits[i] = lengths[i];
    }
    for (unsigned i = 0; i <= count; ++i)
    {
        waits[i] = timeouts[i];
    }
    if (flags & 1)
    {
        const bridge_spi_port_t port =
        {
            .reserve = reserve_frame,
            .commit = commit_frame,
            .cancel = cancel_frame,
            .telemetry = take_telemetry,
            .peek_tx = (flags & 4) ? peek_outbound : NULL,
            .tx_consumed = (flags & 4) ? consumed_frame : NULL,
            .session_changed = session_notice
        };
        CHECK(bridge_spi_configure(&port) == ESP_OK);
    }
    if (failure == 1)
    {
        queue_error_at = 0;
    }
    if (failure == 2)
    {
        get_error_at = 0;
    }
    if (failure == 3)
    {
        alien_at = 0;
    }
    if (failure == 4)
    {
        queue_error_at = 1;
    }
    if (failure == 5)
    {
        get_error_at = 2;
    }
    CHECK(bridge_spi_start(NULL) == ESP_OK);
    CHECK(violations == 0 && lock_depth == 0);
    CHECK(bridge_spi_configure(NULL) == ESP_ERR_INVALID_STATE);
    return 0;
}

/**
 * @brief 返回生产SPI快照类型大小供Python布局核对。
 */
size_t bridge_spi_trace_size(void)
{
    return sizeof(bridge_spi_snapshot_t);
}

/**
 * @brief 按既有id返回提交/取消/会话/READY/DMA等夹具计数，未知id返回违规数。
 */
unsigned bridge_spi_trace_metric(unsigned id)
{
    switch (id)
    {
        case 0:
            return commits;
        case 1:
            return cancels;
        case 2:
            return consumed;
        case 3:
            return session_notifications;
        case 4:
            return (unsigned)ready_level;
        case 5:
            return (unsigned)s_pool.dma_owner;
        case 6:
            return rng_calls;
        case 7:
            return s_protocol.link.ack_seq;
        case 8:
            return s_protocol.link.pending;
        case 9:
            return queued_count;
        case 10:
            return ready_count;
        default:
            return violations;
    }
}

/**
 * @brief 按零基事件索引返回READY值，越界返回99。
 */
unsigned bridge_spi_trace_ready(unsigned index)
{
    return index < ready_count ? ready_events[index] : 99;
}

/**
 * @brief 索引有效且容量至少1024才复制排队TX并返回1024；失败返回零且不写。
 */
size_t bridge_spi_trace_capture(unsigned index, uint8_t *out, size_t capacity)
{
    if (index >= queued_count || capacity < 1024)
    {
        return 0;
    }
    memcpy(out, captured[index], 1024);
    return 1024;
}

/**
 * @brief 索引/容量有效才复制已接管payload并写request，返回字节数；失败零且保留输出。
 */
size_t bridge_spi_trace_commit(unsigned index, uint8_t *out, size_t capacity, uint32_t *request)
{
    if (index >= commits || capacity < committed_length[index])
    {
        return 0;
    }
    memcpy(out, committed[index], committed_length[index]);
    *request = committed_request[index];
    return committed_length[index];
}

/**
 * @brief 核对半绑定端口、清表和空快照的生产拒绝路径；零通过，否则失败行号。
 */
int bridge_spi_port_check_test(void)
{
    int e = setup(0);
    if (e)
    {
        return e;
    }
    const bridge_spi_port_t bad = {.reserve = reserve_frame};
    CHECK(bridge_spi_configure(&bad) == ESP_ERR_INVALID_ARG);
    CHECK(!s_started);
    CHECK(bridge_spi_configure(NULL) == ESP_OK);
    CHECK(bridge_spi_snapshot(NULL) == ESP_ERR_INVALID_ARG);
    return 0;
}
