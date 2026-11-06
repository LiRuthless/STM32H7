联合身份是 session_id + run_id + record index。

六个u32分别位于偏移0、4、8、12、16、20。STM32封包先将ESP32丢包数字段写0，桥接按契约补自己的累计值并重算CRC。索引存在缺口时不能压缩时间轴；上一运行延迟到达的数据不能依据RUN_STARTED事件接收顺序归入新运行。最大负载984B，加SCB头32B为1016B，SPI事务再补零到1024B。

源码与契约：SmartCar/App/services/telemetry.c::Telemetry_Service；SmartCar/Docs/services/communication.md#scb1。