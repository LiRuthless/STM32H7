"""Exercise real G2 HAL adapters, battery math and safety threshold boundaries."""
from pathlib import Path
import subprocess
import tempfile

from app_fixture import APP
from host_support import ROOT, build

MAIN = r'''
#ifndef TEST_MAIN_H
#define TEST_MAIN_H
#include <stdint.h>
typedef struct {uint32_t ARR;} TIM_TypeDef;
typedef struct {TIM_TypeDef *Instance;} TIM_HandleTypeDef;
typedef struct {uint32_t D2CCIP1R;} test_rcc_t;
typedef struct {uint32_t DEMCR;} test_debug_t;
typedef struct {uint32_t CTRL,CYCCNT;} test_dwt_t;
extern test_rcc_t test_rcc;
extern test_debug_t test_debug;
extern test_dwt_t test_dwt;
#define RCC (&test_rcc)
#define CoreDebug (&test_debug)
#define DWT (&test_dwt)
#define CoreDebug_DEMCR_TRCENA_Msk 1u
#define DWT_CTRL_CYCCNTENA_Msk 1u
#define RCC_D2CCIP1R_SPI45SEL 0x30u
#define RCC_SPI45CLKSOURCE_PLL3 0x20u
#define MODIFY_REG(r,m,v) ((r)=((r)&~(m))|(v))
#define TIM6 ((TIM_TypeDef*)6)
#define TIM7 ((TIM_TypeDef*)7)
#define GPIO_PIN_SET 1
#define GPIO_PIN_RESET 0
#define LED_BLUE_GPIO_Port ((void*)2)
#define LED_BLUE_Pin 3
#define MOTOR_L_DIR_GPIO_Port ((void*)3)
#define MOTOR_L_DIR_Pin 1
#define MOTOR_R_DIR_GPIO_Port ((void*)4)
#define MOTOR_R_DIR_Pin 2
#define TIM_CHANNEL_1 1
#define TIM_CHANNEL_3 3
extern TIM_HandleTypeDef htim6,htim7,htim13,htim4,htim14;
uint32_t HAL_GetTick(void);
void HAL_GPIO_WritePin(void*,uint16_t,int);
void HAL_GPIO_TogglePin(void*,uint16_t);
int HAL_TIM_Base_Start_IT(TIM_HandleTypeDef*);
int HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef*);
int HAL_TIM_PWM_Start(TIM_HandleTypeDef*,unsigned);
uint32_t Test_Mask(void);
void Test_SetMask(uint32_t);
#define __get_PRIMASK() Test_Mask()
#define __disable_irq() Test_SetMask(1)
#define __set_PRIMASK(x) Test_SetMask(x)
#define __DMB() ((void)0)
#define __HAL_TIM_GET_AUTORELOAD(t) ((t)->Instance->ARR)
#define __HAL_TIM_SET_COMPARE(t,ch,v) ((void)(t),(void)(ch),(void)(v))
#endif
'''

HARDWARE = r'''
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "main.h"
#include "bsp_runtime.h"
#include "bsp_time.h"
#include "bsp_pwm.h"
test_rcc_t test_rcc;
test_debug_t test_debug;
test_dwt_t test_dwt;
TIM_HandleTypeDef htim6={TIM6},htim7={TIM7},htim13,htim4,htim14;
static char actions[40];static unsigned used,control,task;
static uint32_t tick,mask;
static void action(char c){actions[used++]=c;actions[used]=0;}
uint32_t HAL_GetTick(void){return tick;}
uint32_t Test_Mask(void){return mask;}
void Test_SetMask(uint32_t m){mask=m;}
int HAL_TIM_Base_Start_IT(TIM_HandleTypeDef*t){action(t==&htim6?'6':'7');return 1;}
int HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef*t){action(t==&htim6?'a':'b');return 1;}
int HAL_TIM_PWM_Start(TIM_HandleTypeDef*t,unsigned ch){(void)t;(void)ch;return 0;}
void HAL_GPIO_WritePin(void*p,uint16_t n,int value)
{
    if(p==LED_BLUE_GPIO_Port){assert(n==LED_BLUE_Pin&&value==GPIO_PIN_SET);action('L');}
    else if(p==MOTOR_L_DIR_GPIO_Port){assert(n==MOTOR_L_DIR_Pin);action(value?'H':'h');}
    else{assert(p==MOTOR_R_DIR_GPIO_Port&&n==MOTOR_R_DIR_Pin);action(value?'R':'r');}
}
void HAL_GPIO_TogglePin(void*p,uint16_t n){assert(p==LED_BLUE_GPIO_Port&&n==LED_BLUE_Pin);action('T');}
void App_ControlISR(void){++control;}
void App_TaskISR(void){++task;}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef*);
int main(void)
{
    test_rcc.D2CCIP1R=0xa5a5ffffu;uint32_t before=test_rcc.D2CCIP1R;
    BSP_Runtime_ConfigureSpiClock();assert(test_rcc.D2CCIP1R==((before&~0x30u)|0x20u));
    BSP_Runtime_StartTimers();BSP_Runtime_StopTimers();
    BSP_Runtime_SetRunLed();BSP_Runtime_ToggleLed();
    BSP_PWM_SetDirection(BSP_PWM_MOTOR_L,1);BSP_PWM_SetDirection(BSP_PWM_MOTOR_L,0);
    BSP_PWM_SetDirection(BSP_PWM_MOTOR_R,0);BSP_PWM_SetDirection(BSP_PWM_MOTOR_R,5);
    BSP_PWM_SetDirection(BSP_PWM_FAN,1);BSP_PWM_SetDirection((bsp_pwm_channel_t)99,1);
    assert(strcmp(actions,"67abLTHhrR")==0);
    TIM_HandleTypeDef other={0};HAL_TIM_PeriodElapsedCallback(&other);
    assert(!control&&!task);HAL_TIM_PeriodElapsedCallback(&htim6);assert(control==1&&!task);
    HAL_TIM_PeriodElapsedCallback(&htim7);assert(control==1&&task==1);
    tick=0xffffffffu;assert(BSP_Time_GetTick()==0xffffffffu);tick=0;assert(!BSP_Time_GetTick());
    puts("PASS G2-V4: real HAL wrappers, exact clock mask, timer/LED/direction order, callbacks, tick wrap");
}
'''

BATTERY = r'''
#include <assert.h>
#include <stdio.h>
#include "bsp_adc.h"
#include "battery.h"
#include "filter.h"
static uint16_t input;
uint16_t BSP_ADC_Read(bsp_adc_channel_t ch){assert(ch==BSP_ADC_VBAT);return input;}
int main(void)
{
    filter_lowpass_t expected;Filter_InitLowpass(&expected,.65f);Battery_Init();
    static const uint16_t values[]={0,300,1220,1300,1000,65535,32768,500};
    for(unsigned i=0;i<sizeof values/sizeof *values;++i)
    {
        input=values[i];Battery_Update();int16_t raw=(int16_t)input;
        int16_t value=(int16_t)Filter_UpdateLowpass(&expected,(float)raw);
        assert(Battery_GetRaw()==raw&&Battery_GetFiltered()==value);
    }
    puts("PASS G2-V5: real battery/filter coefficient, signed narrowing and per-sample truncation");
}
'''

SAFETY = r'''
#include <assert.h>
#include <stdio.h>
#include "app_config.h"
#include "safety.h"
#include "tracking.h"
#include "motor.h"
#include "datalog.h"
#include "scb_link.h"
#include "telemetry.h"
#include "bsp.h"
static int16_t input;static unsigned reads;
int16_t Battery_GetFiltered(void){++reads;return input;}
/* These dependencies must never run in the pure threshold and NONE cases. */
void Tracking_AddProtection(uint16_t f){(void)f;assert(0);}
uint32_t BSP_Time_EnterCritical(void){assert(0);return 0;}
void BSP_Time_ExitCritical(uint32_t m){(void)m;assert(0);}
const tracking_state_t *Tracking_GetState(void){assert(0);return 0;}
void Datalog_SetStopSummary(uint8_t r,uint16_t f){(void)r;(void)f;assert(0);}
void Lifecycle_ClearRunFlags(void){assert(0);}
void Tuning_Stop(void){assert(0);}
void Motor_EmergencyStop(void){assert(0);}
const motor_state_t *Motor_GetState(void){assert(0);return 0;}
void BSP_PWM_SetDuty(bsp_pwm_channel_t ch,uint32_t n){(void)ch;(void)n;assert(0);}
void BSP_Sampler_Stop(void){assert(0);}
void BSP_Runtime_StopTimers(void){assert(0);}
void Tracking_Stopped(uint8_t r){(void)r;assert(0);}
void Telemetry_GetStatus(telemetry_status_t*s){(void)s;assert(0);}
uint32_t BSP_Sampler_GetTick(void){assert(0);return 0;}
uint8_t SCB_QueueEvent(const scb_event_t*e){(void)e;assert(0);return 0;}
uint8_t Lifecycle_IsWatchdogStarted(void){assert(0);return 0;}
void Datalog_Stop(void){assert(0);}
void Tuning_FinalizeStop(void){assert(0);}
const datalog_status_t *Datalog_GetStatus(void){assert(0);return 0;}
uint32_t BSP_UART_WriteString(const char*s){(void)s;assert(0);return 0;}
void BSP_Key_RequireRelease(void){assert(0);}
void BSP_WDT_Feed(void){assert(0);}
static void low(void)
{
    input=1000;
    for(unsigned i=0;i<APP_BATTERY_LOW_COUNT;++i)assert(!Safety_CheckLowVoltage());
    assert(Safety_CheckLowVoltage());
}
int main(void)
{
    Safety_Reset();low();
    input=APP_BATTERY_MIN_VALID;assert(!Safety_CheckLowVoltage());low();
    input=APP_BATTERY_LOW_THRESHOLD;reads=0;assert(!Safety_CheckLowVoltage()&&reads==1);low();
    Safety_Reset();assert(input==1000);reads=0;assert(!Safety_CheckLowVoltage()&&reads==2);
    assert(!Safety_IsStopPending()&&Safety_GetStopReason()==APP_STOP_NONE);
    Safety_RequestStop(APP_STOP_NONE);
    assert(!Safety_IsStopPending()&&Safety_GetStopReason()==APP_STOP_NONE);
    puts("PASS G2-V5: real safety strict threshold/count boundaries, reset, short-circuit read count");
}
'''


def main():
    with tempfile.TemporaryDirectory(prefix="g2_system_") as directory:
        work=Path(directory)
        (work/"main.h").write_text(MAIN,encoding="utf-8")
        (work/"tim.h").write_text('#include "main.h"\n',encoding="utf-8")
        headers=("bsp_adc","bsp_time","bsp_pwm","bsp_runtime","bsp_key","bsp_uart","bsp_sampler","bsp_wdt")
        (work/"bsp.h").write_text(''.join(f'#include "{name}.h"\n' for name in headers),encoding="utf-8")
        suites=(
            ("hardware",HARDWARE,[ROOT/"BSP"/f"bsp_{name}.c" for name in ("runtime","pwm","time")]),
            ("battery",BATTERY,[APP/"battery.c",APP/"filter.c"]),
            ("safety",SAFETY,[APP/"safety.c"]),
        )
        for name,harness,sources in suites:
            path=work/f"{name}.c";path.write_text(harness,encoding="utf-8")
            binary=build(work,[path,*sources],name,flags=("-ffunction-sections","-fdata-sections","-Wl,--gc-sections"))
            subprocess.run([str(binary)],check=True)


if __name__=="__main__":
    main()
