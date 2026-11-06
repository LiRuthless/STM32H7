一个 transfer_id，窗口 1，最多 960 B 原始数据 / 块。

Begin要求App_IsStopped、日志不active、无既有导出并且有当前有效头。冻结record_count=written，total_bytes=256+96×written。LOG_DATA负载是u32 offset、u16 length、u16 reserved0及原始字节；最大968B。command_dispatch在同一次前台调用内复制借出的块，不能延长内部指针寿命。导出期间起跑和清日志所有入口都必须拒绝。

源码与契约：SmartCar/App/services/log_export.c::Datalog_BeginExport；Datalog_ReadExport；Datalog_EndExport。