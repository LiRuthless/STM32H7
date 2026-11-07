# 生产桥接与生命周期

2026-11-07：生产代码已接入，最终版本尚未完成目标构建和Phase05验收；用户后续要求先完成固件、不继续测试。阶段决定及替代关系见[Phase05设计](../../specs/features/05-bridge-lifecycle/lifecycle-design.md)。本页描述代码行为，不宣称实机链路通过。

<a id="f06"></a>
## F06 可靠接管、转发与请求信用

startup_idf建立固定池后绑定bridge_state_spi_port、bridge_net_bind_bridge及真实UDP出口；绑定错误进入启动依赖错误。SPI与NET分别持scb_link_t、独立重发图像和解析工作区。电脑TCP COMMAND经NET实际reserve、可靠receive、同步commit复制进入COMMAND槽；SPI peek借出，独立重建session/seq/ack/CRC，复制进SPI可靠缓存后归还数据槽。STM32 RESULT/LOG_DATA经SPI相同接管顺序进入固定上行槽，由NET重封装后发TCP。只有commit返回后才允许后续ACK出线。

全部转发命令共享1个COMMAND槽、2个结果槽和1个活动请求；不再区分普通/查询/导出信用。只有LOG_BEGIN/READ/END继续提取原有导出元数据。RECEIVED用中间槽，其他真实RESULT及匹配LOG_DATA用最终槽。容量不足不接管、不ACK；不伪造BUSY或业务成功。可靠重复取消预留，不重复提交。request_id及业务负载保持。

最终帧复制进TCP可靠缓存后仍保留信用；NET仅在可靠层实际由pending转为已确认时，携带原seq通知bridge_state_net_acked，精确ACK才释放最终信用。IDLE与反向ACK独立推进，send完成不等于电脑确认。SUBSCRIBE仅本地处理，也要求当前非零且ready的SPI会话。

<a id="f07"></a>
## F07 遥测与源事件

SPI TELEMETRY复制入已有16槽池FIFO，满丢旧保新，不跨run拼包。NET借出槽在归还前独占。发送前仅更新payload偏移12的累计ESP32丢弃包数，NET独立UDP seq/CRC，目的IP和端口只取当前控制端订阅。离线/无订阅、满槽丢旧、清队列及出口失败分别计包，累计不随连接复位。sendto成功仅表示交给socket。UDP序号耗尽撤销订阅，不回绕；重新订阅清旧积压。生产绑定后旧bridge_net_telemetry_offer入口拒绝旁路供给。

EVENT使用已有32槽池，先预留后接管，队满不ACK；未接管可靠帧不算缺口。转入TCP缓存后归还槽，同时保存源run_id/tick/control_sequence/revision至精确ACK。断线保留第一个确知无法交付源锚点，由NET可靠生成EVENT_GAP，不用ESP时间或猜值。无可信锚点产生一次关闭待办，允许后续重连查询。SPI新会话清旧锚点与队列。

<a id="f08"></a>
## F08 会话、导出及故障边界

共享bridge身份、信用、FIFO及租借元数据由同一portMUX保护，池操作固定bridge→pool锁序。JSON解析、编码、socket、等待及驱动均在锁外。reserve到commit/cancel及peek到consumed采用代际租借；清理只撤销未归还借用，不复用其数据。NET代际跨任务读写使用同一UDP交接锁；生产连接/session/ready/订阅身份在bridge锁内一致发布。NET在poll前后核对会话。bridge不访问DMA。

普通断线清本连接队列、导出及未发命令，不重置SPI、不重放设置。已经交SPI编码路径的请求不可假定撤回：全局1个墓碑，所有转发命令（含查询）背压至SPI会话改变，迟答只计取消丢弃，收到最终结果也不提前清墓碑。无归属结果计orphan，不交新电脑。此规则可能使断线后的全部转发请求持续不可用，ESP32不自动复位解除墓碑。

导出窗口1，不预读整份日志。记录真实BEGIN成功结果的transfer_id/total_bytes，LOG_DATA匹配已接管READ的request_id/offset/length（1～960B）。块及LOG_END total_bytes/CRC透明转交，不计算整份CRC。BEGIN成功提交起算；只有首个匹配新request_id的LOG_DATA提交刷新，READ接管及旧请求/重复块不续期。now-last_progress≥10000ms取消。NET每轮独立计时，不依赖SPI时钟或网络消息；未发导出命令撤销，已发待答请求形成墓碑。取消不证明STM32互斥已释放，不新增取消操作码。

车辆许可、停车、参数范围、Flash及APPLIED/SAVED均归STM32。socket错误局部清理，SPI可靠故障沿原路径重握手，驱动持有期不写DMA。启动/绑定失败阻止依赖服务，不自动复位。

实现：[bridge_state.c](../../main/bridge_state.c)、[NET](../../main/net_bridge.c)、[SPI适配](../../main/spi_protocol.c)、[启动](../../main/startup_idf.c)。[资源](../system/resource-pools.md)、[统计](../system/diagnostics.md)、[本轮结果与未验项](../validation/2026-11-07-production-bridge.md)。

本组已同步：2026-11-07适度模块化任务组；静态复核及构建限制见[审查报告](../validation/2026-11-07-modularity-review.md)。
clear_queues和bridge_state_identity通过私有clear_udp_queue复用未借出UDP槽的释放及计包；调用者已持bridge锁，助手不取得bridge锁，池调用仍为bridge→pool。正在NET锁外发送的s_udp_borrowed不在该循环内。expire_export在检查活动信用前统一撤销COMMAND，不再在活动分支重复撤销；借用帧只置revoked，归还时才释放。

## Phase02 G4：元数据与验证结果复用

[本组规约](../../specs/features/02-scb-protocol/check-simplification.md)下，bridge_state_net_reserve接收NET已提取的op视图，command_metadata不再重复定位op，信用分类已删除。EVENT锚点、日志参数使用已验证对象查询，仍按精确u32检查本地所需字段。SPI reserve一次提取RESULT status，将OK事实保存在仅SPI owner访问的s_spi_result_ok；同步commit复用该事实，只有当前预留对应BEGIN时读取transfer_id/total_bytes。该布尔不证明业务成功，仅复用车端原始状态字面量判定。取消/重复不提交；下一reserve覆盖该派生状态。

移除bridge_state私有的SPI全量JSON工作区，SPI/NET边界各自原工作区保留。commit短锁读取预留对应操作后在锁外解析BEGIN字段，最终提交仍在原锁内复核revoked/epoch；期间断线不会把旧帧交给新连接。bridge_state_net_peek新增NET工作区参数，仅为本地EVENT_GAP首次验证，验证后与已验证入站队列共用重封装路径。真实ACK、信用、墓碑、导出进度和FIFO策略保持；SPI可配置出站回调仍完整编码。

实际结果与未验项见[检查简化审查](../validation/2026-11-07-check-simplification.md)。

## 第一、二组：单请求准入与资源

2026-11-07，[当前设计](../../specs/features/05-bridge-lifecycle/single-request-design.md)替代三类信用及等待APPLIED时查询仍可前进的旧约束。s_command/s_credit为单对象，COMMAND预留和SPI借用使用布尔，RESULT索引0为RECEIVED、1为最终。NET先预留后接管，最终精确ACK才清active/request；中间ACK不释放；墓碑不因迟答而释放。credit_mask/tombstone_mask只使用bit0。UDP/EVENT仍独立。

expire_export仍按10秒触发，但只有当前单信用active且operation为导出操作才撤销COMMAND/结果；普通查询/控制不能因之前导出窗口到期被撤销。租借撤销与epoch复核、桥接→池锁序不变，无自动复位。COMMAND/RESULT/GAP帧数据由10240B降到4096B，减少6144B，其他池不改。

电脑端全部COMMAND占用同一个本地请求槽。合法可靠负载解析后先发独立IDLE ACK，再释放匹配的最终请求槽并唤醒等待者；RECEIVED和业务超时不释放槽。重复可靠帧只补ACK。编码失败不占槽，分配失败无残留，陌生request_id结果不能释放当前槽。断线清本地资格且保留历史请求；桥接墓碑期间重连查询也可能阻塞到SPI新会话。客户端session锁只保护状态更新，socket与等待不在锁内。

第三组未实施：导出整体状态、10秒期限、源事件/GAP全部保留；不改变STM32业务或Flash。实机状态见[本组审查](../validation/2026-11-07-group12-simplification.md)。
