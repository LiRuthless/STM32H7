# G5 记录、日志存储/导出及 SCB 纯编解码实施契约 — 阶段入口

本页保留Phase17当时的设计/验收契约，不再独立维护现行功能和代码映射。

## 当前docs入口

- [services/recording-and-export.md](../../../docs/services/recording-and-export.md)
- [services/communication.md](../../../docs/services/communication.md)

旧迁移清单/JSON/评审及真实结果仍按原基线解释，不用历史章节替代现行docs或硬件验证。

## 冻结历史基线（68cb508，非现行契约）

以下原文用于保留阶段决定、组顺序、需求历史与真实验证；不继续独立维护算法/布局/源码映射。当前契约先查上方docs条目。旧“本轮”“待执行”“当前”只指原记录时点，不覆盖后续决定。历史任务的全量软件门禁不自动成为以后每次修改的要求。

<details>
<summary>展开原始阶段记录（原文保留）</summary>

# G5 记录、日志存储/导出及 SCB 纯编解码实施契约

日期：2026-10-23。对应 FR-08/09/11/12/13、V-05～08；本增量独立提交后才允许 G5 实现。
承接 G2 生命周期/电池访问器、G3 控制接口及 G4 command_dispatch 边界。G4 已拥有原 tuning 的
命令路由、所有 JSON/缓存/meta 工作区、查询结果格式和 SCB 回调注册；本组不再次移动这些所有权。
本组无未决设计；所有行为以本组实现前源码为准，既有协议/日志版本、历史基线及验证断言不变。

## G5-01 文件和全部函数映射

新文件全部直接位于 App/services/，无子目录。datalog.c 拆后删除；datalog.h 保留为不依赖协议的
固定记录/头部/状态类型及既有 Datalog_* API 声明，不产生第二套实现。scb_protocol.c/h 拆后删除，
全部仓库调用方改含实际模块头，不以别名宏掩盖遗漏。原公共函数除下文明确迁移外保留签名。

| 原符号 | 目标 | 责任/接口决定 |
|---|---|---|
| Datalog_CalculateCrc16、Datalog_SetProfile | run_record.c 私有 | 头部 CRC16 与 bank 到头字段装配，原函数名保留 |
| Datalog_PublishControl | run_record.c | 原 void(void)，持有完整控制快照 |
| Datalog_IsHeaderValid | run_record.c → RunRecord_IsHeaderValid | 内部公开 `uint8_t(const datalog_header_t *)`，原完整短路规则 |
| Datalog_LatchFault | logger.c | 改内部跨模块接口 Logger_LatchFault(uint8_t fault)，语义不变 |
| Datalog_RequestFaultStop | logger.c 私有 | 锁首故障、累计保护、请求安全停车的原调用顺序 |
| Datalog_SetStopSummary/GetStatus/IsActive/GetCount/Init/Start/Clear/GetMeta | logger.c | 唯一会话及落盘统计 owner |
| Datalog_Push/Service/Flush/Stop | logger.c | Flash SPSC 生产/消费和有界收尾；记录内容交 run_record 装配 |
| Datalog_CancelExport/ServiceExport/IsExporting/GetExportStatus/BeginExport/ReadExport/EndExport/Dump | log_export.c | 前台导出状态、租期、重复块及 CSV；返回业务类型，raw byte 接口见下 |
| datalog.c 中所有 BSP_Flash_LogErase/Write/Read | log_store.c | 唯一 App 日志 Flash 后端；调用前后位置/次数保持 |
| SCB_ReadU16/ReadU32/WriteU16/WriteU32、SCB_BuildFrame、SCB_EncodeFrame/DecodeFrame | scb_frame.c/h | 原帧格式及私有字段规则；BuildFrame 仍 static |
| SCB_CalculateCrc | scb_frame.c/h + checksum.c/h | 公共名为薄包装；原 CRC32 真正公式移入通用 Checksum_UpdateCrc32 |
| SCB_ValidateUtf8/SkipSpace/ReadHex/AppendUtf8/ParseString/ParseNumber/ParseValue | scb_json.c 私有 | 原有界解析助手、无递归工作栈 |
| SCB_ParseJson/FindMember/GetString/GetNumber | scb_json.c/h | 原签名及借用文本生命期 |
| SCB_JsonBegin/JsonAppend/JsonU32/JsonI32/JsonFloat | scb_json.c/h | 通用有界 JSON writer，无业务路由 |
| SCB_StatusName/JsonValues/ParseCommand/ValidateValues | scb_command_codec.c/h | 业务格式适配；参数校验复用 G4 业务规则 |
| scb_link.c/h 全部原函数 | 原处 | 只改拆分头和 G4 回调引用，不重写会话/可靠重发/去重算法 |
| telemetry.c/h 全部原函数 | 原处 | 仍为通信上传适配，独立队列及上传字段编码不移动到日志核心 |

run_record.h 声明内部记录装配接口；logger.h 声明跨导出/记录需要的 owner 访问器；
log_store.h 只声明字节存储接口；log_export.h 声明导出内部边界（公共 Datalog_* 声明仍在
datalog.h）。这些头不包含 SCB 头。新模块前缀为 RunRecord/Logger/LogStore/LogExport/Checksum；
原已有 Datalog/SCB/Telemetry 前缀保留，不机械重命名既有公共 API。

## G5-02 单一状态所有者及固定资源

| 对象/原状态 | 唯一 owner | 写入上下文/不变量 |
|---|---|---|
| s_snapshot，96B | run_record.c | TIM6 或起跑前发布；TIM15 借用复制，不额外缓存一份完整拍 |
| s_header，256B | run_record.c | 前台装配一次；logger/export 借只读指针，不能保存第二个当前会话头 |
| s_buf[1024]，98304B；s_head/s_tail volatile uint16_t | logger.c | TIM15 单写 head；前台单写 tail；实际可用 1023 槽，不能改成 1024 满容量队列 |
| s_batch[8*96]，768B | logger.c | 前台单消费者临时批次，Flash 完成前不释放槽 |
| s_status（各原 volatile 字段） | logger.c | active/首 fault/首 reason/累计 flags/accepted/written/rejected/pending 原语义 |
| s_header_valid、s_write_failed | logger.c | 前台构造成功才置 valid；写失败禁止后续重试 |
| Datalog_Start 原 static s_check，256B | log_store.c | 只用于写头后回读比较；不是第二个当前会话状态 |
| s_export、s_export_progress_ms/last_offset/last_length、s_export_block[968] | log_export.c | 前台串行；窗口1、原固定缓存数量；原 first8 空间保留但核心不写 SCB 元数据 |
| Datalog_Dump 原 s_dump_header/record/line[640] | log_export.c | 均保持 static；CSV 回读临时缓冲，不替代当前会话头 |
| s_command_json | scb_command_codec.c | 原静态命令解析工作区，前台串行复用 |
| s_json_containers[8]、s_json_phases[8] | scb_json.c | 唯一共享解析工作栈；所有 ParseJson 仍不可重入，不为拆模块新增每次栈/副本 |
| SCB_StatusName 的 names、ParseCommand 的 keys | scb_command_codec.c | 原只读表，字段顺序不变 |
| scb_link 全部请求/可靠帧/事件/会话状态；telemetry 全部槽/索引/元数据 | 原 owner | 不复制到 codec、logger 或 dispatch；原所有 volatile 和屏障保持 |

禁止新 writable extern 状态、堆分配、整份日志 RAM 副本和第二个传输缓存。保留原 record/profile/
header/status 的类型宽度、布局、对齐与保留区，全部 sizeof/offsetof/capacity 编译期断言移到
run_record.c（含 BSP 容量核对），不删除或改为仅测试检查。Flash 头 256B、记录 96B、容量87336、
样本1ms/控制2ms、灰度16路、参数v3及日志v2均不变。

## G5-03 记录装配、入队和 Flash 服务

新增内部接口均非空、调用方容量足够，非法内部指针不新增容错路径：

- `void RunRecord_ResetSnapshot(void)`：只 memset s_snapshot；在原 Datalog_Init 清 status 之后调用。
- `uint8_t RunRecord_CheckCalibration(void)`：只返回原 Gray_CheckCalibration(0)，起跑的配置门禁后调用。
- `const datalog_header_t *RunRecord_BuildHeader(void)`：前台清 s_header 并按原 Datalog_Start
  字段读取/写入顺序装配，含两 bank、灰度黑白表、weights、共享速度增益、风扇、全部固定配置与
  CRC16，返回同一 static 对象；CRC 仍初值ffff、多项式1021、覆盖8..255共248B。
- `const datalog_header_t *RunRecord_GetHeader(void)`：仅返回同一只读静态对象，不扫描 Flash。
- `void RunRecord_CaptureSample(datalog_record_t *record)`：在 logger 全部门禁通过后，把
  s_snapshot 复制至调用方原栈上 rec，依次读取 tick、EncL/EncR、GyroRaw，再三轴除14.3并转
  int16_t；最后调用一次 Telemetry_PushRecord(record)。完整记录上传先于 Flash 槽发布，时点同原。
- `uint8_t Logger_GetStopReason(void)`、`uint8_t Logger_HasHeader(void)`：只读原字段，不像
  GetStatus 更新 pending；用于原 snapshot.stop_reason 和 CSV 有效位读取位置。
- `void Logger_LatchFault(uint8_t fault)`：原 PRIMASK 临界区内，fault尚零时才写首次故障；
  无论是否已锁首故障，只要本次传入 fault!=READ 就清 active，READ保留 active；不写首reason、
  不自动停车。export读错复用此同一接口，不另建故障副本。

Datalog_PublishControl 的每字段读取位置保持：先取得 Track/Gray/Tracking 指针，memset snapshot，
复制 adc8B、gray32B，再按原顺序写跟踪输出/距离/电池/电磁阶段/序号/偏差/保护/模式/灰度有效/
首 reason，结尾 DMB。首 reason 用 Logger_GetStopReason 在原字段赋值点取值。不因新增函数调用
提前采集传感器或把多次状态读取变成更早的整体快照；TIM15/TIM6 同级不抢占契约持续成立。

Datalog_Push 仍由 logger 执行：active/fault 拒绝→accepted 达容量故障→计算 next 并判断队满→
RunRecord_CaptureSample（含独立上传复制）→写原 s_buf[head]→DMB→发布 head→accepted++→
恰达容量请求 FULL 停车。queue 满先 rejected++，再锁故障并停车；故障后的调用不再 rejected++。
不得在门禁前调用 CaptureSample，否则会改变原上传索引和样本采集次数。

log_store 不拥有会话 active/统计/有效标志，仅提供：

- `uint8_t LogStore_Erase(void)`：原 BSP_Flash_LogErase，返回0成功/非零失败。
- `uint8_t LogStore_WriteHeader(const datalog_header_t *header)`：先写 offset0 的256B，再在原
  static s_check 回读256B，再 memcmp，原 `||` 短路保持；任一失败返回1，否则0，不修改 logger。
- `uint8_t LogStore_Write(uint32_t offset, const uint8_t *data, uint32_t length)`：一一调用
  BSP_Flash_LogWrite，不重复擦除/重试/喂狗；批次地址仍256+written*96、长度n*96。
- `uint8_t LogStore_Read(uint32_t offset, uint8_t *data, uint32_t length)`：一一调用
  BSP_Flash_LogRead，不缓冲/重试，允许底层失败前部分写目标的原副作用。

Datalog_Start 先取得原 tracking 指针，再服务 export 互斥；持有互斥返回1且完全不 Init。
随后 Init（head/tail→status→snapshot→valid→write_failed 原顺序），检查 enable/div，检查标定，
BuildHeader，Erase，WriteHeader。配置错误锁 CONFIG；标定失败锁 HEADER；Erase 失败锁
UNAVAILABLE；写/回读/比较任一失败锁 HEADER；成功才 valid=1、active=1并返回0。Init 不清
s_header 字节、不清 export 身份或 lease；Clear 仍先检测导出，后 active=0/valid=0/Erase，保留
故障、队列和上一 RAM 摘要。不得顺带扫描上电遗留日志或把 Clear 变成新会话。

Datalog_Service 固定 tail→检查 write_failed/valid→计算环形 pending→active 且不足8条返回→
n=min(pending,8)→DMB→逐条 memcpy 到原 batch→LogStore_Write→成功 written+=n→DMB→
tail=(tail+n)%1024。写失败先 write_failed=1，再锁 WRITE/保护/停车，不推进 tail/written，不重试，
不声称已发 Flash 写可回滚。ISR 可在复制/Flash 阻塞时继续入其他槽；不得提早释放本批。
Datalog_Flush 只服务一次；Stop 先 active=0，再最多128次且 tail!=head、未 write_failed 时服务，
每次随后喂狗。不改成无界 drain，不把 Flash 等待移入 ISR。

Datalog_SetStopSummary 保持原临界区内首 reason 与 flags 按位或；GetStatus 仍更新
pending=accepted-written 后返回原静态只读指针，不能宣称是原子快照。GetCount 仅 written。
GetMeta 原临界区内依次复制 valid/written/header；无效头清零，不访问 Flash、不计未成功批次。

## G5-04 前台导出、业务结果及 wire 适配

datalog.h 新增独立 `datalog_result_t`：DATALOG_RESULT_OK、INVALID_ARG、OUT_OF_RANGE、BUSY、
NOT_FOUND、IO_ERROR。它不与 SCB 数值绑定。Begin/Read/EndExport 返回此枚举；其他 Datalog API
返回不变。command_dispatch 使用显式 switch 一一映射相应 SCB_*，未知值映射 IO_ERROR；日志
核心不能包含 scb_*、调用 SCB_Write/Read/CalculateCrc 或保存 SCB status/frame/writer。

BeginExport 仍签名 `datalog_result_t Datalog_BeginExport(datalog_export_status_t *status)`。
按原短路顺序：!App_IsStopped()、s_status.active、IsExporting→BUSY；无有效头→NOT_FOUND；
成功 transfer_id++并跳过0，冻结 written 前缀，total=256+96*written，next_offset/crc32=0、
last_length=0、lease=GetTick、active=1，最后写调用方 status。失败保留输出对象和原身份。
通过 Datalog_IsActive/GetCount、Logger_HasHeader 读取 owner，保持原表达式和写入时点。

ReadExport 明确迁移为
`datalog_result_t Datalog_ReadExport(uint32_t id, uint32_t offset, uint32_t length,
const uint8_t **data, uint16_t *data_length)`。输出 raw 日志字节，不含8B协议头；data 指向唯一
s_export_block+8，成功后只在下一次新读取覆盖缓存前有效，调用方必须在返回后的同一前台调用内
完成复制，不能跨轮持有。只 data_length 先置0；失败不发布 data，成功写 data 和 length。
不新增块缓存，不改变原968B静态分配，前8B仅保留空间、核心不编码 wire 字段。

ReadExport 的顺序固定：服务租期及身份检查→NOT_FOUND；length=0、>960、offset>total、
length>total-offset→OUT_OF_RANGE；完全等同最近 offset/length 的块直接借出缓存→OK，
不访问 Flash、不改 CRC/next/lease；其他 offset!=next→INVALID_ARG；新块先喂狗→Read至
cache+8→读失败再喂狗、锁 READ（不覆盖已有 fault）、Cancel、IO_ERROR；成功喂狗→累计 CRC32→
next=offset+length→last_offset→last_length→lease=GetTick→发布 raw 指针/长度。
原元数据编码从成功喂狗之后移到协议适配，不得把 CRC 数据覆盖范围扩成含 wire 头。

checksum.c/h 新增唯一纯函数
`uint32_t Checksum_UpdateCrc32(uint32_t previous, const uint8_t *data, size_t length)`：
移动 SCB_CalculateCrc 原函数体，初次 previous=0；入/出 xor ffffffff，反射 edb88320，空长
可空指针且返回 previous；循环类型和位运算不变。SCB_CalculateCrc 保留原签名作为薄转发，
LogExport 直接调用通用函数；不复制公式，不让 logging 因 CRC 依赖帧模块。

scb_command_codec 新增
`uint16_t SCB_EncodeLogData(uint8_t *payload, uint32_t offset,
const uint8_t *data, uint16_t length)`：调用方保证 payload至少968B、data可读length、
length为1..960且目标不与源重叠；依次 WriteU32(offset)、WriteU16(length)、WriteU16(0)、
memcpy(payload+8,data,length)，返回8+length。不分配静态块，不做业务状态改变。
command_dispatch 原 LOG_READ 成功位置调用它，目标仍既有 s_json，再原位置
SCB_QueueResult(id,SCB_LOG_DATA,s_json,encoded_length,1)；失败分支仍 RESULT JSON。
此适配保证 wire 的8B头、原始日志字节和缓存去重结果逐字节不变。

EndExport 按原 IsExporting/身份→NOT_FOUND、未到总长度→BUSY，失败 total/crc 不写；成功输出
冻结 total/crc，再 Cancel。Cancel 仅 active=0、last_length=0，保留全部其他历史/故障；不停车。
ServiceExport 用无符号(now-progress)>=10000 超时取消；IsExporting/GetExportStatus 先服务租期，
重复读取/查询不续租。lease 回绕、旧身份、读失败和 G4 session reset 的取消调用保持。

CSV 的 Datalog_Dump 仍先 Stop，再 Flash 读取头，不因有效位为0跳过原读动作。读错锁 READ并
输出原 #dump_error=read；无有效会话或原完整 header 校验失败输出 no_valid_session，随后同样
输出状态摘要及 EOF。全部57列、黑白表行、Gray_Normalize 调用、数字格式/CRLF、读记录地址、
每条成功/失败的原喂狗位置、行容量判断和 goto done 顺序保持。RunRecord_IsHeaderValid 保留
magic/version/各尺寸周期/模式档位/16路/50/200/200/容量/div/CRC/Gray_ValidateCalibration 的原
短路次序。静态 dump缓冲不改栈；CSV 与二进制 export 不擅自新增互斥或隐式 Cancel。

## G5-05 SCB 类型及编解码分层

scb_frame.h 持有 SCB_HEADER_SIZE/PAYLOAD_MAX/SPI_SIZE、可靠标志/消息类型常量与 scb_frame_t。
scb_json.h 持有 TOKENS=128/DEPTH=8、scb_json_type_t/token/json/writer；引用帧头的992B上限，
不含 command_dispatch/tuning/logger。scb_command_codec.h 持有 scb_status_t、scb_values_t、
scb_command_t 和命令/状态/values 格式接口，引用 frame/json 及 G4 的业务参数类型。
scb_link.h 改引实际 frame/codec 头；scb_link.c 中 writer/parser 使用显式 json 头。
不把 SCB 类型搬到 app_types；业务 tuning 类型由 G4 独立拥有。

帧层 EncodeFrame 仍先完整验证参数，不合法返回0且不写 output；合法后清完整1024B，写
SCB1/version1/类型/flags/长度/条数/session/seq/ack/request、复制负载，CRC覆盖头0..27和
实际payload，不含padding或CRC字段。DecodeFrame 保留32..1024大小、magic/version、字段规则、
精确网络长或1024B事务、CRC全部通过才赋值 frame，失败保留调用方 frame；payload借用 input。
BuildFrame 保持全部类型可靠位、握手 seq0/身份、正常非零session/seq、业务 request_id与
异步request_id=0、IDLE空负载、telemetry1..10且24+96*n 的原检查顺序。不得放宽未知标志/消息。

JSON 所有实际算法体只移动，不重写：UTF-8 禁 NUL/过长编码/孤立代理/越界码点；解码转义后
同一对象重复键拒绝；只接受四种空白；最多128 token/8层/992输入、字符串池原边界。
ParseValue 只分配一个令牌，ParseJson 用原静态显式栈，无递归/strtod/堆。失败可留下部分解析
工作区，但返回0不能继续分发。ParseNumber 保留原63B文法、integer标志（小数/指数一律非整数）、
十进制累加/有界指数缩放、<-400归零及有限性；不得换 libc 或修正舍入结果。
FindMember 仅直接对象成员；GetString 非字符串返回空；GetNumber 按原令牌检查写出。
writer 保留原 capacity/failed 锁存，溢出不发部分JSON、不自动NUL；整数INT32_MIN无符号幅值、
float九位有效科学记数法/负零=0/非有限failed 原顺序和文本字节保持。

ParseCommand 原 s_command_json、keys表及全部 op/args/fields 类型规则移 codec，前台串行。
首先 status=INVALID_ARG、清 command；JSON语法失败返回0且不可ACK；语法有效但业务缺项/未知
操作返回1及原业务错误；只status=OK才可执行。mode/gear仅0/1，op长度<24且仅大写/下划线；
SET_PARAMS五float三integer全量键，窄化前原double范围检查；target/transfer_id非零uint32，
offset完整uint32、length1..960。原允许的未知额外键、检查顺序和部分 command 写入不改成新策略。

SCB_ValidateValues 保留原签名及 SCB结果，字段逐项转换成 G4 tuning_values_t 后调用
Tuning_ValidateValues，并显式映射结果；空指针仍 INVALID_ARG，其他非有限/越界 OUT_OF_RANGE。
该唯一业务 validator 由 `control/tuning_values.c/h` 持有，仅依赖业务类型、app_config 限值及
标准 C，不包含事务状态、硬件、日志、协议或调度依赖。codec 与 tuning 事务共同调用这一个实现；
独立 scb_protocol 宿主测试必须链接真实 tuning_values.c，不能为避免链接事务模块而复制限值
判断或用桩 validator。G4 同步该模块的类型/声明与所有权，G5 同步其工程/测试源依赖。
ParseCommand 在窄化前的double范围检查继续使用同源 app_config TUNING_*常量，不能只靠
转换float后的业务校验替代。JsonValues 保持kp/k2p/kd/kp_v/ki_v/base_speed/fan_run/fan_idle顺序；
StatusName固定10项拼写、未知值INVALID_ARG。不新增复制版业务限值判断。

## G5-06 链路/上传不变量、迁移和验收

scb_link 所有 FindRequest/AllocateRequest/SetHandlers/ResetSession/Init/QueueEvent/TakeEvent/
FormatEvent/QueueResult/GetRequestResult/AcceptRequest/HandleAck/ReceiveFrame/StartReliable/
PrepareFrame/Service/GetStatus 函数保持原 owner 及实质函数体。只改真实codec/frame/json依赖及
G4注册的dispatch调用，不拆第二套请求表。原32业务缓存、事件队列、可靠单窗口、ACK/重发/
会话重建、预留最终结果后才执行业务、异步事件先后、传输缓存生命期与SPI非阻塞路径全部保持。

Telemetry_Init/ResetSession/BeginRun/PushRecord/GetStatus/SetRevision/Service 不变：256个108B槽、
ISR独占head/index/producer_dropped、前台tail/consumer_dropped、run/revision发生时固化；
满队仍先 index++ 保留缺口，只丢上传，不改 Flash tail/计数或请求停车。仍每10ms最多10条、
run/revision/index边界可提前结束旧批，24B元数据+96*n payload，DMB后才释放tail，run_id回绕
原会话重建保持。记录核心只通过 RunRecord_CaptureSample 末尾的原上传入口把完整record交给
此适配，logger 不含 SCB 编码或网络背压策略。

GCC五类源扫描自动覆盖新增C；Keil删除原datalog/scb_protocol源并逐一列入run_record/logger/
log_store/log_export/checksum/scb_frame/scb_json/scb_command_codec，每源一次。同步 App所有
调用、tests统一路径解析/源清单、Phase07/14/16现行文档、休眠清单与R08依赖。禁止历史源快照
使用新的拆分路径；历史输出和原protocol/datalog断言不删改。

| 验证 | 需求 | 唯一通过标准 |
|---|---|---|
| G5-V01 | G5-01/02 | 全函数/对象有唯一owner；核心日志无SCB类型/函数/编码头；仅log_store调用日志Flash后端；所有固定缓冲仅一份且无新增堆/大栈 |
| G5-V02 | G5-02/03 | datalog_v2原布局/容量/写入时ISR注入/首因/1023队满/八条批次/失败不重试/尾包/CSV及故障路径断言完整通过；所有header/record字节对历史一致 |
| G5-V03 | G5-04 | spi_log_export原begin/read/end/重复/乱序/范围/超时回绕/断链取消/读错/最大前缀/写失败前缀断言通过；新增raw API适配仅在测试调用codec，完整wire字节与CRC原期望相同 |
| G5-V04 | G5-05 | scb_protocol原帧黄金字节/CRC/坏帧/UTF8/重复键/token/深度/数字/命令/边界及float文本断言完整通过；不改变容差或状态期待 |
| G5-V05 | G5-03/06 | scb_link、spi_wifi_state、runtime_tuning原会话/去重/缓存预留/结果/队列独立/调参整拍断言通过；datalog背压与上传丢弃互不改变对方状态 |
| G5-V06 | 全部 | LCD=0/1 GCC构建各0警告0错误；Keil源清单无缺漏重复；CS-01～15与R08逐函数/依赖证据齐全；RAM/栈对基线差额解释且无复制大缓冲 |

宿主当前侧原 `#include datalog.c/scb_protocol.c` 白盒必须按新模块编译/包含真实实现，静态断言与
故障注入继续命中真实owner。访问 s_status/s_buf 等的既有fixture可在宿主TU包含owner实现，
不为测试新增产品可写全局或生产setter；仍保留原容量/接近满容量注入覆盖。ReadExport测试由
raw接口取得借用bytes后立即调用同一codec形成wire再比较，不在夹具中重写wire公式。
所有核心结果期待迁移成 DATALOG_RESULT_*，实际wire状态仍比较原SCB值，禁止依赖数值强转。
G4 dispatch实际LOG_READ路径必须单独跑通，不能只测试codec而绕过业务结果缓存。

本文件是实施与验收标准，未声称任何新测试已通过。实际结果回填阶段validation；AC5、实体
Flash、SPI/吞吐及700/900μs实测继续单独待验。构建或原字节/并发断言失败即停止本组提交，
完整回退源/工程/调用方/夹具，不以改协议、放宽容量/错误规则或修改历史期望通过验收。

</details>
