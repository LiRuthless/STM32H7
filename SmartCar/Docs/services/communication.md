# SCB/1通信与上传

现行功能说明，源码核对基线 `8d5ab2faf68432ae03b2c1f91bc4593c7dc7cc3e`；2026-11-07补全。需求经[功能索引](../traceability.md)进入本页，再由实现表定位源码。
当前行为不等于全部已批准/实机验收，冲突见[已知问题](../known-issues.md)。

<a id="f23"></a>
## F23 SCB帧JSON与命令
### STM32协议分层

scb_frame负责32B头、合法性、显式小端及CRC；scb_json有界UTF-8/JSON解析；scb_command_codec做命令字段与结果编码；command_dispatch把协议转换为系统/调参/日志业务；scb_link持会话/可靠状态，telemetry适配独立上传。纯checksum不依赖SCB，参数/日志核心不持协议枚举。
STM32业务入口拒绝坏CRC/长度/类型/身份、重复键、非法UTF-8、非有限车辆参数及不完整八项，不截断执行。32B头加至多992B负载，SPI固定1024B尾零；每方向一个可靠帧，传输ACK只表示接管，不表示参数生效/保存。
公共SCB/1完整契约在本页后半部。[P16 protocol](../../specs/features/16-spi-wifi-link/protocol.md)保留完整阶段协议快照，不能用当前实现覆盖其字段、向量与决定；当前适配差异见下文。SUBSCRIBE属于ESP32，STM32不实现PC网络栈。

### 验证

协议固定向量及608计数用例是既有记录，不是本轮重跑，也不是分支覆盖率。可按需scb_protocol/scb_link；跨副本固定向量和实际STM32/桥接/PC仍须按真实交付核对。

**需求来源：** [P16 FR-05, FR-09](../../specs/features/16-spi-wifi-link/requirements.md)；[P17 FR-09](../../specs/features/17-app-architecture/requirements.md)。实现入口：[App/services/scb_frame.c](../../App/services/scb_frame.c)、[App/services/scb_json.c](../../App/services/scb_json.c)、[App/services/scb_command_codec.c](../../App/services/scb_command_codec.c)、[App/services/checksum.c](../../App/services/checksum.c)、[App/services/command_dispatch.c](../../App/services/command_dispatch.c)。

<a id="f24"></a>
## F24 会话可靠传输与回绕
### 会话和可靠状态

HELLO(session0/seq0)→非零HELLO_ACK→本端IDLE确认。两方向seq/ack独立；可靠原帧重发保留身份，精确ack才释放，IDLE可独立确认；重复可靠帧不重复业务，较大不可靠seq不吞掉尚未确认重发。
200ms重发、最多3次；800ms无有效帧重握手，会话重建取消未应用项和导出，不回滚已生效参数，不新增通信停车。缓存最近32请求及原内容/结果，相同ID同内容返回缓存，不同内容INVALID_ARG；事件队列32条，遗漏EVENT_GAP可见。

### 待决定的回绕责任

本端next_seq达到0xFFFFFF00主动重建；接收只检查非零和ack比较。公共“临近回绕重建”若指各发送端负责，则可成立；若要求接收对端临界序号也触发，则缺逻辑。D08未定，不自行修改或宣称全部双端场景已覆盖。
核对session变化、重发、去重、请求占位/结果、事件缺口和seq/revision回绕；既有scb_link/g4_transactions为按需软件证据。仓内已有[ESP32Bridge](../../../ESP32Bridge/README.md)，该工程记录生产消费者/最终交付及复测未完成；目录存在不等于本端回绕责任已联调验证。

**需求来源：** [P16 FR-06, FR-07, FR-08, FR-22](../../specs/features/16-spi-wifi-link/requirements.md)。实现入口：[App/services/scb_link.c](../../App/services/scb_link.c)。

<a id="f25"></a>
## F25 独立上传
### 独立上传

TIM15记录构造后复制到独立256槽SPSC上传队列，内容先写/屏障/head发布，前台消费；堵塞只增上传drop，不能停止Flash记录或新增停车。新run分非零run_id、索引重置；用session/run/index归属，不能按异步事件到达顺序归属延迟包。
同包1～10条连续同run且同revision记录，24B元数据+96*N；缺口/版本改变提前封包。Flash不存revision，电脑另存事件用于参数历史。完整拍生效、暂停/恢复按既有版本发布时点同步，不复制第二份日志大缓冲。
核对索引丢弃不压缩、包长、跨run隔离、revision边界和拥塞Flash独立；scb_link/datalog/runtime组合证据不等于60s实机吞吐。仓内已有桥接工程，网络仍由ESP32负责，当前三端实机吞吐未验。

**需求来源：** [P16 FR-10, FR-11, FR-12](../../specs/features/16-spi-wifi-link/requirements.md)。实现入口：[App/services/telemetry.c](../../App/services/telemetry.c)、[App/services/logger.c](../../App/services/logger.c)。

## 本页实现与模块反查

下表是当前源码定位；完整符号表仅在模块主功能页维护，协作行转向主条目。公共/私有辅助函数归所属功能，不把声明视为另一实现。Core生成部分只作边界，用户区按原标记维护。

| 文件 | 主功能 | 定义/配置定位 |
|---|---|---|
| [App/services/checksum.c](../../App/services/checksum.c) | F23 | `Checksum_UpdateCrc32` |
| [App/services/checksum.h](../../App/services/checksum.h) | F23 | 类型/布局/配置；关联同模块C定义 |
| [App/services/command_dispatch.c](../../App/services/command_dispatch.c) | F23 | `CommandDispatch_MapResult`、`CommandDispatch_ToWire`、`CommandDispatch_ToBusiness`、`CommandDispatch_Begin`、`CommandDispatch_Send`、`CommandDispatch_Position`、`CommandDispatch_ReadBank`、`CommandDispatch_QueryParams`、`CommandDispatch_QueryStatus`、`CommandDispatch_QueryResult`、`CommandDispatch_QueryLogMeta`、`CommandDispatch_MapLogResult`、`CommandDispatch_LogCommand`、`CommandDispatch_HandleCommand`、`CommandDispatch_Set`、`CommandDispatch_Switch`、`CommandDispatch_ResetSession`、`CommandDispatch_Init`、`CommandDispatch_InitLink`、`CommandDispatch_ServiceLink`、`CommandDispatch_Service`、`CommandDispatch_QueueTuningEvent`、`CommandDispatch_PublishRunStarted`、`CommandDispatch_PublishRunStopped` |
| [App/services/command_dispatch.h](../../App/services/command_dispatch.h) | F23 | 类型/布局/配置；关联同模块C定义 |
| [App/services/logger.c](../../App/services/logger.c) | F20 | 协作模块；完整符号表见[F20主条目](recording-and-export.md#f20) |
| [App/services/scb_command_codec.c](../../App/services/scb_command_codec.c) | F23 | `SCB_StatusName`、`SCB_JsonValues`、`SCB_ParseCommand`、`SCB_ValidateValues`、`SCB_EncodeLogData` |
| [App/services/scb_command_codec.h](../../App/services/scb_command_codec.h) | F23 | 类型/布局/配置；关联同模块C定义 |
| [App/services/scb_frame.c](../../App/services/scb_frame.c) | F23 | `SCB_ReadU16`、`SCB_ReadU32`、`SCB_WriteU16`、`SCB_WriteU32`、`SCB_CalculateCrc`、`SCB_BuildFrame`、`SCB_EncodeFrame`、`SCB_DecodeFrame` |
| [App/services/scb_frame.h](../../App/services/scb_frame.h) | F23 | 类型/布局/配置；关联同模块C定义 |
| [App/services/scb_json.c](../../App/services/scb_json.c) | F23 | `SCB_ValidateUtf8`、`SCB_SkipSpace`、`SCB_ReadHex`、`SCB_AppendUtf8`、`SCB_ParseString`、`SCB_ParseNumber`、`SCB_ParseValue`、`SCB_ParseJson`、`SCB_FindMember`、`SCB_GetString`、`SCB_GetNumber`、`SCB_JsonBegin`、`SCB_JsonAppend`、`SCB_JsonU32`、`SCB_JsonI32`、`SCB_JsonFloat` |
| [App/services/scb_json.h](../../App/services/scb_json.h) | F23 | 类型/布局/配置；关联同模块C定义 |
| [App/services/scb_link.c](../../App/services/scb_link.c) | F24 | `SCB_FindRequest`、`SCB_AllocateRequest`、`SCB_SetHandlers`、`SCB_ResetSession`、`SCB_Init`、`SCB_QueueEvent`、`SCB_TakeEvent`、`SCB_FormatEvent`、`SCB_QueueResult`、`SCB_GetRequestResult`、`SCB_AcceptRequest`、`SCB_HandleAck`、`SCB_ReceiveFrame`、`SCB_StartReliable`、`SCB_PrepareFrame`、`SCB_Service`、`SCB_GetStatus` |
| [App/services/scb_link.h](../../App/services/scb_link.h) | F24 | 类型/布局/配置；关联同模块C定义 |
| [App/services/telemetry.c](../../App/services/telemetry.c) | F25 | `Telemetry_Init`、`Telemetry_ResetSession`、`Telemetry_BeginRun`、`Telemetry_PushRecord`、`Telemetry_GetStatus`、`Telemetry_SetRevision`、`Telemetry_Service` |
| [App/services/telemetry.h](../../App/services/telemetry.h) | F25 | 类型/布局/配置；关联同模块C定义 |

<a id="scb1"></a>
# SCB/1 公共契约

以下承接P16公共协议原文与已确认STM32适配，外部消息/数值不变；网络/ESP32描述是跨端协议要求，实际桥接进度见[该工程README](../../../ESP32Bridge/README.md)，不宣称已完成三端实机闭环。原任务书是来源快照，后续协议变更须同步相关端与docs。

## 公共通信协议：SCB/1（v1，三端必须一致）

本节在三份任务书中完全相同。它是外部接口契约，不能由单个 agent 修改；确需改动时，先向用户说明原因，再同步三份文档及三端实现。只实现自己负责的一端，不为等待另一端而停止：用共同测试向量及模拟对端完成可独立执行的验证。

### 1. 物理连接与网络

| STM32H743 | ESP32-S3 | 信号 |
|---|---|---|
| PB12 | GPIO10 | CS，低有效 |
| PB13 | GPIO12 | SCK |
| PB15 | GPIO11 | MOSI |
| PB14 | GPIO13 | MISO |
| PA15 | GPIO14 | READY，低有效 |
| PD15 | EN | 保留；本版不自动复位 ESP32 |
| GND | GND | 共地 |

SPI 信号为 3.3V。开发板电源接法沿用项目已有连接。SPI2 主机为 STM32，从机为 ESP32；Mode 0、8bit、MSB first、全双工、软件 CS。联调先约 1.875MHz，通过后约 7.5MHz，按实际 STM32 SPI2 内核时钟核算并测量，不信任旧文档中的估算频率。

ESP32 开启 WPA2 热点，SSID 为 SmartCar-XXXX，XXXX 取 MAC 最后两字节的四位大写十六进制。密码由本地配置提供，8～63 个 ASCII 字符；缺失或无效时明确报错，不回退开放热点。ESP32 地址固定 192.168.4.1/24，启用 DHCP，只允许一个电脑控制会话。

- TCP 5000：命令、结果、事件、日志。第二个控制连接返回 BUSY 后关闭。
- Python 默认 UDP 5001：实时数据。电脑用 SUBSCRIBE 注册端口，目的 IP 必须取 TCP 对端，不接受任意第三方 IP。
- UDP 每个数据报为一帧，只承载 TELEMETRY，不超过 1016B。
- TCP 传输 32B 帧头加有效负载，不传 SPI 尾部填充；必须处理拆包、粘包和部分发送。
- SPI 每次 CS 有效期间严格交换 1024B，帧尾未使用区域清零。

### 2. 帧头与编码

| 偏移 | 类型 | 字段 | 规则 |
|---:|---|---|---|
| 0 | 4B | magic | ASCII SCB1 |
| 4 | u8 | version | 固定 1 |
| 5 | u8 | type | 见消息类型 |
| 6 | u16 | flags | bit0=RELIABLE，其余为 0 |
| 8 | u16 | payload_len | 0～992 |
| 10 | u16 | record_count | TELEMETRY 为 1～10，其他为 0 |
| 12 | u32 | session_id | 当前 STM32↔ESP32 链路会话 |
| 16 | u32 | seq | 本传输方向的消息序号 |
| 20 | u32 | ack_seq | 最后接受的可靠消息序号；初值 0 |
| 24 | u32 | request_id | 业务请求编号，异步消息为 0 |
| 28 | u32 | crc32 | 校验值 |

所有多字节整数小端，浮点为 IEEE754 binary32。不能直接发送或强转普通 C 结构体，须显式编解码并做尺寸/偏移断言。

CRC32 与 Python zlib.crc32 一致：先计算头字节 0～27，再继续计算 payload_len 字节负载，结果截断为 u32；不包含 CRC 字段及 SPI 填充。Python 帧头格式为 <4sBBHHHIIIII，必须断言长度为 32。

JSON 为 UTF-8，不带结尾 NUL，长度以 payload_len 为准。各端拒绝错误JSON、重复键、非法编码和超长负载，不允许截断执行。ESP32桥接仅检查数字语法，不转换或全局限制binary64范围；车辆参数有限性和范围继续由STM32业务入口判断，电脑调参入口也保留检查。C 端使用有界解析，STM32 不使用动态内存。

### 3. 消息、确认与会话

| type | 名称 | 负载 |
|---:|---|---|
| 0x01 | HELLO | JSON |
| 0x02 | HELLO_ACK | JSON |
| 0x03 | IDLE | 空 |
| 0x10 | COMMAND | JSON |
| 0x11 | RESULT | JSON |
| 0x12 | EVENT | JSON |
| 0x20 | TELEMETRY | 二进制 |
| 0x30 | LOG_DATA | 二进制 |

除 HELLO、HELLO_ACK、IDLE、TELEMETRY 外设 RELIABLE。

- SPI：STM32 以 session_id=0、seq=0 发 HELLO，负载为 {"role":"stm32","protocol":1}。ESP32 生成非零随机 session_id，用 HELLO_ACK 返回，JSON 至少含 protocol、session_id、link_ready。
- 握手期间重复 HELLO 返回相同会话；STM32 用携带新 session_id 的 IDLE 确认后建立会话，正常 seq 从 1 开始。
- 已建立链路再次收到 STM32 的 session_id=0 HELLO 时开启新握手；ESP32 重启后丢弃旧会话帧，等待 STM32 超时重握手。
- TCP：电脑连接后发 {"role":"pc","protocol":1} 的 HELLO，session_id=0、seq=0；ESP32 回 HELLO_ACK 告知当前会话与 link_ready，不重置 SPI。link_ready=false 时电脑只查询连接状态，不下发控制请求。
- 会话改变清除传输缓存、未应用请求和订阅，不回滚已生效控制值；电脑需重新查询并订阅。旧会话不能继续导出或执行命令。
- SPI 与 TCP 各自维护双向 seq/ack，转发时重新封装并重算 CRC，保留业务 request_id 和内容。UDP 序号独立，CRC 独立生成。
- 正常新帧 seq 单调递增，0 保留；重发保留原 seq 和原内容。接近回绕时重建该连接/链路，不跨回绕比较。
- 每方向最多一个可靠帧在途；收到 ack_seq 精确匹配才释放。IDLE 可以在等待期间独立发送确认，不等待对端业务完成。
- 去重以可靠帧身份为准，不能因后来 IDLE/TELEMETRY 的 seq 更大而丢弃尚未确认的可靠重发。重复可靠帧只确认，不重复分发。
- 200ms 未确认重发，最多三次；仍失败时 SPI 重握手，TCP 关闭后重连。通信失败不增加车辆停车条件。
- 帧级确认只表示合法帧已接收并接管，不表示参数已生效或 Flash 已保存。
- request_id 由电脑在会话内递增分配且非零；STM32 缓存最近 32 个请求及结果。相同编号/内容返回缓存结果，不重复执行；相同编号不同内容返回 INVALID_ARG。
- 电脑只允许一个未获最终结果的调参请求。断线后先 GET_REQUEST_RESULT，找不到标 UNKNOWN，不自动重放 SET_PARAMS。
- 非法 SPI 帧丢弃且不确认，依靠可靠超时恢复；非法 TCP 帧关闭连接；非法 UDP 帧计数后丢弃。不能从坏帧执行部分内容。

### 4. JSON 命令与结果

COMMAND 形式：{"op":"GET_STATUS","args":{}}。

RESULT 沿用 request_id，至少含 op/status；异步 EVENT 使用 request_id=0。status 为以下之一：
OK、RECEIVED、APPLIED、SAVED、INVALID_ARG、OUT_OF_RANGE、BUSY、SPECIAL_ELEMENT、NOT_FOUND、IO_ERROR。

| op | args | 结果/限制 |
|---|---|---|
| GET_STATUS | {} | 起停、mode/gear、元素/调试状态、revision、tick、control_sequence、日志/丢弃/保存状态 |
| GET_PARAMS | mode、gear | 完整 values、limits、当前有效值及覆盖状态 |
| SET_PARAMS | mode、gear、values | 整组校验，RECEIVED 后等待 APPLIED；停车配置成功返回 APPLIED，再异步报告保存 |
| TUNE_ENABLE | {} | 显式开启当前运行/当前目标的普通赛道覆盖，返回实际 tick/控制拍 |
| TUNE_DISABLE | {} | 关闭覆盖并恢复基线；保留最后成功调试值供停车保存 |
| GET_REQUEST_RESULT | target_request_id | 查询缓存结果，缺失返回 NOT_FOUND |
| SUBSCRIBE | udp_port | 由 ESP32 处理，限定 1～65535；返回 OK |
| GET_LOG_META | {} | 运行/停车可读缓存头，返回 log_valid、header_hex、written_records；不读写 Flash |
| LOG_BEGIN | {} | 停车且日志收尾完成才允许；返回 transfer_id、total_bytes、record_count、header_size=256、record_size=96 |
| LOG_READ | transfer_id、offset、length | 顺序读，length 为 1～960；返回 LOG_DATA |
| LOG_END | transfer_id | 返回 total_bytes、整体 crc32，结束导出并解锁 |

mode=0 电磁、1 光电；gear=0 低档、1 高档。SET_PARAMS 的 values 必须完整包含八项：

    {"kp":2.0,"k2p":0.008,"kd":15.0,"kp_v":20.0,"ki_v":0.75,
     "base_speed":180,"fan_run":1600,"fan_idle":1100}

五项增益为有限数，速度/风扇为整数。运行时只允许当前 mode/gear，fan_idle 须等于当前值，且覆盖已开启、处于普通循迹。模式/档位切换仍由已有停车入口负责，协议不新增运行切换命令。

limits 至少返回八项参数各自 min/max；初值为五项增益 0～100、速度 0～300 脉冲/2ms、风扇 0～10000，集中放 STM32 代码配置区，电脑不能另行硬编码。

APPLIED 结果必须含实际完整 values、revision、tick、control_sequence。RECEIVED 不是成功生效；SAVED/保存事件不是通信接收确认。

同一事件/结果重复抵达时，电脑按 session_id、request_id/revision、事件类型去重，但保留底层链路统计。

### 5. TELEMETRY 与 96B 记录

负载长度必须为 24 + record_count×96：

| 偏移 | 内容 |
|---:|---|
| 0 | u32 首条记录索引 |
| 4 | u32 参数版本 revision |
| 8 | u32 STM32 累计上传丢弃记录数 |
| 12 | u32 ESP32 累计上传丢弃包数 |
| 16 | u32 首条记录 tick |
| 20 | u32 run_id，当前运行编号 |
| 24 | 1～10 条 96B 日志记录 |

记录索引随每次应产生的采样递增，即使上传丢弃也不压缩索引；新运行重置索引。每次运行分配非零 run_id，在当前 session 内递增，并在每个 TELEMETRY 包、GET_STATUS 和运行事件中携带。用 session_id、run_id、记录索引联合去重；延迟抵达的上一运行包不能归入新运行，不能依赖异步事件先后判断包归属。

同包记录须连续、属于同一运行且 revision 相同；缺口/版本切换提前封包。ESP32 更新自身丢弃计数后重算 CRC。运行暂停覆盖/恢复也增加 revision，通知实际有效参数改变。

96B固定记录的唯一布局见[日志格式F19](recording-and-export.md#f19)，网络直接承载该布局。

以实际 SmartCar 头文件和偏移断言核对；若不一致停止联调并报告，不静默适配另一布局。Flash 记录保留以上格式，不写 revision；revision 仅在上传元数据/电脑事件中记录。

### 6. LOG_DATA、事件与错误边界

LOG_DATA 负载为 u32 offset、u16 length、u16 reserved=0、length 字节原始数据；request_id 对应 LOG_READ。

导出内容是原 256B 会话头加已成功写入的 96B 记录，total_bytes=256+96×record_count。一个时刻一个导出会话，窗口1。重复请求返回相同块，不重复推进导出整体 CRC。LOG_END 仅在全部字节按序交付后成功。超时10秒、控制连接断开或会话改变取消导出，释放起跑/清除互斥；SPI/TCP继续存活但无新导出读取时也执行超时。

不新增掉电后会话恢复，不扫描残留 Flash 猜测有效条数。没有当前可确认有效会话返回 NOT_FOUND。导出期间拒绝起跑和清日志。GET_LOG_META 只读内存缓存头，header_hex 是256B头的512位十六进制文本，未建立日志时 log_valid=false。

EVENT 至少含 event、run_id、tick、control_sequence、revision，按事件附 values/status：
RUN_STARTED、RUN_STOPPED、TUNE_APPLIED、TUNE_SUSPENDED、TUNE_RESUMED、PARAM_SAVED、PARAM_SAVE_FAILED、EVENT_GAP。事件队列有界，遗漏须显式报告；断线期间无法重建的历史标为未知，不能伪造完整历史。

Flash 日志仍约87.3秒，容量满执行原停车策略。实时上传可丢，Flash与现有安全链优先；运行参数历史由电脑 JSONL 另存，单独 Flash 日志不能完整还原运行调参。

### 7. 共同测试向量与集成

三端各自携带同一组固定十六进制向量：IDLE、GET_STATUS、完整参数设置、1条/10条记录、LOG_DATA、错误CRC/长度、重复可靠帧、重复request_id。

向量必须由独立解析及校验方法交叉核对；不能只做同一编解码器自我往返测试。集成顺序：协议向量→SPI握手→Wi-Fi实时上传/导出→运行调参。记录实际结果，未上机项目保持待验。
## STM32 适配说明（不属于上方公共原文）

### 断开与导出释放

ESP32任务书具体实现明确允许“发送内部取消导出通知，或确保其10秒超时释放”。公共消息表
没有定义内部取消通知的操作/载荷。STM32先实现已约定的10秒无新有效读取超时及会话变化
立即取消；ESP停止转发旧连接读取，使断开后最迟10秒释放。不能为通知断开而重置SPI会话，
不能自行增加LOG_CANCEL或EVENT名称。将来要求立即释放且跨端采用内部通知时，先同步三份
任务书和规约；无通知时不声称已经检测TCP断开。此说明来自02任务书的已列替代路径。

### 既有电脑字段与结果封装参考

Tools/SmartCarLink只读参考：GET_STATUS使用running/mode/gear/run_id/revision/tick/
control_sequence/log_valid；GET_PARAMS使用values/limits/effective/overridden，limits逐参数
为min/max；GET_REQUEST_RESULT命中使用status=OK、target_request_id、result（缓存原结果）。
这些字段形状见其scb_tune.py、scb_client.py及tests/mock_esp32.py；mock不是协议权威。
必须独立检查实际编码长度≤992B，不能截断完整结果，或用mock通过代替STM32实现通过。

### 固定向量

既有完整十六进制向量位于
[scb_vectors.py](../../../Tools/SmartCarLink/scb_vectors.py)，仅作为不可变输入读取。
实施时在SmartCar/tests夹具携带相同字节，并以独立逐位CRC和字段断言核对；不得调用同一
编解码器生成期望再用自身解码证明通过。SPI封装在原始网络帧后补零至1024B。

| 向量常量 | 期望 |
|---|---|
| IDLE_HEX | 32B，空负载，seq9、ack4，CRC 0x8940aca1 |
| GET_STATUS_HEX | 61B，payload29、seq1/request1，CRC 0x94efa91c |
| SET_PARAMS_HEX | 192B，payload160、完整八项，seq2/request2，CRC 0x98ef9c12 |
| TELEMETRY_1_HEX | 152B，payload120、1条、revision3/run7，CRC 0x5daa3261 |
| TELEMETRY_10_HEX | 1016B，payload984、10条、索引10～19，CRC 0xc58d37ba |
| LOG_DATA_HEX | 72B，payload40、offset256/length32/reserved0，CRC 0x8105364a |
| 错误CRC/长度与重复向量 | 保留全部既有固定输入；重复可靠帧不重复分发，重复request_id不重复执行 |

共同向量的原Phase16独立校核记录见[阶段验证](../../specs/features/16-spi-wifi-link/validation.md)，本轮只迁移文本、未重跑向量；后续任何差异先报告，不能改向量去迁就实现。
Python结构格式<4sBBHHHIIIII的长度必须独立断言32。记录布局以datalog.h和编译偏移断言
核对，Flash裸数据输出仍保持原v2字节，不为上传元数据更改reserved区。

<a id="parameters"></a>
## STM32 当前参数、调度与实际适配

| 参数 / 真实位置 | 当前值、类型、范围、单位 | 来源 / 覆盖 / 生效 / 持久化 |
|---|---|---|
| SCB_HEADER_SIZE / PAYLOAD_MAX / SPI_SIZE | 32 / 992 / 1024字节；编译常量 | scb_frame.h；无运行覆盖，编码每次清整1024B，尾零不进CRC；线格式version1，非Flash参数 |
| SCB_JSON_TOKENS / DEPTH | 128令牌 / 8层；字符串池993B | scb_json.h；前台静态工作区，解析共享栈不可重入，超界拒绝整帧；无运行覆盖或持久化 |
| SCB_REQUEST_CAPACITY | 32请求 | scb_link.h；前台独占缓存完整原文与结果；会话重建清空；有未完成结果不随意挤占 |
| s_events[32] | 32事件及单独GAP聚合状态 | scb_link.c；ISR/前台发布固定描述，队满保留遗漏计数；不落Flash，不能声称断线历史可恢复 |
| HELLO / reliable / no-RX / IDLE | 200 / 200 / 800 / 5ms；重发最多3次 | PrepareFrame与Service使用BSP毫秒tick；无符号时间差；无READY也检测失联；不是控制拍或车辆停车超时 |
| 本端seq重建边界 | next_seq≥0xFFFFFF00 | 准备帧时重握手；接收只校验非零和会话，D08跨端责任仍未定 |
| TELEMETRY_CAPACITY | 256全可用槽，每槽108B=96B记录+index/run/revision各4B，合27648B | telemetry.h/.c，单生产者TIM15、前台单消费者；全满计上传drop；无Flash持久化，无运行可调容量 |
| 上传封包节奏 | 常规10ms；每包1～10条，元数据24B，最大984B负载/1016B网络帧 | telemetry.c；run/revision/index边界可提前封包；READY及结果优先调度会延后，不保证每10ms物理发出 |
| LOG_READ / 导出租期 | 1～960B / 10000ms无新有效进展 | 日志导出主表见[日志参数](recording-and-export.md#parameters)；通信只封装LOG_DATA，不另持业务状态 |
| 八项调参limits | 五增益0～100，速度0～300脉冲/2ms，风扇0～10000万分比 | app_config.h，GET_PARAMS与validator同源；事务生效见[临时调参](runtime-tuning.md#parameters)；不是给旧镜像追加认证 |

### 前台到物理交换、业务与采样的调用链

前台按生命周期顺序先服务Flash日志，再CommandDispatch_ServiceLink→SCB_Service。后者先BSP_WiFi_Service→取得完成RX→SCB_ReceiveFrame（内部DecodeFrame→SCB_ParseCommand业务语法解析→SCB_AcceptRequest接管→dispatch）→参数/日志业务，再ReleaseRx。未完成DMA绝不读接收缓冲。物理READY许可、IRQ、超时/复位及DMA地址主说明归[SPI/Wi-Fi平台](../platform/spi-wifi.md#f29-wifi)。

随后只有TX空闲且许可未消费才PrepareFrame；优先级为握手确认/握手、到期可靠重发、拒绝结果、请求结果、事件、TELEMETRY、5ms IDLE。等待可靠ACK仍允许TELEMETRY/IDLE；每方向可靠帧在途仅一个。生成帧后HAL_BUSY保留prepared字节和seq，不重复生成；可靠200ms起点在实际交换启动时更新。传输ACK精确命中可靠seq才释放，与RECEIVED/APPLIED/SAVED不同。

上传路径为TIM6发布控制快照→TIM15构造采样记录→RunRecord_CaptureSample内Telemetry_PushRecord→上传槽DMB后发布head。s_next_index先递增再检查队满，缺口真实保留；前台固定head并屏障，逐槽检查run/revision/index相邻，整包复制完成才释放tail。会话重建只丢本次head快照前的积压并计消费drop，不写ISR的head/next_index。run_id在实际起跑前递增，达到末值先重建会话；revision在完整拍同步发布，已入队槽不重标。

### 编解码失败和跨端责任

Decode先构造局部描述，检查SCB1/version、type/flags/record_count、身份和精确网络长度或1024B事务，再算头0～27与实际payload CRC，全部通过才发布frame，失败保留调用方frame。JSON语法失败不ACK、不分发；语法合法但op/args非法返回INVALID_ARG；参数越界OUT_OF_RANGE；结果缓冲在接管前预留，不能先执行再发现无法回结果。编码拒绝输入时不写output，成功才清1024B并显式小端写字段；不存在普通结构体强转发送承诺。

本端不提供TCP、UDP、热点、SUBSCRIBE；这些由ESP32实现。会话变化立即取消未应用调参和导出，TCP断开没有额外未定义通知，最迟靠10秒导出无进展超时释放。当前阶段快照与实际STM32的一致部分是32B头/CRC/可靠身份/八项限值/256+96日志；有边界差异的项目是本端仅发送序号临界重建（D08）及无即时TCP断开通知。保持问题登记，不修改公共协议、向量或对端行为。

本次核对 scb_frame/scb_json/scb_command_codec/scb_link/command_dispatch/checksum/telemetry 与相关头文件，并对照Phase16完整快照；只做语义审查。JSON/请求/可靠状态均前台串行，只有事件发布和上传生产涉及原ISR并发。历史软件计数和向量记录保持，AC5、SPI波形、READY时序、持续吞吐和三端实机联调仍待验。

### 2026-11-07 桥接与消费职责及单请求契约

用户授权的[第一、二组设计](../../../ESP32Bridge/specs/features/05-bridge-lifecycle/single-request-design.md)为当前增量。SCB/1头、版本、CRC与所有字段布局保持。STM32生产端继续按规范构造遥测前缀、首tick及reserved=0；ESP32仅检查记录数/长度，不判断前缀值或逐条扫描保留区。LOG_DATA生产端reserved仍为0，桥接忽略保留字段但保持长度/offset区间；电脑作为消费端可以继续严格拒绝非零reserved。桥接接受不等于消费端内容有效。

普通JSON合法大指数可以经过桥接；NaN/Infinity字面量仍拒绝。STM32车辆业务参数有限性、范围、许可、APPLIED/SAVED与Flash布局均不变。本轮不改STM32源码或配置。

ESP32全部转发命令共享一个待完成请求，中间RECEIVED不释放，最终响应经电脑精确ACK才释放；不再承诺等待APPLIED时查询仍可前进。已交SPI但断线结果不明形成全局墓碑，迟答不能解除，所有新转发命令（包括GET_REQUEST_RESULT）可能背压至SPI会话改变，不自动复位/重放。电脑客户端统一单请求限制，合法可靠结果先发独立IDLE ACK再唤醒请求者；UDP遥测独立。第三组导出与事件职责迁移未执行。三端实机仍待验。
