只有新的有效读取，才推进 offset、CRC 与 10 s 租期。

缓存只允许重取最近成功块，不是任意随机访问。重复块不重读Flash、不重复累计CRC也不续租，避免客户端以重复请求永久占住起跑互斥。新的成功读取更新next_offset与进展时间。普通状态查询同样不续租。STM32无法直接感知TCP断开；桥接停止转发后最迟按10秒无进展超时释放，不虚构LOG_CANCEL消息或用重置SPI实现断线通知。部分读错误取消导出。

源码与契约：SmartCar/App/services/log_export.c::Datalog_ReadExport；Datalog_ServiceExport；SmartCar/Docs/services/communication.md STM32适配说明。