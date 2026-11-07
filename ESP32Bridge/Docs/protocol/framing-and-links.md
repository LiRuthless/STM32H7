# SCB 帧与链路

2026-11-07现行导航；F02/F03身份及旧入口保留。公共线格式见 [SCB/1](../../../SmartCar/Docs/services/communication.md#scb1)，[原协议提取](../../specs/protocol/scb-v1.md)按历史基线保留。需求见 [traceability](../traceability.md)。详细机制由以下主题唯一维护。

<a id="f02"></a>
## F02 显式帧编解码与有界 JSON

### 用途、状态及接口

本端已实现纯编解码及有界JSON，不执行业务。输入显式字节范围，输出成功图像或借用视图；失败无部分交付。

- [帧/CRC、视图生命周期及参数](frame-codec.md#f02-frame)
- [逐类型检查、长度和失败矩阵](frame-codec.md#f02-validation)
- [JSON容器栈、UTF-8、重复键及数字语法](json-validation.md#f02-json)
- [精确u32转换](json-validation.md#f02-u32)

### 实现与状态归属

protocol.c/.h归帧主题，json_guard.c/.h归JSON主题；各调用任务持工作区和输入，无共享解析状态。具体声明及调用路径见对应主题。

### 验证与证据

历史 [P02 G1](../validation/phase-02-group-1.md)、[G3](../validation/phase-02-group-3.md)和 [候选向量](../../tests/vectors/scb-v1.md)保持；本次未重跑，三端对齐未验。

<a id="f03"></a>
## F03 独立可靠窗口与会话

### 状态、输入和输出

已由SPI/NET分别接入；[主说明](reliable-and-sessions.md#f03)解释各任务独占状态和动作输出，不等于完整生产桥接已接通。

### 正常与异常流程

- [窗口、预留后接管、精确ACK及重复帧](reliable-and-sessions.md#f03-window)
- [完整发送计时、重发及耗尽](reliable-and-sessions.md#f03-retry)
- [SPI握手](reliable-and-sessions.md#f03-spi-session)
- [TCP握手、状态连接及会话变化](reliable-and-sessions.md#f03-tcp-session)

### 实现与验证

reliable_link.c/.h及link_session.c/.h的行为、参数和owner由主说明维护。历史 [P02 G2](../validation/phase-02-group-2.md)，真实跨端互操作仍待验。

## 本轮存量整改追溯

2026-11-05：[Phase01～04格式与注释整改契约](../../specs/research/2026-11-05-source-style-remediation-draft.md)；47文件静态整改与人工复核已完成，目标构建未验证，见 [本轮报告](../validation/2026-11-05-source-style-review.md)。本次文档拆分不改上述结果，不关闭阶段或硬件待验。
