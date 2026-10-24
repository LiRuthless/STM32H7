"""Replay real dormant wrapper ordering without enabling it in production.

This is a regression check for the publication relocation in committed G4-09.
The human explicitly approved the two synchronization calls on 2026-10-24.
"""
from pathlib import Path
import subprocess
import tempfile
from host_support import build

HARNESS = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "legacy_control.c"
static char trace[32];static unsigned used;
static track_state_t track;
static gray_state_t gray;
static uint16_t accumulated;
static app_stop_reason_t stopped;
static uint8_t observed_running,observed_reason;
static void emit(char c){trace[used++]=c;trace[used]=0;}
void Track_ReadAdc(void){emit('T');}
void Gray_Update(void){emit('G');}
const track_state_t *Track_GetState(void){return &track;}
const gray_state_t *Gray_GetState(void){return &gray;}
void Tracking_AddProtection(uint16_t flags){accumulated=flags;emit('P');}
void Tracking_Observe(uint8_t running,uint8_t reason)
{observed_running=running;observed_reason=reason;emit('O');}
void TuningApplication_Synchronize(void){emit('S');}
void App_RequestStop(app_stop_reason_t reason){stopped=reason;emit('X');}
void Lifecycle_MarkRunning(void){emit('R');}
void Tracking_Execute(void){emit('E');}
float PID_CalculateAngle(int16_t t,float a,float g){(void)t;(void)a;(void)g;assert(0);return 0;}
void Motor_DistributeSpeed(int16_t output){(void)output;assert(0);}
static void run(uint8_t acquisition,uint8_t calibration,uint8_t line,uint16_t sum,
    uint16_t expected_flags,app_stop_reason_t expected_reason)
{
    used=0;trace[0]=0;stopped=APP_STOP_NONE;accumulated=0;
    memset(&track,0,sizeof track);memset(&gray,0,sizeof gray);
    track.filtered[0]=sum;gray.acquisition_status=acquisition;
    gray.calibration_valid=calibration;gray.line_valid=line;
    Control_UpdateLegacy();
    assert(accumulated==expected_flags);
    if(expected_flags)
    {
        if(strcmp(trace,"TGPOSX"))fprintf(stderr,"failure trace=%s expected=TGPOSX\n",trace);
        assert(!strcmp(trace,"TGPOSX"));
        assert(stopped==expected_reason&&!observed_running&&observed_reason==expected_reason);
    }
    else
    {
        if(strcmp(trace,"TGPOSRE"))fprintf(stderr,"success trace=%s expected=TGPOSRE\n",trace);
        assert(!strcmp(trace,"TGPOSRE"));
        assert(stopped==APP_STOP_NONE&&observed_running&&!observed_reason);
    }
}
int main(void)
{
    run(0,1,1,301,0,APP_STOP_NONE);
    run(1,1,1,301,TRACKING_PROTECT_ACQUIRE,APP_STOP_GRAY_ACQUIRE);
    run(0,0,1,301,TRACKING_PROTECT_CALIBRATION,APP_STOP_GRAY_ACQUIRE);
    run(0,1,0,301,TRACKING_PROTECT_GRAY_LOST,APP_STOP_GRAY_LOST);
    run(0,1,1,300,TRACKING_PROTECT_EM_LOST,APP_STOP_OFF_TRACK);
    run(1,0,0,300,TRACKING_PROTECT_ACQUIRE|TRACKING_PROTECT_CALIBRATION
        |TRACKING_PROTECT_GRAY_LOST|TRACKING_PROTECT_EM_LOST,APP_STOP_GRAY_ACQUIRE);
    puts("PASS G4 legacy wrapper: synchronize after Observe before stop/running/execute; protection priority preserved");
}
'''


def main():
    with tempfile.TemporaryDirectory(prefix='g4_legacy_') as directory:
        work = Path(directory)
        (work/'bsp.h').write_text('#include <stdint.h>\n', encoding='utf-8')
        (work/'test.c').write_text(HARNESS, encoding='utf-8')
        binary = build(work, [work/'test.c'])
        subprocess.run([str(binary)], check=True)


if __name__ == '__main__':
    main()
