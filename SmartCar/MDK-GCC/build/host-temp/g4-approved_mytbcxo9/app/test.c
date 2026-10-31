
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "bsp.h"
#include "app.c"
#include "lifecycle.h"
#include "safety.h"
#include "battery.h"
#include "parameter_application.h"
#include "parameter_bank.h"
#include "parameter_store.h"
#include "lifecycle.h"
#include "menu.h"
#include "wireless.h"
#include "track_sensor.h"
#include "imu_proc.h"
#include "pid.h"
#include "legacy_pid.h"
#include "legacy_control.h"
#include "tracking.h"
#include "legacy_roundabout.h"
#include "motor.h"
#include "filter.h"
#include "datalog.h"
#include "dashboard.h"
#include "gray_sensor.h"
#include "scb_link.h"
#include "telemetry.h"
#include "tuning.h"
#include "command_dispatch.h"
#include "optical_fsm.h"
TIM_HandleTypeDef htim6={TIM6},htim7={TIM7};
static uint16_t adc_values[10]={500,700,400,700,1000,0,0,1300};
static uint32_t pwm[3],cycles,cycle_step=10,gray_seq;
static unsigned timer_starts,timer_stops,sampler_on,encoder_reads,gray_reads,calibration_reads,stop_reports;
static unsigned now,mask,restore_checks;
static int gray_line=1,acquire_error,param_write_error,log_write_error,log_read_error;
static int inject_stop;
void Test_InjectStop(void){if(inject_stop){inject_stop=0;App_RequestStop(APP_STOP_LOW_BATTERY);}}
static uint8_t params[4096],logs[8384512];
static unsigned log_writes;
static unsigned link_services,start_events,stop_events;
static telemetry_status_t upload_status;
void SCB_Init(void){memset(&upload_status,0,sizeof upload_status);}
void SCB_Service(void){++link_services;}
static scb_command_handler_t registered_command;
static scb_session_handler_t registered_reset;
void SCB_SetHandlers(scb_command_handler_t c,scb_session_handler_t r){registered_command=c;registered_reset=r;assert(c&&r);}
uint8_t SCB_QueueResult(uint32_t i,uint8_t t,const uint8_t*p,uint16_t n,uint8_t f){(void)i;(void)t;(void)p;(void)n;(void)f;return 1;}
uint8_t SCB_GetRequestResult(uint32_t i,uint8_t*t,uint8_t*p,uint16_t*n){(void)i;(void)t;(void)p;(void)n;return 0;}
void Telemetry_SetRevision(uint32_t r){upload_status.revision=r;}
uint8_t SCB_QueueEvent(const scb_event_t*e){if(e->kind==SCB_EVENT_RUN_STARTED)++start_events;else if(e->kind==SCB_EVENT_RUN_STOPPED){++stop_events;assert(e->stop_reason==Safety_GetStopReason());}return 1;}
void Telemetry_BeginRun(void){++upload_status.run_id;}
void Telemetry_GetStatus(telemetry_status_t*s){*s=upload_status;}
void Telemetry_PushRecord(const datalog_record_t*r){assert(r);}

uint32_t HAL_GetTick(void){return now++;}
uint32_t BSP_Time_GetTick(void){return HAL_GetTick();}
void HAL_GPIO_WritePin(void*p,uint16_t n,int level){(void)p;(void)n;(void)level;}
void HAL_GPIO_TogglePin(void*p,uint16_t n){(void)p;(void)n;}
int HAL_GPIO_ReadPin(void*p,uint16_t n){(void)p;(void)n;return 0;}
int HAL_TIM_Base_Start_IT(TIM_HandleTypeDef*t){(void)t;assert(!pwm[0]&&!pwm[1]);++timer_starts;return 0;}
int HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef*t){(void)t;++timer_stops;return 0;}
uint32_t BSP_Time_EnterCritical(void){unsigned old=mask;mask=1;return old;}
void BSP_Time_ExitCritical(uint32_t old){mask=old;++restore_checks;}
void BSP_Time_MemoryBarrier(void){}
void BSP_Time_Init(void){}
uint32_t BSP_Time_GetCycles(void){uint32_t v=cycles;cycles+=cycle_step;return v;}
void BSP_UART_Init(void){}
uint32_t BSP_UART_WriteString(const char*s){if(strncmp(s,"stop:",5)==0)++stop_reports;return 0;}
uint32_t BSP_UART_Write(const uint8_t*b,uint16_t n){(void)b;(void)n;return 0;}
uint16_t BSP_UART_Read(uint8_t*b,uint16_t n){assert(!Safety_IsStopPending());(void)b;(void)n;return 0;}
void BSP_Flash_Init(void){}
uint8_t BSP_Flash_Read(uint32_t a,uint8_t*b,uint32_t n){memcpy(b,params+a,n);return 0;}
uint8_t BSP_Flash_Write(uint32_t a,const uint8_t*b,uint32_t n){if(param_write_error)return 1;memcpy(params+a,b,n);return 0;}
uint8_t BSP_Flash_LogErase(void){memset(logs,255,4096);return 0;}
uint8_t BSP_Flash_LogRead(uint32_t a,uint8_t*b,uint32_t n){assert(a+n<=sizeof logs);if(log_read_error)return 1;memcpy(b,logs+a,n);return 0;}
uint8_t BSP_Flash_LogWrite(uint32_t a,const uint8_t*b,uint32_t n){assert(a+n<=sizeof logs);++log_writes;if(log_write_error)return 1;memcpy(logs+a,b,n);return 0;}
void BSP_ADC_Init(void){}
uint16_t BSP_ADC_Read(bsp_adc_channel_t ch){return adc_values[ch];}
void BSP_Gray_Init(void){gray_seq=0;}
void BSP_Gray_ResetSequence(void){gray_seq=0;}
bsp_gray_status_t BSP_Gray_ReadFrame(bsp_gray_frame_t*out){assert(!Safety_IsStopPending());++gray_reads;if(acquire_error)return BSP_GRAY_TIMEOUT;for(unsigned i=0;i<16;++i)out->raw[i]=gray_line?1000:0;out->sequence=++gray_seq;out->scan_cycles=100;cycles+=100;return BSP_GRAY_OK;}
void BSP_PWM_Init(void){memset(pwm,0,sizeof pwm);}
void BSP_PWM_SetDirection(bsp_pwm_channel_t ch,uint8_t high){HAL_GPIO_WritePin(ch==BSP_PWM_MOTOR_L?MOTOR_L_DIR_GPIO_Port:MOTOR_R_DIR_GPIO_Port,ch==BSP_PWM_MOTOR_L?MOTOR_L_DIR_Pin:MOTOR_R_DIR_Pin,high?GPIO_PIN_SET:GPIO_PIN_RESET);}
void BSP_PWM_SetDuty(bsp_pwm_channel_t ch,uint32_t duty){assert(duty<=10000);pwm[ch]=duty;}
void BSP_Encoder_Init(void){}
int16_t BSP_Sampler_ConsumeEncL(void){++encoder_reads;return 0;}
int16_t BSP_Sampler_ConsumeEncR(void){++encoder_reads;return 0;}
void BSP_Sampler_Init(void){}
void BSP_Sampler_Start(void){sampler_on=1;}
void BSP_Sampler_Stop(void){sampler_on=0;}
uint32_t BSP_Sampler_GetTick(void){static uint32_t tick;return ++tick;}
int16_t BSP_Sampler_GetEncL_1ms(void){return 0;}
int16_t BSP_Sampler_GetEncR_1ms(void){return 0;}
void BSP_Sampler_GetGyroRaw(int16_t*x,int16_t*y,int16_t*z){*x=*y=*z=0;}
uint8_t BSP_IMU660RB_Init(void){return 0;}
void BSP_IMU660RB_GetGyro(int16_t*x,int16_t*y,int16_t*z){++calibration_reads;*x=*y=*z=0;}
void BSP_IMU660RB_GetAcc(int16_t*x,int16_t*y,int16_t*z){*x=*y=*z=0;}
uint8_t BSP_DL1B_Init(void){return 0;}
void BSP_DL1B_Update(void){}
uint16_t BSP_DL1B_GetDistanceMm(void){return 8192;}
void BSP_WDT_Init(void){}
void BSP_WDT_Feed(void){}
void BSP_LCD_Init(void){}
void BSP_LCD_Clear(uint16_t x){assert(!Safety_IsStopPending());(void)x;}
void BSP_LCD_SetBacklight(uint8_t x){(void)x;}
void BSP_LCD_ShowString(uint16_t x,uint16_t y,const char*s,uint16_t a,uint16_t b){assert(!Safety_IsStopPending());(void)x;(void)y;(void)s;(void)a;(void)b;}
void BSP_LCD_ShowInt(uint16_t x,uint16_t y,int32_t v,uint8_t w,uint16_t a,uint16_t b){assert(!Safety_IsStopPending());(void)x;(void)y;(void)v;(void)w;(void)a;(void)b;}
void BSP_LCD_ShowFloat(uint16_t x,uint16_t y,float v,uint8_t i,uint8_t d,uint16_t a,uint16_t b){assert(!Safety_IsStopPending());(void)x;(void)y;(void)v;(void)i;(void)d;(void)a;(void)b;}
static void healthy(void){gray_line=1;acquire_error=0;cycle_step=10;adc_values[0]=500;adc_values[1]=700;adc_values[2]=400;adc_values[3]=700;adc_values[7]=1300;}
static void stopped(app_stop_reason_t reason)
{
    assert(!Lifecycle_IsStarted() && !Lifecycle_IsRunning() && !Lifecycle_IsKeyActive() && !pwm[0] && !pwm[1] && !sampler_on);
    assert(Safety_GetStopReason()==reason && pwm[2]==1100);
    unsigned reports=stop_reports;App_Loop();assert(stop_reports==reports+1);App_Loop();assert(stop_reports==reports+1);
    assert(link_services && start_events==stop_events);
}
int main(void)
{
    memset(params,255,sizeof params);App_Init();
    assert(Tracking_GetState()->mode==TRACKING_MODE_OPTICAL && Tracking_GetState()->gear==0);
    mask=1;Tracking_AddProtection(0);assert(mask==1);mask=0;
#if defined(TEST_UNCALIBRATED)
    assert(!Lifecycle_TryStart() && !timer_starts && !Lifecycle_IsStarted() && !pwm[0] && !pwm[1]);
    assert(Lifecycle_TrySetMode(0)==TRACKING_OK);
    assert(!Lifecycle_TryStart() && !timer_starts && !Lifecycle_IsStarted());
    puts("PASS V-05 (FR-16): production -1 calibration blocks BOTH modes before timers/PWM");return 0;
#else
    unsigned calibration=calibration_reads;
    gray_line=0;assert(!Lifecycle_TryStart() && !timer_starts);healthy();
    acquire_error=1;assert(!Lifecycle_TryStart() && !timer_starts);healthy();
    adc_values[0]=adc_values[1]=adc_values[2]=adc_values[3]=0;assert(!Lifecycle_TryStart() && !timer_starts);healthy();
    Param_MarkDirty();param_write_error=1;assert(!Lifecycle_TryStart() && !timer_starts);param_write_error=0;assert(Param_Load());
    log_write_error=1;assert(!Lifecycle_TryStart() && !timer_starts);log_write_error=0;
    assert(Lifecycle_TryStart() && timer_starts==2 && Gray_GetState()->sequence==0);
    assert(start_events==1 && !stop_events && upload_status.run_id==1);
    unsigned services=link_services;App_Loop();assert(link_services==services+1);

    unsigned frames=gray_reads;
    for(unsigned i=0;i<1000;++i)
    {
        App_ControlISR();assert(Lifecycle_IsStarted() && !Lifecycle_IsRunning() && !pwm[0] && !pwm[1]);
        App_SampleISR();App_SampleISR();Datalog_Service();
    }
    assert(gray_reads==frames+1000 && Gray_GetState()->sequence==1000 && OpticalFSM_GetState()->state==OPTICAL_WAIT);
    App_ControlISR();assert(Lifecycle_GetControlTicks()==1001 && Lifecycle_IsRunning() && pwm[0] && pwm[1] && OpticalFSM_GetState()->state==OPTICAL_TRACKING);
    unsigned enc=encoder_reads;gray_line=0;App_ControlISR();assert(encoder_reads==enc && OpticalFSM_GetState()->state==OPTICAL_LOST);
    App_RequestStop(APP_STOP_OFF_TRACK);assert(Safety_GetStopReason()==APP_STOP_GRAY_LOST && (Tracking_GetState()->protection_flags&3)==3);
    assert(Datalog_GetStatus()->reason==APP_STOP_GRAY_LOST);stopped(APP_STOP_GRAY_LOST);
    healthy();assert(Lifecycle_TryStart() && !Lifecycle_GetControlTicks() && Gray_GetState()->sequence==0 && OpticalFSM_GetState()->state==OPTICAL_WAIT && calibration_reads==calibration);
    acquire_error=1;adc_values[0]=adc_values[1]=adc_values[2]=adc_values[3]=0;cycle_step=432000;enc=encoder_reads;
    App_ControlISR();assert(encoder_reads==enc && (Tracking_GetState()->protection_flags&(TRACKING_PROTECT_ACQUIRE|TRACKING_PROTECT_EM_LOST|TRACKING_PROTECT_OVERRUN))==(TRACKING_PROTECT_ACQUIRE|TRACKING_PROTECT_EM_LOST|TRACKING_PROTECT_OVERRUN));stopped(APP_STOP_GRAY_ACQUIRE);
    healthy();assert(Lifecycle_TryStart());adc_values[0]=adc_values[1]=adc_values[2]=adc_values[3]=0;App_ControlISR();stopped(APP_STOP_OFF_TRACK);
    healthy();assert(Lifecycle_TryStart());cycle_step=432000;App_ControlISR();stopped(APP_STOP_CONTROL_OVERRUN);
    healthy();assert(Lifecycle_TryStart());adc_values[7]=1000;for(unsigned i=0;i<205 && Lifecycle_IsStarted();++i)App_TaskISR();stopped(APP_STOP_LOW_BATTERY);
    healthy();assert(Lifecycle_TryStart());for(unsigned i=0;i<1024;++i)App_SampleISR();stopped(APP_STOP_LOG_OVERFLOW);assert(Datalog_GetStatus()->rejected==1);
    healthy();assert(Lifecycle_TryStart());for(unsigned i=0;i<8;++i)App_SampleISR();log_write_error=1;unsigned writes=log_writes;App_Loop();assert(Safety_GetStopReason()==APP_STOP_LOG_WRITE && !pwm[0] && !pwm[1]);
    assert(log_writes==writes+1);App_Loop();assert(log_writes==writes+1);log_write_error=0;
    healthy();assert(Lifecycle_TryStart());for(unsigned i=0;i<87336;++i){App_SampleISR();if(i%8==7)Datalog_Service();}stopped(APP_STOP_LOG_FULL);
    assert(Datalog_GetStatus()->accepted==87336 && Datalog_GetStatus()->written==87336);
    healthy();assert(Lifecycle_TryStart());for(unsigned i=0;i<3;++i)App_SampleISR();
    unsigned reports=stop_reports;inject_stop=1;App_Loop();
    assert(!Safety_IsStopPending() && Datalog_GetStatus()->written==3 && Datalog_GetStatus()->accepted==3 && stop_reports==reports+1);
    App_Loop();assert(stop_reports==reports+1);
    assert(Lifecycle_TrySetMode(0)==TRACKING_OK);healthy();assert(Lifecycle_TryStart());gray_line=0;App_ControlISR();stopped(APP_STOP_GRAY_LOST);
    assert(Tracking_GetState()->mode==0 && OpticalFSM_GetState()->state==OPTICAL_LOST && restore_checks && !mask && timer_stops);
    puts("PASS V-07/14 (FR-16): real App integration, wait, all stops, restart, log faults and interrupt-at-foreground-branch finalization");return 0;
#endif
}
