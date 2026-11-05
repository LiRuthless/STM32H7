# Phase 03：SPI从机 — 实现计划

> 2026-11-05现行导航；SPI事务/协议已接入；电气和吞吐待验。本次整理不启动实现或补勾任务。

## 目标、依据与依赖

依据[编号需求](requirements.md)和[roadmap](../../roadmap.md)。阶段01提供工程基础，02提供协议，03/04提供SPI/网络，05连接两侧，06验证交付；原阶段前置和决定保留在冻结记录。

## 任务分组

### Group 1：驱动与资源

内部任务/启动/6144B静态栈细化见[group-1-design](group-1-design.md)，新增资源先交用户确认并独立纯规约提交；Group 1只真实初始化，READY高/事务循环留Group 2。

- [x] 真实SPI2引脚/Core1优先级12静态任务/同核驱动/固定DMA/描述及初始化错误路径（FR-1、FR-2、TC-1）已提交712fb41；[历史Group 1报告](../../../docs/validation/phase-03-group-1.md)，ISR亲和性/DMA实机待测。

### Group 2：READY与回收循环

- [x] 实现IRAM回调、唯一所有权转移、先回收验证再重排（FR-3、FR-4、TC-2）；实际任务循环测试/ELF调用链通过。
- [x] 注入0/8191/8193驱动长度、CRC/JSON错及排队/get失败，验证不误ACK/回收/READY；实机电气仍待测。

### Group 3：协议接入与测量

- [x] 后续事务握手/可靠响应、预留后接管及遥测丢弃接口已接入（FR-5、FR-6）；当前生产无网络消费者，默认真实背压/丢弃，实际网络队列留04/05。
- [x] 本次32项生产SPI边界/生命周期及178项全回归、最终目标build/size通过；[阶段交审](../../../docs/validation/phase-03.md)。实机波形、缓存关闭/运行堆栈及G-05仍待验，阶段整体不勾选。

顺序Group 1→2→3，各组运行对应验证；按用户最新指令，阶段内连续推进/本地提交，Phase末统一审查一次。Group 2/3的接口/错误/计时细化见[lifecycle-design](lifecycle-design.md)，先纯规约提交；G-06已明确确认。

## 契约、顺序与回退

- [F04 SPI事务与协议适配](../../../Docs/transport/spi.md#f04)
- [F06 可靠接管与请求信用](../../../Docs/bridge/forwarding-and-lifecycle.md#f06)
- [F07 遥测与源事件](../../../Docs/bridge/forwarding-and-lifecycle.md#f07)
- [F09 本地诊断与统计](../../../Docs/system/diagnostics.md#f09)
- [F01 启动、配置与固定池](../../../Docs/system/startup-and-resources.md#f01)

按Group 1→2→3及原阶段依赖推进；每批实现前先独立提交Phase与Docs目标契约。详细符号、状态、资源和方法不在本计划重复。风险涉及协议、资源或兼容性时先核实决定，回退以本批独立实现提交为单位，不改历史证据。阶段内连续实施，阶段末统一交审，05通过审查后才进入06。

## 未决与验收

本次未新增设计决定；跨端向量、硬件及缺失证据见问题入口。[问题入口](../../../Docs/known-issues.md)，[阶段验证](validation.md)。
<details>
<summary>原计划、任务及风险记录（冻结原文，不作为现行规则）</summary>

~~~~~markdown
# Phase 03：SPI从机 — 实现计划

## 目标与依据

交付对网络独立的DMA从机与READY状态循环，依赖01固定池、02协议/可靠层。依据 [需求](requirements.md)、[v6核对](../../research/2026-10-17-idf-v6-compatibility.md)。

## 任务分组

### Group 1：驱动与资源

内部任务/启动/6144B静态栈细化见[group-1-design](group-1-design.md)，新增资源先交用户确认并独立纯规约提交；Group 1只真实初始化，READY高/事务循环留Group 2。

- [x] 真实SPI2引脚/Core1优先级12静态任务/同核驱动/固定DMA/描述及初始化错误路径（FR-1、FR-2、TC-1）已提交712fb41；[历史Group 1报告](../../../docs/validation/phase-03-group-1.md)，ISR亲和性/DMA实机待测。

### Group 2：READY与回收循环

- [x] 实现IRAM回调、唯一所有权转移、先回收验证再重排（FR-3、FR-4、TC-2）；实际任务循环测试/ELF调用链通过。
- [x] 注入0/8191/8193驱动长度、CRC/JSON错及排队/get失败，验证不误ACK/回收/READY；实机电气仍待测。

### Group 3：协议接入与测量

- [x] 后续事务握手/可靠响应、预留后接管及遥测丢弃接口已接入（FR-5、FR-6）；当前生产无网络消费者，默认真实背压/丢弃，实际网络队列留04/05。
- [x] 本次32项生产SPI边界/生命周期及178项全回归、最终目标build/size通过；[阶段交审](../../../docs/validation/phase-03.md)。实机波形、缓存关闭/运行堆栈及G-05仍待验，阶段整体不勾选。

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
~~~~~

</details>
