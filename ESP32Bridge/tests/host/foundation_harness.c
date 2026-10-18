#include "test_sdk.h"
#include "startup.h"
#include "spi_bridge.h"
#include <stdarg.h>
/* Compile the exact production pool here to inject the UINT64_MAX edge case
 * into its private counter without adding test hooks to firmware. */
#include "buffer_pool.c"

#define CHECK(condition) do { if (!(condition)) return __LINE__; } while (0)
#define INJECTED_ERROR 0x601
static _Alignas(64) unsigned char g_storage[4][BRIDGE_EVENT_SLOTS * BRIDGE_EVENT_BYTES + 128];
static void *g_returned[4];
static bool g_active[4];
static size_t g_sizes[4];
static int g_allocs, g_frees, g_fail_at, g_misalign_at, g_alias_at, g_bad_internal, g_bad_dma;
static int g_lock_depth, g_lock_calls, g_fixture_errors, g_nvs_error, g_netif_error, g_event_error;
static int g_latch_error, g_pad_error, g_latch_calls, g_pad_calls;
static int g_order[32], g_order_count;
static char g_log[8192];
static size_t g_log_used;

void *memset(void *destination, int value, size_t count)
{
    unsigned char *out = destination;
    for (size_t i = 0; i < count; ++i) out[i] = (unsigned char)value;
    return destination;
}
void *memcpy(void *destination, const void *source, size_t count)
{
    unsigned char *out = destination;
    const unsigned char *in = source;
    for (size_t i = 0; i < count; ++i) out[i] = in[i];
    return destination;
}
static void record(int code)
{
    if (g_order_count < 32) g_order[g_order_count++] = code;
    else ++g_fixture_errors;
}
/* This fixture tests startup dependency wiring only. The real SPI adapter is
 * separately compiled/executed by test_spi_driver.py with driver/task mocks. */
esp_err_t bridge_spi_start(void *context)
{
    (void)context;
    record(23);
    return ESP_OK;
}
void test_lock(void)
{
    if (g_lock_depth != 0) ++g_fixture_errors;
    ++g_lock_depth;
    ++g_lock_calls;
}
void test_unlock(void)
{
    if (g_lock_depth != 1) ++g_fixture_errors;
    --g_lock_depth;
}
void *heap_caps_aligned_alloc(size_t alignment, size_t bytes, uint32_t caps)
{
    const int i = g_allocs++;
    if (g_lock_depth != 0 || i >= 4) { ++g_fixture_errors; return NULL; }
    record(10 + i);
    const uint32_t expected = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT | (i < 2 ? MALLOC_CAP_DMA : 0);
    if (caps != expected || alignment != (i < 2 ? 64U : 4U) || bytes != s_bytes[i]) ++g_fixture_errors;
    if (g_fail_at == i + 1) return NULL;
    if (g_alias_at == i + 1 && i != 0) return g_returned[0];
    g_returned[i] = g_storage[i] + (g_misalign_at == i + 1 ? 1 : 0);
    g_sizes[i] = bytes;
    g_active[i] = true;
    return g_returned[i];
}
void heap_caps_free(void *ptr)
{
    if (g_lock_depth != 0) ++g_fixture_errors;
    for (int i = 0; i < 4; ++i) {
        if (g_returned[i] == ptr && g_active[i]) { g_active[i] = false; ++g_frees; return; }
    }
    ++g_fixture_errors;
}
static void *mock_alloc(size_t alignment, size_t bytes, bool dma)
{
    return heap_caps_aligned_alloc(alignment, bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT | (dma ? MALLOC_CAP_DMA : 0));
}
static const bridge_pool_ops_t g_memory_ops = { mock_alloc, heap_caps_free, test_lock, test_unlock };
size_t heap_caps_get_free_size(uint32_t caps)
{
    if (caps != (MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)) ++g_fixture_errors;
    size_t result = 200000;
    for (int i = 0; i < 4; ++i) if (g_active[i]) result -= g_sizes[i];
    return result;
}
size_t heap_caps_get_minimum_free_size(uint32_t caps) { return heap_caps_get_free_size(caps); }
bool esp_ptr_internal(const void *ptr)
{
    if (g_bad_internal == 0) return true;
    const int i = g_bad_internal - 1;
    /* Reject the endpoint only, proving the adapter checks the entire range. */
    return ptr != (unsigned char *)g_returned[i] + g_sizes[i] - 1;
}
bool esp_ptr_dma_capable(const void *ptr)
{
    if (g_bad_dma == 0) return true;
    const int i = g_bad_dma - 1;
    return ptr != (unsigned char *)g_returned[i] + g_sizes[i] - 1;
}
esp_err_t gpio_set_level(int gpio, unsigned level)
{
    record(1); ++g_latch_calls;
    if (gpio != GPIO_NUM_14 || level != 1) ++g_fixture_errors;
    return g_latch_error;
}
esp_err_t gpio_config(const gpio_config_t *config)
{
    record(2); ++g_pad_calls;
    if (config->pin_bit_mask != (UINT64_C(1) << GPIO_NUM_14) || config->mode != GPIO_MODE_OUTPUT ||
        config->pull_up_en != GPIO_PULLUP_DISABLE || config->pull_down_en != GPIO_PULLDOWN_DISABLE ||
        config->intr_type != GPIO_INTR_DISABLE) ++g_fixture_errors;
    return g_pad_error;
}
esp_err_t nvs_flash_init(void) { record(20); return g_nvs_error; }
esp_err_t esp_netif_init(void) { record(21); return g_netif_error; }
esp_err_t esp_event_loop_create_default(void) { record(22); return g_event_error; }
const char *esp_err_to_name(esp_err_t error) { return error == ESP_OK ? "ESP_OK" : "ESP_ERROR"; }
static void append_char(char value)
{
    if (g_log_used + 1 < sizeof(g_log)) { g_log[g_log_used++] = value; g_log[g_log_used] = 0; }
}
static void append_text(const char *value) { while (*value) append_char(*value++); }
static void append_uint(unsigned value)
{
    char digits[10]; int count = 0;
    do { digits[count++] = (char)('0' + value % 10); value /= 10; } while (value);
    while (count) append_char(digits[--count]);
}
void test_log(const char *tag, const char *format, ...)
{
    append_text(tag); append_char(':');
    va_list args; va_start(args, format);
    while (*format) {
        if (*format++ != '%') { append_char(format[-1]); continue; }
        const char conversion = *format++;
        if (conversion == 's') append_text(va_arg(args, const char *));
        else if (conversion == 'u') append_uint(va_arg(args, unsigned));
        else if (conversion == 'd') {
            int value = va_arg(args, int);
            if (value < 0) { append_char('-'); append_uint((unsigned)(-(value + 1)) + 1); }
            else append_uint((unsigned)value);
        } else ++g_fixture_errors;
    }
    va_end(args); append_char('\n');
}
const char *bridge_test_log(void) { return g_log; }
static int reset_fixture(void)
{
    CHECK(bridge_pool_deinit() == ESP_OK);
    for (int i = 0; i < 4; ++i) CHECK(!g_active[i]);
    CHECK(g_lock_depth == 0 && g_fixture_errors == 0);
    memset(g_returned, 0, sizeof(g_returned)); memset(g_sizes, 0, sizeof(g_sizes));
    memset(g_storage, 0xA5, sizeof(g_storage));
    g_allocs = g_frees = g_fail_at = g_misalign_at = g_alias_at = g_bad_internal = g_bad_dma = 0;
    g_lock_calls = g_nvs_error = g_netif_error = g_event_error = 0;
    g_latch_error = g_pad_error = g_latch_calls = g_pad_calls = g_order_count = 0;
    g_log_used = 0; g_log[0] = 0;
    return 0;
}

static int test_pool(int scenario)
{
    bridge_pool_snapshot_t snapshot;
    if (scenario >= 1 && scenario <= 4) {
        g_fail_at = scenario;
        CHECK(bridge_pool_init(&g_memory_ops) == ESP_ERR_NO_MEM);
        CHECK(g_allocs == scenario && g_frees == scenario - 1);
        CHECK(bridge_pool_snapshot(&snapshot) == ESP_ERR_INVALID_STATE);
        CHECK(reset_fixture() == 0);
    }
    if (scenario == 5) {
        for (int i = 1; i <= 4; ++i) {
            g_misalign_at = i;
            CHECK(bridge_pool_init(&g_memory_ops) == ESP_ERR_INVALID_STATE);
            CHECK(g_allocs == i && g_frees == i);
            CHECK(reset_fixture() == 0);
        }
    }
    if (scenario == 6) {
        g_alias_at = 2;
        CHECK(bridge_pool_init(&g_memory_ops) == ESP_ERR_INVALID_STATE);
        CHECK(g_allocs == 2 && g_frees == 1);
        CHECK(reset_fixture() == 0);
    }
    CHECK(bridge_pool_init(&g_memory_ops) == ESP_OK);
    CHECK(g_allocs == 4);
    CHECK(bridge_pool_init(&g_memory_ops) == ESP_ERR_INVALID_STATE);
    CHECK(g_allocs == 4);
    CHECK(bridge_pool_snapshot(&snapshot) == ESP_OK);
    CHECK(snapshot.allocated_bytes == 51072 && snapshot.metadata_bytes > 0);
    CHECK(snapshot.capacity[0] == 16 && snapshot.capacity[1] == 32 && snapshot.dma_owner == BRIDGE_DMA_IDLE);
    if (scenario == 0) {
        bridge_dma_pair_t dma;
        bridge_pool_slot_t slot;
        CHECK(bridge_pool_dma_acquire(&dma) == ESP_OK);
        CHECK(dma.tx[0] == 0 && dma.tx[1023] == 0 && dma.rx[0] == 0 && dma.rx[1023] == 0);
        CHECK(bridge_pool_dma_release(&dma) == ESP_OK);
        CHECK(bridge_pool_claim(BRIDGE_POOL_EVENT, &slot) == ESP_OK);
        CHECK(slot.data[0] == 0 && slot.data[1023] == 0);
        CHECK(bridge_pool_release(&slot) == ESP_OK);
    }
    if (scenario == 7) {
        bridge_pool_slot_t slots[48], extra;
        for (int i = 0; i < 48; ++i) {
            CHECK(bridge_pool_claim(i < 16 ? BRIDGE_POOL_UDP : BRIDGE_POOL_EVENT, &slots[i]) == ESP_OK);
            CHECK(slots[i].capacity == (i < 16 ? 1016U : 1024U));
            slots[i].data[slots[i].capacity - 1] = (unsigned char)i;
        }
        CHECK(bridge_pool_claim(BRIDGE_POOL_UDP, &extra) == ESP_ERR_NO_MEM);
        CHECK(bridge_pool_claim(BRIDGE_POOL_EVENT, &extra) == ESP_ERR_NO_MEM);
        CHECK(bridge_pool_deinit() == ESP_ERR_INVALID_STATE);
        CHECK(bridge_pool_snapshot(&snapshot) == ESP_OK);
        CHECK(snapshot.used[0] == 16 && snapshot.used[1] == 32 && snapshot.peak[0] == 16 && snapshot.peak[1] == 32);
        for (int i = 0; i < 48; ++i) {
            CHECK(slots[i].data[slots[i].capacity - 1] == i);
            CHECK(bridge_pool_release(&slots[i]) == ESP_OK);
            CHECK(bridge_pool_release(&slots[i]) == ESP_ERR_INVALID_STATE);
        }
        CHECK(bridge_pool_claim(BRIDGE_POOL_UDP, &extra) == ESP_OK);
        CHECK(bridge_pool_release(&slots[0]) == ESP_ERR_INVALID_STATE);
        bridge_pool_slot_t forged = extra; ++forged.data;
        CHECK(bridge_pool_release(&forged) == ESP_ERR_INVALID_STATE);
        forged = extra; ++forged.capacity;
        CHECK(bridge_pool_release(&forged) == ESP_ERR_INVALID_STATE);
        forged = extra; ++forged.generation;
        CHECK(bridge_pool_release(&forged) == ESP_ERR_INVALID_STATE);
        CHECK(bridge_pool_release(&extra) == ESP_OK);
    }
    if (scenario == 8 || scenario == 11) {
        for (int i = 0; i < (scenario == 11 ? 2000 : 1); ++i) {
            bridge_dma_pair_t dma, other;
            CHECK(bridge_pool_dma_acquire(&dma) == ESP_OK);
            CHECK((uintptr_t)dma.tx % 64 == 0 && (uintptr_t)dma.rx % 64 == 0 && dma.tx != dma.rx && dma.capacity == 1024);
            CHECK(bridge_pool_dma_acquire(&other) == ESP_ERR_INVALID_STATE);
            CHECK(bridge_pool_deinit() == ESP_ERR_INVALID_STATE);
            CHECK(bridge_pool_dma_reclaim(&dma) == ESP_ERR_INVALID_STATE);
            bridge_dma_pair_t forged = dma; ++forged.tx;
            CHECK(bridge_pool_dma_submit(&forged) == ESP_ERR_INVALID_STATE);
            dma.tx[1023] = 0x71; dma.rx[1023] = 0x52;
            CHECK(bridge_pool_dma_submit(&dma) == ESP_OK);
            CHECK(bridge_pool_dma_submit(&dma) == ESP_ERR_INVALID_STATE);
            CHECK(bridge_pool_dma_release(&dma) == ESP_ERR_INVALID_STATE);
            CHECK(bridge_pool_deinit() == ESP_ERR_INVALID_STATE);
            CHECK(bridge_pool_snapshot(&snapshot) == ESP_OK && snapshot.dma_owner == BRIDGE_DMA_DRIVER);
            CHECK(bridge_pool_dma_reclaim(&dma) == ESP_OK);
            CHECK(dma.tx[1023] == 0x71 && dma.rx[1023] == 0x52);
            CHECK(bridge_pool_dma_release(&dma) == ESP_OK);
            CHECK(bridge_pool_dma_release(&dma) == ESP_ERR_INVALID_STATE);
            if (scenario == 11) {
                bridge_pool_slot_t udp, event;
                CHECK(bridge_pool_claim(BRIDGE_POOL_UDP, &udp) == ESP_OK);
                CHECK(bridge_pool_claim(BRIDGE_POOL_EVENT, &event) == ESP_OK);
                CHECK(bridge_pool_release(&udp) == ESP_OK && bridge_pool_release(&event) == ESP_OK);
            }
        }
        CHECK(g_allocs == 4 && g_frees == 0);
    }
    if (scenario == 9) {
        bridge_pool_slot_t old, current;
        bridge_dma_pair_t old_dma, current_dma;
        CHECK(bridge_pool_claim(BRIDGE_POOL_UDP, &old) == ESP_OK);
        CHECK(bridge_pool_release(&old) == ESP_OK);
        CHECK(bridge_pool_dma_acquire(&old_dma) == ESP_OK);
        CHECK(bridge_pool_dma_release(&old_dma) == ESP_OK);
        CHECK(reset_fixture() == 0);
        CHECK(bridge_pool_init(&g_memory_ops) == ESP_OK);
        CHECK(bridge_pool_claim(BRIDGE_POOL_UDP, &current) == ESP_OK);
        CHECK(bridge_pool_dma_acquire(&current_dma) == ESP_OK);
        CHECK(old.generation != current.generation && old_dma.generation != current_dma.generation);
        CHECK(bridge_pool_release(&old) == ESP_ERR_INVALID_STATE);
        CHECK(bridge_pool_dma_submit(&old_dma) == ESP_ERR_INVALID_STATE);
        CHECK(bridge_pool_release(&current) == ESP_OK && bridge_pool_dma_release(&current_dma) == ESP_OK);
    }
    if (scenario == 10) {
        bridge_pool_slot_t slot = { .kind = BRIDGE_POOL_EVENT, .index = 32 };
        CHECK(bridge_pool_init(NULL) == ESP_ERR_INVALID_ARG);
        CHECK(bridge_pool_claim((bridge_pool_kind_t)-1, &slot) == ESP_ERR_INVALID_ARG);
        CHECK(bridge_pool_claim(BRIDGE_POOL_UDP, NULL) == ESP_ERR_INVALID_ARG);
        CHECK(bridge_pool_release(&slot) == ESP_ERR_INVALID_ARG && bridge_pool_release(NULL) == ESP_ERR_INVALID_ARG);
        CHECK(bridge_pool_snapshot(NULL) == ESP_ERR_INVALID_ARG);
        CHECK(bridge_pool_dma_acquire(NULL) == ESP_ERR_INVALID_ARG);
        CHECK(bridge_pool_dma_submit(NULL) == ESP_ERR_INVALID_ARG);
        CHECK(bridge_pool_dma_reclaim(NULL) == ESP_ERR_INVALID_ARG);
        CHECK(bridge_pool_dma_release(NULL) == ESP_ERR_INVALID_ARG);
    }
    CHECK(bridge_pool_deinit() == ESP_OK && g_frees == 4);
    CHECK(bridge_pool_deinit() == ESP_OK && g_frees == 4);
    CHECK(g_lock_depth == 0 && g_fixture_errors == 0 && g_lock_calls > 0);
    return 0;
}

typedef struct { int errors[6], calls[6], count; } steps_fixture_t;
static esp_err_t call_step(void *context, int index)
{
    steps_fixture_t *fixture = context;
    fixture->calls[fixture->count++] = index;
    return fixture->errors[index];
}
static esp_err_t step_nvs(void *p) { return call_step(p, 0); }
static esp_err_t step_netif(void *p) { return call_step(p, 1); }
static esp_err_t step_event(void *p) { return call_step(p, 2); }
static esp_err_t step_ap(void *p) { return call_step(p, 3); }
static esp_err_t step_sockets(void *p) { return call_step(p, 4); }
static esp_err_t step_spi(void *p) { return call_step(p, 5); }
static int test_startup(int scenario)
{
    steps_fixture_t fixture = {0};
    bridge_startup_ops_t ops = {
        .context = &fixture, .nvs = step_nvs, .netif = step_netif, .event_loop = step_event,
        .ap = step_ap, .sockets = step_sockets, .spi = step_spi
    };
    bridge_startup_report_t report;
    if (scenario >= 21 && scenario <= 26) fixture.errors[scenario - 21] = INJECTED_ERROR;
    if (scenario == 27 || scenario == 34) ops.ap = NULL;
    if (scenario == 28) ops.nvs = NULL;
    if (scenario == 29) ops.password_error = INJECTED_ERROR;
    if (scenario == 30) ops.ready_error = INJECTED_ERROR;
    if (scenario == 31) ops.pool_error = INJECTED_ERROR;
    if (scenario == 33) { fixture.errors[0] = INJECTED_ERROR; fixture.errors[5] = INJECTED_ERROR + 1; }
    if (scenario == 34) fixture.errors[5] = INJECTED_ERROR;
    if (scenario == 35) { ops.ready_error = INJECTED_ERROR; ops.pool_error = INJECTED_ERROR + 1; }
    if (scenario == 32) {
        CHECK(bridge_startup_run(NULL, &report) == ESP_ERR_INVALID_ARG);
        CHECK(bridge_startup_run(&ops, NULL) == ESP_ERR_INVALID_ARG);
        CHECK(fixture.count == 0); return 0;
    }
    const esp_err_t result = bridge_startup_run(&ops, &report);
    if (scenario == 20) {
        CHECK(result == ESP_OK && fixture.count == 6);
        for (int i = 0; i < 6; ++i) CHECK(fixture.calls[i] == i && report.steps[i].state == BRIDGE_SUCCEEDED);
    } else if (scenario >= 21 && scenario <= 26) {
        const int failed = scenario - 21;
        CHECK(result == INJECTED_ERROR && report.steps[failed].state == BRIDGE_FAILED);
        CHECK(report.steps[failed].error == INJECTED_ERROR);
        if (failed < 5) {
            CHECK(fixture.count == failed + 2 && fixture.calls[fixture.count - 1] == 5);
            CHECK(report.steps[5].state == BRIDGE_SUCCEEDED);
            for (int i = failed + 1; i < 5; ++i) {
                CHECK(report.steps[i].state == BRIDGE_BLOCKED && report.steps[i].dependency == i - 1);
            }
        }
    } else if (scenario == 27 || scenario == 28 || scenario == 34) {
        int absent = scenario == 28 ? 0 : 3;
        CHECK(report.steps[absent].state == BRIDGE_UNIMPLEMENTED && report.steps[absent + 1].state == BRIDGE_BLOCKED);
        CHECK(fixture.calls[fixture.count - 1] == 5);
        CHECK(result == (scenario == 34 ? INJECTED_ERROR : ESP_ERR_NOT_SUPPORTED));
    } else if (scenario == 29) {
        CHECK(result == INJECTED_ERROR && fixture.count == 4 && fixture.calls[3] == 5);
        CHECK(report.steps[3].state == BRIDGE_BLOCKED && report.steps[3].dependency == BRIDGE_DEP_PASSWORD);
    } else if (scenario == 30) {
        CHECK(result == INJECTED_ERROR && fixture.count == 5);
        CHECK(report.steps[5].state == BRIDGE_BLOCKED && report.steps[5].dependency == BRIDGE_DEP_READY);
        CHECK(report.steps[4].state == BRIDGE_SUCCEEDED);
    } else if (scenario == 31) {
        CHECK(result == INJECTED_ERROR && fixture.count == 0);
        for (int i = 0; i < 6; ++i) CHECK(report.steps[i].state == BRIDGE_BLOCKED && report.steps[i].dependency == BRIDGE_DEP_POOL);
    } else if (scenario == 33) {
        CHECK(result == INJECTED_ERROR && fixture.count == 2 && fixture.calls[1] == 5);
        CHECK(report.steps[0].state == BRIDGE_FAILED && report.steps[5].state == BRIDGE_FAILED);
    } else if (scenario == 35) {
        CHECK(result == INJECTED_ERROR && report.pool_error == INJECTED_ERROR + 1 && fixture.count == 0);
        for (int i = 0; i < 6; ++i) CHECK(report.steps[i].state == BRIDGE_BLOCKED && report.steps[i].dependency == BRIDGE_DEP_POOL);
    }
    return 0;
}

void app_main(void);
static int test_adapter(int scenario)
{
    bridge_startup_report_t report;
    if (scenario == 41 || scenario == 51) g_nvs_error = INJECTED_ERROR;
    if (scenario == 42) g_netif_error = INJECTED_ERROR;
    if (scenario == 43) g_event_error = INJECTED_ERROR;
    if (scenario == 44) g_latch_error = INJECTED_ERROR;
    if (scenario == 45) g_pad_error = INJECTED_ERROR;
    if (scenario == 46) g_bad_internal = 1;
    if (scenario == 47) g_bad_dma = 2;
    if (scenario == 48) g_bad_internal = 3;
    if (scenario == 49) g_fail_at = 4;
    if (scenario == 50 || scenario == 51) {
        app_main();
        CHECK(g_log_used > 0 && g_latch_calls == 1 && g_pad_calls == 1);
    } else {
        const esp_err_t result = bridge_startup_idf_run(&report);
        CHECK(g_order[0] == 1 && (scenario == 44 || g_order[1] == 2));
        if (scenario == 40) {
            CHECK(result == ESP_ERR_NOT_SUPPORTED && report.steps[0].state == BRIDGE_SUCCEEDED);
            CHECK(report.steps[1].state == BRIDGE_SUCCEEDED && report.steps[2].state == BRIDGE_SUCCEEDED);
            CHECK(report.steps[3].state == BRIDGE_UNIMPLEMENTED && report.steps[4].state == BRIDGE_BLOCKED);
            CHECK(report.steps[5].state == BRIDGE_SUCCEEDED && g_order_count == 10 && g_order[9] == 23);
            for (int i = 0; i < 4; ++i) CHECK(g_order[i + 2] == 10 + i);
            CHECK(g_order[6] == 20 && g_order[7] == 21 && g_order[8] == 22);
        } else if (scenario >= 41 && scenario <= 43) {
            const int failed = scenario - 41;
            CHECK(result == INJECTED_ERROR && report.steps[failed].state == BRIDGE_FAILED);
            CHECK(report.steps[5].state == BRIDGE_SUCCEEDED && g_order_count == 8 + failed);
        } else if (scenario == 44 || scenario == 45) {
            CHECK(result == INJECTED_ERROR && report.steps[5].state == BRIDGE_BLOCKED);
            CHECK(report.steps[5].dependency == BRIDGE_DEP_READY && report.steps[2].state == BRIDGE_SUCCEEDED);
            CHECK(g_pad_calls == (scenario == 44 ? 0 : 1));
        } else if (scenario >= 46 && scenario <= 49) {
            CHECK(result == ESP_ERR_NO_MEM && g_frees == g_allocs - (scenario == 49 ? 1 : 0));
            for (int i = 0; i < 6; ++i) CHECK(report.steps[i].state == BRIDGE_BLOCKED && report.steps[i].dependency == BRIDGE_DEP_POOL);
        } else if (scenario == 60) {
            CHECK(report.password_error == ESP_ERR_INVALID_ARG && result == ESP_ERR_INVALID_ARG);
            CHECK(report.steps[3].state == BRIDGE_BLOCKED && report.steps[3].dependency == BRIDGE_DEP_PASSWORD);
            CHECK(report.steps[5].state == BRIDGE_SUCCEEDED && report.steps[2].state == BRIDGE_SUCCEEDED);
        }
    }
    CHECK(g_fixture_errors == 0 && g_lock_depth == 0);
    CHECK(bridge_pool_deinit() == ESP_OK);
    return 0;
}

static int test_password(void)
{
    unsigned char input[64];
    memset(input, 0x41, sizeof(input));
    CHECK(bridge_password_validate(NULL, 8) == ESP_ERR_INVALID_ARG);
    CHECK(bridge_password_validate(input, 0) == ESP_ERR_INVALID_ARG);
    CHECK(bridge_password_validate(input, 7) == ESP_ERR_INVALID_ARG);
    CHECK(bridge_password_validate(input, 8) == ESP_OK);
    CHECK(bridge_password_validate(input, 63) == ESP_OK);
    CHECK(bridge_password_validate(input, 64) == ESP_ERR_INVALID_ARG);
    for (int value = 0; value < 256; ++value) {
        input[4] = (unsigned char)value;
        CHECK(bridge_password_validate(input, 8) == (value != 0 && value <= 127 ? ESP_OK : ESP_ERR_INVALID_ARG));
    }
    return 0;
}
static int test_exhaustion(void)
{
    CHECK(bridge_pool_init(&g_memory_ops) == ESP_OK);
    s_next_generation = UINT64_MAX;
    bridge_pool_slot_t slot, extra;
    CHECK(bridge_pool_claim(BRIDGE_POOL_UDP, &slot) == ESP_OK && slot.generation == UINT64_MAX);
    CHECK(bridge_pool_claim(BRIDGE_POOL_UDP, &extra) == ESP_ERR_INVALID_STATE);
    bridge_dma_pair_t dma;
    CHECK(bridge_pool_dma_acquire(&dma) == ESP_ERR_INVALID_STATE);
    CHECK(bridge_pool_release(&slot) == ESP_OK);
    CHECK(bridge_pool_deinit() == ESP_OK);
    CHECK(reset_fixture() == 0);
    CHECK(bridge_pool_init(&g_memory_ops) == ESP_OK);
    CHECK(bridge_pool_claim(BRIDGE_POOL_EVENT, &extra) == ESP_ERR_INVALID_STATE);
    CHECK(bridge_pool_deinit() == ESP_OK);
    return 0;
}

int bridge_test_run(int scenario)
{
    int result = reset_fixture();
    if (result) return result;
    if (scenario <= 11) return test_pool(scenario);
    if (scenario == 12) return test_password();
    if (scenario >= 20 && scenario <= 35) return test_startup(scenario);
    if (scenario >= 40 && scenario <= 60) return test_adapter(scenario);
    if (scenario == 99) return test_exhaustion();
    return -1;
}
