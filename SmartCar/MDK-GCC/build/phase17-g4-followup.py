exec(open(__file__.replace('followup.py','refactor.py'),encoding='utf-8').read().split('# Four banks')[0])
s=(A/'control/tracking.c').read_text(encoding='utf-8');h=(A/'control/tracking.h').read_text(encoding='utf-8');at=(A/'app_types.h').read_text(encoding='utf-8')
for typ in ['tracking_mode_t','tracking_result_t']:
    definition=re.search(r'typedef enum\n\{[^}]+\} '+typ+';',h).group();h=h.replace(definition+'\n','');at=at.replace('\n#endif','\n'+definition+'\n\n#endif')
    at=at.replace('运行态已更新，但参数持久化写入失败。','模式尚未写入，参数持久化门禁失败。')
write('app_types.h',at)
mode=extract(s,'Tracking_TrySetMode');s=s.replace(mode.strip(),'')
mode=mode.replace('Tracking_TrySetMode','Lifecycle_TrySetMode').replace('s_tracking.mode = mode;','Tracking_SetMode(mode);')
mode=mode.replace(' * @details 按现有调用顺序使用\n *     App_IsStopped、Param_TryFlushPending、BSP_UART_WriteString、Control_ResetRunState\n *     完成本函数处理；条件分支和状态写入均保持函数体所示顺序。',' * @details 先验证模式和停车条件，再刷写待保存参数；成功才写模式、依次复位控制、\n *     电机、PID和IMU历史，最后应用该模式方向参数并回显。相同模式也执行完整流程。')
l=(A/'system/lifecycle.c').read_text(encoding='utf-8');l+='\n'+mode;write('system/lifecycle.c',l)
s=s.replace('#include "system/parameter_application.h"','#include "services/parameter_bank.h"')
s+=fn('void Tracking_SetMode(uint8_t mode)','写入已校验的模式编号。','只更新模式，不校验、保存、应用参数或清历史。',[('in','mode 生命周期已校验的0或1。')],'    s_tracking.mode = mode;','仅Lifecycle_TrySetMode调用。')
s=s.replace(' * @details 按函数体既定条件和写入顺序更新模块状态；提前返回分支不执行其后的状态发布。',' * @details 只递增uint32_t完整控制拍序号一次，按原整数语义自然回绕；不更新其他状态。')
s=s.replace(' * @details 按现有调用顺序使用 BSP_Time_EnterCritical、BSP_Time_ExitCritical 完成本函数处理。',' * @details 在保存PRIMASK的原短临界区内按位或累计保护，退出恢复原中断屏蔽；不清既有位。')
s=s.replace(' * @details 按现有调用顺序使用 Gray_GetState、OpticalFSM_Update 完成本函数处理。',' * @details 用当前灰度帧及累计保护构造停止输入推进光电状态，再依次清方向监视值和拍输出。')
s=s.replace(' * @details 按现有调用顺序使用 Track_GetState、Gray_GetState、Param_GetDirection、IMU_GetState 完成本函数处理。',' * @details 同帧构造两模式输入，先选择各自状态和本拍调参副本，再各计算一次方向；\n *     最后发布电感观察和选中偏差，两个FSM独立持有方向历史。')
write('control/tracking.c',s)
h=h.replace('完整契约见 tracking.c::Tracking_TrySetMode。','完整契约见 tracking.c::Tracking_SetMode。').replace('tracking_result_t Tracking_TrySetMode(uint8_t mode);','void Tracking_SetMode(uint8_t mode);').replace(' * @brief 在停车和待写参数保存成功后切换循迹模式。',' * @brief 只写生命周期已校验的模式编号。').replace(' * @note 前台调用；失败路径打印及副作用、成功后的各模块复位顺序保持。',' * @note 仅生命周期协调器调用，不执行保存、历史复位或参数应用。')
h=h.replace('#include "control/control_profile.h"','#include "control/control_profile.h"\n#include "app_types.h"');write('control/tracking.h',h)
lh=(A/'system/lifecycle.h').read_text(encoding='utf-8').replace('#include <stdint.h>','#include <stdint.h>\n#include "app_types.h"');lh=lh.replace('\n#endif','\n'+decl('tracking_result_t Lifecycle_TrySetMode(uint8_t mode)','停车模式切换及保存门禁','lifecycle.c::Lifecycle_TrySetMode')+'\n#endif');write('system/lifecycle.h',lh)
for f in A.rglob('*'):
    if f.suffix not in ('.c','.h'):continue
    st=f.read_text(encoding='utf-8')
    if 'Tracking_TrySetMode(' in st:
        st=st.replace('Tracking_TrySetMode(','Lifecycle_TrySetMode(')
        if '#include "system/lifecycle.h"' not in st:st=st.replace('#include "app.h"','#include "app.h"\n#include "system/lifecycle.h"')
        write(f.relative_to(A),st)
# The static command handlers require dispatch prototypes before the main router.
p=A/'services/command_dispatch.c';s=p.read_text(encoding='utf-8')
# A JSON literal needs the C escape only once.
s=s.replace(r',\\\"values\\\":',r',\"values\":')
write(p.relative_to(A),s)
# Exact source ownership in Keil, preserving its XML formatting.
k=ROOT/'MDK-ARM/SmartCar.uvprojx';xml=k.read_text(encoding='utf-8')
xml=re.sub(r'\s*<File>\s*<FileName>param.c</FileName>.*?</File>','',xml,flags=re.S)
for cat in ['system','services','control','hmi']:
    g=re.search(r'(<GroupName>'+cat+r'</GroupName>\s*<Files>)(.*?)(</Files>)',xml,re.S)
    assert g,cat
    entries=g[2]
    for f in sorted((A/cat).glob('*.c')):
        if '<FileName>'+f.name+'</FileName>' not in entries:
            entries+='\n          <File>\n            <FileName>'+f.name+'</FileName>\n            <FileType>1</FileType>\n            <FilePath>..\\App\\'+cat+'\\'+f.name+'</FilePath>\n          </File>\n'
    xml=xml[:g.start(2)]+entries+xml[g.end(2):]
k.write_bytes(xml.replace('\n','\r\n').encode('utf-8'))
