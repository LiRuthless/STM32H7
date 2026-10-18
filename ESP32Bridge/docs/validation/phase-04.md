# Phase 04：网络服务软件交付与阶段审查

2026-10-18，Asia/Shanghai，分支`codex/04-network-services`。设计增量已先纯规约提交`8b91e1b`；本批实现按[network-design](../../specs/features/04-network-services/network-design.md)完成并接入生产启动。**网络服务软件已实现，真实Wi-Fi/实机验收未执行，Phase 04总完成不勾选。** 本批代码/证据待本次阶段审查提交，不烧录、不push。

## 生产功能及调用路径

真实调用路径为`app_main → bridge_startup_idf_run → bridge_net_ap_start / bridge_net_start → Core0静态网络任务 → 非阻塞socket/poll → 生产codec/reliable_link/link_session`，UDP出口为启动前绑定的任务自有非阻塞socket。不靠未调用模块声明完成。

| 功能 | 生产符号 / 文件 | 已实现行为 |
|---|---|---|
| WPA2 SoftAP | [net_bridge.c](../../main/net_bridge.c)`bridge_net_ap_start` | 默认AP netif、显式192.168.4.1/24及DHCP、WIFI_INIT_CONFIG_DEFAULT、AP模式、SSID=`SmartCar-`+`esp_read_mac(ESP_MAC_WIFI_SOFTAP)`末两字节四位大写十六进制、WPA2-PSK不回退开放、密码取启动已校验的`CONFIG_BRIDGE_AP_PASSWORD`、channel=1、max_connection=4；任一步失败原样返回并按取得顺序回收（deinit→destroy），不降级不重启 |
| 网络任务与监听 | `bridge_net_start`、`net_task`、`init_sockets` | Core0/优先级5/8192B静态栈+静态TCB（照spi_bridge模式）；bind 5000、listen backlog 2、非阻塞+单poll有界等待≤10ms；监听建立失败只停网络服务并记`init_error`，AP/SPI不受影响，任务删除自身，不整机复位 |
| 单控制会话与BUSY | `accept_service`、`busy_service` | 空槽接管并等HELLO；占用时按G-01立即以生产codec编码HELLO_ACK/BUSY（头flags/seq/ack_seq/request_id=0、session_id/link_ready取当时SPI快照，JSON含status="BUSY"），256B独立缓冲+游标+500ms期限，短写/EAGAIN处理，完成或到期关闭；第三连接在BUSY待发时立即关闭；第一连接资源绝不被读写 |
| 有界TCP流 | `rx_recv`、`rx_consume`、`dispatch`、`tx_service` | 1024B静态组装缓冲，32B头及早校验magic/version/type/payload_len≤992，再收负载并`scb_decode`(SCB_TCP)完整校验；任意拆包/粘包/多帧只按完整帧分派；非法帧关闭不部分执行。1024B静态TX图像+游标处理短写/EAGAIN/send=0；poll可写等待不越过本端下一可靠计时截止；单帧停滞>2000ms关闭回收 |
| 可靠与会话 | 任务独占`scb_link_t`(SCB_TCP)、`link_service`、`idle_service`、`scb_session_tcp_hello_explicit`、`scb_session_tcp_busy` | 精确ACK、200ms/最多3次原图像重发、按可靠身份去重（重复仅IDLE独立确认）；耗尽只关闭本TCP。HELLO前任何帧即关闭；HELLO_ACK报告真实session_id/link_ready；link_session新增纯函数入口不共享SPI会话对象所有权 |
| SUBSCRIBE与UDP | `handle_subscribe`、`bridge_net_telemetry_offer`、`udp_service`、`bridge_net_socket_telemetry` | 须已HELLO、request_id非零、udp_port为1～65535整数，合法回RESULT OK（沿用request_id、可靠），非法回INVALID_ARG且不断连，非法JSON按坏帧关闭；目的IP固定为accept时对端地址，帧内地址不采用。UDP仅TELEMETRY、一报文一帧≤1016B；offer跨上下文经临界区交接静态1016B图像（占线即ESP_ERR_INVALID_STATE）；未绑定/无订阅/断线/代际不符即丢弃计数，发送失败计数不阻塞任务 |
| 会话联动（G-02） | `session_check` | 每轮读`bridge_spi_snapshot`；session_id变化立即关闭TCP、清订阅与待发，不重置SPI、不重放；旧连接延迟工作项由代际凭据拒绝发往新连接 |
| 可观测状态 | `bridge_net_snapshot`、app_main `NET`行 | 只读临界区快照：连接/拒绝/接收/CRC/非法关闭/发送字节/短写/停滞/订阅/UDP发送·失败·丢弃/背压/会话关闭/link_failures/栈余量/listening/open/subscribed/对端IP/订阅端口；不打印密码 |

生产当前仍无Phase 05桥接消费者：除SUBSCRIBE外的可靠COMMAND**不ACK、计背压**，IDLE独立确认照发；不伪造RESULT/APPLIED/SAVED。SPI端遥测端口留待Phase 05绑定，SPI服务与电脑/热点完全独立继续运行。

## 本次实际命令与结果

仓库根目录（经激活入口，去除MSYSTEM注入；详见下文偏差4）：

```powershell
. .\ESP32Bridge\scripts\activate.ps1
python -m unittest discover -s ESP32Bridge\tests
```

最终全量**220项通过，0失败/0跳过**，日志`.local/p4-regression.log`。新增网络流27项（[test_network_stream.py](../../tests/test_network_stream.py)）与订阅/UDP15项（[test_subscribe.py](../../tests/test_subscribe.py)）；既有178项全部保持通过。主机以现有Clang/LLD直接编译verbatim生产C（`net_harness.c`内含`main/net_bridge.c`及02协议/会话/可靠模块），socket/Wi-Fi/netif/RTOS/时钟边界全部为显式脚本夹具（[net_harness.c](../../tests/host/net_harness.c)、[net_host.py](../../tests/net_host.py)）；Python只提供脚本输入并以独立struct/zlib/json解码核验，没有另写一套网络实现。结果不是RF、DHCP、RTOS或真实网络证据。

覆盖：AP参数/顺序/回滚10场景、监听建立5类失败只停网络服务、HELLO_ACK精确字节与真实会话值、逐字节拆包/粘包/截断/993B超长宣称/坏magic·type·CRC·JSON、HELLO前关闭、BUSY完整帧/短写/500ms期限/第三连接立即关闭、短写游标/EAGAIN/send=0/收发错误/2000ms停滞、3次原图像重发后耗尽只关TCP、精确ACK不重发、重复SUBSCRIBE仅IDLE确认、其他命令真实背压、会话变化关闭+重连新HELLO、SUBSCRIBE端口0/1/65535/65536/负/浮点/布尔/字符串/缺args/非对象/request_id=0、帧内第三方地址不采用、订阅后UDP目的=TCP对端且逐字节相等、1016B边界、一报文一帧、断线/重连未订阅/UNBIND/发送失败分别丢弃或失败计数、offer非法帧与占线拒绝、日志无密码、代际凭据拒绝旧工作项。

目标通过正式入口，对当前源码白名单隔离副本（`.local/p4-target`，一次性私有测试密码）实际执行：

```powershell
& .\ESP32Bridge\.local\p4-target\scripts\build.ps1 `
  -WorkRoot 'D:\STM32H7\ESP32Bridge\.local\p4w' -KeepWork
```

**v6.0.0/esp32s3 build与size均退出0**，run_id=`a5d696a4aef14cb49959dfa8b1bf57e3`。SDK commit=`662a3be354759d9487bf4b1a629fadb766cb1800`、dirty=false；Python3.11.15、CMake4.0.3、Ninja1.12.1、Xtensa`esp-15.2.0_20251204`。BIN792208B、image792097B、DIRAM124943B、.data20944B、.bss39056B；较Phase03（image304769B）的增长来自Wi-Fi/lwIP链接，是构建静态数据，不能当成运行堆测量。

最终ELF经xtensa nm核对：含`bridge_net_ap_start`、`bridge_net_start`、`net_task`、`bridge_net_snapshot`、`bridge_net_configure`、`bridge_net_socket_telemetry`、`scb_session_tcp_hello_explicit`、`scb_session_tcp_busy`；静态资源核对网络栈8192B、RX/TX各1024B、BUSY 256B、UDP图像1016B（`.local/p4-final-elf-symbols.txt`）。`bridge_net_telemetry_offer`是Phase05才能接线的入站API，生产暂无调用者，被`--gc-sections`从ELF移除；源码与主机测试均在，见偏差2。

[最终证据JSON/文件清单/产物SHA256](phase-04-evidence.json)由`.local/collect-p4-evidence.py`生成，源码与产物哈希重新核对。产物本体、构建工作区和密码均被忽略、不提交。构建元数据的Flash配置不等于实板容量已核实，本次没有烧录。

## 偏差与限制

1. **GCC专属告警**：`run_service`无限循环在主机Clang下无需返回语句，目标GCC`-Werror=return-type`不接受；循环后补了不可达`return ESP_OK`。不改行为。
2. **入站API无生产调用者**：`bridge_net_telemetry_offer`按设计是Phase05 SPI侧绑定的入口；本阶段只交付并主机验证该入口，SPI遥测端口保持未绑定（离线遥测仍由SPI侧计数丢弃）。不把它接入启动路径冒充已桥接。
3. **AP认证字段**：v6的`wifi_ap_config_t`无threshold字段，直接设`authmode = WIFI_AUTH_WPA2_PSK`，强于阈值语义且不回退开放；与规约"WPA2-PSK阈值"一致，不新增协议。
4. **回归入口**：从Git Bash直接跑`python`会因MSYSTEM注入/SDK依赖检查失败及PATH缺PowerShell而出现8项环境门卫失败（未改动的基线同样如此，非本批引入）；必须用`scripts/activate.ps1`激活的干净PowerShell（进程内去除MSYSTEM）运行，结果为220项全通过。
5. **主机测试暴露并已修复的实现缺陷**：延迟处理的SUBSCRIBE曾每轮重复计入received；自身RESULT未ACK期间重复可靠帧未获IDLE确认。修复为"先以当前预留能力询问可靠层、仅新鲜SUBSCRIBE才缓冲等待"，已由去重/粘包/计数用例锁定。
6. `prepare-current-build.py`名称模式放宽为同时接受`p4-target`（本阶段无Group分组），功能不变。

## 实机待验

1. 真实热点：SSID后缀与实际MAC、WPA2握手、192.168.4.1/24及DHCP租约、密码非法不开放；关联容量≠TCP会话检查。
2. 真实双客户端BUSY、断线/重连/重订阅、UDP抓包核对目的与帧边界、慢TCP下SPI事务持续推进且整机不复位。
3. 三端SCB/1兼容（PC工具与真实STM32）及G-04候选向量三端核对，均不受本阶段主机结果替代。
4. 运行期栈高水位、内部堆与Wi-Fi并发资源实测；构建静态数据不代表运行余量。

## 本次阶段审查清单

- [ ] 生产C（net_bridge.c/.h、link_session新增入口、startup_idf/app_main/CMake接线）逐行审查。
- [ ] 主机夹具边界与脚本语义（net_harness.c/net_host.py）审查，确认无Python冒充逻辑。
- [ ] 220项回归日志、目标构建产物清单/哈希、ELF符号与静态资源核对抽查。
- [ ] 偏差1～6认可；确认SPI遥测端口留待Phase05。
- [ ] 审查通过后提交（建议信息见证据JSON`proposed_commit`），仍不push、不烧录。
