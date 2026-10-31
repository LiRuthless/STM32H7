from pathlib import Path
import re

ROOT=Path(__file__).resolve().parents[2]
APP=ROOT/'App'
def read(p):return p.read_bytes().decode('utf-8').replace('\r\n','\n')
def write(p,s):p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(s.replace('\r\n','\n').replace('\n','\r\n').encode('utf-8'))
old=read(APP/'app.c')
inc=old[old.index('#include'):old.index('/* 状态灯')]
inc += '#include "system/lifecycle.h"\n#include "system/safety.h"\n#include "sensing/battery.h"\n'
def extract(name):
    m=re.search(r'(?m)^(?:static )?(?:void|uint8_t|float) '+name+r'\([^;]*?\)\n\{',old)
    start=old.rfind('/**',0,m.start()); brace=old.index('{',m.start()); depth=1;i=brace+1
    while depth:
        depth += (old[i]=='{')-(old[i]=='}');i+=1
    return old[start:i]+'\n'
def filedoc(name,brief,detail):return f'/**\n * @file {name}\n * @brief {brief}\n * @details {detail}\n */\n'
def doc(brief,details='',params=(),ret='',pre='',note=''):
    s='/**\n * @brief '+brief+'\n'
    for tag,value in [('details',details),*[(f'param[in] {n}',v) for n,v in params],('return',ret),('pre',pre),('note',note)]:
        if value:s+=' * @'+tag+' '+value+'\n'
    return s+' */\n'
def fn(name,ret,brief,body,params=(),details='',returns='',pre='',note=''):
    return doc(brief,details,params,returns,pre,note)+ret+' '+name+'('+(', '.join(n+' '+v for n,v in []) if False else 'void')+')\n{\n'+body+'\n}\n'

# Enumerations are shared business types; hardware headers no longer enter App through app.h.
h=read(APP/'app.h'); enum=h[h.index('typedef enum'):h.index('/* 源 config')]
write(APP/'app_types.h',filedoc('app_types.h','应用跨模块停车原因。','共享停车枚举保持数值，供运行、安全、日志与控制接口使用。')+'#ifndef SMARTCAR_APP_APP_TYPES_H\n#define SMARTCAR_APP_APP_TYPES_H\n\n'+enum+'#endif\n')
h=h[:h.index('#include "main.h"')]+ '#include <stdint.h>\n#include "app_types.h"\n\n'+h[h.index('/**\n * @brief 按既有顺序初始化'):]
write(APP/'app.h',h)

mapping={'g_app_control_ticks':'Lifecycle_GetControlTicks()', 'g_app_dl1b_distance_mm':'Lifecycle_GetDistanceMm()', 'g_app_start_flag':'Lifecycle_IsStarted()', 'g_app_run_flag':'Lifecycle_IsRunning()', 'g_app_key_flag':'Lifecycle_IsKeyActive()', 'g_app_battery_filtered':'Battery_GetFiltered()'}
def getters(s):
    for a,b in mapping.items():s=re.sub(r'\b'+a+r'\b',b,s)
    return s.replace('HAL_GetTick()', 'BSP_Time_GetTick()')

# Lifecycle: states remain individual scalars, including each volatile flag.
life=filedoc('lifecycle.c','应用初始化与起跑事务。','持有运行标志、控制拍、测距和看门狗生命周期；前台启动定时链，ISR经显式命令更新状态。')+inc
life+='''
/* 起跑前台复位、TIM6自增；单位为2ms拍。 */
static int32_t s_control_ticks = 0;
/* 前台发布、首次停车清零；每次读取保持volatile访问。 */
static volatile uint8_t s_key_flag = 0;
static volatile uint8_t s_start_flag = 0;
static volatile uint8_t s_run_flag = 0;
/* 控制拍更新的测距毫米值，8192表示无效。 */
static uint16_t s_dl1b_distance_mm = BSP_DL1B_INVALID;
/* 看门狗首次成功起跑后置位，停车期间仍需喂狗。 */
static uint8_t s_wdt_started = 0;
/* 上电参数加载返回值，仅初始化流程写入。 */
static uint8_t s_flash_available = 0;

static void Lifecycle_ResetRunState(void);

'''
init=extract('App_Init').replace('App_Init','Lifecycle_Init').replace('MODIFY_REG(RCC->D2CCIP1R, RCC_D2CCIP1R_SPI45SEL, RCC_SPI45CLKSOURCE_PLL3);','BSP_Runtime_ConfigureSpiClock();').replace('Filter_InitLowpass(&g_app_battery_filter, 0.65f);','Battery_Init();')
start=extract('App_TryStart').replace('static uint8_t App_TryStart','uint8_t Lifecycle_TryStart').replace('App_ResetRunState','Lifecycle_ResetRunState').replace('HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, APP_LED_RUN_LEVEL);','BSP_Runtime_SetRunLed();').replace('g_app_key_flag = g_app_start_flag = 1;','s_key_flag = s_start_flag = 1;').replace('HAL_TIM_Base_Start_IT(&htim6);\n    HAL_TIM_Base_Start_IT(&htim7);','BSP_Runtime_StartTimers();')
reset=extract('App_ResetRunState').replace('App_ResetRunState','Lifecycle_ResetRunState').replace('g_app_control_ticks = 0;','s_control_ticks = 0;').replace('g_app_low_power_count = 0;\n    s_stop_reason = APP_STOP_NONE;\n    s_stop_pending = 0;','Safety_Reset();')
life+=init+'\n'+start+'\n'+reset+'\n'
life+=fn('Lifecycle_IsStopped','uint8_t','检查启动、按键及待收尾门禁。','    return !s_start_flag && !s_key_flag && !Safety_IsStopPending();',returns='三个条件都为零返回1，否则0。',note='前台/ISR可读；保留短路顺序与每次volatile读取。')
get_defs=[('IsStarted','uint8_t','s_start_flag','读取定时控制启动标志。'),('IsKeyActive','uint8_t','s_key_flag','读取前台运行分支标志。'),('IsRunning','uint8_t','s_run_flag','读取等待期结束后的运行标志。'),('GetControlTicks','int32_t','s_control_ticks','读取本次运行2ms拍数。'),('GetDistanceMm','uint16_t','s_dl1b_distance_mm','读取最近控制拍的测距毫米值。'),('IsWatchdogStarted','uint8_t','s_wdt_started','读取看门狗已启动标志。')]
for n,t,v,b in get_defs:life+='\n'+fn('Lifecycle_'+n,t,b,'    return '+v+';',returns={'GetDistanceMm':'距离毫米值，8192表示无效。','GetControlTicks':'原有int32_t控制拍计数。'}.get(n,'当前标志值，0或1。'),note='单次读值，不加锁或合并调用方重复读取。')
life+='\n'+fn('Lifecycle_AdvanceControlTick','void','推进一个2ms控制拍。','    ++s_control_ticks;',note='仅TIM6原自增位置调用，不改计数宽度。')
life+='\n'+doc('保存本拍的测距结果。',params=[('distance_mm','BSP测距毫米值，8192表示无效。')],note='仅TIM6原赋值位置调用，不夹紧输入。')+'void Lifecycle_SetDistanceMm(uint16_t distance_mm)\n{\n    s_dl1b_distance_mm = distance_mm;\n}\n'
life+='\n'+fn('Lifecycle_MarkRunning','void','发布等待期已结束的运行标志。','    s_run_flag = 1;',note='仅原TIM6及旧控制入口赋值点调用。')
life+='\n'+fn('Lifecycle_ClearRunFlags','void','按原链式顺序清除运行标志。','    s_start_flag = s_run_flag = s_key_flag = 0;',pre='调用者已进入首次停车的PRIMASK临界区。',note='不自行开关中断，保持key、run、start写入顺序。')
life+='\n#if defined(SMARTCAR_HOST_TEST)\n'+doc('在宿主夹具中布置运行状态。',params=[('ticks','控制拍计数。'),('key','前台运行标志，0或1。'),('start','启动标志，0或1。'),('run','运行标志，0或1。')],note='仅宿主测试编译；不操作硬件，不复位安全或算法状态。')+'''void Lifecycle_TestSetRunState(int32_t ticks, uint8_t key, uint8_t start, uint8_t run)
{
    s_control_ticks = ticks;
    s_key_flag = key;
    s_start_flag = start;
    s_run_flag = run;
}
#endif
'''
write(APP/'system/lifecycle.c',life)

# Safety owns the low-voltage decision; sensing owns only ADC/filter history.
safety=filedoc('safety.c','首因锁存、立即停车与前台收尾。','ISR与前台累计保护位；只在原临界区锁存首因和清标志，阻塞日志尾包留前台。')+inc+'''
/* 前台/ISR共享停车事务，首因仅由成功起跑复位。 */
static volatile uint8_t s_stop_pending = 0;
static volatile app_stop_reason_t s_stop_reason = APP_STOP_NONE;
/* TIM7累计区间内低压次数，成功起跑前清零。 */
static int16_t s_low_count = 0;

'''
safety+=extract('App_RequestStop').replace('App_RequestStop','Safety_RequestStop').replace('g_app_start_flag = g_app_run_flag = g_app_key_flag = 0;','Lifecycle_ClearRunFlags();').replace('(void)HAL_TIM_Base_Stop_IT(&htim6);\n    (void)HAL_TIM_Base_Stop_IT(&htim7);','BSP_Runtime_StopTimers();')+'\n'
safety+=extract('App_FinalizeStop').replace('static void App_FinalizeStop','void Safety_FinalizeStop').replace('s_wdt_started','Lifecycle_IsWatchdogStarted()')+'\n'
safety+=fn('Safety_IsStopPending','uint8_t','读取待前台收尾标志。','    return s_stop_pending;',returns='待收尾为1，否则0。',note='单次volatile读取，前台/ISR可读。')+'\n'
safety+=fn('Safety_GetStopReason','app_stop_reason_t','读取已锁存的首次停车原因。','    return s_stop_reason;',returns='首个非零原因；成功起跑复位后为APP_STOP_NONE。',note='单次volatile读取，不清除原因。')+'\n'
safety+=fn('Safety_Reset','void','在成功起跑前清除安全历史。','    s_low_count = 0;\n    s_stop_reason = APP_STOP_NONE;\n    s_stop_pending = 0;',details='依次清低压持续计数、首因和待收尾标志，保留原跨对象写入顺序。',pre='定时链尚未启动，门禁已通过。')+'\n'
safety+=doc('检查本次滤波值的低压持续条件。','先按原顺序读取两次电池值；区间内先累加再比较持续次数，否则清计数。',ret='连续低压超过APP_BATTERY_LOW_COUNT返回1，否则0。',note='仅TIM7采样后调用；本函数不采样、不请求停车。')+'''uint8_t Safety_CheckLowVoltage(void)
{
    if (Battery_GetFiltered() < APP_BATTERY_LOW_THRESHOLD
        && Battery_GetFiltered() > APP_BATTERY_MIN_VALID)
    {
        if (++s_low_count > APP_BATTERY_LOW_COUNT)
        {
            return 1;
        }
    }
    else
    {
        s_low_count = 0;
    }
    return 0;
}
'''
write(APP/'system/safety.c',safety)

sch=filedoc('scheduler.c','前台与1/2/5ms调度编排。','保持前台二次停车收尾与三线工作次序；业务状态由生命周期、安全和感知模块持有。')+inc
loop=getters(extract('App_Loop')).replace('void App_Loop','void Scheduler_Loop').replace('s_wdt_started','Lifecycle_IsWatchdogStarted()').replace('s_stop_pending','Safety_IsStopPending()').replace('App_FinalizeStop','Safety_FinalizeStop').replace('App_TryStart','Lifecycle_TryStart').replace('HAL_GPIO_TogglePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin);','BSP_Runtime_ToggleLed();')
loop=re.sub(r'        g_app_battery_raw =.*?Filter_UpdateLowpass\(&g_app_battery_filter, \(float\)g_app_battery_raw\);','        Battery_Update();',loop,flags=re.S)
# Getter substitution has already replaced only the filtered lvalue in the block; regex above still spans it.
ctrl=extract('App_ControlISR').replace('void App_ControlISR','void Scheduler_ControlISR').replace('++g_app_control_ticks;','Lifecycle_AdvanceControlTick();').replace('g_app_dl1b_distance_mm = BSP_DL1B_GetDistanceMm();','Lifecycle_SetDistanceMm(BSP_DL1B_GetDistanceMm());').replace('g_app_run_flag = 1;','Lifecycle_MarkRunning();')
ctrl=getters(ctrl)
task=extract('App_TaskISR').replace('void App_TaskISR','void Scheduler_TaskISR')
a=task.index('    g_app_battery_raw =');b=task.index('    BSP_WDT_Feed();',a)
task=task[:a]+'''    Battery_Update();
    if (Safety_CheckLowVoltage())
    {
        App_RequestStop(APP_STOP_LOW_BATTERY);
    }

'''+task[b:]
task=getters(task).replace('HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, APP_LED_RUN_LEVEL);','BSP_Runtime_SetRunLed();')
sample=getters(extract('App_SampleISR')).replace('void App_SampleISR','void Scheduler_SampleISR')
write(APP/'system/scheduler.c',sch+loop+'\n'+ctrl+'\n'+task+'\n'+sample)

battery=filedoc('battery.c','电池ADC采样和低通滤波。','停车前台或TIM7刷新原始值与滤波值；低压持续保护由安全模块判断，不反向依赖系统编排。')+'''#include "bsp.h"
#include "sensing/battery.h"
#include "sensing/filter.h"

/* 前台停车采样或运行TIM7更新，显示及记录按原并发契约读取。 */
static int16_t s_raw = 0;
static int16_t s_filtered = 0;
/* 上电初始化alpha=0.65，起跑不清滤波历史。 */
static filter_lowpass_t s_filter;

'''
battery+=fn('Battery_Init','void','初始化电池低通滤波器。','    Filter_InitLowpass(&s_filter, 0.65f);',note='仅上电原滤波初始化位置调用；不清除其他采样状态。')+'\n'
battery+=fn('Battery_Update','void','读取电池ADC并刷新滤波值。','    s_raw = (int16_t)BSP_ADC_Read(BSP_ADC_VBAT);\n    s_filtered = (int16_t)Filter_UpdateLowpass(&s_filter, (float)s_raw);',details='先把ADC缩窄为int16_t，再转float滤波，最后截断为int16_t，保持原标度和计算顺序。',note='停车前台或TIM7调用；不实施低压保护，不复位滤波历史。')+'\n'
for n,v,b in [('Raw','s_raw','读取电池ADC原始标度值。'),('Filtered','s_filtered','读取电池滤波标度值。')]:battery+=fn('Battery_Get'+n,'int16_t',b,'    return '+v+';',returns='最近采样的int16_t值。',note='单次复制，无锁或额外采样。')+'\n'
write(APP/'sensing/battery.c',battery)

# Headers expose commands and copies, never writable objects.
headers={'lifecycle': [('void','Init'),('uint8_t','TryStart'),('uint8_t','IsStopped'),('uint8_t','IsStarted'),('uint8_t','IsKeyActive'),('uint8_t','IsRunning'),('int32_t','GetControlTicks'),('uint16_t','GetDistanceMm'),('void','AdvanceControlTick'),('void','SetDistanceMm','uint16_t distance_mm'),('void','MarkRunning'),('void','ClearRunFlags'),('uint8_t','IsWatchdogStarted')], 'safety':[('void','RequestStop','app_stop_reason_t reason'),('void','FinalizeStop'),('uint8_t','IsStopPending'),('app_stop_reason_t','GetStopReason'),('void','Reset'),('uint8_t','CheckLowVoltage')], 'scheduler':[('void','Loop'),('void','ControlISR'),('void','TaskISR'),('void','SampleISR')], 'battery':[('void','Init'),('void','Update'),('int16_t','GetRaw'),('int16_t','GetFiltered')]}
for name,fs in headers.items():
    folder='sensing' if name=='battery' else 'system';prefix=name.capitalize();src=read(APP/folder/(name+'.c'));guard=f'SMARTCAR_APP_{folder.upper()}_{name.upper()}_H'
    out=filedoc(name+'.h',{'lifecycle':'运行生命周期接口。','safety':'安全保护与收尾接口。','scheduler':'前台及三线调度接口。','battery':'电池采样与滤波只读接口。'}[name],f'接口实现与完整调用约束见{name}.c；模块状态不对外暴露。')+f'#ifndef {guard}\n#define {guard}\n\n#include <stdint.h>\n'
    if name=='safety':out+='#include "app_types.h"\n'
    out+='\n'
    for f in fs:
        t,n,*args=f;symbol=prefix+'_'+n;match=re.search(r'/\*\*\n \* @brief ([^\n]+)\n(?:(?!/\*\*)[\s\S])*?\*/\n'+t+' '+symbol+r'\(',src)
        brief=match.group(1) if match else '执行'+symbol+'接口。'
        out+='/**\n * @brief '+brief+'\n * @note 完整状态、上下文与副作用见'+name+'.c::'+symbol+'。\n */\n'+t+' '+symbol+'('+ (args[0] if args else 'void')+');\n\n'
    if name=='lifecycle':out+='#if defined(SMARTCAR_HOST_TEST)\n/**\n * @brief 布置宿主运行状态。\n * @note 仅宿主测试编译；见lifecycle.c::Lifecycle_TestSetRunState。\n */\nvoid Lifecycle_TestSetRunState(int32_t ticks, uint8_t key, uint8_t start, uint8_t run);\n#endif\n\n'
    write(APP/folder/(name+'.h'),out+'#endif\n')

facade=old[:old.index('#include')]+'''#include "app.h"
#include "system/lifecycle.h"
#include "system/scheduler.h"
#include "system/safety.h"

'''
for name,target in [('Init','Lifecycle_Init'),('Loop','Scheduler_Loop'),('ControlISR','Scheduler_ControlISR'),('TaskISR','Scheduler_TaskISR'),('SampleISR','Scheduler_SampleISR')]:
    source=extract('App_'+name);facade+=source[:source.index('{')]+'{\n    '+target+'();\n}\n\n'
facade+=doc('请求安全停车。',params=[('reason','停车原因；NONE无副作用返回。')],note='ISR或前台调用；完整安全动作见safety.c::Safety_RequestStop。')+'void App_RequestStop(app_stop_reason_t reason)\n{\n    Safety_RequestStop(reason);\n}\n\n'
facade+=fn('App_IsStopped','uint8_t','查询可进入停车事务的门禁。','    return Lifecycle_IsStopped();',returns='启动、按键和待收尾都清零返回1，否则0。',note='完整读取顺序见lifecycle.c::Lifecycle_IsStopped。')+'\n'+extract('App_AbsFloat')
write(APP/'app.c',facade)

for p in APP.rglob('*.c'):
    if p.name in ['app.c','lifecycle.c','safety.c','scheduler.c','battery.c']:continue
    s=read(p)
    s=s.replace('extern int32_t g_app_control_ticks;\n','').replace('extern uint16_t g_app_dl1b_distance_mm;\n','')
    if p.name=='control.c':s=s.replace('g_app_run_flag = 1;','Lifecycle_MarkRunning();')
    new=getters(s)
    if 'Lifecycle_' in new and 'system/lifecycle.h' not in new:new=new.replace('#include "app.h"','#include "app.h"\n#include "system/lifecycle.h"') if '#include "app.h"' in new else new.replace('#include "bsp.h"','#include "bsp.h"\n#include "system/lifecycle.h"')
    if 'Battery_' in new:new=new.replace('#include "app.h"','#include "app.h"\n#include "sensing/battery.h"')
    new=re.sub(r'HAL_GPIO_WritePin\(MOTOR_([LR])_DIR_GPIO_Port, MOTOR_\1_DIR_Pin, GPIO_PIN_(SET|RESET)\)',lambda m:f'BSP_PWM_SetDirection(BSP_PWM_MOTOR_{m[1]}, {1 if m[2]=="SET" else 0})',new)
    new=new.replace('HAL_GetTick','BSP_Time_GetTick').replace('HAL_GPIO_WritePin','BSP_PWM_SetDirection')
    if p.name=='wireless.c':
        new=new.replace('g_app_uart_tx_buffer','s_uart_tx_buffer').replace('g_app_uart_rx_buffer','s_uart_rx_buffer')
        offset=new.index('/**',new.index('#include'))
        new=new[:offset]+'/* 停车前台串口命令的收发缓冲，各32字节，静态清零。 */\nstatic uint8_t s_uart_tx_buffer[32];\nstatic uint8_t s_uart_rx_buffer[32];\n\n'+new[offset:]
    if new!=read(p):write(p,new)

# BSP adapters preserve exact HAL action sequence and ignored statuses.
runtime=filedoc('bsp_runtime.c','运行定时器、状态灯和SPI时钟的板级适配。','在原时点执行HAL动作；TIM6/7回调分派应用入口，TIM15仍由采样器处理。')+'''#include "bsp_runtime.h"
#include "main.h"
#include "tim.h"
#include "app.h"

'''
runtime_funcs=[('ConfigureSpiClock','恢复SPI4/5内核到PLL3Q时钟。','    MODIFY_REG(RCC->D2CCIP1R, RCC_D2CCIP1R_SPI45SEL, RCC_SPI45CLKSOURCE_PLL3);','所有MX外设初始化完成后、应用初始化第一步调用。'),('StartTimers','依次启动TIM6控制与TIM7辅助中断。','    (void)HAL_TIM_Base_Start_IT(&htim6);\n    (void)HAL_TIM_Base_Start_IT(&htim7);','前台成功起跑时调用，保持原HAL失败不回滚行为。'),('StopTimers','依次停止TIM6与TIM7中断。','    (void)HAL_TIM_Base_Stop_IT(&htim6);\n    (void)HAL_TIM_Base_Stop_IT(&htim7);','首次停车的前台或ISR调用，忽略原HAL返回值。'),('SetRunLed','点亮PE3蓝色运行灯。','    HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_SET);','NPN驱动高电平点亮；起跑或TIM7原位置调用。'),('ToggleLed','翻转PE3蓝色状态灯。','    HAL_GPIO_TogglePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin);','仅停车前台250ms到期后调用；不保存软件时基。')]
for n,b,body,note in runtime_funcs:runtime+=fn('BSP_Runtime_'+n,'void',b,body,note=note)+'\n'
runtime+=extract('HAL_TIM_PeriodElapsedCallback')
write(ROOT/'BSP/bsp_runtime.c',runtime)
rh=filedoc('bsp_runtime.h','运行时硬件动作接口。','不暴露HAL类型；具体时钟、灯和定时器动作见bsp_runtime.c。')+'#ifndef SMARTCAR_BSP_BSP_RUNTIME_H\n#define SMARTCAR_BSP_BSP_RUNTIME_H\n\n'
for n,b,body,note in runtime_funcs:rh+='/**\n * @brief '+b+'\n * @note '+note+'见bsp_runtime.c::BSP_Runtime_'+n+'。\n */\nvoid BSP_Runtime_'+n+'(void);\n\n'
write(ROOT/'BSP/bsp_runtime.h',rh+'#endif\n')
p=ROOT/'BSP/bsp.h';write(p,read(p).replace('#include "bsp_time.h"','#include "bsp_time.h"\n#include "bsp_runtime.h"'))
p=ROOT/'BSP/bsp_time.c';write(p,read(p)+'\n'+fn('BSP_Time_GetTick','uint32_t','读取HAL毫秒时基。','    return HAL_GetTick();',returns='uint32_t毫秒计数，自然回绕。',note='只读一次，不关中断、不等待；由原调用者计算无符号差值。'))
p=ROOT/'BSP/bsp_time.h';write(p,read(p).replace('#endif','/**\n * @brief 读取自然回绕的毫秒时基。\n * @note 单次读取，见bsp_time.c::BSP_Time_GetTick。\n */\nuint32_t BSP_Time_GetTick(void);\n#endif'))
p=ROOT/'BSP/bsp_pwm.c';s=read(p).replace('电机方向 GPIO 由 App 的 Motor_UpdateControl 处理，本模块不设置方向。','电机方向由App决定物理电平，再经本模块封装GPIO动作。')
s+='\n'+doc('按物理电平设置左右电机方向。','仅左右电机通道写对应GPIO，风扇及其他枚举无动作；不设置PWM或推导转向。',params=[('channel','左右电机通道；其他值无动作。'),('high','非零为高电平，零为低电平。')],note='前台初始化或控制ISR调用，调用方保持原方向与PWM先后关系。')+'''void BSP_PWM_SetDirection(bsp_pwm_channel_t channel, uint8_t high)
{
    if (channel == BSP_PWM_MOTOR_L)
    {
        HAL_GPIO_WritePin(MOTOR_L_DIR_GPIO_Port, MOTOR_L_DIR_Pin,
            high ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
    else if (channel == BSP_PWM_MOTOR_R)
    {
        HAL_GPIO_WritePin(MOTOR_R_DIR_GPIO_Port, MOTOR_R_DIR_Pin,
            high ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}
''';write(p,s)
p=ROOT/'BSP/bsp_pwm.h';write(p,read(p).replace('方向由电机控制层设置。','方向电平由电机控制层决定。').replace('#endif /*','/**\n * @brief 设置左右电机物理方向电平。\n * @note 其他通道无动作，见bsp_pwm.c::BSP_PWM_SetDirection。\n */\nvoid BSP_PWM_SetDirection(bsp_pwm_channel_t channel, uint8_t high);\n\n#endif /*'))

# Add build entries without serializing/reformatting the rest of the Keil project.
p=ROOT/'MDK-ARM/SmartCar.uvprojx';s=read(p)
for group,files in [('App/system',['scheduler','lifecycle','safety']),('App/sensing',['battery']),('BSP',['bsp_runtime'])]:
    pos=s.index('<Files>',s.index('<GroupName>'+group+'</GroupName>'))+len('<Files>')
    entries=''.join('\n              <File>\n                <FileName>'+n+'.c</FileName>\n                <FileType>1</FileType>\n                <FilePath>../'+(group.replace('App/','App/') if group!='BSP' else 'BSP')+'/'+n+'.c</FilePath>\n              </File>' for n in files)
    s=s[:pos]+entries+s[pos:]
write(p,s)
