# 路线图

## 当前状态（2026-11-05文档核对）

已有工程基础、协议、SPI及网络生产服务；完整桥接消费者/队列/导出生命周期和Phase06最终交付未完成。以实际文件及Docs状态为准，不把历史待提交文字当当前分支状态。

2026-10-20原记录中旧镜像启动/AP/DHCP成功；修复后HELLO保持/BUSY/断线重连/业务拒绝仍待上机复测。222项是历史主机结果，不是本轮重跑。R6另记258项但所述桥接模块当前缺失，见EB-04。

## 阶段总览

| 阶段 | 依赖 | 当前边界 | 资料 |
|---|---|---|---|
| 01 工程基础 | 规约与范围确认 | [ ] 软件基础已存在；阶段实机项未全部验收，整体未验收 | [计划](features/01-project-foundation/plan.md) / [需求](features/01-project-foundation/requirements.md) / [验证](features/01-project-foundation/validation.md) |
| 02 SCB/1协议 | 01 | [ ] 本端协议已实现；候选向量三端未核对，整体未验收 | [计划](features/02-scb-protocol/plan.md) / [需求](features/02-scb-protocol/requirements.md) / [验证](features/02-scb-protocol/validation.md) |
| 03 SPI从机 | 01、02 | [ ] SPI事务/协议已接入；电气和吞吐待验，整体未验收 | [计划](features/03-spi-slave/plan.md) / [需求](features/03-spi-slave/requirements.md) / [验证](features/03-spi-slave/validation.md) |
| 04 网络服务 | 01、02 | [ ] 网络已接入；修复后实机流程待复测，整体未验收 | [计划](features/04-network-services/plan.md) / [需求](features/04-network-services/requirements.md) / [验证](features/04-network-services/validation.md) |
| 05 桥接与故障处理 | 02、03、04；R-05核实 | [ ] 桥接生产消费者未实现；R-05批准记录冲突，整体未验收 | [计划](features/05-bridge-lifecycle/plan.md) / [需求](features/05-bridge-lifecycle/requirements.md) / [验证](features/05-bridge-lifecycle/validation.md) |
| 06 验证与交付 | 01～05；05阶段审查通过 | [ ] 最终负载/三端实机/完整交付未完成，整体未验收 | [计划](features/06-validation-delivery/plan.md) / [需求](features/06-validation-delivery/requirements.md) / [验证](features/06-validation-delivery/validation.md) |

## 决定、门禁与下一步

- 保留2026-10-17 SDK v6.0.0及独立ESP32职责范围；保留六阶段身份，不新增阶段。
- 2026-10-18起阶段内连续推进、阶段末统一交审；既有G-01/G-02/G-03/G-06方案决定保留，三端兼容不由本端决定推定。
- 原记录已恢复Phase05，但R-05批准记录互相冲突；本轮只迁移文档，不选择哪份声明为真，不解除相关门禁。导出计时基准先核实。
- 下一实施步骤是核实P05资源与计时决定，再按已有三组规约先行；不能以本轮整理批准Phase05代码或进入06。
- 当前SmartCar已有SCB/命令/导出实现，旧“无入口/32B”是旧基线；96B/256B存在不等于实机互操作通过。

现行[功能入口](../Docs/index.md)、[问题](../Docs/known-issues.md)、[原治理/排期决定](research/2026-11-05-governance-history.md)、[原开工证据](../Docs/validation/phase-05-preflight.md)。

## 2026-11-05 源码整改批次

用户已确认157文件工程基线导入，独立基线35e5627；[本轮契约](research/2026-11-05-source-style-remediation-draft.md)复用Phase01～04身份，按G1～G4连续执行。47个C/H静态整改及人工复核已完成，所选187项宿主验证通过；v6.0.0目标构建环境缺失、S5未验证，EB-03整体验收保持打开，不启动05/06。见[本轮报告](../Docs/validation/2026-11-05-source-style-review.md)。

## 2026-11-07 现行文档体系

mission、techstack、roadmap、code-style及AGENTS共同约束需求、设计、实施与验收。体系为前层治理文档 → Phase → Docs ⇄ 代码。Phase完整保存背景、需求、详细设计、用户决定、计划、文件与符号映射、验证标准和实际结果，按阶段基线解释，不缩减为摘要跳转。Docs说明当前功能、算法、状态、时序、参数、错误和限制，与代码语义对应。目标草案和未开工组设计保存在Phase。

后续变更：Phase确认并独立提交 → 先更新本组Docs → 紧接实现 → 双向核对 → 匹配Docs与代码同批交付。短暂领先标明“本组实现同步中”、所属Phase/任务组及更新前代码基线，完成前取消。显式关闭阶段后的独立修改新增Phase；进行中的阶段可以补组与回填，不能覆盖历史决定及结果；软件完成但硬件待验不自动关闭。

本次仅文档纠偏，允许明确路径分组提交，不新增Phase07，不启动05/06，不改变ESP-IDF v6.0.0、任务边界、命名及本项目验证政策，不推送或烧录。

## 文档纠偏与验收维度

2026-11-07文档纠偏不改变阶段完成状态。软件已存在：01～04基础/协议/SPI/NET；缺失：05生产桥接与06最终交付。当前目标构建：11月5日S5未验证；历史目标构建结果保留各自版本。硬件/跨端：旧镜像AP/DHCP已有记录，修复后网络和SPI整链待验。阶段关闭：01～06均未整体验收，不补勾。实际文档验收见[交付记录](../../Reports/esp32bridge-documentation-system-repair-results-2026-11-07.md)。
