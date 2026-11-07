# ESP32Bridge文档体系整理记录

- 日期：2026-11-05（Asia/Shanghai）。
- 基线：Git HEAD a0070b8；工程整体未跟踪，工作前143个文件、56份Markdown。
- 状态：本轮授权的文档整理已完成；产品、历史批准及硬件待验项不据此变为完成。
- 范围：仅ESP32Bridge文档。无源码、配置、脚本、测试或原JSON内容修改，无构建/行为回归/烧录/复位/暂存/提交/推送。

## 采用的结构与决定

采用SDD2.0的Phase → Docs → 代码关系，参考STM32通用排版和中文Doxygen/C完整H摘要，保留bridge_/scb_及SDK边界，不移植STM32功能数量、阶段排期、模块前缀、R08/AC5/双LCD。

10个语义功能在7份功能/指南页维护，58条原FR/TC和3条R6补充增量分别追溯。六阶段与原编号保留；原治理、阶段设计及协议提取明确冻结，原结果、日期、哈希和决定声明不写成当前重验。

R-05批准冲突、导出计时基准歧义、当前副本桥接模块/原产物缺失、实机待验及任务书来源哈希差异均保留为未决。现行[索引](../../Docs/traceability.md)、[架构](../../Docs/architecture.md)、[问题](../../Docs/known-issues.md)，原治理见[冻结记录](2026-11-05-governance-history.md)。

## 验收结果

| 项目 | 本轮实际结果 |
|---|---|
| 文档清单 | 70份Markdown；新增14份，既有52份内容调整，其中12份同时迁移位置；4份既有Markdown内容保持 |
| 功能身份 | F01～F10各1个主锚点；源文件按语义归属，不为每C/H建立独立功能 |
| 原需求覆盖 | 42条FR+16条TC：逐条需求锚点、主条目、必要协作及全局映射完整 |
| 补充增量 | R6-01～03单列，不改58条原需求计数，不掩盖桥接模块缺失 |
| 本地链接/锚点/大小写 | 890处活动相对链接；缺失目标0、无效锚点0、大小写问题0（排除两份原附件及冻结代码块） |
| 内容边界 | 87个既有非Markdown文件逐一SHA256相同，包含全部10份阶段JSON；无新增非Markdown文件 |
| 附件 | 两份原附件本轮字节不变；任务书实际哈希与旧表声明本已不符，EB-06保留差异 |
| 产品验证 | 未构建、未行为回归、未实机；旧计数不作当前结果 |
| Git范围 | 交付前工作区仍保留原Hardware等在途项；未暂存/提交，未将未跟踪工程源码导入历史 |

文档检查是本次会话内的临时内容核对，没有新增永久检查器。非Markdown文件清单（原相对路径+逐文件SHA256，排序后）的聚合SHA256为55af19edeb6229c230fb65ca6734d9fd17214a621b4cd257d21873ae43bc7fbf；迁移JSON按原docs→Docs路径对应比较内容。

## 上下文抽查结论

| 对象 | 真实定位与文档结论 |
|---|---|
| 启动失败 | startup.c逐依赖报告，startup_idf.c先READY保护/密码/池；网络失败后仍尝试SPI；F01未把缺回调写成成功 |
| DMA/READY | spi_bridge.c先submit/queue，get返回原描述后reclaim；等待中仅poll，不把超时当回收；F04保留硬件超长饱和限制 |
| 可靠ACK | reliable_link.c已发送图像精确ack释放；reserve决定接管，发送完成才开始计时；F03未升级为业务成功 |
| 会话/网络 | net_bridge.c的session_check关旧TCP，link_service仅非零会话重建才故障关闭；F05保留零会话状态连接 |
| 背压 | startup未绑定bridge_state，NET仅本地SUBSCRIBE；F06未把端口或夹具当生产转发 |
| 导出超时 | 当前无本端完整状态；原P05新READ/匹配DATA两个计时叙述登记EB-05，10秒STM32实际释放仍待验 |
| 日志增量 | SPI已有R6字段，bridge_state/bridge_stats当前不存在；F09和EB-04分开事实与原声明 |
| 格式/注释 | 抽查startup.c、spi_bridge.c及两个接口头有同一行大括号、紧凑声明/控制体、英文普通块注、缺中文文件/函数Doxygen及H完整/C不足；只登记差距，未全量逐定义审计或整改 |

## 新增文档

- Docs/architecture.md
- Docs/bridge/forwarding-and-lifecycle.md
- Docs/guides/validation.md
- Docs/index.md
- Docs/known-issues.md
- Docs/protocol/framing-and-links.md
- Docs/system/diagnostics.md
- Docs/system/startup-and-resources.md
- Docs/traceability.md
- Docs/transport/network.md
- Docs/transport/spi.md
- specs/code-style.md
- specs/research/2026-11-05-document-system-alignment.md
- specs/research/2026-11-05-governance-history.md

## 既有文档调整清单

以下为内容实际改变的52份既有Markdown；迁移路径按整理后列出。原始附件不在其中。

- AGENTS.md
- CHANGELOG.md
- config/local-config.example.md
- Docs/guides/build-and-flash.md
- Docs/validation/phase-01-handoff.md
- Docs/validation/phase-01-group-1.md
- Docs/validation/phase-01-group-2.md
- Docs/validation/phase-01-group-3.md
- Docs/validation/phase-02-group-1.md
- Docs/validation/phase-02-group-2.md
- Docs/validation/phase-02-group-3.md
- Docs/validation/phase-03-group-1.md
- Docs/validation/phase-03.md
- Docs/validation/phase-04.md
- Docs/validation/phase-05-preflight.md
- README.md
- specs/features/01-project-foundation/group-2-design.md
- specs/features/01-project-foundation/plan.md
- specs/features/01-project-foundation/requirements.md
- specs/features/01-project-foundation/validation.md
- specs/features/02-scb-protocol/g-02-session-change.md
- specs/features/02-scb-protocol/group-2-design.md
- specs/features/02-scb-protocol/plan.md
- specs/features/02-scb-protocol/requirements.md
- specs/features/02-scb-protocol/validation.md
- specs/features/03-spi-slave/g-06-task-json.md
- specs/features/03-spi-slave/group-1-design.md
- specs/features/03-spi-slave/lifecycle-design.md
- specs/features/03-spi-slave/plan.md
- specs/features/03-spi-slave/requirements.md
- specs/features/03-spi-slave/validation.md
- specs/features/04-network-services/g-01-busy.md
- specs/features/04-network-services/network-design.md
- specs/features/04-network-services/plan.md
- specs/features/04-network-services/requirements.md
- specs/features/04-network-services/validation.md
- specs/features/05-bridge-lifecycle/g-03-event-gap.md
- specs/features/05-bridge-lifecycle/lifecycle-design.md
- specs/features/05-bridge-lifecycle/plan.md
- specs/features/05-bridge-lifecycle/r6-log-counters.md
- specs/features/05-bridge-lifecycle/requirements.md
- specs/features/05-bridge-lifecycle/validation.md
- specs/features/06-validation-delivery/plan.md
- specs/features/06-validation-delivery/requirements.md
- specs/features/06-validation-delivery/validation.md
- specs/mission.md
- specs/protocol/scb-v1.md
- specs/references/README.md
- specs/research/2026-10-17-baseline-and-decisions.md
- specs/roadmap.md
- specs/techstack.md
- specs/traceability.md

## 路径迁移

- docs/整体改为Docs/；原validation报告和10份JSON相邻保留，JSON字节不变。
- 原docs/build-and-flash.md → Docs/guides/build-and-flash.md。
- 原docs/phase-01-handoff.md → Docs/validation/phase-01-handoff.md。
- 原specs/traceability.md保留M映射历史入口，现行需求索引为Docs/traceability.md。

## 剩余事项

EB-01～EB-06分别保留R-05、跨端/实机、存量标准差距、实现/证据缺失、导出计时及来源哈希问题。修复须按问题归属及明确授权推进，不将文档整理视为行为批准或硬件验收。工程整体未跟踪，本轮只交付工作区文档，不自动提交或导入源码。

## 2026-11-07 适用说明

以上为11月5日历史记录，原结果保持。阶段设计冻结及Docs承载未来目标的旧规则已由[现行AGENTS](../../AGENTS.md)纠正，完整Phase恢复主体。见[本次交付](../../../Reports/esp32bridge-documentation-system-repair-results-2026-11-07.md)。
