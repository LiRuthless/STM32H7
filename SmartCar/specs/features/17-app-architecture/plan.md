# Phase 17 实施计划

2026-10-31文档迁移：**Phase → docs → 代码**。本页保留需求/任务/验收历史，不新增阶段、不修改实现或重新验收。

当前需求导航：[P17功能索引](../../../docs/traceability.md#p17)。具体输入输出、状态/错误/时序、代码符号与验证方法由下列docs条目唯一维护。

## 任务分组与详细设计入口

历史分组/依赖/风险/回退和用户决定保留在下方原记录；本次未重开原实现任务。以后执行本阶段增量时，先确定对应需求和docs目标，按原前置及已确认决定分组；实现文件与符号不在本页复制维护。

| 原需求 | 详细契约/实现/验证入口 | 执行边界 |
|---|---|---|
| FR-01 五类结构 | [F32 标准R08与维护](../../../docs/guides/validation.md#f32) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-02 路径迁移 | [F32 标准R08与维护](../../../docs/guides/validation.md#f32) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-03 双工程源 | [F30 构建烧录与再生](../../../docs/guides/build-and-flash.md#f30) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-04 系统协调 | [F01 初始化与三线调度](../../../docs/system/runtime-and-safety.md#f01)；[F02 起停、模式与首因](../../../docs/system/runtime-and-safety.md#f02)；[F29 时钟DMA采样与驱动](../../../docs/platform/board-and-bsp.md#f29) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-05 感知状态 | [F05 电感偏差与对称度](../../../docs/sensing/sensors-and-calibration.md#f05)；[F06 灰度采集标定与质心](../../../docs/sensing/sensors-and-calibration.md#f06)；[F07 IMU校准滤波与积分](../../../docs/sensing/sensors-and-calibration.md#f07)；[F03 低压持续判定](../../../docs/system/runtime-and-safety.md#f03) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-06 控制拆分 | [F08 方向角度速度与执行](../../../docs/control/tracking-and-controllers.md#f08)；[F09 活动电磁元素](../../../docs/control/elements-and-dormant-code.md#f09)；[F10 光电状态机](../../../docs/control/tracking-and-controllers.md#f10)；[F11 双FSM与唯一执行](../../../docs/control/tracking-and-controllers.md#f11)；[F12 部分配置与全局调参规划](../../../docs/control/tracking-and-controllers.md#f12) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-07 参数事务 | [F18 完整拍临时调参](../../../docs/services/parameters-and-tuning.md#f18)；[F15 多入口参数应用](../../../docs/services/parameters-and-tuning.md#f15)；[F16 参数v3格式](../../../docs/services/parameters-and-tuning.md#f16)；[F17 迁移保存与失败](../../../docs/services/parameters-and-tuning.md#f17) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-08 记录拆分 | [F20 队列前台写入与故障](../../../docs/services/recording-and-export.md#f20)；[F19 日志格式容量](../../../docs/services/recording-and-export.md#f19)；[F21 CSV与会话边界](../../../docs/services/recording-and-export.md#f21)；[F22 二进制流式导出](../../../docs/services/recording-and-export.md#f22-export)；[F25 独立上传](../../../docs/services/communication.md#f25) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-09 协议与HMI | [F23 SCB帧JSON与命令](../../../docs/services/communication.md#f23)；[F14 菜单按键与显示](../../../docs/hmi/operation.md#f14)；[F24 会话可靠传输与回绕](../../../docs/services/communication.md#f24) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-10 legacy隔离 | [F13 legacy与未接入元素](../../../docs/control/elements-and-dormant-code.md#f13) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-11 唯一owner | [F11 双FSM与唯一执行](../../../docs/control/tracking-and-controllers.md#f11)；[F01 初始化与三线调度](../../../docs/system/runtime-and-safety.md#f01)；[F20 队列前台写入与故障](../../../docs/services/recording-and-export.md#f20) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-12 行为兼容 | [F11 双FSM与唯一执行](../../../docs/control/tracking-and-controllers.md#f11)；[F01 初始化与三线调度](../../../docs/system/runtime-and-safety.md#f01)；[F08 方向角度速度与执行](../../../docs/control/tracking-and-controllers.md#f08)；[F19 日志格式容量](../../../docs/services/recording-and-export.md#f19)；[F23 SCB帧JSON与命令](../../../docs/services/communication.md#f23) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-13 文档追溯 | [F32 标准R08与维护](../../../docs/guides/validation.md#f32) | 当前契约及历史替代见docs；不改变原验收状态 |

执行顺序：先读需求→对应docs→检查未决/依赖→纯规约提交→实现→所选验证及记录。回退以该次完整组为单位，同步回退接口/调用/文档，历史兼容和保护边界不借回退改变。纯文档检查不触发旧组软件门禁。














## 冻结历史基线（68cb508，非现行契约）

以下原文用于保留阶段决定、组顺序、需求历史与真实验证；不继续独立维护算法/布局/源码映射。当前契约先查上方docs条目。旧“本轮”“待执行”“当前”只指原记录时点，不覆盖后续决定。历史任务的全量软件门禁不自动成为以后每次修改的要求。

<details>
<summary>展开原始阶段记录（原文保留）</summary>

# Phase 17 实施计划

| 组 | 需求 | 文件/符号范围 | 退出门禁 |
|---|---|---|---|
| G0 | FR-01～13 | 本目录、roadmap、techstack、code-style | 纯文档提交 |
| G1 | FR-01～03/12/13 | path-map全部C/H；全部仓库内include；GCC/Keil；tests | 双LCD构建、token等价、宿主回归、R08 |
| G2 | FR-04/05/11/12 | app、BSP适配、battery、scheduler/lifecycle/safety | 起停/保护/调度回归 |
| G3 | FR-06/10/11/12 | control/pid/roundabout/element/imu | 控制逐拍等价与休眠清单 |
| G4 | FR-07/09/11/12 | param/tuning/menu/dashboard | 参数迁移/调参/交互回归 |
| G5 | FR-08/09/11/12 | datalog/scb_protocol/link/telemetry/dispatch | 格式/会话/队列/导出回归 |
| G6 | FR-01～13 | 全量依赖/注释/资源/文档 | 实际证据与待验列表 |

G1保持原函数签名、返回和对象链接属性，除include路径及头保护宏外不改非注释token。
GCC扫描App五类；Keil分组匹配目录；测试在临时目录建立原平铺夹具，内部包含按真实相对路径复制，历史基线路径不变。
新增测试路径解析器统一定位当前文件，找不到或多义时失败，不回退读取旧源码。

每组规约独立提交后才实现；G2～G5编码前补全逐符号输入输出及状态迁移增量。
回退按完整组恢复源码、工程、测试和文档，不修改Flash存储格式。构建或等价验证失败停止本组提交。

## 实际分组交付（2026-10-24）

G0 82102ba；G1 35443e3（生成ELF恢复单独8a570d7）；G2 e4c7f9b；G3 6125495；
G4 7eefb95；G5 fe274b3。各实现前的G2～G6详细接口规约独立提交，G4原副作用闭包
2ee3401先行，用户2026-10-24具体批准后才实施，拒绝/负对照事实保留。
G6交付最终工程选项、索引/依赖/资源工具、二十文件注释/格式及全量文档；最终暂存R08已通过并在validation回填；软件可执行部分完成，硬件/原样本限制保留。
完整分组回退须同步代码/头/工程/夹具和导航，不运行参数/日志格式迁移、不push。

</details>
