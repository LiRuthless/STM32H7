from pathlib import Path
import json,re
R=Path(__file__).resolve().parents[2];A=R/'App';D=R/'specs/features/17-app-architecture'
rows=json.loads((R/'MDK-GCC/build/phase17-g4-comment-inventory.json').read_text(encoding='utf-8'))
scope=['App/app_types.h','App/control/legacy_control.c','App/control/legacy_control.h',
'App/control/tracking.c','App/control/tracking.h','App/control/tuning.c','App/control/tuning.h',
'App/control/tuning_values.c','App/control/tuning_values.h','App/hmi/menu.c','App/hmi/menu.h',
'App/hmi/menu_keys.c','App/hmi/menu_keys.h','App/hmi/menu_render.c','App/hmi/menu_render.h',
'App/services/param.c','App/services/param.h','App/services/parameter_bank.c','App/services/parameter_bank.h',
'App/services/parameter_image.h','App/services/parameter_store.c','App/services/parameter_store.h',
'App/services/command_dispatch.c','App/services/command_dispatch.h','App/services/scb_protocol.c',
'App/services/wireless.c','App/services/datalog.c','App/system/lifecycle.c','App/system/lifecycle.h',
'App/system/scheduler.c','App/system/safety.c','App/system/parameter_application.c',
'App/system/parameter_application.h','App/system/tuning_application.c','App/system/tuning_application.h',
'MDK-ARM/SmartCar.uvprojx',
'tests/control_equivalence.py','tests/dual_mode_tracking.py','tests/runtime_tuning.py',
'tests/datalog_v2.py','tests/scb_protocol.py','tests/scb_link.py','tests/spi_log_export.py',
'tests/g2_system.py','tests/phase16_check.py','tests/g4_hmi.py','tests/g4_transactions.py','tests/g4_boundaries.py',
'specs/features/17-app-architecture/g4-review.md','specs/features/17-app-architecture/validation.md']
(R/'MDK-GCC/build/phase17-g4-paths.txt').write_text('\n'.join('SmartCar/'+p for p in scope)+'\n',encoding='utf-8')
text='''# G4 参数、调参、HMI与运行事件复核

日期：2026-10-24。产品基线6125495；规约先行03e0053/42d11b0，运行适配9f0cc6c，
模式协调d60a54c。本文记录G4实际事实，不覆盖G5/G6，不标记Phase17或旧阶段实机验收完成。

## 职责与人工内容复核

| 模块 | 状态、顺序、失败留态及复核结论 | 行为证据 |
|---|---|---|
| parameter_bank/image | 唯一四bank；非空镜像显式输入输出，不保留地址；52/56/88B及56/72偏移断言；Store仅四组及两个光电保留字写零 | 原param子项、control_equivalence、g4_hmi |
| parameter_store | 唯一镜像/脏/可用性/dirty_tick；CRC仍44/48/80B；v1/v2迁移、默认、电磁零值、四风扇钳位和重写失败保持；不读PID或菜单 | 原param迁移及失败断言、g4_hmi镜像字节对照 |
| parameter_application | SetDirection复制后先MarkDirty再应用；SelectGear不标脏；无脏/不可用不预读；flush才按PI→page→arrow采集；Save不更新静默起点 | param、runtime_tuning、g4_hmi的vp/vi及2999/3000ms对照 |
| tuning_values | 唯一五float有序/三integer边界校验；NaN/Inf拒绝，不钳位；codec逐字段转换和显式结果映射 | 608项协议原断言、runtime_tuning |
| tuning | baseline/pending/last_success/effective/final分离；运行版本跳0，停车版本允许0；Complete才发布APPLIED；final直到前台take；事件单槽即时清空 | runtime_tuning、g4_transactions |
| tuning_application | 锁外validator/锁内上下文和门禁；停车merge顺序保留；SET锁外末尾重新读取revision；保存失败只尝试一次 | runtime_tuning、g4_hmi、逐调用点审查 |
| command_dispatch | 静态992/993B JSON工作区和meta只各一份；原JSON/日志路由保留；两SCB业务枚举显式映射；handler经Init注册而非公开协议头 | 原runtime/link/export、g4_boundaries |
| menu_keys | 路1后路2各三次、uint32求和/3；严格开区间用未滤波值；第二路覆盖后才更新显示滤波；首次扫描才初始化 | g4_hmi所有阈值/覆盖/滤波序列 |
| menu/menu_render | 页面与编辑、按键沿和绘制哨兵分别唯一owner；BACK保存旧调参页再导航；RST当拍早退，下拍再次清屏；松键保存后绘制 | g4_hmi逐条LCD调用及Flash字节对照、原param断言 |
| lifecycle/safety/scheduler | InitLink→Tuning_Init→注册；原两个前台分支各独立Service→ServiceLink；起停事件仍在原运行位置，全部字段赋值及读取顺序对历史一致 | g4_boundaries、原App中断注入/停车/重启断言 |
| Lifecycle_TrySetMode | 非法mode先拒绝，停车与保存门禁后才写mode；同mode也完整复位；Control→Motor→PID→IMU→参数应用→UART保持 | param新增非法/同mode真实入口，原跨mode/BUSY/保存失败，g4_boundaries顺序 |
| tracking/legacy_control/wireless | 调参改业务值；旧包装不新增调用；Wireless vp/vi只改活动PI，镜像flush前仍旧；d/c和xp/xd回显/清缓冲规则保持 | control_equivalence、runtime_tuning、g4_hmi |

C定义前逐项对照真实输入、状态读写、失败输出、锁和调度；getter/纯转发按实际类别允许省略details。
H仅摘要并指向真实C定义。已替换bank/store/application/menu/keys/render及本批调用方的通用模板，
修正SetDirection阶段注释的错误标脏顺序、store不应用控制、Menu_HandleKey已公开等过期事实。
新C/H无BOM、CRLF、末尾单换行、无Tab/行尾空白；普通行不超过100，原完整协议字符串例外保留。
多行compound literal大括号独占行；排版前后词法token核对相同，不能用此代替功能测试。
菜单来源块保留Sirius快照/平台/日期与未单列作者/独立许可证事实。

## 实际验证

- LCD=0/1 GCC均0警告0错误，text/data/bss分别91656/308/231688、94220/312/231704。
- control_equivalence、scb_protocol（608）、scb_link、datalog_v2、spi_wifi_state、
  spi_log_export、runtime_tuning、g2_system、g3_control全部通过，原断言未删减或放宽。
- 独立FSM、参数/key、完整App（生产未标定及宿主有效标定）子项全部通过。
  新增非法mode及同mode真实入口断言；PE链接器未裁剪的非测试初始化/起跑引用使用进入即失败的替身，
  模式入口及其reset、参数和控制使用真实实现，没有复制业务实现或忽略未定义符号。
- g4_hmi通过6125495/当前真实C对照的全部LCD调用和保存镜像；
  g4_transactions通过final所有权、独立成功值、事件时点、保护/会话取消及两条revision回绕；
  g4_boundaries逐token比较全部历史起停事件语句、Link薄转发与模式协调顺序。
- app_layout_check通过82个C/H、39个App C在Keil各一次及五类GCC目录。
  Phase16仅同步本批真实源清单及已注册dispatch回调owner，资源算法未修改。
- 完整dual_mode_tracking仍在读取原C:/Users/Li/Desktop/LQ_Track.c处失败；
  R07原始样本缺失不伪报通过，不删除SHA256断言；其之前真实gray子项已执行。
- 协议夹具按既有E盘host-temp运行，避免C盘输出路径被旧HARNESS首字符C误识别为命令。
- 最后双LCD的finally回写ELF曾遇Windows OSError22；随后从任务开始干净的HEAD二进制blob
  恢复并逐字节验证受跟踪ELF（805932B），app_config也已恢复，二者Git状态干净。
  hex/bin属于忽略产物，不纳入提交或作为受跟踪恢复证据。

日志统一MDK-GCC/build/phase17-g4-*；构建/宿主产物、临时源码和原始日志不自动纳入Git。

## 静态资源与并发

相对G3双LCD总BSS均减少48B，data不变，text分别增加916B/832B。
锁定ARM编译器-fstack-usage对比G3/G4本批范围：72/118个函数帧，最大均232B（静态有界）。
Tracking_Observe为144→176B，Scheduler_ControlISR为24→48B，
原Tuning_Service为88B，分离后CommandDispatch_Service为120B，TuningApplication_Set为72B。
这是单函数编译帧统计，不能直接当作链路峰值或实测高水位；最终全App调用链预算由G6复核。
原JSON缓冲992/993B保持静态且各一份；pending载体从76B协议命令变为36B业务请求，
baseline/last_success各32B、final52B和事务状态40B仍独立；新增即时发布位和事件槽不构成队列。
未新增堆分配、Flash/上传大副本、ISR JSON或ISR参数Flash访问。

单事件槽的前台生产者只在真正停车后合入/保存；此时timers已停止且pending前台收尾完成，
前台返回前不会起跑。TIM6生产后在同ISR立即同步；TIM15/TIM6同级互不抢占，
前台不能打断此区间。Select版本在Observe后、记录发布前同步；Complete事件在完整序号增加后
及记录发布前同步。Stop/Reset/Switch/Prepare不产生事件，不把final邮箱并入事件槽。

G4只验system自身与新header不直接暴露SCB；datalog.h现存传递SCB类型留G5/G6闭包验收。
AC5、700/900us、SPI波形、吞吐、Flash/实车与R07继续待验。
R08暂存区源码/依赖摘要绑定及提交前检查由父任务执行，本文件不宣称该门禁已经通过。

## 新拆分模块逐函数内容清单

下表每项已核对C权威注释、H摘要定位（公开项）、参数方向/空值和实际返回；
复杂项另外核对阶段、状态与失败保留。字段/类型/layout断言按上表各唯一owner审查。
标签/EOF自动检查覆盖103个定义且无缺项；人工内容结论来自上述真实语句和历史对照，
不以脚本数量代替内容验收。

| 文件 | 定义 | 已核对的职责 | 验证依据 |
|---|---|---|---|
'''
for row in rows:
    path=R/row['path'];s=path.read_text(encoding='utf-8');n=row['symbol']
    m=re.search(r'/\*\*\n(?:(?!\*/).)*?\*/\n(?=(?:static )?[\w *]+\b'+n+r'\([^;]*?\)\n\{)',s,re.S)
    b=re.search(r'@brief ([^\n]+)',m.group())[1]
    group=path.stem
    evidence='runtime_tuning / g4_transactions'
    if group.startswith('parameter'):evidence='param / g4_hmi / control_equivalence'
    elif group.startswith('menu'):evidence='g4_hmi / param'
    elif group=='command_dispatch':evidence='runtime_tuning / export / boundaries'
    elif group=='tuning_values':evidence='scb_protocol608 / runtime_tuning'
    text+=f'| {row["path"]} | {n} | {b} | {evidence} |\n'
text+='''
本批额外修改的既有定义：Lifecycle_Init/TryStart/TrySetMode、Safety_RequestStop/FinalizeStop、
Scheduler_Loop/ControlISR、Tracking_Observe/Execute/SetMode、Control_UpdateLegacy、
Wireless_Adjust、SCB_ValidateValues分别按本文职责表和真实调用点复核；其余getter及未改语句
沿用已提交G2/G3契约与本轮完整回归，不借G4改休眠控制行为。
'''
(D/'g4-review.md').write_text(text,encoding='utf-8',newline='\n')
