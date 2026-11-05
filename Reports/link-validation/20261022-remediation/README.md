# D1/D2 修复与隐患排除：2026-10-22

本轮起点为 `../20261022-联调问题与隐患总结.md` 列出的两个已确认问题（D1 READY 许可脱钩、
D2 错误态永久不恢复）与 9 项未排除隐患。执行顺序经用户批准：先取证复现失败基线，
再定位、规约先行修复、烧录验证；测试专用固件独立存放于仓库根 `TestFirmware/`；
通用测试流程沉淀为 `Docs/链路联调测试流程.md`。主程序与文档的全部修改逐条记录于
`change-log.md`。

## 仓库处置记录（阶段 0）

- 将本地分支 `feature/16-spi-dma-recovery`（3 个提交：规约 12fe38b、实现 f857145、
  文档 56174a3）快进合并入当前检出 `feature/02-workspace-flash`，HEAD 现为 56174a3。
  合并前已验证工作区 7 个 SmartCar 改动文件与分支内容逐字节一致（仅换行符经 Git
  属性归一），未跟踪的 r2-dma-recovery.md 与分支版本一致，无任何信息丢弃。
  合并后 SmartCar 跟踪文件全部干净，仅剩 MDK-ARM 构建产物与烧录日志未跟踪。
- ESP32Bridge 在途改动（24 个修改文件 + 新增文件）保持未提交原样，不在本批处置。
- 隐患 9 的工作区/HEAD 分叉问题由此消除（ESP32 侧在途改动仍待其自身任务提交）。

## 测试状态总表

| 测试项 | 结果 | 说明 / 证据 |
|---|---|---|
| D2 冻结态留证 | 完成 | `baseline/`（stuck-state.log 等） |
| D1 积压基线复现 | 完成 | `baseline/`（backlog 轨迹 12~16） |
| D1 来源定位（测试固件） | 完成（含负结果） | `d1-localization/`；HAL_BUSY 假设被否定，泄漏种子依赖完整 App 时序环境 |
| SPI 图案对齐（低速/高速档） | 完成 | `d1-localization/`；位左移 0xA6868462 复现 18 次 |
| D2 修复（R2+R3 双通道修补） | 完成并机上复测通过 | 规约 d28c4a4/12fe38b/eb16c0a，实现 f857145/06edec6 |
| D1 修复（R3 电平重基线+勘误） | 完成并机上复测通过 | 勘误规约 7c1529e，实现 d317bf8；r3b 复测 backlog≤1 |
| 机上复测（D2 注入恢复/积压归零/端到端查询） | 完成 | `validation/r3b/`：backlog {0:1895,1:1104,-1:1}、errors=0、params get×2 |
| ESP32 日志计数补全（R6） | 规约+实现+构建完成，**烧录留用户** | 规约 eea02a7、实现 4750600、bin 5c1fd75b… |
| OVR 死循环新发现与 R4 外设重初始化 | 完成并机上验证 | `validation/r4-probe/`（SPI2RST 实验）、`validation/r4-onboard/`（10 分钟 16 次复发全部自愈）；规约 1f4abd6、实现 dc0006c、HEX af7b5c60… |
| 7.5MHz 高速档 | 完成（调试器 RAM 补丁方式） | `validation/r4-onboard/fast-clock.log`：60 秒无新错误 |
| G-01 第二连接 BUSY 回归 | 受阻：修复在未烧录的 ESP32Bridge 在途工作树 | 裸 socket 实测第二连接 4 秒零字节；待 ESP32 烧录后复测 |
| SmartCarLink request_id 跨重连碰撞 | 完成（新发现并修复） | dc6b6cb；unittest 146/146，实机 params get×2 通过 |
| 遥测/导出 CRC、3.3V 万用表复核 | 留人工（需车辆运行工况/仪器） | 见 `Docs/链路联调测试流程.md` 验证矩阵 |
| OVR 复发诱发源根因 | 未定位，列后续调查 | 平均约 35 秒复发一次，R4 兜底保证每次自愈 |

## 目录

- `change-log.md` — 主程序与文档全部修改的逐条记录（含烧录 HEX 哈希）。
- `baseline/` — 修复前失败基线证据。
- `d1-localization/` — 测试固件定位证据。
- `validation/` — 修复后机上验证证据（`r3b/` R3勘误复测、`r4-probe/` SPI2RST实验、
  `r4-onboard/` R4十分钟观察与7.5MHz高速档）。
- `r3-build/`、`r4-build/` — Clang/LLD 替代宿主探针与构建记录（含负对照变体）。
