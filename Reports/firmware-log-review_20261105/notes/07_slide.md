擦除、写头、回读比较全部成功，才发布 active。

Datalog_Start 先拒绝导出期间起跑，再初始化会话与队列。DATALOG_ENABLE 必须为1且 DATALOG_DIV 必须为1，标定必须有效。LogStore_WriteHeader 使用独立静态回读缓冲，按写、读、memcmp顺序短路。任何一步失败锁存故障，不建立有效日志。当前文档记录32项标定仍为-1，两种模式不能起跑；此处描述实现路径，不表示当前车辆具备可跑条件。

源码与契约：SmartCar/App/services/logger.c::Datalog_Start；SmartCar/App/services/log_store.c::LogStore_WriteHeader；SmartCar/App/system/lifecycle.c::Lifecycle_Start。