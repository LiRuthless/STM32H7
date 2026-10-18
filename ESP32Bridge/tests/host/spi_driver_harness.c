/* Exact production source, SDK task/driver boundary mocked explicitly.
 * Task park uses compiler builtins to return control to the host scheduler;
 * this tests configuration/rollback, not actual RTOS scheduling or electricity. */
#include "buffer_pool.c"
#include "spi_bridge.c"
#include <stdarg.h>

#define CHECK(c) do { if (!(c)) return __LINE__; } while (0)
static _Alignas(64) uint8_t storage[4][BRIDGE_EVENT_SLOTS * BRIDGE_EVENT_BYTES];
static int alloc_count, driver_count, task_count, violations, core_value;
static int driver_error, create_fail, semaphore_fail, parked, deleted;
static spi_bus_config_t observed_bus;
static spi_slave_interface_config_t observed_slave;
static intptr_t jump_buffer[5];

void *memset(void *p, int c, size_t n) { uint8_t *b = p; for (size_t i=0;i<n;++i) b[i]=(uint8_t)c; return p; }
void *memcpy(void *p, const void *s, size_t n) { uint8_t *b=p; const uint8_t *v=s; for(size_t i=0;i<n;++i)b[i]=v[i]; return p; }
static void *allocate(size_t alignment, size_t n, bool dma)
{
    if (alloc_count >= 4 || n != s_bytes[alloc_count] || alignment != (dma ? 64u : 4u)) { ++violations; return NULL; }
    return storage[alloc_count++];
}
static void release(void *p) { (void)p; }
static void lock(void) { }
static void unlock(void) { }
void test_log(const char *tag, const char *format, ...) { (void)tag; (void)format; }
const char *esp_err_to_name(esp_err_t error) { return error ? "injected" : "ESP_OK"; }
int xPortGetCoreID(void) { return core_value; }
SemaphoreHandle_t xSemaphoreCreateBinaryStatic(StaticSemaphore_t *s) { s->token=0; return semaphore_fail ? NULL : s; }
BaseType_t xSemaphoreGive(SemaphoreHandle_t s) { ++s->token; return pdTRUE; }
BaseType_t xSemaphoreTake(SemaphoreHandle_t s, uint32_t ticks)
{
    if (ticks != portMAX_DELAY || s->token != 1) { ++violations; return 0; }
    --s->token; return pdTRUE;
}
void vTaskDelete(TaskHandle_t task) { if (task) ++violations; ++deleted; }
uint32_t ulTaskNotifyTake(BaseType_t clear, uint32_t ticks)
{
    if (clear != pdTRUE || ticks != portMAX_DELAY) ++violations;
    ++parked;
    __builtin_longjmp((void **)jump_buffer, 1);
}
TaskHandle_t xTaskCreateStaticPinnedToCore(TaskFunction_t task, const char *name, uint32_t bytes,
    void *arg, UBaseType_t priority, StackType_t *stack, StaticTask_t *tcb, BaseType_t core)
{
    ++task_count;
    if (bytes != 6144 || priority != 12 || core != 1 || stack != s_stack || tcb != &s_task_storage || !name)
        ++violations;
    if (create_fail) return NULL;
    if (!__builtin_setjmp((void **)jump_buffer)) task(arg);
    return tcb;
}
esp_err_t spi_slave_initialize(int host, const spi_bus_config_t *bus, const spi_slave_interface_config_t *slave, int dma)
{
    ++driver_count;
    if (host != SPI2_HOST || dma != SPI_DMA_CH_AUTO || core_value != 1) ++violations;
    observed_bus=*bus; observed_slave=*slave;
    return driver_error;
}

int bridge_spi_driver_test(int scenario)
{
    s_started=false; s_init_error=0; s_task=NULL;
    s_transaction=(spi_slave_transaction_t){0}; s_dma=(bridge_dma_pair_t){0};
    alloc_count=driver_count=task_count=violations=parked=deleted=0;
    core_value=1; create_fail=semaphore_fail=driver_error=0;
    const bridge_pool_ops_t ops={allocate,release,lock,unlock};
    if (scenario != 4) CHECK(bridge_pool_init(&ops)==ESP_OK);
    bridge_dma_pair_t other={0};
    if (scenario==1) create_fail=1;
    if (scenario==2) semaphore_fail=1;
    if (scenario==3) core_value=0;
    if (scenario==5) CHECK(bridge_pool_dma_acquire(&other)==ESP_OK);
    if (scenario==6) driver_error=0x601;
    const esp_err_t result=bridge_spi_start(NULL);
    if (scenario==1 || scenario==2) {
        CHECK(result==ESP_ERR_NO_MEM && driver_count==0 && parked==0);
        CHECK(task_count==(scenario==1 ? 1 : 0));
    } else if (scenario>=3 && scenario<=5) {
        CHECK(result==ESP_ERR_INVALID_STATE && driver_count==0 && deleted==1 && parked==0);
    } else if (scenario==6) {
        CHECK(result==0x601 && driver_count==1 && deleted==1 && parked==0);
        bridge_pool_snapshot_t pool;
        CHECK(bridge_pool_snapshot(&pool)==ESP_OK && pool.dma_owner==BRIDGE_DMA_IDLE);
    } else {
        CHECK(result==ESP_OK && driver_count==1 && task_count==1 && parked==1 && deleted==0);
        CHECK(observed_bus.mosi_io_num==11 && observed_bus.miso_io_num==13 && observed_bus.sclk_io_num==12);
        CHECK(observed_bus.quadwp_io_num==-1 && observed_bus.quadhd_io_num==-1 && observed_bus.data4_io_num==-1 &&
              observed_bus.data5_io_num==-1 && observed_bus.data6_io_num==-1 && observed_bus.data7_io_num==-1);
        CHECK(observed_bus.max_transfer_sz==1024 && observed_bus.intr_flags==ESP_INTR_FLAG_IRAM &&
              observed_bus.isr_cpu_id==ESP_INTR_CPU_AFFINITY_1 && observed_bus.flags==0);
        CHECK(observed_slave.spics_io_num==10 && observed_slave.queue_size==1 && observed_slave.mode==0 && observed_slave.flags==0);
        CHECK(!observed_slave.post_setup_cb && !observed_slave.post_trans_cb);
        CHECK(s_transaction.length==8192 && !s_transaction.flags && s_transaction.tx_buffer==s_dma.tx &&
              s_transaction.rx_buffer==s_dma.rx && s_dma.tx!=s_dma.rx && s_dma.capacity==1024);
        bridge_pool_snapshot_t pool;
        CHECK(bridge_pool_snapshot(&pool)==ESP_OK && pool.dma_owner==BRIDGE_DMA_APPLICATION);
        if (scenario==7) CHECK(bridge_spi_start(NULL)==ESP_ERR_INVALID_STATE && driver_count==1);
        CHECK(bridge_pool_dma_release(&s_dma)==ESP_OK);
    }
    if (scenario==5) CHECK(bridge_pool_dma_release(&other)==ESP_OK);
    CHECK(violations==0);
    CHECK(bridge_pool_deinit()==ESP_OK);
    return 0;
}
