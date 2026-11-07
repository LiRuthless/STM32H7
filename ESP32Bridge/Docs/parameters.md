# 全局参数入口

2026-11-07。此页按用途反查，实际值、单位、约束、owner、生效/复位及源码依据保存在下列主说明，避免两套值表。编译容量不是实测资源；当前池额度与未来Phase05候选分别核算。

| 参数组 | 主说明与适用条件 | 阶段来源 |
|---|---|---|
| 启动/密码/失败依赖 | [启动](system/startup-and-resources.md#f01-startup)、[配置](system/startup-and-resources.md#f01-config)，仅记录配置键及校验规则 | [P01](../specs/features/01-project-foundation/group-2-design.md) |
| 固定池/代际/DMA | [池额度](system/resource-pools.md#f01-pools)、[DMA所有权](system/resource-pools.md#f01-dma)，运行期借还不改变启动容量 | [P01设计](../specs/features/01-project-foundation/group-2-design.md) |
| 帧尺寸/CRC/各类长度 | [线格式](protocol/frame-codec.md#f02-frame)、[检查矩阵](protocol/frame-codec.md#f02-validation)，按SPI/TCP/UDP分别适用 | [P02需求](../specs/features/02-scb-protocol/requirements.md) |
| JSON长度/深度/工作区/u32 | [JSON](protocol/json-validation.md#f02-json)、[精确u32](protocol/json-validation.md#f02-u32)，每调用任务独占 | [P02设计](../specs/features/02-scb-protocol/group-2-design.md) |
| 窗口/ACK/重试/session | [窗口](protocol/reliable-and-sessions.md#f03-window)、[重试](protocol/reliable-and-sessions.md#f03-retry)，计时从实际完整发送完成开始 | [P02](../specs/features/02-scb-protocol/plan.md) |
| SPI引脚/READY/任务/等待 | [事务](transport/spi.md#f04-transaction)、[READY](transport/spi.md#f04-ready)、[回收](transport/spi.md#f04-ownership) | [P03生命周期](../specs/features/03-spi-slave/lifecycle-design.md) |
| AP关联/TCP槽/backlog/BUSY/UDP | [AP](transport/network.md#f05-ap)、[TCP](transport/network.md#f05-tcp)、[BUSY](transport/network.md#f05-busy)、[订阅](transport/network.md#f05-subscribe)、[计时](transport/network.md#f05-timing) | [P04设计](../specs/features/04-network-services/network-design.md) |
| SPI/NET/池快照与日志 | [SPI](system/diagnostics.md#f09-spi)、[NET](system/diagnostics.md#f09-net)、[池](system/diagnostics.md#f09-pool)、[日志](system/diagnostics.md#f09-logging) | P03/P04与[R6](../specs/features/05-bridge-lifecycle/r6-log-counters.md)历史边界 |
| 未实现资源/导出额度 | [Phase05完整设计](../specs/features/05-bridge-lifecycle/lifecycle-design.md)，仅未来目标，不纳入当前预算 | R-05批准冲突、导出计时歧义保持未决 |

[模块反查](architecture.md) / [FR/TC追溯](traceability.md) / [已知限制](known-issues.md)。密码实际值不进入文档、日志或交付物，本次未读取密码文件。
