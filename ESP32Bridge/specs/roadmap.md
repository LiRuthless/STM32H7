# 路线图（Roadmap）

> 2026-10-17：全阶段文档已提交，Phase 01已启动，尚无完成阶段。文档提交不勾选功能阶段，也不解除具体设计门禁。

最新状态（2026-10-18）：Phase 01三个软件组已提交8750d6a、a2947a8、1b5b9fe，完整实机验收待验。用户继续Phase 02～06，当前分支`codex/02-scb-protocol`从该有效基线创建；Phase 02 Group 1编解码/JSON已实现，27项新增及104项全回归、目标构建通过，待审查；[报告](../docs/validation/phase-02-group-1.md)。G-02已确认并独立规约提交c524638；Group 2/3及Phase 03～06未实现。

## 阶段总览

| 阶段 / 身份键 | 交付目标 | 依赖 | 规模 | 状态 / 三件套 |
|---|---|---|---|---|
| 01 `01-project-foundation` | 工程基础、配置和无空格路径构建 | 文档审查及纯规约提交 | small | [ ] 三个软件组已提交，实机验收待验；[计划](features/01-project-foundation/plan.md) / [需求](features/01-project-foundation/requirements.md) / [验证](features/01-project-foundation/validation.md) |
| 02 `02-scb-protocol` | 编解码、会话、可靠性及共同向量 | 01；三端向量基线核对 | medium | [ ] Group 1实现/验证待审；Group 2/3待实现；[计划](features/02-scb-protocol/plan.md) / [需求](features/02-scb-protocol/requirements.md) / [验证](features/02-scb-protocol/validation.md) |
| 03 `03-spi-slave` | DMA 从机和 READY 时序 | 01、02 | medium | [ ] 待实现；[计划](features/03-spi-slave/plan.md) / [需求](features/03-spi-slave/requirements.md) / [验证](features/03-spi-slave/validation.md) |
| 04 `04-network-services` | SoftAP、单控制 TCP、订阅 UDP | 01、02；可用模拟 SPI | medium | [ ] 待实现；[计划](features/04-network-services/plan.md) / [需求](features/04-network-services/requirements.md) / [验证](features/04-network-services/validation.md) |
| 05 `05-bridge-lifecycle` | 接管背压、队列、会话生命周期、导出和故障统计 | 02、03、04 | medium | [ ] 待实现；[计划](features/05-bridge-lifecycle/plan.md) / [需求](features/05-bridge-lifecycle/requirements.md) / [验证](features/05-bridge-lifecycle/validation.md) |
| 06 `06-validation-delivery` | 故障/吞吐验收、构建证据及中文交付 | 01～05；真实联调另有布局门禁 | medium | [ ] 待实现；[计划](features/06-validation-delivery/plan.md) / [需求](features/06-validation-delivery/requirements.md) / [验证](features/06-validation-delivery/validation.md) |

## 当前状态与门禁

- 完整验收完成的功能阶段：无。文档基线`5c15c49`；Phase 01软件基础全部提交后，当前从`1b5b9fe`创建阶段02分支；未从旧main启动。
- 下一步：审查[Phase 02 Group 1报告](../docs/validation/phase-02-group-1.md)，确认后提交并实施Group 2；Phase 01实机和阶段02向量/三端对齐仍待验，不直接跳至后续阶段。
- 03、04 的模拟验证可独立开展；本轮不创建模拟器。没有对端代码不能成为跳过独立协议验证的理由。
- 原32B尺寸不匹配已随最新分支合并消除：当前SmartCar为96B记录、256B头，G-05改为完整布局和对端能力待验；未取得偏移及SCB/1功能证据前，不宣称日志、导出或调参端到端通过。
- 共同向量尚未提供固定字节，阶段02须独立核验、与三端对齐后才能标为“公共向量一致”。
- G-02已按用户明确决定选择关闭TCP/重连HELLO握手，纯规约提交c524638；G-01/G-03仍须实施前分别确认，SCB/1归档原文不改。
- 所有验证必须记录实际证据；主机通过、目标构建通过、实机通过分开，不能替代。功能阶段只有所属必要验证完成才能勾选。

## 已合并 / 已取消的计划与决定

- 2026-10-17：采用独立 ESP32Bridge 文档体系，不套用 SmartCar 的功能编号，不修改其他目录。
- 2026-10-17：用户选择沿用本机 **ESP-IDF v6.0.0**，替代来源任务书 v5.5.1；SCB/1 仍为 v1。
- 2026-10-17：用户要求本次先文档、不生成代码，并选择全部阶段完整规约、保留逐阶段审查。暂无已合并或取消的功能阶段。
- 2026-10-17：用户确认G-01～G-03仅登记为实施前必须确认项，本轮不补协议草案，保持SCB/1原文（D-06）；相关功能及验证仍为待实现/未执行。
- 2026-10-17：用户新指令授权将GitHub最新分支合并到本地main，再创建新分支提交本次文档（D-07），替代此前暂不提交的限制。`feature/15-comment-documentation`以24个提交快进至`e3a4410`；不push。同步取证发现96B/256B尺寸已对齐，保留初始32B历史，不新增固件实现。

2026-10-18门禁更新：用户已明确[G-02关闭TCP方案](features/02-scb-protocol/g-02-session-change.md)，先纯规约提交，再实现会话层动作及后续网络关闭；G-01/G-03仍待确认，全部关联验证仍待执行。历史“G-01～G-03未确认”按此更新解释。
