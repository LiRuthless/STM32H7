# 桥接、队列与生命周期

> 核对：2026-11-05 工作区；本页F06～F08均未完成生产实现。main中没有bridge_state.c/.h及bridge_stats.c/.h，不能把SPI/NET端口或测试夹具当成桥接交付。Phase05资源批准记录冲突见 [EB-01](../known-issues.md#eb-01)，本轮不解除门禁。需求见 [traceability](../traceability.md)。

<a id="f06"></a>
## F06 可靠接管、转发与请求信用

### 当前事实与目标接口

当前SPI仅有启动前端口，NET对非本地SUBSCRIBE的可靠COMMAND背压不ACK，尚无双侧生产消费者。以下是既有Phase05设计的待实现目标；含R-05资源决定的部分仍待核实批准记录，不宣称已获本轮批准。

输入为完整合法帧、连接/会话代次、容量许可及时间；输出为reserve/commit/cancel、待发帧、信用及ACK状态。SPI/TCP各自重建session/seq/ack/CRC，保持request_id和业务负载；传输可靠ACK只表示接管，不产生RECEIVED/APPLIED/SAVED。业务幂等由STM32负责，相同request_id但新传输身份仍交给STM32，不用最后seq替代业务缓存。

### 待实现正常流程与异常

每方向独立窗口1，接管前取得下游槽位及结果信用，不能先ACK再丢失。R-05方案不新增BUSY业务结果，容量不足不接管/不ACK。已接管帧复制至应用拥有的槽；借用RX视图仅在回收路径有效，不得转交驱动指针。

调参/普通控制、只读查询、导出分别有独立信用；最多一条未获最终结果的调参请求，查询及IDLE仍可推进。查询包括GET_STATUS/GET_PARAMS/GET_REQUEST_RESULT/GET_LOG_META；LOG_BEGIN/READ/END归导出，其他归普通类；分类只服务额度，不判车辆许可。每类最多一个非最终请求，RECEIVED是中间结果，STM32非RECEIVED最终RESULT或匹配LOG_DATA才结束相应阶段；最终帧交付仍等待精确TCP ACK再释放信用。相同request_id传输重发不重复业务，APPLIED/SAVED原样透传。

| R-05资源方案（待核实批准记录） | 数量×数据字节 | 目标owner / 释放 |
|---|---|---|
| COMMAND | 3×1024B | 普通/查询/导出各1槽；转入SPI可靠缓存后还数据，保留信用 |
| RESULT/LOG_DATA | 6×1024B | 每请求类2槽，中间及最终结果独占，不被EVENT占用 |
| 缺口锚点 | 1×1024B上限 | 首条已验证未交付源EVENT；按F07生命周期维护 |

新增帧数据上限10240B，元数据/锁/统计须目标构建实测；已有池数据51072B见F01。方案沿用SPI/NET两任务，无新增任务/栈/运行期堆分配，不等于全SDK无动态分配。

### 状态归属与实现定位

目标bridge_state.c/.h持有槽、信用、代次及借用标记（文件当前不存在）。现有边界：
[spi_protocol.h](../../main/spi_protocol.h)的bridge_spi_port_t定义reserve/commit/cancel、peek_tx/tx_consumed等；
[net_bridge.c](../../main/net_bridge.c)的dispatch和[启动适配](../../main/startup_idf.c)尚未接入该生产状态。实际可靠图像owner见F03，各自任务独占。

目标共享状态由短临界区保护，锁顺序bridge→pool；JSON在任务私有workspace先解析，再进锁。锁内仅有界复制/元数据操作，无socket/JSON/等待/驱动调用。reserve至commit/cancel、peek至consumed采用租借标记；断线/会话清理撤销提交资格但不改写已借出数据，归还时再释放，避免旧payload配新header。

### 验证方法

P05/V-1/V-2/V-10/V-11：以真实生产C贯通双侧，注入满槽、不同seq/ack、丢ACK、延迟APPLIED及查询；逐条核对已ACK帧后续去向、预留信用和字节一致性。当前没有该功能执行证据，工具为待实现，不能以P04主机结果代替。

<a id="f07"></a>
## F07 遥测丢弃与源事件缺口

### 当前与目标

已有UDP/EVENT池和网络UDP出口，生产桥接队列尚未接入。输入为STM32 TELEMETRY/EVENT及订阅/连接身份；输出为有界UDP帧、可靠EVENT或明确缺口，未实现。

目标遥测16包，满时丢旧保新；离线/无订阅直接丢，清队列/socket失败也计丢弃。发送前仅更新payload偏移12的ESP32累计丢弃包数，再独立UDP seq/CRC，保留session/run_id/索引/revision及记录，不跨run拼包；UDP序号耗尽停止订阅并要求重连，不跨回绕。

目标源EVENT队列32条，先预留再ACK，可靠重复不重复入队。EVENT转入TCP可靠缓存后保留首源锚点至精确ACK；断线时已接管但未确认交付及离线接到的首条可信EVENT可形成缺口。队满未ACK可重试，不算确知丢失。

G-03已选择首个源锚点：EVENT_GAP的run_id/tick/control_sequence/revision取第一条已验证未交付源EVENT，不以ESP时钟、零或猜值替代，不增加字段；缺锚点则要求关闭/重连查询。新SPI会话清旧锚点，不能重标旧session。来源STM32 EVENT（包括额外字段）原样转发，与ESP自生G-03报告区分。方案本端已确认，三端兼容仍待验。

### 目标owner及方法

目标bridge_state管理队列、锚点和代次；SPI复制入应用槽，NET取出并重封装，绝不操作驱动持有DMA。池仅管额度，实际池实现见F01；UDP出口见F05。
P05/V-3/V-4：17包填16槽、离线/断线、两run及32事件饱和、重复可靠、无锚点/换会话；判据是丢弃可归因且只改允许字段，满队未ACK不误报缺口，历史未知不伪造。当前未执行；决定依据：[G-03](../../specs/features/05-bridge-lifecycle/g-03-event-gap.md)。

<a id="f08"></a>
## F08 连接清理、单块导出与故障隔离

### 当前事实及输入输出

NET已有G-02关旧TCP和订阅清理，SPI已有逻辑失效；全桥接信用/导出状态不存在。输入是连接/会话变化、合法LOG请求和STM32结果、单调毫秒时钟；目标输出为本端资源取消/清理，不执行车辆参数或Flash动作。

### 待实现状态与错误规则

断网仅清该连接命令/结果/遥测/导出积压，不重置SPI、不发TUNE_DISABLE、不回滚已应用值。已进入SPI可靠缓存的帧不可撤回，可能已经执行；重连须查询，绝不自动重放。新SPI会话清旧信用和队列，G-02让NET关闭旧连接。

日志窗口1，仅缓存有界块，不预读整份日志。LOG_READ length为1～960B，块request_id/offset/length匹配已接管请求，所有业务字节和最终total_bytes/CRC来自STM32；本端不累计整份CRC假冒LOG_END成功。LOG_BEGIN成功结果记录transfer_id/total_bytes；LOG_DATA为匹配的新进度才推进，LOG_END成功清本端状态。

R-05设计前段写“新LOG_READ时刷新活动”，补充确定性段又规定“匹配LOG_DATA才是有效新进度”。两种计时基准需在实施前核实，见 [EB-05](../known-issues.md#eb-05)；不通过本轮文档迁移裁定。共同边界为重复旧请求/旧块不能无限续期，独立计时10秒取消本端导出；无新消息、SPI无时钟也需推进逻辑计时，不改写驱动持有内存。STM32自身10秒释放才决定真实互斥，本端清理不能证明车辆锁释放。

取消清未入SPI缓存的导出命令；已发未答请求保留有限取消墓碑，迟到结果接管后归入取消丢弃，不恢复导出、不阻塞查询。离线RESULT按断线丢弃计数，不发给新连接；离线EVENT保存可信首锚点并标历史未知。

初始化/驱动错误停止受影响服务，普通socket错误仅连接恢复，SPI重试耗尽重握手；不自动EN复位、不新增停车、不改变Flash/安全链优先级。

### 定位、验证与限制

目标bridge_state持有导出空闲/活动/等待块、计时、取消墓碑及连接代次；尚无实现文件。已有session/网络清理分别见F03/F05，SPI逻辑失效见F04，不把局部能力算作完整F08。

P05/V-5～V-7/V-9/V-12：断网/重握手、旧身份、1/960B/末块/重复块、9.999/10秒及持续连接无读取，注入独立时钟观察清理与后续查询。真实STM32释放互斥及导出整体字节/CRC另上机核验；当前均无完整桥接证据。
