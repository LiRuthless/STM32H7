# Phase 04：网络服务 — 验证

2026-10-20开工增量：04/FR-2、FR-5、FR-7增加一个连续主机流程，运行生产net_task与codec/session/reliable：SPI会话=0/ready=false，HELLO_ACK发出后保持至少2秒，再连接第二客户端并精确核对BUSY及其EOF；主动断开第一连接后新连接HELLO可保持，发送无会话可靠业务必须拒绝关闭，再HELLO仍可用。主机只提供socket/时钟边界，独立struct/zlib验证输出。对应tests/test_network_stream.py；执行前不记通过，实机同流程仍待验。

执行回填：新增连续流程通过，全量222项通过、0跳过；隔离撤回00331c9保护后两项相关用例按预期失败，生产源不变。证据见[开工核对](../../../docs/validation/phase-05-preflight.md)及其JSON索引。实机四项复测仍待验。

> 2026-10-18：主机/目标软件检查已实际执行并回填；实机Wi-Fi/DHCP/抓包/并发检查保持未执行，不标通过。证据见[阶段交审](../../../docs/validation/phase-04.md)与[证据索引](../../../docs/validation/phase-04-evidence.json)。

## 验证清单

2026-10-18：V-2按已确认的[g-01-busy](g-01-busy.md)验证。

2026-10-18：主机/socket夹具及入口细化见 [network-design](network-design.md) 的validation节；实机Wi-Fi/DHCP/抓包仍待验。

| 编号 | 检查项 / 方法 | 可观测通过标准 | 关联需求 | 状态 |
|---|---|---|---|---|
| V-1 | 启动配置、热点连接、DHCP与MAC派生核对 | WPA2、SSID四位大写后缀、192.168.4.1/24及DHCP正确；密码非法不开放；网络任务优先级5 | FR-1 | 主机10场景通过（参数/顺序/回滚/无密码日志，test_network_stream）；真实热点/DHCP/MAC实机未执行 |
| V-2 | 同IP双连接及不同电脑并发连接 | 首控制连接持续服务；第二按G-01规定BUSY帧后关闭；不将AP关联限制当成此用例通过 | FR-2 | 主机通过（完整/短写/500ms期限/第三连接立即关闭，首连接不受影响）；实机并发未执行 |
| V-3 | 每字节拆包、多帧粘包、头/负载截断及宣称超长输入 | 合法帧恰好分派一次；无无限缓存；坏帧关闭且不部分执行 | FR-3 | 主机通过（逐字节/粘包/截断/993B宣称/坏magic·type·CRC/JSON）；实机未执行 |
| V-4 | 短写、EAGAIN/中断、send=0、慢读、断线与端口占用 | 游标无重复/漏字节；有界等待、可靠定时仍服务；故障回收且不复位ESP32；SPI持续推进 | FR-4 | 主机通过（短写游标/EAGAIN/send=0/收发错误/2000ms停滞/监听失败只停网络服务）；SPI并发与实机未执行 |
| V-5 | 0/1/65535/65536、负数、浮点、布尔、字符串端口；额外第三方IP目标 | 仅1～65535整数成立；结果op/request_id对应；UDP目的IP始终为TCP对端，不转发SUBSCRIBE至SPI | FR-5 | 主机通过（test_subscribe端口边界/类型/request_id=0/帧内地址不采用）；实机未执行 |
| V-6 | 未订阅、订阅、断线及重连；检查数据报 | 每报文仅完整TELEMETRY≤1016B；断线取消且不积压；重连重订阅，不重放旧设置 | FR-6 | 主机通过（订阅后逐字节相等/1016B边界/一报文一帧/断线·重连未订阅·UNBIND·失败归因计数）；实机未执行 |
| V-7 | SPI未ready、TCP握手、SPI换会话、旧帧输入 | 报告真实状态且不重置SPI；无会话状态查询连接保持打开、业务帧拒绝；旧订阅/业务清理，关闭旧TCP后重连HELLO/HELLO_ACK获取当前状态 | FR-7 | 主机通过（link_ready真实值/零会话连接send后推进循环仍打开·死链不计故障/无会话业务帧拒绝/会话变化关TCP/旧帧HELLO前关闭/重连新HELLO）；实机HELLO/HELLO_ACK已核，其余未执行 |
| V-8 | TCP/UDP抓包与缓冲所有权断言 | TCP无填充，独立seq/ack/CRC；固定容量，不写驱动持有DMA | TC-1 | 主机独立解码/CRC通过，ELF静态缓冲8192/1024/1024/256/1016B核对通过；真实抓包与DMA行为实机未执行 |
| V-9 | 日志输出、旧连接工作项延迟至新连接后处理 | socket仅SCB/1，串口不含密码；旧工作项不能发给新连接 | TC-2 | 主机通过（日志无密码断言/代际凭据拒绝旧工作项，net_harness）；实机未执行 |

## 自动化验证（已实现）

`tests/test_network_stream.py`（28项）与`tests/test_subscribe.py`（15项）经`tests/net_host.py`编译并驱动 verbatim 生产C（`tests/host/net_harness.c` 内含 `main/net_bridge.c`）；socket/Wi-Fi/netif/时钟边界为显式脚本夹具，可注入accept顺序、recv任意切片/短读/EAGAIN/对端关闭、send短写/0/EAGAIN及poll超时推进。Python只提供输入并独立struct/zlib/json核验，不另写网络逻辑。2026-10-20实机冒烟发现零会话死链误关状态连接后，测试已补send后推进循环断言；全量回归221项通过、0跳过；目标v6.0.0 build/size退出0。目标侧真实热点/DHCP/Wi-Fi性能必须另测。

## 人工验证与回归

1. 配置本地密码后连接热点，记录MAC、IP、DHCP租约和实际固件版本，不保存密码。
2. 保持第一控制会话，发起第二连接；抓取BUSY与关闭证据。
3. 断开/重连并改变UDP端口，检查旧订阅失效和无第三方目的地址。
4. 在慢TCP条件下同时观察SPI事务计数增长，确认没有整机复位。

当前无网络抓包、端口测试或硬件结果；以上实机步骤均未执行，相关检查不标通过。
