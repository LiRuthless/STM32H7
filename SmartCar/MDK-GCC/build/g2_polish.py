from pathlib import Path
import re
R=Path(__file__).resolve().parents[2];A=R/'App'
def read(p):return p.read_bytes().decode().replace('\r\n','\n')
def write(p,s):p.write_bytes(s.replace('\r\n','\n').replace('\n','\r\n').encode())
source='''/**
 * @note 移植来源：Sirius20260718/project/user/main.c、user/isr.c、code/config.c。
 * @note STC AI8051U 省赛快照（2026-07-18）；原算法未单列作者或独立许可证。
 * @note 保留源工程出处，关联 Phase 13/14/15 规约。
 */
'''
includes={
'lifecycle':['bsp.h','app_config.h','app.h','system/lifecycle.h','system/safety.h','sensing/battery.h','services/param.h','sensing/track_sensor.h','sensing/imu_proc.h','control/pid.h','control/control.h','control/roundabout.h','control/motor.h','services/datalog.h','hmi/dashboard.h','sensing/gray_sensor.h','services/scb_link.h','services/telemetry.h','control/tuning.h'],
'safety':['bsp.h','app_config.h','system/safety.h','system/lifecycle.h','sensing/battery.h','control/control.h','control/motor.h','services/datalog.h','services/scb_link.h','services/telemetry.h','control/tuning.h'],
'scheduler':['bsp.h','app_config.h','app.h','system/scheduler.h','system/lifecycle.h','system/safety.h','sensing/battery.h','services/param.h','hmi/menu.h','services/wireless.h','sensing/track_sensor.h','sensing/imu_proc.h','control/pid.h','control/control.h','control/motor.h','services/datalog.h','hmi/dashboard.h','sensing/gray_sensor.h','services/scb_link.h','control/tuning.h']}
for name,headers in includes.items():
    p=A/'system'/f'{name}.c';s=read(p);begin=s.index('#include');end=s.index('/*',begin)
    s=s[:begin]+source+''.join('#include "'+n+'"\n' for n in headers)+('#include <stdio.h>\n' if name=='safety' else '')+'\n'+s[end:]
    s=s.replace('按现有调用顺序使用 MODIFY_REG、BSP_UART_Init、BSP_Flash_Init、Param_Load','先经BSP恢复SPI4/5时钟，再初始化串口、Flash并加载参数；失败只打印提示，')
    s=s.replace('完成既有外设与控制初始化，最后初始化SCB静态链路和独立上传队列。','随后初始化采集、控制、电池、按键、编码器、PWM、LCD、IMU、测距与采样器，\n *     最后初始化SCB静态链路和独立上传队列。')
    s=s.replace('按现有调用顺序使用 BSP_Time_GetCycles、Track_ReadAdc、Gray_Update、Gray_GetState\n *     完成本函数处理；条件分支和状态写入均保持函数体所示顺序。','启动门禁后推进2ms拍并刷新双传感器，先累计采集、灰度和电感保护位，\n *     再按首因优先级请求停车；无故障时更新测距、IMU和双状态机，等待结束后执行电机控制。\n *     所有路径汇合后提交调参和日志快照，最后用无符号周期差检查完整900us预算。')
    s=s.replace('按现有调用顺序使用 BSP_ADC_Read、Filter_UpdateLowpass、App_RequestStop、BSP_WDT_Feed\n *     完成本函数处理；条件分支和状态写入均保持函数体所示顺序。','先刷新电池采样和滤波值，再检查安全模块的持续低压计数；超过200次时请求停车，\n *     随后无条件喂狗，最后只在启动标志仍有效时保持运行灯高电平。')
    s=s.replace('按现有调用顺序使用 Datalog_Push 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。','每个1ms采样钩子先重读启动标志，仅运行时向RAM日志队列写入本拍记录。')
    s=s.replace('按现有调用顺序使用 BSP_WDT_Feed、Datalog_Stop、Datalog_GetStatus、snprintf\n *     完成本函数处理；条件分支和状态写入均保持函数体所示顺序。','先复制停车首因，有条件喂狗后在前台刷写日志尾包，再完成调参停车收尾和再次喂狗；\n *     按当前日志计数输出一次摘要，最后清待收尾标志并要求按键释放。')
    s=s.replace('按现有调用顺序使用 Gray_ResetSequence、Motor_ResetRunState、PID_ResetAll、Control_ResetRunState\n *     完成本函数处理。','先清控制拍数，再依次清低压计数、停车首因和pending，随后复位灰度序号、\n *     电机历史、PID、双模式控制、旧环岛和IMU运行历史；不清参数、滤波电池或陀螺零偏。')
    s=s.replace('零值不替换已锁存首因。','零值直接返回，不累计保护位或操作硬件。')
    s=s.replace('static uint32_t s_led_last = 0;','/* 停车前台LED软件时基，跨起跑保留，单位ms。 */\n            static uint32_t s_led_last = 0;')
    write(p,s)
p=A/'sensing/battery.c';s=read(p);s=s.replace('#include "bsp.h"',source+'#include "bsp.h"');write(p,s)
p=R/'BSP/bsp_runtime.c';s=read(p).replace('#include "bsp_runtime.h"',source+'#include "bsp_runtime.h"')
s=s.replace(' * @brief 依次启动TIM6控制与TIM7辅助中断。\n',' * @brief 依次启动TIM6控制与TIM7辅助中断。\n * @details 先启动htim6再启动htim7，两次HAL返回值均忽略，不加入回滚或重试。\n')
s=s.replace(' * @brief 依次停止TIM6与TIM7中断。\n',' * @brief 依次停止TIM6与TIM7中断。\n * @details 先停止htim6再停止htim7，两次HAL返回值均忽略，不改变调用者中断屏蔽状态。\n')
s=s.replace('按现有调用顺序使用 App_ControlISR、App_TaskISR 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。','读取句柄实例，TIM6调用2ms控制入口、TIM7调用5ms辅助入口；其他实例无动作。')
write(p,s)
# Root entries are single-call facades; their complete contracts describe that fact.
p=A/'app.c';s=read(p)
for name,target,brief in [('Init','Lifecycle_Init','转发生命周期初始化。'),('Loop','Scheduler_Loop','转发前台调度。'),('ControlISR','Scheduler_ControlISR','转发2ms控制拍。'),('TaskISR','Scheduler_TaskISR','转发5ms辅助拍。'),('SampleISR','Scheduler_SampleISR','转发1ms采样记录。')]:
    pattern=r'/\*\*(?:(?!/\*\*)[\s\S])*?\*/\nvoid App_'+name+r'\(void\)'
    s=re.sub(pattern,'/**\n * @brief '+brief+'\n * @note 调用约束与完整流程见'+('lifecycle.c' if name=='Init' else 'scheduler.c')+'::'+target+'。\n */\nvoid App_'+name+'(void)',s)
write(p,s)
# Only source owner names in the configuration note change.
p=A/'app_config.h';write(p,read(p).replace('g_app_control_ticks>1000','Lifecycle_GetControlTicks()>1000'))
# Preserve aggregate guard consistency while touching its include list.
p=R/'BSP/bsp.h';write(p,read(p).replace('/* __BSP_H */','/* SMARTCAR_BSP_BSP_H */'))
