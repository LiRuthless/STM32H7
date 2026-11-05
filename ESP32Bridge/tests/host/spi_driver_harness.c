/* Production task/pool compiled verbatim. SDK scheduling/driver are fixtures.
 * Builtin jumps model a parked task, never real RTOS/electrical evidence. */
#include "buffer_pool.c"
#include "spi_bridge.c"
#include <stdarg.h>
#define CHECK(c) do { if (!(c)) return __LINE__; } while (0)
#define TRACE_MAX 32u
static _Alignas(64) uint8_t storage[4][BRIDGE_EVENT_SLOTS * BRIDGE_EVENT_BYTES];
static int alloc_count,driver_count,task_count,violations,core_value,driver_error,create_fail,semaphore_fail,parked,deleted;
static int queue_error_at,get_error_at,alien_at,ready_level,lock_depth;
static spi_bus_config_t observed_bus;
static spi_slave_interface_config_t observed_slave;
static intptr_t jump_buffer[5];
static uint8_t input[TRACE_MAX][1024],captured[TRACE_MAX][1024],saved_tx[1024],saved_rx[1024];
static uint8_t saved_desc[sizeof(spi_slave_transaction_t)];
static uint32_t bits[TRACE_MAX],waits[TRACE_MAX+1],input_count,completed_count,queued_count;
static uint64_t clock_ms;
static uint32_t rng_value,rng_calls,ready_events[TRACE_MAX*2+4],ready_count;
static uint32_t mode,reserve_after,commits,cancels,consumed,session_notifications;
static bool reservation,outbound;
static uint8_t committed[TRACE_MAX][992];
static uint16_t committed_length[TRACE_MAX];
static uint32_t committed_request[TRACE_MAX];
static uint8_t telemetry_copy[992];
gpio_dev_t GPIO;
void *memset(void *p,int c,size_t n){uint8_t *b=p;for(size_t i=0;i<n;++i)b[i]=(uint8_t)c;return p;}
void *memcpy(void *p,const void *s,size_t n){uint8_t *b=p;const uint8_t *v=s;for(size_t i=0;i<n;++i)b[i]=v[i];return p;}
static bool equal(const void *a,const void *b,size_t n)
{const uint8_t *x=a,*y=b;for(size_t i=0;i<n;++i)if(x[i]!=y[i])return false;return true;}
static void *allocate(size_t alignment,size_t n,bool dma)
{if(alloc_count>=4||n!=s_bytes[alloc_count]||alignment!=(dma?64u:4u)){++violations;return NULL;}return storage[alloc_count++];}
static void release(void *p){(void)p;}
static void lock(void){if(lock_depth++)++violations;}
static void unlock(void){if(--lock_depth)++violations;}
void test_lock(void){lock();}
void test_unlock(void){unlock();}
void test_log(const char *tag,const char *format,...){(void)tag;(void)format;}
const char *esp_err_to_name(esp_err_t e){return e?"injected":"ESP_OK";}
int xPortGetCoreID(void){return core_value;}
int64_t esp_timer_get_time(void){return (int64_t)clock_ms*1000;}
uint32_t esp_random(void){++rng_calls;return rng_value+((mode&8)&&rng_calls>1?1u:0u);}
UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t task){(void)task;return 4096;}
size_t heap_caps_get_free_size(uint32_t caps){(void)caps;return 128000;}
size_t heap_caps_get_minimum_free_size(uint32_t caps){(void)caps;return 120000;}
SemaphoreHandle_t xSemaphoreCreateBinaryStatic(StaticSemaphore_t *s){s->token=0;return semaphore_fail?NULL:s;}
BaseType_t xSemaphoreGive(SemaphoreHandle_t s)
{if(s_health.init_error!=s_init_error)++violations;++s->token;return pdTRUE;}
BaseType_t xSemaphoreTake(SemaphoreHandle_t s,uint32_t ticks)
{if(ticks!=portMAX_DELAY||s->token!=1){++violations;return 0;}--s->token;return pdTRUE;}
void vTaskDelete(TaskHandle_t task){if(task)++violations;++deleted;}
uint32_t ulTaskNotifyTake(BaseType_t clear,uint32_t ticks)
{(void)clear;(void)ticks;++violations;__builtin_longjmp((void **)jump_buffer,1);}
TaskHandle_t xTaskCreateStaticPinnedToCore(TaskFunction_t task,const char *name,uint32_t bytes,
    void *arg,UBaseType_t priority,StackType_t *stack,StaticTask_t *tcb,BaseType_t core)
{
    ++task_count;
    if(bytes!=6144||priority!=12||core!=1||stack!=s_stack||tcb!=&s_task_storage||!name)++violations;
    if(create_fail)return NULL;
    if(!__builtin_setjmp((void **)jump_buffer))task(arg);
    return tcb;
}
void test_ready_level(unsigned pin,unsigned value)
{
    if(pin!=14||value>1)++violations;
    if(value==0&&s_pool.dma_owner!=BRIDGE_DMA_DRIVER)++violations;
    ready_level=(int)value;
    if(ready_count<TRACE_MAX*2+4)ready_events[ready_count++]=value;
}
esp_err_t spi_slave_initialize(int host,const spi_bus_config_t *bus,const spi_slave_interface_config_t *slave,int dma)
{
    ++driver_count;if(host!=SPI2_HOST||dma!=SPI_DMA_CH_AUTO||core_value!=1)++violations;
    observed_bus=*bus;observed_slave=*slave;return driver_error;
}
esp_err_t spi_slave_queue_trans(int host,const spi_slave_transaction_t *transaction,uint32_t ticks)
{
    if(host!=SPI2_HOST||ticks!=0||transaction!=&s_transaction||s_pool.dma_owner!=BRIDGE_DMA_DRIVER||ready_level!=1)++violations;
    if((int)queued_count==queue_error_at)return 0x602;
    if(queued_count>=TRACE_MAX){++violations;return 0x603;}
    memcpy(captured[queued_count++],transaction->tx_buffer,1024);
    memcpy(saved_tx,transaction->tx_buffer,1024);memcpy(saved_rx,transaction->rx_buffer,1024);
    memcpy(saved_desc,transaction,sizeof(*transaction));
    observed_slave.post_setup_cb((spi_slave_transaction_t *)transaction);return ESP_OK;
}
esp_err_t spi_slave_get_trans_result(int host,spi_slave_transaction_t **transaction,uint32_t ticks)
{
    if(host!=SPI2_HOST||ticks!=10||s_pool.dma_owner!=BRIDGE_DMA_DRIVER||ready_level!=0)++violations;
    /* Only the virtual driver reads these while owned. The application timeout
     * branch must not alter ANY byte of descriptor, TX or RX. */
    if(!equal(saved_tx,s_transaction.tx_buffer,1024)||!equal(saved_rx,s_transaction.rx_buffer,1024)||
       !equal(saved_desc,&s_transaction,sizeof(s_transaction)))++violations;
    if((int)completed_count==get_error_at)return 0x604;
    if((int)completed_count==alien_at){*transaction=(spi_slave_transaction_t *)storage[3];return ESP_OK;}
    if(waits[completed_count]){--waits[completed_count];clock_ms+=10;return ESP_ERR_TIMEOUT;}
    if(completed_count==input_count){++parked;__builtin_longjmp((void **)jump_buffer,1);}
    memcpy(s_transaction.rx_buffer,input[completed_count],1024);s_transaction.trans_len=bits[completed_count++];
    observed_slave.post_trans_cb(&s_transaction);*transaction=&s_transaction;return ESP_OK;
}
static bool reserve_frame(void *ctx,const scb_frame_view_t *view)
{
    (void)ctx;(void)view;if(s_pool.dma_owner!=BRIDGE_DMA_APPLICATION)++violations;
    if(completed_count<reserve_after)return false;if(reservation)++violations;
    reservation=true;return true;
}
static void commit_frame(void *ctx,const scb_frame_view_t *view)
{
    (void)ctx;if(!reservation||commits>=TRACE_MAX||s_pool.dma_owner!=BRIDGE_DMA_APPLICATION){++violations;return;}
    memcpy(committed[commits],view->payload,view->header.payload_len);
    committed_length[commits]=view->header.payload_len;committed_request[commits]=view->header.request_id;
    ++commits;reservation=false;
}
static void cancel_frame(void *ctx){(void)ctx;if(!reservation)++violations;reservation=false;++cancels;}
static bool take_telemetry(void *ctx,const scb_frame_view_t *view)
{(void)ctx;if(s_pool.dma_owner!=BRIDGE_DMA_APPLICATION)++violations;
 if(!(mode&2))return false;memcpy(telemetry_copy,view->payload,view->header.payload_len);return true;}
static bool peek_outbound(void *ctx,scb_frame_view_t *view)
{
    (void)ctx;static const uint8_t payload[]="{\"op\":\"GET_STATUS\",\"args\":{}}";if(!outbound)return false;
    *view=(scb_frame_view_t){.header={.type=SCB_COMMAND,.flags=SCB_RELIABLE,.payload_len=sizeof(payload)-1,.request_id=77},.payload=payload};return true;
}
static void consumed_frame(void *ctx,bool encoded){(void)ctx;if(!encoded)++violations;outbound=false;++consumed;}
static void session_notice(void *ctx,uint32_t id,bool ready){(void)ctx;(void)id;(void)ready;++session_notifications;}
static int setup(int scenario)
{
    /* Fixture-only cancellation makes a parked virtual DMA returnable for test
     * isolation. Production has no timeout cancellation path. */
    if(s_pool.initialized){
        if(s_pool.dma_owner==BRIDGE_DMA_DRIVER)CHECK(bridge_pool_dma_reclaim(&s_dma)==ESP_OK);
        if(s_pool.dma_owner==BRIDGE_DMA_APPLICATION)CHECK(bridge_pool_dma_release(&s_dma)==ESP_OK);
        CHECK(bridge_pool_deinit()==ESP_OK);
    }
    s_started=false;s_init_error=0;s_task=NULL;s_port=(bridge_spi_port_t){0};
    s_health=(bridge_spi_snapshot_t){0};s_transaction=(spi_slave_transaction_t){0};s_dma=(bridge_dma_pair_t){0};
    alloc_count=driver_count=task_count=violations=parked=deleted=lock_depth=0;
    core_value=1;create_fail=semaphore_fail=driver_error=0;ready_level=1;
    queued_count=completed_count=input_count=0;clock_ms=0;rng_value=0x12345678;rng_calls=0;
    mode=reserve_after=commits=cancels=consumed=session_notifications=ready_count=0;reservation=outbound=false;
    memset(waits,0,sizeof(waits));queue_error_at=get_error_at=alien_at=-1;
    const bridge_pool_ops_t ops={allocate,release,lock,unlock};if(scenario!=4)CHECK(bridge_pool_init(&ops)==ESP_OK);
    if(scenario==1)create_fail=1;if(scenario==2)semaphore_fail=1;if(scenario==3)core_value=0;if(scenario==6)driver_error=0x601;
    return 0;
}
int bridge_spi_driver_test(int scenario)
{
    int error=setup(scenario);if(error)return error;
    if(scenario==5)CHECK(bridge_pool_dma_acquire(&s_dma)==ESP_OK);
    const esp_err_t result=bridge_spi_start(NULL);
    if(scenario==1||scenario==2)CHECK(result==ESP_ERR_NO_MEM&&!driver_count&&!parked);
    else if(scenario>=3&&scenario<=5)CHECK(result==ESP_ERR_INVALID_STATE&&!driver_count&&deleted==1&&!parked);
    else if(scenario==6)CHECK(result==0x601&&driver_count==1&&deleted==1&&s_pool.dma_owner==BRIDGE_DMA_IDLE);
    else{
        CHECK(result==ESP_OK&&driver_count==1&&task_count==1&&parked==1&&!deleted&&queued_count==1);
        CHECK(observed_bus.mosi_io_num==11&&observed_bus.miso_io_num==13&&observed_bus.sclk_io_num==12);
        CHECK(observed_bus.quadwp_io_num==-1&&observed_bus.quadhd_io_num==-1&&observed_bus.data4_io_num==-1&&
              observed_bus.data5_io_num==-1&&observed_bus.data6_io_num==-1&&observed_bus.data7_io_num==-1);
        CHECK(observed_bus.max_transfer_sz==1024&&observed_bus.intr_flags==ESP_INTR_FLAG_IRAM&&observed_bus.isr_cpu_id==ESP_INTR_CPU_AFFINITY_1);
        CHECK(observed_slave.spics_io_num==10&&observed_slave.queue_size==1&&!observed_slave.mode&&!observed_slave.flags);
        CHECK(observed_slave.post_setup_cb&&observed_slave.post_trans_cb);
        CHECK(s_transaction.length==8192&&!s_transaction.flags&&s_transaction.tx_buffer==s_dma.tx&&s_transaction.rx_buffer==s_dma.rx);
        CHECK(s_pool.dma_owner==BRIDGE_DMA_DRIVER&&ready_level==0);
        if(scenario==7)CHECK(bridge_spi_start(NULL)==ESP_ERR_INVALID_STATE&&driver_count==1);
    }
    CHECK(violations==0&&lock_depth==0);return 0;
}
int bridge_spi_run_trace(unsigned flags,const uint8_t *frames,unsigned count,const uint32_t *lengths,
                         const uint32_t *timeouts,unsigned reserve_from,uint32_t random_value,int failure)
{
    CHECK(count<TRACE_MAX);int error=setup(0);if(error)return error;
    mode=flags;outbound=(flags&4)!=0;reserve_after=reserve_from;rng_value=random_value;input_count=count;
    for(unsigned i=0;i<count;++i){memcpy(input[i],frames+i*1024,1024);bits[i]=lengths[i];}
    for(unsigned i=0;i<=count;++i)waits[i]=timeouts[i];
    if(flags&1){const bridge_spi_port_t port={.reserve=reserve_frame,.commit=commit_frame,.cancel=cancel_frame,
        .telemetry=take_telemetry,.peek_tx=(flags&4)?peek_outbound:NULL,
        .tx_consumed=(flags&4)?consumed_frame:NULL,.session_changed=session_notice};CHECK(bridge_spi_configure(&port)==ESP_OK);}
    if(failure==1)queue_error_at=0;if(failure==2)get_error_at=0;if(failure==3)alien_at=0;if(failure==4)queue_error_at=1;
    if(failure==5)get_error_at=2;
    CHECK(bridge_spi_start(NULL)==ESP_OK);CHECK(violations==0&&lock_depth==0);
    CHECK(bridge_spi_configure(NULL)==ESP_ERR_INVALID_STATE);return 0;
}
size_t bridge_spi_trace_size(void){return sizeof(bridge_spi_snapshot_t);}
unsigned bridge_spi_trace_metric(unsigned id)
{
    switch(id){case 0:return commits;case 1:return cancels;case 2:return consumed;case 3:return session_notifications;
    case 4:return(unsigned)ready_level;case 5:return(unsigned)s_pool.dma_owner;case 6:return rng_calls;
    case 7:return s_protocol.link.ack_seq;case 8:return s_protocol.link.pending;case 9:return queued_count;
    case 10:return ready_count;default:return violations;}
}
unsigned bridge_spi_trace_ready(unsigned index){return index<ready_count?ready_events[index]:99;}
size_t bridge_spi_trace_capture(unsigned index,uint8_t *out,size_t capacity)
{if(index>=queued_count||capacity<1024)return 0;memcpy(out,captured[index],1024);return 1024;}
size_t bridge_spi_trace_commit(unsigned index,uint8_t *out,size_t capacity,uint32_t *request)
{if(index>=commits||capacity<committed_length[index])return 0;
 memcpy(out,committed[index],committed_length[index]);*request=committed_request[index];return committed_length[index];}
int bridge_spi_port_check_test(void)
{int e=setup(0);if(e)return e;const bridge_spi_port_t bad={.reserve=reserve_frame};
 CHECK(bridge_spi_configure(&bad)==ESP_ERR_INVALID_ARG);CHECK(!s_started);
 CHECK(bridge_spi_configure(NULL)==ESP_OK);CHECK(bridge_spi_snapshot(NULL)==ESP_ERR_INVALID_ARG);return 0;}
