# 主程序与文档修改记录（2026-10-22 联调整改）

本文件逐条记录本轮对主程序（SmartCar 固件、ESP32Bridge 固件、Tools）及既有文档的
每一次修改。只读调查、证据目录自身的新增文件、`TestFirmware/` 与 `Docs/` 的新增
不属于"修改既有主程序/文档"，但烧录行为仍记录哈希。

| 日期 | 对象 | 类型 | 原因 / 关联问题 | 规约提交 | 实现提交 | 验证 | 烧录 HEX SHA256 |
|---|---|---|---|---|---|---|---|
| 2026-10-22 | SmartCar 工作区与 HEAD 对齐 | Git 合并 | 隐患 9：R2 修复在 feature/16-spi-dma-recovery 而检出在 feature/02-workspace-flash；快进合并至 56174a3，工作区既有改动与分支内容逐字节一致，无信息丢弃 | 12fe38b（既有） | f857145（既有） | git status 复核 | 未烧录（不涉及固件变化） |
| 2026-10-22 | TestFirmware/ 新建测试固件 | 新增（非主程序） | D1 定位 / 图案对齐 / D2 注入测试；AC5 命令行构建 0 警告 | 不涉及 | 不涉及 | 注入恢复 22/22；hal_busy 恒 0（否定 HAL_BUSY 假设）；位左移 0xA6868462 复现 18 次 | 测试固件 168735b0…（烧录后已回烧生产固件 c208713e…） |
| 2026-10-22 | SmartCar specs 16-spi-wifi-link R3 规约 | 文档修改 | D1 结构性重同步 + D2 双通道定向修补规约（用户已定策略） | eb16c0a | — | 规约评审 | 未烧录 |
| 2026-10-22 | SmartCar/BSP/bsp_wifi.c、tests/spi_wifi_state.py | 代码修改 | R3 实现：AbortDma 扩展非 BUSY 回收、粘性 ABORT 清错误码有界重试、新增 BSP_WiFi_ResyncPermits 两调用点 | eb16c0a | 06edec6 | Clang/LLD 替代宿主 current+5 负对照通过（r3-build/）；Keil 0 错误 0 警告 | 707e753dbe57b8fb19555dfeede65fb2e3452bc0c7bfdc2fbee39eaa5e3ee44d（烧录+verify OK） |
| 2026-10-22 | SmartCar specs R3 规约勘误 + plan.md | 文档修改 | 实机复测证实重同步调用点(b)（state==0）在持续流式工况不可达：backlog 恒定偏移 3→4 棘轮至 20（validation/backlog-after-fix.log）；触发扩展为 state==0 或 state==2 | 7c1529e | — | 机上采样 3000 点取证 | 未烧录 |
| 2026-10-22 | SmartCar/BSP/bsp_wifi.c、tests/spi_wifi_state.py | 代码修改 | R3 勘误实现：Service 重同步触发扩展至待读取安全点；夹具补 state==2 用例及撤回负对照 | 7c1529e | d317bf8 | Clang/LLD current+6 负对照通过（r3-build/clang-host-summary.json）；Keil 0 错误 0 警告（keil-build-r3b.log） | 89ff07dda14ee6588fae183665fa2d2926a89d7a43d37b0960840200dde0d0b6（烧录+verify OK；R3b 机上复测见 validation/r3b/） |
| 2026-10-23 | ESP32Bridge specs 05-bridge-lifecycle R6 规约 | 文档新增 | 隐患 1：5 秒日志缺 invalid/duplicates/backpressure/delivered/rebuilds/telemetry_dropped/invalid_outbound 及 orphan/backpressure 计数，联调无法判读 | eea02a7 | — | 规约评审 | 未烧录 |
| 2026-10-23 | ESP32Bridge/main/spi_bridge.c | 代码修改 | R6-01：spi 5 秒行追加 7 个既有 protocol 计数；不改行为 | eea02a7 | 4750600 | v6.0.0 目标构建通过（artifacts/bc3a4b8e…）；主机回归 253/258（5 项失败均为 build 脚本环境测试的既有环境问题，与本改无关，本改不含 Python/构建脚本） | esp32bridge.bin SHA256 5c1fd75beb18b1ae6ec3b2ba236618a657b0bc306e7141d4a3f8284418531946；**ESP32 烧录按约定由用户人工执行，未代烧** |
| 2026-10-23 | ESP32Bridge/main/bridge_stats.c | 工作区修改（未提交） | R6-02：bridge 5 秒行追加 orphan/canceled/三类 backpressure；该文件属 Phase05 在途未提交（?? 状态），按 Git 边界不随本批提交，留工作区由归属任务统一提交 | eea02a7 | 不提交 | 随同一构建 bc3a4b8e 编译通过 | 同上 bin（构建含此工作区修改） |
| 2026-10-23 | SmartCar specs 16-spi-wifi-link R4 规约 | 文档修改 | 新发现（不属原总结 11 项）：链路陷入持久 OVR 错误死循环，R2/R3 定向修补机械成功但永不治愈（validation/r4-probe/observe.log）；SPI2RST 模拟实验证实外设级复位可治愈但约 35~40 秒自发复发（r4-probe/spi2rst-experiment.log、cfg2-fix.log）。用户弹窗确认推翻"仅定向修补"决定，采用定向修补+连续失败 3 次升级外设重初始化，加调试计数器 | 1f4abd6 | — | 规约评审 | 未烧录 |
| 2026-10-23 | SmartCar/BSP/bsp_wifi.c、tests/spi_wifi_state.py | 代码修改 | R4 实现：s_consecutive_errors 三点递增/成功清零；Service 恢复发布点前按阈值 3 升级 RCC SPI2RST 脉冲+HAL_SPI_Init；新增 s_reinit_count 调试计数器 | 1f4abd6 | dc0006c | Clang/LLD 替代宿主 current+7 负对照通过（r4-build/clang-host-summary.json）；Keil AC5 0 错误 0 警告（MDK-ARM/keil-build-r4.log，Code=75214） | af7b5c60d63ff44fbaad2ddb7bdd9cc070620333033ba3af3c6090a53dae06c1（烧录+verify OK；R4 机上观察见 validation/r4-onboard/） |
| 2026-10-23 | Tools/SmartCarLink/scb_session.py、tests/test_session.py、tests/test_client.py | 代码修改（PC 工具） | 新发现：设备端 request_id 缓存随 SPI 会话存活、跨 TCP 重连保留（FR-08），PC 工具每进程从 1 分配，重连后同号不同内容被设备判 INVALID_ARG（实机复现：params get 稳定失败，req_id=200 探针返回 OK）；起点改为时间派生非零值 | 不涉及（PC 工具） | 见本表 Git 记录 | unittest 146/146 通过；实机 params get×2（独立新进程）成功 | 不涉及 |
| 2026-10-23 | Docs/链路联调测试流程.md | 文档新增 | 用户要求"通用测试过程写入文档"：烧录/OpenOCD 探针/地址表/串口采集/SmartCarLink/判读基准/验证矩阵 | 不涉及 | 不涉及 | 文档内方法均本轮实测 | 不涉及 |
| 2026-10-23 | SmartCar specs 16 三件套+CHANGELOG、roadmap | 文档回填 | R3/R3勘误/R4 追溯与实际结果回填（R3 回填属补漏） | 1f4abd6（规约本体） | 见本表 Git 记录 | 与 r3b/r4-onboard 证据一致 | 不涉及 |
