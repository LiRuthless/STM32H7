# 2026-11-07 适度模块化审查任务组

基线：23520ddf9cea3a41870eca6765da9487fff1c1c4。用户本轮授权审查并实施可证明保持语义的优化；沿用未关闭的Phase01/04/05，不进入06，不解除其他门禁。

## 需求与范围

审查main全部24个自维护C/H及组件CMake接入，逐模块核对实际调用、状态owner、错误与并发边界，执行code-style适度模块化十条。仅实施以下确定项，不删除公开接口、不改变协议或资源方案。原有8个在途文件不修改、不暂存。

## 设计及文件/符号映射

| 归属 | 文件/符号及调整 | Docs |
|---|---|---|
| Phase01 FR-2 | main/app_main.c::app_main：修正末尾过时日志及注释，说明生产桥接已绑定、端到端仍待验；不改初始化和报告 | system/startup-and-resources.md |
| Phase04 FR-3/FR-5 | main/net_bridge.c::close_control移除未使用health参数；所有调用点直接调用；rx_recv仅为其透传health，同步移除参数及调用点 | transport/network.md |
| Phase05 FR-1/FR-3/FR-4 | main/bridge_state.c新增私有clear_udp_queue，复用clear_queues及bridge_state_identity中完全相同的释放/计包循环；expire_export保留首次无条件revoke_frame，删除active分支第二次同帧撤销 | bridge/forwarding-and-lifecycle.md |

调整后职责不变：NET独占socket和归因计数；bridge统一拥有共享租借/FIFO/信用；清队列助手只在已有bridge锁内调用，不取得新锁。startup依赖表、IDF签名适配、SPI协议层、精确ACK适配继续保留。

## 行为不变量与等价依据

- close_control原参数只被(void)丢弃，调用实参均为局部指针、无求值副作用；rx_recv本身不计数。删除参数不改变close/reset/publish_identity及调用顺序。
- 两处UDP清理逐槽release、dropped/cleared各加1、count清零的顺序完全一致；不处理s_udp_borrowed，不释放借出数据；锁序仍bridge→pool。
- revoke_frame只将borrowed帧置revoked或清零；同一锁内无外部调用改变该COMMAND，再次调用幂等。首次无条件调用保持active=false路径。
- SCB/1字段、ACK、连接/订阅身份、FIFO、背压、全部期限/刷新点、断线与会话清理、任务/ISR/DMA所有权不变。固定数组、栈、任务优先级和亲和性不变，不新增运行期分配。
- 日志文字仅纠正实现状态，不报告构建/实机通过，不携带秘密；日志变化限定为本条明确授权内容。

## 实施与验收

本纯规约独立提交后，先更新上述Docs并标记本组实现同步中/基线，再紧接实现，结束撤标。同批交付Docs、源码和审查报告。逐调用复核，无遗漏旧签名、重复清理或错误路径变化；git diff --check。使用已有v6.0.0构建入口，成功须零错误零警告；环境缺失记录未验证，不安装或切换SDK。不新增/运行软件测试，不烧录/复位/push/合并。功能真实三端待验，不关闭阶段。

本批设计已确定；R-05和导出计时按当前已批准设计保留，不重新解释历史冲突。其他可选优化及潜在行为问题仅在审查报告列证据与决定边界。
