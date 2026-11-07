# Phase 05 资源与生命周期设计（2026-10-20）

## 2026-11-07 生产接入实施增量（当前决定）

本组基线为627615d639cc290e19e3629ba49624145db517a0。用户明确授权生产代码开发，替代历史“仅文档、不启动05/06”的本次范围限制；不授权push、合并、烧录、复位或安装/切换SDK。原文及历史结果保留，以下新决定在冲突处优先。阶段内部Group 1→2→3连续实施，Phase05末交审，通过前不进入06。

### 新确认决定

- EB-01：用户重新批准R-05全部固定资源与信用方案。已有池数据51072B；新增COMMAND 3×1024B、RESULT/LOG_DATA 6×1024B、锚点1×1024B，共10240B数据，元数据另计。三类独立信用，最终结果仅在电脑精确ACK后释放。无新增任务或运行期分配。
- EB-05：LOG_BEGIN成功结果提交本端时起算；只有首个匹配已接管READ的request_id/offset/length的LOG_DATA提交才刷新。READ接管、旧请求或重复块不刷新。now-last_progress≥10000ms取消，可靠重试计时独立。本端取消不代表STM32锁释放。
- EB-07：用户批准统一短portMUX保护bridge身份快照及租借状态。NET独占socket；连接代际/session/ready/订阅资格一致发布。reserve/commit与peek/consumed使用代际租借；清理撤销租借而不复用尚未归还的数据。现有UDP交接代际读写同锁。锁内仅有界复制、元数据和bridge→pool操作；不JSON、socket、等待或驱动调用。
- 新增迟答隔离决定：SCB/1没有电脑连接代际字段，普通断线不换SPI会话。每类一个固定墓碑，记录已进入SPI且失去电脑归属的请求；该类自此背压至SPI会话改变，收到最终结果也不解除墓碑。命中旧身份的结果只接管并计断线/取消丢弃，不能交新连接。其他类独立推进；不得驱逐墓碑、自动复位或增加协议字段。导出超时后已发出的请求同样形成墓碑。此为明确可用性限制。

### 生产接口与实现顺序

1. Group 1新增bridge_state.c/.h，启动绑定SPI完整端口及NET接管/出队/ACK接口。NET解码后先解析分类、reserve实际额度，再调用可靠receive；DELIVER后同步复制commit，再允许IDLE ACK；重复/拒绝cancel。SPI沿用同样顺序。每侧独占scb_link_t，不共享DMA或重发图像。
2. COMMAND按普通、查询（GET_STATUS/GET_PARAMS/GET_REQUEST_RESULT/GET_LOG_META）、导出（LOG_BEGIN/READ/END）分三类。信用保存request_id与所属连接/SPI代际，数据复制入SPI可靠缓存后归还COMMAND槽，信用继续保留。RESULT按request_id匹配，RECEIVED占中间槽，其余真实状态占最终槽；LOG_DATA匹配READ后作为最终交付。不得把传输ACK升级为业务成功。最终上行绑定TCP可靠seq；只有可靠层实际从pending转已确认时通知bridge释放相应信用。
3. Group 2用已有16遥测槽和32EVENT槽建FIFO。遥测满丢最旧，离线、无订阅、清队列、socket失败、身份过期逐包归因；累计值写payload+12，UDP重新编码seq/CRC。EVENT可靠队满背压；重复不再次提交。源锚点来自首个已验证未交付EVENT；G-03报告优先可靠发送并保留至精确ACK。新SPI会话清旧锚点，不重标旧session。
4. 连接清理取消未发COMMAND及上行队列/信用、清遥测并计数；已借出槽标撤销，归还前不改写。已进入SPI可靠缓存的COMMAND不可撤回，墓碑隔离迟答。SPI会话变化先撤旧bridge身份，NET随后关闭旧TCP；新连接需HELLO和SUBSCRIBE。无连接RESULT归orphan/断线丢弃，EVENT保留首次可信缺口。
5. Group 3导出只检查线格式及块匹配，不判断停车、Flash或车辆许可。记录BEGIN的transfer_id/total_bytes，READ的request_id/offset/length，LOG_END真实成功清本端导出；不计算整份CRC、不预读。NET有界循环每轮驱动独立毫秒计时，即使无网络数据或SPI无时钟仍推进取消。SPI原poll仍不访问驱动持有DMA。
6. bridge_stats.c/.h聚合各owner快照与bridge计数，NET每5秒触发串口输出，不向socket加入文本。包、记录、字节、背压尝试分别定义；累计计数不按连接清零。CMake纳入新模块；初始化失败阻止受影响服务，不伪装成功绑定。

### 验证和交付

复用tests的生产C协议/SPI/NET夹具；仅补足Phase05生产链路必需的生命周期覆盖，不以Python模型替代。V-1～V-12逐项记录本轮输入、结果和限制；重点包括双侧不同seq/ACK、最终ACK信用、队满/重复、租借撤销、迟答墓碑、9.999/10秒、无SPI时钟及无socket流量。宿主串行夹具不能证明跨核安全，另做全部共享字段读写/锁序审查，实机并发待验。

构建复用既有v6.0.0入口。2026-11-07只读预检：D:\\esp\\v6.0\\esp-idf及C:\\Espressif\\tools\\python\\v6.0\\venv\\Scripts\\python.exe不存在，未经授权不安装或切换；后续实际构建结果单独记录，不拿历史计数或镜像代替。Docs各组先标“本组实现同步中”、任务组及本组更新前代码基线，紧接代码，双向审查后取消标记同批交付。

> 2026-11-07恢复：以下完整正文保存本阶段背景、需求、详细设计、决定、计划、文件/符号映射和验收，按原文日期与基线解释；原“待审/已确认/已完成”不是本次重新批准或验收。当前实现由[Docs](../../../Docs/index.md)说明；未决见[问题页](../../../Docs/known-issues.md)。前层约束：[mission](../../mission.md)、[techstack](../../techstack.md)、[roadmap](../../roadmap.md)、[code-style](../../code-style.md)、[AGENTS](../../../AGENTS.md)。阶段未显式关闭，实机待验保持；本次不启动05/06。

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

## 2026-11-07目标归位与实施前边界

本页原完整资源、信用、并发、事件与导出设计恢复为Phase05主体；原2026-10-20“用户明确确认R-05”保留为当时声明，与[开工证据](../../../Docs/validation/phase-05-preflight.md)及[EB-01](../../../Docs/known-issues.md#eb-01)的“待确认”并列。此次恢复不裁定哪份批准记录有效，实施前仍须核实。3/6/1槽与10240B为未来新增候选额度，不能与已有51072B池混成当前预算。

由原Docs F06～F08归回的详细目标按以下主章节维护：信用/分类/最终精确ACK见“固定额度”；reserve/commit/cancel、peek/consumed、bridge→pool锁序、租借标记和清理见“并发与交付”及“实施接口与确定性边界”；遥测16包/事件32条、首源锚点和导出墓碑见“断线、事件与导出”及“实施接口与确定性边界”。不在现行Docs并存第二套目标。

归位补充：遥测目标按run_id独立封包，不跨run拼包；EVENT_GAP的run_id/tick/control_sequence/revision均取第一条已验证未交付源EVENT，不使用ESP时钟或零猜值。新SPI会话清旧锚点，不能重标旧session；来源STM32 EVENT额外字段原样透传。LOG_READ的length为1～960B，块匹配request_id/offset/length；仅有界缓存一块，不预读整份日志，LOG_END的total_bytes/CRC由STM32提供。目标验证按[本阶段V-1～V-12](validation.md)逐项执行，当前无完整生产桥接证据。

导出计时存在两份原文：前段“新LOG_READ刷新活动”与确定性段“匹配LOG_DATA才为有效新进度”。两者均保留，见[EB-05](../../../Docs/known-issues.md#eb-05)；10秒边界不自行改选。无消息或SPI无时钟仍须推进逻辑计时，但不改写驱动持有DMA；本端取消不证明STM32互斥释放。

统计与并发：R6原258项声明保存在[r6-log-counters](r6-log-counters.md)，当前缺bridge_state/bridge_stats及部分原产物，见[EB-04](../../../Docs/known-issues.md#eb-04)。跨任务代际同步风险见[EB-07](../../../Docs/known-issues.md#eb-07)，这里只登记，不选择锁或原子方案。Phase05/06不因本次文档恢复开工。
