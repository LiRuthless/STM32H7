Flash 保留运行数据；实时上传服务在线观察。

TIM6 结尾发布完整控制快照，TIM15 在日志门禁通过后复制快照并补入当前采样。RunRecord_CaptureSample 内部先向独立上传队列复制完整记录，返回后 logger 才发布 Flash 队列 head。网络拥塞不改变 Flash 的 tail；但上传生产依附于日志生产门禁，不是绕过日志故障仍持续采样的另一套系统。

源码与契约：SmartCar/App/services/run_record.c::RunRecord_CaptureSample；SmartCar/App/services/logger.c::Datalog_Push。