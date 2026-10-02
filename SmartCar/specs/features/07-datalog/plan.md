# Phase 07: 运行数据记录 — 实现计划

> **as-built 追认规约**：本阶段代码已存在，本文件为存量功能的规约化追认，
> 任务分组全部为已实现结构的追溯描述。
> 需求见 [requirements.md](requirements.md)，验证见 [validation.md](validation.md)。

## 目标

交付运行数据记录链路：1ms 采样节拍采集 32B 结构化记录 → RAM 环形缓冲 →
W25Q64 日志区「边写边擦」追加持久化 → 停车后串口阻塞导出 CSV，支撑赛后复盘分析。

## 背景与依据

- 关联宪章：[mission.md](../../mission.md)「参数持久化与运行数据记录」「数据可复盘」成功标准；
  [techstack.md](../../techstack.md) 数据存储节（W25Q64 日志区约 8MB，边写边擦）、硬性约束 3（中断优先级不变量）。
- 前置条件：Phase 01 运行时架构（TIM15 采样器 / TIM7 辅助环 / `App_SampleISR` 钩子）、
  Phase 02 BSP 平台层（`bsp_w25q64` / `bsp_flash` 日志服务 / `bsp_uart`）。
- 串口导出命令 `d` / `c` 的解析在 Phase 06 `wireless.c`（见 [06-hmi-tuning](../06-hmi-tuning/requirements.md) FR-15）。

## 任务分组（Task Groups）

### Group 1: 记录格式定义
- [x] `App/datalog.h::datalog_record_t`：单条记录 32B = 1 个 Flash Word，16 字段
      （tick / adc[4] / speed_l / speed_r / track_error / track_out / gyro_xyz / dl1b_mm /
      battery / kernel_state / roundabout_state），字段明细见 requirements.md 接口约定。
- [x] 记录粒度约束：`BSP_Flash_LogWrite` 要求偏移 32B 对齐、长度 32 的整数倍（`FLASH_WORD_SIZE`）。

### Group 2: 采集通路（生产者）
- [x] `App/app.c::App_SampleISR`（TIM15 采样器 1ms 节拍，`Start_flag` 运行中）→ `Datalog_Push()`。
- [x] `Datalog_Push()`：16 条 RAM 环形缓冲（`LOG_BUF_RECORDS`，= 2 个 256B 页），
      单生产（TIM15，优先级 0）单消费（TIM7，优先级 1）免锁；缓冲满时丢弃新记录。
- [x] `DATALOG_DIV` 编译期分频（每 N 拍记一条，默认 1）；`s_count >= DATALOG_CAPACITY` 写满自动停录。
- [x] 数据来源：编码器/陀螺取 TIM15 采样器 1ms 实时值；电感/偏差/方向输出等控制量按 2ms 控制拍更新。

### Group 3: 刷写通路（消费者）
- [x] `App/app.c::App_TaskISR`（TIM7 5ms）→ `Datalog_Flush()`：攒满 8 条 = 256B = 1 个 W25Q64 页
      整页编程（约 0.7ms）；运行中不足 8 条不刷，避免频繁小额页编程占用 TIM7。
- [x] 写失败即停录（`s_active=0`），保住已有数据。
- [x] `Datalog_Stop()` / `Datalog_Dump()` 兜底刷出不足一页的剩余记录。
- [x] Flash 端「边写边擦」（`bsp_flash.c`）：`Datalog_Start` 仅擦首个 4KB 扇区并把擦除前沿置 4KB
      （起跑几乎无延时）；`BSP_Flash_LogWrite` 每跨入一个新 4KB 扇区自动先擦除（几十 ms）。
- [x] 容量 = (8MB − 4KB 参数扇区) / 32B = **262016 条 ≈ 262s @1ms**；`DATALOG_DIV>1` 可降频延长。

### Group 4: 导出与控制
- [x] `Datalog_Dump()`：停车后串口发 `d` 触发，先 `Datalog_Stop()` 兜底刷写，再逐条读出并
      `snprintf` 为 CSV 行发出（16 列 + 表头 + `EOF` 结尾，阻塞，可粘 Excel/逐飞助手）。
- [x] 串口发 `c` → `Datalog_Start()` 擦除后立即 `Datalog_Stop()`：只复位日志区不开始记录。
- [x] `DATALOG_ENABLE` 编译开关（默认 1）：按启动键（K1）即 `Datalog_Start()` 开始记录。

## 实现顺序与依赖

Group 1（格式）是 Group 2/3/4 的共同契约；Group 2（采集）与 Group 3（刷写）通过环形缓冲解耦，
依赖 Phase 01 的三线中断契约（TIM15 生产 / TIM7 消费）；Group 4 依赖 Phase 06 的串口命令解析。

## 风险与取舍（已知缺陷，如实记录）

| 风险/缺陷 | 现状 | 整改指向 |
|---|---|---|
| **`BSP_Flash_Init()` 全工程从未被调用** | 与 Phase 06 同一缺陷：`s_ready` 恒 0 → `Datalog_Start` 实际返回失败、`Datalog_Push/Flush` 全部落空 → **数据记录在现状代码中实际不生效**（按启动键后记录链路静默失败） | **Phase 08** |
| 注释漂移严重 | `datalog.c/.h` 多处注释与实现不符：头注释称日志区为「Bank2 Sector5+6 共 256KB」、容量「8192 条 @1ms ≈ 8.2s」/「4096 条」、`Datalog_Push`「TIM6 控制环每拍调用」、`Datalog_Start`「阻塞约 2s」——均为早期片内 Flash 方案残留；**实际为 W25Q64 约 8MB 日志区、262016 条 ≈ 262s、TIM15 每拍调用、起跑仅擦首扇区几十 ms**。以 README 与本规约为准 | Phase 11 |
| `wireless.c` 中 `c` 命令注释「擦除日志扇区（约 2s）」 | 同系漂移：实际仅擦首个 4KB 扇区（几十 ms） | Phase 11 |
| 缓冲满丢新记录 | 16 条环形缓冲满时静默丢弃新记录（刷写赶不上的极端情况）；取舍：保时序不保完整 | 设计取舍，保持现状 |
| 掉电丢失未刷写记录 | RAM 缓冲最多 16 条未刷入 Flash 的记录随掉电丢失 | 设计取舍，保持现状 |
| 导出为阻塞式 | 262016 条全量导出耗时较长（串口 115200 逐行发出），阻塞主循环；TIM7 中断持续运行（启动后不停），喂狗不受影响 | 设计取舍，保持现状 |
