# ESP32Bridge

ESP32-S3-DevKitM-1 的 SPI 从机与 Wi-Fi 桥接工程。面向智能车开发者、联调人员和后续维护者，负责 SCB/1 传输，不承担车辆控制决策。

## 当前状态

2026-10-18，当前分支`codex/04-network-services`。Phase 04网络服务软件已实现并接入生产启动：真实WPA2 SoftAP、单控制TCP:5000及G-01立即BUSY、有界TCP收发与独立可靠实例、本地SUBSCRIBE、订阅UDP遥测出口及G-02会话联动、状态快照；**本次220项全回归、v6.0.0目标build/size通过，待本次阶段审查提交**。见[Phase 04统一交审](docs/validation/phase-04.md)及[最终产物索引/哈希](docs/validation/phase-04-evidence.json)。Phase 05透明桥接/导出与Phase 06工具/最终中文实机手册未完成；生产尚无桥接消费者，除SUBSCRIBE外的可靠业务背压不ACK，SPI遥测端口留待Phase 05绑定。14条[本端候选向量](tests/vectors/scb-v1.md)三端待核对，全部硬件实测（含真实热点/DHCP/抓包）待验；未烧录。按用户最新要求，阶段内部连续推进，仅Phase末审查一次。

| 文档 | 用途 |
|---|---|
| [AGENTS.md](AGENTS.md) | 阅读顺序、授权与工作规则 |
| [CHANGELOG.md](CHANGELOG.md) | 文档及后续功能变更记录 |
| [mission.md](specs/mission.md) | 使命、受众、职责与成功标准 |
| [techstack.md](specs/techstack.md) | ESP-IDF v6.0.0、硬件和技术约束 |
| [roadmap.md](specs/roadmap.md) | 六阶段顺序、状态与依赖 |
| [traceability.md](specs/traceability.md) | 任务书到需求、计划模块和验证项的覆盖 |
| [SCB/1](specs/protocol/scb-v1.md) | 共享协议原文及适用说明 |
| [来源索引](specs/references/README.md) | 两份附件原文副本、来源、哈希与解释优先级 |
| [基线与决定](specs/research/2026-10-17-baseline-and-decisions.md) | 用户确认、仓库现状和联调阻塞 |
| [工具链核对](specs/research/2026-10-17-idf-v6-compatibility.md) | 本机 v6.0.0 事实及官方参考 |
| [01 工程基础](specs/features/01-project-foundation/plan.md) | [需求](specs/features/01-project-foundation/requirements.md) / [验证](specs/features/01-project-foundation/validation.md) |
| [02 SCB/1 协议](specs/features/02-scb-protocol/plan.md) | [需求](specs/features/02-scb-protocol/requirements.md) / [验证](specs/features/02-scb-protocol/validation.md) |
| [03 SPI 从机](specs/features/03-spi-slave/plan.md) | [需求](specs/features/03-spi-slave/requirements.md) / [验证](specs/features/03-spi-slave/validation.md) |
| [04 网络服务](specs/features/04-network-services/plan.md) | [需求](specs/features/04-network-services/requirements.md) / [验证](specs/features/04-network-services/validation.md) |
| [05 桥接与故障](specs/features/05-bridge-lifecycle/plan.md) | [需求](specs/features/05-bridge-lifecycle/requirements.md) / [验证](specs/features/05-bridge-lifecycle/validation.md) |
| [06 验证与交付](specs/features/06-validation-delivery/plan.md) | [需求](specs/features/06-validation-delivery/requirements.md) / [验证](specs/features/06-validation-delivery/validation.md) |

## 已确定的接口

目标 `esp32s3`，不依赖 PSRAM。SPI2_HOST：CS GPIO10、SCK GPIO12、MOSI GPIO11、MISO GPIO13、低有效 READY GPIO14；Mode 0，每次交换 1024B。网络：WPA2、`SmartCar-XXXX`、`192.168.4.1/24`、DHCP、TCP 5000、由当前 TCP 对端订阅 UDP 端口（电脑默认 5001）。完整约定以 [协议文档](specs/protocol/scb-v1.md) 为准。

本工程锁定本机 **ESP-IDF v6.0.0**，路径 `D:\esp\v6.0\esp-idf`。原任务书 v5.5.1 是来源基线，已被用户明确替代；既有安装保持不变。

## 后续构建、烧录与联调入口

Group 1提供CMake工程及 `scripts/check_config.py` 输入检查。按 [配置说明](config/local-config.example.md) 在本机准备被忽略的配置；在已核验的v6.0.0环境运行 `python -m unittest discover -s tests -v`。正式步骤见 [中文构建说明](docs/build-and-flash.md)，入口为 `scripts/activate.ps1` 和 `scripts/build.ps1`，Group 3验证见 [报告](docs/validation/phase-01-group-3.md)。当前AP/socket/SPI完整服务和烧录仍未交付/授权；以下涉及后续阶段的要求继续保留：

1. 在 v6.0.0 环境验证 `idf.py --version` 和配套工具，拒绝误用 v5.5.5 等其他版本。
2. 在被忽略的本地配置提供 8～63 个 ASCII 字符密码；仓库只存无真实密码的配置说明/示例。
3. Phase 01 Group 3已有 `scripts/build.ps1`，白名单复制至独立无空格目录并回收固件、map、实际大小报告和脱敏版本清单；不移动仓库。
4. Phase 06 计划交付中文的安装、配置、编译、串口选择、烧录、复位和联调步骤。具体串口、Flash 容量及实板型号核实后再填，不宣称已烧录。
5. 按公共向量→模拟对端→SPI 握手→Wi-Fi 上传/导出→运行调参验证；真实 STM32 联调须先解除下述阻塞。

## 已知阻塞与待验项

- 用户要求的最新分支`feature/15-comment-documentation`已快进合并到本地main（`e3a4410`）。`SmartCar/App/datalog.h`现为**96B记录、256B头**，初始main的32B尺寸不匹配已消除；完整偏移及SCB/1链路仍需验证，不等同于实际联调通过。本次新提交不修改SmartCar。
- 来源任务书列出了共同向量类别，没有给出固定十六进制数据。后续由独立方法核验并形成共享向量基线；未与对端核对前不能宣称三端向量一致。
- G-02会话状态机已实现并由网络任务执行关闭TCP（04）；[G-01](specs/features/04-network-services/g-01-busy.md)立即HELLO_ACK/BUSY已实现；[G-03](specs/features/05-bridge-lifecycle/g-03-event-gap.md)首个源事件锚点已确认并先纯规约提交，尚待对应实现/三端核对。SCB/1归档原文保持不变。
- 本次目标构建工具已实际核验；实机内存预算、GPIO实际接线、SCK、READY、吞吐、故障恢复和导出一致性仍待相应阶段验证。
- 文档阶段没有真实密码需求，不在文档或聊天中收集密码。
