# Phase 02：SCB/1协议 — 验证

Group 3本轮：V-7本端14条固定向量/独立偏移与逐位CRC/逐帧及文件SHA256/生产C可靠身份测试通过；新增6项、全回归145项通过，日志`.local/p2-g3-regression.log`。三端对齐未执行；真实传输适配和实机仍未完成。[Group 3报告](../../../docs/validation/phase-02-group-3.md)。

2026-10-18最新：Group 2已执行18项可靠层、16项会话/字段及139项全量回归，全部通过；本次正式目标build/size退出0。[本次报告](../../../docs/validation/phase-02-group-2.md)及[证据索引](../../../docs/validation/phase-02-group-2-evidence.json)区分实际范围。V-4纯状态机通过；V-5/V-6握手与G-02关闭动作通过（随机数测试注入、尚无生产RNG/驱动/socket适配）；V-9容量/坏帧不ACK、重试动作通过，真实socket/UDP计数仍待后续。下表未执行状态保留为Group 1交审时历史，不表示新增结果缺失。共同向量/三端/实机均未完成。

> Group 1本端软件检查已真实执行，见[本组报告](../../../docs/validation/phase-02-group-1.md)。Group 2/3、真实传输错误动作及实机仍未执行。

## 验证清单

| 编号 | 检查项 / 方法 | 可观测通过标准 | 关联需求 | 状态 |
|---|---|---|---|---|
| V-1 | 独立struct/zlib解析头与0/992B边界、CRC注入 | 头32B，所有字段偏移一致，CRC只覆盖指定范围，SPI填充不影响CRC | FR-1 | 通过：生产C与独立struct/zlib及0/992B边界，见本组报告 |
| V-2 | 全部类型、flags、record_count及payload关系；LOG_DATA保留位/边界 | 1/10条遥测为152/1016B总长；错数量、错长度、非零reserved和越界均拒绝 | FR-2 | 通过：全部类型/flags/数量/长度、遥测reserved、日志边界，见本组报告 |
| V-3 | 错JSON、嵌套/转义重复键、NaN/Infinity/溢出数值、非法UTF-8、尾随垃圾、截断与超长 | 全部拒绝且分派计数0；合法有限JSON完整消费 | FR-3 | 通过：生产C主机、1200组JSON差分及1000组数值比较；不是实机时序结论 |
| V-4 | 可控时钟199/200ms、丢ACK、错ACK、非可靠插队、可靠重复及近回绕 | 精确ACK才释放；首次后最多3重发且原字节/seq不变；重复只交付1次；超时重建，不跨回绕比较 | FR-4 | 未执行 |
| V-5 | SPI HELLO重复、旧会话、IDLE确认、已建立新HELLO、ESP32重启 | 随机session非零；握手重复复用；确认前未ready；旧帧不交付；按后续事务回复 | FR-5 | 未执行 |
| V-6 | TCP HELLO、SPI未ready/换会话、TCP重连 | HELLO_ACK准确报告session/link_ready且不重置SPI；旧会话不执行；换会话输出关闭TCP动作，重连用既有握手获取状态 | FR-6 | 未执行；G-02方案已确认，见[决定](g-02-session-change.md) |
| V-7 | 固定向量由独立解析/CRC复核并与三端核对 | 所有类别都有完整hex、字段/错误期望和哈希；独立方法一致；三端字节一致有记录 | FR-7 | 未执行；G-04待建立 |
| V-8 | 编译期偏移/大小断言、检查普通结构体裸发及实际SmartCar布局 | 本端全偏移断言通过；对端完整布局匹配，不静默转32B | TC-1 | 本端主机/目标断言及当前SmartCar原头文件24项偏移主机编译通过；STM32目标/能力/实机兼容仍待验 |
| V-9 | 坏SPI/TCP/UDP与满可靠槽注入 | SPI无ACK，TCP关闭，UDP计数丢弃；无容量则不接管；重试耗尽行为正确，无车辆停车动作 | TC-2 | 部分：坏帧无输出视图/编码失败不改输出通过；实际SPI不ACK/TCP关闭/UDP计数、满槽及重试未实现 |
| V-10 | ACK后延迟RECEIVED/APPLIED/SAVED；相同request_id同/异内容模拟 | ACK不生成生效/保存，结果原样透传；可靠重复不重复分发；请求缓存语义由模拟STM32体现 | TC-3 | 部分：request_id/ack/RECEIVED/APPLIED/SAVED内容保持原样；可靠去重/模拟STM32业务缓存及真实转发尚未实现 |

## 自动化验证（计划）

Group 3实施前增量：V-7分别执行确定性生成器`--check`、不依赖struct/zlib的独立校验器、生产C codec及可靠重复/请求身份测试；冻结完整hex、字段期望、拒绝错误码和SHA256。通过只证明本端候选，三端核对/实机仍待验。

Group 2将按[group-2-design](group-2-design.md)直接编译生产C执行可靠/会话验证；本条为实现前计划，尚无结果。

`tests/test_protocol.py`、`tests/test_reliable.py`、`tests/test_sessions.py`已实现并直接编译生产C；`tests/vectors/scb-v1.md`仍待Group 3。当前真实入口为`python -m unittest discover -s ESP32Bridge/tests -v`。Python独立参考采用struct/zlib及Decimal；不是生产编解码器自我往返。

## 人工步骤与回归

1. 审查共享协议字段表及向量输入，不得只看生成器的自证结果。
2. 保存每个失败向量的明确拒绝原因、计数和业务分派为零的证据。
3. 回归可靠ACK与业务结果分离；保持用户选定v6.0.0，但不改SCB/1版本。

G-02/G-04/G-05证据未取得时保留待确认/阻塞，不能通过删用例提高通过率。本次Group 1有独立主机及目标构建证据；G-02方案已确认并先提交c524638，关联动作仍未实现。G-04共同向量及G-05实机能力继续待验。
