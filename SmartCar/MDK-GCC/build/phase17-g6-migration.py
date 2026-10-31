from pathlib import Path
import json,sys,subprocess,hashlib
sys.path.insert(0,'SmartCar/tests')
from app_inventory import inventory, mask_comments
from app_migration_check import key
BASE='4745aa2339209d5df9a739449685fc7f1cb66429'
F={
'app':['app','app_types','lifecycle','scheduler','safety','battery','wireless','bsp_runtime','legacy_roundabout'],
'control':['tracking','control_profile','legacy_control','lifecycle','app_types'],
'pid':['pid','direction_controller','angle_controller','speed_controller','legacy_pid','legacy_control'],
'element':['legacy_element','lifecycle'],
'roundabout':['control_profile','legacy_roundabout','tracking','lifecycle'],
'imu_proc':['imu_proc','legacy_imu'],
'param':['parameter_bank','parameter_store','parameter_image','parameter_application','app_types'],
'tuning':['tuning','tuning_values','tuning_application','command_dispatch','app_types'],
'menu':['menu','menu_keys','menu_render'],
'datalog':['datalog','run_record','logger','log_store','log_export','checksum'],
'scb_protocol':['scb_frame','scb_json','scb_command_codec','checksum','tuning_values']}
A={
'App_TryStart':'Lifecycle_TryStart','App_FinalizeStop':'Safety_FinalizeStop','App_ResetRunState':'Lifecycle_ResetRunState','App_AbsFloat':'Roundabout_AbsFloat',
'Tracking_TrySetMode':'Lifecycle_TrySetMode','Param_LoadBanks':'ParamBank_LoadBanks','Param_StoreBanks':'ParamBank_StoreBanks','PID_UpdateSpeed':'SpeedController_Update',
'param_store_v1_t':'param_image_v1_t','param_store_v2_t':'param_image_v2_t','param_store_t':'param_image_v3_t',
'Tuning_IsOrdinary':'TuningApplication_IsOrdinary','Tuning_Merge':'TuningApplication_Merge','Tuning_FinalizeStop':'TuningApplication_FinalizeStop','Tuning_Service':'CommandDispatch_Service','Tuning_Set':'TuningApplication_Set',
'Datalog_LatchFault':'Logger_LatchFault','Datalog_IsHeaderValid':'RunRecord_IsHeaderValid',
'g_app_uart_tx_buffer':'s_uart_tx_buffer','g_app_uart_rx_buffer':'s_uart_rx_buffer',
'g_app_battery_raw':'s_raw','g_app_battery_filtered':'s_filtered','g_app_battery_filter':'s_filter','g_app_low_power_count':'s_low_count',
'g_app_control_ticks':'s_control_ticks','g_app_key_flag':'s_key_flag','g_app_start_flag':'s_start_flag','g_app_run_flag':'s_run_flag','g_app_dl1b_distance_mm':'s_dl1b_distance_mm',
'g_control_track_output':'s_track_output','g_control_kernel_state':'s_kernel_state','g_control_cask_flag':'s_cask_flag',
'g_element_enter_tick':'s_enter_tick','g_menu_page':'s_page','g_menu_arrow':'s_arrow','g_menu_key_adc1':'s_key_adc1','g_menu_key_adc2':'s_key_adc2',
'g_roundabout_state':'s_roundabout_state','g_roundabout_enter_distance1':'s_enter_distance','g_roundabout_out_distance1':'s_out_distance',
'g_roundabout_left_flag':'left_flag','g_roundabout_right_flag':'right_flag','g_roundabout_sign_round':'sign_round',
'g_roundabout_enter_angle1':'enter_angle1','g_roundabout_enter_angle2':'enter_angle2','g_roundabout_out_angle1':'out_angle1'}
for suffix in ['Begin','Send','Position','ReadBank','QueryParams','QueryStatus','QueryResult','QueryLogMeta','LogCommand','HandleCommand']:
 A['Tuning_'+suffix]='CommandDispatch_'+suffix
CONTEXT={
'lifecycle':'前台初始化、起跑、模式协调；TIM6更新运行拍与测距，停车路径经命令清标志；保留volatile及原门禁。',
'scheduler':'前台总循环与TIM15/6/7编排；只持有停车LED刷新时基，模块状态由各owner更新。',
'safety':'前台/ISR锁首因、累计保护、切输出；TIM7计低压，阻塞日志和保存仅前台收尾。',
'battery':'停车前台或TIM7更新raw/filtered与唯一滤波器；起跑不清滤波历史。',
'parameter_application':'前台加载、参数应用与保存快照协调；不持有镜像、bank或PID历史。',
'tuning_application':'前台原短临界区捕获上下文、停车合入与保存；TIM6相邻同步版本与事件。',
'tracking':'TIM6推进双模式并提交唯一控制拍；停车前台预览另用监视量，起跑清历史而保留模式档位。',
'control_profile':'私有只读部分配置与固定距离门限；观察读表，执行按fields顺序应用，不持有FSM历史。',
'pid':'增益与可见输出唯一owner；前台参数操作、TIM6执行以及停车复位使用明确接口。',
'direction_controller':'不持有状态；各FSM/旧方向调用者提供独立int32历史，控制拍推进。',
'angle_controller':'独占角度前拍误差，显式输入计算；仅旧执行适配有原Motor调用，算法不写硬件。',
'speed_controller':'独立左右轮PI历史；TIM6按左后右推进，安全停车和起跑通过原复位顺序清活动历史。',
'legacy_pid':'旧方向预览历史与各轮休眠增量静态历史；ResetAll只清旧方向，不清增量历史。',
'legacy_roundabout':'旧标志/旧角度参数由本模块唯一持有；原入口保留内部调用，活动双模式不接入；复位壳仍按原调用。',
'legacy_element':'旧元素进入拍唯一owner；保留旧判断调用图、int16截断与空壳，不新增活动调用。',
'legacy_control':'旧整体执行及角度到电机适配；保留既有调用效果，不接入活动双模式链。',
'optical_fsm':'TIM6独占光电状态及方向历史，与电磁隔离；起跑复位保留原base_speed。',
'electromagnetic_fsm':'TIM6独占电磁观察状态、左右距离、角度及方向历史；输出意图由选中模式执行。',
'motor':'TIM6独占目标/反馈/距离与独立轮速滤波，前台参数设置；安全停车直接经BSP清输出。',
'track_sensor':'TIM6或停车前台采样/观察发布；四路八次去极值数据由本模块持有；纯误差接受显式数据。',
'gray_sensor':'TIM6或停车前台整帧更新；失败保留上一帧，标定与质心仍由本模块处理。',
'gray_calibration':'静态只读手工标定；32项仍为-1，需真实测量后另行填写，两模式起跑均受校验门禁。',
'filter':'无业务状态；调用方独占所传滤波历史，显式输入与固定浮点运算顺序。',
'imu_proc':'同一IMU状态与所有零偏/积分/高通历史owner；TIM6更新，前台启动校准，起跑复位保留零偏。',
'legacy_imu':'无私有IMU副本；只使用imu_proc显式传入的同一状态/偏置/历史，旧入口仍未接入活动链。',
'parameter_bank':'四个mode/gear方向bank唯一owner；前台装载/设置、控制读取，镜像通过显式参数复制。',
'parameter_store':'参数镜像、dirty/时基及Flash可用位唯一owner；前台迁移/保存；失败副作用和版本字节保持。',
'parameter_image':'固定v1/v2/v3布局与保存快照类型；不定义可写实例，不依赖参数使用者。',
'tuning':'baseline/pending/last_success/effective/revision/final各自唯一；TIM6完整拍提交，前台短临界区查询。',
'tuning_values':'纯数值校验及业务值类型；不持有状态，无协议、事务或硬件依赖。',
'command_dispatch':'前台协议路由/结果工作区及注册回调；起停和调参事件在原发生上下文适配至唯一SCB队列。',
'run_record':'TIM6发布唯一96B完整拍，TIM15复制并组装样本；前台装配唯一256B当前头。',
'logger':'TIM15单写head、前台单写tail；1024槽实际可用1023，8条批次Flash成功后释放，首故障/状态唯一。',
'log_store':'前台唯一日志Flash后端；保留256B写后回读校验临时对象，不持有会话或统计。',
'log_export':'前台串行CSV/二进制导出、身份/租期/968B唯一缓存；只读logger状态，不拥有SCB结果。',
'datalog':'公开固定记录/日志业务类型与API聚合声明，不定义可写实例或SCB别名。',
'scb_frame':'纯帧字节与CRC包装；不持有会话或业务状态。',
'checksum':'纯CRC32更新，previous显式输入；无状态或协议依赖。',
'scb_json':'前台不可重入有界JSON机制，保留唯一静态8层工作栈。',
'scb_command_codec':'前台命令解析工作区与业务字段编码；通过纯业务validator校验，无业务执行。',
'scb_link':'前台会话/可靠单窗口/32请求缓存；事件原队列并发发布，重试ACK去重语义不变。',
'telemetry':'TIM15单写生产头/index/drop，前台单写消费尾/drop；256槽独立于Flash队列。',
'wireless':'停车前台旧字符协议解析与两份32B串口缓冲owner；保留校验/钳位与延迟保存差异。',
'menu':'停车前台页面/光标/编辑/按键沿owner；业务操作通过协调API，BACK保存先于导航。',
'menu_keys':'停车前台ADC均值、独立两路滤波、按键阈值owner；第二路仍覆盖第一路。',
'menu_render':'停车前台只读业务绘图与显示缓存owner；RST早退后下一拍清屏语义保持。',
'dashboard':'停车前台仪表显示与刷新时基owner；两种LCD配置依旧各自原输出。',
'app':'固定App外部入口转发系统协调器，无私有运行业务状态。',
'app_types':'跨业务模式/档位/停车原因及调参值类型，枚举数值和字段宽度保持。',
'app_config':'公共编译配置，原数值/预算与条件分支保持。',
'bsp_runtime':'板级时钟、LED、定时器与HAL固定回调适配；按原位置被系统编排调用。',
'bsp_time':'板级毫秒/周期、PRIMASK与DMB适配；不保存App状态，不改变临界区。',
'bsp_pwm':'板级方向物理电平/PWM适配，保持左右方向极性和原调用顺序。'}
V={ 'app':['g2_system','dual_mode_tracking::app_tests'], 'system':['g2_system','runtime_tuning','dual_mode_tracking::app_tests'],
'sensing':['control_equivalence','dual_mode_tracking::gray_tests','g3_control'],
'control':['control_equivalence','dual_mode_tracking::fsm_tests','runtime_tuning','g3_control'],
'services':['datalog_v2','scb_protocol','scb_link','spi_log_export','runtime_tuning','dual_mode_tracking::param_tests'],
'hmi':['g4_hmi','dual_mode_tracking::param_tests'], 'BSP':['g2_system','spi_wifi_state']}
old=[]
for name in subprocess.check_output(['git','ls-tree','-r','--name-only',BASE,'SmartCar/App'],text=True).splitlines():
 if name.endswith(('.c','.h')):old+=inventory(name,subprocess.check_output(['git','show',BASE+':'+name]).decode('utf-8'))
paths=sorted([p for p in Path('SmartCar/App').rglob('*') if p.suffix in ('.c','.h')]+[Path('SmartCar/BSP')/(stem+'.'+ext) for stem in ['bsp_runtime','bsp_time','bsp_pwm'] for ext in ['c','h']])
new=[];digests={}
for p in paths:
 raw=p.read_bytes();digests[p.as_posix()]=hashlib.sha256(raw.replace(b'\r\n',b'\n')).hexdigest();new+=inventory(p.as_posix(),raw.decode())

def validations(p):
 parts=Path(p).parts
 return V['BSP'] if parts[1]=='BSP' else V.get(parts[2],V['app'])

bind=[];missing=[]
for r in old:
 stem=Path(r['path']).stem;family=F.get(stem,[stem]);name=A.get(r['symbol'],r['symbol']);scope=A.get(r.get('scope',''),r.get('scope',''))
 if r['kind']=='static_local' and stem=='app' and scope=='App_Loop':scope='Scheduler_Loop'
 if r['kind']=='member' and scope=='tuning_result_t' and name=='status':name='code'
 candidates=[n for n in new if n['symbol']==name and Path(n['path']).stem in family]
 if r['symbol'].startswith('g_roundabout_') and name in ['left_flag','right_flag','sign_round','enter_angle1','enter_angle2','out_angle1']:
  candidates=[n for n in candidates if n.get('scope')=='legacy_roundabout_state_t']
 if r['kind']=='member':candidates=[n for n in candidates if n['kind']=='member' and n.get('scope')==scope]
 elif r['kind'] in ['function','declaration']:candidates=[n for n in candidates if n['kind'] in ['function','declaration']]
 elif r['kind']=='static_local':candidates=[n for n in candidates if n['kind']=='static_local' and n.get('scope')==scope]
 elif r['kind']=='object':candidates=[n for n in candidates if n['kind'] in ['object','member']]
 else:candidates=[n for n in candidates if n['kind']==r['kind']]
 if r['kind']=='static_local' and stem=='datalog' and r['symbol']=='s_check':
  candidates=[n for n in new if n['symbol']=='s_check' and n['kind']=='object' and n['path']=='SmartCar/App/services/log_store.c']
 reason='按已提交G2～G5 owner契约绑定同名真实声明/定义；原上下文和字节行为由对应回归约束。'
 if r['kind'] in ('macro','branch') and ('SMARTCAR_APP_' in r['symbol']):
  candidates=[n for n in new if Path(n['path']).stem in family and n['kind']==r['kind'] and ('SMARTCAR_APP_' in n['symbol'])]
  reason='旧聚合模块拆分为各owner头，保护宏/对应条件入口按最终头分别保留，不影响业务编译条件。'
 if r['symbol']=='APP_LED_RUN_LEVEL':
  candidates=[n for n in new if n['kind']=='function' and n['symbol']=='BSP_Runtime_SetRunLed']
  reason='原GPIO_PIN_SET宏使用点收拢为BSP运行灯适配，函数内保留相同物理电平。'
 if r['kind'] in ('function','declaration'):
  expanded={
   'App_Init':['Lifecycle_Init'],'App_Loop':['Scheduler_Loop'],'App_ControlISR':['Scheduler_ControlISR'],
   'App_TaskISR':['Scheduler_TaskISR'],'App_SampleISR':['Scheduler_SampleISR'],
   'App_RequestStop':['Safety_RequestStop'],'App_IsStopped':['Lifecycle_IsStopped'],
   'Tracking_TrySetMode':['Tracking_SetMode'],
   'Param_Load':['ParamStore_Load'],'Param_SetDirection':['ParamBank_SetDirection'],
   'Param_TryFlushPending':['ParamStore_TryFlushPending'],
   'Tuning_Init':['CommandDispatch_Init'],'Tuning_ResetSession':['CommandDispatch_ResetSession'],
   'Tuning_Service':['TuningApplication_ServiceSave','Tuning_TakeResult'],
   'Tuning_Set':['CommandDispatch_Set','Tuning_PrepareSet','Tuning_CommitParked'],
   'Tuning_Switch':['TuningApplication_Switch','CommandDispatch_Switch'],
   'Tuning_Event':['CommandDispatch_QueueTuningEvent','Tuning_CopyEventValues'],
   'Menu_Update':['MenuRender_Reset','MenuRender_PreparePage','MenuRender_Draw'],
   'PID_UpdateAngle':['PID_CalculateAngle','AngleController_Update'],'PID_UpdateTrack':['PID_DirectionStep'],
   'IMU_UpdateAccelVelocity':['LegacyIMU_UpdateAccelVelocity'],'IMU_CalibrateAccel':['LegacyIMU_CalibrateAccel'],
   'Datalog_Start':['RunRecord_BuildHeader','LogStore_WriteHeader','LogStore_Erase'],
   'Datalog_Init':['RunRecord_ResetSnapshot'],'Datalog_Push':['RunRecord_CaptureSample'],
   'SCB_CalculateCrc':['Checksum_UpdateCrc32'],'SCB_ValidateValues':['Tuning_ValidateValues']}
  for symbol in expanded.get(r['symbol'],[]):
   actual=[n for n in new if n['symbol']==symbol and n['kind']=='function']
   assert actual,('missing split implementation',symbol)
   candidates+=actual
  if r['symbol'] in expanded:reason='原函数的包装、业务机制与跨模块操作按已提交契约拆到下列真实实现；分别定位全部拆分部分。'
 if not candidates:missing.append(r);continue
 bind.append(dict(original=key(r),targets=[key(n) for n in candidates],reason=reason,validation=validations(candidates[0]['path'])))
print('original',len(old),'current',len(new),'missing',len(missing))
for r in missing:print(r['kind'],r['path'],r.get('scope',''),r['symbol'])
Path('SmartCar/MDK-GCC/build/phase17-map-missing.json').write_text(json.dumps(missing,ensure_ascii=False,indent=2),encoding='utf-8')
if missing:sys.exit(1)
external_sources={}
for folder in ['SmartCar/BSP','SmartCar/Core/Src']:
 for p in Path(folder).rglob('*.c'):
  if p.as_posix() not in digests:external_sources[p.as_posix()]=mask_comments(p.read_text(encoding='utf-8'))
owners=[]
for row in new:
 stem=Path(row['path']).stem;context=CONTEXT[stem]
 callers=[key(n) for n in new if n['kind']=='function' and row['symbol'] in n.get('calls',[])] if row['kind'] in ['function','declaration'] else []
 referenced_by=[key(n) for n in new if n['kind']=='function' and row['symbol'] in n.get('references',[]) and row['symbol'] not in n.get('calls',[])] if row['kind'] in ['function','declaration'] else []
 implementations=[n for n in new if n['kind']=='function' and n['symbol']==row['symbol']] if row['kind']=='declaration' else []
 owner=implementations[0]['path'] if len(implementations)==1 else row['path']
 external_call_sites=[]
 if row['kind'] in ['function','declaration']:
  for path,source in external_sources.items():
   for m in __import__('re').finditer(r'\b'+__import__('re').escape(row['symbol'])+r'\s*\(',source):
    external_call_sites.append(dict(path=path,line=source[:m.start()].count('\n')+1))
 owners.append(dict(item=key(row),owner=owner,context=context,external_call_sites=external_call_sites,callers=callers,referenced_by=referenced_by,implementations=[key(n) for n in implementations],validation=validations(owner)))
result=dict(baseline=BASE,original=old,current_sha256=digests,current=new,migrations=bind,owners=owners)
Path('SmartCar/specs/features/17-app-architecture/final-symbol-map.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
