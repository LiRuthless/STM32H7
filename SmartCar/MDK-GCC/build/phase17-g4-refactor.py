from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[2]
A=ROOT/'App'
snap=Path(__file__).parent/'phase17-g4-baseline';snap.mkdir(exist_ok=True)
for p in A.rglob('*'):
    if p.suffix in ('.c','.h') and not (snap/p.name).exists(): (snap/p.name).write_bytes(p.read_bytes())
def read(n): return (snap/n).read_text(encoding='utf-8')
def write(p,s): (A/p).write_bytes((s.strip()+'\n').replace('\n','\r\n').encode('utf-8'))
def filedoc(n,b,d): return f'/**\n * @file {n}\n * @brief {b}\n * @details {d}\n */\n'
def extract(s,n):
    m=re.search(r'(?m)^(?:static )?[\w *]+\b'+n+r'\([^;]*?\)\n\{',s)
    assert m,n
    start=s.rfind('/**',0,m.start()); i=m.end(); depth=1
    state='code'
    while depth:
        c=s[i]; n=s[i+1:i+2]
        if state=='code':
            if c=='/' and n=='*':state='block';i+=1
            elif c=='/' and n=='/':state='line';i+=1
            elif c in ('"', "'"):state=c
            elif c=='{':depth+=1
            elif c=='}':depth-=1
        elif state=='block':
            if c=='*' and n=='/':state='code';i+=1
        elif state=='line':
            if c=='\n':state='code'
        elif c=='\\':i+=1
        elif c==state:state='code'
        i+=1
    return s[start:i]+'\n'
def body(s): return s[s.index('\n{')+2:s.rfind('}')]
def inc(*names):return ''.join(f'#include "{n}"\n' for n in names)
def header(path,b,d,includes,content):
    g='SMARTCAR_APP_'+path.replace('/','_').replace('.','_').upper()
    write(path,filedoc(Path(path).name,b,d)+f'#ifndef {g}\n#define {g}\n'+includes+'\n'+content+f'\n#endif /* {g} */')
def decl(sig,brief,location,note=''):
    return '/**\n * @brief '+brief+'\n * @note '+(note+' ' if note else '')+'完整契约见 '+location+'。\n */\n'+sig+';\n'
def fn(sig,brief,details,params,code,note=''):
    c='/**\n * @brief '+brief+'\n'
    if details:c+=' * @details '+details+'\n'
    for p,t in params:c+=' * @param['+p+'] '+t+'\n'
    if note:c+=' * @note '+note+'\n'
    return c+' */\n'+sig+'\n{\n'+code.strip('\n')+'\n}\n'

# Four banks and image ownership.
p=read('param.c'); ph=read('param.h')
types=p[p.index('typedef struct'):p.index('/* 前台持有')]
for old,new in [('param_store_v1_t','param_image_v1_t'),('param_store_v2_t','param_image_v2_t'),('param_store_t','param_image_v3_t')]:types=types.replace(old,new)
types+='#define PARAM_IMAGE_DEFAULT_PAGE 0u\n#define PARAM_IMAGE_DEFAULT_ARROW 1u\n'
header('services/parameter_image.h','参数镜像版本布局与保存快照。','v1/v2只读解码，v3按原88字节存储；本头不定义可写镜像。','#include <stdint.h>\n#include <stddef.h>\n',types+'''\ntypedef struct
{
    float kp_v; /**< 本次保存采集的活动速度比例增益。 */
    float ki_v; /**< 本次保存采集的活动速度积分增益。 */
    uint8_t page; /**< 保存时的菜单页面原字节。 */
    uint8_t arrow; /**< 保存时的菜单光标原字节。 */
} param_save_snapshot_t;
''')
at=read('app_types.h').replace('#define SMARTCAR_APP_APP_TYPES_H','#define SMARTCAR_APP_APP_TYPES_H\n\n#include <stdint.h>')
direction=ph[ph.index('typedef struct'):ph.index('/* 速度档位')]
values='''typedef struct
{
    float kp; /**< 方向比例增益。 */
    float k2p; /**< 方向非线性增益。 */
    float kd; /**< 方向拍间差分增益。 */
    float kp_v; /**< 两轮共享速度比例增益。 */
    float ki_v; /**< 两轮共享速度积分增益。 */
    int32_t base_speed; /**< 基础速度，脉冲/2ms，校验后才窄化。 */
    int32_t fan_run; /**< 运行风扇万分比，0..10000。 */
    int32_t fan_idle; /**< 空闲风扇万分比，0..10000。 */
} tuning_values_t;
'''
write('app_types.h',at.replace('\n#endif','\n'+direction+values+'\n#endif'))
bank=filedoc('parameter_bank.c','四组方向参数的唯一存储。','前台加载或设置，控制拍只读；镜像由调用方传入，不保存地址。')+inc('services/parameter_bank.h')+'\n/* 前台写入、控制拍读取的电磁/光电低高档参数。 */\nstatic param_direction_t s_direction[2][2];\n\n'
for old,new,sig in [('Param_LoadBanks','ParamBank_LoadBanks','void ParamBank_LoadBanks(const param_image_v3_t *image)'),('Param_StoreBanks','ParamBank_StoreBanks','void ParamBank_StoreBanks(param_image_v3_t *image)'),('Param_GetDirection','Param_GetDirection',None)]:
    f=extract(p,old)
    if sig:f=re.sub(r'(?:static )?void '+old+r'\(void\)',sig,f).replace('s_param.','image->')
    bank+=f+'\n'
bank+=fn('uint8_t ParamBank_SetDirection(uint8_t mode, uint8_t gear, const param_direction_t *profile)','复制有效目标的一组方向参数。','非法模式、档位或空输入提前返回0；有效复制后返回1，不应用或标脏。',[('in','mode 0电磁、1光电。'),('in','gear 0低档、1高档。'),('in','profile 非空输入；空指针拒绝。')],'''    if (mode >= 2 || gear >= 2 || !profile)
    {
        return 0;
    }
    s_direction[mode][gear] = *profile;
    return 1;''','仅参数加载或停车前台写入，不访问硬件。').replace(' * @note 仅参数',' * @return 成功1；非法输入0且不写入。\n * @note 仅参数')
write('services/parameter_bank.c',bank)
bankdecl=''
for sig,n,b in [('void ParamBank_LoadBanks(const param_image_v3_t *image)','ParamBank_LoadBanks','由镜像装载四组方向参数'),('void ParamBank_StoreBanks(param_image_v3_t *image)','ParamBank_StoreBanks','把四组参数按字段顺序写入镜像'),('const param_direction_t *Param_GetDirection(uint8_t mode, uint8_t gear)','Param_GetDirection','读取指定模式档位的方向参数'),('uint8_t ParamBank_SetDirection(uint8_t mode, uint8_t gear, const param_direction_t *profile)','ParamBank_SetDirection','复制有效目标的一组方向参数')]:bankdecl+=decl(sig,b,'parameter_bank.c::'+n)
header('services/parameter_bank.h','四组方向参数访问。','只声明bank读写，不应用控制参数或访问Flash。',inc('app_types.h','services/parameter_image.h'),bankdecl)
store=filedoc('parameter_store.c','参数镜像兼容与Flash保存。','前台拥有镜像及脏标志；加载四bank，实际保存时消费瞬时快照，不访问控制或菜单。')+inc('bsp.h','app_config.h','services/parameter_store.h','services/parameter_bank.h')+'#include <string.h>\n#define PARAM_MAGIC 0xA55A3C3Cu\n#define PARAM_VERSION 3u\n\n/* 参数镜像与保存可用性仅由前台更新。 */\nstatic param_image_v3_t s_param;\nstatic uint8_t s_dirty;\nstatic uint8_t s_flash_usable;\nstatic uint32_t s_dirty_tick;\n\n'
for n in ['Param_CalculateCrc16','Param_CalculateCrcV1','Param_CalculateCrcV2','Param_CalculateCrcV3','Param_ClampFanDuty','Param_SetOpticalDefaults','Param_SetDefaults','Param_WriteMirror','Param_GetFan','Param_MarkDirty','Param_GetSpeedGains']:
    f=extract(p,n).replace('Param_StoreBanks();','ParamBank_StoreBanks(&s_param);').replace('MENU_PAGE_HOME','PARAM_IMAGE_DEFAULT_PAGE')
    for old,new in [('param_store_v1_t','param_image_v1_t'),('param_store_v2_t','param_image_v2_t'),('param_store_t','param_image_v3_t')]:f=f.replace(old,new)
    store+=f+'\n'
load=extract(p,'Param_Load');load=load[:load.index('    PID_SetGain')]+'''    return s_flash_usable;
}
''';load=load.replace('Param_Load(void)','ParamStore_Load(void)').replace('Param_LoadBanks();','ParamBank_LoadBanks(&s_param);')
for old,new in [('param_store_v1_t','param_image_v1_t'),('param_store_v2_t','param_image_v2_t'),('param_store_t','param_image_v3_t')]:load=load.replace(old,new)
store+=load+'\n'
fan=extract(p,'Param_SetFan');fan=fan[:fan.index('    if (gear == Tracking')]+ '}\n';fan=fan.replace('Param_SetFan(', 'ParamStore_SetFan(');store+=fan+'\n'
sp=extract(p,'Param_SetSpeedGains');sp=sp[:sp.index('    PID_SetGain')]+'}\n';store+=sp.replace('Param_SetSpeedGains(', 'ParamStore_SetSpeedGains(')+'\n'
flush=extract(p,'Param_TryFlushPending').replace('Param_TryFlushPending(void)','ParamStore_TryFlushPending(const param_save_snapshot_t *runtime)')
flush=flush.replace('PID_GetState()->kp_v','runtime->kp_v').replace('PID_GetState()->ki_v','runtime->ki_v').replace('g_menu_page','runtime->page').replace('g_menu_arrow','runtime->arrow');store+=flush+'\n'
extras=[('void ParamStore_GetPosition(uint8_t *page, uint8_t *arrow)','复制镜像的菜单位置原字节。','不校验页面；由菜单owner执行合法性恢复。',[('out','page 非空页面输出。'),('out','arrow 非空光标输出。')],'    *page = s_param.page;\n    *arrow = s_param.arrow;'),('uint8_t ParamStore_IsUsable(void)','读取参数Flash可用状态。','',[],'    return s_flash_usable;'),('uint8_t ParamStore_IsDirty(void)','读取参数待保存状态。','',[],'    return s_dirty;'),('void ParamStore_RequestSave(void)','标记立即保存请求。','只置脏，不更新时间；与延迟保存静默时间分离。',[],'    s_dirty = 1;'),('uint8_t ParamStore_IsSaveDue(void)','检查静默3000ms保存期限。','只有脏状态才读取毫秒时间；无符号相减允许计数回绕。',[],'    return s_dirty && (uint32_t)(BSP_Time_GetTick() - s_dirty_tick) >= 3000u;')]
for sig,b,d,ps,c in extras:
    f=fn(sig,b,d,ps,c)
    if sig.startswith('uint8_t'):f=f.replace(' */\n'+sig,' * @return 条件成立返回1，否则0。\n */\n'+sig)
    store+=f+'\n'
write('services/parameter_store.c',store)
storesigs=['uint8_t ParamStore_Load(void)','void ParamStore_GetPosition(uint8_t *page, uint8_t *arrow)','void ParamStore_SetFan(uint8_t gear, uint8_t idle, int16_t duty)','void ParamStore_SetSpeedGains(float kp, float ki)','uint8_t ParamStore_IsUsable(void)','uint8_t ParamStore_IsDirty(void)','void ParamStore_RequestSave(void)','uint8_t ParamStore_IsSaveDue(void)','uint8_t ParamStore_TryFlushPending(const param_save_snapshot_t *runtime)','int16_t Param_GetFan(uint8_t gear, uint8_t idle)','void Param_GetSpeedGains(float *kp, float *ki)','void Param_MarkDirty(void)']
header('services/parameter_store.h','参数镜像加载及保存输入。','保存只接收瞬时活动快照；具体应用由system协调。',inc('services/parameter_image.h'),''.join(decl(s,'访问参数镜像或保存状态','parameter_store.c::'+re.search(r'(\w+)\(',s)[1]) for s in storesigs))
app=filedoc('parameter_application.c','参数应用与保存协调。','停车前台显式连接bank、镜像、控制和菜单；无脏或不可用时不捕获活动快照。')+inc('system/parameter_application.h','services/parameter_bank.h','services/parameter_store.h','control/tracking.h','control/pid.h','control/motor.h','hmi/menu.h')+'\n'
for n in ['Param_ApplyCurrentDirection','Param_SetDirection','Param_SelectGear','Param_Save','Param_ServiceSave','Param_FlushPending']:
    f=extract(p,n)
    if n=='Param_SetDirection':f=re.sub(r'    if \(mode >= 2 \|\| gear >= 2 \|\| !p\).*?    s_direction\[mode\]\[gear\] = \*p;', '    if (!ParamBank_SetDirection(mode, gear, p))\n    {\n        return 0;\n    }',f,flags=re.S)
    if n=='Param_Save':f=f.replace('s_dirty = 1;','ParamStore_RequestSave();')
    if n=='Param_ServiceSave':f=f.replace('s_dirty && (uint32_t)(BSP_Time_GetTick() - s_dirty_tick) >= 3000u','ParamStore_IsSaveDue()')
    app+=f+'\n'
app+=fn('uint8_t Param_Load(void)','加载镜像并依次恢复活动PI、档位和菜单。','底层读错仍应用默认RAM；必要重写失败不回滚已装载参数。',[],'''    uint8_t usable = ParamStore_Load();
    float kp;
    float ki;
    uint8_t page;
    uint8_t arrow;
    Param_GetSpeedGains(&kp, &ki);
    PID_SetGain(PID_GAIN_KP_V, kp);
    PID_SetGain(PID_GAIN_KI_V, ki);
    Param_SelectGear(PARAM_GEAR_LOW);
    ParamStore_GetPosition(&page, &arrow);
    Menu_RestorePosition(page, arrow);
    return usable;''','仅上电前台，可阻塞。').replace(' * @note 仅上电',' * @return Flash可用1；读取或重写失败0。\n * @note 仅上电')+'\n'
fan=extract(p,'Param_SetFan');start=fan.index('    value = Param_ClampFanDuty(value);');end=fan.index('    if (gear == Tracking',start)
fan=fan[:start]+'''    ParamStore_SetFan(gear, idle, value);
    value = Param_GetFan(gear, idle);
'''+fan[end:];app+=fan+'\n'
sp=extract(p,'Param_SetSpeedGains').replace('    s_param.kp_v = kp;\n    s_param.ki_v = ki;','    ParamStore_SetSpeedGains(kp, ki);');app+=sp+'\n'
f=extract(p,'Param_TryFlushPending');f=f[:f.index('    s_param.kp_v')]+'''    param_save_snapshot_t runtime;
    runtime.kp_v = PID_GetState()->kp_v;
    runtime.ki_v = PID_GetState()->ki_v;
    runtime.page = Menu_GetPage();
    runtime.arrow = Menu_GetArrow();
    return ParamStore_TryFlushPending(&runtime);
}
''';f=f.replace('!s_flash_usable','!ParamStore_IsUsable()').replace('!s_dirty','!ParamStore_IsDirty()');app+=f
write('system/parameter_application.c',app)
ah=ph[ph.index('/* 速度档位'):];ah=ah[:ah.index('#endif')]
for n in ['Param_GetDirection','Param_GetFan','Param_GetSpeedGains','Param_MarkDirty']:
    ah=re.sub(r'/\*\*[^*]*(?:\*(?!/)[^*]*)*\*/\n[^;]*\b'+n+r'\([^;]+;\n','',ah)
ah=ah.replace('param.c::','parameter_application.c::')
header('system/parameter_application.h','参数使用者和保存流程协调。','只声明业务应用接口；bank与镜像各自拥有存储。',inc('app_types.h','services/parameter_bank.h','services/parameter_store.h'),ah)
# Replace the removed aggregate header with real owners at every production caller.
for f in A.rglob('*'):
    if f.suffix not in ('.c','.h'):continue
    s=f.read_text(encoding='utf-8')
    if '#include "services/param.h"' in s:write(f.relative_to(A),s.replace('#include "services/param.h"','#include "system/parameter_application.h"'))
(A/'services/param.c').unlink();(A/'services/param.h').unlink()

# Menu ownership split, preserve all original display calls.
m=read('menu.c');mh=read('menu.h')
menuincs=m[m.index('#include'):m.index('/* 小屏布局')].replace('services/param.h','system/parameter_application.h')
menuincs+=inc('hmi/menu_keys.h','hmi/menu_render.h')
keys=filedoc('menu_keys.c','两路ADC按键扫描与显示滤波。','停车前台依次读取两路各三次，按未滤波值判键后更新显示值；第二路可覆盖第一路。')+inc('bsp.h','hmi/menu_keys.h','sensing/filter.h')+'\n/* 仅停车扫描更新的两路显示值和首次初始化的滤波历史。 */\nstatic uint16_t s_key_adc1 = 0;\nstatic uint16_t s_key_adc2 = 0;\nstatic filter_lowpass_t s_filt_key1;\nstatic filter_lowpass_t s_filt_key2;\nstatic uint8_t s_filt_inited = 0;\n\n'
for n in ['Menu_ReadKeyAverage','Menu_ScanKey']:keys+=extract(m,n).replace('g_menu_key_adc','s_key_adc').replace('连续读取十次','连续读取三次')+'\n'
for i in [1,2]:keys+=fn(f'uint16_t MenuKeys_GetAdc{i}(void)',f'读取第{i}路按键显示滤波值。','',[],f'    return s_key_adc{i};').replace(' */\nuint16_t',' * @return 最近扫描得到的滤波ADC值。\n */\nuint16_t')+'\n'
write('hmi/menu_keys.c',keys)
keymac=mh[mh.index('/* 按键值'):mh.index('/* 页面ID')]
header('hmi/menu_keys.h','按键分类和只读ADC显示值。','停车前台扫描的实现和滤波状态位于menu_keys.c。','#include <stdint.h>\n',keymac+''.join(decl(s,b,'menu_keys.c::'+n) for s,n,b in [('uint8_t Menu_ScanKey(void)','Menu_ScanKey','扫描两路ADC按键'),('uint16_t MenuKeys_GetAdc1(void)','MenuKeys_GetAdc1','读取第一路滤波显示值'),('uint16_t MenuKeys_GetAdc2(void)','MenuKeys_GetAdc2','读取第二路滤波显示值')]))
logic=filedoc('menu.c','停车菜单导航与编辑。','持有页面、光标、编辑模式、待保存和按键沿；按键采集与绘制分别委托独立owner。')+menuincs+'\n/* 停车前台拥有导航及编辑状态，参数加载仅恢复页面和光标。 */\nstatic uint8_t s_page = MENU_PAGE_HOME;\nstatic uint8_t s_arrow = 1;\nstatic uint8_t s_mode = 1;\nstatic uint8_t s_param_dirty = 0;\nstatic uint8_t s_fan_edit_idle = 0;\n\n'
f=extract(m,'Menu_HandleKey').replace('g_menu_page','s_page').replace('g_menu_arrow','s_arrow');logic+=f+'\n'
f=extract(m,'Menu_Update');f=f.replace('        BSP_LCD_Clear(BSP_LCD_WHITE);\n        s_last_displayed_page = 0xFFFF;\n        s_last_displayed_arrow = 0xFF;','        MenuRender_Reset();')
f=re.sub(r'    if \(g_menu_page != s_last_displayed_page\)\n    \{.*?\n    \}', '    MenuRender_PreparePage();',f,flags=re.S).replace('    Menu_DrawContent();\n    Menu_DrawCursor();','    MenuRender_Draw();');logic+=f+'\n'
for n,s,b in [('Page','s_page','页面编号'),('Arrow','s_arrow','光标编号'),('FanEditIdle','s_fan_edit_idle','风扇编辑选择')]:logic+=fn(f'uint8_t Menu_Get{n}(void)','读取当前'+b+'。','',[],'    return '+s+';').replace(' */\nuint8_t',' * @return 当前'+b+'原值。\n */\nuint8_t')+'\n'
restore=read('param.c');rs=restore[restore.index('    if (s_param.page =='):restore.index('    return s_flash_usable;',restore.index('    if (s_param.page =='))].replace('s_param.page','page').replace('s_param.arrow','arrow').replace('g_menu_page','s_page').replace('g_menu_arrow','s_arrow')
logic+=fn('void Menu_RestorePosition(uint8_t page, uint8_t arrow)','按镜像合法性恢复页面与光标。','只接受六个既定页面及1..5光标；不改编辑模式、滤波、按键沿或绘制缓存。',[('in','page 镜像页面原字节。'),('in','arrow 镜像光标原字节。')],rs)
write('hmi/menu.c',logic)
render=filedoc('menu_render.c','停车菜单绘制与刷新缓存。','只读菜单和业务状态，保持显示调用顺序；仅拥有上次页面和光标，不编辑参数。')+menuincs+m[m.index('/* 小屏布局'):m.index('/* 停车前台持有')]+'\n/* 仅绘制前台维护；复位清屏后强制下一轮重画。 */\nstatic uint16_t s_last_displayed_page = 0xFFFF;\nstatic uint8_t s_last_displayed_arrow = 0xFF;\n\n'
for n in ['Menu_DrawContent','Menu_DrawCursor']:
    f=extract(m,n)
    for old,new in [('g_menu_page','Menu_GetPage()'),('g_menu_arrow','Menu_GetArrow()'),('g_menu_key_adc1','MenuKeys_GetAdc1()'),('g_menu_key_adc2','MenuKeys_GetAdc2()'),('s_fan_edit_idle','Menu_GetFanEditIdle()')]:f=f.replace(old,new)
    render+=f+'\n'
render+=fn('void MenuRender_Reset(void)','清屏并使下次页面绘制失效。','RST按键沿调用后立即返回；下一轮仍需要按页面哨兵再次清屏。',[],'''    BSP_LCD_Clear(BSP_LCD_WHITE);
    s_last_displayed_page = 0xFFFF;
    s_last_displayed_arrow = 0xFF;''')+'\n'
render+=fn('void MenuRender_PreparePage(void)','页面改变时清屏并复位光标缓存。','清屏先于菜单松键保存及内容绘制；不修改导航状态。',[],'''    if (Menu_GetPage() != s_last_displayed_page)
    {
        BSP_LCD_Clear(BSP_LCD_WHITE);
        s_last_displayed_page = Menu_GetPage();
        s_last_displayed_arrow = 0xFF;
    }''')+'\n'
render+=fn('void MenuRender_Draw(void)','依次绘制页面内容与光标。','内容每轮刷新，光标只在位置或页面变化时刷新。',[],'''    Menu_DrawContent();
    Menu_DrawCursor();''')
write('hmi/menu_render.c',render)
header('hmi/menu_render.h','菜单绘制入口。','绘制缓存位于menu_render.c，导航和编辑仍由menu.c持有。','#include <stdint.h>\n',''.join(decl('void MenuRender_'+n+'(void)',b,'menu_render.c::MenuRender_'+n) for n,b in [('Reset','清屏并强制下轮重画'),('PreparePage','按页面变化准备显示'),('Draw','按内容和光标顺序绘制')] ))
pages=mh[mh.index('/* 页面ID'):mh.index('/* 当前页面编号')]
header('hmi/menu.h','停车菜单导航与业务编辑。','只声明页面、光标只读接口及按键业务处理；不公开可写状态。','#include <stdint.h>\n'+inc('hmi/menu_keys.h'),pages+''.join(decl(s,b,'menu.c::'+n) for s,n,b in [('void Menu_Update(void)','Menu_Update','处理按键沿、保存与绘制'),('void Menu_HandleKey(uint8_t key)','Menu_HandleKey','处理一个既定键值'),('uint8_t Menu_GetPage(void)','Menu_GetPage','读取页面'),('uint8_t Menu_GetArrow(void)','Menu_GetArrow','读取光标'),('uint8_t Menu_GetFanEditIdle(void)','Menu_GetFanEditIdle','读取风扇编辑选择'),('void Menu_RestorePosition(uint8_t page, uint8_t arrow)','Menu_RestorePosition','按镜像合法性恢复页面和光标')]))
