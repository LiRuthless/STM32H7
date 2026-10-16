/* Phase14 日志v2：固定little-endian布局，单生产者/单消费者；Flash仅前台访问。
 * TIM6/TIM15同优先级，控制快照发布不交错；其他共享标志用极短PRIMASK锁存。 */
#include "bsp.h"
#include "app_config.h"
#include "datalog.h"
#include "app.h"
#include "control.h"
#include "track_sensor.h"
#include "gray_sensor.h"
#include "optical_fsm.h"
#include "electromagnetic_fsm.h"
#include "param.h"
#include "pid.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#define LOG_MAGIC 0x324C4353u
#define BATCH_RECORDS 8u
#define ASSERT_LAYOUT(name,expr) typedef char name[(expr)?1:-1]
ASSERT_LAYOUT(record_size,sizeof(datalog_record_t)==96);
ASSERT_LAYOUT(header_size,sizeof(datalog_header_t)==256);
ASSERT_LAYOUT(record_offsets,offsetof(datalog_record_t,gray_raw)==32 && offsetof(datalog_record_t,gray_sequence)==64 &&
    offsetof(datalog_record_t,protection_flags)==76 && offsetof(datalog_record_t,reserved)==82);
ASSERT_LAYOUT(header_offsets,offsetof(datalog_header_t,black)==28 && offsetof(datalog_header_t,weights)==92 &&
    offsetof(datalog_header_t,em)==108 && offsetof(datalog_header_t,kp_v)==140 && offsetof(datalog_header_t,reserved_tail)==172);
ASSERT_LAYOUT(log_capacity,(BSP_FLASH_LOG_SIZE-256)/96==DATALOG_CAPACITY);
static datalog_record_t s_buf[DATALOG_BUFFER_RECORDS];
static datalog_record_t s_snapshot;
static datalog_header_t s_header;
static uint8_t s_batch[BATCH_RECORDS*DATALOG_RECORD_SIZE];
static volatile uint16_t s_head,s_tail;
static datalog_status_t s_status;
static uint8_t s_header_valid;
static uint8_t s_write_failed;
static uint16_t crc16(const uint8_t *data,uint32_t len)
{
    uint16_t crc=0xffff;
    while(len--) { crc^=(uint16_t)*data++<<8;for(uint8_t i=0;i<8;++i)crc=(crc&0x8000)?(uint16_t)((crc<<1)^0x1021):(uint16_t)(crc<<1); }
    return crc;
}
static void latch_fault(uint8_t fault)
{
    uint32_t mask=BSP_Time_EnterCritical();
    if(!s_status.fault) s_status.fault=fault;
    if(fault!=DATALOG_FAULT_READ) s_status.active=0;
    BSP_Time_ExitCritical(mask);
}
static void run_fault(uint8_t fault,app_stop_reason_t reason,uint16_t flag)
{
    latch_fault(fault);Tracking_AddProtection(flag);App_RequestStop(reason);
}
void Datalog_SetStopSummary(uint8_t reason,uint16_t flags)
{
    uint32_t mask=BSP_Time_EnterCritical();
    if(!s_status.reason) s_status.reason=reason;
    s_status.protection_flags|=flags;
    BSP_Time_ExitCritical(mask);
}
const datalog_status_t *Datalog_GetStatus(void)
{
    s_status.pending=s_status.accepted-s_status.written;
    return &s_status;
}
uint8_t Datalog_IsActive(void) { return s_status.active; }
uint32_t Datalog_GetCount(void) { return s_status.written; }
void Datalog_Init(void)
{
    s_head=s_tail=0;memset(&s_status,0,sizeof s_status);
    memset(&s_snapshot,0,sizeof s_snapshot);s_header_valid=0;s_write_failed=0;
}
static void set_profile(datalog_profile_t *dst,const param_direction_t *src)
{ dst->kp=src->kp;dst->k2p=src->k2p;dst->kd=src->kd;dst->base_speed=src->base_speed;dst->reserved=0; }
uint8_t Datalog_Start(void)
{
    static datalog_header_t check;
    const tracking_state_t *t=Tracking_GetState();
    Datalog_Init();
    if(!DATALOG_ENABLE || DATALOG_DIV!=1) {latch_fault(DATALOG_FAULT_CONFIG);return 1;}
    if(!Gray_CheckCalibration(0)) {latch_fault(DATALOG_FAULT_HEADER);return 1;}
    memset(&s_header,0,sizeof s_header);
    s_header.magic=LOG_MAGIC;s_header.version=2;s_header.header_size=256;s_header.record_size=96;
    s_header.sample_period_us=1000;s_header.control_period_us=2000;
    s_header.mode=t->mode;s_header.gear=t->gear;s_header.channel_count=16;
    s_header.noise=50;s_header.lost=200;s_header.min_contrast=200;
    memcpy(s_header.black,Gray_GetBlack(),32);memcpy(s_header.white,Gray_GetWhite(),32);
    for(uint8_t i=0;i<16;++i)s_header.weights[i]=(int8_t)(15-2*(int)i);
    set_profile(&s_header.em,Param_GetDirection(0,t->gear));set_profile(&s_header.optical,Param_GetDirection(1,t->gear));
    s_header.kp_v=PID_GetState()->kp_v;s_header.ki_v=PID_GetState()->ki_v;
    s_header.fan_run=Param_GetFan(t->gear,0);s_header.fan_idle=Param_GetFan(t->gear,1);
    s_header.gray_budget_us=700;s_header.control_budget_us=900;s_header.run_delay_count=1000;
    s_header.capacity=DATALOG_CAPACITY;s_header.datalog_div=1;
    s_header.crc16=crc16((const uint8_t *)&s_header+8,248);
    if(BSP_Flash_LogErase()) {latch_fault(DATALOG_FAULT_UNAVAILABLE);return 1;}
    if(BSP_Flash_LogWrite(0,(const uint8_t *)&s_header,256) ||
       BSP_Flash_LogRead(0,(uint8_t *)&check,256) || memcmp(&check,&s_header,256))
    {latch_fault(DATALOG_FAULT_HEADER);return 1;}
    s_header_valid=1;s_status.active=1;return 0;
}
uint8_t Datalog_Clear(void)
{
    s_status.active=0;s_header_valid=0;
    /* 擦除不伪造新会话；首次故障和上一会话RAM摘要保留到下次Start。 */
    return BSP_Flash_LogErase();
}
void Datalog_PublishControl(void)
{
    const track_state_t *em=Track_GetState();const gray_state_t *g=Gray_GetState();
    const tracking_state_t *t=Tracking_GetState();
    memset(&s_snapshot,0,sizeof s_snapshot);
    memcpy(s_snapshot.adc,em->filtered,8);memcpy(s_snapshot.gray_raw,g->raw,32);
    s_snapshot.track_error=t->active_error;s_snapshot.track_out=t->track_out;
    s_snapshot.dl1b_mm=g_app_dl1b_distance_mm;s_snapshot.battery=(uint16_t)battery_filt;
    s_snapshot.kernel_state=ElectromagneticFSM_GetState()->kernel_state;
    s_snapshot.roundabout_state=ElectromagneticFSM_GetState()->roundabout_state;
    s_snapshot.gray_sequence=g->sequence;s_snapshot.control_sequence=t->control_sequence;
    s_snapshot.em_error=em->error;s_snapshot.optical_error=g->error;
    s_snapshot.protection_flags=t->protection_flags;s_snapshot.mode=t->mode;
    s_snapshot.optical_state=(uint8_t)OpticalFSM_GetState()->state;
    s_snapshot.gray_valid=(uint8_t)(g->calibration_valid && g->line_valid && !g->acquisition_status);
    s_snapshot.stop_reason=s_status.reason;
    BSP_Time_MemoryBarrier();
}
void Datalog_Push(void)
{
    uint16_t next;int16_t raw[3];datalog_record_t rec;
    if(!s_status.active || s_status.fault) return;
    if(s_status.accepted>=DATALOG_CAPACITY) {run_fault(DATALOG_FAULT_FULL,APP_STOP_LOG_FULL,TRACKING_PROTECT_LOG_FULL);return;}
    next=(uint16_t)((s_head+1u)%DATALOG_BUFFER_RECORDS);
    if(next==s_tail) {++s_status.rejected;run_fault(DATALOG_FAULT_OVERFLOW,APP_STOP_LOG_OVERFLOW,TRACKING_PROTECT_LOG_OVERFLOW);return;}
    rec=s_snapshot;rec.tick=BSP_Sampler_GetTick();
    rec.speed_l=BSP_Sampler_GetEncL_1ms();rec.speed_r=BSP_Sampler_GetEncR_1ms();
    BSP_Sampler_GetGyroRaw(&raw[0],&raw[1],&raw[2]);
    rec.gyro_x=(int16_t)((float)raw[0]/GYRO_RAW_TO_DPS);rec.gyro_y=(int16_t)((float)raw[1]/GYRO_RAW_TO_DPS);
    rec.gyro_z=(int16_t)((float)raw[2]/GYRO_RAW_TO_DPS);
    s_buf[s_head]=rec;BSP_Time_MemoryBarrier();s_head=next;++s_status.accepted;
    if(s_status.accepted==DATALOG_CAPACITY) run_fault(DATALOG_FAULT_FULL,APP_STOP_LOG_FULL,TRACKING_PROTECT_LOG_FULL);
}
void Datalog_Service(void)
{
    uint32_t pending,n;uint16_t tail=s_tail;
    if(s_write_failed || !s_header_valid) return;
    pending=((uint32_t)s_head+DATALOG_BUFFER_RECORDS-tail)%DATALOG_BUFFER_RECORDS;
    if(!pending || (s_status.active && pending<BATCH_RECORDS)) return;
    n=pending<BATCH_RECORDS?pending:BATCH_RECORDS;
    BSP_Time_MemoryBarrier();
    for(uint32_t i=0;i<n;++i) memcpy(s_batch+i*96,&s_buf[(tail+i)%DATALOG_BUFFER_RECORDS],96);
    if(BSP_Flash_LogWrite(256+s_status.written*96,s_batch,n*96))
    {s_write_failed=1;run_fault(DATALOG_FAULT_WRITE,APP_STOP_LOG_WRITE,TRACKING_PROTECT_LOG_WRITE);return;}
    s_status.written+=n;BSP_Time_MemoryBarrier();s_tail=(uint16_t)((tail+n)%DATALOG_BUFFER_RECORDS);
}
void Datalog_Flush(void) { Datalog_Service(); }
void Datalog_Stop(void)
{
    s_status.active=0;
    for(uint16_t i=0;i<128 && s_tail!=s_head && !s_write_failed;++i)
    {Datalog_Service();BSP_WDT_Feed();}
}
static uint8_t valid_header(const datalog_header_t *h)
{
    return h->magic==LOG_MAGIC && h->version==2 && h->header_size==256 && h->record_size==96 &&
        h->sample_period_us==1000 && h->control_period_us==2000 && h->mode<2 && h->gear<2 &&
        h->channel_count==16 && h->noise==50 && h->lost==200 && h->min_contrast==200 &&
        h->capacity==DATALOG_CAPACITY && h->datalog_div==1 && h->crc16==crc16((const uint8_t *)h+8,248) &&
        !Gray_ValidateCalibration(h->black,h->white);
}
void Datalog_Dump(void)
{
    /* 仅停车前台调用，不可重入；大缓冲静态分配，避开AC5的1KB栈。 */
    static datalog_header_t header;
    static datalog_record_t rec;
    static char line[640];
    int len;
    Datalog_Stop();
    if(BSP_Flash_LogRead(0,(uint8_t *)&header,256))
    {latch_fault(DATALOG_FAULT_READ);BSP_UART_WriteString("#dump_error=read\r\n");goto done;}
    if(!s_header_valid || !valid_header(&header))
    {BSP_UART_WriteString("#dump_error=no_valid_session\r\n");goto done;}
    (void)snprintf(line,sizeof line,"#log_version=2,record_size=96,mode=%u,gear=%u\r\n",header.mode,header.gear);
    BSP_UART_WriteString(line);
    for(uint8_t color=0;color<2;++color)
    {
        const int16_t *values=color?header.white:header.black;
        len=snprintf(line,sizeof line,color?"#white":"#black");
        for(uint8_t i=0;i<16;++i) len+=snprintf(line+len,sizeof line-(size_t)len,",%d",values[i]);
        BSP_UART_WriteString(line);BSP_UART_WriteString("\r\n");
    }
    BSP_UART_WriteString("tick,adc0,adc1,adc2,adc3,spdL,spdR,err,tout,gx,gy,gz,dl1b,bat,kstate,rstate");
    for(uint8_t i=0;i<16;++i){snprintf(line,sizeof line,",gray%u",i);BSP_UART_WriteString(line);}
    for(uint8_t i=0;i<16;++i){snprintf(line,sizeof line,",q%u",i);BSP_UART_WriteString(line);}
    BSP_UART_WriteString(",em_err,opt_err,mode,opt_state,gray_valid,protect,reason,gray_seq,control_seq\r\n");
    for(uint32_t i=0;i<s_status.written;++i)
    {
        if(BSP_Flash_LogRead(256+i*96,(uint8_t *)&rec,96))
        {latch_fault(DATALOG_FAULT_READ);BSP_UART_WriteString("#dump_error=read\r\n");break;}
        len=snprintf(line,sizeof line,"%lu,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u,%u,%u,%u",
            (unsigned long)rec.tick,rec.adc[0],rec.adc[1],rec.adc[2],rec.adc[3],rec.speed_l,rec.speed_r,
            rec.track_error,rec.track_out,rec.gyro_x,rec.gyro_y,rec.gyro_z,rec.dl1b_mm,rec.battery,rec.kernel_state,rec.roundabout_state);
        for(uint8_t j=0;j<16;++j) len+=snprintf(line+len,sizeof line-(size_t)len,",%u",rec.gray_raw[j]);
        for(uint8_t j=0;j<16;++j) len+=snprintf(line+len,sizeof line-(size_t)len,",%u",Gray_Normalize(rec.gray_raw[j],header.black[j],header.white[j]));
        len+=snprintf(line+len,sizeof line-(size_t)len,",%d,%d,%u,%u,%u,%u,%u,%lu,%lu\r\n",rec.em_error,rec.optical_error,
            rec.mode,rec.optical_state,rec.gray_valid,rec.protection_flags,rec.stop_reason,(unsigned long)rec.gray_sequence,(unsigned long)rec.control_sequence);
        if(len>0 && (size_t)len<sizeof line) BSP_UART_Write((const uint8_t *)line,(uint16_t)len);
        BSP_WDT_Feed();
    }
done:
    {
        const datalog_status_t *s=Datalog_GetStatus();
        snprintf(line,sizeof line,"#status,fault=%u,accepted=%lu,written=%lu,rejected=%lu,pending=%lu,reason=%u,protect=%u\r\n",
            s->fault,(unsigned long)s->accepted,(unsigned long)s->written,(unsigned long)s->rejected,(unsigned long)s->pending,s->reason,s->protection_flags);
        BSP_UART_WriteString(line);
    }
    BSP_UART_WriteString("EOF\r\n");
}
