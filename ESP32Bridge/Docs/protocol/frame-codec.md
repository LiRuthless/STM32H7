# 帧编解码与本端检查矩阵

核对基线441a778，2026-11-07。生产依据：[protocol.c](../../main/protocol.c)的scb_encode/scb_decode/header_check/payload_check/frame_crc及[声明](../../main/protocol.h)；阶段依据 [Phase02需求](../../specs/features/02-scb-protocol/requirements.md)。公共含义见 [SCB/1](../../../SmartCar/Docs/services/communication.md#scb1)。

<a id="f02-frame"></a>
## F02 线表示与借用

纯编解码不操作DMA/socket/ACK/业务。普通结构体是字段值，不是线图像。头32B偏移为magic0、version4、type5、flags6、payload_len8、record_count10、session12、seq16、ack20、request24、CRC28；多字节显式小端，不要求整数对齐。CRC从UINT32_MAX开始，以反射多项式0xedb88320逐位累计头前28B及实际payload，最后取反，跳过CRC字段及SPI填充。

SPI编码清满1024B后写头及payload，尾部零；SPI解码忽略填充值。TCP/UDP恰为32+payload_len，无多帧或填充。TCP流拼帧属于网络层。decode先清有效view，首错返回不发布部分视图；成功头是副本、payload借用原输入，读完前输入不可释放/改变。encode先清有效used，复制header，验证头/容量/payload范围及别名/内容后才写输出；范围溢出或payload与输出重叠为ALIAS，控制对象也不得非法别名。失败used=0，不可发送旧输出。

<a id="f02-validation"></a>
## 检查顺序与失败

| 层/类型 | 本端条件 | SCB_错误 |
|---|---|---|
| 参数 | 有效view/used与输入/输出；t仅SPI/TCP/UDP | ARGUMENT/TRANSPORT |
| 头 | 至少32B、magic SCB1、version1 | LENGTH/MAGIC/VERSION |
| 通用头 | payload≤992；已知8种type | LENGTH/TYPE |
| flags | COMMAND/RESULT/EVENT/LOG_DATA恰为1，其余恰为0 | FLAGS |
| UDP | 仅TELEMETRY | TRANSPORT |
| TELEMETRY头 | record_count 1～10，payload_len=24+96×N | RECORD_COUNT/LENGTH |
| 其他头 | record_count=0；IDLE空负载 | RECORD_COUNT/PAYLOAD |
| 完整传输 | SPI n=1024，TCP/UDP n=32+payload_len | LENGTH |
| CRC | 等于frame_crc结果 | CRC |
| 五种JSON类型 | HELLO/HELLO_ACK/COMMAND/RESULT/EVENT全量JSON合法 | JSON |
| TELEMETRY内容 | payload+20的u32非零；+16的u32等于首记录tick（+24） | PAYLOAD |
| 每个96B记录 | 偏移82～95保留区全零 | PAYLOAD |
| LOG_DATA | payload至少9B，+4的u16 length在1～960，payload_len=8+length | LENGTH |
| LOG_DATA内容 | +6的u16保留零，+0的u32 offset≤UINT32_MAX-length | PAYLOAD |

decode依序参数→最小头→magic→version→header_check→实际长度→CRC→payload_check；多错取首错。encode共享头/负载校验，另有capacity检查，不对已有图像解CRC。JSON错误详见 [JSON](json-validation.md#f02-json)。codec仅保存身份字段，不检查会话/角色/业务幂等；HELLO全零身份、role和protocol由 [会话层](reliable-and-sessions.md#f03-spi-session)验证，不能把公共协议全要求称为codec检查。

## 参数主表与边界

| 来源/符号 | 当前值及条件 | owner、生效/复位 |
|---|---|---|
| protocol.h SCB_HEADER_BYTES/SCB_PAYLOAD_BYTES | 32/992 B，全部传输 | 编译固定，任务逐帧校验 |
| SCB_SPI_BYTES | 1024 B，仅SPI | encode每次清全图像 |
| SCB_TELEMETRY_PREFIX/SCB_RECORD_BYTES | 24/96 B，N=1～10，UDP最大1016B | 编译固定，无运行期覆盖 |
| SCB_RELIABLE | uint16 flags=1，四种可靠类型 | 编译固定，其他类型必须零 |
| payload_check LOG_DATA | 1～960B，offset加length不溢出u32 | 每帧检查，不验证真实日志总长 |

尺寸、偏移和binary32有编译期断言，不代表资源实测。历史 [P02 G1](../validation/phase-02-group-1.md)、[G3](../validation/phase-02-group-3.md)及 [候选向量](../../tests/vectors/scb-v1.md)保持原字节；本次未重跑，三端一致仍待验。
