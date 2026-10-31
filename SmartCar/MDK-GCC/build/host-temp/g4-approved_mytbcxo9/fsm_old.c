#include <assert.h>
#include <string.h>

#include <stdint.h>
#include <stdio.h>
#include "bsp.h"
#include "pid.h"
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
    read_encoder();
}


#include "control.h"
#include "roundabout.h"
#include "gray_sensor.h"
#include "electromagnetic_fsm.h"
#include "optical_fsm.h"
#include "app.h"
#include "param.h"
static param_direction_t profile={2,0.008f,15,100};
const param_direction_t *Param_GetDirection(uint8_t m,uint8_t g) {(void)m;(void)g;return &profile;}
uint8_t Param_TryFlushPending(void) {return 1;}
void Param_ApplyCurrentDirection(void) {}
uint8_t App_IsStopped(void) {return 1;}
void BSP_UART_WriteString(const char *s) {(void)s;}
uint32_t BSP_Time_EnterCritical(void) {return 0;}
void BSP_Time_ExitCritical(uint32_t m) {(void)m;}
int32_t time=1001;
volatile uint8_t Run_flag,Start_flag=1,key_flag;
uint16_t dl1b_distance_mm=8192;

int32_t Lifecycle_GetControlTicks(void){return time;}
uint16_t Lifecycle_GetDistanceMm(void){return dl1b_distance_mm;}
uint8_t Lifecycle_IsStarted(void){return Start_flag;}
void Lifecycle_MarkRunning(void){Run_flag=1;}
static gray_state_t gray;
const gray_state_t *Gray_GetState(void) { return &gray; }
void Gray_Update(void) {}
void App_RequestStop(app_stop_reason_t r) { assert(r==APP_STOP_NONE); }
float float_abs(float x) { return x<0 ? -x:x; }
static void step(int a,int b,int c,int d,int left,int right)
{
    adc_input[0]=a;adc_input[1]=b;adc_input[2]=c;adc_input[3]=d;
#ifdef FSM_CURRENT
    read_adc();
    electromagnetic_inputs_t in={Track_GetState()->filtered,dl1b_distance_mm,0,0,1};
    motor_state_t saved_motor=*Motor_GetState(); pid_state_t saved_pid=*PID_GetState();
    ElectromagneticFSM_Update(&in);
    assert(memcmp(&saved_motor,Motor_GetState(),sizeof saved_motor)==0);
    assert(memcmp(&saved_pid,PID_GetState(),sizeof saved_pid)==0);
    const electromagnetic_state_t *e=ElectromagneticFSM_GetState();
    Control_ApplyProfile(&e->configuration); speed_control(e->steering);
    if(e->reset_distance) Motor_ResetDistance();
    if(e->zero_angle) IMU_ZeroAngleX();
    kernel_state=e->kernel_state;roundabout_state=e->roundabout_state;
    track_out=e->output;
    printf("%u %u %d %d %d %d %ld %ld\n",e->kernel_state,e->roundabout_state,e->error,e->output,
        Motor_GetState()->left.target_speed,Motor_GetState()->right.target_speed,
        (long)e->left_distance,(long)e->right_distance);
#else
    whole_test();
    printf("%u %u %d %d %d %d %ld %ld\n",kernel_state,roundabout_state,Track_GetState()->error,track_out,
        Motor_GetState()->left.target_speed,Motor_GetState()->right.target_speed,
        (long)Motor_GetState()->left.distance,(long)Motor_GetState()->right.distance);
#endif
    set_wheels(Motor_GetState()->left.target_speed,Motor_GetState()->right.target_speed,left,right);
#ifdef FSM_CURRENT
    ElectromagneticFSM_Accumulate(Motor_GetState()->left.real_speed,Motor_GetState()->right.real_speed);
#endif
}
static void reset_all(void)
{
    Motor_ResetRunState();PID_ResetAll();Control_ResetRunState();Roundabout_ResetRunState();
#ifdef FSM_CURRENT
    ElectromagneticFSM_Reset();
#endif
}
int main(void)
{
#ifdef FSM_CURRENT
    optical_inputs_t opt={1,1,1,0,1000,30,100,0,{2,0.008f,15,1}};
    OpticalFSM_Reset();OpticalFSM_Update(&opt);assert(OpticalFSM_GetState()->state==OPTICAL_WAIT);
    opt.time=1001;OpticalFSM_Update(&opt);assert(OpticalFSM_GetState()->state==OPTICAL_TRACKING);
    opt.gray_valid=0;OpticalFSM_Update(&opt);assert(OpticalFSM_GetState()->state==OPTICAL_LOST);
    opt.gray_valid=1;OpticalFSM_Update(&opt);assert(OpticalFSM_GetState()->state==OPTICAL_LOST);
    OpticalFSM_Reset();opt.stop_reason=2;OpticalFSM_Update(&opt);assert(OpticalFSM_GetState()->state==OPTICAL_STOPPED);
#endif
    encoder_init();reset_all();
    step(500,1500,1500,500,0,0); /* 十字 */
    step(500,1400,1400,500,0,0); /* 等号不退出 */
    step(500,1399,1400,500,0,0);
    step(1500,1500,1500,1600,7000,7001); /* 同拍预环岛覆盖十字 */
    step(500,100,100,700,0,0); /* >5000但入环先清零，不超时 */
    for(int i=0;i<90;++i) step(500,700,400,700,1000,999);
    reset_all();
    step(1700,700,400,1500,6000,6001); /* 左标志，不启用左环岛 */
    step(500,100,100,700,0,0);
    reset_all();dl1b_distance_mm=99;
    step(1500,700,400,1600,6000,6001); /* cask抑制右入环 */
    step(500,100,100,700,0,0);
#ifdef FSM_CURRENT
    {
        int16_t seed[4]={1500,700,400,1600},low[4]={500,150,150,700};
        electromagnetic_inputs_t in={seed,8192,0,0,1};
        ElectromagneticFSM_Reset();ElectromagneticFSM_Update(&in);
        in.adc=low;ElectromagneticFSM_Update(&in);
        assert(ElectromagneticFSM_GetState()->kernel_state==KERNEL_REISLAND); /* 300不入 */
        low[1]=149;ElectromagneticFSM_Update(&in);
        assert(ElectromagneticFSM_GetState()->roundabout_state==ISLAND_LPREENTER);
        ElectromagneticFSM_Accumulate(7999,7999);ElectromagneticFSM_Update(&in);
        assert(ElectromagneticFSM_GetState()->roundabout_state==ISLAND_LPREENTER);
        ElectromagneticFSM_Accumulate(1,1);ElectromagneticFSM_Update(&in);
        assert(ElectromagneticFSM_GetState()->roundabout_state==ISLAND_TURN_LEFT);
        ElectromagneticFSM_Accumulate(13000,13000);ElectromagneticFSM_Update(&in);
        assert(ElectromagneticFSM_GetState()->roundabout_state==ISLAND_TURN_LEFT);
        ElectromagneticFSM_Accumulate(1,1);ElectromagneticFSM_Update(&in);
        assert(ElectromagneticFSM_GetState()->roundabout_state==ISLAND_IN);
        ElectromagneticFSM_Accumulate(32000,32000);ElectromagneticFSM_Update(&in);
        assert(ElectromagneticFSM_GetState()->roundabout_state==ISLAND_IN);
        ElectromagneticFSM_Accumulate(1,1);ElectromagneticFSM_Update(&in);
        assert(ElectromagneticFSM_GetState()->roundabout_state==ISLAND_OUT);
        ElectromagneticFSM_Accumulate(10000,10000);ElectromagneticFSM_Update(&in);
        assert(ElectromagneticFSM_GetState()->roundabout_state==ISLAND_OUT);
        ElectromagneticFSM_Accumulate(1,1);ElectromagneticFSM_Update(&in);
        assert(ElectromagneticFSM_GetState()->kernel_state==KERNEL_TRACKING);
    }
#endif
    return 0;
}
