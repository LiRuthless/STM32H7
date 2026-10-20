# Phase 05 资源与生命周期设计（2026-10-20）

状态：2026-10-20用户明确确认R-05，先独立提交规约后实施；G-01/G-02/G-03/G-06沿用已确认决定，不修改SCB/1。

## 固定额度（新增资源决定 R-05）

| 用途 | 数量 × 数据字节 | 所有者与释放条件 |
|---|---|---|
| SPI DMA | 2 × 1024（已有） | 既有驱动所有权规则不变 |
| 遥测队列 | 16 × 1016（已有池） | SPI复制入队，网络取出；满丢旧，无订阅直接丢并计数 |
| 源EVENT队列 | 32 × 1024（已有池） | 先预留后ACK；转入TCP可靠缓存后释放池槽 |
| COMMAND槽 | 3 × 1024（新增静态） | 调参/普通控制、只读查询、导出各1槽；转入SPI可靠缓存后释放数据，但保留请求信用 |
| RESULT/LOG_DATA槽 | 6 × 1024（新增静态） | 每类请求独占2槽，分别容纳中间RECEIVED与最终结果；不能被EVENT占用 |
| 缺口锚点 | 1 × 1024（新增静态上限） | 本会话第一条确知未交付源EVENT；生成G-03报告后清除 |

已有池数据51072B不变；新增静态帧数据上限10240B，元数据、锁及统计以目标ELF实测回填。沿用SPI/NET两任务，不增任务、栈或运行期堆分配。查询信用独立于调参最终结果，导出信用独立于查询；每侧传输仍窗口1，不等于所有业务共享一把锁。

每类最多1个非最终请求，RESULT先透传RECEIVED，只有STM32的非RECEIVED最终RESULT或LOG_DATA能结束该类请求。最终结果转入TCP可靠缓存后请求仍等待该帧精确ACK，防止下一请求占用最终交付信用。重传按传输身份去重；新传输身份的相同request_id仍转交STM32缓存决定。连接内递增request_id只用作导出新读取识别，不解释参数。

查询包括GET_STATUS/GET_PARAMS/GET_REQUEST_RESULT/GET_LOG_META；LOG_BEGIN/READ/END归导出类，其他COMMAND归普通类。分类只服务额度，不判断车辆许可。只校验已有线格式；不生成新的BUSY业务结果，容量不足不接管、不ACK。

## 并发与交付

生产SPI端绑定reserve/commit/cancel、peek/consumed、telemetry、session_changed。生产NET端绑定同一状态的TCP接管和出队接口。共享状态用短临界区保护，区内只做有界内存复制/池元数据操作；无JSON、socket、等待或驱动调用。JSON字段在各任务自有workspace中先解析，再进锁。借用帧指针在独占暂存槽中保持到consumed，不借出驱动DMA；清理用连接/会话代次撤销提交，不能把旧payload配新header。

网络发送从固定队列取出后重新封装本侧session/seq/ack/CRC，TCP可靠缓存承担重试；SPI同理。IDLE与查询持续推进。ACK仅表示接管；调参最终结果与保存状态必须由STM32原样提供。

## 断线、事件与导出

断线清除该连接命令/结果/导出/遥测积压，不重置SPI、不发TUNE_DISABLE。已进入SPI可靠缓存的帧不能撤回，可能已应用，重连须查询，绝不自动重放。断线时已接管而未确认交付的首个EVENT以及离线时接到的首个EVENT可作G-03锚点；缺少可信设备字段时关闭/要求重连查询，绝不猜值。可靠满队未ACK不算缺口。新SPI会话清旧锚点、全部旧信用及队列，G-02关闭TCP。

LOG_BEGIN成功结果启用本端导出计时；LOG_READ新request_id且线格式有效时刷新活动；重复旧request_id不续期。LOG_DATA/LOG_END字节透明，不计算或伪造整体CRC。10秒无新有效读取独立取消本端导出待办；已进入传输缓存的帧遵循原可靠性规则，真实STM32仍须依自身10秒超时释放互斥。没有新增取消操作码。

遥测发送时仅更新payload偏移12累计ESP32丢弃包数，独立UDP seq/CRC；socket失败、离线、无订阅、丢旧及清队列均累计。序号耗尽停止该订阅并要求重连，不能跨回绕。

## 验证约束

用生产C贯通双侧协议与生命周期，不以Python模型替代实现。覆盖最终结果信用、查询并行、32事件饱和、16遥测丢旧、跨侧重封装、丢ACK重发、会话代次竞态、断线锚点及9.999/10秒导出边界。主机、目标build/size和实机三类证据分开。

## 实施接口与确定性边界

新增main/bridge_state.c/.h负责上述固定槽、池队列、信用、缺口和导出；main/bridge_stats.c/.h聚合SPI/NET/桥接/池快照，串口定期诊断只由网络任务发起。修改startup_idf绑定消费者、net_bridge接管COMMAND及消费可靠上行/遥测、spi_protocol沿用既有生产端口、CMake纳入新模块。tests/host编译生产C，新增tests/test_bridge_lifecycle.py覆盖05/V-1～V-12。

SPI端reserve解析RESULT status及EVENT锚点必须用SPI独占有界workspace；NET用自身workspace。互斥只保护内存和元数据，池锁顺序固定为bridge→pool。reserve到commit/cancel、peek到consumed之间采用租借标记；断线/会话清理只取消租借，不改写借出数据，归还时再释放。EVENT入TCP缓存后保留首源锚点直到精确ACK，避免断线丢失最后一条待交付事件。缺口报告也按可靠ACK生命周期管理，满队未ACK不生成缺口。

无控制连接时SPI仍接管并计数过期RESULT和EVENT：RESULT显式归入断线丢弃，EVENT保存首次可信锚点并计历史未知；不向新连接转发旧业务结果。连接建立后尚未HELLO/订阅的遥测均计丢弃。任何COMMAND只有SPI非零session且ready=true才可接管，包括SUBSCRIBE；等待IDLE握手的非零session不表示业务就绪。

导出只跟踪线格式，不代替STM32检查停车、许可或Flash；记录LOG_BEGIN成功的transfer_id/total_bytes，LOG_DATA匹配本端已接管READ的request_id/offset/length时才作为有效新进度，重复旧块不续期。LOG_END成功清本端导出。取消清未进入SPI可靠缓存的导出命令；已经发出且尚未回答的导出请求保留有限取消墓碑，后续结果接管后计入取消丢弃，不恢复导出、不阻塞普通查询。资源占用不得因10秒取消而增加。
