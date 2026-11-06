Flash 与网络承载相同原始字节，保留区清零。

当前 datalog_record_t 是固定小端目标布局，尺寸和偏移由头文件断言核对。gyro 为原始值除 14.3 后截断到 int16_t 的度/秒；battery 保留 ADC 值；dl1b_mm=8192 表示无效。gray_raw 的 ch0 在最左侧。布局中没有 revision、angle_x 或 accel_x，网络不得随意改 reserved 字节存入额外信息。页面按字段分组示意，不按字节长度比例绘制。

源码与契约：SmartCar/App/services/datalog.h::datalog_record_t；SmartCar/Docs/services/recording-and-export.md#f19。