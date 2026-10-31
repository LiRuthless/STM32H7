from pathlib import Path
R=Path(__file__).resolve().parents[2];T=R/'tests'
def read(p):return p.read_bytes().decode().replace('\r\n','\n')
def write(p,s):p.write_bytes(s.replace('\r\n','\n').replace('\n','\r\n').encode())
p=T/'dual_mode_tracking.py';s=read(p)
s=s.replace('#include "app.c"','#include "bsp.h"\n#include "app.c"\n#include "lifecycle.h"\n#include "safety.h"\n#include "battery.h"\n#include "param.h"\n#include "menu.h"\n#include "wireless.h"\n#include "track_sensor.h"\n#include "imu_proc.h"\n#include "pid.h"\n#include "control.h"\n#include "roundabout.h"\n#include "motor.h"\n#include "filter.h"\n#include "datalog.h"\n#include "dashboard.h"\n#include "gray_sensor.h"\n#include "scb_link.h"\n#include "telemetry.h"\n#include "tuning.h"')
s=s.replace('App_TryStart','Lifecycle_TryStart').replace('s_stop_pending','Safety_IsStopPending()').replace('s_stop_reason','Safety_GetStopReason()')
s=s.replace('headers=(\'bsp_adc\'',"headers=('bsp_runtime','bsp_adc'")
s=s.replace("('app.c','gray_sensor.c','gray_sensor.h')","('app.c','scheduler.c','gray_sensor.c','gray_sensor.h')")
s=s.replace("app_copy=(work/'app.c')","app_copy=(work/'scheduler.c')").replace("(work/'app.c').write_text(injected","(work/'scheduler.c').write_text(injected")
s=s.replace(r'\(\s*g_app_key_flag\s*\)',r'\(\s*Lifecycle_IsKeyActive\(\)\s*\)')
s=s.replace("injected, count = branch.subn", "app_copy = '#include \"main.h\"\\n' + app_copy\n    injected, count = branch.subn")
s=s.replace("files=('pid'","files=('lifecycle','safety','battery','pid'")
s=s.replace("*(['-DTEST_UNCALIBRATED']", "'-DSMARTCAR_HOST_TEST',*(['-DTEST_UNCALIBRATED']")
s=s.replace("str(ROOT/'BSP/bsp_key.c'),", "str(ROOT/'BSP/bsp_key.c'),str(ROOT/'BSP/bsp_runtime.c'),str(work/'scheduler.c'),")
s=s.replace("(work/'main.h').write_text(main,encoding='utf-8')", "(work/'main.h').write_text(main,encoding='utf-8')\n    (work/'tim.h').write_text('#include \"main.h\"\\n',encoding='utf-8')")
start=s.index('APP_MAIN=r');end=s.index('def app_tests(',start)
piece=s[start:end]
for a,b in {'Start_flag':'Lifecycle_IsStarted()','Run_flag':'Lifecycle_IsRunning()','key_flag':'Lifecycle_IsKeyActive()','time':'Lifecycle_GetControlTicks()'}.items():
    import re
    piece=re.sub(r'\b'+a+r'\b',b,piece)
s=s[:start]+piece+s[end:]
s=s.replace('uint32_t HAL_GetTick(void){return now++;}', 'uint32_t HAL_GetTick(void){return now++;}\nuint32_t BSP_Time_GetTick(void){return HAL_GetTick();}')
s=s.replace('void BSP_PWM_Init(void){memset(pwm,0,sizeof pwm);}', 'void BSP_PWM_Init(void){memset(pwm,0,sizeof pwm);}\nvoid BSP_PWM_SetDirection(bsp_pwm_channel_t ch,uint8_t high){HAL_GPIO_WritePin(ch==BSP_PWM_MOTOR_L?MOTOR_L_DIR_GPIO_Port:MOTOR_R_DIR_GPIO_Port,ch==BSP_PWM_MOTOR_L?MOTOR_L_DIR_Pin:MOTOR_R_DIR_Pin,high?GPIO_PIN_SET:GPIO_PIN_RESET);}')
# Standalone FSM/parameter fixtures own local values behind the new read interfaces.
getters='''
int32_t Lifecycle_GetControlTicks(void){return time;}
uint16_t Lifecycle_GetDistanceMm(void){return dl1b_distance_mm;}
uint8_t Lifecycle_IsStarted(void){return Start_flag;}
void Lifecycle_MarkRunning(void){Run_flag=1;}
'''
s=s.replace('static gray_state_t gray;',getters+'static gray_state_t gray;')
s=s.replace('int16_t battery_filt;','int16_t battery_filt;\nint16_t Battery_GetFiltered(void){return battery_filt;}')
s=s.replace('uint32_t HAL_GetTick(void){return now;}','uint32_t HAL_GetTick(void){return now;}\nuint32_t BSP_Time_GetTick(void){return HAL_GetTick();}')
# Expose BSP millisecond declaration in stand-alone fake aggregate headers too.
s=s.replace('uint32_t HAL_GetTick(void);','uint32_t HAL_GetTick(void);\\nuint32_t BSP_Time_GetTick(void);')
# The replacement above crosses Python text: C raw main block needs actual newline.
s=s.replace('uint32_t HAL_GetTick(void);\\nuint32_t BSP_Time_GetTick(void);\n','uint32_t HAL_GetTick(void);\nuint32_t BSP_Time_GetTick(void);\n')
write(p,s)

p=T/'runtime_tuning.py';s=read(p)
s=s.replace('g_app_start_flag=g_app_key_flag=g_app_run_flag=0;s_stop_pending=0;s_stop_reason=APP_STOP_NONE;', 'Lifecycle_TestSetRunState(Lifecycle_GetControlTicks(),0,0,0);Safety_Reset();')
s=s.replace('g_app_control_ticks=1001;', 'Lifecycle_TestSetRunState(1001,Lifecycle_IsKeyActive(),Lifecycle_IsStarted(),Lifecycle_IsRunning());').replace('g_app_control_ticks=1000;', 'Lifecycle_TestSetRunState(1000,Lifecycle_IsKeyActive(),Lifecycle_IsStarted(),Lifecycle_IsRunning());')
s=s.replace('g_app_start_flag=g_app_key_flag=1;', 'Lifecycle_TestSetRunState(Lifecycle_GetControlTicks(),1,1,Lifecycle_IsRunning());').replace('s_stop_reason=APP_STOP_NONE;', 'Safety_Reset();')
write(p,s)

p=T/'control_equivalence.py';s=read(p)
s=s.replace('void BSP_PWM_SetDuty(int channel, uint32_t duty);','void BSP_PWM_SetDuty(int channel, uint32_t duty);\nvoid BSP_PWM_SetDirection(int channel, uint8_t high);')
s=s.replace('void BSP_PWM_Init(void) {}','void BSP_PWM_Init(void) {}\nvoid BSP_PWM_SetDirection(int channel,uint8_t high){HAL_GPIO_WritePin(channel==BSP_PWM_MOTOR_L?MOTOR_L_DIR_GPIO_Port:MOTOR_R_DIR_GPIO_Port,channel==BSP_PWM_MOTOR_L?MOTOR_L_DIR_Pin:MOTOR_R_DIR_Pin,high?GPIO_PIN_SET:GPIO_PIN_RESET);}')
write(p,s)

p=T/'datalog_v2.py';s=read(p).replace('HAL_GetTick','BSP_Time_GetTick')
s=s.replace('int16_t battery_filt=1300;uint16_t dl1b_distance_mm=8192;', 'int16_t battery_filt=1300;uint16_t dl1b_distance_mm=8192;\nint16_t Battery_GetFiltered(void){return battery_filt;}\nuint16_t Lifecycle_GetDistanceMm(void){return dl1b_distance_mm;}')
write(p,s)
p=T/'scb_link.py';write(p,read(p).replace('HAL_GetTick','BSP_Time_GetTick'))
p=T/'spi_log_export.py';write(p,read(p).replace('App_TryStart','Lifecycle_TryStart'))

# G1 byte equivalence is historical, structural checks continue after intentional extraction.
p=T/'app_layout_check.py';s=read(p).replace('parser.add_argument("--base", required=True,','parser.add_argument("--base", default="4745aa2339209d5df9a739449685fc7f1cb66429",').replace('    args = parser.parse_args()','    parser.add_argument("--g1-equivalence", action="store_true")\n    args = parser.parse_args()')
s=s.replace('assert {path.name for path in paths} == historical_names, "G1 must preserve all baseline modules"','assert historical_names <= {path.name for path in paths}, "original module names preserved through G2"')
s=s.replace('assert relative == mapping.get(source.name, source.name), (source, mapping)','if source.name in mapping:\n            assert relative == mapping[source.name], (source, mapping)')
a=s.index('        original = subprocess.run');b=s.index('        assert (APP / source.name)',a)
part=s[a:b];s=s[:a]+'        if args.g1_equivalence:\n'+''.join('    '+line+'\n' for line in part.rstrip().splitlines())+s[b:]
s=s.replace('PASS G1:', 'PASS App structure:').replace('C/H preserved; fixture exact;', 'C/H located; fixture exact;')
write(p,s)
