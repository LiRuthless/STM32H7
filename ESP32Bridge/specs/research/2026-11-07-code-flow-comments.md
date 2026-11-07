# 2026-11-07 函数与流程注释补充契约

用户要求：为 ESP32Bridge 代码注明函数功能与工作流程。

沿用 Phase01～04 及 [既有整改契约](2026-11-05-source-style-remediation-draft.md)，功能目标分别为 [F01](../../Docs/system/startup-and-resources.md)、[F02/F03](../../Docs/protocol/framing-and-links.md)、[F04](../../Docs/transport/spi.md)、[F05](../../Docs/transport/network.md)。

本批仅补充 main 下现有 C 文件的中文流程说明。保留现有完整函数 Doxygen，增加启动、内存池、编解码、JSON、会话、可靠窗口、SPI 与网络关键路径的阶段说明。既有函数说明已覆盖用途与参数，不机械重复。保持所有可执行代码、接口、状态、协议、日志及配置不变；不推进 Phase05/06，不烧录、复位或推送。

验收：对照完整实现审查注释，比较移除注释后的 C 词法 token 序列，并检查 Git 差异与换行。复用现有构建入口；环境不足则如实记录未验证。实际结果回填 Docs/validation/2026-11-07-code-flow-comments.md。
