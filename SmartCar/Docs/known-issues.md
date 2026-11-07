# 已知问题与当前状态

本页是当前问题入口。来源编号保留，详细选项/未来验收见[决策台账](../../Reports/smartcar-documentation-execution-plan/04-decisions-and-fix-backlog.md)。文档迁移不批准保留缺陷或执行修复。

| 问题 | 当前状态 | 主契约 |
|---|---|---|
| R01方向窄化 | 未修；合法增益域可超int16，先转再限 | [F08](control/tracking-and-controllers.md#f08) |
| R02/D02/H03阻塞与时基 | 未修/待设计；DL1B位于TIM6 | [F28](platform/board-and-bsp.md#f28) |
| R03 IMU错误发布 | 未修 | [F07](sensing/sensors-and-calibration.md#f07)、[F29](platform/board-and-bsp.md#f29) |
| R04旧UART分帧 | 未修；一次Read不等于帧 | [F15](services/parameters-and-tuning.md#f15) |
| R05/D03停车风扇 | 待决定应用时点 | [F15](services/parameters-and-tuning.md#f15) |
| R06活动PI/查询镜像 | P17明确保持，未来语义待定 | [F15](services/parameters-and-tuning.md#f15) |
| R07原样本 | 缺失；不能替换原SHA断言 | [F31](guides/validation.md#f31) |
| R08注释 | 已建立逐项复核，发现新事实仍需重审 | [F32](guides/validation.md#f32) |
| D01/M01 UART返回 | 文档与实现冲突，待决定 | [F27](platform/board-and-bsp.md#f27) |
| D05/H04 IOC栈 | 实际8KB，元数据1KB，本轮不改配置 | [F30](guides/build-and-flash.md#f30) |
| D07/M03首因 | 请求进入先后与锁存先后可不同 | [F02.STOP](system/runtime-and-safety.md#f02-stop) |
| D08/F24回绕 | 双端责任待明确 | [F24](services/communication.md#f24) |
| D09/M04停车文本 | 新摘要与旧精确文本兼容待定 | [F02.STOP](system/runtime-and-safety.md#f02-stop) |
| H01/H02旧验收、H05格式 | 由docs承接现行契约，Phase保留历史，不代表硬件已验 | [F17](services/parameters-and-tuning.md#f17)、[F19](services/recording-and-export.md#f19) |
| M05迁移重试保证 | docs收窄至实际RAM/擦写保证，未改实现 | [F17](services/parameters-and-tuning.md#f17) |
| M06 IMU旧CSV观察 | 当前无这两个字段，改为调试器/上下文审查方法 | [F07](sensing/sensors-and-calibration.md#f07) |
| D04/M02 SPI2描述 | 68cb508已更正，波形仍待验 | [F29](platform/board-and-bsp.md#f29) |
| D06 HOME显示、N01导航 | 本轮只修措辞/导航，原行为与历史证据保留 | [F14](hmi/operation.md#f14) |
| H06/M07/M08治理入口 | 本轮统一入口；验证政策以2026-10-31规则为准 | [F32](guides/validation.md#f32) |
| F04 fatal | 补齐现状，安全恢复设计未完成 | [F04](system/runtime-and-safety.md#f04) |
| P09/P10/P11未完成 | 元素/全局调参/部分健壮性与最终处置仍按原排期 | [F12](control/tracking-and-controllers.md#f12)、[F13](control/elements-and-dormant-code.md#f13) |

标定32项-1、AC5/实机待验是预期限制或未完成门禁，不伪装成软件测试通过。运行风险不因文档登记变成已解决。

## 2026-11-07源码与跨端复核

- 本仓已有[ESP32Bridge](../../ESP32Bridge/README.md)，其2026-11-05状态记录明确生产桥接消费者、完整队列/导出生命周期及Phase06最终验证尚未完成，HELLO保持/BUSY/重连等需实机复测。替代旧“没有桥接工程”陈述，不宣称三端闭环通过。
- Flash LogErase函数只擦首4KiB并设擦除前沿，旧源码注释的全区64KiB循环描述过时；以[原语说明](platform/w25q64-and-flash.md#f29-flash)登记事实，本次不改源码注释。
- ADC/PWM及部分初始化忽略HAL失败、IMU坏读无完整有效性传播、DL1B高优先级ISR阻塞时基、首因锁存先后、参数查询镜像滞后、非事务Flash写等既有风险保持未修。需要改变行为时按完整Phase设计决定，不由本次文档说明批准。
- 未知物理事实：16灰度左右/极性/串扰、编码器方向及每圈计数/脉冲到距离、ADC电池伏特阈值、元素门限整定、实际IRQ预算/波形/吞吐/栈仍需板测或用户实测依据。无新的行为设计选择在本次被擅自作出。
