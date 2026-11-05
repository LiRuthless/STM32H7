# 已知问题与待验入口

> 2026-11-05。只维护状态与导航，详细契约在功能页，决定/修复任务在所属Phase；登记不代表批准或解决。

<a id="eb-01"></a>
## EB-01 R-05批准记录冲突

状态：待核实，依赖此决定的实现不推进。本轮整理前AGENTS/mission/roadmap/traceability写“待确认”，P05三件套及lifecycle-design却写2026-10-20“已确认”。原文见[治理历史](../specs/research/2026-11-05-governance-history.md)与[P05需求冻结记录](../specs/features/05-bridge-lifecycle/requirements.md)。
用户需核实是否批准原固定资源方案及后续确定性条款；本轮不替代该决定、不否认可能存在的旧批准。影响[F06](bridge/forwarding-and-lifecycle.md#f06)～F08，修复归P05。

<a id="eb-02"></a>
## EB-02 跨端及实机验收未完成

状态：硬件/三端待验。14条本端候选向量尚不能称三端一致；SPI电气/READY/吞吐/真实STM32导出释放未完整验收。旧镜像启动/AP/DHCP已有历史记录，修复后网络流程待复测。[F04](transport/spi.md#f04)、[F05](transport/network.md#f05)、[F08](bridge/forwarding-and-lifecycle.md#f08)、[F10](guides/validation.md#f10)；阶段03～06分别记录，不补勾完成。

<a id="eb-03"></a>
## EB-03 存量代码/注释未按新标准全量整改

状态：差距已登记，未整改。抽查存在紧凑排版、英文普通注释、缺中文Doxygen和H完整/C不足等情况；[标准](../specs/code-style.md)规定后续要求，不宣称全量合规，不自动建立整改阶段。

<a id="eb-04"></a>
## EB-04 当前副本缺桥接实现及部分历史证据

状态：副本事实待核实。[R6](../specs/features/05-bridge-lifecycle/r6-log-counters.md)称bridge_state/bridge_stats在途且提258项回归；当前main无这两个模块，SPI日志新增字段存在。保留原声明，不把缺失端模块编造为已交付。影响[F09](system/diagnostics.md#f09)、P05。
本副本无历史artifacts/.local原产物；JSON可读不证明其所列日志/镜像当前可复核。工程整体未跟踪，原提交顺序不能在本仓核验。历史结果注明原基线与缺失，未来导入源码/重验另行授权。

<a id="eb-05"></a>
## EB-05 导出空闲计时基准存在歧义

状态：待决定。原P05设计前段以新LOG_READ刷新，后段又要求匹配LOG_DATA作为有效新进度；不得用迁移选择其一。两者对STM32迟答/丢包时取消时点不同，实施前需明确。详细目标和共同边界见[F08](bridge/forwarding-and-lifecycle.md#f08)，原文见[P05设计冻结记录](../specs/features/05-bridge-lifecycle/lifecycle-design.md)。

## 资料修复与历史身份

旧datalog.h导航已修到SmartCar/App/services/datalog.h，原路径保留在明确历史记录中；不改写原始日志。
原任务映射两个M-27分别指“窗口/CRC”和“10秒取消”；以原编号+用途+关联FR区分，保留[M映射历史](../specs/traceability.md)，不重新分配已发布身份。

<a id="eb-06"></a>
## EB-06 任务书副本哈希与旧来源声明不符

状态：原附件待核对。本次整理前后任务书副本实际SHA256均为3559FD075B6BB4675976F5BA159F3B2E7DB7177C9EB293417C23BC93F8071269，与旧表491B1E66开头的声明不同；不推断是哪次导入造成，不修改原附件内容。完整值及旧声明见[来源索引](../specs/references/README.md)。这不影响本轮文档链接检查，但不能报告原始来源字节一致。
