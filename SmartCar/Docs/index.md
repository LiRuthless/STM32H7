# SmartCar 当前固件说明

2026-11-07对照代码基线 `8d5ab2faf68432ae03b2c1f91bc4593c7dc7cc3e`。前层治理 → 完整 Phase → Docs ⇄ 代码；功能语义对应不代表实机验收。

阅读顺序：[mission](../specs/mission.md) → [techstack](../specs/techstack.md) → [roadmap](../specs/roadmap.md) → [code-style](../specs/code-style.md) → 本次完整Phase → 本页当前能力 → 源码。开发执行见[AGENTS](../AGENTS.md)。

| 能力/F身份 | 当前启用条件与路径 | 限制与主说明 |
|---|---|---|
| 运行与安全 F01～04 | NONE正常启动；裸机前台与1/2/5ms三线；PC13起跑/切模式 | 双标定/双信号/保存/日志门禁；[完整起停与保护](system/runtime-and-safety.md)；fatal尚无统一安全恢复 |
| 感知 F05～07 | 正常2ms电感/灰度与姿态，1ms缓存原码 | [电感](sensing/electromagnetic.md)、[灰度](sensing/grayscale.md)、[姿态](sensing/attitude.md)；32标定仍-1，两模式禁止起跑 |
| 控制 F08/F10～12 | 默认光电低档；同拍双FSM观察、只选中模式执行 | [控制器](control/controllers.md)、[电机执行](control/motor-execution.md)、[双模式](control/tracking-modes.md)、[状态矩阵](control/profiles.md)；R01/部分覆写风险保留 |
| 元素/休眠 F09/F13 | 电磁只活动十字与右环，光电仅普通循迹 | [元素与休眠](control/elements-and-dormant-code.md)；左环/跷跷板/路障不得自行启用 |
| HMI F14 | 默认板载仪表；外屏宏=1才完整ADC菜单，停车更新 | [操作与步进](hmi/operation.md)；显示/活动控制/镜像值须区分 |
| 参数 F15～18 | 两档×两模式bank；上电Flash恢复，菜单/UART/SCB应用 | [持久参数](services/parameters.md)、[完整拍临时调参](services/runtime-tuning.md)；镜像可能滞后，掉电写非事务 |
| 记录 F19～22 | 起跑必须日志可用；TIM15采样、前台批写、停车导出 | [格式/队列/CSV/二进制](services/recording-and-export.md)；只支持当前会话，断电恢复不具备 |
| 通信 F23～25 | STM32 SCB帧、可靠会话、独立上传 | [通信](services/communication.md)；ESP32源码存在但生产闭环未完成，不宣称联调通过 |
| 平台 F26～29 | ADC/编码器/PWM、UART、DL1B及资源/驱动 | [平台总览与子主题](platform/board-and-bsp.md)；HAL阻塞/错误传播限制保留 |
| 工程 F30～32 | GCC现有入口、AC5锁定、生成区/BSP接管 | [构建烧录](guides/build-and-flash.md)、[验证/R08](guides/validation.md)；不自动烧录 |
| 独立板测 F33 | BOARD_TEST_SELECT=1～15进入最小初始化，与正常链隔离 | [选择、机制与操作](platform/isolated-board-tests.md)；默认NONE=0，所有真实板测待验 |

[参数反查](parameters.md) · [架构/源码反查](architecture.md) · [Phase/FR/父子功能追溯](traceability.md) · [已知问题](known-issues.md) · [分类维护契约](maintenance.md)

R07原始样本、AC5、真实标定、实测栈、700/900μs、SPI波形、持续吞吐和实车均保持待验。没有因文档交付补勾任何阶段。正常运行、独立板测、休眠入口分别说明；目录存在和可编译入口不是硬件通过证据。
