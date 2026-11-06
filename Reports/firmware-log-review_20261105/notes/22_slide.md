能验证字节一致性，不能补回未成功落盘的尾包。

头CRC16覆盖8..255字节；SCB帧CRC32覆盖头字节0–27与payload，不含CRC字段和SPI尾部填充；导出整体CRC32按顺序对原始256B头及written记录续算，zlib兼容。三个校验范围不同，帧校验成功不替代整体导出校验。Flash写错可能已部分落盘，但只有完整成功批次计入written，导出不包含失败批次。没有掉电会话恢复、多会话或续录，不能因芯片非易失而承诺重启后读取上次日志。

源码与契约：SmartCar/App/services/run_record.c；scb_frame.c；log_export.c；checksum.c。