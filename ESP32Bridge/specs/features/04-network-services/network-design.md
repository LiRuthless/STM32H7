# Phase 04：网络服务资源与适配细化

2026-10-18，分支`codex/04-network-services`。Phase 03已按阶段审查提交2335b3d；本设计及三件套同步先独立纯规约提交，再实施。SCB/1不变；[G-01](g-01-busy.md)、[G-02](../02-scb-protocol/g-02-session-change.md)按已确认方案落实，不代表三端已同步。本阶段不实现Phase 05的接管/队列/导出，不宣称透明桥接可用。

2026-10-20修订（实机冒烟暴露）：无SPI会话时死链轮询被误并入“重发耗尽关TCP”，导致状态查询连接在HELLO_ACK后立即被关闭、BUSY拒绝无法观察；FR-5段已明确死链不关状态连接，仅为实现符合既定规约的修正，协议不变。

## requirements

### 04/FR-1：热点与启动接入

`startup_idf.c`将`.ap`接为真实`bridge_net_ap_start`：默认AP netif、显式192.168.4.1/24及DHCP、`esp_wifi_init`/AP模式、SSID由`esp_read_mac(ESP_MAC_WIFI_SOFTAP)`末两字节派生四位大写十六进制、WPA2-PSK阈值且不回退开放、密码取启动已校验配置、channel=1、max_connection=4。任一步失败原样返回错误并回收本步已取得资源，不降级、不重启；AP关联数不代替TCP单会话检查。SDK Wi-Fi/lwIP内部动态分配沿用01既有声明，数据路径仍只用固定资源。

### 04/FR-2、FR-3、FR-4：单会话TCP与有界流

`.sockets`接为`bridge_net_start`：创建静态Core0/优先级5网络任务（8192B静态栈及静态TCB，任务栈上独占1492B JSON工作区，与SPI任务同规格复用模式）；任务内建监听socket：bind 5000、listen、非阻塞加有界poll。监听或建任失败只停止网络服务并明确报错，AP/SPI不受影响，不整机复位。新增固定资源：网络任务独占1024B接收组装缓冲、1024B发送图像缓冲（含游标）、256B BUSY拒绝缓冲（含游标）、1016B UDP发送图像缓冲；全部静态，不借DMA池，不改驱动持有缓冲。

- accept：控制槽空则接管并等待HELLO；占用则按G-01立即编码HELLO_ACK/BUSY——头flags/seq/ack_seq/request_id=0、session_id取当时只读SPI快照（未建为0）、JSON含protocol/session_id/真实link_ready/status="BUSY"；独立游标、500ms期限、短写/EAGAIN处理，完成或到期关闭，绝不读写第一连接资源。
- 接收：先32B头并及早校验magic/version/type/payload_len≤992，再收负载，`scb_decode`(TCP、无填充)完整校验；任意拆包/粘包/多帧连续只按完整帧分派；非法帧关闭连接、不部分执行。recv=0对端关闭、EAGAIN等超时、错误即关闭并计数。
- 发送：游标处理短写/EAGAIN/send=0；poll可写等待不超本端下一可靠计时截止，单帧停滞超2000ms判定连接失效并关闭回收；不阻塞SPI。TCP只发头+负载实际字节，无SPI填充。

### 04/FR-5、FR-6、FR-7：订阅、UDP与会话耦合

网络任务独占一个SCB_TCP可靠实例：精确ACK、200ms/最多三次重发、按可靠身份去重；重发耗尽只关闭TCP不触碰SPI。无SPI会话（session_id=0）时该实例为死链：HELLO_ACK如实报告session_id=0/link_ready=false后**状态查询连接保持打开**，不计链路故障、不主动关闭（SCB/1约定电脑此时只查询连接状态）；业务帧因无合法当前会话不被接管而关闭连接。HELLO前收到其他帧即关闭。HELLO经既有codec/JSON校验后，以新增纯函数入口按显式session_id/link_ready编码HELLO_ACK（复用02字段规则，不共享SPI会话对象所有权）。SUBSCRIBE本地处理：合法当前HELLO后连接、request_id非零、udp_port为1～65535整数则记录订阅（目的IP固定为accept时对端地址，不接受帧内地址）并回RESULT OK；合法JSON但端口非法回INVALID_ARG；非法JSON按坏TCP帧关闭。其他可靠COMMAND本阶段无Phase 05消费者：不ACK、计背压，IDLE独立确认照发；不伪造RESULT。UDP仅TELEMETRY、一报文一帧≤1016B、无订阅或断线即丢弃计数；出口为启动前绑定的`bridge_net_port_t`遥测回调（未绑定即全部丢弃计数），发送失败计数不阻塞任务。每轮读`bridge_spi_snapshot`检测session_id变化：按G-02立即置业务许可无效并关闭TCP、清订阅及待发，由电脑重连HELLO获取状态；轮询快照不共享/不锁SPI会话对象。普通断网只回收该连接资源，不重置SPI、不重放设置。

### 04/TC-1、TC-2：边界

业务socket只承载SCB/1帧；诊断仅串口且不打印密码；旧连接延迟工作项不得发往新连接（代际凭据校验）。快照接口报告连接/拒绝、接收/CRC/非法关闭、发送字节/短写/停滞、订阅变更、UDP发送/失败/丢弃、背压及会话关闭计数。

## plan

新增`main/net_bridge.c/.h`及`main/CMakeLists.txt`组件源；AP/任务接入`startup_idf.c`两个回调；`app_main`按实际报告AP/socket状态，不再声明UNIMPLEMENTED占位语义之外的成果。主机侧新增lwip sockets/wifi/netif边界替身、网络任务C夹具、`net_host.py`及`test_network_stream.py`/`test_subscribe.py`：socket夹具可注入accept顺序、recv任意切片/短读/EAGAIN/对端关闭、send短写/0/EAGAIN及poll超时推进注入时钟。实施顺序按既有plan的Group 1→2→3连续推进，阶段末统一交审；实机热点/DHCP/抓包/性能另测，不由主机通过冒充。

## validation

- 04/V-1：主机断言AP配置参数（SSID派生、WPA2阈值、IP/掩码/DHCP、密码非法阻塞AP而不阻塞SPI）；真实热点/DHCP待实机。
- 04/V-2：同槽双连接——第一连接持续服务，第二连接立即收到独立解析的合法HELLO_ACK/BUSY并在期限后关闭；部分发送与超时路径覆盖。
- 04/V-3/V-4：逐字节拆包、粘包、截断、超长宣称、坏CRC/JSON关闭；短写/EAGAIN/send=0/慢读/停滞关闭；可靠计时在socket空闲时仍推进。
- 04/V-5/V-6：SUBSCRIBE端口0/1/65535/65536/负数/浮点/布尔/字符串及帧内第三方地址；UDP仅在订阅后向TCP对端IP发送、≤1016B、断线清理、无订阅丢弃计数。
- 04/V-7：未ready时HELLO_ACK报告真实link_ready且不转交控制；注入SPI会话变化后旧TCP被关闭、旧帧拒绝、重连需新HELLO。
- 04/V-8/V-9：TCP字节流无填充、独立seq/ack/CRC；串口日志无密码；代际凭据拒绝旧连接工作项。
- 真实v6.0.0 build/size、ELF确认网络任务/入口符号、全回归及文档链接检查；实机Wi-Fi/DHCP/抓包/吞吐与三端兼容性仍待验。
