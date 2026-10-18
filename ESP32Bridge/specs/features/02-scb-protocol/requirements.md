# Phase 02：SCB/1协议 — 需求

2026-10-18最新：Group 1已提交455f817，Group 2生产C状态机已实现并经本次139项回归/目标构建，待审查；实际SPI/socket接入与固定向量仍待后续，阶段未完成。

> Group 1已实施/验证待审；Group 2/3未实施。原规约基线5c15c49，用户本次已继续启动Phase 02～06；依据：[SCB/1完整契约](../../protocol/scb-v1.md) 第2～7节，任务书验收1。

## 功能需求

- **FR-1**：逐字段显式编解码32B头，校验magic/version/type/flags、payload_len≤992及帧边界；小端整数、IEEE754 binary32。CRC先头0～27再负载，等于zlib.crc32，不覆盖CRC字段和SPI填充；SPI发送尾部清零，TCP无填充。
- **FR-2**：按消息类型约束RELIABLE和record_count，IDLE空负载；TELEMETRY为1～10条且负载恰为24+96×条数，UDP总长≤1016。校验LOG_DATA的offset、length、reserved=0及实际长度，length≤960；保留request_id和不属于本端修改范围的字节。96B记录偏移及reserved=0按契约核对，不解释控制含义。
- **FR-3**：JSON按payload_len有界解析、无尾NUL，消费完整UTF-8文本；拒绝语法错误、重复键（含转义后相同键）、非法UTF-8、非有限数及超长/截断数据，不从坏帧分派部分内容。
- **FR-4**：SPI和TCP双向各维护独立seq/ack，UDP另有seq；普通seq从1单调增加，0用于握手。每方向一个可靠帧在途，精确ack_seq匹配才释放；200ms超时最多重发三次，原seq/内容不变，ACK可以用独立IDLE发送。重复可靠帧只确认、不重复分发，不因后续非可靠seq较大丢掉可靠重发；不跨回绕比较，接近回绕重建链路/连接。
- **FR-5**：SPI收到session=0/seq=0且role=stm32、protocol=1的HELLO，生成非零随机session并后续事务回复HELLO_ACK；握手重复HELLO复用该会话；收到新会话IDLE后才ready。已建立时新HELLO开启新握手，重启后旧会话不接管，等STM32重握手；重试耗尽撤销ready并进入重握手。
- **FR-6**：TCP接受role=pc、protocol=1的零会话/序号HELLO，返回当前session和link_ready，不重置SPI。未ready只提供连接状态，不转交控制请求。旧会话帧不能执行/导出；SPI会话改变后按用户确认的[G-02决定](g-02-session-change.md)关闭TCP，电脑重连并用既有HELLO/HELLO_ACK获取状态，不增加消息类型。
- **FR-7**：交付固定十六进制向量清单，涵盖IDLE、GET_STATUS、完整SET_PARAMS、1/10条记录、LOG_DATA、错CRC/长度、可靠重发和重复request_id。用独立CRC/解析方法交叉核验并留哈希、字段期望和错误原因；与三端核对前只能称本端候选向量。

## 技术约束

2026-10-18实施细化：FR-4～FR-6的内部接口、固定身份缓存与完成发送后起算计时见[group-2-design](group-2-design.md)；不新增外部字段。

- **TC-1**：禁止普通C结构体强转/裸发；对头长、96B布局和每项偏移作断言；Python独立参考格式`<4sBBHHHIIIII`长度32。对端实际头文件不匹配时停止真实联调，不转换为32B。
- **TC-2**：非法SPI丢弃且不ACK；非法TCP关闭；非法UDP计数丢弃。ACK必须在接管容量已预留后推进；重试耗尽SPI重握手、TCP关闭，均不得改变车辆停车条件。
- **TC-3**：帧ACK只证明合法接收接管，不产生APPLIED/SAVED；STM32负责32请求缓存及相同ID不同内容的INVALID_ARG，电脑负责请求编号和断线结果查询。本端透明传输，不把可靠去重当成业务request_id缓存替代。

## 接口与非目标

完整线格式、命令表、状态码、记录偏移和事件枚举仅以共享契约为准。内部编解码器输出合法帧/明确错误码，可靠层输入时钟、帧和容量许可，输出发送/重发/交付/重建动作；不执行参数校验、Flash操作或车辆控制。G-02/G-04见 [确认门禁](../../research/2026-10-17-baseline-and-decisions.md)。
