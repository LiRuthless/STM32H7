#ifndef DATALOG_H
#define DATALOG_H
#include <stdint.h>
#define DATALOG_RECORD_SIZE 96u
#define DATALOG_HEADER_SIZE 256u
#define DATALOG_CAPACITY 87336u
#define DATALOG_BUFFER_RECORDS 1024u

typedef struct {
    uint32_t tick;
    int16_t adc[4],speed_l,speed_r,track_error,track_out,gyro_x,gyro_y,gyro_z;
    uint16_t dl1b_mm,battery;
    uint8_t kernel_state,roundabout_state;
    uint16_t gray_raw[16];
    uint32_t gray_sequence,control_sequence;
    int16_t em_error,optical_error;
    uint16_t protection_flags;
    uint8_t mode,optical_state,gray_valid,stop_reason;
    uint8_t reserved[14];
} datalog_record_t;
typedef struct { float kp,k2p,kd; int16_t base_speed; uint16_t reserved; } datalog_profile_t;
typedef struct {
    uint32_t magic;
    uint16_t version,crc16,header_size,record_size,sample_period_us,control_period_us;
    uint8_t mode,gear;
    uint16_t channel_count,noise,lost,min_contrast,reserved;
    int16_t black[16],white[16];
    int8_t weights[16];
    datalog_profile_t em,optical;
    float kp_v,ki_v;
    int16_t fan_run,fan_idle;
    uint32_t gray_budget_us,control_budget_us,run_delay_count,capacity,datalog_div;
    uint8_t reserved_tail[84];
} datalog_header_t;
typedef enum { DATALOG_FAULT_NONE=0,DATALOG_FAULT_CONFIG,DATALOG_FAULT_UNAVAILABLE,
    DATALOG_FAULT_HEADER,DATALOG_FAULT_OVERFLOW,DATALOG_FAULT_WRITE,DATALOG_FAULT_FULL,
    DATALOG_FAULT_READ } datalog_fault_t;
typedef struct {
    volatile uint8_t active,fault,reason;
    volatile uint16_t protection_flags;
    volatile uint32_t accepted,written,rejected,pending;
} datalog_status_t;
void Datalog_Init(void);
uint8_t Datalog_Start(void);
uint8_t Datalog_Clear(void);
void Datalog_Stop(void);
void Datalog_Push(void);
void Datalog_Service(void);
void Datalog_Flush(void);
void Datalog_Dump(void);
void Datalog_PublishControl(void);
void Datalog_SetStopSummary(uint8_t reason,uint16_t flags);
const datalog_status_t *Datalog_GetStatus(void);
uint8_t Datalog_IsActive(void);
uint32_t Datalog_GetCount(void);
#endif
