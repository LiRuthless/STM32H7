# 文档整理前治理记录

日期：2026-11-05；状态：历史冻结。下列原文保留整理前的用户决定、日期、提交号及冲突，不是当前授权或进度入口；现行规则见AGENTS，阶段状态见roadmap。旧路径/命令只适用于原环境，不批量改成当前路径。

## AGENTS.md

<details>
<summary>整理前原文（冻结原文，不作为现行规则）</summary>

~~~~~markdown
# ESP32Bridge 协作入口

当前执行状态（2026-10-20，优先于后文历史）：用户明确恢复Phase05；阶段内部连续实施与验证，每Phase完成后统一交审，05审查通过才进入06。规约先行并独立提交，新增外部协议/资源设计先确认；只改ESP32Bridge，不烧录、不复位、不push、不合并、不安装/切换SDK。开工09feab0包含00331c9；Phase01～04本端软件已交付，实机只确认启动/AP/DHCP，修复后网络复测待验。Phase05资源表R-05待确认，实现尚未开始。见[开工核对](docs/validation/phase-05-preflight.md)。

## 历史记录（不作为当前进度入口）

2026-10-18最新用户指令优先：**完成一个Phase审查一次**，取消阶段内部逐组人工审查/提交等待；组内继续按顺序实现、运行验证并可本地提交，阶段软件交付后统一请用户审查再推进下一阶段。规约先行、纯规约独立提交及未决外部协议/资源设计明确确认仍保留。用户同时明确采用[G-06](specs/features/03-spi-slave/g-06-task-json.md)回收后SPI任务有界解析。后文逐组门禁为历史约定，按此更新解释。

2026-10-18当前决定：[G-01立即HELLO_ACK/BUSY](specs/features/04-network-services/g-01-busy.md)、[G-03首个缺失源事件锚点](specs/features/05-bridge-lifecycle/g-03-event-gap.md)已获用户分别明确确认。本轮先独立纯规约提交。后文“待确认”为历史记录，方案门禁已解除，代码/三端核对仍待执行；逐组/逐阶段审查不变。

本目录采用规约驱动开发（SDD），只治理 ESP32-S3 SPI/Wi-Fi 桥接器。开始工作前依次阅读：

当前执行状态（优先于后文历史记录）：Phase 03/04已分别按阶段审查确认提交2335b3d/b7f6c67，实机/G-04/G-05待验。分支codex/04-network-services。**用户指示Phase 05先不推进**；Phase 05～06及完整手册/负载工具未完成，不能宣称整机桥接可用。

1. [使命](specs/mission.md)。
2. [技术栈](specs/techstack.md)。
3. [路线图](specs/roadmap.md)。
4. 当前阶段的 plan / requirements / validation，以及 [SCB/1 契约](specs/protocol/scb-v1.md)。

## 当前授权与工作规则

- 继续指令（2026-10-17）：用户要求“继续完成剩下的五个”，按Phase 02～06理解；先补齐Phase 01 Group 2/3，分组/逐阶段审查、必要规约增量先确认并独立提交及G-01～G-03门禁不变。上次Group 1已随本次继续指令核对清单并提交为`8750d6a`；Group 2增量已获用户确认并先独立提交为73a2a2f；对应启动/固定池已随2026-10-18继续指令复核并提交为a2947a8；当前Group 3脚本与说明实施/验证，完成后仍交用户审查再提交。此确认不批准G-01～G-03共享协议编码。
- 最新授权（2026-10-17）：用户启动 Phase 01，已提交的 `5c15c49` 为实施规约基线；此前“仅文档”限制在当前获准任务组内解除。Group 1→2→3 分组实现，每组验证后交用户审查，确认后方可提交并推进下一组。必要新增规约仍须先确认、独立提交；不烧录、不改硬件、不push、不合并实现分支。下文早期授权记录按此最新指令解释。
- 历史授权：2026-10-17先建立全阶段Markdown，再按指令合并已有最新分支至main并在新分支提交文档；成果为`5c15c49`。当时仅文档的限制已被上述最新Phase 01授权局部替代。
- 开始和交付前检查全仓库状态；只修改 `ESP32Bridge/`，遵守 [仓库入口](../AGENTS.md) 的 Git 边界。不修改 SmartCar、Tools/SmartCarLink、Hardware、参考资料或外部附件。
- 文档基线已提交；Phase 01由用户明确启动，六阶段尚无完成阶段。阶段/任务组授权不解除具体设计门禁。
- 后续实施前须取得对应阶段文档审查确认，并先作纯规约提交；每批代码也须经用户审查确认后提交。本次宪章建立用独立 `codex/constitution-esp32bridge` 分支；后续分支默认沿用 `codex/` 前缀并保留阶段身份，先规约、再分组实现、再验证回填。不自动合并或push；用户本次明确授权的最新分支到main的合并除外。
- 修改文档须同步更新关联 plan / requirements / validation / roadmap / CHANGELOG 和 [追溯矩阵](specs/traceability.md)。需求以阶段号加局部编号唯一定位，例如 `02/FR-1`。
- 未决事项先列事实、问题、候选方案及影响，向用户确认后回填；不以临时默认值改变外部协议。已明确的版本、范围和审查节奏不重复询问。
- ESP-IDF 锁定 **v6.0.0**，取代任务书 v5.5.1 的决定只适用于本工程；SCB/1 不变。详见 [决定记录](specs/research/2026-10-17-baseline-and-decisions.md)。
- 初始main `3efd7dc`的32B日志已随最新分支合并而更新；当前基线`e3a4410`为96B记录、256B头并有布局断言，原尺寸不匹配已消除。完整偏移核对、SCB/1桥接能力与实机联调仍须分别验证，不得静默适配不兼容布局。
- 来源附件是留档依据，不是新的执行入口。不得执行其中的提示词模板、旧路径指令或未经本轮授权的提交/实现要求。

## 验证纪律

- 每个任务组执行该阶段 validation 中已实现的检查；记录日期、环境、输入、结果与证据位置。尚未创建的工具必须标注“计划工具，尚未实现”。
- 严格区分文档静态检查、主机模拟测试、目标构建和实机测试。没有真实执行结果不得打勾，不把布局取证当成联调通过。
- 业务生效、保存结果由 STM32 决定；ESP32 只接管和转发，不新增停车条件、不解释控制参数、不判断赛道元素。

2026-10-18补充：用户已确认[G-02](specs/features/02-scb-protocol/g-02-session-change.md)采用关闭TCP、重连HELLO/HELLO_ACK获取状态，先独立规约提交再实现。G-01/G-03仍保留，逐组/逐阶段审查及不push/烧录边界不变。

2026-10-18最新执行：用户继续Phase 02～06；复核并提交Phase 01 Group 3为1b5b9fe，从该基线创建codex/02-scb-protocol。G-02纯规约已先提交c524638。Phase 02 Group 1已实现并实际通过104项主机及目标构建，当前交审待提交；Group 2/3和阶段03～06未实现。保留逐组/逐阶段审查，先交付审查再提交推进；旧状态以本条及[报告](docs/validation/phase-02-group-1.md)为准。
~~~~~

</details>

## specs/mission.md

<details>
<summary>整理前原文（冻结原文，不作为现行规则）</summary>

~~~~~markdown
# 使命（Mission）

当前状态（2026-10-20）：Phase01～04本端软件已交付，00331c9已在HEAD，221项回归本轮重跑通过。实机仅启动/AP/DHCP已验，修复后网络复测待验。用户已恢复Phase05，Phase末交审，05审查通过才进入06。资源方案R-05待确认，实现尚未开始；[开工核对](../docs/validation/phase-05-preflight.md)是本轮事实入口。后文旧日期进度、旧逐组门禁和待提交描述均为历史，不覆盖本段。

当前Phase 04网络服务软件已真实接入生产启动（SoftAP/单控制TCP/订阅UDP/G-01 BUSY/G-02会话联动），220项全回归/目标build/size通过，阶段末统一交审；硬件实测和Phase 05～06仍未完成。[阶段报告](../docs/validation/phase-04.md)为当前状态，以下旧组交付记录按时间保留。

2026-10-18最新审查节奏：用户明确每完成一个Phase审查一次，阶段内部验证后可提交/连续推进；本文件旧逐组人工等待约定以此替代。未决外部协议/资源设计仍需明确确认，规约必须先独立提交。

当前已推进Phase 03 Group 1真实SPI初始化及app_main启动接入，154项回归及目标build/size通过，待审提交；SPI事务/协议服务、网络/桥接/最终验证工具手册尚未完成，完整阶段实机验收不勾选。

Group 3本轮：本端候选向量/独立核验已交付，145项回归通过，三端对齐及真实适配未完成；本组与Phase 02本端软件交用户审查后再推进Phase 03独立实现，不标记完整目标完成。

2026-10-18：G-01立即HELLO_ACK/BUSY及G-03首个源事件锚点已获用户明确确认；先独立规约提交，再实施，逐组/逐阶段审查不变。

> 2026-10-17 文档，已获本次提交授权；治理范围仅为 ESP32Bridge，实施仍需按阶段授权。

最新状态：用户已要求继续余下五阶段，按Phase 02～06理解；先补齐Phase 01剩余任务组，并保留分组/逐阶段审查。此前“本轮仅文档”为历史授权；当前按门禁推进工程、配置、测试和验证回填，六阶段范围与职责不变。

## 愿景

将 ESP32-S3-DevKitM-1 作为 STM32 智能车与电脑之间可验证、可追溯的 SPI/Wi-Fi 桥接器，为实时上传、调参结果及日志导出提供独立传输通道。

## 目标用户 / 受众

- 参赛开发者：连接热点，使用配套电脑端观察数据、提交调参和导出日志。
- 联调人员：依据公共协议、测试向量和逻辑分析仪记录定位链路问题。
- 维护者与审查者：通过规约追踪需求、模块、版本决定及真实验证状态。

## 范围

### 范围内

- SCB/1 显式编解码、会话和独立可靠传输；有界、非阻塞于网络的 SPI 服务。
- WPA2 SoftAP、单控制会话 TCP、订阅 UDP、可靠业务和遥测的独立背压与统计。
- 透明转发 STM32 命令、结果、事件和日志块；ESP32 本地处理 HELLO/订阅等链路职责。
- 固定缓冲、故障恢复、模拟对端验证、后续构建脚本和中文交付说明。
- 按六阶段路线图分组实施、逐阶段审查；当前先完成Phase 01，再顺序推进其余五阶段。

### 范围外

- 车辆参数含义、元素判断、覆盖许可、生效拍、Flash 保存、停车策略和起跑互斥的决策。
- SmartCar、电脑上位机产品、硬件设计及参考资料目录的改造。
- PSRAM 依赖、整份 Flash 日志缓存、断线自动重放设置、掉电日志恢复和自动复位 STM32/ESP32。
- 单端修改 SCB/1、虚构对端完成状态或用主机模拟结果代替实机验收。

## 语气与风格

使用简洁、严谨的中文，保留 API、字段和单位原文。事实、目标、用户决定和未执行验证分开描述；采用 Markdown 相对链接，需求与验证可双向定位。

## 成功标准

2026-10-18本轮新增证据：Group 2可靠窗口/会话的生产C主机及真实目标归档已验证，待审查提交；真实SPI/网络/透明桥接、客户端、60秒负载及最终中文手册尚未交付。阶段仍不勾选完成。

1. 任务书所有本端职责均能定位至编号需求、计划模块和验收项，公共协议无未授权差异。
2. SPI 不等待网络；慢 TCP、UDP 拥堵或断线只造成受控上传丢弃/连接恢复，不增加车辆停车条件。
3. 接管 ACK、STM32 RECEIVED/APPLIED 及持久化结果保持不同语义，不伪造生效或保存成功。
4. 后续具备 v6.0.0 真构建、独立协议向量及模拟验证证据；实机部分只有取得测量数据才完成。
5. 文档先审查、规约先提交、实现和验证分步回填；当前获准任务组实施后先交用户审查，确认后在独立分支提交，不push。

## 决策与治理

本轮用户决定见 [基线与决定](research/2026-10-17-baseline-and-decisions.md)。任务书 v5.5.1 被 v6.0.0 替代；协议不变。后续任何接口矛盾或跨端依赖须先向用户明确说明，不把来源任务书中的“用户已确认”当作新授权。宪章变更独立分支、纯文档审查提交；功能循环按 [AGENTS](../AGENTS.md) 执行。

2026-10-18执行更新：Phase 01三个软件组已提交，实机验收待验；Phase 02三个本端软件组已提交，三端向量未核对；Phase 03 SPI服务/协议已审查提交2335b3d，实机待验；Phase 04网络服务软件已实现待审查提交，实机Wi-Fi未验。G-01/G-02/G-03本端方案均已确认，G-01/G-02已实现，G-03待实现；三端兼容待核。
~~~~~

</details>

## specs/techstack.md

<details>
<summary>整理前原文（冻结原文，不作为现行规则）</summary>

~~~~~markdown
# 技术栈（Tech Stack）

当前核对（2026-10-20）：锁定已有ESP-IDF v6.0.0。开工代码的最近产物e861be93b9bc4471a8122e4a5fbb14ae已核验30项源码及全部产物哈希；221项主机回归本轮重跑通过。SPI/NET生产服务已存在，Phase05消费者/生命周期仍待实施，新增资源表先确认。实机启动/AP/DHCP已有证据，运行期资源及桥接吞吐待验。后文阶段组进度为历史，当前以[开工核对](../docs/validation/phase-05-preflight.md)为准。

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
~~~~~

</details>

## specs/roadmap.md

<details>
<summary>整理前原文（冻结原文，不作为现行规则）</summary>

~~~~~markdown
# 路线图（Roadmap）

## 当前状态（2026-10-20）

用户解除Phase05暂停，按Phase末统一交审恢复推进，05审查通过才允许06。Phase01～04本端软件已交付，00331c9包含在09feab0中；本轮221项回归通过，最近已有目标构建的源码/产物哈希相符。实机仅启动/AP/DHCP已验，修复后HELLO保持/BUSY/重连/无合法会话拒绝待复测。[开工核对与烧录步骤](../docs/validation/phase-05-preflight.md)。

Phase05资源方案R-05待用户确认，生产消费者/可靠队列/导出生命周期尚未实现。Phase06负载故障工具、三端实机与完整中文手册未交付。当前SmartCar已存在SCB协议/命令/导出实现入口，旧无入口结论仅适用于当时基线；三端对齐仍待验。

## 历史进度快照（旧表状态不覆盖上述当前状态）

2026-10-18，分支`codex/04-network-services`。Phase 04网络服务软件已审查提交b7f6c67（220项全回归及v6.0.0目标build/size通过）；[Phase 04交审](../docs/validation/phase-04.md)及[最终证据](../docs/validation/phase-04-evidence.json)。**用户指示Phase 05先不推进。** 全部实机待验，不勾选整体完成。

Phase 02三个本端软件组已提交455f817/a0973a0/3c830de，固定候选向量三端未核对；Phase 03已审查提交2335b3d。当前SPI可独立握手/交换，网络服务可提供热点/单控制TCP/订阅UDP；生产桥接消费者尚未实现，除SUBSCRIBE外的可靠业务不ACK，SPI遥测端口留待Phase 05绑定。Phase 05～06、真实透明桥接及最终验证工具/中文实机手册仍未完成；不烧录、不push、不合并。

## 阶段总览

| 阶段 / 身份键 | 交付目标 | 依赖 | 规模 | 状态 / 三件套 |
|---|---|---|---|---|
| 01 `01-project-foundation` | 工程基础、配置和无空格路径构建 | 文档审查及纯规约提交 | small | [ ] 三个软件组已提交，实机验收待验；[计划](features/01-project-foundation/plan.md) / [需求](features/01-project-foundation/requirements.md) / [验证](features/01-project-foundation/validation.md) |
| 02 `02-scb-protocol` | 编解码、会话、可靠性及共同向量 | 01；三端向量基线核对 | medium | [ ] 本端三个软件组已审查提交，三端对齐及真实适配待验；[计划](features/02-scb-protocol/plan.md) / [需求](features/02-scb-protocol/requirements.md) / [验证](features/02-scb-protocol/validation.md) |
| 03 `03-spi-slave` | DMA 从机和 READY 时序 | 01、02 | medium | [ ] 本端SPI服务/协议软件已审查提交2335b3d，实机待验；[计划](features/03-spi-slave/plan.md) / [需求](features/03-spi-slave/requirements.md) / [验证](features/03-spi-slave/validation.md) |
| 04 `04-network-services` | SoftAP、单控制 TCP、订阅 UDP | 01、02；可用模拟 SPI | medium | [ ] 软件已实现待阶段审查，220项回归及目标build/size通过，实机未验；[计划](features/04-network-services/plan.md) / [需求](features/04-network-services/requirements.md) / [验证](features/04-network-services/validation.md) |
| 05 `05-bridge-lifecycle` | 接管背压、队列、会话生命周期、导出和故障统计 | 02、03、04 | medium | [ ] 待实现；[计划](features/05-bridge-lifecycle/plan.md) / [需求](features/05-bridge-lifecycle/requirements.md) / [验证](features/05-bridge-lifecycle/validation.md) |
| 06 `06-validation-delivery` | 故障/吞吐验收、构建证据及中文交付 | 01～05；真实联调另有布局门禁 | medium | [ ] 待实现；[计划](features/06-validation-delivery/plan.md) / [需求](features/06-validation-delivery/requirements.md) / [验证](features/06-validation-delivery/validation.md) |

## 当前状态与门禁

- 按用户最新指令，Phase末统一审查一次，阶段内部不再逐组等待；Phase 03软件已审查提交2335b3d，Phase 04软件已实现待审查。全部硬件验收仍待验。
- G-01立即HELLO_ACK/BUSY、G-02换SPI会话关闭TCP、G-03首个缺失源事件锚点均获用户明确确认；分别见对应阶段决定。只确认本端方案，三端兼容仍待核对。
- G-04：14条固定候选向量本端独立CRC/字段/SHA256与生产C检查通过，三端比对未执行。候选JSON的Git LF检出规则已修复，固定文件SHA256未变。
- G-05：只读核对SmartCar原头文件24项记录、29项头部偏移及96B/256B尺寸主机编译通过；未发现现有自维护代码具备SCB/1处理入口。真实协议能力、目标运行与电气SPI仍待验，不改SmartCar。
- 所有实机项目未执行；不烧录、不push、不合并、不自动EN复位。缺硬件不阻止其余独立实现/软件验证。

## 已合并 / 已取消的计划与决定

- 2026-10-17：采用独立 ESP32Bridge 文档体系，不套用 SmartCar 的功能编号，不修改其他目录。
- 2026-10-17：用户选择沿用本机 **ESP-IDF v6.0.0**，替代来源任务书 v5.5.1；SCB/1 仍为 v1。
- 2026-10-17：用户要求本次先文档、不生成代码，并选择全部阶段完整规约、保留逐阶段审查。暂无已合并或取消的功能阶段。
- 2026-10-17：用户确认G-01～G-03仅登记为实施前必须确认项，本轮不补协议草案，保持SCB/1原文（D-06）；相关功能及验证仍为待实现/未执行。
- 2026-10-17：用户新指令授权将GitHub最新分支合并到本地main，再创建新分支提交本次文档（D-07），替代此前暂不提交的限制。`feature/15-comment-documentation`以24个提交快进至`e3a4410`；不push。同步取证发现96B/256B尺寸已对齐，保留初始32B历史，不新增固件实现。

2026-10-18门禁更新：用户已明确[G-02关闭TCP方案](features/02-scb-protocol/g-02-session-change.md)，先纯规约提交，再实现会话层动作及后续网络关闭；G-01/G-03仍待确认，全部关联验证仍待执行。历史“G-01～G-03未确认”按此更新解释。
~~~~~

</details>

## README.md

<details>
<summary>整理前原文（冻结原文，不作为现行规则）</summary>

~~~~~markdown
# ESP32Bridge

ESP32-S3-DevKitM-1 的 SPI 从机与 Wi-Fi 桥接工程。面向智能车开发者、联调人员和后续维护者，负责 SCB/1 传输，不承担车辆控制决策。

## 当前状态

2026-10-18，当前分支`codex/04-network-services`。Phase 04网络服务软件已实现并接入生产启动：真实WPA2 SoftAP、单控制TCP:5000及G-01立即BUSY、有界TCP收发与独立可靠实例、本地SUBSCRIBE、订阅UDP遥测出口及G-02会话联动、状态快照；**本次220项全回归、v6.0.0目标build/size通过，待本次阶段审查提交**。见[Phase 04统一交审](docs/validation/phase-04.md)及[最终产物索引/哈希](docs/validation/phase-04-evidence.json)。Phase 05透明桥接/导出与Phase 06工具/最终中文实机手册未完成；生产尚无桥接消费者，除SUBSCRIBE外的可靠业务背压不ACK，SPI遥测端口留待Phase 05绑定。14条[本端候选向量](tests/vectors/scb-v1.md)三端待核对，全部硬件实测（含真实热点/DHCP/抓包）待验；未烧录。按用户最新要求，阶段内部连续推进，仅Phase末审查一次。

| 文档 | 用途 |
|---|---|
| [AGENTS.md](AGENTS.md) | 阅读顺序、授权与工作规则 |
| [CHANGELOG.md](CHANGELOG.md) | 文档及后续功能变更记录 |
| [mission.md](specs/mission.md) | 使命、受众、职责与成功标准 |
| [techstack.md](specs/techstack.md) | ESP-IDF v6.0.0、硬件和技术约束 |
| [roadmap.md](specs/roadmap.md) | 六阶段顺序、状态与依赖 |
| [traceability.md](specs/traceability.md) | 任务书到需求、计划模块和验证项的覆盖 |
| [SCB/1](specs/protocol/scb-v1.md) | 共享协议原文及适用说明 |
| [来源索引](specs/references/README.md) | 两份附件原文副本、来源、哈希与解释优先级 |
| [基线与决定](specs/research/2026-10-17-baseline-and-decisions.md) | 用户确认、仓库现状和联调阻塞 |
| [工具链核对](specs/research/2026-10-17-idf-v6-compatibility.md) | 本机 v6.0.0 事实及官方参考 |
| [01 工程基础](specs/features/01-project-foundation/plan.md) | [需求](specs/features/01-project-foundation/requirements.md) / [验证](specs/features/01-project-foundation/validation.md) |
| [02 SCB/1 协议](specs/features/02-scb-protocol/plan.md) | [需求](specs/features/02-scb-protocol/requirements.md) / [验证](specs/features/02-scb-protocol/validation.md) |
| [03 SPI 从机](specs/features/03-spi-slave/plan.md) | [需求](specs/features/03-spi-slave/requirements.md) / [验证](specs/features/03-spi-slave/validation.md) |
| [04 网络服务](specs/features/04-network-services/plan.md) | [需求](specs/features/04-network-services/requirements.md) / [验证](specs/features/04-network-services/validation.md) |
| [05 桥接与故障](specs/features/05-bridge-lifecycle/plan.md) | [需求](specs/features/05-bridge-lifecycle/requirements.md) / [验证](specs/features/05-bridge-lifecycle/validation.md) |
| [06 验证与交付](specs/features/06-validation-delivery/plan.md) | [需求](specs/features/06-validation-delivery/requirements.md) / [验证](specs/features/06-validation-delivery/validation.md) |

## 已确定的接口

目标 `esp32s3`，不依赖 PSRAM。SPI2_HOST：CS GPIO10、SCK GPIO12、MOSI GPIO11、MISO GPIO13、低有效 READY GPIO14；Mode 0，每次交换 1024B。网络：WPA2、`SmartCar-XXXX`、`192.168.4.1/24`、DHCP、TCP 5000、由当前 TCP 对端订阅 UDP 端口（电脑默认 5001）。完整约定以 [协议文档](specs/protocol/scb-v1.md) 为准。

本工程锁定本机 **ESP-IDF v6.0.0**，路径 `D:\esp\v6.0\esp-idf`。原任务书 v5.5.1 是来源基线，已被用户明确替代；既有安装保持不变。

## 后续构建、烧录与联调入口

Group 1提供CMake工程及 `scripts/check_config.py` 输入检查。按 [配置说明](config/local-config.example.md) 在本机准备被忽略的配置；在已核验的v6.0.0环境运行 `python -m unittest discover -s tests -v`。正式步骤见 [中文构建说明](docs/build-and-flash.md)，入口为 `scripts/activate.ps1` 和 `scripts/build.ps1`，Group 3验证见 [报告](docs/validation/phase-01-group-3.md)。当前AP/socket/SPI完整服务和烧录仍未交付/授权；以下涉及后续阶段的要求继续保留：

1. 在 v6.0.0 环境验证 `idf.py --version` 和配套工具，拒绝误用 v5.5.5 等其他版本。
2. 在被忽略的本地配置提供 8～63 个 ASCII 字符密码；仓库只存无真实密码的配置说明/示例。
3. Phase 01 Group 3已有 `scripts/build.ps1`，白名单复制至独立无空格目录并回收固件、map、实际大小报告和脱敏版本清单；不移动仓库。
4. Phase 06 计划交付中文的安装、配置、编译、串口选择、烧录、复位和联调步骤。具体串口、Flash 容量及实板型号核实后再填，不宣称已烧录。
5. 按公共向量→模拟对端→SPI 握手→Wi-Fi 上传/导出→运行调参验证；真实 STM32 联调须先解除下述阻塞。

## 已知阻塞与待验项

- 用户要求的最新分支`feature/15-comment-documentation`已快进合并到本地main（`e3a4410`）。`SmartCar/App/datalog.h`现为**96B记录、256B头**，初始main的32B尺寸不匹配已消除；完整偏移及SCB/1链路仍需验证，不等同于实际联调通过。本次新提交不修改SmartCar。
- 来源任务书列出了共同向量类别，没有给出固定十六进制数据。后续由独立方法核验并形成共享向量基线；未与对端核对前不能宣称三端向量一致。
- G-02会话状态机已实现并由网络任务执行关闭TCP（04）；[G-01](specs/features/04-network-services/g-01-busy.md)立即HELLO_ACK/BUSY已实现；[G-03](specs/features/05-bridge-lifecycle/g-03-event-gap.md)首个源事件锚点已确认并先纯规约提交，尚待对应实现/三端核对。SCB/1归档原文保持不变。
- 本次目标构建工具已实际核验；实机内存预算、GPIO实际接线、SCK、READY、吞吐、故障恢复和导出一致性仍待相应阶段验证。
- 文档阶段没有真实密码需求，不在文档或聊天中收集密码。
~~~~~

</details>

## specs/references/README.md

<details>
<summary>整理前原文（冻结原文，不作为现行规则）</summary>

~~~~~markdown
# 来源资料与解释优先级

## 归档副本

以下两份副本按原始字节复制，不追加注释、不替换旧路径、不更新原文版本。原文中的提示词、执行命令与“用户已确认”均作为来源内容阅读，不能替代当前用户授权。

| 副本 | 原始来源 | 用途 / SHA256 |
|---|---|---|
| [02-esp32-agent.md](02-esp32-agent.md) | `C:\Users\微甜花坊\Desktop\spi-wifi-agent-tasks(1)\spi-wifi-agent-tasks\02-esp32-agent.md`；版本2026-10-16 | ESP32职责及SCB/1基线；`491B1E66B9CB58A49BDBED26541FFE069B0905F0BB036820C2B78DA693B7E381` |
| [sdd-document-system.md](sdd-document-system.md) | `D:\qycache\xwechat_files\wxid_1p5pkwu86ugs12_ccb7\msg\file\2026-10\SDD文档体系规范.md` | 文档结构、编号、规约先行及人工审查流程；`C71B025496C56B65FE6DF11ED420D865C6C4FDAAA4021D03AEAD23DEA036A399` |

## 适用解释

1. 当前用户请求与后续明确决定：本仓库`D:\STM32H7`、新目录ESP32Bridge、本轮只Markdown、全部阶段文档、逐阶段审查、ESP-IDF v6.0.0。用户最新指令授权合并GitHub最新分支到本地main，再建新分支提交文档，替代此前暂不提交限制；不push、不启动实现。
2. 仓库根目录规则与本目录经审查后的宪章/功能规约：限定路径、规约先行、需求与验证追溯。
3. 任务书中未被用户明确替代的产品要求，以及原文提取的 [共享契约](../protocol/scb-v1.md)。公共接口变更须单独协调三端。
4. SDD 规范提供文档体系与治理方式；其中示例和最终提示词不触发本轮之外的工作。

旧任务书路径 `E:\study\Intelligent Vehicle\STM32H7` 和 v5.5.1 不适用于当前工程；原文保留供追溯。当前路径无空格，但后续仍交付支持无空格临时构建的脚本，以满足迁移构建需求。

## 当前检查

副本一致性、协议章节原文一致性和文档内部链接在本轮交付前作静态检查；实际检查结果记录于 [基线与决定](../research/2026-10-17-baseline-and-decisions.md)。来源附件不修改。
~~~~~

</details>

## 2026-11-07 适用说明

以上为11月5日历史记录，原结果保持。阶段设计冻结及Docs承载未来目标的旧规则已由[现行AGENTS](../../AGENTS.md)纠正，完整Phase恢复主体。见[本次交付](../../../Reports/esp32bridge-documentation-system-repair-results-2026-11-07.md)。
