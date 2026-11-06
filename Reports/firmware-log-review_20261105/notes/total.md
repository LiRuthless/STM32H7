# 01_slide

从 1 ms 采样到 Flash、实时上传与停车导出。

本次评审围绕当前 STM32 端实现展开，建立数据生成、持久化、上传与导出的完整心智模型。重点审查并发所有权、发布与释放顺序、异常时保留的数据，以及软件证据和硬件证据的区别。ESP32 与电脑网络部分按公共契约说明，不能将其当成本仓已完成的桥接固件。

源码与契约：SmartCar/Docs/services/recording-and-export.md；SmartCar/Docs/services/communication.md。

---

# 02_slide

Flash 保留运行数据；实时上传服务在线观察。

TIM6 结尾发布完整控制快照，TIM15 在日志门禁通过后复制快照并补入当前采样。RunRecord_CaptureSample 内部先向独立上传队列复制完整记录，返回后 logger 才发布 Flash 队列 head。网络拥塞不改变 Flash 的 tail；但上传生产依附于日志生产门禁，不是绕过日志故障仍持续采样的另一套系统。

源码与契约：SmartCar/App/services/run_record.c::RunRecord_CaptureSample；SmartCar/App/services/logger.c::Datalog_Push。

---

# 03_slide

业务状态与协议状态分开，Flash 只有一个 App 访问点。

run_record 持有唯一 256B 会话头和 96B 控制快照；logger 持有 active、header_valid、accepted、written、故障及环形索引。log_store 只负责转发 BSP 字节 I/O 和写头回读比较，不维护会话。log_export 持有 transfer_id、next_offset、CRC 和块缓存，command_dispatch 将业务结果映射为 SCB。不要因模块拆分而制造另一份可写权威状态。

源码与契约：SmartCar/App/services/run_record.c；logger.c；log_store.c；log_export.c；telemetry.c；scb_link.c。

---

# 04_slide

相邻记录可以共享控制序号，采样 tick 仍逐拍保留。

TIM6 控制周期 2ms；TIM15 采样记录周期 1ms。两者同抢占优先级，不互相嵌套，因此快照发布与复制不交错；末尾内存屏障仍要保留。灰度序号与 control_sequence 表示记录所采用的完整状态版本，并不要求每条日志递增。轮速记录为原始 1ms 编码器脉冲，不能直接当作控制目标使用的脉冲/2ms。

源码与契约：SmartCar/App/services/run_record.c::Datalog_PublishControl；RunRecord_CaptureSample；SmartCar/App/system/scheduler.c。

---

# 05_slide

Flash 与网络承载相同原始字节，保留区清零。

当前 datalog_record_t 是固定小端目标布局，尺寸和偏移由头文件断言核对。gyro 为原始值除 14.3 后截断到 int16_t 的度/秒；battery 保留 ADC 值；dl1b_mm=8192 表示无效。gray_raw 的 ch0 在最左侧。布局中没有 revision、angle_x 或 accel_x，网络不得随意改 reserved 字节存入额外信息。页面按字段分组示意，不按字节长度比例绘制。

源码与契约：SmartCar/App/services/datalog.h::datalog_record_t；SmartCar/Docs/services/recording-and-export.md#f19。

---

# 06_slide

版本 2 固定；头部保存起跑时配置快照。

魔数 0x324C4353，version=2，header_size=256，record_size=96。RunRecord_BuildHeader 清零后构造会话头，并计算从偏移8开始248B的CRC16。两模式方向参数取当前档显式参数 bank。700/900us 是写在头中的灰度与控制预算，并非已测得耗时。会话头是起跑快照，不能代替运行调参历史。

源码与契约：SmartCar/App/services/run_record.c::RunRecord_BuildHeader；datalog.h::datalog_header_t。

---

# 07_slide

擦除、写头、回读比较全部成功，才发布 active。

Datalog_Start 先拒绝导出期间起跑，再初始化会话与队列。DATALOG_ENABLE 必须为1且 DATALOG_DIV 必须为1，标定必须有效。LogStore_WriteHeader 使用独立静态回读缓冲，按写、读、memcmp顺序短路。任何一步失败锁存故障，不建立有效日志。当前文档记录32项标定仍为-1，两种模式不能起跑；此处描述实现路径，不表示当前车辆具备可跑条件。

源码与契约：SmartCar/App/services/logger.c::Datalog_Start；SmartCar/App/services/log_store.c::LogStore_WriteHeader；SmartCar/App/system/lifecycle.c::Lifecycle_Start。

---

# 08_slide

预留一个空槽，让 head == tail 唯一表示空队列。

s_buf 是1024×96=98304B。Flash 队列用模1024索引与预留空槽法，不能称其可存1024条积压。队满时拒绝本条、rejected加1并请求安全停车，不能覆盖未消费记录。约1.023秒只是1000条/秒下静态可积压时间，不是Flash能阻塞这么久仍满足全部实时与看门狗要求的实测证明。

源码与契约：SmartCar/App/services/logger.c::Datalog_Push；SmartCar/App/services/datalog.h。

---

# 09_slide

先写内容再发布 head；Flash 成功后才释放 tail。

Datalog_Service 先固定本批 tail，从已发布 head 计算 pending，DMB后复制。复制阶段及Flash阻塞阶段均不移动tail，因此ISR生产者无法复用消费者仍在使用的槽。写成功才累计written，随后屏障并发布tail。写失败可能已部分写入芯片，不能承诺回滚；本批不计入成功前缀并锁存s_write_failed，以防重复写入。

源码与契约：SmartCar/App/services/logger.c::Datalog_Push；Datalog_Service。

---

# 10_slide

单次运行追加记录；新起跑覆盖上一会话。

W25Q64总容量8MiB，日志区0..0x7FEFFF，共8384512B，减256B头再按96B整除，容量87336条。剩余不可整除字节不用作记录。第87336条被接受后触发容量满停车，不等待下一条才停车。上电不扫描旧Flash来恢复有效记录条数，Flash非易失不等于应用具有掉电恢复能力。地址示意不按实际空间比例绘制。

源码与契约：SmartCar/BSP/bsp_flash.h；SmartCar/App/services/datalog.h；SmartCar/App/services/logger.c::Datalog_Push。

---

# 11_slide

768 B 批次降低服务频度；跨扇区先擦再写。

前台每次Datalog_Service最多8条，不无限追赶。LogStore_Write只是字节后端，跨页与懒擦由BSP_Flash_LogWrite负责。起跑只擦首个4KiB，维护s_log_frontier，写范围超出前沿时擦后续扇区，再逐页编程。普通批次768B可能从非页首开始，不能固定称每批恰好三次页写。96kB/s为记录负载，不包含擦除、命令、页编程等待等开销。

源码与契约：SmartCar/App/services/logger.c::Datalog_Service；SmartCar/BSP/bsp_flash.c::BSP_Flash_LogWrite。

---

# 12_slide

先锁存故障与首因，前台再有界收尾。

日志首fault只写一次；保护位按或累加，停车首次原因不被后续故障覆盖。Datalog_Stop清active，再最多128次服务，覆盖1023条最大积压在8条一批时的正常收尾需求。每批喂狗；遇写失败停止而不重试。Datalog_Flush本身只做一次Service，不是清空队列。读取故障在Logger_LatchFault里不清active，不能把所有fault归为同一生产停止语义。

源码与契约：SmartCar/App/services/logger.c::Datalog_RequestFaultStop；Datalog_Stop；SmartCar/App/system/safety.c。

---

# 13_slide

256 槽全容量；满时丢本条并保留索引缺口。

Telemetry_PushRecord先取index=s_next_index++，再检查队满，所以丢弃不压缩采样索引。上传使用单调uint32计数及掩码取槽，区别于Flash预留空槽算法，全部256槽可用。slot还保存index、run_id和revision。生产丢弃与消费丢弃使用分开的计数器，查询时在短临界区求和。BeginRun分配非零运行编号，Service清理非当前运行积压。

源码与契约：SmartCar/App/services/telemetry.c::Telemetry_PushRecord；Telemetry_BeginRun；Telemetry_Service。

---

# 14_slide

通常每 10 ms 最多 10 条；边界到来可提前封包。

Telemetry_Service先固定head并DMB，只选择同一run、同一revision且index连续的1到10条。遇到缺口或身份变化时boundary置位，即使未到10ms也允许结束旧批。正常连续批未到10ms不消费。封包复制完成后才DMB并释放tail。revision在完整调参拍、暂停和恢复的既有时点发布，已入队槽不会被追溯改版本。

源码与契约：SmartCar/App/services/telemetry.c::Telemetry_Service；Telemetry_SetRevision。

---

# 15_slide

联合身份是 session_id + run_id + record index。

六个u32分别位于偏移0、4、8、12、16、20。STM32封包先将ESP32丢包数字段写0，桥接按契约补自己的累计值并重算CRC。索引存在缺口时不能压缩时间轴；上一运行延迟到达的数据不能依据RUN_STARTED事件接收顺序归入新运行。最大负载984B，加SCB头32B为1016B，SPI事务再补零到1024B。

源码与契约：SmartCar/App/services/telemetry.c::Telemetry_Service；SmartCar/Docs/services/communication.md#scb1。

---

# 16_slide

STM32 提供 SCB / SPI2；桥接与网络是跨端契约。

物理层STM32为SPI2主机、ESP32为从机，READY低有效。网络按公共契约由ESP32开热点192.168.4.1，单电脑控制会话；TCP用于命令结果事件日志，UDP仅用于实时TELEMETRY。当前文档明确未找到ESP32桥接交付，本页连接线表示接口目标，不是本次已验证的三端闭环。USB/串口CSV是另一个既有停车导出入口。

源码与契约：SmartCar/Docs/services/communication.md#scb1；SmartCar/Docs/services/communication.md#f25。

---

# 17_slide

头部显式小端编码；先完整验证，再进入业务。

头布局：0魔数SCB1，4版本，5类型，6flags，8payload_len，10record_count，12session_id，16seq，20ack_seq，24request_id，28crc32。CRC先计算头字节0–27，再续算有效负载；不包含CRC字段和SPI尾部填充。帧校验包括长度、类型、身份与CRC，不能坏帧执行一部分命令。TCP按字节流处理拆粘包，SPI与TCP转发重新封装seq/ack并计算CRC。

源码与契约：SmartCar/App/services/scb_frame.c；scb_frame.h；SmartCar/Docs/services/communication.md#scb1。

---

# 18_slide

传输 ACK 只表示接管；TELEMETRY 不做可靠重传。

SCB_PrepareFrame先检查会话超时与本端seq临界值，处理握手和已到期的可靠重发；无可靠帧在途才取待发结果和事件，然后尝试Telemetry，最后5ms轮询IDLE。因此等待可靠ACK期间仍可发不可靠遥测，不应画成整个链路被可靠窗口阻塞。重发保留原seq与内容。request_id缓存最近32请求，业务去重与帧去重分层；RECEIVED、APPLIED、SAVED不等于帧ACK。通信失败不新增停车条件。

源码与契约：SmartCar/App/services/scb_link.c::SCB_PrepareFrame；SCB_ReceiveFrame；SCB_AcceptRequest。

---

# 19_slide

READY 给许可；TX / RX 缓冲在途期间不可复用。

BSP使用两个1024B AXI缓冲，GCC段固定在0x2407F000与0x2407F400，均32B对齐；当前D-Cache关闭，不是实现了缓存维护。回调不做CRC、JSON或阻塞等待。HAL_BUSY保留许可；启动错误消费此次许可并等待新沿。前台10ms超时或错误态中止，局部屏蔽DMA/SPI IRQ，READY IRQ继续计数。实际SPI时钟必须核算并测量，不能用旧估算频率证明吞吐。

源码与契约：SmartCar/BSP/bsp_wifi.c::BSP_WiFi_TryExchange；BSP_WiFi_Service；BSP_WiFi_ReleaseRx。

---

# 20_slide

一个 transfer_id，窗口 1，最多 960 B 原始数据 / 块。

Begin要求App_IsStopped、日志不active、无既有导出并且有当前有效头。冻结record_count=written，total_bytes=256+96×written。LOG_DATA负载是u32 offset、u16 length、u16 reserved0及原始字节；最大968B。command_dispatch在同一次前台调用内复制借出的块，不能延长内部指针寿命。导出期间起跑和清日志所有入口都必须拒绝。

源码与契约：SmartCar/App/services/log_export.c::Datalog_BeginExport；Datalog_ReadExport；Datalog_EndExport。

---

# 21_slide

只有新的有效读取，才推进 offset、CRC 与 10 s 租期。

缓存只允许重取最近成功块，不是任意随机访问。重复块不重读Flash、不重复累计CRC也不续租，避免客户端以重复请求永久占住起跑互斥。新的成功读取更新next_offset与进展时间。普通状态查询同样不续租。STM32无法直接感知TCP断开；桥接停止转发后最迟按10秒无进展超时释放，不虚构LOG_CANCEL消息或用重置SPI实现断线通知。部分读错误取消导出。

源码与契约：SmartCar/App/services/log_export.c::Datalog_ReadExport；Datalog_ServiceExport；SmartCar/Docs/services/communication.md STM32适配说明。

---

# 22_slide

能验证字节一致性，不能补回未成功落盘的尾包。

头CRC16覆盖8..255字节；SCB帧CRC32覆盖头字节0–27与payload，不含CRC字段和SPI尾部填充；导出整体CRC32按顺序对原始256B头及written记录续算，zlib兼容。三个校验范围不同，帧校验成功不替代整体导出校验。Flash写错可能已部分落盘，但只有完整成功批次计入written，导出不包含失败批次。没有掉电会话恢复、多会话或续录，不能因芯片非易失而承诺重启后读取上次日志。

源码与契约：SmartCar/App/services/run_record.c；scb_frame.c；log_export.c；checksum.c。

---

# 23_slide

57 列便于分析；参数 revision 历史要另存事件。

CSV只在停车导出当前有效会话，先验证Flash头，再逐条读written记录，输出16路灰度原始值与16路按头内黑白标定重算q值；结束有status和EOF，无有效会话明确报告原因。串口c只擦首区与重置前沿，不创建有效会话。Flash头是起跑配置、记录不存revision，因此单独Flash无法完整复原运行中临时调参；必须结合电脑事件，断线无法重建的历史明确未知。

源码与契约：SmartCar/App/services/log_export.c::Datalog_Dump；SmartCar/Docs/services/recording-and-export.md#f21。

---

# 24_slide

先确认不变量，再用板上证据关闭性能与联调问题。

本次仅制作讲解资料，没有改动固件或重新运行固件测试。历史Phase16软件验证、双LCD构建与Phase17软件收口不等于三端实机闭环。700/900us预算、SPI波形和持续吞吐必须实测；AC5与栈高水位保持待验。文档D08说明本端next_seq临近回绕会重建，是否还要求接收端检测对端临界序号尚未决定。32项标定仍为-1导致不能起跑，需真实标定后才具备实车验证条件。

源码与契约：SmartCar/Docs/known-issues.md；SmartCar/Docs/services/communication.md#f24；SmartCar/specs/roadmap.md。

---

