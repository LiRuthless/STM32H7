/* Phase14: v3 并列参数；v1/v2只读解码，方向保存只从显式bank取值。 */
#include "bsp.h"
#include "app_config.h"
#include "param.h"
#include "menu.h"
#include "pid.h"
#include "motor.h"
#include "control.h"
#include <stddef.h>
#include <string.h>
#define PARAM_MAGIC 0xA55A3C3Cu
#define PARAM_VERSION 3u
typedef struct
{
    uint32_t magic;
    uint16_t version;
    uint16_t crc16;
    uint8_t page;
    uint8_t arrow;
    uint8_t reserved[2];
    float kp_v;
    float ki_v;
    float KP_x_low;
    float K2P_x_low;
    float KD_x_low;
    int16_t base_speed_low;
    int16_t fan_duty_low;
    float KP_x_high;
    float K2P_x_high;
    float KD_x_high;
    int16_t base_speed_high;
    int16_t fan_duty_high;
} param_store_v1_t;
typedef struct
{
    uint32_t magic;
    uint16_t version;
    uint16_t crc16;
    uint8_t page;
    uint8_t arrow;
    uint8_t reserved[2];
    float kp_v;
    float ki_v;
    float KP_x_low;
    float K2P_x_low;
    float KD_x_low;
    int16_t base_speed_low;
    int16_t fan_duty_low;       /* 运行风扇值 */
    float KP_x_high;
    float K2P_x_high;
    float KD_x_high;
    int16_t base_speed_high;
    int16_t fan_duty_high;      /* 运行风扇值 */
    int16_t fan_duty_idle_low;
    int16_t fan_duty_idle_high;
} param_store_v2_t;
typedef struct
{
    uint32_t magic;
    uint16_t version;
    uint16_t crc16;
    uint8_t page;
    uint8_t arrow;
    uint8_t reserved[2];
    float kp_v;
    float ki_v;
    float KP_x_low;
    float K2P_x_low;
    float KD_x_low;
    int16_t base_speed_low;
    int16_t fan_duty_low;       /* 运行风扇值 */
    float KP_x_high;
    float K2P_x_high;
    float KD_x_high;
    int16_t base_speed_high;
    int16_t fan_duty_high;      /* 运行风扇值 */
    int16_t fan_duty_idle_low;
    int16_t fan_duty_idle_high;
    float optical_kp_low, optical_k2p_low, optical_kd_low;
    int16_t optical_speed_low;
    uint16_t optical_reserved_low;
    float optical_kp_high, optical_k2p_high, optical_kd_high;
    int16_t optical_speed_high;
    uint16_t optical_reserved_high;
} param_store_t;

typedef char param_v1_size[(sizeof(param_store_v1_t)==52)?1:-1];
typedef char param_v2_size[(sizeof(param_store_v2_t)==56)?1:-1];
typedef char param_v3_size[(sizeof(param_store_t)==88)?1:-1];
typedef char param_v3_offset[(offsetof(param_store_t,optical_kp_low)==56 && offsetof(param_store_t,optical_kp_high)==72)?1:-1];
static param_store_t s_param;
static param_direction_t s_direction[2][2];
static uint8_t s_dirty, s_flash_usable;
static uint32_t s_dirty_tick;
static uint16_t crc16_ccitt(const uint8_t *data, uint32_t len)
{
    uint16_t crc=0xFFFFu;
    while(len--) {
        crc^=(uint16_t)(*data++)<<8;
        for(uint8_t i=0;i<8;i++) crc=(crc&0x8000u)?(uint16_t)((crc<<1)^0x1021u):(uint16_t)(crc<<1);
    }
    return crc;
}
static uint16_t param_crc_v1(const param_store_v1_t *s) { return crc16_ccitt((const uint8_t *)s+8,44); }
static uint16_t param_crc_v2(const param_store_v2_t *s) { return crc16_ccitt((const uint8_t *)s+8,48); }
static uint16_t param_crc(const param_store_t *s) { return crc16_ccitt((const uint8_t *)s+8,80); }
static int16_t clamp_fan_duty(int16_t x) { return x<0?0:(x>10000?10000:x); }
static void optical_defaults(void)
{
    s_param.optical_kp_low=s_param.optical_kp_high=2.0f;
    s_param.optical_k2p_low=s_param.optical_k2p_high=0.008f;
    s_param.optical_kd_low=s_param.optical_kd_high=15.0f;
    s_param.optical_speed_low=s_param.optical_speed_high=100;
    s_param.optical_reserved_low=s_param.optical_reserved_high=0;
}
static void param_set_defaults(void)
{
    memset(&s_param,0,sizeof s_param);
    s_param.magic=PARAM_MAGIC;s_param.version=3;s_param.page=PAGE_HOME;s_param.arrow=1;
    s_param.kp_v=20.0f;s_param.ki_v=0.75f;
    s_param.fan_duty_low=s_param.fan_duty_high=FAN_DUTY_RUN;
    s_param.fan_duty_idle_low=s_param.fan_duty_idle_high=FAN_DUTY_IDLE;
    optical_defaults();
}
static void load_banks(void)
{
    s_direction[0][0]=(param_direction_t){s_param.KP_x_low,s_param.K2P_x_low,s_param.KD_x_low,s_param.base_speed_low};
    s_direction[0][1]=(param_direction_t){s_param.KP_x_high,s_param.K2P_x_high,s_param.KD_x_high,s_param.base_speed_high};
    s_direction[1][0]=(param_direction_t){s_param.optical_kp_low,s_param.optical_k2p_low,s_param.optical_kd_low,s_param.optical_speed_low};
    s_direction[1][1]=(param_direction_t){s_param.optical_kp_high,s_param.optical_k2p_high,s_param.optical_kd_high,s_param.optical_speed_high};
}
static void store_banks(void)
{
    const param_direction_t *p=&s_direction[0][0];
    s_param.KP_x_low=p->kp;s_param.K2P_x_low=p->k2p;s_param.KD_x_low=p->kd;s_param.base_speed_low=p->base_speed;
    p=&s_direction[0][1];
    s_param.KP_x_high=p->kp;s_param.K2P_x_high=p->k2p;s_param.KD_x_high=p->kd;s_param.base_speed_high=p->base_speed;
    p=&s_direction[1][0];
    s_param.optical_kp_low=p->kp;s_param.optical_k2p_low=p->k2p;s_param.optical_kd_low=p->kd;s_param.optical_speed_low=p->base_speed;
    p=&s_direction[1][1];
    s_param.optical_kp_high=p->kp;s_param.optical_k2p_high=p->k2p;s_param.optical_kd_high=p->kd;s_param.optical_speed_high=p->base_speed;
    s_param.optical_reserved_low=s_param.optical_reserved_high=0;
}
const param_direction_t *Param_GetDirection(uint8_t mode,uint8_t gear)
{ return mode<2 && gear<2 ? &s_direction[mode][gear] : 0; }
void Param_ApplyCurrentDirection(void)
{
    const param_direction_t *p=Param_GetDirection(Tracking_GetState()->mode,Tracking_GetState()->gear);
    const pid_track_gains_t gains={p->kp,p->k2p,p->kd,1.0f};
    PID_SetTrackGains(&gains);Motor_SetBaseSpeed(p->base_speed);
}
uint8_t Param_SetDirection(uint8_t mode,uint8_t gear,const param_direction_t *p)
{
    if(mode>=2 || gear>=2 || !p) return 0;
    s_direction[mode][gear]=*p;Param_MarkDirty();
    if(mode==Tracking_GetState()->mode && gear==Tracking_GetState()->gear) Param_ApplyCurrentDirection();
    return 1;
}
int16_t Param_GetFan(uint8_t gear,uint8_t idle)
{
    if(gear>=2) return 0;
    return gear ? (idle?s_param.fan_duty_idle_high:s_param.fan_duty_high) :
                  (idle?s_param.fan_duty_idle_low:s_param.fan_duty_low);
}
void Param_SetFan(uint8_t gear,uint8_t idle,int16_t value)
{
    if(gear>=2) return;
    value=clamp_fan_duty(value);
    if(gear) { if(idle)s_param.fan_duty_idle_high=value;else s_param.fan_duty_high=value; }
    else { if(idle)s_param.fan_duty_idle_low=value;else s_param.fan_duty_low=value; }
    if(gear==Tracking_GetState()->gear) { if(idle)Motor_SetFanDutyIdle(value);else Motor_SetFanDuty(value); }
    Param_MarkDirty();
}
void Param_SelectGear(uint8_t gear)
{
    if(gear>=2) return;
    Tracking_SetGear(gear);Param_ApplyCurrentDirection();
    Motor_SetFanDuty(Param_GetFan(gear,0));Motor_SetFanDutyIdle(Param_GetFan(gear,1));
}
static uint8_t param_write_mirror(void)
{
    store_banks();s_param.magic=PARAM_MAGIC;s_param.version=3;s_param.crc16=param_crc(&s_param);
    if(BSP_Flash_Write(0,(const uint8_t *)&s_param,sizeof s_param))
    { BSP_UART_WriteString("parameter save fail\r\n");return 0; }
    return 1;
}
uint8_t Param_Load(void)
{
    uint8_t header[8], rewrite=0, io_error=0, invalid=0;
    uint16_t version=0;
    s_dirty=0;param_set_defaults();
    if(BSP_Flash_Read(0,header,8)) io_error=1;
    else {
        uint32_t magic=(uint32_t)header[0]|((uint32_t)header[1]<<8)|((uint32_t)header[2]<<16)|((uint32_t)header[3]<<24);
        if(magic==PARAM_MAGIC) version=(uint16_t)(header[4]|((uint16_t)header[5]<<8));
        if(version==1) {
            param_store_v1_t old;
            if(BSP_Flash_Read(0,(uint8_t *)&old,sizeof old)) io_error=1;
            else if(old.magic==PARAM_MAGIC && old.version==1 && old.crc16==param_crc_v1(&old)) {
                memcpy(&s_param,&old,sizeof old);
                s_param.fan_duty_idle_low=s_param.fan_duty_idle_high=FAN_DUTY_IDLE;
                optical_defaults();rewrite=1;
            } else rewrite=invalid=1;
        } else if(version==2) {
            param_store_v2_t old;
            if(BSP_Flash_Read(0,(uint8_t *)&old,sizeof old)) io_error=1;
            else if(old.magic==PARAM_MAGIC && old.version==2 && old.crc16==param_crc_v2(&old)) {
                memcpy(&s_param,&old,sizeof old);optical_defaults();rewrite=1;
            } else rewrite=invalid=1;
        } else if(version==3) {
            param_store_t current;
            if(BSP_Flash_Read(0,(uint8_t *)&current,sizeof current)) io_error=1;
            else if(current.magic==PARAM_MAGIC && current.version==3 && current.crc16==param_crc(&current)) s_param=current;
            else rewrite=invalid=1;
        } else rewrite=invalid=1;
    }
    if(io_error) BSP_UART_WriteString("parameter read fail; defaults\r\n");
    else if(invalid) BSP_UART_WriteString("parameter invalid; defaults\r\n");
    /* 旧兼容风扇钳位；方向/速度字段保持原值，不用默认值覆盖有效旧镜像。 */
    {
        int16_t *fans[]={&s_param.fan_duty_low,&s_param.fan_duty_high,&s_param.fan_duty_idle_low,&s_param.fan_duty_idle_high};
        for(uint8_t i=0;i<4;++i) { int16_t x=clamp_fan_duty(*fans[i]);if(x!=*fans[i]){*fans[i]=x;rewrite=1;} }
    }
    load_banks();s_flash_usable=(uint8_t)!io_error;
    if(rewrite && s_flash_usable && !param_write_mirror()) s_flash_usable=0;
    PID_SetGain(PID_GAIN_KP_V,s_param.kp_v);PID_SetGain(PID_GAIN_KI_V,s_param.ki_v);
    Param_SelectGear(PARAM_GEAR_LOW);
    if(s_param.page==PAGE_HOME || s_param.page==PAGE_ADC_ERR || s_param.page==PAGE_SPD_DIS ||
       s_param.page==PAGE_GYRO || s_param.page==PAGE_ADJUST1 || s_param.page==PAGE_ADJUST2) page=s_param.page;
    else page=PAGE_HOME;
    arrow=(s_param.arrow>=1 && s_param.arrow<=5)?s_param.arrow:1;
    return s_flash_usable;
}
void Param_MarkDirty(void) { s_dirty=1;s_dirty_tick=HAL_GetTick(); }
uint8_t Param_TryFlushPending(void)
{
    if(!s_flash_usable) return 0;
    if(!s_dirty) return 1;
    s_param.kp_v=PID_GetState()->kp_v;s_param.ki_v=PID_GetState()->ki_v;
    s_param.page=page;s_param.arrow=arrow;
    if(!param_write_mirror()) {s_flash_usable=0;return 0;}
    s_dirty=0;return 1;
}
void Param_Save(void) { s_dirty=1;(void)Param_TryFlushPending(); }
void Param_ServiceSave(void)
{ if(s_dirty && (uint32_t)(HAL_GetTick()-s_dirty_tick)>=3000u) (void)Param_TryFlushPending(); }
void Param_FlushPending(void) { (void)Param_TryFlushPending(); }
