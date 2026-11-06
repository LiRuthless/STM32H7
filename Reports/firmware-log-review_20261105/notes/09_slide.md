先写内容再发布 head；Flash 成功后才释放 tail。

Datalog_Service 先固定本批 tail，从已发布 head 计算 pending，DMB后复制。复制阶段及Flash阻塞阶段均不移动tail，因此ISR生产者无法复用消费者仍在使用的槽。写成功才累计written，随后屏障并发布tail。写失败可能已部分写入芯片，不能承诺回滚；本批不计入成功前缀并锁存s_write_failed，以防重复写入。

源码与契约：SmartCar/App/services/logger.c::Datalog_Push；Datalog_Service。