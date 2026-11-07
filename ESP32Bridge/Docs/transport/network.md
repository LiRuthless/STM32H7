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

UDP出口仅发送TELEMETRY，一报文一帧≤1016B；消费时无订阅或旧身份计丢弃；断线直接清pending，不保证该清理逐包计数，重连须HELLO并重新订阅，不重放设置。startup启动前绑定bridge_net_socket_telemetry，但SPI→队列→NET的生产供给链未接入；有出口不等于已实时上传。

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


<a id="f05-ap"></a>
## AP建立与失败回收

net_bridge.c::bridge_net_ap_start依次创建AP接口→停DHCP→设置本机/网关192.168.4.1、掩码255.255.255.0→开DHCP→wifi_init→AP模式→读AP MAC→写WPA2配置→start。错误保留原SDK返回，若wifi已初始化则deinit，再销毁本接口；不开放回退，不重启设备。来源Phase04 network-design，密码检查的前置责任见[F01配置](../system/startup-and-resources.md#f01-config)。

<a id="f05-tcp"></a>
## TCP部分收发与循环顺序

RX满时不调用零长度recv，避免把返回零误当EOF。rx_consume只在完整帧并consumed后移动剩余字节；SUBSCRIBE待TX图像/可靠窗口空闲可保留完整RX到下一轮。TX正数send才刷新进度，全部字节完成才scb_link_sent启动可靠计时；send=0计short_writes但不刷新，EAGAIN/EWOULDBLOCK保留游标，其他错误关本连接。每轮session_check→可靠/IDLE→poll→BUSY→控制TX/RX→accept→UDP→publish，防止旧连接工作进入新控制槽。非零会话耗尽关闭，零会话状态连接保持；许可检查仍拒绝无会话业务。

<a id="f05-busy"></a>
## BUSY不占用首连接图像

控制槽已占时rejected递增；如已有一个BUSY fd直接关新fd，否则独立编码HELLO_ACK/BUSY，使用256B图像及独立游标。到now≥deadline或发送完成关闭第二连接；非暂时错误也只关闭第二连接。BUSY正数发送计入sent_bytes，不能用该字节量代表首连接业务。

<a id="f05-subscribe"></a>
## 本地订阅与身份

合法COMMAND通过可靠层且TX/窗口可用后handle_subscribe检查非零request_id、args.udp_port精确u32且1～65535；非法参数编码可靠RESULT INVALID_ARG。合法则先写订阅和端口、计subscriptions，再编码RESULT OK；若编码失败关闭并清刚写订阅。目的IP只取accept网络字节序地址，不接受帧内第三方地址。close_control清RX/TX、握手、订阅、IDLE、可靠窗口和UDP pending，但peer_ipv4可保留；新控制连接增加s_generation。SPI会话变化关旧TCP且重连须重新HELLO/订阅，不重置SPI。

<a id="f05-udp"></a>
## UDP交接与当前缺口

bridge_net_telemetry_offer先完整UDP decode，锁内检查pending/in_use。忙则置布尔overflow并返回INVALID_STATE，空则复制完整帧并记录当前generation，不调用socket、不承诺订阅。NET锁内取pending转in_use，锁外核对连接/订阅/代际/出口，成功一次sendto完整长度才udp_sent，失败udp_failed，无资格udp_dropped，完成锁内结束in_use。overflow可合并多次忙拒绝，不是精确每次丢包计数。跨任务generation读写同步风险仍为EB-07，不因这里描述而修复或批准。启动已绑定真实出口，但SPI生产消费者→桥接队列→offer仍未接通，不能宣称完整实时遥测。

<a id="f05-timing"></a>
## 网络参数主表

以下来源net_bridge.h及net_bridge.c，owner为NET任务；编译固定值在AP/start初始化生效，无运行期配置覆盖。连接游标、计时和订阅随close_control清理；累计计数保留见[诊断](../system/diagnostics.md#f09-net)。

| 符号/字段 | 当前值/单位/范围及条件 |
|---|---|
| BRIDGE_NET_CORE/PRIORITY/STACK_BYTES | Core0 / 5 / 静态栈8192B；非实测余量 |
| ap.channel/max_connection | 1 / 最多4个AP关联，不等于控制槽 |
| ap.ssid_len / SSID | 13字节SmartCar-XXXX，AP MAC末两字节大写十六进制 |
| BRIDGE_NET_TCP_PORT/BACKLOG | 5000 / 2；监听排队参数，控制槽始终1 |
| BRIDGE_NET_RX_BYTES/TX_BYTES | 各1024B，接收拼帧/发送游标图像 |
| BRIDGE_NET_BUSY_BYTES/UDP_BYTES | 256B / 1016B，独立BUSY及单UDP图像 |
| BRIDGE_NET_BUSY_LIMIT_MS | 500ms；accept设deadline，now≥截止关闭 |
| BRIDGE_NET_STALL_LIMIT_MS | 2000ms；自图像创建/最后正数send计时，严格大于才关闭 |
| BRIDGE_NET_POLL_LIMIT_MS | 10ms等待上限，按可靠与BUSY最近截止缩短 |
| SCB_RETRY_MS | 200ms，完整发送才起算，主说明见[F03](../protocol/framing-and-links.md#f03) |
| udp_port / peer_ipv4 | uint16 1～65535主机序 / uint32网络序，运行期本地订阅/accept生效 |
| s_generation / s_udp_generation | uint32连接代次，accept推进，供消费时拒绝旧图像；EB-07未决 |

单调时间来自esp_timer_get_time()/1000（毫秒），不是车辆时钟。网络poll不等待SPI时钟，不把2000ms停滞、500ms BUSY或200ms可靠等待混为同一个超时。

### 初始化通知与可选出口失败

bridge_net_start先置started，再创建静态信号量及任务；失败不自动重试。启动者portMAX_DELAY只等网络初始化通知，不等待SPI握手。init_sockets创建TCP，bind固定192.168.4.1:5000→listen(backlog2)→fcntl非阻塞，任一步失败关闭本fd并报ESP_FAIL。TCP发布成功后才尝试UDP socket及非阻塞设置；UDP失败关闭该fd并保留TCP服务，后续实际遥测调用归udp_failed。NET循环若返回，关闭控制/BUSY/监听/UDP，发布service_error后删除任务，不复位。启动成功只证明监听初始化，不证明对端HELLO或桥接供给链完成。
