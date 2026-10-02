# Phase 11: 健壮性与休眠代码清理 — 实现计划

## 目标

消除已知健壮性隐患（死循环等待、栈上大缓冲、除零、类型截断），修正注释漂移，治理高危命名，并按休眠代码清单逐项落实处置。

## 背景与依据

- 关联宪章：[mission](../../mission.md) 成功标准 1"稳定完赛"、标准 4"规约与代码同步"；排期见 [roadmap](../../roadmap.md) Phase 11。
- 前置条件：建议在 Phase 08–10 之后实施（避免与功能整改冲突；`in_time` 截断与 `cask_flag` 已由 [Phase 09](../09-element-completion/plan.md) 先行处理的项，本阶段只做核对收尾）。
- 休眠代码处置依据清单文档 [specs/research/2026-10-02-dormant-code-inventory.md](../../research/2026-10-02-dormant-code-inventory.md)；逐项处置前与用户确认。
- 现状缺陷均已对照源码核实，分四组如下。

## 任务分组（Task Groups）

### Group 1: 健壮性

| 项 | 证据位置 | 问题 |
|---|---|---|
| W25Q64 等待无超时 | `bsp_w25q64.c:68` `w25q_wait_idle` | 裸 `while` 等 WIP 位，芯片异常时永久挂死（且可能发生在 TIM7 中断上下文） |
| 栈上 4KB 缓冲 | `bsp_flash.c:77` `BSP_Flash_Write` 内 `uint8_t current[4096]` | 单帧栈占用 4KB，溢出风险（`s_param_shadow` 已是静态，此处遗漏） |
| 除零保护缺失 | `track_sensor.c:56-57`、`track_sensor.c:79-80` | `symmetry_x/y` 计算除以电感和，电感全零时除零（同函数内 `denominator` 已有保护，此两处遗漏） |
| `in_time` 类型截断 | `element.h:12`（int16_t）vs `app.c:33`（int32_t） | 运行约 65s 后赋值截断（Phase 09 Group 2 先行处理，此处核对） |
| 串口数字位无校验 | `wireless.c:58-65` | 未校验 `dat[2..len-2]` 为 `'0'-'9'`，非数字字节被解析为参数值 |

### Group 2: 注释漂移修正（逐条核对）

| 位置 | 注释现状 | 实际（以代码为准） |
|---|---|---|
| `datalog.c:5-7` / `datalog.h:8-9` | "Bank2 Sector5+6 共 256KB"、"容量 8192 条 ≈ 8.2s" | W25Q64 日志区 8MB−4KB；`BSP_FLASH_LOG_SIZE/32` = **262016 条** |
| `datalog.h:32` | `DATALOG_CAPACITY` 注释"4096 条" | 同上，实为 262016 条 |
| `datalog.h:37` / `datalog.c:29` | "TIM6 控制环每拍调用" / "写指针（TIM6 生产）" | 实为 TIM15 采样器 1ms 经 `App_SampleISR`（`app.c:219`）调用 |
| `datalog.c:53` / `datalog.h:35` | "约 2s 阻塞擦除" | `BSP_Flash_LogErase` 只擦首个 4KB 扇区，几十 ms |
| `wireless.c:40` | `c` 命令"约 2s" | 同上，几十 ms |
| `bsp_wdt.h:7` / `app_config.h:13` | "约 1s 超时" | 实际约 6s（`bsp_wdt.c:23`，32000/64/3000）；"TIM7 喂狗"半句属实（`app.c:194`） |
| `bsp_wdt.c:24-25` | "菜单保存参数时 Flash 整扇区擦除关中断可达约 2s" | 片内 Flash 旧设计残留；W25Q64 擦除几十 ms 且不关中断 |
| `bsp_dl1b.h:12` | "周期调用（5ms）" | 实际 TIM6 2ms 轮询（`app.c:142`） |
| `bsp_encoder.h:7` | "每 2ms 读一次计数并清零" | 实际 TIM15 采样器 1ms 读取（2ms 为控制环累计消费） |
| `app_config.h:9` | `TASK_PERIOD_MS` 职责注释含"DL1B/…/按键" | DL1B 已迁 TIM6，按键扫描在主循环；TIM7 实为日志刷写/电池/喂狗/状态灯 |
| `datalog.c:5` / `datalog.h:8` 布局描述 | "Bank2 Sector5+6" 片内 Flash 布局残留 | 参数+日志全部在 W25Q64 |

### Group 3: 命名与结构治理

- 裸宏/高危全局名加模块前缀：按键值宏 `OK/UP/DOWN/BACK/LEFT/RIGHT/RST/ADJUST1/ADJUST2/FOUR`（`menu.h:7-16`）；全局变量 `uart/dat/time`（`app.c:26-33`）、`max/min`（`track_sensor.c:28-29`）；高危函数名 `judge/straight_judge` 等（随 Group 4 一并处置）。
- `menu.c` 的 `key_scan`（`menu.c:76-135`）区间判定链改表驱动。
- `BSP/lcd/font.h:4`：`const unsigned char asc2_1608[95][16]` 为头文件中的**非 static 定义**，改 `static` 或拆出 `font.c`。
- `bsp_adc.c:21-23`：DMA 缓冲绝对地址 `0x30000000` 的 AC5 `__attribute__((at))` 与 GCC `section` 双写——评估工具链耦合，确认与 `MDK-GCC/STM32H743VITx.ld` 的一致性策略（只评估与必要的注释同步，不改链接行为）。

### Group 4: 休眠代码处置
> 依据 [休眠代码清单](../../research/2026-10-02-dormant-code-inventory.md) 逐项落实"保留 / 接入 / 删除"；**每项处置前与用户确认**。

已核实的候选项（以清单文档为准）：

- 空壳函数：`element_judge`、`straight_judge`（`element.c:20/29`）；`judge`、`pre_out_judge`（`roundabout.c:244/212`）
- 从未被调用：`R_reroundabout_judge`（`roundabout.c:133`）、`entered_entered_judge`（`roundabout.c:202`）
- 写而不读：`sign_round`（`roundabout.c:25`，仅 `control.c:116/134` 赋值）
- 闲置宏：`roundabout.h` 的 `ISLAND_RPREENTER/TURN_RIGHT/EXIT/LENTER/RENTER/TURN_TURN_LEFT`（与 Phase 09 协调：左环接入后可能复用）
- 未用的环岛角度参数 `enter_angle1/enter_angle2/out_angle1`（`roundabout.c:30-32`，角度法残留）
- 未被调用的薄封装 `front_adc_init`（`track_sensor.c:133`）等，以清单为准

## 实现顺序与依赖

1. Group 1（健壮性）优先：挂死与栈溢出是竞赛现场最不可接受的故障。
2. Group 2（注释）纯文档性修改，可与 Group 1 并行评审、分开提交。
3. Group 3（命名）改动面广，安排在功能整改（Phase 08–10）全部合并后，一次扫净。
4. Group 4（休眠代码）最后：与 Phase 09 确认哪些宏/函数已被复用，再对剩余项逐个处置。

## 风险与取舍

- **纯结构整改的行为等价性**：本阶段不允许改变任何运行时行为与控制参数数值；验证以"构建零警告 + 等价性回归"为门槛。
- **命名治理的波及面**：`OK/UP` 等裸宏改名涉及 menu 全文件，宜机械替换一次完成，避免半改状态。
- **休眠代码"接入"项**：若处置结论是接入（如角度法环岛），转入后续独立 Phase，不在本阶段实施功能。
- 明确排除：不动 ST vendor 组件（`BSP/lcd/st7735*.c/h`、`Drivers/`）内部逻辑；不改控制参数；不改 Flash 布局。
