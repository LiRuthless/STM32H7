# SoftAP、TCP 与 UDP

> 核对：2026-11-05 工作区。公共格式见 [SCB/1](../../../SmartCar/Docs/services/communication.md#scb1)，本页说明 ESP32 服务适配。需求见 [traceability](../traceability.md)。

<a id="f05"></a>
## F05 网络接入、单控制会话与订阅

### 状态及配置

已接入生产启动。历史旧镜像启动/AP/DHCP 已有记录，修复后网络完整流程仍待上机复测；不声明实时桥接已完成。

WPA2 SoftAP：SSID SmartCar-XXXX，取 AP MAC 末两字节四位大写十六进制；192.168.4.1/24、DHCP、channel1、max_connection4，密码来自 [F01](../system/startup-and-resources.md#f01)，非法不开放回退。TCP监听5000，只有一个控制连接；AP关联容量不代替控制槽。Core0/优先级5、静态栈8192B、任务独占1492B JSON workspace。RX/TX各1024B、BUSY256B、UDP1016B 静态图像，绝不借驱动 DMA。

### 流程、状态及异常

AP按实际SDK步骤取得资源，失败按已取得步骤回收并返回原错误。网络任务内建监听和非阻塞poll；任务/监听失败只停止网络，不复位、不永久停止 SPI。

首连接 HELLO 前不能接受业务；先组装32B头、及早校验类型/长度，再收至多992B负载并完整解码。拆包/粘包按完整帧分派，坏帧关闭，无部分业务。recv=0为对端关闭，EAGAIN保留进度。发送保留独立游标，处理短写/EAGAIN/send=0；poll不越过可靠截止，单帧停滞2000ms关闭。

按已确认 G-01，控制槽已占时第二连接无需先HELLO，立即获得独立 HELLO_ACK/BUSY：flags/record_count/seq/ack/request_id=0，session和link_ready取当时SPI快照，JSON附status=BUSY。独立游标，500ms完成或到期仅关闭第二连接，不挤掉第一连接。该适配获本端确认，不证明三端已同步。

无SPI会话时session=0/link_ready=false的HELLO_ACK发出后，状态连接保持打开；死链poll不计真实链路故障、不立即关连接。无合法会话的业务仍拒绝并关闭。非零会话可靠重试耗尽才按对应故障关闭；SPI会话变化按G-02关旧TCP，清订阅和待发，不重置SPI。

SUBSCRIBE仅本地处理，要求当前合法会话及握手、非零request_id、udp_port为1～65535整数。目的IP只取accept对端；合法JSON但端口非法回INVALID_ARG，非法JSON关闭；不转发STM32。其他可靠COMMAND当前没有桥接消费者，背压不ACK；IDLE可继续确认。

UDP出口仅发送TELEMETRY，一报文一帧≤1016B；无订阅/断线清理丢弃计数，重连须HELLO并重新订阅，不重放设置。startup启动前绑定bridge_net_socket_telemetry，但SPI→队列→NET的生产供给链未接入；有出口不等于已实时上传。

### 实现与 owner

| 文件 / 符号 | 职责与上下文 |
|---|---|
| [net_bridge.c](../../main/net_bridge.c)、[net_bridge.h](../../main/net_bridge.h)：bridge_net_ap_start/start/configure/snapshot、net_task、dispatch、handle_subscribe、session_check、link_service | 网络任务独占socket、连接/订阅身份、缓冲/游标及TCP可靠实例；快照短临界区 |
| 同模块：bridge_net_telemetry_offer、bridge_net_socket_telemetry | 有界UDP出口与代际检查；socket只由网络上下文使用 |
| [startup_idf.c](../../main/startup_idf.c)：bridge_startup_idf_run | 启动前绑定真实出口并按依赖启动，不在运行期换表 |

### 验证与证据

复用 test_network_stream/test_subscribe，检查逐字节拆包、粘包、错帧、短写、停滞、首连接保持、BUSY仅关第二端、端口边界/类型/第三方IP、换会话和旧代次。零SPI会话连续流程必须区分recv超时与EOF，至少保持2秒，断线重连及无会话业务拒绝后仍能再HELLO。
历史软件/目标：[P04](../validation/phase-04.md)；连续流程222项历史记录及负对照：[开工核对](../validation/phase-05-preflight.md)。本轮未重跑。真实抓包、并发SPI、修复后BUSY/重连仍待验。

## 本轮存量整改追溯

2026-11-05：[Phase01～04格式与注释整改契约](../../specs/research/2026-11-05-source-style-remediation-draft.md)；47文件静态整改与人工复核已完成，目标构建未验证，见[本轮报告](../validation/2026-11-05-source-style-review.md)；现行功能、状态owner和模块映射不变。阶段验收及硬件待验不因本次整改自动完成。
