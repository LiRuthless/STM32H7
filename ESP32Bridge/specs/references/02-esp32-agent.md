# ESP32-S3 agent 执行任务书
> 文件：02-esp32-agent.md；版本：2026-10-16；交付 ESP-IDF 工程、协议测试及构建/烧录说明。

## 任务与授权边界

你负责ESP32-S3-DevKitM-1作为SPI从机和Wi-Fi桥接器。只转发业务，不解释控制参数、不判断元素、不决定停车。

仓库：E:\study\Intelligent Vehicle\STM32H7。检查Git状态并阅读根目录AGENTS，在ESP32Bridge新建独立工程。不修改SmartCar、Tools/SmartCarLink、Hardware、Material、报告任务书或其他在途工作，不把实现放参考资料目录。

ESP-IDF C固定v5.5.1，目标esp32s3，不依赖PSRAM。当前仓库路径有空格，提供脚本将本工程复制到无空格临时目录构建，再回收产物，不移动仓库。先检测工具链，缺失明确记录并给安装步骤，不静默换版。

用户已确认本设计，外部协议不能单端改动。仅明确路径提交，不push，不为等待对端停止模拟验证。

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

## ESP32 具体实现

### A. 工程和配置

模块建议：app_main、spi_bridge、net_bridge、protocol、固定缓冲/队列、Kconfig配置。启动顺序NVS→网络接口→SoftAP→socket→SPI任务。

SPI引脚10/12/11/13，GPIO14作READY；未用四线信号设-1，不能把GPIO14同时用为WP。配置SPI2_HOST、SPI_DMA_CH_AUTO。DMA缓冲固定池，启动分配一次并检查，不在数据路径反复malloc；不依赖外部RAM。

热点地址192.168.4.1/24、DHCP开启，SSID及密码按公共协议。密码放被忽略的本地配置，示例不包含真实密码。只接一个控制客户端，网络是否连接不控制SPI启停。

### B. SPI任务

固定Core1，优先级12，事务深度1，在该核初始化驱动。使用spi_slave_initialize、spi_slave_queue_trans、spi_slave_get_trans_result。网络任务优先级5。

post_setup回调拉低READY，post_trans回调撤销READY；回调只做IRAM兼容的GPIO/轻量标志，不JSON、不socket、不阻塞。事务收回后验证trans_len==8192，再检查长度、CRC、会话。驱动持有缓冲时应用不读写。

处理结果、回收/准备TX与RX、重排事务后才能再给READY。SPI收到命令后的结果在后续事务返回，不能假设同次响应。

SPI任务绝不等待socket发送/电脑连接。离线时实时包直接丢弃并计数；可靠命令/结果独立受额度控制。CRC错不接管业务，可靠层超时恢复；截短事务可回收重排但不执行内容。STM32不发时钟时阻塞等待事务是正常状态，不轮询忙等。

### C. 网络桥接

TCP服务维护单控制会话，socket使用有界等待/可写检查，处理部分发送、拆包粘包、连接关闭和端口占用。一个消息最大1024B，输入长度先校验，禁止无限缓存。

网络可靠队列和SPI可靠队列独立；转发重编帧头/CRC，request_id及业务内容保持。只有SPI提交成功/结果返回后转发相应业务状态，不能把ESP32收包伪装为STM32 APPLIED。

SUBSCRIBE由本地处理，验证端口1～65535，IP取当前TCP对端。UDP队列16包，满时丢旧包并计数；更新丢弃字段后重算CRC。每次断线取消订阅，不积攒离线实时记录。

可靠通道不足时停止接管更多可靠请求或返回BUSY；不能先确认接管后静默丢失。保持一条业务调参在途，对GET_REQUEST_RESULT等查询提供通路。导出窗口1，只转交电脑请求的块，不缓存整份日志。

EVENT队列32条，遗漏上报EVENT_GAP。控制端断开时向STM32发送内部取消导出通知，或确保其10秒超时释放；不能因此关闭调试覆盖或清空已应用值。重连不重放任何旧设置。

### D. 状态和故障

公开SPI/TCP/UDP包数、CRC错、截短事务、重试、队列峰值、UDP丢弃及当前session；调试打印只走本地串口，不混入业务socket。

ESP32重启换新会话；STM32重握手期间丢旧帧。网络断开不重置SPI会话，SPI会话改变通知电脑并取消旧订阅/导出。

电源/驱动初始化失败可明确报错停止启动相应服务；不能在运行路径遇到普通socket错误就整机重启。无权改变STM32安全策略。

## 验收和交付

1. 编解码/可靠性测试覆盖公共向量、CRC、错长度、重复帧、背压、会话变化、ACK与业务结果分离。
2. ESP-IDF v5.5.1编译目标esp32s3，记录实际工具版本、静态/堆内存及启动配置；未安装/未实测如实标记。
3. SPI联调验证GPIO、Mode0、READY/CS、一事务1024B、trans_len=8192、截短恢复。
4. 连续1kHz/96kB每秒原始记录转发，统计队列/丢弃。慢TCP、UDP拥堵、断线不能阻塞SPI任务。
5. 验证第二个控制端拒绝、无第三方UDP目标、旧命令不重放、订阅重建和导出窗口1。
6. 对照STM32验证APPLIED/保存事件原样透传、导出字节及CRC一致。
7. 交付工程、协议头/向量、被忽略的本地配置与示例、无空格路径构建脚本、中文安装/编译/烧录/联调README。只提交任务明确路径，不push。

官方参考：
- https://docs.espressif.com/projects/esp-idf/en/v5.5.1/esp32s3/api-reference/peripherals/spi_slave.html
- https://docs.espressif.com/projects/esp-idf/en/v5.5.1/esp32s3/get-started/windows-start-project.html
- https://raw.githubusercontent.com/espressif/esp-idf/v5.5.1/examples/protocols/sockets/tcp_server/main/tcp_server.c
