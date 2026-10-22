# R6：5秒诊断日志补全协议与桥接状态计数（2026-10-22 联调整改）

关联FR-8/M-39（bridge_stats定期诊断）。本批为20261022联调问题整改的ESP32侧增量，
用户已批准：只扩充既有5秒串口日志的计数字段，不改任何行为、协议、时序或资源。
背景：D1许可脱钩与链路诊断需要invalid/duplicates/backpressure/delivered/rebuilds/
telemetry_dropped/invalid_outbound及orphan/backpressure计数随日志可见；这些计数
已在bridge_spi_protocol_stats_t与bridge_state_snapshot_t中存在，本批仅暴露。

## 可判定需求

- **R6-01 spi日志**：spi_bridge.c既有5秒"spi"行追加输出protocol快照的invalid、
  duplicates、backpressure、delivered、rebuilds、telemetry_dropped、invalid_outbound。
  数据来源为既有health->protocol字段，不新增计数点、不改快照结构、不改日志周期。
- **R6-02 bridge日志**：bridge_stats.c既有5秒"bridge"行追加输出bridge快照的
  orphan_results、canceled_results及command/result/event三类backpressure。
  数据来源为既有bridge_state_snapshot_t字段，不新增计数点。
- **R6-03 不变量**：不改SCB/1、不改任何计数语义或清零时机、不增堆分配、
  不增任务、不向socket输出调试文本；仅串口ESP_LOGI格式串变化。
  bridge_stats.c/.h属Phase05在途未提交文件，本批对工作区的修改不随本批提交，
  由其归属任务统一提交；本批仅提交spi_bridge.c与本规约及证据。

## 验证

| 编号 | 方法 | 通过标准 |
|---|---|---|
| R6-V1 | v6.0.0目标build | 构建通过，无新增警告 |
| R6-V2 | 主机回归（既有258项） | 全过，日志格式不在主机断言内 |
| R6-V3 | 用户实机烧录后串口 | 两行各含新增字段且计数随负载变化 |

ESP32烧录按仓库约定由用户人工执行；本批不代烧。
