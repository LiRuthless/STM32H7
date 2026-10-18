"""Run the real logger, Flash service and gray math against native fault injection.

Calibration fixtures exist only in this temporary host build. Firmware stays -1.
"""
from pathlib import Path
from control_equivalence import current_names
import re
import tempfile,subprocess,shutil,csv,io
ROOT=Path(__file__).resolve().parents[1]
W25=r'''
#include <stdint.h>
#define BSP_W25Q64_TOTAL_SIZE 8388608u
#define BSP_W25Q64_SECTOR_SIZE 4096u
#define BSP_W25Q64_PAGE_SIZE 256u
uint8_t BSP_W25Q64_Init(void);
uint8_t BSP_W25Q64_Read(uint32_t,uint8_t*,uint32_t);
uint8_t BSP_W25Q64_WritePage(uint32_t,const uint8_t*,uint16_t);
uint8_t BSP_W25Q64_EraseSector(uint32_t);
'''
BSP=r'''
#include "bsp_gray.h"
#include "bsp_flash.h"
uint32_t BSP_Time_EnterCritical(void);
void BSP_Time_ExitCritical(uint32_t);
void BSP_Time_MemoryBarrier(void);
void BSP_WDT_Feed(void);
uint32_t BSP_Sampler_GetTick(void);
uint32_t HAL_GetTick(void);
int16_t BSP_Sampler_GetEncL_1ms(void);
int16_t BSP_Sampler_GetEncR_1ms(void);
void BSP_Sampler_GetGyroRaw(int16_t*,int16_t*,int16_t*);
void BSP_UART_WriteString(const char*);
uint32_t BSP_UART_Write(const uint8_t*,uint16_t);
'''
HARNESS=r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "datalog.c"
static uint8_t flash[8388608];
uint32_t HAL_GetTick(void){return 0;}
void SCB_ResetSession(uint32_t now){(void)now;}
static int unavailable,write_fail,read_fail,corrupt,inject;
static uint32_t fail_read_address=0xffffffffu;
static unsigned page_calls,erase_calls,stop_calls,tick;
static char output[20000];static unsigned output_len;
static tracking_state_t tracking={.mode=1};
static track_state_t track;
static electromagnetic_state_t em;
static optical_state_t optical={.state=OPTICAL_TRACKING};
static pid_state_t pid={.KP_v=20,.KI_v=.75f};
static param_direction_t direction[2]={{7,.1f,8,245},{2,.008f,15,100}};
int16_t battery_filt=1300;uint16_t dl1b_distance_mm=8192;
uint32_t BSP_Time_EnterCritical(void){return 0;}
void BSP_Time_ExitCritical(uint32_t x){(void)x;}
void BSP_Time_MemoryBarrier(void){}
void BSP_WDT_Feed(void){}
uint32_t BSP_Sampler_GetTick(void){return ++tick;}
int16_t BSP_Sampler_GetEncL_1ms(void){return -32768;}
int16_t BSP_Sampler_GetEncR_1ms(void){return 32767;}
void BSP_Sampler_GetGyroRaw(int16_t*x,int16_t*y,int16_t*z){*x=-32768;*y=32767;*z=0;}
void BSP_UART_WriteString(const char*s){unsigned n=strlen(s);assert(output_len+n<sizeof output);memcpy(output+output_len,s,n+1);output_len+=n;}
uint32_t BSP_UART_Write(const uint8_t*s,uint16_t n){assert(n<640);assert(output_len+n<sizeof output);memcpy(output+output_len,s,n);output_len+=n;output[output_len]=0;return 0;}
const tracking_state_t *Tracking_GetState(void){return &tracking;}
void Tracking_AddProtection(uint16_t f){tracking.protection_flags|=f;}
const track_state_t *Track_GetState(void){return &track;}
const electromagnetic_state_t *ElectromagneticFSM_GetState(void){return &em;}
const optical_state_t *OpticalFSM_GetState(void){return &optical;}
const pid_state_t *PID_GetState(void){return &pid;}
const param_direction_t *Param_GetDirection(uint8_t m,uint8_t g){assert(m<2&&g<2);return &direction[m];}
int16_t Param_GetFan(uint8_t g,uint8_t idle){assert(g<2);return idle?1100:1600;}
void App_RequestStop(app_stop_reason_t reason){++stop_calls;Datalog_SetStopSummary(reason,tracking.protection_flags);}
void BSP_Gray_Init(void){}
void BSP_Gray_ResetSequence(void){}
bsp_gray_status_t BSP_Gray_ReadFrame(bsp_gray_frame_t*out){static unsigned seq;for(unsigned i=0;i<16;++i)out->raw[i]=(i&1)?0:1000;out->sequence=++seq;out->scan_cycles=100;return BSP_GRAY_OK;}
uint8_t BSP_W25Q64_Init(void){return unavailable;}
uint8_t BSP_W25Q64_Read(uint32_t a,uint8_t*b,uint32_t n){assert(a+n<=8388608);if(read_fail||a==fail_read_address)return 1;memcpy(b,flash+a,n);if(corrupt&&a==0)b[0]^=1;return 0;}
uint8_t BSP_W25Q64_EraseSector(uint32_t a){assert(a%4096==0 && a<8384512);++erase_calls;memset(flash+a,255,4096);return 0;}
uint8_t BSP_W25Q64_WritePage(uint32_t a,const uint8_t*b,uint16_t n)
{
    assert(a+n<=8384512 && n && a/256==(a+n-1)/256);++page_calls;
    if(inject){inject=0;Datalog_Push();Datalog_SetStopSummary(APP_STOP_LOW_BATTERY,PROTECT_LOW_BATTERY);}
    if(write_fail)return 1;
    memcpy(flash+a,b,n);return 0;
}
static void start(void){write_fail=read_fail=corrupt=unavailable=0;tracking.protection_flags=0;BSP_Flash_Init();assert(!Datalog_Start());Gray_Update();Datalog_PublishControl();Telemetry_BeginRun();}
int main(void)
{
#if defined(TEST_UNCALIBRATED)
    Gray_Init();BSP_Flash_Init();assert(Datalog_Start() && !erase_calls && Datalog_GetStatus()->fault==DATALOG_FAULT_HEADER);return 0;
#elif !DATALOG_ENABLE || DATALOG_DIV != 1
    BSP_Flash_Init();assert(Datalog_Start() && Datalog_GetStatus()->fault==DATALOG_FAULT_CONFIG);return 0;
#else
    Gray_Init();start();assert(sizeof s_buf==98304 && sizeof s_batch==768);
    assert(s_header.crc16==crc16(flash+8,248) && s_header.capacity==87336);
    unsigned calls=page_calls;for(unsigned i=0;i<7;++i)Datalog_Push();Datalog_Service();assert(page_calls==calls);
    Datalog_Push();inject=1;Datalog_Service();assert(page_calls==calls+3 && s_status.accepted==9 && s_status.written==8);
    assert(Datalog_GetStatus()->pending==1 && s_status.reason==APP_STOP_LOW_BATTERY);
    telemetry_status_t upload;Telemetry_GetStatus(&upload);assert(upload.next_index==9 && upload.queued==9);
    Datalog_Stop();assert(s_status.written==9 && !Datalog_GetStatus()->pending);
    for(unsigned i=0;i<9;++i){datalog_record_t r;memcpy(&r,flash+256+96*i,96);assert(r.tick==i+1 && r.gray_sequence==1);}
    start();for(unsigned i=0;i<1050;++i){Datalog_Push();if(i%8==7)Datalog_Service();}Datalog_Stop();assert(s_status.written==1050);
    Telemetry_GetStatus(&upload);assert(upload.queued==256 && upload.dropped && upload.next_index==1050 && !s_status.fault);
    assert(erase_calls>10); /* 包含首扇区、跨页、跨扇区及环绕 */
    start();for(unsigned i=0;i<1023;++i)Datalog_Push();assert(s_status.accepted==1023);
    unsigned stops=stop_calls;Datalog_Push();assert(s_status.fault==DATALOG_FAULT_OVERFLOW && s_status.rejected==1 && stop_calls==stops+1);
    Datalog_Push();assert(s_status.rejected==1);Datalog_Stop();assert(s_status.written==1023);
    start();for(unsigned i=0;i<8;++i)Datalog_Push();write_fail=1;Datalog_Service();calls=page_calls;
    Datalog_Stop();Datalog_Service();assert(page_calls==calls && !s_status.written && s_status.fault==DATALOG_FAULT_WRITE);
    start();for(unsigned i=0;i<87336;++i){Datalog_Push();if(i%8==7)Datalog_Service();}
    Datalog_Stop();assert(s_status.fault==DATALOG_FAULT_FULL && s_status.accepted==87336 && s_status.written==87336);
    assert(256+87336u*96==8384512 && !Datalog_GetStatus()->pending);
    /* 先FULL后尾包写错时仍禁止重试，不能只靠首fault==WRITE判断。 */
    start();s_status.accepted=87335;s_status.written=87335;Datalog_Push();write_fail=1;Datalog_Stop();calls=page_calls;Datalog_Stop();assert(page_calls==calls);
    unavailable=1;BSP_Flash_Init();assert(Datalog_Start() && s_status.fault==DATALOG_FAULT_UNAVAILABLE);
    unavailable=0;BSP_Flash_Init();write_fail=1;assert(Datalog_Start() && s_status.fault==DATALOG_FAULT_HEADER);
    write_fail=0;corrupt=1;assert(Datalog_Start() && s_status.fault==DATALOG_FAULT_HEADER);
    start();tick=0xfffffffeu;tracking.control_sequence=0xffffffffu;Datalog_PublishControl();for(unsigned i=0;i<3;++i)Datalog_Push();Datalog_Stop();
    datalog_header_t h;memcpy(&h,flash,256);h.black[0]=100;h.white[0]=1100;h.crc16=crc16((uint8_t*)&h+8,248);memcpy(flash,&h,256);
    output_len=0;Datalog_Dump();puts("CSV_BEGIN");fputs(output,stdout);puts("CSV_END");
    fail_read_address=352;output_len=0;Datalog_Dump();assert(strstr(output,"#dump_error=read") && strstr(output,"EOF"));fail_read_address=0xffffffffu;
    read_fail=1;output_len=0;Datalog_Dump();assert(strstr(output,"#dump_error=read") && strstr(output,"EOF"));
    read_fail=0;flash[0]=0;output_len=0;Datalog_Dump();assert(strstr(output,"no_valid_session") && !strstr(output,"tick,"));
    uint8_t fault=s_status.fault;Datalog_Clear();assert(s_status.fault==fault && !Datalog_IsActive());
    puts("PASS V-13/14/15: real logger/Flash/gray math, wrap, 87336 capacity, overflow, no retry, header and CSV faults");
    return 0;
#endif
}
'''


def override_define(source, name, expected, replacement):
    pattern = re.compile(r'(?m)^([ \t]*#define[ \t]+' + re.escape(name) + r'[ \t]+)' + str(expected) + r'\b')
    changed, count = pattern.subn(lambda match: match[1] + str(replacement), source)
    assert count == 1, (name, 'fault injection must match once', count)
    actual = re.findall(r'(?m)^[ \t]*#define[ \t]+' + re.escape(name) + r'[ \t]+(\d+)\b', changed)
    assert actual == [str(replacement)], (name, 'fault value not applied', actual)
    return changed

def main():
 with tempfile.TemporaryDirectory(prefix='datalog_v2_') as temp:
    work=Path(temp)
    for name in ('bsp_flash.c','bsp_flash.h','bsp_gray.h','bsp_adc.h'):
        shutil.copyfile(ROOT/'BSP'/name,work/name)
    for name in ('gray_sensor.c','gray_sensor.h','datalog.c','telemetry.c'):
        shutil.copyfile(ROOT/'App'/name,work/name)
    (work/'main.h').write_text('#include <stdint.h>\n',encoding='utf-8')
    (work/'bsp_w25q64.h').write_text(W25,encoding='utf-8')
    (work/'bsp.h').write_text(BSP,encoding='utf-8')
    black=','.join('1000' if i&1 else '0' for i in range(16))
    white=','.join('0' if i&1 else '1000' for i in range(16))
    (work/'gray_calibration.h').write_text(f'static const int16_t s_gray_black[16]={{{black}}};\nstatic const int16_t s_gray_white[16]={{{white}}};\n',encoding='utf-8')
    (work/'test.c').write_text(current_names(HARNESS),encoding='utf-8')
    config=(ROOT/'App/app_config.h').read_text(encoding='utf-8')
    for mode in ('normal','disabled','div2','uncalibrated'):
        changed = override_define(config, 'DATALOG_ENABLE', 1, 0) if mode == 'disabled' else config
        if mode == 'div2':
            changed = override_define(changed, 'DATALOG_DIV', 1, 2)
        (work/'app_config.h').write_text(changed,encoding='utf-8')
        if mode=='uncalibrated':shutil.copyfile(ROOT/'App/gray_calibration.h',work/'gray_calibration.h')
        binary=work/(mode+'.exe')
        subprocess.run(['gcc','-std=c99','-O2','-Wall','-Wextra','-Werror','-Wno-unused-function',
                        *(['-DTEST_UNCALIBRATED'] if mode=='uncalibrated' else []),
                        '-I',str(work),'-I',str(ROOT/'App'),str(work/'test.c'),str(work/'gray_sensor.c'),
                        str(work/'bsp_flash.c'),str(work/'telemetry.c'),str(ROOT/'App/scb_protocol.c'),'-o',str(binary)],check=True)
        result=subprocess.run([str(binary)],check=True,capture_output=True,text=True).stdout
        if mode=='normal':
            content=result.split('CSV_BEGIN\n')[1].split('CSV_END')[0]
            rows=list(csv.reader(io.StringIO(content)))
            header=next(row for row in rows if row and row[0]=='tick')
            data=[row for row in rows if row and row[0].isdigit()]
            assert len(header)==57 and len(data)==3 and all(len(row)==57 for row in data)
            assert int(data[0][32])==900 and int(data[0][33])==1000
            assert content.rstrip().endswith('EOF') and '#status,' in content
            print(result.split('CSV_END\n')[1].strip())
    print('PASS V-14: DATALOG_ENABLE=0 and DATALOG_DIV=2 rejected by real C')
if __name__=='__main__':main()
