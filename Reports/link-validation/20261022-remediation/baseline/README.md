# 失败基线：2026-10-22（修复前，R2 固件 c208713e）

本轮在烧录任何修复前复现并留证。结论：**D2 两次实机确认（含一次现场再冻结全过程）；
D1 以"查询负载下积压持续增长后恒定"的形态完整复现，并锁定泄漏机制候选为
HAL_BUSY 空 CS 脉冲**。

## 冻结态留证（复位前）

板子上电后处于 D2 冻结态。被动探针（无 halt/reset/写）抓取：

| 证据 | 内容 |
|---|---|
| `stuck-state.log` | SPI2 SPE=0；DMA1 LISR=0x10020；S1(TX) CR=0x440/NDTR=0x3d7，S2(RX) CR=0x406/NDTR=0；hdmarx ErrorCode=0x80（HAL_DMA_ERROR_NO_XFER）；WIFI permits=0x896b consumed=0x8969 completed=0x8967 errors=2 timeouts=0 state=3 |
| `handles.log` | hspi2/hdmatx/hdmarx 句柄原始转储 |
| `backlog-probe.log` | 200×10ms 全部计数冻结，GPIOA_IDR 仅 bit7 翻转，PA15(READY)=0 |
| `cpu-state.log` | target running，tick 前进（0x3f2e15→0x3f2f47/300ms），无 fault（修正原 cfg 第13行 `reg` 命令在运行态不可用，去掉后重测） |

## 复位后基线（OpenOCD `reset run`，配置 `reset-run.cfg`）

抓取窗口约 60 秒：OpenOCD 3000 点×约16ms（`backlog-during-query.log`，解析见
`baseline-analysis.json`、`backlog-trajectory.json`）+ ESP32 COM3 被动串口 75 秒
（`esp32-passive-serial.txt`，`serial_capture.py` 为 12 秒原脚本的参数化版本，
不改原脚本）。窗口内用 SmartCarLink 执行两次 `params get`
（`params-get-baseline.txt`，控制台 GBK 显示乱码但内容为"失败：TCP 连接断开"与
"失败：超时，GET_REQUEST_RESULT 未收到业务结果"）。

### 时间线

| 阶段 | 样点 | 现象 |
|---|---|---|
| 查询前稳态 | S0–S300 | backlog 0–1，permits 与 consumed 同步前进，约 70 笔/秒，session=485037712 与 ESP32 串口一致 |
| 查询 #1 起 | S303–S450 | **errors=0、timeouts=0 的情况下 backlog 从 0 稳步涨到 16**；ESP32 同窗口 trunc +17 |
| 查询中稳态 | S450–S1800 | backlog 恒定 12–16（D1 签名形态：陈旧积压恒定不消除）；吞吐升到约 112 笔/秒（自由发，不再等 READY） |
| 会话重建 | 约 S1879 | STM32 link session→0、ESP32 串口 session 485037712→4101938600；查询 #1 的 TCP 被按 G-02 关闭（电脑端表现为"TCP 连接断开"） |
| 第一次错误 | S1929 | errors=1，state 经 3 恢复回 0（R2 修复对 BUSY 情形有效） |
| 第二次错误 | S1942 | errors=2，**state=3 永久冻结，backlog=18 定格**；ESP32 queued/completed 同时定格 47569/47568 |

### D1 泄漏机制（本轮证据锁定）

- `BSP_WiFi_TryExchange`（bsp_wifi.c:107-113）在 HAL 启动前先拉低 CS；HAL_BUSY 时立即拉高
  CS、**保留许可、不计 consumed**。
- ESP32 从机视角这是一次 0 位事务：CS 中途断开 → post_trans 触发、计入 truncated，
  READY 拉高后重新装载再拉低 → **STM32 多计一个 permit**。
- 净效果：每次 HAL_BUSY 空 CS 脉冲 backlog 永久 +1。高负载下（本窗口约 6% 触发率）
  持续增长；backlog>0 后 `consumed==permits` 闸门永不闭合，STM32 不再等 READY 自由发，
  吞吐上升进一步抬高 HAL_BUSY 竞争率，自维持。与 162357 目录观察到的恒定 109 同一机制，
  数值随每次运行的竞争情况不同。
- 佐证：查询窗口 ESP32 trunc +28 而 STM32 errors=0/timeouts=0 —— 无错误无超时的截短
  只能来自空 CS 脉冲。

### D2 冻结机制（再次确认）

第一次 SPI 错误经 R2 路径成功恢复（hdma 处于 BUSY，HAL_DMA_Abort 生效）；第二次错误后
hdmarx 处于 ERROR 态（ErrorCode=0x80），`BSP_WiFi_AbortDma` 只中止 BUSY 句柄 → 跳过 →
READY 条件永不满足 → state=3 永久。与 175621 目录结论一致，本次为全过程现场记录。

## 本轮结束时的板卡状态

STM32 处于 D2 冻结态（state=3，backlog=18）；ESP32 持续装载等待（READY=0）。
未烧录任何新固件。

## 文件索引

- `stuck-state.cfg/.log`、`handles.cfg/.log`、`backlog-probe.cfg/.log`、`cpu-state.cfg/.log`、`reset-run.cfg`
- `backlog-long.cfg`、`backlog-during-query.log`、`baseline-analysis.json`、`backlog-trajectory.json`
- `serial_capture.py`、`esp32-passive-serial.bin/.txt`、`esp32-passive-serial-meta.json`
- `params-get-baseline.txt`
- `backlog-after-reset.log`（复位后首轮 200 点，backlog 0–1）
