# Phase 07: 运行数据记录 — 需求

2026-10-31文档迁移：**Phase → docs → 代码**。本页保留需求/任务/验收历史，不新增阶段、不修改实现或重新验收。

当前需求导航：[P07功能索引](../../../docs/traceability.md#p07)。具体输入输出、状态/错误/时序、代码符号与验证方法由下列docs条目唯一维护。

## 需求意图与现行功能条目

编号身份及原用户决定保留；摘要不替代详细契约。涉及冲突时，以docs中并列的当前事实、既有要求和待决定边界阅读，不能自动选代码或旧文档为新设计。

<a id="fr-01"></a>
### FR-1 记录节拍

目标与实现依据：[F19 日志格式容量](../../../docs/services/recording-and-export.md#f19)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-02"></a>
### FR-2 记录字段

目标与实现依据：[F19 日志格式容量](../../../docs/services/recording-and-export.md#f19)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-03"></a>
### FR-3 RAM队列

目标与实现依据：[F20 队列前台写入与故障](../../../docs/services/recording-and-export.md#f20)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-04"></a>
### FR-4 前台消费演进

目标与实现依据：[F20 队列前台写入与故障](../../../docs/services/recording-and-export.md#f20)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-05"></a>
### FR-5 会话建立

目标与实现依据：[F20 队列前台写入与故障](../../../docs/services/recording-and-export.md#f20)；[F02 起停、模式与首因](../../../docs/system/runtime-and-safety.md#f02)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-06"></a>
### FR-6 容量

目标与实现依据：[F19 日志格式容量](../../../docs/services/recording-and-export.md#f19)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-07"></a>
### FR-7 CSV

目标与实现依据：[F21 CSV与会话边界](../../../docs/services/recording-and-export.md#f21)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-08"></a>
### FR-8 清除

目标与实现依据：[F21 CSV与会话边界](../../../docs/services/recording-and-export.md#f21)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-09"></a>
### FR-9 日志配置门禁

目标与实现依据：[F20 队列前台写入与故障](../../../docs/services/recording-and-export.md#f20)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="tc-01"></a>
### TC-1 日志生产消费与兼容（第1项）

目标与实现依据：[F19 日志格式容量](../../../docs/services/recording-and-export.md#f19)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="tc-02"></a>
### TC-2 日志生产消费与兼容（第2项）

目标与实现依据：[F17 迁移保存与失败](../../../docs/services/parameters-and-tuning.md#f17)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="tc-03"></a>
### TC-3 日志生产消费与兼容（第3项）

目标与实现依据：[F20 队列前台写入与故障](../../../docs/services/recording-and-export.md#f20)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="tc-04"></a>
### TC-4 日志生产消费与兼容（第4项）

目标与实现依据：[F30 构建烧录与再生](../../../docs/guides/build-and-flash.md#f30)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="tc-05"></a>
### TC-5 日志生产消费与兼容（第5项）

目标与实现依据：[F21 CSV与会话边界](../../../docs/services/recording-and-export.md#f21)。状态：当前契约及历史替代见docs；不改变原验收状态。

## 授权与完成边界

本轮仅迁移说明。历史用户决定、动机、来源、兼容性及未决事项完整保留在下方基线；新的行为决定须明确日期、理由和适用范围，更新本需求及docs目标后独立提交，再进入实现。不得用本次文档完成替代AC5、R07原样本或任何硬件待验。

## 冻结历史基线（68cb508，非现行契约）

以下原文用于保留阶段决定、组顺序、需求历史与真实验证；不继续独立维护算法/布局/源码映射。当前契约先查上方docs条目。旧“本轮”“待执行”“当前”只指原记录时点，不覆盖后续决定。历史任务的全量软件门禁不自动成为以后每次修改的要求。

<details>
<summary>展开原始阶段记录（原文保留）</summary>

# Phase 07: 运行数据记录 — 需求

> 2026-10-18 目标增量：[Phase 16](../16-spi-wifi-link/requirements.md) FR-10～12/19～21
> 计划在完整96B记录后增加独立上传支路、缓存元信息及停车流式导出；256B头/96B记录/87336条容量和原故障停车不变。本轮仅文档，未修改日志实现；验收见[Phase 16验证](../16-spi-wifi-link/validation.md)。

> **2026-10-16 增量状态**：[Phase 14](../14-dual-mode-tracking/requirements.md) FR-17～FR-21 已以256B会话头/96B记录、1024槽、主循环写入、57列CSV和日志异常停车替代下文32B/16槽/TIM7写入/允许丢样的契约。参数扇区与1ms节拍保留，旧日志须升级前导出；目标格式、映射及验收见 [Phase 14 计划](../14-dual-mode-tracking/plan.md) / [验证](../14-dual-mode-tracking/validation.md)。下文及历史验证结果保留，不能替代新格式实车验证。 2026-10-16实现状态及V-01～15通过证据见上述Phase14验证页，V-16～20仍待上机。

> as-built 追认规约：以下需求逆向自现状代码，描述可观测、可检验的现有行为。
> 计划见 [plan.md](plan.md)，验证见 [validation.md](validation.md)。

## 功能需求

- FR-1: 系统以 1ms 采样节拍（TIM15 采样器，经 `App_SampleISR`）在运行态（`g_app_start_flag` 置位后）
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
- TC-2: 写粒度约束：日志写偏移必须 32B 对齐、长度必须 32 的整数倍（`BSP_FLASH_WORD_SIZE`）；
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

> 2026-10-16 Phase 15 G3：现行调度、HMI、参数和日志名称已按[符号表](../15-code-style-consistency/symbol-map.md)统一。
> 本文原阶段行为记录与未完成待办不因此重新验收；现行运行链以 Phase 14 和当前头文件契约为准，历史 validation 保留原文。

## Phase 17 当前位置（2026-10-23）

App按system/sensing/control/services/hmi归类。现行文件及逐函数位置见[迁移清单](../17-app-architecture/migration-map.md)。
历史路径、符号与验证结果保留；本说明不表示后续职责拆分或硬件验收完成。

</details>
