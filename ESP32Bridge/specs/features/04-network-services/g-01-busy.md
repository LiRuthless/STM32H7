# G-01：第二控制连接的 BUSY 编码

2026-11-05职责迁移：现行/目标契约见[F05 网络与单控制会话](../../../Docs/transport/network.md#f05)。本页保留原动机、候选、用户决定、提交及验证声明的冻结原文，不继续维护现行详细接口；原“待确认/已完成”等只对应记录时点，当前冲突见[问题页](../../../Docs/known-issues.md)。

<details>
<summary>原设计、决定及阶段验证（冻结原文，不作为现行规则）</summary>

~~~~~markdown
# G-01：第二控制连接的 BUSY 编码

2026-10-18用户明确选择“采用推荐的立即 HELLO_ACK/BUSY 格式”。先独立纯规约提交，再实施；不代表三端已同步或联调通过。

## requirements

04/FR-2：已有控制连接时，第二连接accept后立即发送SCB/1 HELLO_ACK，不等待其HELLO。头type=0x02、flags=0、record_count=0、seq=0、ack_seq=0、request_id=0；session_id为生成拒绝帧时当前SPI会话（未建立为0）。payload_len与CRC按生产codec计算，TCP无SPI填充。

JSON为`{"protocol":1,"session_id":当前SPI会话,"link_ready":真实布尔值,"status":"BUSY"}`。这是明确批准的HELLO_ACK/BUSY扩展约定；三端对齐前不能声称公共实现一致。BUSY不伪造车辆业务结果、不挤掉第一连接、不创建第二订阅。

## plan

拒绝路径独占有界小缓冲和发送游标，处理短写/EAGAIN/断开；发送完成或有限期限后关闭第二连接。由网络任务处理，SPI不等待它；不用裸文本，不将AP关联数量代替TCP限制。具体期限在Phase 04内部设计中明确。

## validation

04/V-2、06/V-5：保留第一连接，第二连接不发送字节，独立解析HELLO_ACK字段、CRC及BUSY；验证真实link_ready=false/true、部分发送、超时关闭、第一连接/SPI持续推进。方案已确认，代码/测试尚未完成。
~~~~~

</details>
