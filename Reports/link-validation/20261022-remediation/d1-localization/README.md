# D1/D2 定位：2026-10-22（仪表化测试固件 TestFirmware）

本轮烧录专用测试固件（`TestFirmware/`，AC5 命令行构建，HEX SHA256
`168735b03c3d8a8d2820b54bdd790573994a6a55a02b9929fa3aec0dd5eb7114`）：
生产 `bsp_wifi.c`（源 SHA256
`85567fea0d0ca6aabfd25597b04805a79b3d29a71256c9206c752b7cadacd8ab`）逐字复制加仪表，
App 换为仅跑 WiFi 链路的最小前台（HELLO/IDLE 帧，车辆停车），全部计数镜像到绝对地址
`0x2403A000`（32 字，布局见 `TestFirmware/README.md`），OpenOCD 被动探针采样。

## 结论表

| 问题 | 结论 | 依据 |
|---|---|---|
| D1：`HAL_BUSY 空 CS 脉冲`是否为泄漏机制 | **否定（在本架构下该分支不可达）**：三轮运行（含 65 次真实 `params get` 查询风暴）共约 6.7 万笔带负载事务，`hal_busy_count` 恒为 0，backlog 全程无持续增长 | `run-a*/run-a3-analysis.json`；代码路径分析见下 |
| D1：测试固件上是否复现 backlog 增长 | **未复现**。裸 TCP 负载与真实查询风暴下 backlog 均保持 0（±1 仅为非原子镜像的采样伪影，`backlog_sustained_nonzero_samples=0`） | `run-a-timeline.csv`、`run-a3-timeline.csv` |
| D1：ESP32 `short/trunc` 与泄漏的关系 | **trunc 分为两种**：在途截断（有 consumed 配对，不泄漏）与 0 位脉冲（无配对，泄漏 +1）。本轮观测到约 2~99 次/窗的背景在途截断，backlog 均不增长——证明"trunc 增加"不等于泄漏，基线中随查询增长的 trunc 才是无配对型 | `run-a*`/`run-b` 的 short 增量 vs backlog |
| D2：注入式在途 `HAL_SPI_Abort` 能否恢复 | **22/22 + 8/8 全部在 100ms 内恢复**（经 10ms timeout 路径，`timeouts` 同步 +1）；恢复链路在纯注入下稳健 | `run-a3`/`run-b`：`abort=rec_ok`、`rec_fail=0` |
| D2：冻结是否再现 | **再现一次**（两轮之间约 8 分钟无人值守窗口内）：`errors=2, state=3` 永久冻结，与基线签名一致；并新发现粘滞 `HAL_SPI_ERROR_ABORT` 细节（见下） | `freeze-state.log` |

## D1 机理分析（本轮否定的具体路径）

生产假设：`BSP_WiFi_TryExchange` 先拉 CS 再调 `HAL_SPI_TransmitReceive_DMA`，HAL_BUSY 时抬 CS、
保留许可 → 0 位脉冲 → ESP32 trunc → 重新装载 → permit 无配对 +1。

本轮代码核查（`SmartCar/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_spi.c`）发现该分支
**在现有 BSP 状态机下不可达**：

1. `HAL_SPI_IRQHandler` 的 EOT 路径在调用 `HAL_SPI_TxRxCpltCallback` **之前**就执行
   `hspi->State = HAL_SPI_STATE_READY`（hal_spi.c:2999 先于 3028）；完成回调把
   `s_status.state` 置 2 时句柄必然 READY。
2. `HAL_SPI_TransmitReceive_DMA` 只在入口 `hspi->State != READY` 时返回 HAL_BUSY；
   DMA 流启动失败返回的是 HAL_ERROR（走 `errors++`/`state=3`，**消费许可、不泄漏**）。
3. 其余 `state==0` 的时刻（Init、ReleaseRx 后、Service 恢复后）句柄均已 READY。

与实测一致：最小前台下 `hal_busy_count` 恒 0，泄漏不自我播种。**基线 backlog 增长的
触发条件依赖生产 App 前台环境（LCD/SPI4、ADC DMA Stream0 prio2、UART、JSON 处理、
TIM ISR 等改变 IRQ/前台交错），或种子并非 HAL_BUSY 路径。**建议下一步把
`hal_busy` 计数直接加进生产固件验证；若仍为 0，则种子需从 ESP32 侧重装载异常或
READY 线毛刺方向排查（本轮观察到成因未定的"在途边沿簇"，见观察记录）。

## D2 机理分析（本轮新增细节）

冻结现场（`freeze-state.log`，与基线逐寄存器同型）：

- SPI2：CR1=0x1000（SPE=0）、CFG1=0x44400000（TXDMAEN/RXDMAEN=0）；
- DMA1：LISR=0x10000；S1(TX) CR=0x440/NDTR=0x3d6；S2(RX) CR=0x406/NDTR=0（EN 均为 0）；
- `hdmarx` ErrorCode=**0x80（HAL_DMA_ERROR_NO_XFER，与基线相同）**；
- **新发现**：`hspi2.ErrorCode=0x44 = HAL_SPI_ERROR_OVR | HAL_SPI_ERROR_ABORT`。
  `HAL_SPI_Abort` 结尾检查 `ErrorCode` 中既有的 ERROR_ABORT 位：置位则返回 HAL_ERROR
  且**只有成功路径才清 ErrorCode** → 粘滞位使每次 `BSP_WiFi_Service` 重试的
  `spi_result != HAL_OK` → `state=3` 永久。这给出 D2 的第二条冻结通道
  （基线通道为 hdma 处于非 BUSY 态被 `BSP_WiFi_AbortDma` 跳过）。
- 前台仍存活（tick 镜像持续推进），仅链路冻结；backlog 定格 1（冻结交易的 ESP32
  重装载边沿 `edges_error=1` 到达时 state 已为 3，许可无人消费）。

## 运行记录

### 冒烟（烧录后立即）

magic=0x54455354 ✓，tick 前进，permits/consumed 同步增长（约 235 笔/秒 IDLE 流量），
链路握手正常（RX 绝大多数为 SCB1 帧）。

### Run A（裸 TCP 负载）：`run-a*`

60s 窗口：20s 空闲 → 30s TCP 连接风暴（连接/发 64B/保持 2s/断开，15 次连接）→ 空闲；
同步 ESP32 COM3 串口 85s。
结果：`hal_busy Δ=0`；backlog 无持续增长（min/max=-1/+1 单样本伪影）；permits Δ=14751；
注入 4 次全部恢复（timeouts Δ=4）；ESP32 short Δ=9（4 次注入各截断 1 笔，其余 5 次为背景
在途截断，不伴随 backlog 增长）；`edges_idle/inflight/rxready/error Δ=14724/27/0/0`。

### Run A2（查询风暴打在已冻结链路上）：`run-a2*`

排程时未知板卡已在两轮之间冻结（见下）。6 次 `params get` 全部超时失败；
探针 1200 点全部 `state=3`、计数定格；ESP32 `queued/completed` 定格 138561/138560。
侧面记录了冻结链路对 PC 查询的外在表现。

### 两轮之间的无人值守窗口（D2 现场）

Run A 结束（tick≈24 万 ms）到 Run A2 开始（tick≈77 万 ms）约 530s 内：注入 11 次
（abort 11→22）全部恢复；期间发生 **2 次真实 SPI 错误**（errors 0→2），第一次恢复，
第二次冻结（与基线"两次错误、第二次冻结"模式一致）。冻结全过程计数见
`freeze-state.log`（TESTFW 32 字 + 句柄 + 寄存器）。

### Run A3（干净启动 + 真实查询风暴）：`run-a3*`

复位后立即采集：8s 空闲 → 25s 内 65 次 `params get`（ESP32 回答
"HELLO_ACK 缺少有效 session_id / link_ready=false"，因测试固件不应答业务层；
WiFi/TCP+JSON 抢占强度高于基线的 2 次查询）→ 空闲。
结果：`hal_busy Δ=0`；backlog 无持续增长；permits Δ=14807；注入 3 次全部恢复；
ESP32 short Δ=8；`edges Δ=14750/56/0/0`。

### Run B（D2 注入观察，无 PC 负载）：`run-b*`

复位后 187s 窗口：注入 8 次（tick≈30021 起每 20s），**8/8 在 100ms 内恢复**
（`rec_ok=8, rec_fail=0, timeouts=8`），链路持续运行至终（state 正常翻转，
permits Δ=37159）。本轮未再冻结。ESP32 short Δ=99，其中 8 次为注入截断，其余为背景
在途截断（见观察记录）。

### 观察记录：在途 READY 边沿簇与背景截断

- `edges_inflight`（边沿到达时 state==1）以约 8 个为一簇出现（A3 中 7 簇共 56 次，
  空闲与负载时段均有；B 中 945 次）。按 ESP32 驱动语义（post_setup 于装载、post_trans
  于 CS 释放），正常重装载边沿应落在 state==0/2，该现象的成因未完全确定，
  可能与 ESP32 侧装载时序或 READY 线完整性有关；这些边沿最终都被消费，未造成泄漏。
- 每次运行均存在少量背景截断（A:5、A3:5、B:91，扣除注入），伴随 RX 魔数分类中的
  非 SCB1 帧（`rx_other`/`rx_zero`/`rx_shifted`）。**左移签名 0xA6868462 实证存在**：
  全生命周期 `rx_shifted=18`（A 窗 +4、A3 窗 +2、B 窗 +39），即确有比特级错位帧到达。
- Run B 背景截断率（约 0.5/s）显著高于 A/A3（约 0.06/s），物理环境因素未排除，
  数值只作参考，不影响上述结论（均不伴随 backlog 增长）。

## 本轮结束时的板卡状态

已重新烧录生产 Keil HEX（`SmartCar/MDK-ARM/SmartCar/SmartCar.hex`，SHA256
`c208713e70f895fb04cbbcb8588cb2ac6831d16eaed404b914c0004e08273c0d`），
OpenOCD `program verify` 通过，复位运行。测试固件留存于 `TestFirmware/`。

## 文件索引

- `run-a-probe.log`（1200 点原始采样）、`run-a-timeline.csv`、`run-a-analysis.json`、
  `run-a-load.txt`、`run-a-serial-meta.log`、`run-a/esp32-passive-serial.{bin,txt}`（含 meta.json）
- `run-a2-probe.log`、`run-a2-load.txt`、`run-a2-serial-meta.log`、
  `run-a2/esp32-passive-serial.{bin,txt}`（冻结链路 + 查询失败表现）
- `freeze-state.log`（D2 冻结现场：SPI2/DMA1 寄存器、hspi2/hdma 句柄、s_status、计数结构）
- `healthy-handles.log`（复位后健康态句柄对照，用于字段偏移标定）
- `run-a3-probe.log`、`run-a3-timeline.csv`、`run-a3-analysis.json`、`run-a3-load.txt`、
  `run-a3-serial-meta.log`、`run-a3/esp32-passive-serial.{bin,txt}`
- `run-b-probe.log`（3000 点）、`run-b-timeline.csv`、`run-b-serial-meta.log`、
  `run-b/esp32-passive-serial.{bin,txt}`、`run-b-reset.log`
- `run-a-analysis.json`（首轮分析脚本输出）

探针 cfg 与采集/分析脚本位于 `TestFirmware/probes/` 与 `TestFirmware/scripts/`
（`testfw-status.cfg` 单次读、`testfw-loop.cfg` 60s、`testfw-loop150.cfg` 150s、
`testfw-stuck.cfg` 冻结现场；`serial_capture.py`、`pc_load.py`、`run_a.sh`、
`run_a2.sh`、`run_a3.sh`、`run_b.sh`、`analyze_run_a.py`）。
计数地址均按本构建 map（`s_status`=0x24000814、`hspi2`=0x24000188、
`hdma_spi2_tx`=0x24000210、`hdma_spi2_rx`=0x24000288、计数结构=0x2403A000）。
