# SPI 从机与 READY

> 核对：2026-11-05 工作区；需求见 [traceability](../traceability.md)。公共引脚及线格式引用 [SCB/1](../../../SmartCar/Docs/services/communication.md#scb1)。

<a id="f04"></a>
## F04 DMA 事务、READY 与 SPI 协议适配

### 输入输出、配置与状态

本端事务循环及协议适配已实现，真实电气和吞吐待验。SPI2_HOST、自动 DMA、Mode0、8bit、MSB first、全双工；CS10/SCK12/MOSI11/MISO13、READY14 低有效，WP/HD/扩展线 -1。一次事务配置 8192bit，队列深度 1。Core1、优先级12、静态栈6144B；实际核内初始化驱动。驱动 SDK 分配另计，不依赖 PSRAM。

<a id="f04-transaction"></a>
### 调用与所有权

应用准备 TX、清 RX/描述 → pool submit → queue_trans(0) → post_setup 拉低 READY → post_trans 拉高 → get_trans_result 成功且返回原描述 → reclaim → 首先核对 trans_len → 处理 RX/准备下一帧。两回调 IRAM_ATTR，只访问 GPIO，不解析、不日志、不加等待锁；回收后任务上下文才做有界 codec/JSON，1492B 工作区由 SPI 任务独占。

get 每次有界等待约10ms（至少1 tick）；无时钟时继续阻塞等待，并轮询逻辑重试/统计，不接触驱动持有的描述或 DMA。定时器失效只撤销逻辑状态，不等于回收。重发待办须回收后才能复制入 TX；等待中精确 ACK 可撤销待办。

完整物理事务先确认上一 TX 完整发送，再处理 RX；坏 RX CRC 不抹掉真实 TX 完成事实。截短不确认 TX 完成，后续继续原可靠图像。合法 HELLO 的回复位于后续事务；随机数最多尝试4次，不合格就等待对端重发，不伪造握手成功。入站只接受既定方向；可靠业务调用 reserve/commit/cancel，回调借用视图必须立即复制，不得保留 RX 指针。telemetry/peek_tx/tx_consumed/session_changed 均为有界无 socket 的启动绑定端口。

<a id="f04-ownership"></a>
### 异常与兼容

排队失败若证明未接受本描述，可撤回所有权并停止该服务。get 错误或意外描述不能证明回收：保持固定内存及驱动所有权有效，撤销 READY、停止服务，不销毁池或复位。可见截短/超长、坏帧不 ACK，回收后重排；硬件可能将超长 trans_len 饱和到8192，不能用该值证明没有额外时钟。

2026-11-07启动绑定bridge_state生产消费者；reserve/commit/cancel及peek/consumed实际连接固定信用/FIFO。无额度仍背压，不能由宿主结果推定目标/实机已通过。

### 实现位置

| 文件 / 关键符号 | owner 与上下文 |
|---|---|
| [spi_bridge.c](../../main/spi_bridge.c)、[spi_bridge.h](../../main/spi_bridge.h)：bridge_spi_start/configure/snapshot、spi_task、run_transactions、ready_loaded、ready_finished | SPI 任务独占 DMA/描述/协议对象；快照短临界区发布；回调仅 IRAM GPIO |
| [spi_protocol.c](../../main/spi_protocol.c)、[spi_protocol.h](../../main/spi_protocol.h)：bridge_spi_protocol_init/poll/lost/sent/receive/prepare、bridge_spi_port_valid | 同一 SPI owner；poll 不接收 DMA 指针，sent/receive/prepare 仅在回收路径 |

### 验证方法与限制

复用 test_spi_driver/test_spi_lifecycle：0/8191/8192/可见8193bit、坏帧、排队失败、get错误/错描述、无时钟定时及回调顺序。判据是持有期间描述与 TX/RX 不被应用改变，无虚假 ACK/READY。IRAM 段和调用链须对对应目标 ELF 核验；上板以逻辑分析仪测 CS/SCK/READY 和实际1024B，约1.875MHz到约7.5MHz按真实主机时钟核算。历史：[P03](../validation/phase-03.md)，G-06 决定见[阶段记录](../../specs/features/03-spi-slave/g-06-task-json.md)。

## 本轮存量整改追溯

2026-11-05：[Phase01～04格式与注释整改契约](../../specs/research/2026-11-05-source-style-remediation-draft.md)；47文件静态整改与人工复核已完成，目标构建未验证，见[本轮报告](../validation/2026-11-05-source-style-review.md)；现行功能、状态owner和模块映射不变。阶段验收及硬件待验不因本次整改自动完成。


<a id="f04-ready"></a>
## READY与任务参数

参数来源spi_bridge.h/c，均编译期固定，初始化于Core1的spi_task，owner为SPI任务；READY引脚另由启动保护先撤销。静态容量不能当作实测余量。

| 真正符号/配置字段 | 值、单位、范围/生效条件 |
|---|---|
| BRIDGE_SPI_CORE/PRIORITY/STACK_BYTES | Core1 / 12 / 6144B静态栈，单次start |
| SPI2_HOST / SPI_DMA_CH_AUTO | SPI2从机、SDK自动DMA，驱动初始化时 |
| buscfg引脚 | CS10、SCK12、MOSI11、MISO13；WP/HD及扩展线-1 |
| mode / flags | Mode0，8bit，MSB first，全双工 |
| queue_size / transaction.length | 1个 / 8192bit，每次固定1024B |
| wait_ticks | max(pdMS_TO_TICKS(10),1)，单位tick；真实等待依赖当前tick配置 |
| READY GPIO14 | 低有效；post_setup低、post_trans高；仅实际排队回调发布 |
| random候选次数 | 每次HELLO最多4次，不合格不固定值回退 |

READY表示驱动已装载事务，不能由逻辑ready或计时器推定。无主机时钟时get循环继续等待，协议poll可请求重建但不能释放DMA；错误退出也保留可能被驱动引用的固定资源。主机实际频率和电气上限不能从从机配置推定；约1.875MHz至7.5MHz属于待按主机/波形核验范围，不是本轮实测性能。

<a id="f04-adapter"></a>
## 回收后的协议适配

spi_protocol.c只在回收后收到RX借用视图；decode成功后仍检查READY阶段、session、非零且非UINT32_MAX的seq、入站类型IDLE/TELEMETRY/RESULT/EVENT/LOG_DATA、EVENT/TELEMETRY request_id=0和RESULT/LOG_DATA request_id非零。不匹配增加invalid不交付。可靠帧先reserve，link_receive允许DELIVER才commit；重复/过期或其他拒绝对已预留槽cancel。reserve/commit/cancel、peek_tx/tx_consumed必须成组绑定，启动后不得替换。peek借用到tx_consumed，编码失败也结束借用。当前生产已绑定bridge_state，可靠帧实际预留后接管，遥测进入固定FIFO；回调不允许socket或阻塞网络。池状态主说明见[DMA](../system/resource-pools.md#f01-dma)，计数见[诊断](../system/diagnostics.md#f09-spi)。

### 初始化与停止边界

bridge_spi_start先置started，再建静态信号量及Core1任务，失败不自动重建；portMAX_DELAY仅等待初始化通知，不等待主机SPI流量。spi_task核对实际核号后acquire DMA，驱动配置isr_cpu_id为Core1、ESP_INTR_FLAG_IRAM、max_transfer_sz=1024B。初始化失败且已取得应用DMA时release回IDLE，错误仍保留原初始化结果。初始化成功才进入事务循环；运行失败lost逻辑会话、GPIO14高撤READY、发布service_error与driver_owned后删任务，保留可能被驱动引用内存。不要把任务退出当作driver回收证明。
