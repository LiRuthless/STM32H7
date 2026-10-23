# 数据链联调：已发现问题与未排除隐患（2026-10-22）

## 一、已确认的问题（有实机证据）

### D1 READY 许可脱钩（SPI 握手层，业务查询失败的直接原因）
- **现象**：`permits − consumed ≈ 109` 的许可积压恒定存在且永不消除；STM32 实际不再等待 ESP32 的 READY 许可，全速自由发起事务（约 150-190 笔/秒，接近 1024B@1.875MHz 物理上限）。
- **后果**：ESP32 处理稍慢（查询时 Wi-Fi/TCP 抢占）即在 CS 中途才完成装载 → 事务被截短丢弃；STM32→ESP32 的响应帧始终未被接受（ESP32 发出帧的 ACK 恒为 0），STM32 重发 3 次后重建会话，TCP 按 G-02 关闭 → 电脑端表现为查询超时。
- **证据**：`../20261022-162357/post-flash-170730/command-capture/decoded-snapshots.json`（积压恒定 109）、`stm32-reliable-cache.json`（响应帧本身 CRC 有效）、ESP32 串口计数（trunc 在查询窗口 +7）。
- **规约依据**：违背“READY 一次许可对应一笔事务”。

### D2 错误态永久不恢复（当前板子正处此状态，链路完全停摆）
- **现象**：一次 SPI  overrun 后，SPI2 被禁用（SPE=0）、RX DMA FIFO/请求错误锁存（LISR.FEIF2、DMA ErrorCode=0x80、SPI ErrorCode=OVR|ABORT），`BSP` state=3 冻结，无任何计数增长，无任何 CS 活动；ESP32 一直装载等待（READY 持续低）。
- **机制**：`HAL_SPI_Abort` 只在 CFG1 的 DMA 请求位仍置位时中止 DMA（本次已被 HAL 错误路径提前清除）→ 跳过；`BSP_WiFi_AbortDma` 只中止 `BUSY` 态句柄 → `ERROR` 态句柄被跳过且永远不等于 READY → `state=3` 永远不满足恢复条件。刚烧录的 DMA 修复固件未覆盖此情形。
- **证据**：`stuck-state.log`、`handles.log`、`backlog-probe.log`（本目录）。

### 历史问题（已修复并验证）
- 旧 STM32 固件 DMA 残留死锁（TX DMA 卡在 493）→ 已由 `feature/16-spi-dma-recovery` 的 DMA 回收修复解决，烧录后 readback 与 HEX 全等。
- ESP32 旧固件未含业务转发 → 已重烧 Phase05 镜像（写校验通过，启动版本一致）。

## 二、未排除的隐患

1. **D1 积压的最初来源未定位**：启动补获双计、HAL_BUSY 空 CS 脉冲、EXTI 噪声均有可能；无逻辑分析仪，波形项始终未验证。
2. **SPI 波形从未实测**：Mode0 时序、CS/SCK/MOSI/MISO/READY 均未上过仪器；修复前出现的“SCB1 整体左移一位”读数未解释（可能是旧 DMA 缺陷的伪影，也可能是真实采样边沿问题）。
3. **OVR 的触发链未闭环**：D2 的 overrun 大概率由 D1 的失步流量诱发，但未逐一证实。
4. **ESP32 侧仍有黑盒**：`invalid/duplicates/backpressure/orphan` 计数不进 5 秒日志，不能完全排除 ESP32 软件路径也有丢弃贡献。
5. **长效稳定性未知**：即使两项修复落地，截短率的长期水位、Wi-Fi 共存抖动、高速档（7.5MHz）均未验证（目前只跑过 1.875MHz 低速档）。
6. **T6 全段未验证**：遥测产生条件（标定/起跑）未满足，UDP 链路、60s 流水统计、dump 导出与 CRC 校验均未执行。
7. **供电只有静态 3.3V 一次读数**，瞬态跌落/纹波未测。
8. **“返回路径能正常工作”的说法与 18:11 实测（`params get` 与标准探针均超时）矛盾**，发生时的板卡状态未知，需对质或复现条件。
9. **仓库卫生**：修复提交在 `feature/16-spi-dma-recovery` 分支而检出仍在原分支，工作区与 HEAD 长期分叉；ESP32 侧在途改动未提交。

## 三、下一步建议（待用户决定）
- 先按规约先行修复 D1+D2（仅 STM32 侧，不改协议/ESP32/电脑工具），烧录后执行 6 项测试（物理→SPI→会话→单查询→恢复→流水/导出）。
- 用户曾否决“先修复后全量测试/先复位复现再修复”的计划草案，执行顺序待其明确。
