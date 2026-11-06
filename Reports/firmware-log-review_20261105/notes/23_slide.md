57 列便于分析；参数 revision 历史要另存事件。

CSV只在停车导出当前有效会话，先验证Flash头，再逐条读written记录，输出16路灰度原始值与16路按头内黑白标定重算q值；结束有status和EOF，无有效会话明确报告原因。串口c只擦首区与重置前沿，不创建有效会话。Flash头是起跑配置、记录不存revision，因此单独Flash无法完整复原运行中临时调参；必须结合电脑事件，断线无法重建的历史明确未知。

源码与契约：SmartCar/App/services/log_export.c::Datalog_Dump；SmartCar/Docs/services/recording-and-export.md#f21。