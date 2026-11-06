版本 2 固定；头部保存起跑时配置快照。

魔数 0x324C4353，version=2，header_size=256，record_size=96。RunRecord_BuildHeader 清零后构造会话头，并计算从偏移8开始248B的CRC16。两模式方向参数取当前档显式参数 bank。700/900us 是写在头中的灰度与控制预算，并非已测得耗时。会话头是起跑快照，不能代替运行调参历史。

源码与契约：SmartCar/App/services/run_record.c::RunRecord_BuildHeader；datalog.h::datalog_header_t。