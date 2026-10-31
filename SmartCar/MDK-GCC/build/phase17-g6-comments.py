from pathlib import Path
import sys,re
sys.path.insert(0,'SmartCar/tests')
from app_inventory import mask_comments,inventory
D={
'sensing/filter.c':{
'Filter_InitLowpass':'保存调用方直接给定的 alpha，并清零历史输出；本入口不从截止频率推导系数。',
'Filter_UpdateLowpass':'先以 alpha*input+(1-alpha)*last_output 覆盖历史，再返回同一输出；只写调用方状态。',
'Filter_InitHighpass':'先按 rc=1/(2*pi*cutoff_hz) 求时间常数，再取 rc/(rc+dt_s)；输入及输出历史均清零。',
'Filter_UpdateHighpass':'先计算 alpha*(上拍输出+本拍输入-上拍输入)，再保存本拍输入；返回新输出。'},
'sensing/track_sensor.c':{
'Track_CalculateError':'以横差和竖差的加权和为分子，横电感和与竖差绝对值的加权和为分母；\n *     分母为零直接返回0，否则先乘100、再整数除法并窄化至int16_t，不修改传感器状态。',
'Track_PublishObservation':'先复制本拍权重，再写偏差和横、竖不对称百分比；原始值及滤波值均保持。',
'Track_UpdateSymmetry':'分别用横、竖两侧差的绝对值乘100除以对应和；各自和为零时发布0，不改变偏差。',
'Track_ReadAdc':'每轮按横左、竖左、竖右、横右读取DMA当前值，共八轮；各通道独立累计、\n *     去一次最大和最小后除以6，整数结果写filtered，raw保留最后一轮，内部不等待新转换。'},
'sensing/imu_proc.c':{
'IMU_ZeroAngleX':'仅把共享X轴累计角清零；本拍角增量及上一拍角速度仍可供独立电磁观察器使用。',
'IMU_Init':'先调用板级IMU初始化，再以0.2Hz和APP_CTRL_DT_S初始化X/Y两份高通历史；\n *     板级返回值在此入口不处理，X实例保留而活动更新只使用Y实例。',
'IMU_UpdateAccelVelocity':'把本模块同一状态、三轴加速度零偏及积分历史地址交给legacy_imu；\n *     不复制第二套历史，也不在活动调度中新增调用，保留旧读取源与计算字段不一致的语义。',
'IMU_CalibrateAccel':'把本模块同一IMU状态及三轴加速度零偏地址交给休眠标定实现；\n *     一百次直接采样的结果写回原零偏，原积分量清零副作用仍落在同一状态。',
'IMU_CalibrateGyro':'连续读取一百组三轴陀螺原始值，以int32_t累加后分别转float求平均零偏；\n *     随后清三轴累计角，不清梯形历史，也不在循环内增加等待或采样同步。',
'IMU_ResetRunState':'依次清本拍增量、加速度、角速度、速度、角度和两组梯形历史，再重置两个高通；\n *     原始采样缓存及六项上电零偏保留，再次起跑不触发SPI读取或校准。'},
'sensing/gray_sensor.c':{
'Gray_ValidateCalibration':'逐路检查黑白值均在0..4095且绝对差至少200；每条失败只设置其掩码位，\n *     遍历完十六路后一次返回完整失败位图，不读取模块状态。',
'Gray_CheckCalibration':'先由静态黑白表求失败位图并更新总有效标志；report非零才按通道顺序输出\n *     每条错误，报告不影响返回值，不修改标定表。',
'Gray_Normalize':'先拒绝范围无效或对比度不足的标定，再以1000*(raw-black)/(white-black)\n *     做有符号整数归一化；低于50清零、高于1000截顶，其余返回千分强度，不写模块状态。',
'Gray_ResetSequence':'先复位BSP帧序号，再清App已发布序号；原始值、耗时、偏差和有效标志均保留。',
'Gray_Init':'先清唯一灰度状态，再初始化完整帧采集BSP，最后执行带UART报告的标定校验；\n *     初始化不会自动填写或修正手工黑白表。'},
'control/control_profile.c':{
'Control_ApplyProfile':'按权重、基础速度、方向增益、角度增益的次序检查四个独立fields位，\n *     只写被选中的组；未选组继续沿用前一拍，增益应用不清控制器历史。',
'Control_MergeProfile':'依次把src位图选中的四组配置复制到dst，最后按位或累计dst.fields；\n *     未选组保留dst原值，供观察器合成后续实际执行配置，不访问硬件。'},
'control/motor.c':{
'Motor_SetTargets':'依次写左右轮目标，不读取反馈、不运行PI，也不向BSP发布PWM。',
'Motor_SetBaseSpeed':'只写差速分配所用基础速度；当前左右目标和PWM保留，下一次分配时才使用新值。',
'Motor_SetFanDuty':'只更新运行风扇设定；实际PWM仍由运行生命周期和控制拍的既有调用点写出。',
'Motor_SetFanDutyIdle':'只更新停车风扇设定，不隐式切换运行状态或写风扇PWM。',
'Motor_ResetDistance':'依次清左轮、右轮及平均累计距离，不清反馈、目标或采样器累计计数。',
'Motor_DistributeSpeed':'非负方向量使左轮减速、右轮加速；负值时反向分配。内侧用3/2倍、\n *     外侧用1倍，先乘后整数除法并写int16_t目标，不额外限幅或执行速度PI。',
'Motor_UpdateControl':'先用两轮各自目标与反馈计算左右PI，再依次发布左轮、右轮的方向和绝对PWM；\n *     左轮非负输出置高、右轮非负输出置低，负输出取反后转uint32_t，不交换硬件写入顺序。',
'Motor_ReadEncoder':'先读清左右采样累计量，再分别经过独立低通并窄化为int16_t反馈；\n *     将各反馈累计到各自距离，最后以两侧累计和的整数平均发布总距离。',
'Motor_Init':'先初始化电机和风扇PWM，再依次将左右方向脚置高；不运行PI或恢复轮速目标。',
'Motor_InitEncoder':'先初始化编码器，再以当前alpha分别清左右低通历史；不重写基础速度或风扇参数。',
'Motor_ResetRunState':'先清两轮目标和反馈，再清两侧及平均距离，最后分别清低通历史；\n *     不重复初始化编码器，不清PI历史，PI由生命周期相邻的专属复位入口处理。',
'Motor_EmergencyStop':'先清两轮目标并复位活动速度PI，随后左、右PWM置零；\n *     不等待下个控制拍，不改变方向脚、反馈距离或风扇，风扇由安全协调器处理。'},
'control/electromagnetic_fsm.c':{
'ElectromagneticFSM_Reset':'清完整私有电磁上下文和独立方向历史，再恢复15/20/22/10权重；\n *     方向计算许可由下一次Select刷新，不写共享执行参数。',
'ElectromagneticFSM_ClearDistance':'先清私有左右累计距离，再设置共享距离复位请求；\n *     同一观察拍后续判断读取清零后的距离，实际电机距离由tracking按选中模式执行复位。',
'ElectromagneticFSM_ClearAngle':'把私有累计角清零并置zero_angle请求；观察层不直接修改共享IMU角。',
'ElectromagneticFSM_Measure':'先以横、竖信号差绝对值占各自和的百分比求不对称度，和为零对应发布0；\n *     再用当前私有配置权重求电感归一化误差，结果只写电磁上下文。'},
'control/optical_fsm.c':{
'OpticalFSM_Reset':'恢复WAIT并清偏差、方向输出和本模式方向历史；基础速度保留，\n *     本拍计算许可仍由Select重新决定，不访问另一模式状态。'},
'control/legacy_element.c':{
'Element_Judge':'按左环进入、右环进入、十字、跷跷板四个旧入口依次判断；\n *     这些调用之间不增加互斥或早退，后续判断仍可能覆写前一判断选择的主状态。',
'Element_JudgeCrossroads':'竖电感和严格大于2800且竖不对称度严格小于25时写十字主态；条件不满足不写状态。',
'Element_JudgeCrossroadsExit':'竖电感和严格小于2800时恢复普通循迹主态；等于门限时保留当前状态。',
'Element_JudgeTeeterboard':'四电感和严格小于800时选择跷跷板，并把当前2ms拍计数窄化存入int16_t进入时间。',
'Element_JudgeTeeterboardExit':'四电感和严格大于1000时恢复循迹，随后独立检查当前拍减进入拍是否大于300；\n *     两个if均可执行，进入时间仍使用原int16_t截断值，不合并退出条件。',
'Element_JudgeCask':'共享Y轴累计角严格大于60时写路障主态；不在本入口清角或执行电机控制。',
'Element_JudgeCaskExit':'共享Y轴累计角严格小于20时恢复普通循迹主态；其余输入不改变状态。'}
}
for name,entries in D.items():
 p=Path('SmartCar/App')/name;raw=p.read_bytes();text=raw.decode();original=text
 rows={r['symbol']:r for r in inventory(name,text) if r['kind']=='function'}
 changes=[]
 for fn,detail in entries.items():
  row=rows[fn];end=sum(len(line) for line in text.splitlines(keepends=True)[:row['line']-1]);start=text.rfind('/**',0,end)
  block=text[start:end];old=re.search(r' \* @details .*?(?=\r?\n \* @|\r?\n \*/)',block,re.S)
  assert old,(name,fn)
  new=' * @details '+detail
  if '\r\n' in text:new=new.replace('\n','\r\n')
  changes.append((start+old.start(),start+old.end(),new))
 for a,b,value in sorted(changes,reverse=True):text=text[:a]+value+text[b:]
 if name=='sensing/track_sensor.c':
  text=text.replace('a 为调用方提供的只读输入，必须指向满足接口容量要求的有效对象。','非空四路滤波电感，顺序为横左、竖左、竖右、横右。').replace('w 为调用方提供的只读输入，必须指向满足接口容量要求的有效对象。','非空四项误差权重；不在此入口校验系数范围。')
 if name=='control/control_profile.c':
  text=text.replace('@param[out] dst dst 为调用方提供的输出对象，必须非空；函数按实现顺序写入。','@param[in,out] dst 非空观察配置；未选字段和已有位图继续保留。').replace('src 为调用方提供的只读输入，必须指向满足接口容量要求的有效对象。','非空部分配置，以fields选择四组中的待覆写组。')
 assert re.sub(r'\s+','',mask_comments(text))==re.sub(r'\s+','',mask_comments(original)),name
 p.write_bytes(text.encode())
 print(name,len(entries),'comment-only')
