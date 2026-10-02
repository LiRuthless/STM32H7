# Phase 07: 运行数据记录 — 需求

> as-built 追认规约：以下需求逆向自现状代码，描述可观测、可检验的现有行为。
> 计划见 [plan.md](plan.md)，验证见 [validation.md](validation.md)。

## 功能需求

- FR-1: 系统以 1ms 采样节拍（TIM15 采样器，经 `App_SampleISR`）在运行态（`Start_flag` 置位后）
  记录结构化运行数据，每条记录定长 32 字节。
- FR-2: 记录内容字段与类型固定为 `datalog_record_t`（见接口约定字段表）；其中编码器计数与陀螺 xyz
  为 1ms 实时值（取 TIM15 采样器缓存），电感/偏差/方向输出等控制量按 2ms 控制拍更新，
  陀螺 xyz 以 °/s 取整存储。
- FR-3: 记录先进入 16 条 RAM 环形缓冲；生产者（TIM15，优先级 0）与消费者（TIM7，优先级 1）
  单生产单消费免锁；缓冲满时丢弃新记录（保时序不保完整）。
- FR-4: TIM7（5ms 辅助环）把缓冲记录刷入 W25Q64 日志区：攒满 8 条 = 256B = 1 个 Flash 页整页编程；
  运行中不足 8 条不刷；`Datalog_Stop()` / `Datalog_Dump()` 时兜底刷出不足一页的剩余记录；
  任一刷写失败即停止记录，保住已有数据。
- FR-5: 按启动键 K1 起跑即开始记录（`DATALOG_ENABLE=1` 时）；日志区「边写边擦」：
  起跑前仅擦除首个 4KB 扇区（几十 ms，几乎无延时），之后每跨入一个新 4KB 扇区自动先擦除。
- FR-6: 记录容量 = (8MB − 4KB 参数扇区) / 32B = 262016 条，1ms/条时约 262 秒；
  写满自动停止记录。`DATALOG_DIV` 编译期分频（N = 每 N 拍记一条）可降频延长总时长。
- FR-7: 停车后串口发 `d` 阻塞导出全部记录为 CSV：先发表头行，再逐条输出 16 列数据行，
  以 `EOF` 行结尾；导出前自动停止记录并兜底刷写。导出内容可直接粘贴到 Excel / 逐飞助手分析。
- FR-8: 停车后串口发 `c` 复位日志区（擦除首个扇区 + 重置擦除前沿与计数），不开始记录；
  回显 `log erasing...` → `log erased`（失败时 `erase fail`）。
- FR-9: `DATALOG_ENABLE` 编译开关（`app_config.h`，默认 1）：置 0 时按启动键不触发记录，
  记录链路整体关闭。

## 技术约束

- TC-1: 存储后端为板载 W25Q64（SPI1，30MHz），不使用片内 Flash；日志区固定布局为
  `0x000000 ~ 0x7FEFFF`（8MB 减去末尾 4KB 参数扇区），追加写模式。
- TC-2: 写粒度约束：日志写偏移必须 32B 对齐、长度必须 32 的整数倍（`FLASH_WORD_SIZE`）；
  记录定长 32B = 1 个 Flash Word 即源于此约束。
- TC-3: 采集在 TIM15（优先级 0）、刷写在 TIM7（优先级 1），不得改动中断优先级配置
  （技术栈硬性约束 3）；`Datalog_Push` 内不得有阻塞调用。
- TC-4: 编译开关集中在 `app_config.h`：`DATALOG_ENABLE` / `DATALOG_DIV`。
- TC-5: 导出与擦除命令仅在停车态可用（串口命令解析本身只在停车态进行，
  见 [06-hmi-tuning](../06-hmi-tuning/requirements.md) FR-15）。

## 接口约定

### 记录格式（`datalog_record_t`，32 字节 = 1 Flash Word）

| # | 字段 | 类型 | 说明 | 更新节拍 |
|---|---|---|---|---|
| 1 | tick | uint32 | 采样节拍计数（1ms） | 1ms |
| 2~5 | adc[0..3] | int16 ×4 | 四路电感滤波值（12bit；[0]横左 [1]竖左 [2]竖右 [3]横右） | 2ms |
| 6 | speed_l | int16 | 左轮 1ms 编码器计数 | 1ms |
| 7 | speed_r | int16 | 右轮 1ms 编码器计数 | 1ms |
| 8 | track_error | int16 | 循迹偏差 | 2ms |
| 9 | track_out | int16 | 方向环输出 | 2ms |
| 10~12 | gyro_x / gyro_y / gyro_z | int16 ×3 | 陀螺仪角速度，°/s 取整（原始值 /14.3） | 1ms |
| 13 | dl1b_mm | uint16 | DL1B 激光距离 mm（8192 = 无效） | 2ms |
| 14 | battery | uint16 | 电池电压滤波值（12bit） | 5ms |
| 15 | kernel_state | uint8 | 主状态机状态 | 2ms |
| 16 | roundabout_state | uint8 | 环岛子状态机状态 | 2ms |

### CSV 导出格式（串口 `d` 命令响应）

```
dump start\r\n
tick,adc0,adc1,adc2,adc3,spdL,spdR,err,tout,gx,gy,gz,dl1b,bat,kstate,rstate\r\n
<16 列十进制数据行>\r\n  × s_count 条
EOF\r\n
```

列序与字段表一一对应；全部为十进制整数（无浮点）；每次导出固定以 `EOF` 结束。

### 控制流

```
K1 按下 → Datalog_Start（擦首扇区，几十 ms）→ s_active=1
TIM15(1ms) → App_SampleISR → Datalog_Push → RAM 环形缓冲（16 条）
TIM7(5ms)  → Datalog_Flush → 攒 8 条=256B 页 → BSP_Flash_LogWrite（跨扇区自动先擦）
停车后发 'd' → Datalog_Stop（兜底刷写）→ 逐条 LogRead → CSV 串口输出 → EOF
停车后发 'c' → Datalog_Start + Datalog_Stop（仅复位，不记录）
```

## 非目标（Non-goals）

- 不做多段日志 / 断点续录：每次起跑复位日志区，只保留最近一次记录。
- 不做记录内容的运行期可配置（字段固定，改字段即改结构体与 CSV 列）。
- 不做上位机解析软件（复用 Excel / 逐飞助手）。
- 不做未刷写记录的掉电保护（RAM 缓冲最多 16 条随掉电丢失，为已接受取舍）。
- 不记录浮点原始值（陀螺取整到 °/s，控制量本身为整数类型）。
