# G5 记录、日志后端与协议编解码复核

日期：2026-10-24。产品基线`7eefb95`；先行契约`42d11b0`中的
`g5-recording-protocol.md`。本记录只覆盖G5实施，不将AC5、原始外部样本、实机预算或旧阶段
待验项目标为完成。原Hardware和父任务G6的其他在途文件不在本组修改清单内。

## 唯一状态与边界

| owner | 本次实际状态和跨模块出口 | 复核依据 |
|---|---|---|
| run_record | 唯一`s_snapshot`96B和`s_header`256B；前台构造头，TIM6发布快照，TIM15只在logger门禁后采样；头和记录全部sizeof/offsetof/容量断言仍在产品编译 | `run_record.c`断言、Publish/Capture、真实历史字节对照 |
| logger | `s_buf`98304B、`s_batch`768B、volatile head/tail、status、header_valid/write_failed均只在此定义；1024槽实际可用1023，写成功前不释放批次 | 原datalog_v2队满/写时ISR/八条批次/不重试、G5结构检查 |
| log_store | 唯一App日志BSP_Flash_LogErase/Write/Read访问点；仅`s_check`256B回读临时对象，不拥有会话 | G5真实写→读→比较短路及故障注入 |
| log_export | 唯一968B块缓存与身份/租期/进度；数据始终写缓存+8，前8B不编码；CSV原256/96/640B临时对象仍static | raw地址/保留8B/失败不发布/部分读取、最大前缀和完整CSV比较 |
| checksum | CRC32唯一公式，无共享可写状态；frame薄转发，export直接调用通用函数 | 对G4公式逐token一致、原Python zlib断言 |
| scb_frame/json/codec | 帧只处理字节与字段，JSON持8层显式共享栈，codec唯一4080B命令JSON工作区；无新堆或第二个大块缓存 | 608项原协议断言、27个原函数体逐token一致、符号大小 |
| command_dispatch/link | dispatch使用原992B目标立即复制raw并补8B头，显式switch映射独立日志结果；link原函数体、请求/事件/可靠窗口保持 | 原真实LOG_READ缓存与查询、原link门禁、逐token比较 |

上传入口仍在`RunRecord_CaptureSample`完成记录后调用`Telemetry_PushRecord`，早于Flash槽/head
发布。telemetry仍拥有独立上传队列，满队丢上传不修改Flash消费位置。日志核心的C/H及传递
闭包不依赖SCB类型/格式；只有通信适配层将日志结果映射为SCB，未使用枚举数值强转。

## 逐函数内容清单

以下每个列出函数均核对其C定义前的brief/details、形参方向、真实返回、共享状态及调用上下文；
H只保留用途/约束/真实C定义定位。覆盖与格式由108定义清单补查，人工内容和行为依据分别如下，
不能用清单数量或自动脚本替代内容结论。

| 文件与函数 | 人工核对的具体内容 | 行为证据 |
|---|---|---|
| run_record：Datalog_CalculateCrc16 | 初值ffff，逐字节高8位异或、8次1021反馈，16位截断，覆盖头8..255 | 原CRC/历史256B头逐字节 |
| Datalog_SetProfile | 显式bank四项依次复制，reserved清零，不读元素覆写的活动方向值 | 实际历史头比较 |
| RunRecord_ResetSnapshot、CheckCalibration、GetHeader | 分别单次memset、Gray_CheckCalibration(0)转发、静态只读头借用；不清导出身份或把内存头当有效会话 | Init/Start原错误路径与边界 |
| RunRecord_BuildHeader | 先清256B，再按原顺序读取模式/档位、黑白表、weights、两bank、活动PI、风扇、固定预算，最后CRC | 历史头全部字节 |
| Datalog_PublishControl | 先取Track/Gray/Tracking指针再清快照，adc8B和gray32B，后续字段按原点读取，最后reason读取与DMB | 原控制/上传与历史记录 |
| RunRecord_CaptureSample | 门禁后复制快照→tick→EncL→EncR→GyroRaw→三轴/14.3截断；完整上传复制发生在Flash发布前 | datalog原断言/ISR注入历史记录 |
| RunRecord_IsHeaderValid | 原完整短路字段、CRC、标定顺序；未新增预算/方向/保留区校验，不接受空内部指针 | 原CSV无效头/读错 |
| logger：Logger_LatchFault | PRIMASK内仅锁首fault；本次传入非READ即清active，即使首fault已存在；不写reason或隐式停车 | 原首故障和新partial-read留态 |
| Datalog_RequestFaultStop、SetStopSummary | 先锁fault再保护再App_RequestStop；summary独立临界区首reason/累计flags，重复不覆盖 | 原日志故障/保护首因 |
| Datalog_GetStatus、IsActive、GetCount、Logger_GetStopReason、HasHeader | GetStatus会更新pending后返回非原子静态视图；其余各单次原字段读取，不扫描Flash | 原状态查询与边界 |
| Datalog_Init、Start、Clear、GetMeta | Init保留header字节/导出历史；Start先原tracking读取及export互斥，依次配置/标定/装配/擦/写回读，成功才有效；Clear不新建会话；meta在原锁内只取成功前缀 | 原配置/未标定/Flash失败/互斥、历史字节 |
| Datalog_Push | active/fault→容量→队满门禁；队满只首次rejected++，采样/上传后槽→DMB→head→accepted，恰达容量停车 | 原1023队满/87336容量/独立上传 |
| Datalog_Service | 固定tail，valid/write_failed门禁，运行不足8不写；复制最多8条不释放，成功written→DMB→tail；失败不重试 | 原Flash写时ISR注入/失败尾包 |
| Datalog_Flush、Stop | Flush单次转发；Stop先active=0，再最多128次服务且每次随后喂狗，不无界drain | 原停止尾包与不重试 |
| log_store：Erase、Write、Read | 仅BSP一一转发，可阻塞、不重试/缓存/喂狗；读失败可能已部分写入目标 | 原Flash门禁、新部分读取 |
| LogStore_WriteHeader | 写256B成功才读256B，读成功才memcmp；仅临时static校验副本，不改logger有效位 | 新真实W/WR/比较次数断言 |
| log_export：Datalog_CancelExport、ServiceExport、IsExporting、GetExportStatus | cancel只清active和last_length；无符号10秒差可回绕；读取状态先服务租期，查询/重复块不续租 | 原超时/回绕/取消及新raw断言 |
| Datalog_BeginExport | 原停车短路→active→已有导出→有效头；失败保留输出和身份，成功跳过ID0，冻结written两次原读取点 | 原BUSY/NOT_FOUND/前缀/最大容量 |
| Datalog_ReadExport | 仅length先置0；失败data不写；最近同块直接借出+8，无Flash/CRC/lease变化；新读前后喂狗，失败锁READ并cancel，不回滚已写缓存 | 新指针/len、重复、partial-read/首fault及原wire |
| Datalog_EndExport、Dump | END未到总长BUSY且不写total/crc；CSV先Stop再读头，有效位不能跳过读动作，全部列/错误/summary/EOF与喂狗位置保留；静态缓冲避免占用运行栈 | 最大导出/Python CRC、完整历史CSV |
| checksum：Checksum_UpdateCrc32 | 入/出xor ffffffff、反射edb88320；previous初次0，空长可空数据且返回previous | G4公式token、原独立zlib |
| frame：SCB_ReadU16/ReadU32/WriteU16/WriteU32 | 字节先扩展再按0/8/16/24位移，不要求对齐；固定小端读写，getter/setter不访问额外字节 | 原帧黄金字节、原体token |
| SCB_CalculateCrc、BuildFrame | CRC仅薄转发；Build保留类型、可靠位、seq/session/request及telemetry长度检查顺序 | 608项坏帧/CRC/字段 |
| SCB_EncodeFrame、DecodeFrame | 编码先完整验证再清1024B；解码在局部描述全部通过才写frame，借用input负载，失败输出不变 | 黄金帧/坏帧原断言及token |
| json：SCB_ValidateUtf8、SkipSpace、ReadHex、AppendUtf8 | 最短编码/非NUL/代理/Unicode上界；四种空白；4位hex失败可能前移；池预检后编码 | 原UTF8/转义/边界断言 |
| SCB_ParseString、ParseNumber、ParseValue | 失败可有部分令牌/池；数字保留63B文法和十进制累加/逐次缩放；只分配单令牌，子容器由显式栈推进 | 原数字/重复解码键/深度/token断言 |
| SCB_ParseJson、FindMember、GetString、GetNumber | 输入前置失败保留工作区，解析中失败不可继续读取；共享8层栈不可重入；只查直接对象；getter越界/类型失败输出保持 | 608项JSON及原体token |
| SCB_JsonBegin、JsonAppend、JsonU32、JsonI32、JsonFloat | Begin空/零/超限只锁failed；Append整片段预检不NUL终止；INT32_MIN无符号幅值，float9位科学计数法/负零/非有限保持 | 原writer守卫/浮点文本 |
| codec：SCB_StatusName、JsonValues、ParseCommand、ValidateValues | 固定10拼写/八字段顺序；入口清command并写INVALID_ARG；语法错不可ACK，业务错保留部分字段；真实pure validator逐字段转换且double范围仍在窄化前检查 | 原608项/真实tuning_values链接 |
| SCB_EncodeLogData | 写LE32 offset、LE16 length、LE16零，再复制原字节；目标调用方至少968B，不与源重叠，不产生静态缓存 | 新960B极值guard/原dispatch wire |
| dispatch：MapLogResult、LogCommand | 独立六结果显式映射，未知IO_ERROR；成功raw同前台调用立即编码既有s_json并入原缓存，失败仍RESULT | 原真实LOG_READ缓存/查询全断言 |

dispatch其余逐函数复核清单：MapResult、ToWire、ToBusiness、Begin、Send、Position、ReadBank、
QueryParams、QueryStatus、QueryResult、QueryLogMeta、HandleCommand、Set、Switch、ResetSession、
Init、InitLink、ServiceLink、Service、QueueTuningEvent、PublishRunStarted、PublishRunStopped。
它们保留G4事务/查询/事件职责与顺序，本组只补ReadBank/Set的实际说明及新协议头依赖。
原G4边界、HMI、事务、旧包装和runtime_tuning均通过；没有重设请求表、发布槽或JSON工作区。

link逐函数复核清单：FindRequest、AllocateRequest、SetHandlers、ResetSession、Init、QueueEvent、
TakeEvent、FormatEvent、QueueResult、GetRequestResult、AcceptRequest、HandleAck、ReceiveFrame、
StartReliable、PrepareFrame、Service、GetStatus（均为SCB前缀）。这些17个函数体逐token对G4
一致，只有实际拆分头依赖改变；32请求预留、窗口1、ACK/重发/去重/事件gap均跑原真实门禁。

省略details的函数只属于只读getter、单调用转发：Datalog_IsActive/GetCount/Flush、
Logger_GetStopReason/HasHeader、RunRecord_ResetSnapshot/CheckCalibration/GetHeader、
LogStore_Erase/Write/Read、SCB_CalculateCrc/StatusName/GetString/GetNumber/
GetRequestResult/GetStatus。当前接口的空值规则写在实际C定义，不以H摘要替代。

## 实际验证与资源

- 原scb_protocol608项、scb_link、spi_wifi_state、control_equivalence、datalog_v2、runtime_tuning、
  spi_log_export、g2_system、g3_control、g4_boundaries、g4_transactions、g4_hmi和
  g4_legacy_boundary全部通过。独立FSM、参数/key、完整App（原未标定/宿主有效标定）通过。
- 完整dual_mode_tracking入口仍在原`C:/Users/Li/Desktop/LQ_Track.c`缺失处失败。
  原SHA256断言未删减，之前真实gray子项仍执行；不能把独立子项说成完整入口通过。
- 新g5_boundaries真实包含四个日志owner、实际BSP Flash及codec：raw失败指针保持/len先0、
  最近块不读Flash/不喂狗/不推进CRC或租期、同一缓存新读覆盖、partial读取失败取消但保留旧
  WRITE首fault和CRC/offset/lease、codec968B上下guard、写头W/WR/比较短路全部通过。
- 新历史对照独立编译真实`7eefb95`和当前C，同输入下256B头、12条96B记录及完整CSV字节完全
  相同；包含Flash写期间ISR入队、原因变更、完整序号回绕与多个快照。未复制当前算法生成期待。
- app_layout_check确认95个C/H、45个App C由Keil各编译一次、五类GCC目录均覆盖。
  父任务的G6资源helper单独管理；本组仅更新Phase16源清单，不覆盖历史phase16.json。
- LCD=0/1 GCC均0警告0错误，text/data/bss分别91432/308/231688、93996/312/231704。
  相对G4代码区均减少224B，data/bss不变。构建finally逐字节恢复原app_config、ELF、HEX和BIN，
  before/restored SHA256完全相同，受跟踪配置和ELF无差异。
- 保存的真实LCD0 ELF符号确认缓冲各一份：snapshot96、header256、buf98304、batch768、
  export_block968、check256、dump_header256、dump_record96、dump_line640、command_json4080、
  json_phases8、json_containers16字节。无新堆分配，无整份日志RAM副本。
- 锁定ARM编译器-fstack-usage对G4/G5相关模块分别65/80个编译函数帧，最大均200B。
  Datalog_Start32→24B（新增BuildHeader16B、WriteHeader8B），Push120→112B（Capture16B），
  Service32→32B，Dump112→104B，ReadExport32→32B，ParseJson56→56B，ParseCommand72→80B。
  这些是单函数编译帧，不是链路峰值或实测高水位；最终全App/中断叠加预算由G6独立验收。

CS-01～15均适用本组自维护范围。新增C/H为UTF-8无BOM，旧文件换行保持，4空格/独立大括号，
无Tab/行尾空白；普通行≤100，原完整协议/CSV字符串属于既定例外。108个C定义的标签、返回、
文档定位及逐函数清单已查，无占位模板。仅注释/格式补充前后非注释token逐项相同。
删除datalog.c/scb_protocol.c/h后所有实际产品/当前夹具引用转向真实owner，无兼容别名宏。
R08已由父任务按7eefb952d96c2bb1e6368afd7029c003abb8455b及最终暂存区绑定并通过：23个变更C/H、160份当前/历史复核记录，实际源码与依赖摘要一致；不以该检查替代上文人工内容审查。

日志、原始before/after字节备份、函数清单及局部帧统一位于`MDK-GCC/build/phase17-g5-*`，不自动
纳入提交。AC5、Flash/SPI实机吞吐、700/900us测量、长时录制和实车均未执行，保持待验。
