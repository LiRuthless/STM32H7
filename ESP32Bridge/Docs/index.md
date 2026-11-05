# ESP32Bridge 功能文档

2026-11-05现行入口。采用Phase → Docs → 代码：开发依据与验收在Phase，当前/目标契约及实现定位在功能页。核对基线是当前未跟踪工作区，不将历史提交声明当成本仓已验证历史。

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
