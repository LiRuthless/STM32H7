# Phase 02：SCB/1协议 — 实现计划

> 2026-11-05现行导航；本端协议已实现；候选向量三端未核对。本次整理不启动实现或补勾任务。

## 目标、依据与依赖

依据[编号需求](requirements.md)和[roadmap](../../roadmap.md)。阶段01提供工程基础，02提供协议，03/04提供SPI/网络，05连接两侧，06验证交付；原阶段前置和决定保留在冻结记录。

## 任务分组

### Group 1：编解码与验证

- [x] 显式头编解码、CRC、类型与二进制边界检查（FR-1、FR-2、TC-1）。
- [x] 有界JSON校验和独立错误路径（FR-3、TC-2）。

### Group 2：传输状态

内部所有权、发送完成计时与测试细化见[group-2-design](group-2-design.md)，先纯规约提交，不改变外部契约。

- [x] 独立可靠窗口、seq/ack、固定重复帧身份、完成发送计时、200ms及三次重发生产C实现/主机验证（FR-4）；代码待审提交，实际适配待03～05。
- [x] SPI/TCP握手、旧会话拒绝及G-02立即失效/关闭待办状态机（FR-5、FR-6、TC-3）；真实驱动/socket动作待后续阶段。

### Group 3：共同向量

2026-10-18换行修复先行：本仓库core.autocrlf=true，当前固定JSON未设换行属性；新增ESP32Bridge/.gitattributes，仅锁定tests/vectors/scb-v1.json为text eol=lf，保证重新检出后固定文件SHA256及--check不变。增加Git属性/检出字节回归，不改帧/字段或既有候选字节。

2026-10-18实施细化（先纯规约提交）：冻结`tests/vectors/scb-v1.json`的固定hex/语义字段/预期codec错误与逐帧SHA256；`tests/vectors/scb-v1.md`列文件SHA256和三端核对表。`scripts/generate_vectors.py`用struct/zlib确定性生成，`scripts/check_vectors.py`独立逐字节解析与逐位CRC核验（不导入生产codec或生成器）；`tests/test_vectors.py`同时调用生产C codec/可靠接口验证。相同request_id不同内容只验证透传/可交付，不声称ESP32执行STM32业务缓存。生成器`--check`仅比较冻结基线，不静默覆盖。三端核对未执行，始终称本端候选向量。

- [x] 本端14条候选固定字节、字段期望及SHA256；独立生成/逐位CRC核验及生产C测试通过（FR-7本端）；G-04三端对齐未执行。
- [x] 按 [验证](validation.md) 区分本端与三端状态；[Group 3交审](../../../docs/validation/phase-02-group-3.md)，本组待审提交。

Group 1→2→3；G-02于2026-10-18已获用户确认，先独立提交本决定，再实施对应动作；保留分组审查和G-01/G-03门禁。

## 契约、顺序与回退

- [F02 显式帧编解码与有界JSON](../../../Docs/protocol/framing-and-links.md#f02)
- [F03 独立可靠窗口与会话](../../../Docs/protocol/framing-and-links.md#f03)
- [F05 网络与单控制会话](../../../Docs/transport/network.md#f05)
- [F10 工具、验证与交付](../../../Docs/guides/validation.md#f10)
- [F04 SPI事务与协议适配](../../../Docs/transport/spi.md#f04)
- [F06 可靠接管与请求信用](../../../Docs/bridge/forwarding-and-lifecycle.md#f06)

按Group 1→2→3及原阶段依赖推进；每批实现前先独立提交Phase与Docs目标契约。详细符号、状态、资源和方法不在本计划重复。风险涉及协议、资源或兼容性时先核实决定，回退以本批独立实现提交为单位，不改历史证据。阶段内连续实施，阶段末统一交审，05通过审查后才进入06。

## 未决与验收

本次未新增设计决定；跨端向量、硬件及缺失证据见问题入口。[问题入口](../../../Docs/known-issues.md)，[阶段验证](validation.md)。
## 2026-11-05 整改任务

- [ ] 按[本轮整改契约](../../research/2026-11-05-source-style-remediation-draft.md)的G2 协议与会话逐文件整改及审查；G1→G2→G3→G4连续推进，共用替身不重复记完成。
- [ ] 执行S1～S4格式、注释及上下文审查，S5当前构建/环境记录，S6交付前人工复核，S7文档回填。

基线35e5627已经独立保存；本批纯规约提交完成后才修改实现。无法证明行为保持的部分暂缓，发现缺陷只登记不修复。

<details>
<summary>原计划、任务及风险记录（冻结原文，不作为现行规则）</summary>

~~~~~markdown
# Phase 02：SCB/1协议 — 实现计划

## 目标与依据

将 [共享契约](../../protocol/scb-v1.md) 实现为独立可测的编解码、可靠层和握手状态机。前置阶段01；依据 [需求](requirements.md)。共同向量尚未给出固定字节，不能宣称已与对端一致。

## 任务分组与顺序

### Group 1：编解码与验证

- [x] 显式头编解码、CRC、类型与二进制边界检查（FR-1、FR-2、TC-1）。
- [x] 有界JSON校验和独立错误路径（FR-3、TC-2）。

### Group 2：传输状态

内部所有权、发送完成计时与测试细化见[group-2-design](group-2-design.md)，先纯规约提交，不改变外部契约。

- [x] 独立可靠窗口、seq/ack、固定重复帧身份、完成发送计时、200ms及三次重发生产C实现/主机验证（FR-4）；代码待审提交，实际适配待03～05。
- [x] SPI/TCP握手、旧会话拒绝及G-02立即失效/关闭待办状态机（FR-5、FR-6、TC-3）；真实驱动/socket动作待后续阶段。

### Group 3：共同向量

2026-10-18换行修复先行：本仓库core.autocrlf=true，当前固定JSON未设换行属性；新增ESP32Bridge/.gitattributes，仅锁定tests/vectors/scb-v1.json为text eol=lf，保证重新检出后固定文件SHA256及--check不变。增加Git属性/检出字节回归，不改帧/字段或既有候选字节。

2026-10-18实施细化（先纯规约提交）：冻结`tests/vectors/scb-v1.json`的固定hex/语义字段/预期codec错误与逐帧SHA256；`tests/vectors/scb-v1.md`列文件SHA256和三端核对表。`scripts/generate_vectors.py`用struct/zlib确定性生成，`scripts/check_vectors.py`独立逐字节解析与逐位CRC核验（不导入生产codec或生成器）；`tests/test_vectors.py`同时调用生产C codec/可靠接口验证。相同request_id不同内容只验证透传/可交付，不声称ESP32执行STM32业务缓存。生成器`--check`仅比较冻结基线，不静默覆盖。三端核对未执行，始终称本端候选向量。

- [x] 本端14条候选固定字节、字段期望及SHA256；独立生成/逐位CRC核验及生产C测试通过（FR-7本端）；G-04三端对齐未执行。
- [x] 按 [验证](validation.md) 区分本端与三端状态；[Group 3交审](../../../docs/validation/phase-02-group-3.md)，本组待审提交。

Group 1→2→3；G-02于2026-10-18已获用户确认，先独立提交本决定，再实施对应动作；保留分组审查和G-01/G-03门禁。

## 计划模块与接口

| 需求 | 计划模块 / 职责 | 输入→输出 | 验证 |
|---|---|---|---|
| FR-1、TC-1 | `main/protocol.c/.h`；显式头字段与CRC | 有界字节/字段→合法编码或错误 | V-1、V-8 |
| FR-2 | `main/protocol.c/.h`；类型/负载验证 | 类型与负载→完整接受/拒绝 | V-2 |
| FR-3 | `main/json_guard.c/.h`；JSON传输校验 | 指针及长度→合法性与有界字段视图 | V-3 |
| FR-4 | `main/reliable_link.c/.h`；每方向状态机 | 帧/当前时间/容量→ACK、投递、重发或超时 | V-4 |
| FR-5、FR-6 | `main/link_session.c/.h`；握手/会话生命周期 | HELLO/IDLE/重启/会话变化→会话状态和动作 | V-5、V-6 |
| FR-7 | `tests/vectors/scb-v1.md`、独立主机参考测试（计划） | 固定字节→字段/CRC/有效性期望 | V-7 |
| TC-2、TC-3 | 协议分派边界与可靠层 | 坏帧/容量/业务结果→丢弃/关闭/接管语义 | V-9、V-10 |

帧对象不借用已归还驱动或socket重用的缓冲；接管前复制到固定槽或移交唯一所有权。ACK推进和业务交付分开，可靠重发不可重写其旧ACK字段以改变原内容，新的确认通过新帧发送。

## 状态、风险与兼容性

SPI状态为等待HELLO→等待新session的IDLE→建立；TCP为等待HELLO→报告当前链路→正常服务/关闭。两个传输的seq/ack互不借用。重发计时只在已发出后开始；首次发送不算重发三次。

所有外部字段照抄契约；G-01/G-02/G-03的未定跨端行为须先协调，不靠新增私有操作码绕开。具体库实现和C签名可以在组内细化，但不得改变有界解析与外部格式。

## Group 1实际细化与交审

2026-10-18本轮复核Group 1后提交455f817；Group 2已按纯规约16a5c92实现，本次139项回归及目标build/size通过，见[Group 2交审](../../../docs/validation/phase-02-group-2.md)。以下Group 1待审文字为历史记录；当前Group 2待审，Group 3尚未推进。

接口已落实为main/protocol.c/.h的scb_encode/scb_decode及main/json_guard.c/.h的bridge_json_validate/bridge_json_object_get。视图借用输入，失败清空且不接管；编码拒绝别名并保持失败目标缓冲不变。JSON使用调用者独占1492B主机类型大小的固定工作区，992B输入内最多496层容器，无递归/动态分配；重复键有界扫描，数值只按精确binary64溢出边界校验，不解释车辆值。具体C签名按原计划允许在组内细化，无额外外部协议选择。

27项新增、104项全回归、真实目标构建及当前SmartCar原头文件24项记录偏移主机核对已完成，见[报告](../../../docs/validation/phase-02-group-1.md)。代码仍未暂存/提交；确认后推进Group 2，固定共同向量与实际传输错误动作未完成，不勾选后续项。
~~~~~

</details>
