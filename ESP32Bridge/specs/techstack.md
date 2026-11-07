# 技术栈与约束

> 2026-11-05核对本工程配置/源码及历史环境记录；未激活、安装或切换SDK。阶段进度不在此维护，见[roadmap](roadmap.md)。

## 平台与来源

| 类别 | 选型 / 边界 |
|---|---|
| 平台/语言 | ESP32-S3-DevKitM-1、esp32s3、C、ESP-IDF提供的FreeRTOS/lwIP |
| SDK | 锁定ESP-IDF v6.0.0；2026-10-17用户选择替代任务书v5.5.1，仅适用本工程 |
| 驱动/系统 | 对应SDK的esp_driver_spi、GPIO、esp_wifi/netif/event、nvs_flash、esp_timer |
| JSON | 自维护json_guard，有界UTF-8/重复键/全量消费校验；不引入新依赖 |
| 构建 | CMake/IDF工具；实际编译器/Python/CMake/Ninja版本以具体构建记录为准 |
| 验证 | 既有Python标准库unittest及生产C边界夹具，不等于目标硬件 |

activate.ps1当前仍固定D盘SDK路径，不存在时会阻塞；本轮仅文档，未改脚本。实际约束见构建指南。旧D盘环境结果属于[原环境记录](research/2026-10-17-idf-v6-compatibility.md)，不是当前安装状态。新增库、工具链版本或资源改变先明确影响及用户决定，不自动安装。

## 硬性约束

- 不依赖PSRAM；数据路径固定额度、启动分配。SDK内部动态分配另计，不声称系统完全无堆。
- SPI/网络各自任务持有协议实例及工作区，ISR不解析或做socket；DMA驱动持有期间应用不可访问。
- SPI不等待网络，网络故障不新增车辆停车/复位；车端业务及Flash决策属于STM32。
- 公共SCB/1不得单端静默修改，格式正文见[公共契约](../../SmartCar/Docs/services/communication.md#scb1)，本端适配见[协议功能页](../Docs/protocol/framing-and-links.md)。
- 密码本地忽略，不提交、不写日志/产物清单，非法不开放热点回退。
- 可靠ACK只表示合法接管，不能升级APPLIED/SAVED；断线不重放旧设置，旧身份不发给新连接。

## 功能、构建和标准入口

阶段设计映射完整保存在Phase；当前容量、引脚、时序、资源/owner由[Docs](../Docs/index.md)说明；[架构](../Docs/architecture.md)反查模块。
构建使用[既有入口说明](../Docs/guides/build-and-flash.md)，检查SDK、目标、配置，白名单复制至无空格临时目录并脱敏回收，不移动仓库。
代码/注释标准见[code-style](code-style.md)，执行政策见[AGENTS](../AGENTS.md)，证据方法见[验证指南](../Docs/guides/validation.md)。原治理版本见[历史](research/2026-11-05-governance-history.md)。

## 2026-11-07 现行文档体系

mission、techstack、roadmap、code-style及AGENTS共同约束需求、设计、实施与验收。体系为前层治理文档 → Phase → Docs ⇄ 代码。Phase完整保存背景、需求、详细设计、用户决定、计划、文件与符号映射、验证标准和实际结果，按阶段基线解释，不缩减为摘要跳转。Docs说明当前功能、算法、状态、时序、参数、错误和限制，与代码语义对应。目标草案和未开工组设计保存在Phase。

后续变更：Phase确认并独立提交 → 先更新本组Docs → 紧接实现 → 双向核对 → 匹配Docs与代码同批交付。短暂领先标明“本组实现同步中”、所属Phase/任务组及更新前代码基线，完成前取消。显式关闭阶段后的独立修改新增Phase；进行中的阶段可以补组与回填，不能覆盖历史决定及结果；软件完成但硬件待验不自动关闭。

本次仅文档纠偏，允许明确路径分组提交，不新增Phase07，不启动05/06，不改变ESP-IDF v6.0.0、任务边界、命名及本项目验证政策，不推送或烧录。