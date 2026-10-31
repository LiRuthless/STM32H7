import sys,tempfile
from pathlib import Path
root=Path(__file__).parent/'SmartCar'
sys.path.insert(0,str(root/'tests'))
import runtime_tuning as r
prefix=r.prefix.replace('uint16_t BSP_UART_Read(uint8_t*b,uint16_t n){assert(!s_stop_pending);(void)b;(void)n;return 0;}', 'static char uart_input[64];\nuint16_t BSP_UART_Read(uint8_t*b,uint16_t n){unsigned len=strlen(uart_input);assert(len<=n);memcpy(b,uart_input,len);uart_input[0]=0;return len;}')
main=r.MAIN.split('int main(void)')[0]+r'''
int main(void)
{
 memset(params,255,sizeof params);App_Init();
 pid_track_gains_t gains={0,4,0,1};int32_t history=0;
 int16_t out=PID_DirectionStep(100,0,&gains,&history);
 printf("PID: raw=40000 expected_saturation=1000 actual=%d\n",out);
 assert(out!=1000);
 float before=Param_GetDirection(1,0)->kp;
 strcpy(uart_input,"xp");Wireless_Adjust();strcpy(uart_input,"100\n");Wireless_Adjust();
 printf("UART split: old=%.2f expected=1 actual=%.2f\n",before,Param_GetDirection(1,0)->kp);
 assert(Param_GetDirection(1,0)->kp==before);
 strcpy(uart_input,"vp100\n");Wireless_Adjust();
 float kp,ki;Param_GetSpeedGains(&kp,&ki);
 printf("UART PI: active=%.2f bank=%.2f\n",PID_GetState()->kp_v,kp);
 assert(PID_GetState()->kp_v==1 && kp==20);
 command.mode=1;command.gear=0;command.values=(scb_values_t){2,.008f,15,20,.75f,100,1600,1200};
 unsigned id=send("SET_PARAMS");status(id,"APPLIED");
 printf("Fan parked: configured=%d PWM=%lu\n",Motor_GetState()->fan_duty_idle,(unsigned long)pwm[2]);
 assert(Motor_GetState()->fan_duty_idle==1200 && pwm[2]==1100);
 Tuning_Service();
 printf("Fan after save: PWM=%lu\n",(unsigned long)pwm[2]);
 assert(pwm[2]==1100);
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='audit_repro_') as t:
    print(r.dual.app_tests(Path(t),prefix+main,real_tuning=True))
