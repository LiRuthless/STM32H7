# Phase 05：桥接与故障处理 — 验证

> 2026-11-07验收说明：下方原执行记录保留各自日期、版本与计数，不是本次重跑。当前47文件静态整改和187项宿主历史结果、目标构建S5未验证分别见[最终复核](../../../Docs/validation/2026-11-05-source-style-review.md)。本次仅核对文档，未运行固件/宿主/硬件验收；阶段不补勾关闭。


## 当前验收边界（2026-11-05文档核对）

桥接生产消费者未实现；R-05批准记录冲突。本轮未构建、未运行行为回归、未烧录；仅执行文档迁移检查，不覆盖下方原运行记录。2026-11-05工程导入前无法在本仓验证原提交顺序；其后35e5627已完成工程导入，缺本地产物的历史结果不作当前重验。

## 可复用方法导航

| 需求 | 方法主入口 |
|---|---|
| [FR-1](requirements.md#fr-01) | [F06 可靠接管与请求信用](../../../Docs/bridge/forwarding-and-lifecycle.md#f06) |
| [FR-2](requirements.md#fr-02) | [F06 可靠接管与请求信用](../../../Docs/bridge/forwarding-and-lifecycle.md#f06) |
| [FR-3](requirements.md#fr-03) | [F07 遥测与源事件](../../../Docs/bridge/forwarding-and-lifecycle.md#f07) |
| [FR-4](requirements.md#fr-04) | [F07 遥测与源事件](../../../Docs/bridge/forwarding-and-lifecycle.md#f07) |
| [FR-5](requirements.md#fr-05) | [F08 生命周期与单块导出](../../../Docs/bridge/forwarding-and-lifecycle.md#f08) |
| [FR-6](requirements.md#fr-06) | [F08 生命周期与单块导出](../../../Docs/bridge/forwarding-and-lifecycle.md#f08) |
| [FR-7](requirements.md#fr-07) | [F08 生命周期与单块导出](../../../Docs/bridge/forwarding-and-lifecycle.md#f08) |
| [FR-8](requirements.md#fr-08) | [F09 本地诊断与统计](../../../Docs/system/diagnostics.md#f09) |
| [FR-9](requirements.md#fr-09) | [F08 生命周期与单块导出](../../../Docs/bridge/forwarding-and-lifecycle.md#f08) |
| [TC-1](requirements.md#tc-01) | [F06 可靠接管与请求信用](../../../Docs/bridge/forwarding-and-lifecycle.md#f06) |
| [TC-2](requirements.md#tc-02) | [F06 可靠接管与请求信用](../../../Docs/bridge/forwarding-and-lifecycle.md#f06) |
| [TC-3](requirements.md#tc-03) | [F08 生命周期与单块导出](../../../Docs/bridge/forwarding-and-lifecycle.md#f08) |

## 原阶段标准与历史执行记录

以下保留原日期、计数和逐项标准/执行事实。旧“当前/本轮”、旧路径、已被替代的审查节奏只对应原记录时点；不作为本次执行声明。现行政策见[AGENTS](../../../AGENTS.md)，现行契约以Docs入口为准。


> 2026-10-20：R-05已获用户确认，先独立提交[lifecycle-design](lifecycle-design.md)，再以生产C实现tests/test_bridge_lifecycle.py及双侧协议夹具。下表仍为待执行，不能用Phase04基线222项替代Phase05证据。

## 验证清单

2026-10-18：V-4按已确认的[g-03-event-gap](g-03-event-gap.md)核对首个源锚点；下表“待确认”为历史状态，代码/测试仍未执行。

| 编号 | 检查项 / 方法 | 可观测通过标准 | 关联需求 | 状态 |
|---|---|---|---|---|
| V-1 | 满可靠槽、双侧不同seq/ack、丢ACK和重发 | 无额度不ACK/不接管；已接管不丢；精确ACK释放，跨端重封装CRC正确，request_id/负载保持 | FR-1 | 未执行 |
| V-2 | SET_PARAMS延迟APPLIED，同时查询结果并注入保存事件 | 第二调参不进入待应用；查询和IDLE仍推进；RECEIVED不是完成，APPLIED/SAVED只转真实STM32内容 | FR-2 | 未执行 |
| V-3 | 连续17包填满16槽，离线/无订阅/断线积压及两run混合 | 满时丢旧，计数含离线/清队列；只改ESP32元数据，重新算CRC/seq；run/索引/revision不被重标 | FR-3 | 未执行 |
| V-4 | 32条EVENT后继续输入、可靠重复、断线历史缺口 | 可靠容量不足不先ACK；明确缺口只报告一次所需语义，不伪造设备字段；报告格式符合G-03决定 | FR-4 | 未执行；G-03待确认 |
| V-5 | 普通断网、重连、SPI新HELLO与ESP32重启 | 断网不重置SPI/值，重连不重放设置；会话变化清旧缓存/订阅/导出并关闭旧TCP；重连握手获取当前状态 | FR-5 | 未执行；G-02方案已确认 |
| V-6 | 1/960B块、末块、重复请求、错偏移/长度和LOG_END | 窗口1不预读整份；合法字节/request_id透传；重复不重复推进整体CRC；无伪造最终成功 | FR-6 | 未执行 |
| V-7 | 9.999/10秒无新读取、重复旧请求、连接仍活、断线和会话变化 | 10秒取消本端状态；旧请求不无限续期；模拟STM32释放互斥，真实释放另验，不新增取消操作码 | FR-7 | 未执行 |
| V-8 | 对已知数量的收发/坏帧/丢弃/重试做状态快照 | SPI/TCP/UDP、CRC、截短、重试、峰值、丢弃及session可核对，socket无调试文本 | FR-8 | 未执行 |
| V-9 | 初始化/驱动/socket/可靠超时逐项故障 | 限定服务停止/恢复，不自动EN复位，不新增停车，不回滚已应用值 | FR-9 | 未执行 |
| V-10 | 最大负载、队列饱和与所有权断言 | 固定容量、无启动后反复分配；可靠窗口1，查询及最终结果有预留槽；不写驱动持有DMA | TC-1 | 未执行 |
| V-11 | 非普通元素/越界业务/保存失败及32B真实记录输入 | 业务判定由模拟STM32，结果原样保留；不自行解析控制值；32B真实联调明确阻塞 | TC-2 | 未执行 |
| V-12 | SPI无时钟、TCP无新数据，推进模拟单调时钟 | 重试/取消定时仍记录到待办；无忙等或无限socket等待，不重写在驱动中的TX | TC-3 | 未执行 |

## 自动化验证（计划）

计划入口 `tests/test_bridge_backpressure.py`、`tests/test_bridge_sessions.py`、`tests/test_log_forwarding.py`，均尚未实现。测试同时观察SPI侧、TCP侧独立状态和业务结果，不只测试单个队列函数。

## 人工步骤与回归

1. 对照报文抓包和统计快照，核对每条已ACK可靠业务的后续去向。
2. 调参过程中断网，确认既有覆盖/参数保持；再次连接先查询结果，不发送旧设置。
3. 导出中保持连接但停止读取，验证真实STM32在10秒超时后释放互斥；G-05未解除前只能运行模拟版本。
4. 确认所有业务状态由STM32提供，串口调试不污染socket。

每个用例保存输入时间线、帧身份、队列峰值、实际结果和版本；当前无功能证据，不能声明上述行为已完成。
