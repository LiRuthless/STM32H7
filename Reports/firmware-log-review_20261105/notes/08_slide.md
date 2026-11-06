预留一个空槽，让 head == tail 唯一表示空队列。

s_buf 是1024×96=98304B。Flash 队列用模1024索引与预留空槽法，不能称其可存1024条积压。队满时拒绝本条、rejected加1并请求安全停车，不能覆盖未消费记录。约1.023秒只是1000条/秒下静态可积压时间，不是Flash能阻塞这么久仍满足全部实时与看门狗要求的实测证明。

源码与契约：SmartCar/App/services/logger.c::Datalog_Push；SmartCar/App/services/datalog.h。