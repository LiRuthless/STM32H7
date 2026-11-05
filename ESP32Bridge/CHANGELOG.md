# Changelog

## [Unreleased]

### 2026-11-05 文档体系与代码/注释标准

- 建立Phase → Docs → 代码，新增十个语义功能主条目、模块反查、58条FR/TC与3条R6增量导航、问题及验证入口；六阶段身份不变。
- 参考STM32通用排版和中文Doxygen/C完整H摘要规范建立ESP32标准，保留bridge_/scb_命名；未整改源码、未引入R08。
- 迁移Docs及构建指南，冻结旧设计/治理/协议基线，保留原日期、计数、哈希和JSON；修复失效datalog导航，原重复M-27按用途区分而不改号。
- R-05批准记录冲突、导出计时歧义、桥接模块/原产物缺失及实机待验均保留；本轮未推进Phase05/06、未构建/运行行为回归/烧录、未暂存/提交/推送。
- 文档检查与范围结果见[整理记录](specs/research/2026-11-05-document-system-alignment.md)。


- 2026-10-20：按先行规约bd1ee7b增加零SPI会话HELLO保持2秒→第二连接BUSY→断线重连→无会话业务拒绝→再次HELLO的生产C连续流程回归；222项全部通过。隔离副本撤回00331c9保护后两项相关用例预期失败，证明能捕获原缺陷；无生产C改动、无实机操作。资源表R-05仍待确认，Phase05实现未开始。

- 2026-10-20：用户解除Phase05暂停，恢复按Phase末统一交审；开工09feab0含00331c9，221项回归本轮重跑通过。核验最新已有v6.0.0产物30项源码及全部产物哈希；修复后实机复测无新证据，仍待验。更新当前状态与SmartCar已有SCB入口事实，补[产物、人工烧录及复测步骤](Docs/validation/phase-05-preflight.md)。Phase05资源方案待确认，实现未开始；Phase06未推进。

- 2026-10-20：实机冒烟发现并修复Phase 04实现缺陷：无SPI会话（session_id=0）时TCP可靠实例为死链，`link_service`把死链轮询的`SCB_LINK_REBUILD`与“真实会话重发耗尽”混同，HELLO_ACK发出后立即误关状态查询连接，致G-01 BUSY拒绝无法观察（第二连接被当作新控制连接）。修复为仅非零会话的链路故障才关TCP并计link_failures；零会话死链保持连接、不计故障，业务帧仍按无合法会话拒绝关闭。主机测试补send后推进循环断言（原测试在send后即结束，存在盲区）及无会话业务帧拒绝用例；[network-design](specs/features/04-network-services/network-design.md)FR-5段先行明确该语义，SCB/1不变。221项全回归通过、目标v6.0.0构建通过；待审查提交，重新烧录后复测BUSY冒烟。

- 2026-10-20：首次实机验证（用户执行，未由代理烧录）：Phase 04构建`bf24b81807e842f8b66fded4d9186e17`经COM3烧录启动，六阶段SUCCEEDED、固定池/双任务/监听/堆栈水位日志符合预期；电脑连接热点`SmartCar-2201`经DHCP取得192.168.4.2/24。SPI电气、BUSY抓包及协议对话仍待验；见[Phase 03](Docs/validation/phase-03.md)/[Phase 04](Docs/validation/phase-04.md)报告补充段。

- 2026-10-18：Phase 04网络服务软件已实现并接入生产启动：真实WPA2 SoftAP（SSID按MAC派生、显式192.168.4.1/24+DHCP、失败按序回收不开放回退）、Core0/优先级5静态网络任务、单控制TCP:5000及G-01立即HELLO_ACK/BUSY（独立缓冲/游标/500ms期限）、1024B有界收发与2000ms停滞回收、任务独占SCB_TCP可靠实例（精确ACK/200ms×3原图像重发/耗尽只关TCP）、新增link_session纯函数握手/BUSY编码入口、本地SUBSCRIBE（端口边界校验、对端IP固定）、≤1016B一报文一帧UDP出口（启动绑定、代际凭据、丢弃/失败归因计数）、G-02会话变化立即关TCP、只读状态快照。220项全回归（新增42项）、v6.0.0目标build/size及ELF符号/静态资源核对通过，统一[阶段交审](Docs/validation/phase-04.md)；实机Wi-Fi/DHCP/抓包与Phase 05/06未完成，不烧录。SPI遥测端口留待Phase 05绑定，其余可靠业务仍背压不ACK。

- 2026-10-18：Phase 03经阶段审查确认并提交2335b3d。Phase 04网络任务/缓冲/AP参数/单会话TCP/BUSY/订阅/UDP/会话耦合细化见 [network-design](specs/features/04-network-services/network-design.md)，先独立纯规约提交，未改SCB/1，实现未开始。

- 2026-10-18：Phase 03 Group 2/3按82613f6实现真实深度1事务/READY/所有权回收、严格长度及错误隔离、10ms有界等待、SPI握手与独立可靠协议、预留后ACK端口、离线丢弃及生产状态快照。178项全回归、最终v6.0.0目标build/size及IRAM回调反汇编审计通过，统一[阶段交审](Docs/validation/phase-03.md)；全部实机与Phase 04～06未完成，不烧录。

- 2026-10-18：按用户最新决定取消阶段内逐组人工等待，改为Phase末统一审查；G-06回收后SPI任务有界解析已确认。Group 1已保护提交712fb41，Group 2/3事务/READY/独立计时/协议接管细化先纯规约提交。

- 2026-10-18：Phase 03 Group 1真实Core1静态SPI任务、SPI2驱动/固定DMA/初始化错误回收已接入生产启动，154项回归和目标build/size通过，待审提交；READY高、事务循环未交付。新增本工程固定候选JSON的LF检出规则及哈希稳定性回归；[交审](Docs/validation/phase-03-group-1.md)。

- 2026-10-18：Group 3经用户确认提交3c830de；Phase 03 Group 1静态任务/真实驱动启动资源已获确认，先纯规约提交。固定向量JSON的core.autocrlf检出哈希风险同步先行规定LF修复。

- 2026-10-18：Group 3新增14条固定本端候选向量、完整hex/字段/CRC/SHA256、独立生成与逐位CRC核验工具、生产C向量/可靠重复测试；新增6项、全回归145项通过，待审提交。三端核对、实际SPI/socket适配与实机未完成。

- 2026-10-18：Group 2经用户明确确认提交a0973a0；Group 3固定候选向量及独立核验/生产C测试细化先纯规约提交。

- 2026-10-18本轮实现：Phase 02 Group 2独立可靠窗口、精确ACK、固定身份去重、实际发送完成后200ms/最多三次重发、SPI/TCP握手及G-02关闭待办；JSON精确字段读取及工具启动OS错误诊断。139项全回归与正式目标build/size通过，代码待审查提交，尚未接入真实SPI/socket；[交审报告](Docs/validation/phase-02-group-2.md)。

- 2026-10-18：用户明确G-01立即HELLO_ACK/BUSY及G-03首个缺失源事件锚点；先独立纯规约记录，未改归档协议/来源副本，代码及三端兼容验证待完成。

- 2026-10-18本轮：Group 1复核104项通过并提交455f817；先提交Group 2内部接口/独立计时/生产C验证及环境启动错误诊断细化，外部协议门禁不变。

### Added

- 2026-10-18：Phase 01 Group 3新增显式环境选择、正式复制构建/产物回收及安全清理、中文构建说明、文件系统与PowerShell错误路径测试；77项主机检查通过，正式入口集成结果单列于 [Group 3报告](Docs/validation/phase-01-group-3.md)。本组待审查提交，未烧录或改变共享协议。

- 2026-10-18：Phase 01 Group 2按纯规约73a2a2f实现内部固定池、代际/容量/峰值、DMA所有权与拒绝忙池销毁；实现可注入启动协调、READY保护、生产C密码校验及真实NVS/netif/event适配。59项主机检查及v6.0.0目标build/size通过；代码待用户审查提交，AP/socket/SPI完整服务和实机测量未完成。详见 [Group 2报告](Docs/validation/phase-01-group-2.md)。

- 2026-10-17：依据已提交规约 `5c15c49` 实施 Phase 01 Group 1：独立v6.0.0/esp32s3工程、显式SDK组件依赖、无PSRAM/双核配置、本地密码Kconfig及忽略规则、构建输入/最终配置检查、真实CMake拒绝路径测试和配置自检入口。待用户审查提交；启动/资源、复制构建脚本及后续阶段未实现。实际结果见 [Group 1证据](Docs/validation/phase-01-group-1.md)。
- 2026-10-17：建立ESP32Bridge独立SDD文档、六阶段规约、共享协议留档及任务书追溯矩阵；初始按要求不提交，后获D-07新指令授权在独立分支提交。
- 原始任务书与 SDD 规范按字节保留；记录用户选择 ESP-IDF v6.0.0、只做 ESP32 和本轮仅文档的决定。
- 初始main `3efd7dc`存在32B日志与96B契约不匹配；合并最新分支至`e3a4410`后已为96B记录/256B头，历史取证保留，当前转为完整布局与对端SCB/1能力待验，不静默适配其他布局。
- 2026-10-17：回填用户决定D-06，第二TCP BUSY、SPI换会话通知及桥接端EVENT_GAP三项继续作为实施前必须确认门禁，保持SCB/1原文；同步README、路线图、决定记录和追溯矩阵，不补充线格式。

暂无发布版本；历史文档记录与本次实现/验证分别列明，Group 1不等于Phase 01完成。

### Changed

- 2026-10-17：用户要求继续余下五阶段；先按既有审查节奏提交Group 1为`8750d6a`，再准备Phase 01 Group 2启动/固定池必要增量，明确内部接口、数据区容量、所有权、独立失败路径及生产C验证方法；增量已获用户确认，先独立提交，未混入新实现，G-01～G-03未解除。
- 2026-10-17：按用户要求，将GitHub最新`feature/15-comment-documentation`快进合并到本地main，再建立`codex/constitution-esp32bridge`提交本次31份Markdown；合并前后差异与SDK/协议门禁同步记录，无本次新增固件代码，不push。

- 2026-10-18：用户明确G-02采用SPI会话变化关闭TCP、电脑重连用既有HELLO/HELLO_ACK取状态；独立纯规约记录及同步阶段02/04/05，不新增协议类型，G-01/G-03保留。见[G-02决定](specs/features/02-scb-protocol/g-02-session-change.md)。

- 2026-10-18：Phase 01 Group 3已复核提交1b5b9fe；Phase 02 Group 1新增生产C显式编解码、CRC、全部类型/二进制边界、有界UTF-8/重复键/有限数JSON校验及字段视图。27项新增、104项全回归和v6.0.0目标构建通过，代码交审待提交；[报告](Docs/validation/phase-02-group-1.md)区分主机、布局取证、目标对象和实机边界。
