# ESP32Bridge

ESP32-S3的SPI从机/Wi-Fi桥接工程，负责SCB/1传输，不承担车辆控制决策。

## 当前状态（2026-11-05文档核对）

源码已有启动与固定池、帧/JSON、可靠会话、SPI事务和网络服务。生产桥接消费者、完整队列/导出生命周期及Phase06最终验证交付未完成。
历史记录确认旧镜像启动/AP/DHCP成功；修复后的HELLO保持、BUSY、重连等仍待实机复测。既有主机结果不代表本轮重跑或整机已可用。
R-05批准记录冲突保留待核实；本轮仅整理文档，不推进Phase05/06。阶段及证据状态统一见[roadmap](specs/roadmap.md)。

## 阅读入口

| 目的 | 入口 |
|---|---|
| 了解现行功能 | [Docs](Docs/index.md)、[架构与模块反查](Docs/architecture.md) |
| 查需求、变更依据和进度 | [Phase → Docs](Docs/traceability.md)、[roadmap](specs/roadmap.md) |
| 开发及代码/注释标准 | [AGENTS](AGENTS.md)、[code-style](specs/code-style.md) |
| 构建与配置 | [构建指南](Docs/guides/build-and-flash.md)、[无密码配置说明](config/local-config.example.md) |
| 验证及限制 | [验证方法](Docs/guides/validation.md)、[已知问题](Docs/known-issues.md) |
| 公共线格式 / ESP32适配 | [SCB/1](../SmartCar/Docs/services/communication.md#scb1)、[本端协议](Docs/protocol/framing-and-links.md) |
| 变更和来源 | [CHANGELOG](CHANGELOG.md)、[附件来源](specs/references/README.md) |

工程锁定ESP-IDF v6.0.0、目标esp32s3、不依赖PSRAM。不要把历史机器路径、串口号或构建产物直接当作当前环境。烧录与复位需明确授权；本轮未执行，也未导入未跟踪源码或提交。
