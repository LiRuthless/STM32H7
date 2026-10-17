# Changelog

## [Unreleased]

### Added

- 2026-10-17：建立ESP32Bridge独立SDD文档、六阶段规约、共享协议留档及任务书追溯矩阵；初始按要求不提交，后获D-07新指令授权在独立分支提交。
- 原始任务书与 SDD 规范按字节保留；记录用户选择 ESP-IDF v6.0.0、只做 ESP32 和本轮仅文档的决定。
- 初始main `3efd7dc`存在32B日志与96B契约不匹配；合并最新分支至`e3a4410`后已为96B记录/256B头，历史取证保留，当前转为完整布局与对端SCB/1能力待验，不静默适配其他布局。
- 2026-10-17：回填用户决定D-06，第二TCP BUSY、SPI换会话通知及桥接端EVENT_GAP三项继续作为实施前必须确认门禁，保持SCB/1原文；同步README、路线图、决定记录和追溯矩阵，不补充线格式。

本记录描述文档草案，不代表功能发布、实现完成或测试通过。暂无发布版本。

### Changed

- 2026-10-17：按用户要求，将GitHub最新`feature/15-comment-documentation`快进合并到本地main，再建立`codex/constitution-esp32bridge`提交本次31份Markdown；合并前后差异与SDK/协议门禁同步记录，无本次新增固件代码，不push。
