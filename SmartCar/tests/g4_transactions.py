"""Exercise real transaction owner boundary cases without protocol or BSP behavior."""
from pathlib import Path
import subprocess
import tempfile
from app_fixture import APP
from host_support import build

HARNESS = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "tuning.c"
static pid_state_t active_pid;
static motor_state_t active_motor;
static unsigned depth;
uint32_t BSP_Time_EnterCritical(void){return depth++;}
void BSP_Time_ExitCritical(uint32_t previous){depth=previous;}
const pid_state_t *PID_GetState(void){return &active_pid;}
void PID_SetGain(pid_gain_id_t id,float value)
{assert(id==PID_GAIN_KP_V||id==PID_GAIN_KI_V);if(id==PID_GAIN_KP_V)active_pid.kp_v=value;else active_pid.ki_v=value;}
const motor_state_t *Motor_GetState(void){return &active_motor;}
void Motor_SetFanDuty(int16_t value){active_motor.fan_duty=value;}
void Motor_SetFanDutyIdle(int16_t value){active_motor.fan_duty_idle=value;}
int main(void)
{
    tuning_context_t context={0,1,0,0,1,0,1001,123,7};
    tuning_request_t request={0,0,{3,.002f,4,11,.4f,123,1900,1100}};
    tuning_reply_t reply;
    tuning_event_t event={0};
    tuning_values_t values;
    uint32_t revision,id;
    pid_track_gains_t original={2,.008f,15,1};
    active_pid.kp_v=20;active_pid.ki_v=.75f;
    active_motor.fan_duty=1600;active_motor.fan_duty_idle=1100;
    Tuning_Init();Tuning_Switch(1,&context,&reply);assert(reply.code==TUNING_OK);
    s_tuning.revision=0xffffffffu;
    Tuning_PrepareSet(41,&request,&context,TUNING_OK,&reply);assert(reply.code==TUNING_RECEIVED);
    Tuning_SelectControl(1,1,&original,245,&context);
    assert(Tuning_TakeRevision(&revision)&&revision==1&&!Tuning_TakeRevision(&revision));
    assert(Tuning_GetControl(&values)&&values.kp==3&&values.kp_v==11);
    assert(!Tuning_TakeEvent(&event)); /* Select cannot publish APPLIED before completion. */
    context.control_sequence=8;context.sample_tick=125;
    Tuning_CompleteControl(&context);
    assert(Tuning_TakeEvent(&event)&&event.kind==TUNING_EVENT_TUNE_APPLIED);
    assert(!Tuning_TakeEvent(&event));
    Tuning_PrepareSet(42,&request,&context,TUNING_OK,&reply);
    assert(reply.code==TUNING_BUSY); /* final mailbox still owns the first request. */
    assert(Tuning_TakeResult(&reply,&id)&&id==41&&reply.code==TUNING_APPLIED);
    assert(reply.position.tick==125&&reply.position.control_sequence==8&&reply.position.revision==1);
    assert(!Tuning_TakeResult(&reply,&id));
    tuning_request_t unsaved;
    assert(Tuning_CopyUnsaved(&unsaved)&&unsaved.values.kp==3);
    Tuning_SelectControl(0,1,&original,245,&context);
    assert(!Tuning_TakeEvent(&event));
    assert(Tuning_TakeRevision(&revision)&&revision==2);
    Tuning_CompleteControl(&context);
    assert(Tuning_TakeEvent(&event)&&event.kind==TUNING_EVENT_TUNE_SUSPENDED);
    Tuning_CopyEventValues(event.kind,&revision,&values);
    assert(revision==2&&values.kp==2&&values.kp_v==20);
    Tuning_CopyEventValues(TUNING_EVENT_PARAM_SAVED,&revision,&values);
    assert(values.kp==3&&values.kp_v==11); /* Success and effective remain independent. */
    Tuning_SelectControl(1,1,&original,245,&context);Tuning_TakeRevision(&revision);
    Tuning_CompleteControl(&context);
    assert(Tuning_TakeEvent(&event)&&event.kind==TUNING_EVENT_TUNE_RESUMED);
    Tuning_PrepareSet(43,&request,&context,TUNING_OK,&reply);assert(reply.code==TUNING_RECEIVED);
    Tuning_ResetSession();assert(!Tuning_TakeResult(&reply,&id)&&s_tuning.enabled);
    assert(Tuning_CopyUnsaved(&unsaved)&&unsaved.values.kp==3);
    Tuning_PrepareSet(44,&request,&context,TUNING_OK,&reply);
    context.protection_flags=1;Tuning_SelectControl(1,1,&original,245,&context);
    assert(Tuning_TakeResult(&reply,&id)&&id==44&&reply.code==TUNING_BUSY);
    Tuning_Stop();assert(!s_tuning.enabled&&!s_tuning.overridden);
    assert(active_pid.kp_v==20&&active_motor.fan_duty==1600);
    context=(tuning_context_t){1,0,0,0,1,0,0,200,10};
    s_tuning.revision=0xffffffffu;
    Tuning_PrepareSet(45,&request,&context,TUNING_OK,&reply);assert(reply.code==TUNING_OK);
    Tuning_CommitParked(&request,&reply);
    assert(reply.code==TUNING_APPLIED&&reply.position.revision==0&&reply.position.tick==200);
    assert(Tuning_TakeRevision(&revision)&&revision==0); /* Parked path permits zero. */
    assert(Tuning_TakeEvent(&event)&&event.kind==TUNING_EVENT_TUNE_APPLIED);
    Tuning_CompleteSave(0);
    assert(!Tuning_CopyUnsaved(&unsaved)&&s_tuning.save_state==3);
    assert(Tuning_TakeEvent(&event)&&event.kind==TUNING_EVENT_PARAM_SAVE_FAILED);
    assert(depth==0);
    puts("PASS G4 transactions: final ownership, independent values, event timing, protection/session cancel, revision wrap");
}
'''


def main():
    with tempfile.TemporaryDirectory(prefix="g4_transactions_") as directory:
        work = Path(directory)
        # Only the original critical-section boundary is substituted; the owner is real C.
        (work / "bsp.h").write_text(
            '#include <stdint.h>\nuint32_t BSP_Time_EnterCritical(void);\n'
            'void BSP_Time_ExitCritical(uint32_t);\n', encoding="utf-8")
        (work / "test.c").write_text(HARNESS, encoding="utf-8")
        binary = build(work, [work / "test.c"])
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
