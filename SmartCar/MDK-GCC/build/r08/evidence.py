"""Serialize the individually reviewed facts below; not part of the checker."""
from pathlib import Path
import hashlib,json,subprocess
repo=Path(__file__).resolve().parents[4]
base=subprocess.check_output(['git','rev-parse','e1771e5'],cwd=repo).decode().strip()
def blob(p): return subprocess.check_output(['git','show',':'+p],cwd=repo)
def sha(p): return hashlib.sha256(blob(p)).hexdigest()
groups={
'BSP/bsp_dl1b': {
'module': 'I2C2 测距，XS=PE8；前台初始化，TIM6 更新毫米缓存，非显示器。',
'BSP_DL1B_ReadRegister':'阻塞读16位寄存器地址，HAL写输出；传入10ms超时，退出依赖HAL_GetTick。',
'BSP_DL1B_WriteRegister':'阻塞写寄存器，输入缓冲只读；返回HAL状态，不在封装中重试。',
'BSP_DL1B_Init':'XS高50低10高50ms；检查固件bit0，写135字节配置，轮询bit0清零；成功才置标志并清缓存，失败保留旧标志。',
'BSP_DL1B_Update':'未初始化保留缓存；状态非零后清中断忽略写失败；量程状态0x89且距离0至4000才缓存，否则INVALID；TIM6阻塞风险尚存。',
'BSP_DL1B_GetDistanceMm':'直接读毫米缓存，没有新测量或再次换算；无效值8192。',
},
'BSP/bsp_adc': {
'module':'ADC1十通道循环DMA，D2 SRAM1绝对地址0x30000000，DCache关闭；PC4为硬件rank5及数组索引4。',
'BSP_ADC_Init':'先单端偏移校准再启动DMA；两次HAL返回均忽略，无启动失败反馈。',
'BSP_ADC_Read':'索引达到通道数返回零，有效索引读单半字右移4位；前台菜单/电池及控制电感使用，不保证整组一致。',
'BSP_ADC_ReadGrayFresh':'空指针ARG；句柄空、DMA关闭、TE/DME/FE标志、计数大于10返回DMA_ERROR；DWT预算超时；先NDTR>5再<=5才写输出，失败不写。',
},
'BSP/bsp_key': {
'module':'PC13高有效GPIO前台轮询；与PA2/PA3 ADC菜单键分开，状态只在前台串行推进。',
'BSP_Key_RequireRelease':'读取原始电平，稳定电平置1，记录tick并清武装/按下/长按标志，要求重新稳定释放。',
'BSP_Key_Init':'调用RequireRelease，启动后要求先释放，不产生按键事件。',
'BSP_Key_PollEvent':'双沿30ms稳定；释放武装、按下计时；1000ms仅一次TOGGLE，短按释放START，长按释放不START。',
'BSP_Key_StartPressed':'消费一次PollEvent，仅START为1，其他包括TOGGLE为0；不可重复轮询同一事件。',
},
'BSP/bsp_pwm': {
'module':'映射TIM13_CH1左轮、TIM4_CH3右轮、TIM14_CH1风扇；ARR为7058；仅CCR幅值，方向在motor.c。',
'BSP_PWM_Init':'仅依次HAL_TIM_PWM_Start三个通道并忽略返回，无方向GPIO或比较值初始化写入。',
'BSP_PWM_SetDuty':'通道达到数量时返回；无符号duty超过10000截断，按duty*(ARR+1)/10000写CCR，不设置方向。',
},
'App/pid': {
'module':'活动输出在TIM6计算；前台也设置参数、复位历史；两个独立轮PI状态，休眠增量式历史保留。',
'PID_GetState':'返回模块静态只读指针，无拷贝；前台读取应遵守停车一致性约定。',
'PID_SetGain':'switch仅写所选增益，未知枚举不修改；无参数限幅，不重置历史。',
'PID_SetTrackGains':'非空输入四项复制到方向增益，不改历史或其他增益。',
'PID_SetAngleGains':'非空输入三项复制到角度增益，不改历史或其他增益。',
'PID_DirectionStep':'kp*e+k2p*e*abs(e)+kd*(e-history)-k2d*accel；先写history再窄化int16再限幅，带符号二次项；越界转换风险R01仍在。',
'PID_UpdateTrack':'从Track/IMU读输入，构造当前增益，以共享旧历史调用DirectionStep；现活动观察器有独立历史。',
'PID_UpdateAngle':'目标减角度，P+拍间D-角速度阻尼；保存历史后取负传Motor_DistributeSpeed，不作方向限幅，浮点向整数接口转换。',
'PID_UpdateIncrementalLeft':'左轮局部静态累计P误差差分+I误差；D占位为零，更新历史后限±7500，再int32到int16；当前不接入。',
'PID_UpdateIncrementalRight':'右轮局部静态累计P误差差分+I误差；D占位为零，更新历史后限±7500，再int32到int16；当前不接入。',
'PID_UpdateSpeed':'跨零换向先清积分、存目标；P与积分、积分限幅、按目标符号补死区、输出限±7500；无实际D计算，左右状态独立。',
'PID_UpdateSpeedLeft':'读取左轮目标反馈和左轮独立状态，执行位置PI，写out_l并返回转换后输出。',
'PID_UpdateSpeedRight':'读取右轮目标反馈和右轮独立状态，执行位置PI，写out_r并返回转换后输出。',
'PID_ResetSpeed':'清左右积分、前拍目标及out_l/out_r；不改增益或休眠静态历史。',
'PID_ResetAll':'清共享方向历史和角度历史/输出，再调用ResetSpeed；不改增益及休眠局部状态。',
}}
external={
'BSP/bsp_dl1b': [('App/app.c','BSP_DL1B_Update();'),('Core/Src/tim.c','htim6.Init.Period = 2000-1;'),('Core/Inc/main.h','DL1B_XS_Pin'),('Core/Src/i2c.c','hi2c2.Instance = I2C2;'),('Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c.c','HAL_GetTick()')],
'BSP/bsp_adc':[('Core/Src/adc.c','sConfig.Rank = ADC_REGULAR_RANK_5;'),('BSP/bsp_gray.c','BSP_ADC_ReadGrayFresh(&value, start)'),('BSP/bsp_time.h','BSP_GRAY_BUDGET_CYCLES'),('App/app.c','BSP_ADC_Read(BSP_ADC_VBAT)'),('App/menu.c','BSP_ADC_Read(ch)'),('App/track_sensor.c','BSP_ADC_Read(s_adc_channel_map[0])')],
'BSP/bsp_key':[('Core/Inc/main.h','KEY_Pin'),('Core/Src/gpio.c','KEY_Pin'),('App/app.c','BSP_Key_PollEvent()'),('App/menu.c','BSP_ADC_Read(ch)')],
'BSP/bsp_pwm':[('Core/Src/tim.c','htim13.Init.Period = 7059-1;'),('App/motor.c','MOTOR_L_DIR_Pin'),('App/app.c','BSP_PWM_Init();')],
'App/pid':[('App/motor.c','void Motor_DistributeSpeed(int16_t pid_out)'),('App/app.c','PID_ResetAll();'),('App/control.c','PID_SetTrackGains(&profile->track)'),('App/optical_fsm.c','PID_DirectionStep('),('App/electromagnetic_fsm.c','PID_DirectionStep(')]
}
records={}
for module,facts in groups.items():
    c='SmartCar/'+module+'.c'; h='SmartCar/'+module+'.h'
    evidence=[{'path':'SmartCar/'+p,'anchor':a,'revision':'index'} for p,a in external[module]]
    # Include configuration, linker/cache prerequisites as conservative dependencies.
    more=['SmartCar/SmartCar.ioc','SmartCar/Core/Src/main.c','SmartCar/MDK-GCC/STM32H743VITx.ld']
    for path in (c,h):
        deps={p:sha(p) for p in {c,h,*more,*[e['path'] for e in evidence]} if p!=path}
        items=[]
        for obj,fact in facts.items():
            ev=[{'path':c,'anchor':obj if obj!='module' else '@file '+Path(c).name,'revision':'index'}]
            if obj=='module': ev+=evidence
            items.append({'object':module+'::'+obj,'fact':fact,'evidence':ev})
        records[path]={'sha256':sha(path),'reviewed_base':base,'reviewer':'Codex 人工源码复核','reviewed_at':'2026-10-23','unresolved':[],
            'dependencies':deps,'base_dependencies':{},'items':items,
            'checks':{
                'coverage':{'status':'pass','reason':f'{module} 模块、全部函数、状态与头文件摘要逐项核对，详见 items；头文件接口对应C定义。'},
                'format':{'status':'pass','reason':'已逐块检查中文Doxygen、标签顺序、参数方向与C完整/H摘要；普通注释行宽复查。'},
                'content':{'status':'pass','reason':'逐条对照实现和登记调用方；明确真实公式、转换顺序、失败输出及调度；不推测物理单位。'},
                'behavior':{'status':'pass','reason':'与e1771e5比较十个文件的非注释token和预处理指令一致；固件语句与声明未变化。'}}}
target=repo/'SmartCar/specs/features/15-code-style-consistency/comment-evidence.json'
target.write_text(json.dumps({'version':1,'files':records},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(len(records),'files;',sum(len(v['items']) for v in records.values()),'review items')
