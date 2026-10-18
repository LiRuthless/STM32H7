# 路线图（Roadmap）

> 2026-10-17：全阶段文档已提交，Phase 01已启动，尚无完成阶段。文档提交不勾选功能阶段，也不解除具体设计门禁。

最新状态（2026-10-18）：用户要求继续余下五阶段，按Phase 02～06理解，先补齐Phase 01。分支`codex/01-project-foundation`上Group 1已提交为`8750d6a`；[Group 2必要增量](features/01-project-foundation/group-2-design.md)已先独立提交为73a2a2f，对应代码及59项主机检查/目标构建现待用户审查，见 [Group 2报告](../docs/validation/phase-01-group-2.md)。Group 3及阶段02～06尚未实施，Phase 01整体仍不勾选。[Group 1证据](../docs/validation/phase-01-group-1.md)保留为该组执行时的记录，未解除G-01～G-03。

## 阶段总览

| 阶段 / 身份键 | 交付目标 | 依赖 | 规模 | 状态 / 三件套 |
|---|---|---|---|---|
| 01 `01-project-foundation` | 工程基础、配置和无空格路径构建 | 文档审查及纯规约提交 | small | [ ] 实施中，Group 1已提交、Group 2实现待审查；[计划](features/01-project-foundation/plan.md) / [需求](features/01-project-foundation/requirements.md) / [验证](features/01-project-foundation/validation.md) |
| 02 `02-scb-protocol` | 编解码、会话、可靠性及共同向量 | 01；三端向量基线核对 | medium | [ ] 待实现；[计划](features/02-scb-protocol/plan.md) / [需求](features/02-scb-protocol/requirements.md) / [验证](features/02-scb-protocol/validation.md) |
| 03 `03-spi-slave` | DMA 从机和 READY 时序 | 01、02 | medium | [ ] 待实现；[计划](features/03-spi-slave/plan.md) / [需求](features/03-spi-slave/requirements.md) / [验证](features/03-spi-slave/validation.md) |
| 04 `04-network-services` | SoftAP、单控制 TCP、订阅 UDP | 01、02；可用模拟 SPI | medium | [ ] 待实现；[计划](features/04-network-services/plan.md) / [需求](features/04-network-services/requirements.md) / [验证](features/04-network-services/validation.md) |
| 05 `05-bridge-lifecycle` | 接管背压、队列、会话生命周期、导出和故障统计 | 02、03、04 | medium | [ ] 待实现；[计划](features/05-bridge-lifecycle/plan.md) / [需求](features/05-bridge-lifecycle/requirements.md) / [验证](features/05-bridge-lifecycle/validation.md) |
| 06 `06-validation-delivery` | 故障/吞吐验收、构建证据及中文交付 | 01～05；真实联调另有布局门禁 | medium | [ ] 待实现；[计划](features/06-validation-delivery/plan.md) / [需求](features/06-validation-delivery/requirements.md) / [验证](features/06-validation-delivery/validation.md) |

## 当前状态与门禁

- 已完成的功能阶段：无。文档基线`5c15c49`位于`codex/constitution-esp32bridge`；当前实现分支`codex/01-project-foundation`从该提交创建，未从旧main启动。
- 下一步：用户审查Group 2实现及验证结果，确认后再提交代码并推进Group 3；不跨过Group 2/3或逐阶段审查直接铺开五阶段代码。对应跨端设计门禁仍在所属功能实施前确认。
- 03、04 的模拟验证可独立开展；本轮不创建模拟器。没有对端代码不能成为跳过独立协议验证的理由。
- 原32B尺寸不匹配已随最新分支合并消除：当前SmartCar为96B记录、256B头，G-05改为完整布局和对端能力待验；未取得偏移及SCB/1功能证据前，不宣称日志、导出或调参端到端通过。
- 共同向量尚未提供固定字节，阶段02须独立核验、与三端对齐后才能标为“公共向量一致”。
- 来源仍有未完全定义的跨端行为，见 [决定记录的实施前问题](research/2026-10-17-baseline-and-decisions.md)。用户已确认G-01～G-03保留为实施前必须确认项，SCB/1原文不变；对应功能组实现前须逐项确认具体线格式，不凭本次处理方式确认擅自扩展协议。
- 所有验证必须记录实际证据；主机通过、目标构建通过、实机通过分开，不能替代。功能阶段只有所属必要验证完成才能勾选。

## 已合并 / 已取消的计划与决定

- 2026-10-17：采用独立 ESP32Bridge 文档体系，不套用 SmartCar 的功能编号，不修改其他目录。
- 2026-10-17：用户选择沿用本机 **ESP-IDF v6.0.0**，替代来源任务书 v5.5.1；SCB/1 仍为 v1。
- 2026-10-17：用户要求本次先文档、不生成代码，并选择全部阶段完整规约、保留逐阶段审查。暂无已合并或取消的功能阶段。
- 2026-10-17：用户确认G-01～G-03仅登记为实施前必须确认项，本轮不补协议草案，保持SCB/1原文（D-06）；相关功能及验证仍为待实现/未执行。
- 2026-10-17：用户新指令授权将GitHub最新分支合并到本地main，再创建新分支提交本次文档（D-07），替代此前暂不提交的限制。`feature/15-comment-documentation`以24个提交快进至`e3a4410`；不push。同步取证发现96B/256B尺寸已对齐，保留初始32B历史，不新增固件实现。
