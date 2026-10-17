# 任务书覆盖与双向追溯矩阵

> 下表以已提交文档为覆盖基线，当前Phase 01 Group 1的实际定位与验证另列；其余仍是需求/计划映射。公共职责中属于STM32/电脑的部分只作为兼容边界留档。

最新状态：Phase 01 Group 1已实施待审查，实际定位如下；其余阶段仍为计划。用户最新授权为 D-08，保留分组提交及协议门禁。

| 映射 | 实际文件 / 符号 | 当前验证 |
|---|---|---|
| M-01、M-02、M-05、M-47 / 01/FR-1、FR-3、TC-1、TC-3 | [CMake入口](../CMakeLists.txt)、[显式依赖](../main/CMakeLists.txt)、[默认配置](../sdkconfig.defaults)、[Kconfig](../main/Kconfig.projbuild)、[忽略规则](../.gitignore) | [Group 1证据](../docs/validation/phase-01-group-1.md)；01/V-1、V-3、V-7、V-9按分项记录，非阶段通过 |
| M-02、M-05 | [check_config.py](../scripts/check_config.py)：`read_password`、`read_idf_version`、`check_environment`、`check_generated`；[配置说明](../config/local-config.example.md) | [配置测试](../tests/test_config.py)、[真实CMake拒绝测试](../tests/test_cmake_guards.py) |
| M-31 / 01/FR-2 | [app_main](../main/app_main.c) 当前仅配置自检与未实现状态报告 | 不算启动实现；V-2仍未执行，待Group 2 |

## 读法与阶段入口

`02/FR-1`表示阶段02的FR-1，编号在阶段内唯一。每阶段plan的“计划模块与追溯”表将每条FR/TC对应到未来模块和V项，validation列出方法/唯一标准/实际状态。实现后须将“计划位置”更新为实际符号并链接证据，不能用本表代替实现检查。

| 阶段 | 需求 | 计划 | 验证 |
|---|---|---|---|
| 01 工程基础 | [requirements](features/01-project-foundation/requirements.md) | [plan](features/01-project-foundation/plan.md) | [validation](features/01-project-foundation/validation.md) |
| 02 SCB/1协议 | [requirements](features/02-scb-protocol/requirements.md) | [plan](features/02-scb-protocol/plan.md) | [validation](features/02-scb-protocol/validation.md) |
| 03 SPI从机 | [requirements](features/03-spi-slave/requirements.md) | [plan](features/03-spi-slave/plan.md) | [validation](features/03-spi-slave/validation.md) |
| 04 网络服务 | [requirements](features/04-network-services/requirements.md) | [plan](features/04-network-services/plan.md) | [validation](features/04-network-services/validation.md) |
| 05 桥接与故障 | [requirements](features/05-bridge-lifecycle/requirements.md) | [plan](features/05-bridge-lifecycle/plan.md) | [validation](features/05-bridge-lifecycle/validation.md) |
| 06 验证与交付 | [requirements](features/06-validation-delivery/requirements.md) | [plan](features/06-validation-delivery/plan.md) | [validation](features/06-validation-delivery/validation.md) |

来源：[ESP32任务书原文](references/02-esp32-agent.md)、[SDD规范原文](references/sdd-document-system.md)。公共协议全文另存 [scb-v1](protocol/scb-v1.md)，未修改其字段与规则。

## 任务书 → 需求 → 计划模块 → 验证

| 编号 | 来源位置 / 要求 | 对应需求 | 计划模块/责任边界 | 验证项 |
|---|---|---|---|---|
| M-01 | 授权边界：独立工程、仅本端、不改其他目录 | 01/FR-1、TC-3；06/TC-2 | 工程入口、忽略规则；当前仅Phase 01获准组 | 01/V-1、V-9；06/V-9 |
| M-02 | SDK固定、目标esp32s3、缺工具说明 | 01/TC-1；06/FR-2 | build入口；用户已将5.5.1替为6.0.0 | 01/V-7；06/V-2 |
| M-03 | 无空格临时构建、不移动仓库、回收产物 | 01/FR-5、FR-6 | scripts/build.ps1（计划）、中文说明 | 01/V-5、V-6 |
| M-04 | 协议1：3.3V共地、引脚、Mode0、1024B、低频到高频 | 03/FR-1、FR-4、TC-3 | SPI驱动和真实波形；供电/接线先核实 | 03/V-1、V-4、V-9 |
| M-05 | 协议1/A：WPA2、SSID后缀、固定IP/DHCP、密码 | 01/FR-3；04/FR-1 | 本地配置/AP初始化 | 01/V-3；04/V-1 |
| M-06 | 协议1：TCP5000唯一控制连接，第二端BUSY | 04/FR-2 | TCP唯一连接槽；G-01门禁 | 04/V-2 |
| M-07 | 协议1/C：TCP拆包粘包、部分发送、有界缓存 | 04/FR-3、FR-4、TC-1 | TCP流状态机/发送游标 | 04/V-3、V-4、V-8 |
| M-08 | 协议1/4/C：SUBSCRIBE、整数端口、对端IP、UDP≤1016 | 04/FR-5、FR-6 | 本地订阅/UDP | 04/V-5、V-6 |
| M-09 | 协议2：32B头、字段/端序/CRC/禁止裸结构体 | 02/FR-1、TC-1 | protocol头编码与独立参考 | 02/V-1、V-8 |
| M-10 | 协议2：UTF-8、重复键、非有限数、超长不截断 | 02/FR-3、TC-2 | json_guard、接管边界 | 02/V-3、V-9 |
| M-11 | 协议3：8种消息及RELIABLE、IDLE空载 | 02/FR-2 | 类型/负载验证 | 02/V-2 |
| M-12 | 协议3：SPI HELLO/session/重复握手/新IDLE/重启 | 02/FR-5 | link_session SPI状态机 | 02/V-5 |
| M-13 | 协议3：TCP HELLO不重置SPI、link_ready、旧会话 | 02/FR-6；04/FR-7 | TCP会话；G-02通知门禁 | 02/V-6；04/V-7 |
| M-14 | 协议3：双侧独立seq/ack、UDP独立、重封装 | 02/FR-4；05/FR-1 | reliable_link、bridge_state | 02/V-4；05/V-1 |
| M-15 | 协议3：窗口1、精确ACK、IDLE独立ACK、可靠身份去重 | 02/FR-4；05/TC-1 | 每方向状态、固定额度 | 02/V-4；05/V-10 |
| M-16 | 协议3：200ms、最多三重发、耗尽恢复/近回绕 | 02/FR-4、TC-2；05/TC-3 | 可注入时钟及重建动作 | 02/V-4、V-9；05/V-12 |
| M-17 | 协议3：坏SPI不ACK、坏TCP关闭、坏UDP丢计数 | 02/TC-2；03/FR-4 | 完整验证后接管 | 02/V-9；03/V-4 |
| M-18 | 协议3/4：32请求缓存、同ID同/异内容、查询UNKNOWN | 02/TC-3；05/FR-2、TC-2 | STM32/电脑职责；本端保request_id和结果、查询通路 | 02/V-10；05/V-2、V-11 |
| M-19 | 协议4：全部COMMAND/RESULT、8参数/limits、覆盖/保存语义 | 05/FR-2、TC-2 | 协议表原文留档；本端不解释/执行车辆参数 | 05/V-2、V-11 |
| M-20 | 协议4：RECEIVED≠APPLIED、APPLIED字段、SAVED独立 | 02/TC-3；05/FR-2 | 透明结果/事件转交，不升级业务状态 | 02/V-10；05/V-2 |
| M-21 | 协议5：24+N×96、record_count、每项偏移/reserved | 02/FR-2、TC-1 | protocol断言和格式验证 | 02/V-2、V-8 |
| M-22 | 协议5：session/run/索引、tick、revision、跨run延迟 | 05/FR-3、TC-2 | 保留元数据、不跨run重标；采样/封包属STM32 | 05/V-3、V-11 |
| M-23 | 协议5/C：ESP32丢弃数、更新字段并重算CRC | 05/FR-3 | 16包遥测队列及UDP封包 | 05/V-3 |
| M-24 | 协议6：LOG_DATA offset/length/reserved、960B块 | 02/FR-2；05/FR-6 | 格式验证和导出窗口 | 02/V-2；05/V-6 |
| M-25 | 协议6：256B+96N、缓存头GET_LOG_META、不扫Flash | 05/FR-6、TC-2 | 原样转发；头/记录有效性由STM32提供 | 05/V-6、V-11；06/V-5 |
| M-26 | 协议6：导出窗口1、顺序、重复块/整体CRC/LOG_END | 05/FR-6 | 单块转交；STM32 CRC、独立测试对照 | 05/V-6；06/V-5 |
| M-27 | 协议6：10秒、断线/换会话取消、起跑/清除互斥 | 05/FR-7、TC-3 | 本端计时清理；STM32自身超时/互斥 | 05/V-7、V-12 |
| M-28 | 协议6：事件字段、枚举、EVENT_GAP、未知历史 | 05/FR-4 | 32事件队列；G-03门禁 | 05/V-4 |
| M-29 | 协议6：Flash优先/约87.3s/不新增停车 | 05/FR-9、TC-2 | Flash/停车属STM32；87.3秒为任务假设非当前事实 | 05/V-9、V-11 |
| M-30 | 协议7：全部固定向量类别、独立交叉核验、三端一致 | 02/FR-7；06/FR-1 | 向量基线/独立参考；G-04 | 02/V-7；06/V-1 |
| M-31 | A：NVS→网络→AP→socket→SPI | 01/FR-2 | app_main启动协调/错误状态 | 01/V-2 |
| M-32 | A：固定DMA池、不依赖外RAM、启动分配检查 | 01/FR-4、TC-2；03/TC-1 | buffer_pool/驱动所有权 | 01/V-4、V-8；03/V-7 |
| M-33 | B：Core1/优先级12/深度1/同核初始化、网络优先级5 | 03/FR-2；04/FR-1 | SPI和网络任务配置 | 03/V-2；04/V-1 |
| M-34 | B：READY回调/IRAM/先回收再重排/后续事务结果 | 03/FR-3、FR-5、TC-2 | SPI回调与应用循环 | 03/V-3、V-5、V-8 |
| M-35 | B：trans_len、CRC/截短恢复、无时钟不忙等 | 03/FR-2、FR-4、TC-3 | 回收校验及逻辑分析仪 | 03/V-2、V-4、V-9 |
| M-36 | B/C：SPI不等socket/电脑、离线丢实时 | 03/FR-5、FR-6；05/FR-3 | 固定队列，独立SPI | 03/V-5、V-6；05/V-3 |
| M-37 | C：可靠额度不足不先ACK、调参一在途/查询通路 | 05/FR-1、FR-2、TC-1 | 接管与最终结果预留 | 05/V-1、V-2、V-10 |
| M-38 | C/D：断线取消/保已生效值、重连不重放/新会话 | 05/FR-5、FR-7、FR-9 | 生命周期/导出/故障隔离 | 05/V-5、V-7、V-9 |
| M-39 | D：计数/CRC/截短/重试/峰值/丢弃/session、串口诊断 | 05/FR-8；04/TC-2 | bridge_stats和本地诊断 | 05/V-8；04/V-9 |
| M-40 | D：初始化错误限定服务/普通socket不整机复位 | 01/FR-2；04/FR-4；05/FR-9 | 服务错误路径 | 01/V-2；04/V-4；05/V-9 |
| M-41 | 验收1：向量、可靠性、背压、会话、ACK分离 | 06/FR-1 | 01～05回归/独立参考 | 06/V-1 |
| M-42 | 验收2：实际构建/版本/静态和堆内存/配置 | 06/FR-2 | build、map、启动报告 | 06/V-2 |
| M-43 | 验收3：GPIO/Mode0/READY/CS/1024B/截短 | 06/FR-3 | SPI波形与门禁证据 | 06/V-3 |
| M-44 | 验收4：连续1kHz/96kB每秒、拥堵不阻SPI | 06/FR-4 | 60秒流量/故障发生器 | 06/V-4 |
| M-45 | 验收5/6：多端拒绝/UDP/重订阅/导出/透传/CRC | 06/FR-5 | 模拟与真实对照测试 | 06/V-5 |
| M-46 | 验收7：工程/头/向量/配置/脚本/中文说明、只commit不push | 06/FR-6、FR-7 | 后续交付清单/审查；本次按D-07仅提交文档 | 06/V-6、V-7 |
| M-47 | SDD：宪章/三件套/编号/顺序/逐批人工审查 | 01/TC-3；06/FR-7 | AGENTS、宪章、全部六阶段规约 | 01/V-9；06/V-7 |
| M-48 | 用户本轮：仅Markdown、不实现/安装/烧录；D-07另行授权合并及新分支提交 | 06/TC-2 | 本轮文件清单与Git状态；不push | 06/V-9；静态结果单列 |

## 验收状态与门禁索引

Phase 01 Group 1的实现及实际验证见上表，阶段验收仍未完成；其余阶段没有实现/运行证据，计划文件仍为后续目标。对端业务只作兼容性要求，不声称ESP32已实现STM32请求缓存、参数生效或Flash互斥。

G-01（第二TCP BUSY）、G-02（会话通知）、G-03（桥接缺口时序）、G-04（共同向量）、G-05（STM32布局）见 [基线记录](research/2026-10-17-baseline-and-decisions.md)。对应V项已明确保留门禁，不将未定义部分写成已确认接口。

2026-10-17用户已确认D-06：G-01～G-03保留为实施前必须确认项，本轮保持SCB/1原文。此决定不解除M-06、M-13、M-28及其他关联用例的具体线格式门禁，不改变其未执行状态。

用户D-07授权先合并最新分支再提交文档；基线已更新至`e3a4410`。G-05的32B尺寸不匹配不再存在，完整布局/对端SCB/1能力与实机证据仍待验，不能因此标记关联功能测试通过。
