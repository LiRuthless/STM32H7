相邻记录可以共享控制序号，采样 tick 仍逐拍保留。

TIM6 控制周期 2ms；TIM15 采样记录周期 1ms。两者同抢占优先级，不互相嵌套，因此快照发布与复制不交错；末尾内存屏障仍要保留。灰度序号与 control_sequence 表示记录所采用的完整状态版本，并不要求每条日志递增。轮速记录为原始 1ms 编码器脉冲，不能直接当作控制目标使用的脉冲/2ms。

源码与契约：SmartCar/App/services/run_record.c::Datalog_PublishControl；RunRecord_CaptureSample；SmartCar/App/system/scheduler.c。