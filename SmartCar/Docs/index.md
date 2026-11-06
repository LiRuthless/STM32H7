# SmartCar 现行功能文档

**开发需求看Phase，功能与实现看docs，再到代码。** 当前基线68cb508；本轮文档迁移不代表新固件或实机验收。

- [Phase/FR到docs](traceability.md)：需求入口与33个功能（含2026-11-06独立板测增量）。
- [架构和模块反查](architecture.md)：按源码模块找唯一主功能页。
- [已知问题](known-issues.md)：当前冲突、待设计和验证限制。
- [运行起停/双保护](system/runtime-and-safety.md)、[传感/标定](sensing/sensors-and-calibration.md)。
- [循迹控制](control/tracking-and-controllers.md)、[元素/休眠](control/elements-and-dormant-code.md)。
- [参数/调参](services/parameters-and-tuning.md)、[日志/导出](services/recording-and-export.md)、[通信/SCB](services/communication.md)。
- [HMI操作](hmi/operation.md)、[BSP平台](platform/board-and-bsp.md)、[构建烧录](guides/build-and-flash.md)、[验证维护](guides/validation.md)。

32项灰度标定仍-1，两种模式都不能起跑。光电默认、低档、板载屏；单信号赛道不满足双保护。R07原样本、AC5、实测栈、700/900us、波形/吞吐/实车待验。

Phase保留变更动机、FR/TC、用户决定、分组和历史结果；docs是现行规则/接口/实现定位，未决定或未实现的契约明确标记，不能用代码现状替用户决策。标准/开发执行仍分别引用code-style/AGENTS。

独立板测：见[F33 传感器、按键与板载屏](platform/isolated-board-tests.md#f33)，Phase02 FR-27～30。
