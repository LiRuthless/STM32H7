#include <assert.h>
#include <string.h>

#include <stdint.h>
#include <stdio.h>
#include "bsp.h"
#include "pid.h"
#include "legacy_pid.h"
#include "legacy_control.h"
#include "track_sensor.h"
#include "imu_proc.h"
#include "motor.h"
static uint16_t adc_input[4];
static int16_t gyro_input[3];
static int16_t encoder_input[2];
uint16_t BSP_ADC_Read(bsp_adc_channel_t ch) { return adc_input[ch]; }
void BSP_ADC_Init(void) {}
int16_t BSP_Sampler_ConsumeEncL(void) { int16_t v = encoder_input[0]; encoder_input[0] = 0; return v; }
int16_t BSP_Sampler_ConsumeEncR(void) { int16_t v = encoder_input[1]; encoder_input[1] = 0; return v; }
void BSP_Encoder_Init(void) {}
void BSP_PWM_Init(void) {}
void BSP_PWM_SetDirection(int channel,uint8_t high){HAL_GPIO_WritePin(channel==BSP_PWM_MOTOR_L?MOTOR_L_DIR_GPIO_Port:MOTOR_R_DIR_GPIO_Port,channel==BSP_PWM_MOTOR_L?MOTOR_L_DIR_Pin:MOTOR_R_DIR_Pin,high?GPIO_PIN_SET:GPIO_PIN_RESET);}
void BSP_PWM_SetDuty(int channel, uint32_t duty) { (void)channel; (void)duty; }
void HAL_GPIO_WritePin(void *port, uint16_t pin, int state)
{ (void)port; (void)pin; (void)state; }
uint8_t BSP_IMU660RB_Init(void) { return 0; }
void BSP_IMU660RB_GetAcc(int16_t *x, int16_t *y, int16_t *z)
{ *x = 0; *y = 0; *z = 0; }
void BSP_IMU660RB_GetGyro(int16_t *x, int16_t *y, int16_t *z)
{ *x = gyro_input[0]; *y = gyro_input[1]; *z = gyro_input[2]; }
void BSP_Sampler_GetGyroRaw(int16_t *x, int16_t *y, int16_t *z)
{ BSP_IMU660RB_GetGyro(x, y, z); }

static void set_wheels(int16_t tl, int16_t tr, int16_t ml, int16_t mr)
{
#ifdef REFACTORED
    Motor_SetTargets(tl, tr);
#else
    target_speed_L = tl; target_speed_R = tr;
#endif
    encoder_input[0] = ml; encoder_input[1] = mr;
    Motor_ReadEncoder();
}


#include "parameter_store.c"
#include "parameter_application.h"
#include "parameter_bank.h"
#include "lifecycle.h"
#include "tracking.h"
#include "menu.h"
#include "bsp_key.c"
#include "gray_sensor.h"
#include "app.h"
#include <assert.h>
#include <string.h>
uint8_t g_app_uart_tx_buffer[32],g_app_uart_rx_buffer[32]; volatile uint8_t g_app_run_flag,g_app_start_flag,g_app_key_flag;
int32_t g_app_control_ticks;
uint16_t g_app_dl1b_distance_mm=8192;





static gray_state_t gray;
static uint8_t flash[4096],read_error,write_error;
static unsigned writes,now,key_level;
static char rx[32],last_message[128];
int16_t g_app_battery_filtered;
int16_t Battery_GetFiltered(void){return g_app_battery_filtered;}
void BSP_LCD_Clear(uint16_t c){(void)c;}
void BSP_LCD_ShowString(uint16_t x,uint16_t y,const char*s,uint16_t a,uint16_t b)
{(void)x;(void)y;(void)s;(void)a;(void)b;}
void BSP_LCD_ShowInt(uint16_t x,uint16_t y,int32_t v,uint8_t w,uint16_t a,uint16_t b)
{(void)x;(void)y;(void)v;(void)w;(void)a;(void)b;}
void BSP_LCD_ShowFloat(uint16_t x,uint16_t y,float v,uint8_t i,uint8_t d,uint16_t a,uint16_t b)
{(void)x;(void)y;(void)v;(void)i;(void)d;(void)a;(void)b;}
const gray_state_t *Gray_GetState(void){return &gray;}
void Gray_Update(void){}
void App_RequestStop(app_stop_reason_t reason){(void)reason;}
uint8_t App_IsStopped(void){return Lifecycle_IsStopped();}
uint8_t Safety_IsStopPending(void){return 0;}
float App_AbsFloat(float x){return x<0?-x:x;}
uint32_t HAL_GetTick(void){return now;}
uint32_t BSP_Time_GetTick(void){return HAL_GetTick();}
int HAL_GPIO_ReadPin(void*p,uint16_t n){(void)p;(void)n;return key_level;}
void BSP_UART_WriteString(const char*s){snprintf(last_message,sizeof last_message,"%s",s);}
uint16_t BSP_UART_Read(uint8_t*b,uint16_t n){unsigned len=strlen(rx);assert(len<=n);memcpy(b,rx,len);rx[0]=0;return len;}
uint8_t BSP_Flash_Read(uint32_t off,uint8_t*b,uint32_t n){if(read_error)return 1;memcpy(b,flash+off,n);return 0;}
uint8_t BSP_Flash_Write(uint32_t off,const uint8_t*b,uint32_t n){++writes;if(write_error)return 1;memcpy(flash+off,b,n);return 0;}
uint8_t Datalog_Start(void){return 1;}
void Datalog_Stop(void){}
void Datalog_Dump(void){}
uint8_t Datalog_Clear(void){return 0;}
uint32_t BSP_Time_EnterCritical(void){return 0;}
void BSP_Time_ExitCritical(uint32_t x){(void)x;}
void Menu_HandleKey(uint8_t);
void Wireless_Adjust(void);
int main(void)
{
    param_image_v1_t old1={0};param_image_v2_t old2={0};
    Motor_InitEncoder();set_wheels(0,0,0,0);
    old1.magic=PARAM_MAGIC;old1.version=1;old1.page=11;old1.arrow=3;
    old1.kp_v=37;old1.ki_v=0.55f;old1.kp_x_low=7;old1.k2p_x_low=0.123f;old1.kd_x_low=19;
    old1.base_speed_low=345;old1.fan_duty_low=1700;
    old1.kp_x_high=8;old1.k2p_x_high=0.321f;old1.kd_x_high=21;old1.base_speed_high=456;old1.fan_duty_high=1800;
    old1.crc16=Param_CalculateCrcV1(&old1);assert(old1.crc16==59967);memcpy(flash,&old1,sizeof old1);
    assert(Param_Load() && writes==1 && s_param.version==3 && sizeof s_param==88);
    assert(memcmp(flash+8,((uint8_t*)&old1)+8,44)==0);
    assert(Param_GetFan(0,1)==1100 && Param_GetDirection(0,1)->base_speed==456);
    assert(Param_GetDirection(1,0)->kp==2 && Param_GetDirection(1,1)->base_speed==100);
    memcpy(&old2,&old1,sizeof old1);old2.version=2;old2.fan_duty_idle_low=1200;old2.fan_duty_idle_high=1300;
    old2.crc16=Param_CalculateCrcV2(&old2);assert(old2.crc16==47062);memcpy(flash,&old2,sizeof old2);
    assert(Param_Load() && memcmp(flash+8,((uint8_t*)&old2)+8,48)==0);
    param_direction_t p={11,0.5f,12,222};
    assert(Param_SetDirection(1,0,&p));assert(Param_GetDirection(0,0)->kp==7);
    assert(!Param_SetDirection(2,0,&p) && !Param_GetDirection(0,2));
    assert(Lifecycle_TrySetMode(0)==TRACKING_OK && Tracking_GetState()->mode==0);
    assert(s_param.optical_kp_low==11 && s_param.kp_x_low==7 && s_param.crc16==Param_CalculateCrcV3(&s_param));
    assert(Param_Load() && Param_GetDirection(1,0)->kp==11 && Param_GetDirection(0,0)->kp==7);
    strcpy(rx,"xp1234\n");Wireless_Adjust();assert(Param_GetDirection(0,0)->kp>12.339f && Param_GetDirection(0,0)->kp<12.341f);
    assert(strcmp(last_message,"xp=1234\r\n")==0 && Param_GetDirection(1,0)->kp==11);
    unsigned before=writes;now=2999;Param_ServiceSave();assert(writes==before);now=3000;Param_ServiceSave();assert(writes==before+1);
    Menu_HandleKey(MENU_KEY_ADJUST2);Menu_RestorePosition(Menu_GetPage(),1);float kp=Param_GetDirection(0,1)->kp;Menu_HandleKey(MENU_KEY_RIGHT);
    assert(Param_GetDirection(0,1)->kp>kp && Param_GetDirection(1,1)->kp==2);
    Lifecycle_TestSetRunState(0,1,1,1);assert(Lifecycle_TrySetMode(1)==TRACKING_BUSY);Lifecycle_TestSetRunState(0,0,0,0);
    write_error=1;tracking_state_t saved=*Tracking_GetState();
    assert(Lifecycle_TrySetMode(1)==TRACKING_SAVE_FAILED && memcmp(&saved,Tracking_GetState(),sizeof saved)==0);
    write_error=0;read_error=1;assert(!Param_Load() && !Param_TryFlushPending());read_error=0;
    memset(flash,0,sizeof flash);assert(Param_Load());flash[4]=99;assert(Param_Load() && s_param.version==3);flash[7]^=1;assert(Param_Load());
    /* Mode validation precedes all writes; same-mode requests still reset completed history. */
    tracking_state_t mode_before=*Tracking_GetState();unsigned mode_writes=writes;
    assert(Lifecycle_TrySetMode(2)==TRACKING_ARG);
    assert(writes==mode_writes && memcmp(&mode_before,Tracking_GetState(),sizeof mode_before)==0);
    Tracking_CompleteControl();assert(Tracking_GetState()->control_sequence==1);
    uint8_t same_mode=Tracking_GetState()->mode;
    assert(Lifecycle_TrySetMode(same_mode)==TRACKING_OK);
    assert(Tracking_GetState()->mode==same_mode && Tracking_GetState()->control_sequence==0);
    assert(strcmp(last_message,same_mode?"track mode: optical\r\n":"track mode: em\r\n")==0);
    key_level=1;now=0;BSP_Key_Init();now=5000;assert(BSP_Key_PollEvent()==BSP_KEY_NONE);
    key_level=0;BSP_Key_PollEvent();now+=29;assert(!BSP_Key_PollEvent());now++;assert(!BSP_Key_PollEvent());
    key_level=1;BSP_Key_PollEvent();now+=29;assert(!BSP_Key_PollEvent());now++;assert(!BSP_Key_PollEvent());
    now+=999;assert(!BSP_Key_PollEvent());key_level=0;assert(!BSP_Key_PollEvent());now+=30;assert(BSP_Key_PollEvent()==BSP_KEY_START);
    key_level=1;BSP_Key_PollEvent();now+=30;BSP_Key_PollEvent();now+=1000;assert(BSP_Key_PollEvent()==BSP_KEY_TOGGLE);
    now+=5000;assert(!BSP_Key_PollEvent());key_level=0;BSP_Key_PollEvent();now+=30;assert(!BSP_Key_PollEvent());
    key_level=1;BSP_Key_PollEvent();now+=30;BSP_Key_PollEvent();now+=1001;assert(BSP_Key_PollEvent()==BSP_KEY_TOGGLE);
    BSP_Key_RequireRelease();key_level=1;now+=100;assert(!BSP_Key_PollEvent());
    puts("PASS V-08/11/12: real key, four banks, menu/serial route, 3s save, v1/v2/v3 migrations and save failure gating");
    return 0;
}

#include "tuning.h"
void Tuning_Init(void){}
void Tuning_SelectControl(uint8_t ordinary,uint8_t allowed,const pid_track_gains_t*g,int16_t base,const tuning_context_t*c)
{(void)ordinary;(void)allowed;(void)g;(void)base;(void)c;}
uint8_t Tuning_GetControl(tuning_values_t*v){memset(v,0,sizeof *v);return 0;}
void Tuning_CompleteControl(const tuning_context_t*c){(void)c;}
void Tuning_Stop(void){}
void Tuning_FinalizeStop(void){}
void Tuning_Service(void){}
