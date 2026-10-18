# 路线图（Roadmap）

2026-10-18，分支`codex/04-network-services`。Phase 04网络服务软件已实现并接入生产启动，220项全回归及v6.0.0目标build/size通过，待阶段审查提交；[Phase 04交审](../docs/validation/phase-04.md)及[最终证据](../docs/validation/phase-04-evidence.json)。全部实机待验，不勾选整体完成。

Phase 02三个本端软件组已提交455f817/a0973a0/3c830de，固定候选向量三端未核对；Phase 03已审查提交2335b3d。当前SPI可独立握手/交换，网络服务可提供热点/单控制TCP/订阅UDP；生产桥接消费者尚未实现，除SUBSCRIBE外的可靠业务不ACK，SPI遥测端口留待Phase 05绑定。Phase 05～06、真实透明桥接及最终验证工具/中文实机手册仍未完成；不烧录、不push、不合并。

## 阶段总览

| 阶段 / 身份键 | 交付目标 | 依赖 | 规模 | 状态 / 三件套 |
|---|---|---|---|---|
| 01 `01-project-foundation` | 工程基础、配置和无空格路径构建 | 文档审查及纯规约提交 | small | [ ] 三个软件组已提交，实机验收待验；[计划](features/01-project-foundation/plan.md) / [需求](features/01-project-foundation/requirements.md) / [验证](features/01-project-foundation/validation.md) |
| 02 `02-scb-protocol` | 编解码、会话、可靠性及共同向量 | 01；三端向量基线核对 | medium | [ ] 本端三个软件组已审查提交，三端对齐及真实适配待验；[计划](features/02-scb-protocol/plan.md) / [需求](features/02-scb-protocol/requirements.md) / [验证](features/02-scb-protocol/validation.md) |
| 03 `03-spi-slave` | DMA 从机和 READY 时序 | 01、02 | medium | [ ] 本端SPI服务/协议软件已审查提交2335b3d，实机待验；[计划](features/03-spi-slave/plan.md) / [需求](features/03-spi-slave/requirements.md) / [验证](features/03-spi-slave/validation.md) |
| 04 `04-network-services` | SoftAP、单控制 TCP、订阅 UDP | 01、02；可用模拟 SPI | medium | [ ] 软件已实现待阶段审查，220项回归及目标build/size通过，实机未验；[计划](features/04-network-services/plan.md) / [需求](features/04-network-services/requirements.md) / [验证](features/04-network-services/validation.md) |
| 05 `05-bridge-lifecycle` | 接管背压、队列、会话生命周期、导出和故障统计 | 02、03、04 | medium | [ ] 待实现；[计划](features/05-bridge-lifecycle/plan.md) / [需求](features/05-bridge-lifecycle/requirements.md) / [验证](features/05-bridge-lifecycle/validation.md) |
| 06 `06-validation-delivery` | 故障/吞吐验收、构建证据及中文交付 | 01～05；真实联调另有布局门禁 | medium | [ ] 待实现；[计划](features/06-validation-delivery/plan.md) / [需求](features/06-validation-delivery/requirements.md) / [验证](features/06-validation-delivery/validation.md) |

## 当前状态与门禁

- 按用户最新指令，Phase末统一审查一次，阶段内部不再逐组等待；Phase 03软件已审查提交2335b3d，Phase 04软件已实现待审查。全部硬件验收仍待验。
- G-01立即HELLO_ACK/BUSY、G-02换SPI会话关闭TCP、G-03首个缺失源事件锚点均获用户明确确认；分别见对应阶段决定。只确认本端方案，三端兼容仍待核对。
- G-04：14条固定候选向量本端独立CRC/字段/SHA256与生产C检查通过，三端比对未执行。候选JSON的Git LF检出规则已修复，固定文件SHA256未变。
- G-05：只读核对SmartCar原头文件24项记录、29项头部偏移及96B/256B尺寸主机编译通过；未发现现有自维护代码具备SCB/1处理入口。真实协议能力、目标运行与电气SPI仍待验，不改SmartCar。
- 所有实机项目未执行；不烧录、不push、不合并、不自动EN复位。缺硬件不阻止其余独立实现/软件验证。

## 已合并 / 已取消的计划与决定

- 2026-10-17：采用独立 ESP32Bridge 文档体系，不套用 SmartCar 的功能编号，不修改其他目录。
- 2026-10-17：用户选择沿用本机 **ESP-IDF v6.0.0**，替代来源任务书 v5.5.1；SCB/1 仍为 v1。
- 2026-10-17：用户要求本次先文档、不生成代码，并选择全部阶段完整规约、保留逐阶段审查。暂无已合并或取消的功能阶段。
- 2026-10-17：用户确认G-01～G-03仅登记为实施前必须确认项，本轮不补协议草案，保持SCB/1原文（D-06）；相关功能及验证仍为待实现/未执行。
- 2026-10-17：用户新指令授权将GitHub最新分支合并到本地main，再创建新分支提交本次文档（D-07），替代此前暂不提交的限制。`feature/15-comment-documentation`以24个提交快进至`e3a4410`；不push。同步取证发现96B/256B尺寸已对齐，保留初始32B历史，不新增固件实现。

2026-10-18门禁更新：用户已明确[G-02关闭TCP方案](features/02-scb-protocol/g-02-session-change.md)，先纯规约提交，再实现会话层动作及后续网络关闭；G-01/G-03仍待确认，全部关联验证仍待执行。历史“G-01～G-03未确认”按此更新解释。
