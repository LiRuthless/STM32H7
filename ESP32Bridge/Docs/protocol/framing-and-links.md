# SCB 帧与链路

> 核对：2026-11-05 工作区。公共字段、格式和命令正文唯一引用 [SCB/1 公共契约](../../../SmartCar/Docs/services/communication.md#scb1)；本页只维护 ESP32 适配。原提取协议是[历史基线](../../specs/protocol/scb-v1.md)，不随本轮改写。需求见 [traceability](../traceability.md)。

<a id="f02"></a>
## F02 显式帧编解码与有界 JSON

### 用途、状态及接口

已实现的纯编解码与 JSON 校验，不执行 ACK、socket、驱动或车辆业务。scb_encode 输入字段值及负载、传输类型、输出容量和工作区，返回错误码及 used；scb_decode 输入完整字节和长度，输出借用负载的 frame view。输入在视图使用期间保持有效；失败清空 view/used，不分派部分数据，输入输出不得非法别名。普通 C 结构体只是字段值，不是线图像。

本端逐字段小端处理，头 32B、payload≤992B。CRC 按公共契约覆盖头前 28B 和实际负载，不覆盖 CRC 字段与 SPI 填充。SPI 编码恰为 1024B、尾零；SPI 解码忽略填充，TCP/UDP 必须恰好一完整帧无填充。UDP 只容纳 TELEMETRY，上限 1016B。按类型检查 flags、record_count、LOG_DATA 长度和 reserved 等格式；车辆业务参数、请求缓存、生效和保存不属于本端。

JSON 使用调用任务独占的有界 workspace，按 payload_len 消费整个 UTF-8 文本，无需尾 NUL。拒绝非法 UTF-8、语法/尾随数据、转义后重复键、非有限数和超长/截断；字段读取按 JSON 语义，不靠子串搜索。返回的视图不转为长期共享状态。

### 实现与状态归属

| 文件 / 关键符号 | 职责与上下文 |
|---|---|
| [protocol.c](../../main/protocol.c)、[protocol.h](../../main/protocol.h)：scb_encode、scb_decode、header_check、payload_check | 纯字段校验、显式编码、布局断言；调用者持输入/输出 |
| [json_guard.c](../../main/json_guard.c)、[json_guard.h](../../main/json_guard.h)：bridge_json_validate、bridge_json_object_get | 有界解析和字段视图；SPI/NET 使用各自工作区 |

### 验证与证据

复用 test_protocol、test_vectors 以及 generate_vectors/check_vectors。对合法/错 CRC/错长/重复键/非法文本进行独立解析与 CRC 核验；失败无部分视图，SPI 尾零且 TCP 无填充。固定向量须保持原始 LF 字节和 SHA256；[本端候选向量](../../tests/vectors/scb-v1.md)不等于三端已一致。历史结果：[P02 G1](../validation/phase-02-group-1.md)、[G3](../validation/phase-02-group-3.md)。当前不重新计数或重跑。

<a id="f03"></a>
## F03 独立可靠窗口与会话

### 状态、输入和输出

已实现并由 SPI/NET 各自接入。每传输实例独占 session、seq/ack、TX 重发图像和最近可靠 RX 身份（各图像上限 1024B）；无 DMA/socket 指针，无运行期分配。API 由该实例所属任务串行调用，不保证多任务同时调用安全。输入是完整帧、已拥有的下游预留槽、实际发送完成和单调毫秒时间；输出 DELIVER、DUPLICATE、BACKPRESSURE、RESEND、REBUILD 等动作。

### 正常与异常流程

普通 seq 从 1 单调增加，每方向窗口 1；精确 ACK 才释放 TX。reserve 表示真正取得槽所有权，不能以“当前看似有空位”代替；返回 DELIVER 后必须提交复制，再允许可靠 ACK 发出。重复可靠帧按完整身份判定，仅确认不重复交付；同 seq 改内容为错误，非可靠高序号不能遮蔽可靠重发。接管 ACK 不生成 APPLIED/SAVED，业务 request_id 幂等仍由 STM32 负责。

首次全部字节/完整事务实际发送完成后才启动 200ms 计时；短写不算完成。重发使用原字节/seq，每次完整发送才增加次数，最多三次，第三次后再等 200ms 无 ACK 才输出重建。IDLE 可在可靠窗口被占用时推进 ACK；近回绕不跨回绕比较，返回重建。具体错误动作由 SPI/NET owner 执行。

SPI 为 WAIT_HELLO → WAIT_IDLE → READY：零会话 HELLO 获取非零且不同于旧值的硬件随机 session，后续事务发 HELLO_ACK；握手重复 HELLO 复用会话，收到新会话 IDLE 后才 ready。READY 收到新 HELLO 换会话；随机值不合格不能固定值回退。TCP HELLO 只读 SPI 快照，不重置 SPI。按 G-02，会话变化立即撤销旧业务许可，由网络 owner 关闭 TCP，重连重新 HELLO/订阅；SPI 不执行 socket。零会话状态连接与 BUSY 适配见 [F05](../transport/network.md#f05)。

### 实现与验证

| 文件 / 关键符号 | owner / 上下文 |
|---|---|
| [reliable_link.c](../../main/reliable_link.c)、[reliable_link.h](../../main/reliable_link.h)：scb_link_reset/receive/prepare/sent/poll | 各 SPI/TCP owner 独占实例；轮询只输出动作 |
| [link_session.c](../../main/link_session.c)、[link_session.h](../../main/link_session.h)：scb_session_spi、scb_session_spi_lost、scb_session_spi_pending_reply、scb_session_tcp_open/close/hello/permitted、scb_session_tcp_hello_explicit、scb_session_tcp_busy | SPI 任务持会话；NET 用显式快照编码，不共享可写 SPI 对象 |

复用 test_reliable/test_sessions：199/200ms、短写、错 ACK、精确 ACK、三次重发、满槽、内容改变、旧会话、重复 HELLO、近回绕；判据是独立窗口不互相重置，未预留不 ACK，不会重复分派。历史：[P02 G2](../validation/phase-02-group-2.md)。真实跨端互操作仍待验，来源决定见 [G-02](../../specs/features/02-scb-protocol/g-02-session-change.md)。

## 本轮存量整改追溯

2026-11-05：[Phase01～04格式与注释整改契约](../../specs/research/2026-11-05-source-style-remediation-draft.md)；47文件静态整改与人工复核已完成，目标构建未验证，见[本轮报告](../validation/2026-11-05-source-style-review.md)；现行功能、状态owner和模块映射不变。阶段验收及硬件待验不因本次整改自动完成。
