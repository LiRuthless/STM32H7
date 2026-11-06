from pathlib import Path
from html import escape
import sys

ROOT = Path(__file__).resolve().parent
P = '#123252'; A = '#007E87'; B = '#F5F8FB'; T = '#233647'; M = '#52697B'; D = '#CBD9E2'; W = '#FFFFFF'; O = '#AD551D'
FONT = 'Microsoft YaHei'
slides = []
def add(title, message, blocks, note, source, kind='content', rel='order / link: 数据按页面所示顺序流转'):
    slides.append(dict(title=title,message=message,blocks=blocks,note=note,source=source,kind=kind,rel=rel))

add('固件日志记录与上传', '从 1 ms 采样到 Flash、实时上传与停车导出', [],
    '本次评审围绕当前 STM32 端实现展开，建立数据生成、持久化、上传与导出的完整心智模型。重点审查并发所有权、发布与释放顺序、异常时保留的数据，以及软件证据和硬件证据的区别。ESP32 与电脑网络部分按公共契约说明，不能将其当成本仓已完成的桥接固件。', 'SmartCar/Docs/services/recording-and-export.md；SmartCar/Docs/services/communication.md', 'cover', 'link: 同一采样记录分别供存储和上传使用')
add('一份记录，走向两条独立通道', 'Flash 保留运行数据；实时上传服务在线观察', ['TIM6｜2 ms 控制快照','TIM15｜1 ms 合成 96 B 记录','Flash 队列 → W25Q64','上传队列 → SPI2 → ESP32 → 电脑'],
    'TIM6 结尾发布完整控制快照，TIM15 在日志门禁通过后复制快照并补入当前采样。RunRecord_CaptureSample 内部先向独立上传队列复制完整记录，返回后 logger 才发布 Flash 队列 head。网络拥塞不改变 Flash 的 tail；但上传生产依附于日志生产门禁，不是绕过日志故障仍持续采样的另一套系统。', 'SmartCar/App/services/run_record.c::RunRecord_CaptureSample；SmartCar/App/services/logger.c::Datalog_Push')
add('职责边界决定状态归谁所有', '业务状态与协议状态分开，Flash 只有一个 App 访问点', ['run_record｜头与控制快照','logger｜会话、队列与故障','log_store｜Flash 字节访问','telemetry / scb_link｜上传与链路','log_export｜导出身份与进度'],
    'run_record 持有唯一 256B 会话头和 96B 控制快照；logger 持有 active、header_valid、accepted、written、故障及环形索引。log_store 只负责转发 BSP 字节 I/O 和写头回读比较，不维护会话。log_export 持有 transfer_id、next_offset、CRC 和块缓存，command_dispatch 将业务结果映射为 SCB。不要因模块拆分而制造另一份可写权威状态。', 'SmartCar/App/services/run_record.c；logger.c；log_store.c；log_export.c；telemetry.c；scb_link.c', rel='membership: 各模块分别拥有列明状态；link: logger经log_store访问Flash')
add('1 ms 采样，不等于 1 ms 控制', '相邻记录可以共享控制序号，采样 tick 仍逐拍保留', ['TIM6：完整控制 → 发布快照','TIM15：复制最近快照 → 补本拍采样','控制 / 灰度序号允许重复','tick 保留 1 ms 时间索引'],
    'TIM6 控制周期 2ms；TIM15 采样记录周期 1ms。两者同抢占优先级，不互相嵌套，因此快照发布与复制不交错；末尾内存屏障仍要保留。灰度序号与 control_sequence 表示记录所采用的完整状态版本，并不要求每条日志递增。轮速记录为原始 1ms 编码器脉冲，不能直接当作控制目标使用的脉冲/2ms。', 'SmartCar/App/services/run_record.c::Datalog_PublishControl；RunRecord_CaptureSample；SmartCar/App/system/scheduler.c')
add('96 B 固定记录：把时间与状态放在一起', 'Flash 与网络承载相同原始字节，保留区清零', ['0–31｜tick、电感、轮速、输出、IMU、距离、电池、状态','32–63｜16 路完整灰度原始 ADC','64–81｜双序号、双误差、保护、模式、首因','82–95｜14 B 保留区'],
    '当前 datalog_record_t 是固定小端目标布局，尺寸和偏移由头文件断言核对。gyro 为原始值除 14.3 后截断到 int16_t 的度/秒；battery 保留 ADC 值；dl1b_mm=8192 表示无效。gray_raw 的 ch0 在最左侧。布局中没有 revision、angle_x 或 accel_x，网络不得随意改 reserved 字节存入额外信息。页面按字段分组示意，不按字节长度比例绘制。', 'SmartCar/App/services/datalog.h::datalog_record_t；SmartCar/Docs/services/recording-and-export.md#f19', rel='membership: 四段字段共同组成96B记录')
add('256 B 会话头：解释这一次运行', '版本 2 固定；头部保存起跑时配置快照', ['格式｜magic、版本、长度、周期','传感｜黑白标定、权重、阈值','控制｜mode / gear、两模式参数、PI、风扇','容量｜87336 条；CRC16 覆盖字节 8–255'],
    '魔数 0x324C4353，version=2，header_size=256，record_size=96。RunRecord_BuildHeader 清零后构造会话头，并计算从偏移8开始248B的CRC16。两模式方向参数取当前档显式参数 bank。700/900us 是写在头中的灰度与控制预算，并非已测得耗时。会话头是起跑快照，不能代替运行调参历史。', 'SmartCar/App/services/run_record.c::RunRecord_BuildHeader；datalog.h::datalog_header_t', rel='membership: 格式/传感/控制/容量共同解释当前会话')
add('先建立有效日志，再允许起跑', '擦除、写头、回读比较全部成功，才发布 active', ['导出互斥检查','配置 / 标定检查','擦首扇区 → 写 256 B 头','回读并逐字节比较','header_valid = 1 → active = 1'],
    'Datalog_Start 先拒绝导出期间起跑，再初始化会话与队列。DATALOG_ENABLE 必须为1且 DATALOG_DIV 必须为1，标定必须有效。LogStore_WriteHeader 使用独立静态回读缓冲，按写、读、memcmp顺序短路。任何一步失败锁存故障，不建立有效日志。当前文档记录32项标定仍为-1，两种模式不能起跑；此处描述实现路径，不表示当前车辆具备可跑条件。', 'SmartCar/App/services/logger.c::Datalog_Start；SmartCar/App/services/log_store.c::LogStore_WriteHeader；SmartCar/App/system/lifecycle.c::Lifecycle_Start')
add('Flash 队列：1024 槽，最多积压 1023 条', '预留一个空槽，让 head == tail 唯一表示空队列', ['TIM15 只推进 head','前台只推进 tail','next(head) == tail → 队满','98304 B 分配；可缓存约 1.023 s 采样'],
    's_buf 是1024×96=98304B。Flash 队列用模1024索引与预留空槽法，不能称其可存1024条积压。队满时拒绝本条、rejected加1并请求安全停车，不能覆盖未消费记录。约1.023秒只是1000条/秒下静态可积压时间，不是Flash能阻塞这么久仍满足全部实时与看门狗要求的实测证明。', 'SmartCar/App/services/logger.c::Datalog_Push；SmartCar/App/services/datalog.h', rel='link: TIM15生产与前台消费通过环形队列交接；contrast: 分配槽数与可用条数')
add('发布与释放，各有一个不可提前的时点', '先写内容再发布 head；Flash 成功后才释放 tail', ['生产者｜写完整 96 B → DMB → head','消费者｜固定 tail → 复制批次 → Flash 写入','成功｜written += n → DMB → tail','失败｜保留 tail，锁存写错，不重试'],
    'Datalog_Service 先固定本批 tail，从已发布 head 计算 pending，DMB后复制。复制阶段及Flash阻塞阶段均不移动tail，因此ISR生产者无法复用消费者仍在使用的槽。写成功才累计written，随后屏障并发布tail。写失败可能已部分写入芯片，不能承诺回滚；本批不计入成功前缀并锁存s_write_failed，以防重复写入。', 'SmartCar/App/services/logger.c::Datalog_Push；Datalog_Service', rel='order: 写槽先于发布；完成Flash先于释放；contrast: 成功与失败路径')
add('Flash 空间：日志与参数有明确边界', '单次运行追加记录；新起跑覆盖上一会话', ['0x000000｜256 B 会话头','0x000100 起｜96 B × 87336 条','0x7FF000 起｜最后 4 KiB 参数区','日志约 87.336 s；不循环覆盖'],
    'W25Q64总容量8MiB，日志区0..0x7FEFFF，共8384512B，减256B头再按96B整除，容量87336条。剩余不可整除字节不用作记录。第87336条被接受后触发容量满停车，不等待下一条才停车。上电不扫描旧Flash来恢复有效记录条数，Flash非易失不等于应用具有掉电恢复能力。地址示意不按实际空间比例绘制。', 'SmartCar/BSP/bsp_flash.h；SmartCar/App/services/datalog.h；SmartCar/App/services/logger.c::Datalog_Push', rel='membership: 日志头和记录属于日志区，参数在末扇区；order: 记录顺序追加')
add('前台最多写 8 条，底层按页拆写', '768 B 批次降低服务频度；跨扇区先擦再写', ['运行｜不足 8 条暂不写','停车｜允许不足 8 条的尾包','BSP｜跨 4 KiB 扇区先懒擦','BSP｜拆成不跨 256 B 页的片段'],
    '前台每次Datalog_Service最多8条，不无限追赶。LogStore_Write只是字节后端，跨页与懒擦由BSP_Flash_LogWrite负责。起跑只擦首个4KiB，维护s_log_frontier，写范围超出前沿时擦后续扇区，再逐页编程。普通批次768B可能从非页首开始，不能固定称每批恰好三次页写。96kB/s为记录负载，不包含擦除、命令、页编程等待等开销。', 'SmartCar/App/services/logger.c::Datalog_Service；SmartCar/BSP/bsp_flash.c::BSP_Flash_LogWrite')
add('日志故障停车，网络拥塞不新增停车', '先锁存故障与首因，前台再有界收尾', ['容量满 / 队满 / 写错 → 安全停车请求','ISR 停生产与运动；Flash 留给前台','Datalog_Stop：最多 128 批，每批喂狗','写错后保留成功前缀，尾包不保证落盘'],
    '日志首fault只写一次；保护位按或累加，停车首次原因不被后续故障覆盖。Datalog_Stop清active，再最多128次服务，覆盖1023条最大积压在8条一批时的正常收尾需求。每批喂狗；遇写失败停止而不重试。Datalog_Flush本身只做一次Service，不是清空队列。读取故障在Logger_LatchFault里不清active，不能把所有fault归为同一生产停止语义。', 'SmartCar/App/services/logger.c::Datalog_RequestFaultStop；Datalog_Stop；SmartCar/App/system/safety.c', rel='order: 请求停车先于前台收尾；contrast: 日志故障与上传拥塞')
add('上传队列独立，丢弃不挪动 Flash 的 tail', '256 槽全容量；满时丢本条并保留索引缺口', ['同一完整记录 → 上传槽副本','head − tail >= 256 → dropped++','index 每次应产生记录都递增','新 run_id 重置索引；旧运行积压被丢弃'],
    'Telemetry_PushRecord先取index=s_next_index++，再检查队满，所以丢弃不压缩采样索引。上传使用单调uint32计数及掩码取槽，区别于Flash预留空槽算法，全部256槽可用。slot还保存index、run_id和revision。生产丢弃与消费丢弃使用分开的计数器，查询时在短临界区求和。BeginRun分配非零运行编号，Service清理非当前运行积压。', 'SmartCar/App/services/telemetry.c::Telemetry_PushRecord；Telemetry_BeginRun；Telemetry_Service', rel='contrast: 上传槽与Flash槽独立；order: 索引递增先于队满判断')
add('封包有三条边界：运行、版本、连续索引', '通常每 10 ms 最多 10 条；边界到来可提前封包', ['同 run_id','同 revision','index 连续','任一变化 → 先结束旧批次'],
    'Telemetry_Service先固定head并DMB，只选择同一run、同一revision且index连续的1到10条。遇到缺口或身份变化时boundary置位，即使未到10ms也允许结束旧批。正常连续批未到10ms不消费。封包复制完成后才DMB并释放tail。revision在完整调参拍、暂停和恢复的既有时点发布，已入队槽不会被追溯改版本。', 'SmartCar/App/services/telemetry.c::Telemetry_Service；Telemetry_SetRevision', rel='membership: 同一包的记录必须满足三个约束；order: 边界切开相邻批次')
add('24 B 元数据，让缺口与归属可解释', '联合身份是 session_id + run_id + record index', ['记录索引 / 参数 revision','STM32 丢记录数 / ESP32 丢包数','首条 tick / run_id','24 + 96 × N；N = 1…10'],
    '六个u32分别位于偏移0、4、8、12、16、20。STM32封包先将ESP32丢包数字段写0，桥接按契约补自己的累计值并重算CRC。索引存在缺口时不能压缩时间轴；上一运行延迟到达的数据不能依据RUN_STARTED事件接收顺序归入新运行。最大负载984B，加SCB头32B为1016B，SPI事务再补零到1024B。', 'SmartCar/App/services/telemetry.c::Telemetry_Service；SmartCar/Docs/services/communication.md#scb1', rel='membership: 六个u32与N条记录共同组成上传负载')
add('STM32 到电脑：实现边界必须可见', 'STM32 提供 SCB / SPI2；桥接与网络是跨端契约', ['STM32｜日志、上传队列、SCB、SPI2 DMA','ESP32｜SPI 从机 ↔ Wi-Fi 桥接','电脑｜TCP 命令 / 导出；UDP 实时数据','TCP 5000；UDP 默认 5001'],
    '物理层STM32为SPI2主机、ESP32为从机，READY低有效。网络按公共契约由ESP32开热点192.168.4.1，单电脑控制会话；TCP用于命令结果事件日志，UDP仅用于实时TELEMETRY。当前文档明确未找到ESP32桥接交付，本页连接线表示接口目标，不是本次已验证的三端闭环。USB/串口CSV是另一个既有停车导出入口。', 'SmartCar/Docs/services/communication.md#scb1；SmartCar/Docs/services/communication.md#f25', rel='link: STM32经ESP32桥接到电脑；membership: 网络栈属于外部端')
add('SCB/1：同一帧，传输外壳不同', '头部显式小端编码；先完整验证，再进入业务', ['32 B 头｜类型、长度、身份、seq / ack、CRC32','payload ≤ 992 B','SPI｜固定 1024 B，未用尾部清零','TCP｜头 + 有效负载；UDP｜一报一帧'],
    '头布局：0魔数SCB1，4版本，5类型，6flags，8payload_len，10record_count，12session_id，16seq，20ack_seq，24request_id，28crc32。CRC先计算头字节0–27，再续算有效负载；不包含CRC字段和SPI尾部填充。帧校验包括长度、类型、身份与CRC，不能坏帧执行一部分命令。TCP按字节流处理拆粘包，SPI与TCP转发重新封装seq/ack并计算CRC。', 'SmartCar/App/services/scb_frame.c；scb_frame.h；SmartCar/Docs/services/communication.md#scb1', rel='membership: 头与payload组成帧；contrast: SPI/TCP/UDP封装')
add('可靠结果与实时数据，采用不同取舍', '传输 ACK 只表示接管；TELEMETRY 不做可靠重传', ['每方向最多 1 个可靠帧在途','精确 ack_seq 才释放；200 ms 后重发','最多重发 3 次；800 ms 无有效帧重握手','结果 / 事件优先；实时包可在等待 ACK 时发送'],
    'SCB_PrepareFrame先检查会话超时与本端seq临界值，处理握手和已到期的可靠重发；无可靠帧在途才取待发结果和事件，然后尝试Telemetry，最后5ms轮询IDLE。因此等待可靠ACK期间仍可发不可靠遥测，不应画成整个链路被可靠窗口阻塞。重发保留原seq与内容。request_id缓存最近32请求，业务去重与帧去重分层；RECEIVED、APPLIED、SAVED不等于帧ACK。通信失败不新增停车条件。', 'SmartCar/App/services/scb_link.c::SCB_PrepareFrame；SCB_ReceiveFrame；SCB_AcceptRequest', rel='order: 发送调度有优先顺序；contrast: 可靠业务和可丢遥测')
add('DMA 事务：中断只交接，前台做解析', 'READY 给许可；TX / RX 缓冲在途期间不可复用', ['READY IRQ｜只累计许可','前台｜填帧 → CS 低 → 启动 DMA','完成 IRQ｜CS 高 → 标记 RX 可读','前台｜解析 → ReleaseRx；超时 10 ms 恢复'],
    'BSP使用两个1024B AXI缓冲，GCC段固定在0x2407F000与0x2407F400，均32B对齐；当前D-Cache关闭，不是实现了缓存维护。回调不做CRC、JSON或阻塞等待。HAL_BUSY保留许可；启动错误消费此次许可并等待新沿。前台10ms超时或错误态中止，局部屏蔽DMA/SPI IRQ，READY IRQ继续计数。实际SPI时钟必须核算并测量，不能用旧估算频率证明吞吐。', 'SmartCar/BSP/bsp_wifi.c::BSP_WiFi_TryExchange；BSP_WiFi_Service；BSP_WiFi_ReleaseRx', rel='order: 许可→启动→完成→解析→释放')
add('停车导出：先冻结成功前缀，再顺序读取', '一个 transfer_id，窗口 1，最多 960 B 原始数据 / 块', ['GET_LOG_META｜只查内存头与 written','LOG_BEGIN｜停车收尾完成，冻结长度','LOG_READ｜按 next_offset 顺序读取','LOG_END｜全部交付，返回整体 CRC32 并解锁'],
    'Begin要求App_IsStopped、日志不active、无既有导出并且有当前有效头。冻结record_count=written，total_bytes=256+96×written。LOG_DATA负载是u32 offset、u16 length、u16 reserved0及原始字节；最大968B。command_dispatch在同一次前台调用内复制借出的块，不能延长内部指针寿命。导出期间起跑和清日志所有入口都必须拒绝。', 'SmartCar/App/services/log_export.c::Datalog_BeginExport；Datalog_ReadExport；Datalog_EndExport')
add('重读最近块不推进，也不续租', '只有新的有效读取，才推进 offset、CRC 与 10 s 租期', ['重复最近 offset + length → 返回缓存','乱序 / 越界 / 身份错误 → 拒绝','10 s 无新进展 → 释放导出互斥','会话变化立即取消；TCP 断开靠超时释放'],
    '缓存只允许重取最近成功块，不是任意随机访问。重复块不重读Flash、不重复累计CRC也不续租，避免客户端以重复请求永久占住起跑互斥。新的成功读取更新next_offset与进展时间。普通状态查询同样不续租。STM32无法直接感知TCP断开；桥接停止转发后最迟按10秒无进展超时释放，不虚构LOG_CANCEL消息或用重置SPI实现断线通知。部分读错误取消导出。', 'SmartCar/App/services/log_export.c::Datalog_ReadExport；Datalog_ServiceExport；SmartCar/Docs/services/communication.md STM32适配说明', rel='contrast: 新读取与重复读取；order: 无进展到期释放')
add('三层校验，各自回答不同问题', '能验证字节一致性，不能补回未成功落盘的尾包', ['头 CRC16｜本次会话配置是否完整','帧 CRC32｜这一次传输是否一致','导出 CRC32｜完整导出字节是否一致','成功前缀｜只包含 written 条，不扫描残留'],
    '头CRC16覆盖8..255字节；SCB帧CRC32覆盖头字节0–27与payload，不含CRC字段和SPI尾部填充；导出整体CRC32按顺序对原始256B头及written记录续算，zlib兼容。三个校验范围不同，帧校验成功不替代整体导出校验。Flash写错可能已部分落盘，但只有完整成功批次计入written，导出不包含失败批次。没有掉电会话恢复、多会话或续录，不能因芯片非易失而承诺重启后读取上次日志。', 'SmartCar/App/services/run_record.c；scb_frame.c；log_export.c；checksum.c', rel='contrast: 三类校验覆盖不同字节域')
add('离线分析：CSV 与运行参数历史互补', '57 列便于分析；参数 revision 历史要另存事件', ['串口 d｜元信息 / 标定 → 57 列 → 状态 → EOF','q0…q15｜按会话标定重新归一化','Flash｜不保存动态 revision / 调参历史','电脑事件 JSONL｜补充运行参数变化'],
    'CSV只在停车导出当前有效会话，先验证Flash头，再逐条读written记录，输出16路灰度原始值与16路按头内黑白标定重算q值；结束有status和EOF，无有效会话明确报告原因。串口c只擦首区与重置前沿，不创建有效会话。Flash头是起跑配置、记录不存revision，因此单独Flash无法完整复原运行中临时调参；必须结合电脑事件，断线无法重建的历史明确未知。', 'SmartCar/App/services/log_export.c::Datalog_Dump；SmartCar/Docs/services/recording-and-export.md#f21', rel='contrast: CSV数据与事件参数历史；link: 结合用于分析')
add('评审落点：机制已落地，实测证据仍需补齐', '先确认不变量，再用板上证据关闭性能与联调问题', ['已记录｜STM32 软件实现、构建与历史软件验证','待实测｜Flash ≥96 kB/s 负载 + 擦写开销；60 s 上传','待确认｜SPI 波形、700 / 900 μs、栈、AC5、实车','待设计｜D08 对端临界序号回绕责任；桥接交付'],
    '本次仅制作讲解资料，没有改动固件或重新运行固件测试。历史Phase16软件验证、双LCD构建与Phase17软件收口不等于三端实机闭环。700/900us预算、SPI波形和持续吞吐必须实测；AC5与栈高水位保持待验。文档D08说明本端next_seq临近回绕会重建，是否还要求接收端检测对端临界序号尚未决定。32项标定仍为-1导致不能起跑，需真实标定后才具备实车验证条件。', 'SmartCar/Docs/known-issues.md；SmartCar/Docs/services/communication.md#f24；SmartCar/specs/roadmap.md', 'ending', 'contrast: 软件记录、硬件待验与待决定事项')

def plan():
    info = {'Project Name':'firmware-log-review','Canvas Format':'ppt169 / 1280 × 720','Page Count':len(slides),'Primary Language':'zh-CN','Target Audience':'熟悉C与STM32的固件技术评审人员','Communication Intent':'深入讲解日志与上传实现，审查并发、故障和验证边界','Desired Audience Outcome':'能够追踪关键函数、解释数据归属并识别硬件证据缺口','Core Message / Ask / Action':'同一96B记录进入独立Flash与上传队列，Flash及安全优先，导出仅成功前缀','Delivery Context':'现场技术评审25至30分钟','Artifact Afterlife':'可编辑PPTX与讲者备注，团队维护参考','Reading Mode':'balanced','Content Strategy':'保留必要讲解，减少小字注释，以布局和排版解释关系','Design Style':'数据流主线：浅底深蓝青色，流程/时序/内存示意','AI Image Acquisition Path':'not applicable','Generation Mode':'continuous','Spec Refinement':'disabled','Speaker Notes':'enabled — final Stage-2 proactive policy','Custom Animations':'disabled — final Stage-2 proactive policy','Narration Audio':'disabled — final Stage-2 proactive policy','Created Date':'2026-11-05'}
    s = '<!-- ppt-master-schema: design-spec/v1 -->\n# Firmware Log Review - Design Spec\n\n## I. Project Information\n\n| Item | Value |\n| --- | --- |\n'+''.join(f'| {k} | {v} |\n' for k,v in info.items())
    s += '\n## II. Canvas Specification\n\n| Property | Value |\n| --- | --- |\n| Format | ppt169 |\n| Dimensions | 1280 × 720 |\n| viewBox | `0 0 1280 720` |\n| Margins | 56 px |\n| Content Area | 56 160 1168 480 |\n'
    s += '\n## III. Visual Theme\n\n### Theme Style\n\n- **Mode**: custom\n- **Mode Behavior**: 以同一96B记录为主线，先给整体路径，再沿发布、写入、封包、导出逐层解释边界。\n- **Visual style**: custom\n- **Visual Style Behavior**: 浅底深蓝标题，青色标记数据流；大字图解，独立关系用位置与线连接，必要解释单独成页。\n- **Theme**: 数据流技术评审\n- **Tone**: 准确、直接、证据明确\n\n### Color Scheme\n\n| Role | HEX | Purpose |\n| --- | --- | --- |\n'
    for k,v in [('Background',W),('Secondary background',B),('Primary',P),('Accent',A),('Secondary accent',O),('Body text',T),('Secondary text',M),('Divider',D)]: s+=f'| {k} | {v} | {k} |\n'
    s += '\n## IV. Typography System\n\n### Font Plan\n\n| Role | Character (Reference) | Primary | English if non-English | Fallback tail |\n| --- | --- | --- | --- | --- |\n| Title | 清晰无衬线 | Microsoft YaHei | Microsoft YaHei | sans-serif |\n| Body | 清晰无衬线 | Microsoft YaHei | Microsoft YaHei | sans-serif |\n\n- **Title stack**: Microsoft YaHei\n- **Body stack**: Microsoft YaHei\n\n### Font Size Hierarchy\n\n| Purpose | Anchor Size (px) |\n| --- | ---: |\n| Body | 30 |\n| Title | 42 |\n| Subtitle | 30 |\n| Annotation | 24 |\n| Display | 64 |\n\n## V. Layout Principles\n\n### Deck-wide Direction\n\n- **Hierarchy direction**: 标题→主图→结论；必要限制以正文说明。\n- **Composition tendency**: 路径、泳道、分叉和内存区域随语义变化，不重复套卡片。\n- **Cross-page continuity**: 青色为上传/流转，深蓝为Flash/状态，橙色为限制。\n- **Spacing posture**: 开放，长内容分行而不缩小。\n- **Spacing anchors**: page margin 56; block gap 24; column gutter 40; corner radius 12; body leading 44\n\n## VI. Icon Usage Specification\n\n- **Primary bundled library**: none\n\n| Icon Path | Suitable Scenarios |\n| --- | --- |\n\n## VIII. Image Resource List\n\n| Filename | Dimensions | Ratio | Purpose | Type | Image pattern | Crop Policy | Acquire Via | Status | Reference | text_policy | page_role |\n| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |\n\n## IX. Content Outline\n\n### Part 1: 记录、存储、上传与导出\n'
    for i,p in enumerate(slides,1):
        s+=f'\n#### Slide {i:02d} - {p["title"]}\n\n- **Audience move**: 从知道模块到理解本页机制与边界\n- **Relationships**: {p["rel"]}\n- **Title**: {p["title"]}\n- **Core message**: {p["message"]}\n- **Content**: '+ '；'.join(p['blocks'] or [p['message']])+'\n'
    s+='\n## X. Speaker Notes Requirements\n\n- **Generation**: enabled\n- **Filename**: match SVG basenames\n- **Content**: 每页解释调用关系、关键边界与源码定位；不虚构硬件结果。\n- **Total duration**: 25至30分钟\n- **Notes style**: 正式技术评审讲解\n- **Presentation purpose**: 深入讲解并审查日志和上传机制\n'
    (ROOT/'design_spec.md').write_text(s,encoding='utf-8')
    lock='''<!-- ppt-master-schema: spec-lock/v1 -->
# Execution Lock

## canvas
- viewBox: 0 0 1280 720
- format: ppt169

## communication
- primary_language: zh-CN
- audience: 熟悉C与STM32的固件技术评审人员
- objective: 深入讲解日志上传机制，使评审者追踪数据、理解故障并识别待实测项。
- core_message: 同一96B记录进入独立Flash与上传队列，Flash及安全优先，导出仅成功前缀。

## mode
- mode: custom
- mode_behavior: 沿同一记录的数据流逐层讲解，从整体图到并发与错误边界。

## visual_style
- visual_style: custom
- visual_style_behavior: 浅底深蓝青色，关系用布局与连接表达，大字拆页，必要限制保留为正文。

## colors
- background: #FFFFFF
- secondary_bg: #F5F8FB
- primary: #123252
- accent: #007E87
- secondary_accent: #AD551D
- body_text: #233647
- secondary_text: #52697B
- divider: #CBD9E2

## typography
- font_family: Microsoft YaHei
- title_family: Microsoft YaHei
- body_family: Microsoft YaHei
- body: 30
- title: 42
- subtitle: 30
- annotation: 24
- display: 64

## icons
- library: none
- inventory: none

## page_rhythm
'''
    lock+=''.join(f'- P{i:02d}: {"anchor" if i in [1,2,10,16,20,24] else "dense"}\n' for i in range(1,25))
    lock+='''
## pptx_structure
- mode: flat

## forbidden
- `mask`, `<style>`, `class`, external CSS, `<foreignObject>`, `textPath`, `@font-face`, `<animate*>`, `<set>`, `<script>` / event attributes, `<iframe>`
- HTML named entities in text; write typography as raw Unicode and escape XML reserved characters
'''
    (ROOT/'spec_lock.md').write_text(lock,encoding='utf-8')
    (ROOT/'sources').mkdir(exist_ok=True)
    (ROOT/'sources'/'source_map.md').write_text('# 来源与审查边界\n\n当前HEAD a0070b8848e262391e0b874c89231a2538946ff1。资料核对2026-11-05。\n仅制作演示文稿，不更改固件，不重新宣称验收。\n\n'+ '\n'.join(f'- P{i:02d} {p["title"]}：{p["source"]}' for i,p in enumerate(slides,1)),encoding='utf-8')
    print('24-page plan and execution lock written')

def txt(x,y,lines,size=30,color=T,bold=False,anchor='start',leading=44):
    if isinstance(lines,str): lines=[lines]
    spans=''.join(f'<tspan x="{x}" y="{y+j*leading}">{escape(line)}</tspan>' for j,line in enumerate(lines))
    return f'<text font-size="{size}" fill="{color}" font-weight="{"bold" if bold else "normal"}" text-anchor="{anchor}">{spans}</text>'
def rect(x,y,w,h,fill=B,stroke='none',radius=0): return f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{radius}" fill="{fill}" stroke="{stroke}" stroke-width="2"/>'
def line(x,y,x2,y2,color=D,arrow=False): return f'<line x1="{x}" y1="{y}" x2="{x2}" y2="{y2}" stroke="{color}" stroke-width="3"'+(' marker-end="url(#arrow)"' if arrow else '')+'/>'
def group(name,body,bounds='56 160 1168 480'): return f'<g id="{name}" data-pptx-bounds="{bounds}">{body}</g>'
def wrap(s,n=31):
    # CJK-aware width budget; ASCII counts about half a Chinese character.
    out=[]; cur=''; width=0
    for c in s:
        cw=.55 if ord(c)<128 else 1
        if width+cw>n: out.append(cur);cur='';width=0
        cur+=c;width+=cw
    if cur: out.append(cur)
    return out

def render(i,p):
    body=rect(0,0,1280,720,W)
    body+=group('title',txt(56,92,p['title'],42,P,True),'56 40 1168 70')
    body+=group('claim',txt(56,145,p['message'],30,A),'56 113 1168 42')
    b=p['blocks']
    content=''
    if i==1:
        body=rect(0,0,1280,720,B)
        content=txt(72,180,'STM32H7 / SMARTCAR',30,A,True)+txt(72,290,['固件日志记录','与上传'],64,P,True,leading=88)+txt(76,505,['从 1 ms 采样到 Flash','实时上传与停车导出'],30,T)+line(76,594,1204,594,A)+txt(76,638,'技术评审 · 2026.11.05',24,M)
        body+=group('cover',content,'56 130 1168 520')
    elif i==2:
        content+=rect(56,218,322,300,B)+txt(82,280,['TIM6','2 ms 控制快照'],30,P,True)+line(215,330,215,373,A,True)+txt(82,430,['TIM15','1 ms 合成 96 B'],30,P,True)
        content+=line(378,408,476,408,A)+line(476,300,476,506,A)+line(476,300,548,300,A,True)+line(476,506,548,506,A,True)
        content+=rect(564,224,660,152,B)+txt(592,275,['Flash 队列 → W25Q64','保存后：停车导出'],30,P,True)
        content+=rect(564,430,660,152,B)+txt(592,481,['独立上传队列 → SPI2','ESP32 桥接 → 电脑'],30,A,True)
    elif i==3:
        for j,(name,desc) in enumerate([('run_record','头与控制快照'),('logger','会话、队列与故障'),('log_store','Flash 字节访问')]):
            x=56+j*400;content+=txt(x,246,name,30,P,True)+line(x,270,x+360,270,A)+txt(x,320,desc,30)
        content+=line(56,375,1224,375)+txt(56,447,'telemetry / scb_link',30,A,True)+txt(56,497,'独立队列、封包与链路状态',30)+txt(704,447,'log_export',30,P,True)+txt(704,497,'导出身份、进度与块缓存',30)
    elif i==4:
        content+=txt(56,244,'TIM6',30,P,True)+txt(56,402,'TIM15',30,A,True)
        for j,label in enumerate(['快照 A','快照 B','快照 C']):
            x=255+j*320;content+=rect(x,200,258,72,B)+txt(x+129,247,label,30,P,True,'middle')
        for j,label in enumerate(['A + 采样','A + 采样','B + 采样','B + 采样','C + 采样','C + 采样']):
            x=225+j*164;content+=rect(x,358,153,76,B)+txt(x+76,407,label,24,A,False,'middle')
        content+=txt(240,532,['重复的是最近控制版本','保留的是每个采样 tick'],30,P,True)
    elif i in [5,6,15,17,22,23,24]:
        # Full-width ruled rows keep qualifiers in the main reading path.
        for j,s in enumerate(b):
            y=214+j*103
            content+=txt(56,y+24,f'{j+1:02d}',30,A,True)+txt(132,y+24,wrap(s,35),30,T,leading=40)
            if j<3:content+=line(132,y+73,1224,y+73)
    elif i==7:
        for j,s in enumerate(b):
            y=210+j*78; content+=rect(56,y,62,54,A)+txt(87,y+38,str(j+1),30,W,True,'middle')+txt(154,y+38,s,30,P)
            if j<4: content+=line(87,y+54,87,y+78,A)
    elif i==8:
        for j in range(10):
            x=72+j*114; content+=rect(x,270,100,116,W if j==8 else B,A if j==8 else D)+txt(x+50,343,'空' if j==8 else '96 B',24,A if j==8 else P,False,'middle')
        content+=txt(72,230,'概念示意：预留 1 空槽，不按实际槽数绘制',24,M)+txt(72,454,'TIM15 → head',30,P,True)+txt(770,454,'tail → 前台',30,A,True)
        content+=txt(72,548,['next(head) == tail → 队满停车','1024 × 96 B 分配，1023 条可用'],30,T)
    elif i==9:
        content+=rect(56,204,1168,165,B)+txt(80,247,'生产者',30,A,True)+txt(80,312,'写完整 96 B',30)+line(365,300,455,300,A,True)+txt(485,312,'DMB',30,A,True)+line(620,300,710,300,A,True)+txt(744,312,'发布 head',30,P,True)
        content+=txt(56,432,'消费者',30,A,True)+txt(56,490,'固定 tail → 复制批次 → Flash 写入',30,P,True)+txt(56,558,'成功：written → DMB → tail',30,A,True)+txt(700,558,'失败：不释放、不重试',30,O,True)
    elif i==10:
        content+=txt(56,239,'W25Q64',30,P,True)+rect(56,270,1168,104,B)+rect(56,270,244,104,P)+txt(178,334,'256 B 头',30,W,True,'middle')+txt(630,334,'96 B 记录依次追加',30,P,True,'middle')
        content+=txt(56,420,'日志区 0…0x7FEFFF',30,P)+txt(56,476,'87336 条 × 1 ms = 87.336 s',30,A,True)+line(56,510,1224,510)+txt(56,572,'参数独占最后 4 KiB：0x7FF000 起',30,O,True)
    elif i==11:
        content+=txt(56,226,'前台批次',30,P,True)
        for j in range(8):content+=rect(56+j*146,252,134,70,B)+txt(123+j*146,299,'96 B',30,P,False,'middle')
        content+=txt(56,390,'运行：攒够 8 条；停车：允许短尾包',30,A,True)+line(56,424,1224,424)+txt(56,486,'BSP 先保证写范围已擦除',30,P,True)+txt(56,547,'再按 256 B 页边界拆写；跨扇区先擦后写',30)
    elif i==12:
        content+=rect(56,216,1168,82,B)+txt(84,269,b[0],30,O,True)+line(640,298,640,342,A,True)+txt(56,405,b[1],30,P,True)+txt(56,477,b[2],30)+line(56,510,1224,510)+txt(56,570,b[3],30,O,True)
    elif i==13:
        content+=rect(56,218,422,326,P)+txt(84,282,['完整记录','+ index','+ run_id','+ revision'],30,W,True,leading=57)+line(478,378,568,378,A,True)
        content+=txt(608,258,'上传独立 256 槽',30,A,True)+txt(608,335,['先 index++','满了只 dropped++','Flash tail 保持不变'],30,T,leading=61)+txt(56,607,'新运行分配非零 run_id；索引重新从 0 开始',30,P,True)
    elif i==14:
        for j,labels in enumerate([['run 7 / rev 3','index 10、11、12'],['run 7 / rev 4','index 13、14'],['run 7 / rev 4','index 17、18']]):
            x=56+j*400; content+=rect(x,234,368,170,B)+txt(x+24,286,labels,30,P,True,leading=60)
        content+=txt(56,470,'版本变化切包',30,A,True)+txt(800,470,'索引缺口切包',30,A,True)+txt(56,568,'连续同版本批次：通常每 10 ms，最多 10 条',30,T)
    elif i==16:
        for j,(x,w,title,desc) in enumerate([(56,326,'STM32',['记录与上传','SCB / SPI2 DMA']),(452,326,'ESP32',['SPI ↔ Wi-Fi','桥接契约']),(848,376,'电脑',['TCP 命令 / 导出','UDP 实时记录'])]):
            content+=txt(x,257,title,42,P,True)+line(x,281,x+w,281,A)+txt(x,340,desc,30,T,leading=56)
        content+=line(382,310,440,310,A,True)+line(778,310,836,310,A,True)+rect(56,479,1168,118,B)+txt(80,526,['网络部分为跨端契约','本仓 STM32 实现 ≠ 已验证三端实机闭环'],30,O,True)
    elif i==18:
        content+=rect(56,215,1168,100,P)+txt(84,277,'可靠业务：发送 → 等精确 ACK → 释放',30,W,True)
        content+=txt(56,390,'200 ms 重发；最多 3 次',30,P,True)+txt(700,390,'800 ms 无有效帧重握手',30,O,True)+line(56,433,1224,433)+txt(56,498,'等待 ACK 时仍可发送 TELEMETRY',30,A,True)+txt(56,565,'传输 ACK ≠ 参数已生效 ≠ 已保存 Flash',30,T)
    elif i in [19,20,21]:
        for j,s in enumerate(b):
            y=209+j*100;content+=txt(56,y+34,str(j+1),42,A,True)+line(115,y+22,170,y+22,A,True)+txt(202,y+34,wrap(s,32),30,P,leading=40)
    else:
        for j,s in enumerate(b):content+=txt(56,245+j*103,wrap(s,36),30,P,leading=40)
    if i!=1:body+=group('main',content)
    body+=group('page-number',txt(1224,682,f'{i:02d}',24,M,anchor='end'),'1160 650 64 40')
    defs='<defs><marker id="arrow" markerWidth="10" markerHeight="10" refX="8" refY="4" orient="auto" markerUnits="userSpaceOnUse"><path d="M 0 0 L 8 4 L 0 8 Z" fill="#007E87"/></marker></defs>'
    return f'<svg xmlns="http://www.w3.org/2000/svg" width="1280" height="720" viewBox="0 0 1280 720" font-family="{FONT}" data-pptx-page-role="{p["kind"]}">{defs}{body}</svg>'

def pages(start,end):
    (ROOT/'svg_output').mkdir(exist_ok=True)
    for i in range(start,end+1):
        p=slides[i-1]; (ROOT/'svg_output'/f'{i:02d}_slide.svg').write_text(render(i,p),encoding='utf-8')
        print(f'P{i:02d} modules: flat, topology; geometry: {p["title"]} → source-shaped diagram / ruled open field')

def notes():
    (ROOT/'notes').mkdir(exist_ok=True)
    text=''
    for i,p in enumerate(slides,1):
        final_svg=(ROOT/'svg_output'/f'{i:02d}_slide.svg').read_text(encoding='utf-8')
        assert escape(p['title']) in final_svg or i==1
        text+=f'# {i:02d}_slide\n\n{p["message"]}。\n\n{p["note"]}\n\n源码与契约：{p["source"]}。\n\n---\n\n'
    (ROOT/'notes'/'total.md').write_text(text,encoding='utf-8')

if __name__=='__main__':
    if sys.argv[1]=='plan':plan()
    elif sys.argv[1]=='pages':pages(int(sys.argv[2]),int(sys.argv[3]))
    elif sys.argv[1]=='notes':notes()
