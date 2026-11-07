# ESP32Bridge 功能文档

2026-11-07现行入口。采用前层治理 → 完整Phase → Docs ⇄ 代码：完整阶段设计与验收在Phase，当前功能机制及实现定位在Docs，未来目标留在Phase。原工程现状已独立导入35e5627，纯规约a0e0917先于本轮源码整改；原历史提交声明不等于在本仓重验。

| 主题 | 功能 |
|---|---|
| [启动与资源](system/startup-and-resources.md#f01) | F01 启动、密码、固定池 |
| [帧与链路](protocol/framing-and-links.md#f02) | F02 编解码/JSON、F03 可靠窗口/会话 |
| [SPI](transport/spi.md#f04) | F04 DMA事务/READY/协议端口 |
| [网络](transport/network.md#f05) | F05 AP/TCP/订阅UDP |
| [桥接](bridge/forwarding-and-lifecycle.md#f06) | F06 接管/信用、F07 遥测/事件、F08 清理/导出 |
| [诊断](system/diagnostics.md#f09) | F09 快照与串口 |
| [验证与工具](guides/validation.md#f10) | F10 构建、验证及交付；[操作指南](guides/build-and-flash.md) |

导航：[需求追溯](traceability.md)、[架构反查](architecture.md)、[已知问题](known-issues.md)、[阶段状态](../specs/roadmap.md)。
F06～F08生产链未完成；F09统一桥接统计、F10最终交付也未完成。R-05批准冲突及导出计时歧义不因文档整理解除；旧镜像AP/DHCP成功与修复后待复测分开。原报告/JSON见[验证记录](validation/)。

## 本轮存量整改追溯

2026-11-05：[Phase01～04格式与注释整改契约](../specs/research/2026-11-05-source-style-remediation-draft.md)；47文件静态整改与人工复核已完成，目标构建未验证，见[本轮报告](validation/2026-11-05-source-style-review.md)；现行功能、状态owner和模块映射不变。阶段验收及硬件待验不因本次整改自动完成。


## 主题与参数反查

[全局参数](parameters.md)；[固定池/DMA](system/resource-pools.md#f01-pools)；[帧检查](protocol/frame-codec.md#f02-validation)；[JSON/u32](protocol/json-validation.md#f02-json)；[可靠与会话](protocol/reliable-and-sessions.md#f03)。保留system/protocol/transport/bridge/guides五领域，validation为证据层，不是第六类固件功能。
