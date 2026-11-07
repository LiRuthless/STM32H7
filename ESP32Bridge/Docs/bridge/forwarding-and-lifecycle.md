# 桥接端口与当前缺口

2026-11-07对照main/startup_idf.c、spi_protocol.c及net_bridge.c。F06～F08尚未完成生产实现；未来详细设计完整保存在[Phase05设计](../../specs/features/05-bridge-lifecycle/lifecycle-design.md)。不以端口或固定池存在推定整桥接可用。

<a id="f06"></a>
## F06 可靠接管、转发与请求信用

当前[spi_protocol.h](../../main/spi_protocol.h)提供bridge_spi_port_t的reserve/commit/cancel及peek_tx/tx_consumed组合，configure要求成组提供；回调须在返回前复制借用视图。SPI owner在原事务回收后校验帧、方向、session及request_id，再预留、调用可靠receive；DELIVER同步commit，其他结果撤销预留。无真实消费者时可靠业务背压，不伪造接管ACK。传输ACK绝不代表APPLIED/SAVED。

[startup_idf.c](../../main/startup_idf.c)未绑定双侧桥接生产消费者，只为NET配置bridge_net_socket_telemetry出口。NET本地SUBSCRIBE以外的可靠COMMAND没有生产下游容量，不能被当成已交给STM32。两个传输任务各持独立可靠图像；不会把原DMA指针交给NET。

当前没有bridge_state.c/.h，故无生产请求信用、结果队列及双侧转发。候选3/6/1槽和新增10240B额度属于[阶段目标](../../specs/features/05-bridge-lifecycle/lifecycle-design.md)，不计入[当前固定池](../system/resource-pools.md#f01-pools)。R-05批准冲突保持[EB-01](../known-issues.md#eb-01)。

<a id="f07"></a>
## F07 遥测与源事件

SPI协议对合法非可靠TELEMETRY累计包和记录；telemetry回调缺失或拒绝时累计丢弃包/记录，当前生产启动未绑定此供给链。EVENT等可靠业务无预留不ACK。池提供UDP/EVENT额度，未实现FIFO、丢旧保新或源事件缺口生命周期。

NET有[订阅与UDP出口](../transport/network.md#f05-subscribe)，要求当前连接/会话/订阅身份；提供出口不等于SPI遥测已进入网络。当前不能宣称实时桥接、源EVENT可靠交付或EVENT_GAP生产链完成。G-03源锚点及队列目标见[阶段决定](../../specs/features/05-bridge-lifecycle/g-03-event-gap.md)。

<a id="f08"></a>
## F08 连接清理与导出边界

当前NET在SPI会话变化后撤销许可、关闭旧TCP并清订阅；SPI逻辑失效不等于驱动DMA释放。socket错误局部恢复，不自动复位、不新增车辆停车动作，不重放旧设置。实际owner分别见[可靠会话](../protocol/reliable-and-sessions.md#f03)、[SPI回收](../transport/spi.md#f04-ownership)及[网络](../transport/network.md#f05)。

尚无完整导出状态、信用、取消墓碑或10秒清理实现。LOG_DATA格式校验能力不能证明请求匹配、整份CRC或STM32互斥释放。导出计时基准的两种原声明并列保留在[Phase05设计](../../specs/features/05-bridge-lifecycle/lifecycle-design.md)，见[EB-05](../known-issues.md#eb-05)，本次不裁决。跨任务代际风险见[EB-07](../known-issues.md#eb-07)。

当前端口与模块可由[架构](../architecture.md)反查，FR/TC及R6由[追溯](../traceability.md)进入阶段完整设计。F06～F08不计为已实现生产功能；静态资源、历史宿主、目标构建及实机各自验收，见[验证指南](../guides/validation.md#f10)。
