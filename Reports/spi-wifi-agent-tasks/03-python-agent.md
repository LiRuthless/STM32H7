# 电脑 Python agent 执行任务书
> 文件：03-python-agent.md；版本：2026-10-16；交付可运行工具、自动测试及中文README。

## 任务与授权边界

你负责电脑接收、记录、运行调参和停车日志导出。不开发完整GUI，不修改嵌入式控制策略。

仓库：E:\study\Intelligent Vehicle\STM32H7。检查Git状态、根目录及目标目录AGENTS。新建Tools/SmartCarLink，只修改该目录及其必要忽略规则，不修改SmartCar、ESP32Bridge、Hardware、Material或其他在途工作。不push。

使用Python 3.10+标准库socket、struct、csv、json、zlib、threading/queue、argparse、unittest，不增加第三方运行依赖。

用户已确认本设计；接口冲突先出示证据，不用猜测兼容另一协议。不要把实时丢包、UNKNOWN请求或未校验导出显示为完整/成功。无需等待对端，提供模拟服务端完成测试。

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

JSON 为 UTF-8，不带结尾 NUL，长度以 payload_len 为准。拒绝错误 JSON、重复键、非有限数、非法编码和超长负载，不允许截断执行。C 端使用有界解析，STM32 不使用动态内存。

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

现有 datalog v2 的 96B 固定记录：

| 偏移 | 类型 | 字段 |
|---:|---|---|
| 0 | u32 | tick，1ms |
| 4 | i16×4 | adc[4] |
| 12 | i16×2 | speed_l、speed_r，脉冲/1ms |
| 16 | i16×2 | track_error、track_out |
| 20 | i16×3 | gyro_x/y/z，度/秒整数 |
| 26 | u16×2 | dl1b_mm、battery |
| 30 | u8×2 | kernel_state、roundabout_state |
| 32 | u16×16 | gray_raw[16] |
| 64 | u32×2 | gray_sequence、control_sequence |
| 72 | i16×2 | em_error、optical_error |
| 76 | u16 | protection_flags |
| 78 | u8×4 | mode、optical_state、gray_valid、stop_reason |
| 82 | u8×14 | reserved，保持0 |

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

## Python 具体实现

### A. 模块和入口

提供smartcar_link.py及协议、会话、记录、导出、CLI模块。协议层不依赖socket，便于单测；socket增量解析、UDP接收、磁盘写入解耦。

命令示例：

    python smartcar_link.py monitor --output run01
    python smartcar_link.py tune enable
    python smartcar_link.py tune set --kp 2 --k2p 0.008 --kd 15 --speed 180
    python smartcar_link.py tune disable
    python smartcar_link.py params get
    python smartcar_link.py dump --output run01-full

公共选项host、TCP端口、UDP端口、请求超时；默认192.168.4.1:5000、UDP5001。params get可指定mode/gear，默认读GET_STATUS当前目标。调参只支持已确认的项，不添加远程起跑、停车或运行模式切换。

monitor持有唯一设备TCP，在127.0.0.1:15000提供本地逐行JSON命令接口，其他子命令转交它；请求含op/args，返回RESULT样式及请求状态。只绑定loopback。启动时本地端口占用明确报错，不抢占其他进程；没有monitor时单次CLI自行连接设备，不偷偷启动第二个设备会话。

TCP/可靠层和业务等待分离。RECEIVED保留待生效；得到APPLIED/错误才结束调参。网络读线程不得被CSV/终端输出阻塞。终端每秒刷新一次，不逐条打印1000条记录。

### B. 实时记录和文件

UDP接收队列1024包，溢出计数并写本地事件。验证CRC、长度、版本、session、record_count和payload关系；坏包不解析半条记录。

输出文件：原始报文文件、57列CSV、metadata.json、events.jsonl、link_stats.json。原始文件每条采用本机接收时间ns的u64、小端u32报文长度、原始报文字节；本机时间不代替设备采样tick。

原始报文按到达顺序保存，CSV写首次有效记录，重复不再次写；乱序明确标记到统计/事件，不插值补假样。以session/run边界、记录索引和tick区分丢失、重复、乱序、自然回绕。新run新建文件段，不能仅按索引去重导致新运行数据被删。读到RUN_STARTED或状态边界后与上传首索引核对；不确定边界保留原始数据并标记，不能拼成完整记录。

CSV列严格对应SmartCar当前Datalog_Dump的57列，单位/哨兵一致。只读SmartCar头文件/导出代码获取布局，不修改它们。GET_LOG_META获取缓存头，检查256B长度、日志magic/version/头CRC，再按黑白标定执行与Gray_Normalize一致的计算、截断、钳位和无效值处理。没有有效头先保原始数据，q字段留空并记缺元数据，不猜标定。

数据批量写盘，至少每秒flush；参数最终结果立即flush。磁盘失败停止该记录会话并明确报告，不默默丢文件继续宣称保存成功。Ctrl+C有序退出，记录最后计数，不删除未完成文件。

### C. 调参及事件

tune enable先查起停/元素状态，实际许可仍由STM32决定。tune set先GET_PARAMS完整8项及limits，只替换CLI指定项，再完整提交。验证有限数、整数类型和设备范围；不将用户输入静默钳位。base_speed单位脉冲/2ms、风扇万分比，清楚展示。

运行时target只能当前mode/gear、fan_idle不变；特殊元素设置返回SPECIAL_ELEMENT。覆盖暂停/恢复以STM32事件为准，不在电脑重现状态机。

每次只一条待最终结果的调参。事件保存请求编号、设备session/run、tick/控制拍/revision、实际完整值、本机时间和确认状态。APPLIED才称已生效，PARAM_SAVED/SAVED才称已持久化。结果与TUNE_APPLIED重复不重复计入参数变更，但保留链路统计。

断线后继续保存已收到数据并标记间断，重连查询未确认请求/状态，绝不自动重放SET_PARAMS。NOT_FOUND为UNKNOWN，不擅选重试成功。EVENT_GAP和暂停期间缺失历史在JSONL明确保留。

Flash没有运行参数历史，复盘需同时保留事件文件。不能宣称断网期间全部参数变化已重建。

### D. 完整导出及离线转换

只在停车且日志收尾完成后LOG_BEGIN，记录元信息。窗口1顺序读，length最大960，末块按剩余字节读取。验证request_id、offset、length、边界和CRC；可靠重发不得重复写或重复累计整体CRC。

流式写.partial并计算CRC32；LOG_END返回长度/CRC均一致、头CRC/布局和条数核对通过后才改为正式二进制，再生成57列CSV。失败保留.partial及原因，重连新建导出从0开始，不自动续传旧会话；清晰区分完整导出和实时文件。

处理取消、设备BUSY、NOT_FOUND、IO_ERROR、TCP断开和磁盘失败。正常取消尽力结束/关闭导出连接；设备超时负责最终释放互斥。不猜测掉电后Flash残留记录有效性。

提供离线convert子命令，接受完整二进制日志及可选events.jsonl；没有事件时明确注明参数历史不完整。完整日志与实时记录可按tick/运行段对照，不自动把不同运行拼接。

## 验收和交付

1. unittest覆盖帧长32B、独立CRC向量、空/最大负载、记录96B及每项偏移、错长度/版本/CRC/JSON。
2. 模拟TCP任意拆包粘包、部分发送、可靠重发、业务结果延迟、会话变化及NOT_FOUND。
3. UDP缺口、乱序、重复、PC队列满、索引新运行归零和tick回绕测试。
4. 验证参数整组拼装、非有限/越界拒绝、RECEIVED不等于APPLIED、SAVED与生效分离、断线不重放。
5. 导出重复块/错偏移/整体CRC失败/.partial保留/CSV57列/灰度归一化对照测试。
6. 用模拟对端先完成自动测试；实机60秒1kHz对照Flash导出，核对调参生效拍/元素暂停恢复/停车保存，未实测标待验。
7. 中文README包含Python启动、热点连接、Windows UDP防火墙放行、全部命令、文件格式和故障排查。无需安装第三方包。
8. 仅提交任务明确路径，附实际测试结果，不push。
