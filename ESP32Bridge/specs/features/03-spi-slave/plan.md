# Phase 03：SPI从机 — 实现计划

> 2026-11-07恢复：以下完整正文保存本阶段背景、需求、详细设计、决定、计划、文件/符号映射和验收，按原文日期与基线解释；原“待审/已确认/已完成”不是本次重新批准或验收。当前实现由[Docs](../../../Docs/index.md)说明；未决见[问题页](../../../Docs/known-issues.md)。前层约束：[mission](../../mission.md)、[techstack](../../techstack.md)、[roadmap](../../roadmap.md)、[code-style](../../code-style.md)、[AGENTS](../../../AGENTS.md)。阶段未显式关闭，实机待验保持；本次不启动05/06。

## 目标与依据

交付对网络独立的DMA从机与READY状态循环，依赖01固定池、02协议/可靠层。依据 [需求](requirements.md)、[v6核对](../../research/2026-10-17-idf-v6-compatibility.md)。

## 任务分组

### Group 1：驱动与资源

内部任务/启动/6144B静态栈细化见[group-1-design](group-1-design.md)，新增资源先交用户确认并独立纯规约提交；Group 1只真实初始化，READY高/事务循环留Group 2。

- [x] 真实SPI2引脚/Core1优先级12静态任务/同核驱动/固定DMA/描述及初始化错误路径（FR-1、FR-2、TC-1）已提交712fb41；[历史Group 1报告](../../../Docs/validation/phase-03-group-1.md)，ISR亲和性/DMA实机待测。

### Group 2：READY与回收循环

- [x] 实现IRAM回调、唯一所有权转移、先回收验证再重排（FR-3、FR-4、TC-2）；实际任务循环测试/ELF调用链通过。
- [x] 注入0/8191/8193驱动长度、CRC/JSON错及排队/get失败，验证不误ACK/回收/READY；实机电气仍待测。

### Group 3：协议接入与测量

- [x] 后续事务握手/可靠响应、预留后接管及遥测丢弃接口已接入（FR-5、FR-6）；当前生产无网络消费者，默认真实背压/丢弃，实际网络队列留04/05。
- [x] 本次32项生产SPI边界/生命周期及178项全回归、最终目标build/size通过；[阶段交审](../../../Docs/validation/phase-03.md)。实机波形、缓存关闭/运行堆栈及G-05仍待验，阶段整体不勾选。

顺序Group 1→2→3，各组运行对应验证；按用户最新指令，阶段内连续推进/本地提交，Phase末统一审查一次。Group 2/3的接口/错误/计时细化见[lifecycle-design](lifecycle-design.md)，先纯规约提交；G-06已明确确认。

## 计划模块与追溯

| 需求 | 计划模块 / 职责 | 输入→输出 | 验证 |
|---|---|---|---|
| FR-1 | `main/spi_bridge.c/.h`；总线配置 | 固定引脚/模式→驱动或明确错误 | V-1 |
| FR-2 | SPI任务入口和初始化 | Core1任务→同核SPI服务 | V-2 |
| FR-3 | post_setup / post_trans与收发循环 | 驱动事件→READY/所有权转换 | V-3 |
| FR-4 | 收回事务验证路径 | trans_len及RX→合法帧或丢弃/计数 | V-4 |
| FR-5 | 协议层与固定队列接口 | 待发帧/容量→下一次TX或合法IDLE | V-5 |
| FR-6 | `main/bridge_state.c/.h`的队列和统计入口 | 遥测/连接状态→接管或丢弃计数 | V-6 |
| TC-1 | 固定池与事务描述 | 唯一槽位→提交/回收 | V-7 |
| TC-2 | 回调及GPIO调用链 | ISR事件→轻量GPIO/标志 | V-8 |
| TC-3 | `docs/evidence/`下后续测量报告 | CS/SCK/READY波形→时钟数/频率 | V-9 |

## 时序与失败边界

启动READY高→应用准备TX并提交→驱动装载后READY低→主机CS有效完成交换→回调READY高→任务成功取回事务→校验/接管→准备下次事务。单个缓冲只归应用或驱动一方所有，回调不能越过应用收回步骤提前处理下一条业务。

SPI握手由阶段02状态机决定，网络有无连接只影响上层数据去向。等待主机期间不需要socket进度，可靠计时工作不能依赖忙等SPI事务；阶段05维护共享时钟/待发动作，SPI取回后读取动作，不在其他任务改驱动持有TX。

## 风险与取舍

实测7.5MHz不得从旧文档推算。DMA错误或实测READY失序时先保留波形和计数，不改线格式掩盖问题。合并后对端已声明96B记录/256B头，原32B不匹配已消除；完整偏移及SCB/1对端能力仍需核对，不阻止独立从机测试。

## 2026-11-05 后续源码整改增量
- [x] 按[本轮整改契约](../../research/2026-11-05-source-style-remediation-draft.md)的G3 SPI逐文件整改及审查；G1→G2→G3→G4连续推进，共用替身不重复记完成。
- [x] 完成S1～S4、S6静态复核及S7回填，详见本轮报告。
- [ ] S5 ESP-IDF v6.0.0当前目标构建：既有固定入口环境缺失，未验证。

基线35e5627已经独立保存；本批纯规约提交完成后才修改实现。无法证明行为保持的部分暂缓，发现缺陷只登记不修复。

## 现行功能导航（2026-11-07）

- [F04 SPI事务与协议适配](../../../Docs/transport/spi.md#f04)
- [F06 可靠接管与请求信用](../../../Docs/bridge/forwarding-and-lifecycle.md#f06)
- [F07 遥测与源事件](../../../Docs/bridge/forwarding-and-lifecycle.md#f07)
- [F09 本地诊断与统计](../../../Docs/system/diagnostics.md#f09)
- [F01 启动、配置与固定池](../../../Docs/system/startup-and-resources.md#f01)
