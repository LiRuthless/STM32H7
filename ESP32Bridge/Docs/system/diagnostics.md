# 本地诊断

> 核对：2026-11-05 工作区；需求见 [traceability](../traceability.md)。

<a id="f09"></a>
## F09 快照与串口计数

### 状态、接口与规则

2026-11-07新增bridge_state快照及bridge_stats汇总，NET独立每5秒触发串口诊断，不依赖SPI等待超时分支；旧R6的258项仍属历史。最终代码未完成目标构建/阶段验收，详见[本轮报告](../validation/2026-11-07-production-bridge.md)。

bridge_state_snapshot由同一bridge锁复制：commands/results为实际提交包数；orphan_results为无归属结果，canceled_results为墓碑或租借失去归属结果；三类backpressure为预留被拒尝试数，可能包含可靠重复的预留尝试，不是丢包数。telemetry_dropped为本端累计丢弃包数，其offline/overflow/cleared/failed四归因之和对应总数；peak为固定队列历史包/条峰值。credit_mask/tombstone_mask分别为全局活动请求与墓碑，仅bit0有效。export_timeouts为10秒独立取消次数。均沿用uint32自然回绕，不提供运行期清零；连接变化不清累计值。

bridge_stats各自读取SPI/NET/池/bridge快照，不能宣称四者是同一时刻的整体事务快照；日志在所有锁外，不含业务负载或密码。完整sendto不是电脑接收证据，帧数据预算不是实测堆栈。

输入为各服务实际处理事件；输出是包数、CRC错、截短、重试、背压、丢弃、队列额度/峰值、session/ready、服务错误和栈余量等内部快照。单位以实际字段为准：事务/包/记录不能混用；容量推导与真实堆栈测量分开。完整公开字段和计数点见下表；头文件保存C声明。

SPI任务发布自己的health快照，NET发布自己的状态，调用者短临界区读取副本，不持锁解析、操作socket或等待驱动。app_main打印启动状态；SPI现有约5秒日志包括invalid、duplicates、backpressure、delivered、rebuilds、telemetry_dropped、invalid_outbound等既有protocol计数。本地日志不承担业务传输，不包含密码或向SCB/1增加命令/统计字段。统计汇总不应改变计数点、重试/队列语义或清零时机。

### 实现与状态归属

| 文件 / 关键符号 | owner |
|---|---|
| [app_main.c](../../main/app_main.c)：app_main | 启动报告打印；具体服务职责见F01 |
| [spi_bridge.c](../../main/spi_bridge.c)：publish_health、bridge_spi_snapshot、run_transactions | SPI任务写，其他任务读取快照 |
| [net_bridge.c](../../main/net_bridge.c)：bridge_net_snapshot | NET任务写，短临界区读取 |
| [buffer_pool.c](../../main/buffer_pool.c)：bridge_pool_snapshot | 池元数据锁内复制；无数据指针转移 |
| [bridge_stats.c](../../main/bridge_stats.c) / [bridge_state.c](../../main/bridge_state.c) | NET调度串口汇总；bridge快照在共享短锁内复制，数据解析/日志在锁外 |

### 验证及历史边界

向既有SPI/网络/池测试注入已知数量事件，核对快照增量和错误归因；串口只含本地诊断、无密码、业务socket无调试文本。实际内存/周期与负载需对应镜像实测。历史P03/P04证据不等于整机统计通过。
[R6增量记录](../../specs/features/05-bridge-lifecycle/r6-log-counters.md)保留2026-10-22原批准、目标和258项回归描述；2026-11-05只读发现SPI日志字段存在、桥接模块缺失，2026-11-07已新增桥接模块，未发现足以重验其全部声明的当前原始产物，不把258改为当前运行结果。

## 本轮存量整改追溯

2026-11-05：[Phase01～04格式与注释整改契约](../../specs/research/2026-11-05-source-style-remediation-draft.md)；47文件静态整改与人工复核已完成，目标构建未验证，见[本轮报告](../validation/2026-11-05-source-style-review.md)；现行功能、状态owner和模块映射不变。阶段验收及硬件待验不因本次整改自动完成。


## 快照一致性与复位规则

SPI任务局部health从初始化开始累计，publish_health同时复制协议stats/session/ready及SDK水位，短临界区发布；NET同理publish，连接关闭不会重置health累计计数，初始化/服务退出结果分别保留。没有运行期清零API，uint32计数不承诺饱和。SPI协议lost只重建逻辑链路、不清stats；init才清实例。池used/peak由池锁保护，release只减used不减peak，deinit后重建重置池统计但代际计数跨deinit保留。快照是最近一次发布副本，不保证读取时仍在线，不能凭peer_ipv4推断活动身份。SDK水位直接保留uxTaskGetStackHighWaterMark返回值；ESP-IDF v6.0 ESP32-S3官方说明单位为字节，不乘字长，不能当成声明的静态栈容量。依据见[官方v6.0内存指南](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-guides/performance/ram-usage.html#run-time-methods-to-determine-stack-size)，本次未装载或更换SDK。

<a id="f09-spi"></a>

### SPI服务

来源[spi_bridge.h](../../main/spi_bridge.h)，写入owner：SPI任务；复位/保留按上节。

| 字段（类型） | 真实计数点/状态、单位与限制 |
|---|---|
| `esp_err_t init_error` | 实际驱动初始化结果。 |
| `esp_err_t service_error` | 事务循环返回错误，未返回时为初值。 |
| `uint32_t queued` | 驱动成功接受的排队事务次数。 |
| `uint32_t completed` | 取回原描述且成功reclaim的事务次数。 |
| `uint32_t truncated` | 回收长度小于8192bit的次数。 |
| `uint32_t overlength` | 回收报告长度大于8192bit的次数；不覆盖硬件饱和盲区。 |
| `uint32_t wait_timeouts` | get_trans_result等待超时次数，不等于回收。 |
| `uint32_t session_id` | 发布时逻辑SPI会话，零表示无会话。 |
| `uint32_t stack_free_min` | 当前SDK报告的任务历史最小剩余栈字节数。 |
| `bool link_ready` | 发布时SPI握手已进入READY。 |
| `bool driver_owned` | 发布时应用尚未取得驱动回收证据。 |
| `bridge_spi_protocol_stats_t protocol` | 同一发布点的SPI协议累计计数副本。 |

### SPI协议子快照

来源[spi_protocol.h](../../main/spi_protocol.h)，写入owner：SPI任务；复位/保留按上节。

| 字段（类型） | 真实计数点/状态、单位与限制 |
|---|---|
| `uint32_t received` | 成功完整解码的SPI入帧数。 |
| `uint32_t crc_errors` | 入帧CRC不符次数。 |
| `uint32_t invalid` | 其他格式、方向或会话拒绝次数。 |
| `uint32_t duplicates` | 可靠重复或旧帧累计次数。 |
| `uint32_t backpressure` | 可靠下游未预留槽导致的背压次数。 |
| `uint32_t delivered` | 同步commit可靠帧次数。 |
| `uint32_t telemetry_packets` | 已交付到遥测分支的包数。 |
| `uint32_t telemetry_records` | 这些遥测包中记录条数总和。 |
| `uint32_t telemetry_dropped` | 无遥测消费者或消费者拒绝的包数。 |
| `uint32_t records_dropped` | 上述丢弃包内记录条数。 |
| `uint32_t retries` | 实际接受的可靠重发完成次数。 |
| `uint32_t rebuilds` | 逻辑SPI会话重建次数。 |
| `uint32_t rng_failures` | 握手未取得合格随机候选次数。 |
| `uint32_t invalid_outbound` | 待发COMMAND方向或编码被拒绝次数。 |

<a id="f09-net"></a>

### 网络服务

来源[net_bridge.h](../../main/net_bridge.h)，写入owner：NET任务；复位/保留按上节。

| 字段（类型） | 真实计数点/状态、单位与限制 |
|---|---|
| `esp_err_t init_error` | 网络任务socket初始化结果。 |
| `esp_err_t service_error` | 服务循环返回的错误，未返回时为初值。 |
| `uint32_t connected` | 成功占用首控制槽的连接次数。 |
| `uint32_t rejected` | 控制槽已占时的新连接次数。 |
| `uint32_t received` | HELLO成功及COMMAND/IDLE处理分支计数，不等于业务交付数。 |
| `uint32_t crc_errors` | 完整入帧CRC错误次数。 |
| `uint32_t invalid_closes` | 非法头/帧/许可或应答编码失败导致关闭的计数。 |
| `uint32_t sent_bytes` | 控制及BUSY正数send累计字节数，不含UDP。 |
| `uint32_t short_writes` | send短写或返回零次数。 |
| `uint32_t stalls` | 控制帧无进展超过阈值而关闭次数。 |
| `uint32_t link_failures` | 非零会话可靠重建导致的关闭次数。 |
| `uint32_t subscriptions` | 合法本地SUBSCRIBE接受次数。 |
| `uint32_t udp_sent` | 出口确认完整发送的遥测报文数。 |
| `uint32_t udp_failed` | 调用出口但未完整发送的报文数。 |
| `uint32_t udp_dropped` | 消费时无资格或折叠溢出的丢弃计数。 |
| `uint32_t backpressure` | 无生产消费者的可靠COMMAND背压次数。 |
| `uint32_t session_closes` | SPI会话变化而关闭控制连接次数。 |
| `uint32_t stack_free_min` | SDK报告的网络任务历史最小剩余栈字节数。 |
| `uint32_t peer_ipv4` | 最近记录的TCP对端IPv4，网络字节序；关闭后可保留旧值。 |
| `uint16_t udp_port` | 发布时有效订阅端口，未订阅为零。 |
| `bool listening` | 发布时监听fd有效。 |
| `bool control_open` | 发布时首控制连接fd有效。 |
| `bool subscribed` | 发布时本连接已本地订阅。 |

<a id="f09-pool"></a>

### 固定池

来源[buffer_pool.h](../../main/buffer_pool.h)，写入owner：池元数据锁；复位/保留按上节。

| 字段（类型） | 真实计数点/状态、单位与限制 |
|---|---|
| `size_t allocated_bytes` | 数据块容量总和，字节，不含SDK/任务占用。 |
| `size_t metadata_bytes` | 池状态及代际计数器大小，字节。 |
| `uint32_t capacity[BRIDGE_POOL_KIND_COUNT]` | 各类池配置槽数。 |
| `uint32_t used[BRIDGE_POOL_KIND_COUNT]` | 快照时各类借出槽数。 |
| `uint32_t peak[BRIDGE_POOL_KIND_COUNT]` | 各类历史借出槽数峰值。 |
| `bridge_dma_owner_t dma_owner` | 快照时成对DMA所有者。 |

### 计数路径补充

SPI received在完整scb_decode成功后立即增加，之后仍可能因方向/会话非法被拒；delivered仅reserve成功并同步commit后增加。telemetry_packets/records先计进入遥测分支，再以telemetry_dropped/records_dropped区分回调缺失或拒绝；不能相减推断端到端电脑接收。retries由可靠层接受实际重发完成才增加，rebuilds为逻辑lost调用，rng_failures是一次HELLO四候选均不合格的一次失败。

NET received在HELLO成功及COMMAND/IDLE对应分支推进时增加：SUBSCRIBE因TX窗口忙而延后时不增加，其他无消费者COMMAND可计received同时计backpressure。sent_bytes只累计控制和BUSY正数send，short_writes含send=0但不含EAGAIN；stalls在控制帧无正进展严格大于2000ms时增加。udp_dropped包括无资格/旧代际和布尔overflow折叠：多个忙offer可只折为一次，不是精确每次丢包数。udp_failed与udp_sent只在真正调用出口后区分，完整sendto不证明空口到达。

pool allocated_bytes固定51072B，metadata_bytes为sizeof(s_pool)+sizeof(s_next_generation)，不是实测总堆。capacity/used/peak的数组下标分别UDP、EVENT；DMA owner是状态不是次数。

<a id="f09-logging"></a>
## 日志触发与字段

spi_bridge.c::run_transactions只在get等待超时分支检查now-last_log≥5000ms并打印，随后重置last_log；持续完成事务时不保证每5秒打印。日志queued/completed/short分别映射排队/回收/截短，crc/retry/invalid/dup/backpressure/delivered/rebuilds/tele_drop/invalid_out为同次快照协议字段，session/ready为逻辑状态，stack_min为SDK返回的剩余栈字节数，internal_free/internal_min是内部8bit堆当前/历史最小空闲字节。app_main另打印启动报告和池容量；本地诊断不进入业务socket，不含密码。2026-11-07统一bridge_stats已新增，NET独立5秒调度；历史258项不变成本次统计验收。
