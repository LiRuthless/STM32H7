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

> 2026-11-07 文档纠偏：本页恢复本阶段完整正文（恢复依据 `68cb508`），原路径、符号、决定和结果按当时基线解释。当前功能见 [Docs](../../../Docs/index.md)，后续演进见 [路线图](../../roadmap.md)。历史验证安排不自动成为后续每次改动的门禁；本次未重新验收固件或实机。


## 现行功能导航

- [P17功能索引](../../../Docs/traceability.md#p17)
- [F32 标准R08与维护](../../../Docs/guides/validation.md#f32)
- [F30 构建烧录与再生](../../../Docs/guides/build-and-flash.md#f30)
- [F01 初始化与三线调度](../../../Docs/system/runtime-and-safety.md#f01)
- [F02 起停、模式与首因](../../../Docs/system/runtime-and-safety.md#f02)
- [F29 时钟DMA采样与驱动](../../../Docs/platform/board-and-bsp.md#f29)
- [F05 电感偏差与对称度](../../../Docs/sensing/sensors-and-calibration.md#f05)
- [F06 灰度采集标定与质心](../../../Docs/sensing/sensors-and-calibration.md#f06)
- [F07 IMU校准滤波与积分](../../../Docs/sensing/sensors-and-calibration.md#f07)
- [F03 低压持续判定](../../../Docs/system/runtime-and-safety.md#f03)
- [F08 方向角度速度与执行](../../../Docs/control/tracking-and-controllers.md#f08)
- [F09 活动电磁元素](../../../Docs/control/elements-and-dormant-code.md#f09)
- [F10 光电状态机](../../../Docs/control/tracking-and-controllers.md#f10)
- [F11 双FSM与唯一执行](../../../Docs/control/tracking-and-controllers.md#f11)
- [F12 部分配置与全局调参规划](../../../Docs/control/tracking-and-controllers.md#f12)
- [F18 完整拍临时调参](../../../Docs/services/parameters-and-tuning.md#f18)
- [F15 多入口参数应用](../../../Docs/services/parameters-and-tuning.md#f15)
- [F16 参数v3格式](../../../Docs/services/parameters-and-tuning.md#f16)
- [F17 迁移保存与失败](../../../Docs/services/parameters-and-tuning.md#f17)
- [F20 队列前台写入与故障](../../../Docs/services/recording-and-export.md#f20)
- [F19 日志格式容量](../../../Docs/services/recording-and-export.md#f19)
- [F21 CSV与会话边界](../../../Docs/services/recording-and-export.md#f21)
- [F22 二进制流式导出](../../../Docs/services/recording-and-export.md#f22-export)
- [F25 独立上传](../../../Docs/services/communication.md#f25)
- [F23 SCB帧JSON与命令](../../../Docs/services/communication.md#f23)
- [F14 菜单按键与显示](../../../Docs/hmi/operation.md#f14)
- [F24 会话可靠传输与回绕](../../../Docs/services/communication.md#f24)
- [F13 legacy与未接入元素](../../../Docs/control/elements-and-dormant-code.md#f13)
