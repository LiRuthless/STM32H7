# ESP32Bridge

ESP32-S3-DevKitM-1 的 SPI 从机与 Wi-Fi 桥接工程。面向智能车开发者、联调人员和后续维护者，负责 SCB/1 传输，不承担车辆控制决策。

## 当前状态

2026-10-18：文档基线`5c15c49`，当前分支`codex/02-scb-protocol`。Phase 01三个软件组已提交（8750d6a、a2947a8、1b5b9fe），实机验收仍未完成。现已实现Phase 02 Group 1帧编解码、CRC、二进制边界和JSON校验，新增27项、全量104项主机检查及v6.0.0目标构建通过，待用户审查提交；见[本组报告](docs/validation/phase-02-group-1.md)。G-02已确认并先纯规约提交c524638，可靠窗口/握手/固定共同向量尚未实施；Phase 03～06未实施。当前入口仍只运行基础启动，AP/socket/SPI完整服务待后续，未烧录。

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
- G-02已确认SPI换会话关闭TCP、重连HELLO握手并独立规约提交c524638；第二控制连接BUSY（G-01）及桥接端EVENT_GAP（G-03）仍须实施前分别确认。SCB/1归档原文保持不变。
- 本次目标构建工具已实际核验；实机内存预算、GPIO实际接线、SCK、READY、吞吐、故障恢复和导出一致性仍待相应阶段验证。
- 文档阶段没有真实密码需求，不在文档或聊天中收集密码。
