"""Replay committed G3 versus split G4 menu, parameter application and legacy UART.

Real menu/filter/parameter/UART C is linked on both sides. Only board IO and
controller state are recorded substitutes; flash bytes and every LCD call are
compared, including the page observed at the instant of a save.
"""
import difflib
from pathlib import Path
import subprocess
import tempfile

from app_fixture import APP, flatten_includes

REPO = Path(__file__).resolve().parents[2]
BASE = '6125495'
BSP = r'''
#ifndef TEST_G4_BSP_H
#define TEST_G4_BSP_H
#include <stdint.h>
typedef enum {BSP_ADC_KEY1,BSP_ADC_KEY2} bsp_adc_channel_t;
#define BSP_LCD_BLACK 0
#define BSP_LCD_WHITE 65535
#define BSP_LCD_RED 63488
uint16_t BSP_ADC_Read(bsp_adc_channel_t ch);
uint32_t BSP_Time_GetTick(void);
uint8_t BSP_Flash_Read(uint32_t offset,uint8_t *data,uint32_t length);
uint8_t BSP_Flash_Write(uint32_t offset,const uint8_t *data,uint32_t length);
uint16_t BSP_UART_Read(uint8_t *data,uint16_t capacity);
void BSP_UART_WriteString(const char *text);
void BSP_LCD_Clear(uint16_t color);
void BSP_LCD_ShowString(uint16_t x,uint16_t y,const char *text,uint16_t fg,uint16_t bg);
void BSP_LCD_ShowInt(uint16_t x,uint16_t y,int32_t value,uint8_t width,uint16_t fg,uint16_t bg);
void BSP_LCD_ShowFloat(uint16_t x,uint16_t y,float value,uint8_t iw,uint8_t dw,uint16_t fg,uint16_t bg);
#endif
'''
HARNESS = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "bsp.h"
#include "menu.h"
#include "pid.h"
#include "motor.h"
#include "track_sensor.h"
#include "imu_proc.h"
#include "wireless.h"
#include "tracking.h"
#ifdef CURRENT
#include "parameter_application.h"
#include "tracking.h"
#include "menu_keys.h"
#else
#include "param.h"
#endif
void Menu_HandleKey(uint8_t key);
uint8_t Menu_ScanKey(void);
static pid_state_t pid;
static motor_state_t motor;
static track_state_t track={.filtered={100,200,300,400},.error=-12,.symmetry_x=7,.symmetry_y=8};
static imu_state_t imu={.gyro={1.25f,-2.5f,3.75f}};
static tracking_state_t tracking={.mode=TRACKING_MODE_OPTICAL};
static uint8_t flash[4096];
static uint32_t tick;
static uint16_t adc[2];
static unsigned reads[2],writes,clears,draws;
static char uart[32];
static uint8_t saved_page[1024];
static uint8_t page(void){
#ifdef CURRENT
return Menu_GetPage();
#else
return g_menu_page;
#endif
}
static uint8_t arrow(void){
#ifdef CURRENT
return Menu_GetArrow();
#else
return g_menu_arrow;
#endif
}
const pid_state_t *PID_GetState(void){return &pid;}
void PID_SetGain(pid_gain_id_t id,float value){
    printf("gain %d %a\n",id,value);
    switch(id){case PID_GAIN_KP_V:pid.kp_v=value;break;case PID_GAIN_KI_V:pid.ki_v=value;break;default:assert(0);}
}
void PID_SetTrackGains(const pid_track_gains_t *g){
    pid.kp_x=g->kp;pid.k2p_x=g->k2p;pid.kd_x=g->kd;pid.k2d_x=g->k2d;
    printf("trackgain %a %a %a %a\n",g->kp,g->k2p,g->kd,g->k2d);
}
const motor_state_t *Motor_GetState(void){return &motor;}
void Motor_SetBaseSpeed(int16_t value){motor.base_speed=value;printf("base %d\n",value);}
void Motor_SetFanDuty(int16_t value){motor.fan_duty=value;printf("fan %d\n",value);}
void Motor_SetFanDutyIdle(int16_t value){motor.fan_duty_idle=value;printf("idle %d\n",value);}
const track_state_t *Track_GetState(void){return &track;}
const imu_state_t *IMU_GetState(void){return &imu;}
const tracking_state_t *Tracking_GetState(void){return &tracking;}
void Tracking_SetGear(uint8_t gear){tracking.gear=gear;printf("gear %u\n",gear);}
int16_t Battery_GetFiltered(void){return 1320;}
uint32_t BSP_Time_GetTick(void){printf("tick %u\n",(unsigned)tick);return tick;}
uint16_t BSP_ADC_Read(bsp_adc_channel_t ch){++reads[ch];printf("adc %u %u\n",ch,adc[ch]);return adc[ch];}
uint8_t BSP_Flash_Read(uint32_t offset,uint8_t *data,uint32_t length){assert(offset+length<=sizeof(flash));memcpy(data,flash+offset,length);return 0;}
uint8_t BSP_Flash_Write(uint32_t offset,const uint8_t *data,uint32_t length){
    assert(offset+length<=sizeof(flash)&&writes<1024);saved_page[writes++]=page();
    printf("save %u %u %u %u ",page(),arrow(),(unsigned)offset,(unsigned)length);
    for(uint32_t i=0;i<length;++i){printf("%02x",data[i]);}puts("");
    memcpy(flash+offset,data,length);return 0;
}
uint16_t BSP_UART_Read(uint8_t *data,uint16_t capacity){size_t n=strlen(uart);assert(n<=capacity);memcpy(data,uart,n);uart[0]=0;return (uint16_t)n;}
void BSP_UART_WriteString(const char *text){printf("uart %s",text);}
void Datalog_Dump(void){puts("dump");}
uint8_t Datalog_Clear(void){puts("logclear");return 0;}
void BSP_LCD_Clear(uint16_t color){++clears;printf("clear %u\n",color);}
void BSP_LCD_ShowString(uint16_t x,uint16_t y,const char *text,uint16_t fg,uint16_t bg){++draws;printf("str %u %u %s %u %u\n",x,y,text,fg,bg);}
void BSP_LCD_ShowInt(uint16_t x,uint16_t y,int32_t value,uint8_t width,uint16_t fg,uint16_t bg){++draws;printf("int %u %u %d %u %u %u\n",x,y,value,width,fg,bg);}
void BSP_LCD_ShowFloat(uint16_t x,uint16_t y,float value,uint8_t iw,uint8_t dw,uint16_t fg,uint16_t bg){++draws;printf("float %u %u %a %u %u %u %u\n",x,y,value,iw,dw,fg,bg);}
static void update(uint16_t a,uint16_t b){
    adc[0]=a;adc[1]=b;unsigned l=reads[0],r=reads[1];
    printf("frame %u %u\n",a,b);Menu_Update();
    assert(reads[0]-l==3&&reads[1]-r==3);printf("state %u %u %u %a %a\n",page(),arrow(),tracking.gear,pid.kp_x,pid.ki_v);
}
static void press(uint16_t a,uint16_t b){update(a,b);update(0,0);}
static void serial(const char *text){strcpy(uart,text);Wireless_Adjust();}
static void mirror(float expected_kp,float expected_ki){float kp,ki;Param_GetSpeedGains(&kp,&ki);assert(fabsf(kp-expected_kp)<.000001f&&fabsf(ki-expected_ki)<.000001f);printf("mirror %a %a active %a %a\n",kp,ki,pid.kp_v,pid.ki_v);}
int main(void){
    memset(flash,255,sizeof(flash));assert(Param_Load());mirror(20,.75f);
    unsigned w=writes;serial("vp15\n");mirror(20,.75f);assert(fabsf(pid.kp_v-.15f)<.000001f);
    tick=2999;Param_ServiceSave();assert(writes==w);mirror(20,.75f);
    tick=3000;Param_ServiceSave();assert(writes==w+1);mirror(.15f,.75f);
    serial("vi123\n");mirror(.15f,.75f);assert(fabsf(pid.ki_v-1.23f)<.000001f);
    tick=5999;Param_ServiceSave();assert(writes==w+1);tick=6000;Param_ServiceSave();mirror(.15f,1.23f);
    const uint16_t edges[]={0,299,300,399,400,899,900,999,1000,1499,1500,1599,1600,1999,2000,2099,2100,2499,2500,2599,2600,3099,3100,4095};
    for(unsigned side=0;side<2;++side)for(unsigned i=0;i<sizeof(edges)/sizeof(edges[0]);++i){
        adc[0]=side?0:edges[i];adc[1]=side?edges[i]:0;unsigned l=reads[0],r=reads[1];uint8_t k=Menu_ScanKey();assert(reads[0]-l==3&&reads[1]-r==3);printf("scan %u %u %u\n",side,edges[i],k);
    }
    adc[0]=500;adc[1]=1200;assert(Menu_ScanKey()==MENU_KEY_ADJUST1);
    update(0,0);press(0,1200);assert(page()==MENU_PAGE_ADJUST1);
    float initial=pid.kp_x;update(2900,0);assert(pid.kp_x==initial+.001f);
    update(2900,0);assert(pid.kp_x==initial+.001f);
    w=writes;update(0,2900);assert(page()==MENU_PAGE_HOME&&writes==w+1&&saved_page[w]==MENU_PAGE_ADJUST1);
    update(0,0);assert(writes==w+2&&saved_page[w+1]==MENU_PAGE_HOME);
    unsigned c=clears,d=draws;update(0,500);assert(clears==c+1&&draws==d);
    update(0,500);assert(clears==c+2&&draws>d);update(0,0);
    for(unsigned mode=0;mode<2;++mode)for(unsigned gear=0;gear<2;++gear){
        tracking.mode=(uint8_t)mode;press(0,gear?1800:1200);
        for(unsigned i=0;i<4;++i){press(1200,0);}assert(arrow()==5);
        Param_SetFan((uint8_t)gear,0,0);press(2300,0);assert(motor.fan_duty==0);
        press(1800,0);Param_SetFan((uint8_t)gear,1,10000);press(2900,0);assert(motor.fan_duty_idle==10000);
        press(2300,0);assert(motor.fan_duty_idle==9900);press(0,2900);
        press(1800,0);assert(page()==MENU_PAGE_ADC_ERR);press(0,2900);
        press(1200,0);press(1800,0);assert(page()==MENU_PAGE_SPD_DIS);press(0,2900);
        press(1200,0);press(1200,0);press(1800,0);assert(page()==MENU_PAGE_GYRO);press(0,2900);
    }
    puts("complete");return 0;
}
'''


def main():
    with tempfile.TemporaryDirectory(prefix='g4_hmi_') as temporary:
        work = Path(temporary)
        (work/'bsp.h').write_text(BSP, encoding='utf-8')
        (work/'main.c').write_text(HARNESS, encoding='utf-8')
        old = work/'old'
        old.mkdir()
        names = subprocess.check_output(['git', 'ls-tree', '-r', '--name-only', BASE, 'SmartCar/App'], cwd=REPO, text=True).splitlines()
        for name in names:
            if name.endswith('.h') or Path(name).name in ('menu.c', 'param.c', 'wireless.c', 'filter.c'):
                source = subprocess.check_output(['git', 'show', BASE+':'+name], cwd=REPO)
                (old/Path(name).name).write_bytes(flatten_includes(source))
        outputs = []
        for current in (False, True):
            source = APP if current else old
            files = ['menu', 'wireless', 'filter']
            files += ['menu_keys', 'menu_render', 'parameter_bank', 'parameter_store', 'parameter_application'] if current else ['param']
            binary = work/('current.exe' if current else 'old.exe')
            result = subprocess.run(['gcc', '-std=c99', '-O2', '-Wall', '-Wextra', '-Werror',
                                     '-D__USE_MINGW_ANSI_STDIO=1', *(['-DCURRENT'] if current else []),
                                     '-I', str(work), '-I', str(source), str(work/'main.c'),
                                     *[str(source/(name+'.c')) for name in files], '-o', str(binary)],
                                    capture_output=True, text=True)
            assert result.returncode == 0, result.stdout+result.stderr
            result = subprocess.run([str(binary)], capture_output=True, text=True)
            assert result.returncode == 0, result.stdout[-4000:]+result.stderr
            outputs.append(result.stdout)
        assert outputs[0] == outputs[1], ''.join(difflib.unified_diff(outputs[0].splitlines(True), outputs[1].splitlines(True)))[:12000]
    print('PASS G4 HMI: real G3/current ADC edge/second-key override, all LCD calls, BACK save page, held/released edits, RST next-frame clear, two modes/gears, UART mirror/active PI and 3000ms flash bytes match')


if __name__ == '__main__':
    main()
