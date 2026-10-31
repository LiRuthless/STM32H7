from pathlib import Path
import re
R=Path(__file__).resolve().parents[2];A=R/'App'
def wr(p,s):p.write_bytes((s.rstrip()+'\n').replace('\n','\r\n').encode('utf-8'))
def doc(p,n,details=None,extra=None,brief=None,note=None):
    s=p.read_text(encoding='utf-8');m=re.search(r'/\*\*\n(?:(?!\*/).)*?\*/\n(?=(?:static )?[\w *]+\b'+n+r'\([^;]*?\)\n\{)',s,re.S)
    assert m,(p,n)
    d=m.group()
    if details is not None:
        d=re.sub(r' \* @details[^@]*?(?= \* @| \*/)',(' * @details '+details+'\n') if details else '',d)
        if details and '@details' not in d:d=d.replace('\n * @', '\n * @',1).replace('\n * @param','\n * @details '+details+'\n * @param',1) if '@param' in d else d.replace('\n */','\n * @details '+details+'\n */')
    if brief:d=re.sub(r' \* @brief[^\n]*',' * @brief '+brief,d)
    if note is not None:
        d=re.sub(r' \* @note[^@]*?(?= \* @| \*/)',' * @note '+note+'\n' if note else '',d)
    if extra:
        # append new params before return/pre/note in fixed tag order
        idx=min([d.find(' * @'+tag) for tag in ['return','pre','note'] if ' * @'+tag in d] or [d.rfind(' */')])
        d=d[:idx]+extra+d[idx:]
    wr(p,s[:m.start()]+d+s[m.end():])
P=A/'services/parameter_bank.c'
doc(P,'ParamBank_LoadBanks','依次把镜像中的电磁低高档、光电低高档字段复制到唯一四bank；不保留镜像地址。',' * @param[in] image 非空v3镜像，只读其中四组方向字段。\n')
doc(P,'ParamBank_StoreBanks','依次复制四bank到调用方镜像，再将两个光电保留字段写零；不读活动控制值。',' * @param[in,out] image 非空v3镜像，仅写四组方向字段和两个光电保留字。\n')
doc(P,'Param_GetDirection','')
P=A/'services/parameter_store.c'
for n,d in {
'Param_CalculateCrc16':'逐字节高位先异或，再执行8次条件移位；初值ffff、多项式1021，返回16位结果。',
'Param_CalculateCrcV1':'从字节8开始只覆盖44字节，排除magic、version及crc16字段。',
'Param_CalculateCrcV2':'从字节8开始只覆盖48字节，包含v2新增的空闲风扇字段。',
'Param_CalculateCrcV3':'从字节8开始只覆盖80字节，包含两组光电参数及保留字段。',
'Param_ClampFanDuty':'输入小于0返回0，大于10000返回10000，否则返回原int16_t值；不修改镜像。',
'Param_SetOpticalDefaults':'两档依次设置kp=2、k2p=0.008、kd=15、速度100，并清两个保留字；\n *     仅默认加载和旧版本迁移使用，不初始化电磁方向字段。',
'Param_SetDefaults':'清整份88字节镜像，依次装魔数、版本、schema页面/光标、共享PI和两档风扇默认值，\n *     最后补光电默认；清零后的电磁方向字段不猜测可跑值。',
'Param_WriteMirror':'从四bank装方向字段，依次写魔数、版本、CRC后向地址0写88字节；\n *     写错只打印原错误文本并返回0，不回滚已整理的RAM镜像。',
'Param_GetFan':'档位越界返回0；有效档位按idle零/非零选择运行或空闲镜像槽，只读取不应用。',
'Param_MarkDirty':'先置脏，再读取一次BSP毫秒tick作为3000ms静默保存起点；不写Flash。',
'ParamStore_Load':'先清脏并装默认RAM，再读8字节头并按v1/v2/v3完整读取和CRC判断；\n *     有效旧版补缺失字段，无效数据保留默认并要求重写，读取错误不重写。\n *     随后依次钳位四风扇、装四bank和锁存可用性；必要重写失败锁存不可用，RAM仍可读取。',
'ParamStore_SetFan':'档位越界无操作；有效值先钳位到0..10000，再按档位及idle选择唯一镜像槽写入。',
'ParamStore_SetSpeedGains':'依次写镜像kp_v、ki_v，不改活动PID或脏标志；应用及标脏由system负责。',
'ParamStore_TryFlushPending':'Flash不可用立即0，无脏立即1且不读输入；需写时依次复制快照PI、页面、光标，\n *     再整理bank并写镜像。失败锁存不可用且保留脏，成功才清脏。',
}.items():doc(P,n,d)
doc(P,'ParamStore_SetFan',brief='只更新指定档位的运行或空闲风扇镜像。',note='仅前台镜像写入，不应用活动风扇、不标脏、不直接写PWM。')
doc(P,'ParamStore_SetSpeedGains',brief='只更新共享速度PI镜像。',note='仅前台写入，不调用控制器，不复位历史或访问Flash。')
doc(P,'ParamStore_TryFlushPending',extra=' * @param[in] runtime 非空本次保存快照，仅在可用且脏时读取四字段，不保存地址。\n')
P=A/'system/parameter_application.c'
for n,d in {
'Param_ApplyCurrentDirection':'分别读取当前模式和档位，取得bank后先写方向四增益（阻尼固定1），再写基础速度；\n *     不复位历史，也不把元素临时增益回写bank。',
'Param_SetDirection':'非法目标或空输入由bank拒绝且无副作用；成功复制后先标脏，再比较当前模式/档位，\n *     匹配才应用方向和基础速度，保持标脏先于活动值变更。',
'Param_SelectGear':'非法档位直接返回；有效时依次写选中gear、应用方向、写运行风扇和空闲风扇；\n *     不标脏、不更换模式或清PI历史。',
'Param_Save':'请求立即保存只置脏而不更新时间，然后尝试写入并忽略返回；失败仍由store锁存。',
'Param_ServiceSave':'只有store判断有脏且静默满3000ms时才尝试写入；无脏路径不读取时钟。',
'Param_TryFlushPending':'先检查可用性，再检查脏标志；任一无需保存路径均不读取PID或菜单。\n *     真正保存时按kp_v、ki_v、page、arrow顺序采集瞬时快照，交镜像owner写入。',
'Param_SetFan':'先校验档位，再由store钳位并写目标镜像；当前档位匹配才按idle分支更新活动值，\n *     最后标脏。其余风扇槽和控制历史保持。',
}.items():doc(P,n,d)
P=A/'hmi/menu_keys.c'
doc(P,'Menu_ReadKeyAverage','每路连续执行三次BSP_ADC_Read，以uint32_t累加后整数除3；不延时或触发新转换。')
doc(P,'Menu_ScanKey','首次扫描以alpha=0.7初始化两个显示滤波器；先读路1三次，再读路2三次。\n *     用未滤波ADC除100后的严格开区间判键，路2可覆盖路1；最后才更新两路显示滤波值。')
P=A/'hmi/menu.c'
doc(P,'Menu_HandleKey','入口先复制当前模式、档位及bank，即使只是导航也执行该读取。\n *     浏览按页面项数循环，BACK调参先保存旧位置再回主页；左右编辑保留五个独立分支和重复标脏。\n *     调参快捷键先切页面、应用档位，再选择运行风扇及编辑模式，保留原光标。')
doc(P,'Menu_Update','先扫描并处理无键沿；RST新沿清屏失效后立即返回，当拍不导航、保存或绘制。\n *     其余新沿先处理业务，再准备页面；有菜单脏且松键才保存，最后依次绘制内容和光标。')
P=A/'hmi/menu_render.c'
doc(P,'Menu_DrawContent','按当前页面的固定坐标、颜色和数值位宽输出LCD调用；方向、速度和风扇显示活动值，\n *     ADC显示取按键owner滤波值。仅绘制不改bank、页面、编辑状态或按键沿。')
doc(P,'Menu_DrawCursor','仅页面或位置变化时执行；按原页面行数用空格擦旧光标，再画新光标并更新arrow缓存，\n *     不修复非法位置，不改变页面导航。')
P=A/'control/tuning.c'
doc(P,'Tuning_Init','只清调参状态、最终邮箱及pending/成功/脏/暂存/过渡标志，再读取活动值作为effective；\n *     不清baseline、成功值存储或目标历史，不注册通信回调，不在起跑调用。',brief='上电初始化调参事务。')
doc(P,'Tuning_SelectControl',extra=' * @param[in] context 非空同拍业务输入，读取模式、档位、启动许可和累计保护位。\n')
doc(P,'Tuning_CompleteControl',extra=' * @param[in] context 非空已完成拍输入，成功时固定其中采样tick和完整序号。\n')
P=A/'services/command_dispatch.c'
doc(P,'CommandDispatch_QueueTuningEvent','先取上传run_id，再取采样tick和完整序号，随后显式映射事件种类和状态；\n *     在原位置读取版本及成功/有效值，字段复制后入原事件队列，队满仍忽略返回。',brief='把即时业务事件映射为SCB事件描述。')
s=P.read_text(encoding='utf-8');m=re.search(r'/\*\*\n(?:(?!\*/).)*?\*/\n(?=void CommandDispatch_QueueTuningEvent)',s,re.S);d=m.group();d=re.sub(r' \* @param\[in\] (kind|status)[^\n]*\n','',d);d=d.replace(' * @note',' * @param[in] business 非空即时业务事件，只含kind/code，当前上下文同步适配。\n * @note',1);wr(P,s[:m.start()]+d+s[m.end():])
# Remove the duplicate host-only declaration; the mode API is always public.
p=A/'system/lifecycle.h';s=p.read_text(encoding='utf-8');start=s.index('#if defined(SMARTCAR_HOST_TEST)');end=s.index('#endif',start)
sec=s[start:end];sec=re.sub(r'/\*\*\n(?:(?!\*/).)*?\*/\ntracking_result_t Lifecycle_TrySetMode\(uint8_t mode\);\n','',sec,flags=re.S);wr(p,s[:start]+sec+s[end:])
# Correct header briefs by reading the actual corresponding implementation, avoiding generic placeholders.
newmods=['services/parameter_bank','services/parameter_store','system/parameter_application','control/tuning','control/tuning_values','services/command_dispatch','system/tuning_application','hmi/menu','hmi/menu_keys','hmi/menu_render']
for module in newmods:
    cp=A/(module+'.c');hp=A/(module+'.h');cs=cp.read_text(encoding='utf-8');hs=hp.read_text(encoding='utf-8')
    for match in list(re.finditer(r'/\*\*\n(?:(?!\*/).)*?\*/\n(?=[\w *]+\b(\w+)\([^;]+;)',hs,re.S))[::-1]:
        n=match[1];cm=re.search(r'/\*\*\n(?:(?!\*/).)*?\*/\n(?=(?:static )?[\w *]+\b'+n+r'\([^;]*?\)\n\{)',cs,re.S)
        assert cm,(module,n)
        brief=re.search(r'@brief ([^\n]+)',cm.group())[1]
        new='/**\n * @brief '+brief+'\n * @note 完整契约见 '+Path(module).name+'.c::'+n+'。\n */\n'
        hs=hs[:match.start()]+new+hs[match.end():]
    wr(hp,hs)
# Format only G4-owned modules, including newly created untracked files.
owned=newmods+['services/parameter_image']
for module in owned:
    for ext in ['.c','.h']:
        p=A/(module+ext)
        if not p.exists():continue
        s=p.read_text(encoding='utf-8');lines=[]
        for line in s.splitlines():
            if line.lstrip().startswith('*') and len(line)>100:
                while len(line)>100:
                    cut=line.rfind(' ',10,98)
                    if cut<10:cut=96
                    lines.append(line[:cut]);line=' *     '+line[cut:].lstrip()
            if len(line)>100 and not line.lstrip().startswith(('#','*','/*')) and '(' in line and line.rstrip().endswith((')',');')) and '"' not in line:
                # Split long declarations on commas without changing tokens.
                parts=line.split(', ');current=parts[0]
                for part in parts[1:]:
                    if len(current)+len(part)+2>98:lines.append(current+',');current='    '+part
                    else:current+=', '+part
                line=current
            lines.append(line.rstrip())
        wr(p,'\n'.join(lines))
