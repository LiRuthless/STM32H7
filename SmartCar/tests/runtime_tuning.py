"""Run real App/TIM6/FSM/PID/parameter C against independent equations and fake BSP."""
import json
from pathlib import Path
import tempfile
import dual_mode_tracking as dual

prefix = dual.APP_MAIN.split('int main(void)')[0]
start = prefix.index('void SCB_Init(void)')
end = prefix.index('void Telemetry_BeginRun', start)
prefix = prefix[:start] + r'''
static uint8_t cached[128][993], final_flags[128];
static uint16_t lengths[128];
static unsigned parameter_writes, event_counts[8];
static scb_values_t saved_event;
void SCB_Init(void){memset(&upload_status,0,sizeof upload_status);}
void SCB_Service(void){++link_services;}
void SCB_SetHandlers(scb_command_handler_t c,scb_session_handler_t s){assert(c&&s);}
void Telemetry_SetRevision(uint32_t r){upload_status.revision=r;}
uint8_t SCB_QueueEvent(const scb_event_t*e){++event_counts[e->kind];if(e->kind==SCB_EVENT_PARAM_SAVED)saved_event=e->values;return 1;}
uint8_t SCB_QueueResult(uint32_t id,uint8_t type,const uint8_t*p,uint16_t n,uint8_t final)
{assert(id<128&&type==SCB_RESULT&&n<=992);memcpy(cached[id],p,n);cached[id][n]=0;lengths[id]=n;final_flags[id]=final;return 1;}
uint8_t SCB_GetRequestResult(uint32_t id,uint8_t*t,uint8_t*p,uint16_t*n)
{if(id>=128||!lengths[id])return 0;*t=SCB_RESULT;*n=lengths[id];memcpy(p,cached[id],*n);return 1;}
''' + prefix[end:]
prefix = prefix.replace('out->raw[i]=gray_line?1000:0',
                        'out->raw[i]=gray_line?(i<3?1000:0):0')
prefix = prefix.replace('if(param_write_error)return 1;memcpy(params+a,b,n);',
                        '++parameter_writes;if(param_write_error)return 1;memcpy(params+a,b,n);')

MAIN = r'''
#include <stdlib.h>
#include "electromagnetic_fsm.h"
static unsigned next_id;
static scb_command_t command;
static unsigned send(const char*op)
{
    unsigned id=++next_id;assert(id<128);strcpy(command.op,op);
    Tuning_HandleCommand(id,&command);return id;
}
static void status(unsigned id,const char*s)
{char part[80];snprintf(part,sizeof part,"\"status\":\"%s\"",s);if(!strstr((char*)cached[id],part))fprintf(stderr,"id=%u expected=%s got=%s\n",id,s,cached[id]);assert(strstr((char*)cached[id],part));}
static int16_t expected(int16_t error,int16_t prior,const scb_values_t*v)
{
    int16_t output=(int16_t)(v->kp*(float)error+v->k2p*(float)error*abs(error)
        +v->kd*(float)(error-prior));
    return output>1000?1000:(output<-1000?-1000:output);
}
static void simulate_run(uint8_t mode,uint8_t gear)
{
    Lifecycle_TestSetRunState(Lifecycle_GetControlTicks(),0,0,0);Safety_Reset();
    assert(Tracking_TrySetMode(mode)==TRACKING_OK);
    Param_SelectGear(gear);Param_SetFan(gear,0,1600);Param_SetFan(gear,1,1100);Param_SetSpeedGains(20,.75f);
    assert(Param_TryFlushPending());Tuning_Init();healthy();Lifecycle_TestSetRunState(1001,Lifecycle_IsKeyActive(),Lifecycle_IsStarted(),Lifecycle_IsRunning());
    Lifecycle_TestSetRunState(Lifecycle_GetControlTicks(),1,1,Lifecycle_IsRunning());command.mode=mode;command.gear=gear;
    upload_status.run_id++;
}
int main(void)
{
    memset(params,255,sizeof params);App_Init();
    for(uint8_t target=0;target<4;++target)
    {
        uint8_t mode=target/2,gear=target%2;simulate_run(mode,gear);
        unsigned writes=parameter_writes;
        float baseline_kp=PID_GetState()->kp_v,baseline_ki=PID_GetState()->ki_v;
        scb_values_t values={3,.002f,4,11,.4f,123,1900,1100};command.values=values;
        unsigned id=send("SET_PARAMS");status(id,"BUSY");
        id=send("TUNE_ENABLE");status(id,"OK");
        Lifecycle_TestSetRunState(1000,Lifecycle_IsKeyActive(),Lifecycle_IsStarted(),Lifecycle_IsRunning());id=send("SET_PARAMS");status(id,"BUSY");Lifecycle_TestSetRunState(1001,Lifecycle_IsKeyActive(),Lifecycle_IsStarted(),Lifecycle_IsRunning());
        command.values.fan_idle=1101;id=send("SET_PARAMS");status(id,"INVALID_ARG");command.values=values;
        id=send("SET_PARAMS");status(id,"RECEIVED");assert(!final_flags[id]);
        unsigned other=send("SET_PARAMS");status(other,"BUSY");
        App_ControlISR();
        int16_t error=Tracking_GetState()->active_error;assert(error);
        assert(Tracking_GetState()->track_out==expected(error,0,&values));
        assert(Motor_GetState()->base_speed==123&&PID_GetState()->kp_v==11&&PID_GetState()->ki_v==.4f);
        assert(pwm[2]==1900&&upload_status.revision);
        assert(parameter_writes==writes); /* TIM6没有Flash */
        Tuning_Service();status(id,"APPLIED");assert(final_flags[id]);
        printf("JSON %s\n",cached[id]);
        App_ControlISR();assert(Tracking_GetState()->track_out==expected(error,error,&values));
        /* 进入十字的同一拍拒绝pending并恢复PI/风扇，而非再计算一次历史。 */
        command.values.kp=9;id=send("SET_PARAMS");status(id,"RECEIVED");
        int16_t prior=error;
        adc_values[1]=adc_values[2]=2000;
        App_ControlISR();Tuning_Service();status(id,"SPECIAL_ELEMENT");
        scb_values_t original={2,.008f,15,baseline_kp,baseline_ki,mode?100:245,1600,1100};
        assert(Tracking_GetState()->track_out==expected(Tracking_GetState()->active_error,prior,&original));
        assert(PID_GetState()->kp_v==baseline_kp&&PID_GetState()->ki_v==baseline_ki&&pwm[2]==1600);
        tuning_status_t tune;Tuning_GetStatus(&tune);assert(tune.phase==TUNING_SUSPENDED&&!tune.overridden);
        assert(event_counts[SCB_EVENT_TUNE_SUSPENDED]);
        id=send("SET_PARAMS");status(id,"SPECIAL_ELEMENT");
        healthy();App_ControlISR();Tuning_GetStatus(&tune);
        assert(tune.overridden&&Motor_GetState()->base_speed==123&&pwm[2]==1900);
        assert(event_counts[SCB_EVENT_TUNE_RESUMED]);
        id=send("TUNE_DISABLE");status(id,"OK");App_ControlISR();Tuning_GetStatus(&tune);
        assert(!tune.overridden&&PID_GetState()->kp_v==baseline_kp&&pwm[2]==1600);
        assert(Motor_GetState()->base_speed==(mode?100:245));
        id=send("TUNE_ENABLE");status(id,"OK");App_ControlISR();assert(Motor_GetState()->base_speed==123);
        /* 预环/右环所有活动分支：私有部分配置不能携带调试增益。 */
        command.values.kp=8;id=send("SET_PARAMS");status(id,"RECEIVED");
        adc_values[0]=1000;adc_values[3]=2000;adc_values[1]=adc_values[2]=400;
        App_ControlISR();Tuning_Service();status(id,"SPECIAL_ELEMENT");
        assert(ElectromagneticFSM_GetState()->kernel_state==CONTROL_KERNEL_REISLAND);
        assert(ElectromagneticFSM_GetState()->configuration.track.kp==2);
        adc_values[1]=adc_values[2]=149;App_ControlISR();
        assert(ElectromagneticFSM_GetState()->roundabout_state==ROUNDABOUT_ISLAND_LPREENTER);
        ElectromagneticFSM_Accumulate(8000,8000);App_ControlISR();
        assert(ElectromagneticFSM_GetState()->roundabout_state==ROUNDABOUT_ISLAND_TURN_LEFT);
        ElectromagneticFSM_Accumulate(13001,13001);App_ControlISR();
        assert(ElectromagneticFSM_GetState()->roundabout_state==ROUNDABOUT_ISLAND_IN);
        ElectromagneticFSM_Accumulate(32001,32001);App_ControlISR();
        assert(ElectromagneticFSM_GetState()->roundabout_state==ROUNDABOUT_ISLAND_OUT);
        ElectromagneticFSM_Accumulate(10001,10001);App_ControlISR();Tuning_GetStatus(&tune);
        assert(tune.overridden&&ElectromagneticFSM_GetState()->kernel_state==CONTROL_KERNEL_TRACKING);
        assert(Motor_GetState()->base_speed==123&&PID_GetState()->kp_v==11);
        assert(ElectromagneticFSM_GetState()->configuration.track.kp==2);
        healthy();App_ControlISR();
        /* 重建会话取消未生效项，不撤销已经使用的123。 */
        command.values.kp=7;id=send("SET_PARAMS");status(id,"RECEIVED");
        Tuning_ResetSession();App_ControlISR();Tuning_Service();assert(!final_flags[id]);
        assert(Motor_GetState()->base_speed==123&&PID_GetState()->kp_v==11);
        /* 保护发生在Select前：不能发布APPLIED；停车保存来源仍为成功的3。 */
        id=send("SET_PARAMS");status(id,"RECEIVED");acquire_error=1;
        App_ControlISR();Tuning_Service();status(id,"BUSY");assert(!App_IsStopped());
        assert(parameter_writes==writes);acquire_error=0;
        App_Loop();assert(App_IsStopped());Tuning_Service();
        assert(parameter_writes==writes+1&&saved_event.kp==3&&saved_event.base_speed==123);
        assert(Param_GetDirection(mode,gear)->kp==3&&Param_GetDirection(mode,gear)->base_speed==123);
        assert(Param_GetFan(gear,0)==1900&&PID_GetState()->kp_v==11);
        Tuning_GetStatus(&tune);assert(!tune.enabled&&tune.save_state==2);
        Tuning_Service();assert(parameter_writes==writes+1);
        command.mode=mode;command.gear=gear;id=send("GET_PARAMS");status(id,"OK");printf("JSON %s\n",cached[id]);
        command.target_request_id=id;
        for(unsigned n=0;n<8;++n){id=send("GET_REQUEST_RESULT");status(id,"OK");command.target_request_id=id;}
        printf("JSON %s\n",cached[id]);
    }

    /* 停车预开启后切换mode/gear，再次ENABLE应绑定当前目标。 */
    Tuning_Init();assert(Tracking_TrySetMode(0)==TRACKING_OK);Param_SelectGear(0);
    unsigned id=send("TUNE_ENABLE");status(id,"OK");
    Param_SelectGear(1);assert(Tracking_TrySetMode(1)==TRACKING_OK);
    id=send("TUNE_ENABLE");status(id,"OK");
    float base_pi=PID_GetState()->kp_v;int16_t base_fan=Motor_GetState()->fan_duty;
    Lifecycle_TestSetRunState(Lifecycle_GetControlTicks(),1,1,Lifecycle_IsRunning());Safety_Reset();Lifecycle_TestSetRunState(1001,Lifecycle_IsKeyActive(),Lifecycle_IsStarted(),Lifecycle_IsRunning());healthy();
    command.mode=command.gear=1;command.values=(scb_values_t){4,.003f,5,12,.5f,140,1800,1100};
    id=send("SET_PARAMS");status(id,"RECEIVED");App_ControlISR();Tuning_Service();status(id,"APPLIED");
    id=send("TUNE_ENABLE");status(id,"OK"); /* 重复不得重捕获12 */
    id=send("TUNE_DISABLE");status(id,"OK");
    id=send("TUNE_ENABLE");status(id,"OK"); /* 恢复拍前重开仍沿用原基线 */
    id=send("TUNE_DISABLE");status(id,"OK");App_ControlISR();
    assert(PID_GetState()->kp_v==base_pi&&Motor_GetState()->fan_duty==base_fan);
    App_RequestStop(APP_STOP_OFF_TRACK);App_Loop();Tuning_Service();
    /* 最大合法结果必须完整；非有限旧bank编码错误仍保留原操作名。 */
    scb_values_t maximum={100,100,100,100,100,300,10000,10000};
    assert(SCB_ValidateValues(&maximum)==SCB_OK&&SCB_ValidateValues(0)==SCB_INVALID_ARG);
    scb_values_t minimum={0};assert(SCB_ValidateValues(&minimum)==SCB_OK);
    union {uint32_t bits;float value;} bad={0x7fc00000u};
    minimum.kp=bad.value;assert(SCB_ValidateValues(&minimum)==SCB_OUT_OF_RANGE);
    minimum=maximum;minimum.base_speed=301;assert(SCB_ValidateValues(&minimum)==SCB_OUT_OF_RANGE);
    command.values=maximum;id=send("SET_PARAMS");status(id,"APPLIED");
    id=send("GET_PARAMS");status(id,"OK");printf("MAXJSON %s\n",cached[id]);
    command.target_request_id=id;id=send("GET_REQUEST_RESULT");status(id,"OK");printf("MAXJSON %s\n",cached[id]);
    param_direction_t invalid={bad.value,1,1,100};assert(Param_SetDirection(1,1,&invalid));
    id=send("GET_PARAMS");status(id,"IO_ERROR");assert(strstr((char*)cached[id],"\"op\":\"GET_PARAMS\""));
    /* 停车整组及异步保存失败，不放宽原Flash门禁。 */
    unsigned before=parameter_writes;command.values=(scb_values_t){4,.003f,5,12,.5f,140,1800,1200};
    id=send("SET_PARAMS");status(id,"APPLIED");assert(parameter_writes==before);
    param_write_error=1;Tuning_Service();tuning_status_t tune;Tuning_GetStatus(&tune);
    assert(tune.save_state==3&&!Param_TryFlushPending()&&event_counts[SCB_EVENT_PARAM_SAVE_FAILED]);
    assert(parameter_writes==before+1);Tuning_Service();assert(parameter_writes==before+1);
    puts("PASS: real App/TIM6/FSM/PID atomic tuning, history, transition, recovery, save/failure");
    return 0;
}
'''

if __name__ == '__main__':
    with tempfile.TemporaryDirectory(prefix='runtime_tuning_') as directory:
        output = dual.app_tests(Path(directory), prefix + MAIN, real_tuning=True)
        objects = [json.loads(line[5:]) for line in output.splitlines() if line.startswith('JSON ')]
        assert len(objects) == 12
        for item in objects:
            assert item['status'] in ('OK', 'APPLIED')
            direct = item.get('result', item)
            if 'limits' in direct:
                assert direct['limits']['base_speed'] == {'min': 0, 'max': 300}
                assert direct['values']['kp'] == 3
                assert len(json.dumps(item, separators=(',', ':')).encode()) <= 992
        maximums = [json.loads(line[8:]) for line in output.splitlines() if line.startswith('MAXJSON ')]
        assert len(maximums) == 2 and maximums[0]['values']['kp'] == 100
        assert maximums[1]['result'] == maximums[0]
        print(output.splitlines()[-1])
