"""Compare retained dormant entry effects against committed G2, plus pure API boundary."""
from pathlib import Path
import difflib
import os
import subprocess
import tempfile
from app_fixture import APP, flatten_includes
from control_equivalence import STUB_BSP

ROOT = Path(__file__).resolve().parents[2]
BASE = 'e4c7f9b'
HARNESS = r'''
#include <assert.h>
#include <stdio.h>
#include "pid.h"
#include "motor.h"
#include "track_sensor.h"
#include "imu_proc.h"
#ifdef CURRENT
#include "legacy_pid.h"
#include "legacy_control.h"
#include "tracking.h"
#include "gray_sensor.h"
#include "app.h"
static gray_state_t gray;
void Track_ReadAdc(void){}
void Gray_Update(void){}
const gray_state_t *Gray_GetState(void){return &gray;}
void Tracking_AddProtection(uint16_t flags){(void)flags;}
void Tracking_Observe(uint8_t running,uint8_t reason){(void)running;(void)reason;}
void TuningApplication_Synchronize(void){assert(0);}
void Tracking_Execute(void){}
void Lifecycle_MarkRunning(void){}
void App_RequestStop(app_stop_reason_t reason){(void)reason;}
#endif
static motor_state_t motor;
static track_state_t track;
static unsigned calls;
static int16_t distributed;
static int16_t sample=143;
const motor_state_t *Motor_GetState(void){return &motor;}
const track_state_t *Track_GetState(void){return &track;}
void Motor_DistributeSpeed(int16_t value){++calls;distributed=value;printf("call %u %d %a %a\n",calls,value,PID_GetState()->angle_err,PID_GetState()->angle_out);}
uint8_t BSP_IMU660RB_Init(void){return 0;}
void BSP_IMU660RB_GetGyro(int16_t*x,int16_t*y,int16_t*z){*x=sample;*y=-sample;*z=sample/2;}
void BSP_IMU660RB_GetAcc(int16_t*x,int16_t*y,int16_t*z){*x=sample;*y=sample*2;*z=-sample;}
void BSP_Sampler_GetGyroRaw(int16_t*x,int16_t*y,int16_t*z){BSP_IMU660RB_GetGyro(x,y,z);}
int main(void)
{
    PID_SetGain(PID_GAIN_KP_V,2.25f);PID_SetGain(PID_GAIN_KI_V,.75f);
    const pid_angle_gains_t gains={2.1f,1.2f,.3f};PID_SetAngleGains(&gains);
    IMU_Init();
    for(int i=0;i<8;++i){
        sample=(int16_t)(143+i*17);IMU_UpdateGyroAngle();
        if(i==3)PID_ResetSpeed();
        if(i==5)PID_ResetAll();
        motor.left.target_speed=(int16_t)(40-i*13);motor.left.real_speed=(int16_t)(i*3);
        motor.right.target_speed=(int16_t)(i*17-25);motor.right.real_speed=(int16_t)(-i*2);
#ifdef CURRENT
        PID_UpdateAngle((int16_t)(i-3),IMU_GetState()->angle[0],IMU_GetState()->gyro[0]);
        int16_t l=PID_UpdateIncrementalLeft(motor.left.target_speed,motor.left.real_speed);
        int16_t r=PID_UpdateIncrementalRight(motor.right.target_speed,motor.right.real_speed);
#else
        PID_UpdateAngle((int16_t)(i-3));
        int16_t l=PID_UpdateIncrementalLeft(),r=PID_UpdateIncrementalRight();
#endif
        printf("legacy %d %d %d %u %d\n",i,l,r,calls,distributed);
    }
    sample=286;IMU_CalibrateAccel();
    for(int i=0;i<5;++i){
        sample=(int16_t)(100+i*31);IMU_UpdateGyroAngle();IMU_UpdateAccelVelocity();
        const imu_state_t*s=IMU_GetState();
        printf("acc %d %a %a %a %a %a %a\n",i,s->accel[0],s->accel[1],s->accel[2],s->velocity[0],s->velocity[1],s->velocity[2]);
        if(i==1)IMU_ResetRunState();
    }
#ifdef CURRENT
    unsigned before=calls;
    float output=PID_CalculateAngle(4,1.5f,-2.0f);
    assert(calls==before&&output==PID_GetState()->angle_out);
    assert(PID_GetState()->angle_err==2.5f);
#endif
    return 0;
}
'''


def main():
    with tempfile.TemporaryDirectory(prefix='g3_control_') as directory:
        work=Path(directory)
        (work/'bsp.h').write_text(STUB_BSP,encoding='utf-8')
        (work/'main.c').write_text(HARNESS,encoding='utf-8')
        outputs=[]
        for current in (False,True):
            source=APP if current else work/'old'
            if not current:
                source.mkdir()
                for relative in ('control/pid.c','control/pid.h','control/motor.h',
                                 'sensing/track_sensor.h','sensing/imu_proc.c','sensing/imu_proc.h',
                                 'sensing/filter.c','sensing/filter.h','app_config.h'):
                    data=subprocess.run(['git','show',f'{BASE}:SmartCar/App/{relative}'],cwd=ROOT,capture_output=True,check=True).stdout
                    (source/Path(relative).name).write_bytes(flatten_includes(data))
            files=['pid','imu_proc','filter']
            if current:files+=['direction_controller','angle_controller','speed_controller','legacy_pid','legacy_control','legacy_imu']
            binary=work/('current.exe' if current else 'old.exe')
            subprocess.run(['gcc','-std=c99','-O2','-Wall','-Wextra','-Werror','-D__USE_MINGW_ANSI_STDIO=1','-ffunction-sections','-Wl,--gc-sections',
                            *(['-DCURRENT'] if current else []),'-I',str(work),'-I',str(source),'-I',str(APP),str(work/'main.c'),
                            *[str(source/(name+'.c')) for name in files],'-o',str(binary)],check=True)
            outputs.append(subprocess.run([str(binary)],capture_output=True,text=True,check=True).stdout)
        assert outputs[0]==outputs[1], ''.join(difflib.unified_diff(outputs[0].splitlines(True),outputs[1].splitlines(True)))
    print('PASS G3: G2/current angle wrapper call count/arguments/state, dormant L/R history across reset, same-owner IMU calibration/update/reset match; pure angle has no motor call')


if __name__=='__main__':main()
