# G-01：第二控制连接的 BUSY 编码

> 2026-11-07恢复：以下完整正文保存本阶段背景、需求、详细设计、决定、计划、文件/符号映射和验收，按原文日期与基线解释；原“待审/已确认/已完成”不是本次重新批准或验收。当前实现由[Docs](../../../Docs/index.md)说明；未决见[问题页](../../../Docs/known-issues.md)。前层约束：[mission](../../mission.md)、[techstack](../../techstack.md)、[roadmap](../../roadmap.md)、[code-style](../../code-style.md)、[AGENTS](../../../AGENTS.md)。阶段未显式关闭，实机待验保持；本次不启动05/06。

2026-10-18用户明确选择“采用推荐的立即 HELLO_ACK/BUSY 格式”。先独立纯规约提交，再实施；不代表三端已同步或联调通过。

## requirements

04/FR-2：已有控制连接时，第二连接accept后立即发送SCB/1 HELLO_ACK，不等待其HELLO。头type=0x02、flags=0、record_count=0、seq=0、ack_seq=0、request_id=0；session_id为生成拒绝帧时当前SPI会话（未建立为0）。payload_len与CRC按生产codec计算，TCP无SPI填充。

JSON为`{"protocol":1,"session_id":当前SPI会话,"link_ready":真实布尔值,"status":"BUSY"}`。这是明确批准的HELLO_ACK/BUSY扩展约定；三端对齐前不能声称公共实现一致。BUSY不伪造车辆业务结果、不挤掉第一连接、不创建第二订阅。

## plan

拒绝路径独占有界小缓冲和发送游标，处理短写/EAGAIN/断开；发送完成或有限期限后关闭第二连接。由网络任务处理，SPI不等待它；不用裸文本，不将AP关联数量代替TCP限制。具体期限在Phase 04内部设计中明确。

## validation

04/V-2、06/V-5：保留第一连接，第二连接不发送字节，独立解析HELLO_ACK字段、CRC及BUSY；验证真实link_ready=false/true、部分发送、超时关闭、第一连接/SPI持续推进。方案已确认，代码/测试尚未完成。
