# SPI 从机与 READY

> 核对：2026-11-05 工作区；需求见 [traceability](../traceability.md)。公共引脚及线格式引用 [SCB/1](../../../SmartCar/Docs/services/communication.md#scb1)。

<a id="f04"></a>
## F04 DMA 事务、READY 与 SPI 协议适配

### 输入输出、配置与状态

本端事务循环及协议适配已实现，真实电气和吞吐待验。SPI2_HOST、自动 DMA、Mode0、8bit、MSB first、全双工；CS10/SCK12/MOSI11/MISO13、READY14 低有效，WP/HD/扩展线 -1。一次事务配置 8192bit，队列深度 1。Core1、优先级12、静态栈6144B；实际核内初始化驱动。驱动 SDK 分配另计，不依赖 PSRAM。

### 调用与所有权

应用准备 TX、清 RX/描述 → pool submit → queue_trans(0) → post_setup 拉低 READY → post_trans 拉高 → get_trans_result 成功且返回原描述 → reclaim → 首先核对 trans_len → 处理 RX/准备下一帧。两回调 IRAM_ATTR，只访问 GPIO，不解析、不日志、不加等待锁；回收后任务上下文才做有界 codec/JSON，1492B 工作区由 SPI 任务独占。

get 每次有界等待约10ms（至少1 tick）；无时钟时继续阻塞等待，并轮询逻辑重试/统计，不接触驱动持有的描述或 DMA。定时器失效只撤销逻辑状态，不等于回收。重发待办须回收后才能复制入 TX；等待中精确 ACK 可撤销待办。

完整物理事务先确认上一 TX 完整发送，再处理 RX；坏 RX CRC 不抹掉真实 TX 完成事实。截短不确认 TX 完成，后续继续原可靠图像。合法 HELLO 的回复位于后续事务；随机数最多尝试4次，不合格就等待对端重发，不伪造握手成功。入站只接受既定方向；可靠业务调用 reserve/commit/cancel，回调借用视图必须立即复制，不得保留 RX 指针。telemetry/peek_tx/tx_consumed/session_changed 均为有界无 socket 的启动绑定端口。

### 异常与兼容

排队失败若证明未接受本描述，可撤回所有权并停止该服务。get 错误或意外描述不能证明回收：保持固定内存及驱动所有权有效，撤销 READY、停止服务，不销毁池或复位。可见截短/超长、坏帧不 ACK，回收后重排；硬件可能将超长 trans_len 饱和到8192，不能用该值证明没有额外时钟。

当前未绑定 Phase05 生产桥接消费者：可靠业务无容量不 ACK，遥测丢弃计数；协议夹具的接管成功不代表生产桥接完成。

### 实现位置

| 文件 / 关键符号 | owner 与上下文 |
|---|---|
| [spi_bridge.c](../../main/spi_bridge.c)、[spi_bridge.h](../../main/spi_bridge.h)：bridge_spi_start/configure/snapshot、spi_task、run_transactions、ready_loaded、ready_finished | SPI 任务独占 DMA/描述/协议对象；快照短临界区发布；回调仅 IRAM GPIO |
| [spi_protocol.c](../../main/spi_protocol.c)、[spi_protocol.h](../../main/spi_protocol.h)：bridge_spi_protocol_init/poll/lost/sent/receive/prepare、bridge_spi_port_valid | 同一 SPI owner；poll 不接收 DMA 指针，sent/receive/prepare 仅在回收路径 |

### 验证方法与限制

复用 test_spi_driver/test_spi_lifecycle：0/8191/8192/可见8193bit、坏帧、排队失败、get错误/错描述、无时钟定时及回调顺序。判据是持有期间描述与 TX/RX 不被应用改变，无虚假 ACK/READY。IRAM 段和调用链须对对应目标 ELF 核验；上板以逻辑分析仪测 CS/SCK/READY 和实际1024B，约1.875MHz到约7.5MHz按真实主机时钟核算。历史：[P03](../validation/phase-03.md)，G-06 决定见[阶段记录](../../specs/features/03-spi-slave/g-06-task-json.md)。

## 本轮存量整改追溯

2026-11-05：[Phase01～04格式与注释整改契约](../../specs/research/2026-11-05-source-style-remediation-draft.md)；47文件静态整改与人工复核已完成，目标构建未验证，见[本轮报告](../validation/2026-11-05-source-style-review.md)；现行功能、状态owner和模块映射不变。阶段验收及硬件待验不因本次整改自动完成。
