头部显式小端编码；先完整验证，再进入业务。

头布局：0魔数SCB1，4版本，5类型，6flags，8payload_len，10record_count，12session_id，16seq，20ack_seq，24request_id，28crc32。CRC先计算头字节0–27，再续算有效负载；不包含CRC字段和SPI尾部填充。帧校验包括长度、类型、身份与CRC，不能坏帧执行一部分命令。TCP按字节流处理拆粘包，SPI与TCP转发重新封装seq/ack并计算CRC。

源码与契约：SmartCar/App/services/scb_frame.c；scb_frame.h；SmartCar/Docs/services/communication.md#scb1。