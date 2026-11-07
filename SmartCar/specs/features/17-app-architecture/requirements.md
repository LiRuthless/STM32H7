# Phase 17 App 架构与职责重构需求

日期：2026-10-23。用户明确授权实施完整五大类方案，覆盖此前方案中“仅形成方案”的历史范围。
调查基线：`4745aa2339209d5df9a739449685fc7f1cb66429`。既有 Hardware 修改和 MDK-GCC/build 产物不纳入提交。

## 需求

- FR-01：App 只有 system/sensing/control/services/hmi 五个业务目录，无嵌套目录。根保留入口、配置和业务公共类型。
- FR-02：G1 按 path-map.json 搬迁现有 C/H，保持函数体、对象、链接属性、预处理条件和初始化顺序；不在机械迁移批次拆函数。所有跨模块包含以 App 根的相对模块路径定位。
- FR-03：GCC 覆盖 App 根及五类全部 C，Keil 建立五类分组，每源只编译一次。宿主测试读取当前源码使用统一解析，历史 git show 路径保持。
- FR-04：system 分离调度、生命周期和安全；App 不含 HAL/CubeMX 类型或寄存器操作，平台回调与硬件动作归 BSP。保持前台二次停车检查及立即停车/前台收尾分离。
- FR-05：sensing 持有电感、灰度、IMU、电池及滤波状态，算法不依赖通信/HMI/日志；采样次数、系数、失败输出及标定不变。
- FR-06：control 分离双模式协调、部分配置和方向/角度/速度算法。显式输入参数和独立历史；左右PI、两模式观察互不共享历史；配置 fields 未置位组保持。
- FR-07：参数 bank、持久化与应用协调分离；参数不依赖菜单。调参使用业务类型，保留 baseline/pending/last_success/effective/revision 独立状态及完整拍生效。
- FR-08：记录组装、队列、Flash和导出分离；上传与Flash队列独立。日志结果由通信适配映射 SCB，不依赖协议结果类型。
- FR-09：帧、JSON、命令编码、会话链路、业务分发分离；HMI 分离按键/逻辑/绘制，调用业务接口。
- FR-10：休眠入口隔离 legacy_*，保持编译、内部调用和原状态；不新增活动调用。App_AbsFloat 转旧环岛私有函数。
- FR-11：每状态唯一所有者；共享读取按原并发要求复制，不扩大临界区，不新增堆分配或大缓冲。
- FR-12：保持 TIM15/6/7 1/2/5ms及优先级；700/900us预算；保持全部算法、阈值、窄化/截断、操作顺序、保护首因、协议、CSV、参数v3和日志v2兼容。错误返回及失败后的副作用按当前源码保留，不顺带修复既有缺陷。
- FR-13：同步 README、techstack、code-style、roadmap、相关Phase、休眠清单、注释证据及CHANGELOG；历史结果不改写。

## 授权和门禁

G0/G1 无未决设计。G2～G5 的逐符号接口增量必须在相应组实现前完成并独立提交；不能仅凭目录目标推断签名。
硬件、AC5、R07外部样本阻塞不伪造通过，不标记旧Phase完成。

## 最终追溯入口（2026-10-24）

FR-01～13逐条实现与实际验证对应见validation的“需求—实现—验证闭环”；完整公开/私有函数、
类型/字段、宏/分支及状态见migration-map.md和final-symbol-map.json，源/依赖R08证据见Phase15。
软件可执行部分与R07、AC5及实机未执行项分别记录，不把目录完成或编译通过当作全部行为等价。

> 2026-11-07 文档纠偏：本页恢复本阶段完整正文（恢复依据 `68cb508`），原路径、符号、决定和结果按当时基线解释。当前功能见 [Docs](../../../Docs/index.md)，后续演进见 [路线图](../../roadmap.md)。历史验证安排不自动成为后续每次改动的门禁；本次未重新验收固件或实机。


## 现行功能导航与稳定需求锚点

下列导航连接本阶段编号与当前说明；本阶段完整需求、设计、映射和验收以上文原正文为准，当前功能说明不覆盖阶段决定。

<a id="fr-01"></a>
### FR-01 五类结构

当前说明：[F32 标准R08与维护](../../../Docs/guides/validation.md#f32)。

<a id="fr-02"></a>
### FR-02 路径迁移

当前说明：[F32 标准R08与维护](../../../Docs/guides/validation.md#f32)。

<a id="fr-03"></a>
### FR-03 双工程源

当前说明：[F30 构建烧录与再生](../../../Docs/guides/build-and-flash.md#f30)。

<a id="fr-04"></a>
### FR-04 系统协调

当前说明：[F01 初始化与三线调度](../../../Docs/system/runtime-and-safety.md#f01)；[F02 起停、模式与首因](../../../Docs/system/runtime-and-safety.md#f02)；[F29 时钟DMA采样与驱动](../../../Docs/platform/board-and-bsp.md#f29)。

<a id="fr-05"></a>
### FR-05 感知状态

当前说明：[F05 电感偏差与对称度](../../../Docs/sensing/sensors-and-calibration.md#f05)；[F06 灰度采集标定与质心](../../../Docs/sensing/sensors-and-calibration.md#f06)；[F07 IMU校准滤波与积分](../../../Docs/sensing/sensors-and-calibration.md#f07)；[F03 低压持续判定](../../../Docs/system/runtime-and-safety.md#f03)。

<a id="fr-06"></a>
### FR-06 控制拆分

当前说明：[F08 方向角度速度与执行](../../../Docs/control/tracking-and-controllers.md#f08)；[F09 活动电磁元素](../../../Docs/control/elements-and-dormant-code.md#f09)；[F10 光电状态机](../../../Docs/control/tracking-and-controllers.md#f10)；[F11 双FSM与唯一执行](../../../Docs/control/tracking-and-controllers.md#f11)；[F12 部分配置与全局调参规划](../../../Docs/control/tracking-and-controllers.md#f12)。

<a id="fr-07"></a>
### FR-07 参数事务

当前说明：[F18 完整拍临时调参](../../../Docs/services/parameters-and-tuning.md#f18)；[F15 多入口参数应用](../../../Docs/services/parameters-and-tuning.md#f15)；[F16 参数v3格式](../../../Docs/services/parameters-and-tuning.md#f16)；[F17 迁移保存与失败](../../../Docs/services/parameters-and-tuning.md#f17)。

<a id="fr-08"></a>
### FR-08 记录拆分

当前说明：[F20 队列前台写入与故障](../../../Docs/services/recording-and-export.md#f20)；[F19 日志格式容量](../../../Docs/services/recording-and-export.md#f19)；[F21 CSV与会话边界](../../../Docs/services/recording-and-export.md#f21)；[F22 二进制流式导出](../../../Docs/services/recording-and-export.md#f22-export)；[F25 独立上传](../../../Docs/services/communication.md#f25)。

<a id="fr-09"></a>
### FR-09 协议与HMI

当前说明：[F23 SCB帧JSON与命令](../../../Docs/services/communication.md#f23)；[F14 菜单按键与显示](../../../Docs/hmi/operation.md#f14)；[F24 会话可靠传输与回绕](../../../Docs/services/communication.md#f24)。

<a id="fr-10"></a>
### FR-10 legacy隔离

当前说明：[F13 legacy与未接入元素](../../../Docs/control/elements-and-dormant-code.md#f13)。

<a id="fr-11"></a>
### FR-11 唯一owner

当前说明：[F11 双FSM与唯一执行](../../../Docs/control/tracking-and-controllers.md#f11)；[F01 初始化与三线调度](../../../Docs/system/runtime-and-safety.md#f01)；[F20 队列前台写入与故障](../../../Docs/services/recording-and-export.md#f20)。

<a id="fr-12"></a>
### FR-12 行为兼容

当前说明：[F11 双FSM与唯一执行](../../../Docs/control/tracking-and-controllers.md#f11)；[F01 初始化与三线调度](../../../Docs/system/runtime-and-safety.md#f01)；[F08 方向角度速度与执行](../../../Docs/control/tracking-and-controllers.md#f08)；[F19 日志格式容量](../../../Docs/services/recording-and-export.md#f19)；[F23 SCB帧JSON与命令](../../../Docs/services/communication.md#f23)。

<a id="fr-13"></a>
### FR-13 文档追溯

当前说明：[F32 标准R08与维护](../../../Docs/guides/validation.md#f32)。
