STM32 提供 SCB / SPI2；桥接与网络是跨端契约。

物理层STM32为SPI2主机、ESP32为从机，READY低有效。网络按公共契约由ESP32开热点192.168.4.1，单电脑控制会话；TCP用于命令结果事件日志，UDP仅用于实时TELEMETRY。当前文档明确未找到ESP32桥接交付，本页连接线表示接口目标，不是本次已验证的三端闭环。USB/串口CSV是另一个既有停车导出入口。

源码与契约：SmartCar/Docs/services/communication.md#scb1；SmartCar/Docs/services/communication.md#f25。