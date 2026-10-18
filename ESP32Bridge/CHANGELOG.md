# Changelog

## [Unreleased]

- 2026-10-18本轮：Group 1复核104项通过并提交455f817；先提交Group 2内部接口/独立计时/生产C验证及环境启动错误诊断细化，外部协议门禁不变。

### Added

- 2026-10-18：Phase 01 Group 3新增显式环境选择、正式复制构建/产物回收及安全清理、中文构建说明、文件系统与PowerShell错误路径测试；77项主机检查通过，正式入口集成结果单列于 [Group 3报告](docs/validation/phase-01-group-3.md)。本组待审查提交，未烧录或改变共享协议。

- 2026-10-18：Phase 01 Group 2按纯规约73a2a2f实现内部固定池、代际/容量/峰值、DMA所有权与拒绝忙池销毁；实现可注入启动协调、READY保护、生产C密码校验及真实NVS/netif/event适配。59项主机检查及v6.0.0目标build/size通过；代码待用户审查提交，AP/socket/SPI完整服务和实机测量未完成。详见 [Group 2报告](docs/validation/phase-01-group-2.md)。

- 2026-10-17：依据已提交规约 `5c15c49` 实施 Phase 01 Group 1：独立v6.0.0/esp32s3工程、显式SDK组件依赖、无PSRAM/双核配置、本地密码Kconfig及忽略规则、构建输入/最终配置检查、真实CMake拒绝路径测试和配置自检入口。待用户审查提交；启动/资源、复制构建脚本及后续阶段未实现。实际结果见 [Group 1证据](docs/validation/phase-01-group-1.md)。
- 2026-10-17：建立ESP32Bridge独立SDD文档、六阶段规约、共享协议留档及任务书追溯矩阵；初始按要求不提交，后获D-07新指令授权在独立分支提交。
- 原始任务书与 SDD 规范按字节保留；记录用户选择 ESP-IDF v6.0.0、只做 ESP32 和本轮仅文档的决定。
- 初始main `3efd7dc`存在32B日志与96B契约不匹配；合并最新分支至`e3a4410`后已为96B记录/256B头，历史取证保留，当前转为完整布局与对端SCB/1能力待验，不静默适配其他布局。
- 2026-10-17：回填用户决定D-06，第二TCP BUSY、SPI换会话通知及桥接端EVENT_GAP三项继续作为实施前必须确认门禁，保持SCB/1原文；同步README、路线图、决定记录和追溯矩阵，不补充线格式。

暂无发布版本；历史文档记录与本次实现/验证分别列明，Group 1不等于Phase 01完成。

### Changed

- 2026-10-17：用户要求继续余下五阶段；先按既有审查节奏提交Group 1为`8750d6a`，再准备Phase 01 Group 2启动/固定池必要增量，明确内部接口、数据区容量、所有权、独立失败路径及生产C验证方法；增量已获用户确认，先独立提交，未混入新实现，G-01～G-03未解除。
- 2026-10-17：按用户要求，将GitHub最新`feature/15-comment-documentation`快进合并到本地main，再建立`codex/constitution-esp32bridge`提交本次31份Markdown；合并前后差异与SDK/协议门禁同步记录，无本次新增固件代码，不push。

- 2026-10-18：用户明确G-02采用SPI会话变化关闭TCP、电脑重连用既有HELLO/HELLO_ACK取状态；独立纯规约记录及同步阶段02/04/05，不新增协议类型，G-01/G-03保留。见[G-02决定](specs/features/02-scb-protocol/g-02-session-change.md)。

- 2026-10-18：Phase 01 Group 3已复核提交1b5b9fe；Phase 02 Group 1新增生产C显式编解码、CRC、全部类型/二进制边界、有界UTF-8/重复键/有限数JSON校验及字段视图。27项新增、104项全回归和v6.0.0目标构建通过，代码交审待提交；[报告](docs/validation/phase-02-group-1.md)区分主机、布局取证、目标对象和实机边界。
