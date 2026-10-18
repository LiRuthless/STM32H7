# 技术栈（Tech Stack）

当前Phase 03最终：SPI实际事务/READY/codec-session-reliable已接入生产，178项回归、v6.0.0目标build/size通过；最终ELF两GPIO回调各16B且在.iram0.text、无call；静态栈6144B/协议状态2216B。真实运行堆栈/电气和网络消费待验，[阶段报告](../docs/validation/phase-03.md)。后文旧组结果为历史。

2026-10-18最新：阶段内部连续实现/验证/本地提交，Phase末统一人工审查一次。G-06允许回收后SPI任务有界解析，ISR仍只GPIO；[生命周期设计](features/03-spi-slave/lifecycle-design.md)先纯规约提交，固定资源/协议不变。

当前Phase 03 Group 1：真实Core1/优先级12/6144B静态任务接入启动与SPI2驱动；154项回归、v6.0.0真实build/size通过。最终ELF已含驱动/任务/启动符号；目标静态栈6144B、TCB100B、信号量84B、描述24B。事务服务/真实运行堆栈/电气测量仍未完成，[报告](../docs/validation/phase-03-group-1.md)。

Group 3本轮：固定本端候选向量14条，struct/zlib生成、独立偏移/逐位CRC核验、生产C测试，全回归145项通过。未改生产C，本组未重复目标构建，引用Group 2本轮已核验build/size；三端和真实适配待验。

2026-10-18：G-01/G-03已明确确认，见[决定记录](research/2026-10-17-baseline-and-decisions.md)D-15；历史待确认文字不再代表当前方案状态，仍须实现/验证及三端核对。

> 2026-10-17 文档，已获本次提交授权。版本核对不等于环境激活或目标构建通过。

最新实施状态：Phase 01三个软件组已提交8750d6a、a2947a8、1b5b9fe，实机待验。Phase 02 Group 1生产C编解码/JSON校验已实现，27项新增、104项全回归及v6.0.0目标build/size通过，见[本组报告](../docs/validation/phase-02-group-1.md)。尚未接入真实SPI/网络；可靠窗口、会话和共同向量留待后续组。G-02已独立规约提交c524638，G-01/G-03保留。当前授权以[AGENTS](../AGENTS.md)为准。

## 语言与运行时

- 目标为 ESP32-S3-DevKitM-1，`IDF_TARGET=esp32s3`，C，ESP-IDF 提供的 FreeRTOS/lwIP。
- **ESP-IDF v6.0.0（锁定）**，已读取本机 `D:\esp\v6.0\esp-idf\tools\cmake\version.cmake` 确认 6.0.0。
- 用户于 2026-10-17 明确替代原任务书 v5.5.1；不静默改用同时安装的 v5.5.5，不修改其他项目工具链。

## 框架与核心库

| 类别 | 选型 / 版本 | 用途 |
|---|---|---|
| SDK / 构建 | ESP-IDF v6.0.0，随该版 CMake 与工具清单 | 唯一目标构建基线 |
| SPI | `esp_driver_spi`，随 v6.0.0 | SPI2_HOST，DMA 自动分配 |
| GPIO / 中断 | v6.0.0 GPIO 驱动及 IRAM 安全接口 | GPIO14 READY 回调 |
| 网络 | v6.0.0 esp_wifi、esp_netif、esp_event、lwIP sockets | WPA2 AP、TCP、UDP |
| 存储 | v6.0.0 nvs_flash | SDK 初始化；不承担车辆日志持久化 |
| JSON | 项目内有界校验层；底层库须通过重复键、UTF-8、非有限数及全量消费测试 | 只验证传输结构，不解释车辆参数 |
| 主机验证 | Python 标准库 struct / zlib / unittest，版本在实际运行时记录 | 独立向量、模拟对端和故障注入 |

第三方 JSON 依赖不在本轮引入；若后续新增库须先审查并锁定具体版本。SDK 自带示例只供 API 对照，不继承其开放热点回退或密码打印行为。

## 数据存储与资源

- 协议最大负载 992B、头 32B、SPI 1024B；TCP 只发送头加有效负载，TELEMETRY 的 UDP 上限 1016B。
- DMA 固定池使用内部 DMA 可达内存，启动一次分配并检查，事务深度 1。不依赖 PSRAM，不在桥接数据路径反复 malloc。
- UDP 队列 16 包、EVENT 队列 32 条；可靠通道两侧独立，每方向至多一个可靠帧在途。可靠接管必须预留槽位。
- 记录契约为96B、日志头256B；合并后的SmartCar基线`e3a4410`已声明相同尺寸，并在datalog.c设有尺寸和关键偏移断言。原32B基线仅保留为历史，不能以尺寸对齐替代完整SCB/1及实机验证。
- 队列、任务栈和驱动内存预算由后续目标构建及启动测量确定，未测量不填猜测数值。

## 构建与工具链

- 正式构建入口为 [scripts/build.ps1](../scripts/build.ps1)，通过build_support白名单复制到独立无空格目录、回收规定产物并核对清理范围；不移动仓库。显式环境入口为scripts/activate.ps1，不自动安装/换版。
- SDK 环境激活及编译器/工具选择以 v6.0.0 的 `tools/tools.json` 和导出环境为准，不以 PATH 中碰巧出现的工具替代；实际版本、提交号及是否有 SDK 本地改动写入构建证据。
- 构建前必须核验版本、目标、配置和本地密码存在性。回收固件、map、大小报告及烧录参数；产物和密码本地配置须被忽略。
- 工具路径、接口与命令约定见 [Phase 01](features/01-project-foundation/plan.md)；Group 1已有工程配置和输入检查，Group 3现已交付复制构建入口并单列真实验证状态。

## 测试

2026-10-18本轮：139项全部通过（原104＋18可靠＋16会话/字段＋1工具错误诊断），当前源码隔离副本通过正式入口目标build/size；报告[Group 2](../docs/validation/phase-02-group-2.md)。新模块进入真实组件归档，但app_main未引用，被最终链接剔除，未宣称生产服务接入或最终资源预算。

先独立向量和纯协议测试，再双侧模拟，再目标构建与实机；详见各阶段 validation 和 [Phase 06](features/06-validation-delivery/validation.md)。Group 3本次执行文档检查、77项主机检查、两次正式入口目标构建及三项真实预期失败；Group 2历史59项另存，实机测量未执行，各类证据分别记录。

## 硬性约束

1. SCB/1 共享接口必须与 [契约](protocol/scb-v1.md) 一致；端序、CRC、长度和保留字段均显式校验，禁止直接发普通 C 结构体。
2. SPI Mode 0、8bit、MSB first、全双工；GPIO10/12/11/13，READY GPIO14 低有效；未使用的扩展数据引脚置 -1，GPIO14 不作 WP。
3. SPI 任务 Core1 / 优先级12，在该核初始化驱动；网络任务优先级5；ISR/回调只作 IRAM 安全 GPIO 和轻量通知，不做解析、socket 或阻塞。
4. SPI 驱动持有缓冲期间应用不得访问；回收后检查 `trans_len==8192`，非法帧不执行、不确认；重排成功再允许 READY。
5. SPI 不等待电脑/发送 socket；网络断开不停止 SPI、不改变 STM32 已应用值或车辆安全策略。
6. WPA2 密码 8～63 个 ASCII 字符，缺失/无效明确报错，不开放回退；实际密码不提交、不输出日志。
7. 只允许一个 TCP 控制会话；UDP 目的 IP 只来自该会话对端；不接受第三方目的 IP，不缓存整份日志。
8. 重发 200ms、至多三次、窗口1；容量不足不得先 ACK 再丢可靠业务。ACK 与 APPLIED/SAVED 分离。
9. 不为断线重放旧设置，不自动复位 ESP32，不新增停车条件；调试打印只走本地串口。
10. 未审查规约不实施；未取得真实结果不标记验证通过；仅实施当前获准任务组，验证后经用户审查确认再提交，不push。

## 目录与代码规范

文档文件名采用小写英文和连字符，入口 `AGENTS.md` / `README.md` / `CHANGELOG.md` 按体系约定。后续 C 模块使用任务书建议的 `app_main`、`protocol`、`spi_bridge`、`net_bridge`，内部模块划分在各阶段计划中说明；未来文件路径只用行内代码，不链接不存在的文件。功能编号与路线图固定对应。

## 依据

[本机与 v6.0 官方核对记录](research/2026-10-17-idf-v6-compatibility.md)；[来源与优先级](references/README.md)。
