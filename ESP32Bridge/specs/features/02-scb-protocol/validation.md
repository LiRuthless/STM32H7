# Phase 02：SCB/1协议 — 验证

> 2026-11-07验收说明：下方原执行记录保留各自日期、版本与计数，不是本次重跑。当前47文件静态整改和187项宿主历史结果、目标构建S5未验证分别见[最终复核](../../../Docs/validation/2026-11-05-source-style-review.md)。本次仅核对文档，未运行固件/宿主/硬件验收；阶段不补勾关闭。


## 当前验收边界（2026-11-05文档核对）

本端协议已实现；候选向量三端未核对。本轮未构建、未运行行为回归、未烧录；仅执行文档迁移检查，不覆盖下方原运行记录。2026-11-05工程导入前无法在本仓验证原提交顺序；其后35e5627已完成工程导入，缺本地产物的历史结果不作当前重验。

## 可复用方法导航

| 需求 | 方法主入口 |
|---|---|
| [FR-1](requirements.md#fr-01) | [F02 显式帧编解码与有界JSON](../../../Docs/protocol/framing-and-links.md#f02) |
| [FR-2](requirements.md#fr-02) | [F02 显式帧编解码与有界JSON](../../../Docs/protocol/framing-and-links.md#f02) |
| [FR-3](requirements.md#fr-03) | [F02 显式帧编解码与有界JSON](../../../Docs/protocol/framing-and-links.md#f02) |
| [FR-4](requirements.md#fr-04) | [F03 独立可靠窗口与会话](../../../Docs/protocol/framing-and-links.md#f03) |
| [FR-5](requirements.md#fr-05) | [F03 独立可靠窗口与会话](../../../Docs/protocol/framing-and-links.md#f03) |
| [FR-6](requirements.md#fr-06) | [F03 独立可靠窗口与会话](../../../Docs/protocol/framing-and-links.md#f03) |
| [FR-7](requirements.md#fr-07) | [F10 工具、验证与交付](../../../Docs/guides/validation.md#f10) |
| [TC-1](requirements.md#tc-01) | [F02 显式帧编解码与有界JSON](../../../Docs/protocol/framing-and-links.md#f02) |
| [TC-2](requirements.md#tc-02) | [F03 独立可靠窗口与会话](../../../Docs/protocol/framing-and-links.md#f03) |
| [TC-3](requirements.md#tc-03) | [F06 可靠接管与请求信用](../../../Docs/bridge/forwarding-and-lifecycle.md#f06) |

## 原阶段标准与历史执行记录

以下保留原日期、计数和逐项标准/执行事实。旧“当前/本轮”、旧路径、已被替代的审查节奏只对应原记录时点；不作为本次执行声明。现行政策见[AGENTS](../../../AGENTS.md)，现行契约以Docs入口为准。


Group 3本轮：V-7本端14条固定向量/独立偏移与逐位CRC/逐帧及文件SHA256/生产C可靠身份测试通过；新增6项、全回归145项通过，日志`.local/p2-g3-regression.log`。三端对齐未执行；真实传输适配和实机仍未完成。[Group 3报告](../../../Docs/validation/phase-02-group-3.md)。

2026-10-18最新：Group 2已执行18项可靠层、16项会话/字段及139项全量回归，全部通过；本次正式目标build/size退出0。[本次报告](../../../Docs/validation/phase-02-group-2.md)及[证据索引](../../../Docs/validation/phase-02-group-2-evidence.json)区分实际范围。V-4纯状态机通过；V-5/V-6握手与G-02关闭动作通过（随机数测试注入、尚无生产RNG/驱动/socket适配）；V-9容量/坏帧不ACK、重试动作通过，真实socket/UDP计数仍待后续。下表未执行状态保留为Group 1交审时历史，不表示新增结果缺失。共同向量/三端/实机均未完成。

> Group 1本端软件检查已真实执行，见[本组报告](../../../Docs/validation/phase-02-group-1.md)。Group 2/3、真实传输错误动作及实机仍未执行。

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

候选文件换行修复计划：验证git check-attr输出text=set/eol=lf，在临时文件中以Git checkout过滤规则取得当前索引字节，核对其无CR且SHA256等于冻结基线。固定帧字节、原始向量与三端待验状态不变。

Group 3实施前增量：V-7分别执行确定性生成器`--check`、不依赖struct/zlib的独立校验器、生产C codec及可靠重复/请求身份测试；冻结完整hex、字段期望、拒绝错误码和SHA256。通过只证明本端候选，三端核对/实机仍待验。

Group 2将按[group-2-design](group-2-design.md)直接编译生产C执行可靠/会话验证；本条为实现前计划，尚无结果。

`tests/test_protocol.py`、`tests/test_reliable.py`、`tests/test_sessions.py`已实现并直接编译生产C；`tests/vectors/scb-v1.md`仍待Group 3。当前真实入口为`python -m unittest discover -s ESP32Bridge/tests -v`。Python独立参考采用struct/zlib及Decimal；不是生产编解码器自我往返。

## 人工步骤与回归

1. 审查共享协议字段表及向量输入，不得只看生成器的自证结果。
2. 保存每个失败向量的明确拒绝原因、计数和业务分派为零的证据。
3. 回归可靠ACK与业务结果分离；保持用户选定v6.0.0，但不改SCB/1版本。

G-02/G-04/G-05证据未取得时保留待确认/阻塞，不能通过删用例提高通过率。本次Group 1有独立主机及目标构建证据；G-02方案已确认并先提交c524638，关联动作仍未实现。G-04共同向量及G-05实机能力继续待验。

## 2026-11-05 本轮整改验收（G2 协议与会话）

[本轮整改契约](../../research/2026-11-05-source-style-remediation-draft.md)定义S1～S7。实施前准备时点（2026-11-05，以下执行结果之前）仅清点/规约准备，未整改、未做全量人工复核、未运行行为测试或目标构建，不以历史计数代替本轮结果。环境预检见[本轮记录](../../../Docs/validation/2026-11-05-source-style-preflight.md)；现有固定SDK及默认工具路径缺失，目标构建未验证。实施后回填各项实际结果，硬件项保留。

### 本轮G2执行结果

47文件按所属组完成静态整改；本阶段组的格式/命名、注释覆盖/格式/准确性及上下文具体结论见[人工复核报告](../../../Docs/validation/2026-11-05-source-style-review.md)。所选9个生产相关宿主模块共187项最终通过，0失败/跳过；11个生产C的同配置优化宿主IR与基线一致，47文件规范化词法序列一致。固定SDK入口实际失败（D盘不存在），目标构建S5未验证；不替代实机项，不改变原阶段未整体验收状态。

## 2026-11-07 G4 检查简化增量

详见[检查复用设计](check-simplification.md)。复用FR-1～6、TC-1～3；保持外部协议及资源/并发行为，生产路径复用验证结果。用户已授权执行第一批；第二批与公共契约冲突，不放宽检查。静态审查、当前配置目标构建及实机待验分别记录，不运行宿主或模拟测试。

### G4实际交付（2026-11-07）

实现与Docs同步完成，静态审查及正式v6.0.0当前配置构建通过，最终run b5bc560ef329489d8bc93ddf7f4e30ff零错误零警告；源码哈希与manifest一致。SPI可配置peek_tx边界保留完整编码，EVENT_GAP首次验证后重封装。未执行宿主/模拟或实机验证，不关闭阶段。具体owner、顺序、保留边界及环境证据见[本组审查](../../../Docs/validation/2026-11-07-check-simplification.md)。

## 2026-11-07 第一、二组当前决定

用户明确授权减少桥接内容检查并统一单请求槽；本增量优先于本文旧三类信用、独立查询通路和桥接全局有限数/保留区检查要求。完整输入输出、状态/失败、跨端映射与验证见[单请求与检查简化设计](../05-bridge-lifecycle/single-request-design.md)。第三组不实施，实机/阶段未验保持。

### 第一、二组实际结果（2026-11-07）

代码与Docs已同步；正式v6.0.0构建e7a2d485bb2742f799a7b0e3a515c1c2通过，零错误零警告；电脑端语法编译通过，无模块执行。静态核对单信用/双RESULT、最终ACK、普通请求不被导出期限撤销、全局墓碑和三端消费职责，见[实际报告](../../../Docs/validation/2026-11-07-group12-simplification.md)。未执行宿主/模拟或实机测试，阶段与第三组状态保持。
