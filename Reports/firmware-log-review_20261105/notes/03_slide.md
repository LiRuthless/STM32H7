业务状态与协议状态分开，Flash 只有一个 App 访问点。

run_record 持有唯一 256B 会话头和 96B 控制快照；logger 持有 active、header_valid、accepted、written、故障及环形索引。log_store 只负责转发 BSP 字节 I/O 和写头回读比较，不维护会话。log_export 持有 transfer_id、next_offset、CRC 和块缓存，command_dispatch 将业务结果映射为 SCB。不要因模块拆分而制造另一份可写权威状态。

源码与契约：SmartCar/App/services/run_record.c；logger.c；log_store.c；log_export.c；telemetry.c；scb_link.c。