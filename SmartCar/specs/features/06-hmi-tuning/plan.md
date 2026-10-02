# Phase 06: 人机交互与调参 — 实现计划

> **as-built 追认规约**：本阶段代码已存在（移植自省赛 STC 工程 `Sirius20260718`），
> 本文件为存量功能的规约化追认，任务分组全部为已实现结构的追溯描述。
> 需求见 [requirements.md](requirements.md)，验证见 [validation.md](validation.md)。

## 目标

交付车载人机交互与调参链路：编译期二选一的显示方案（板载屏仪表盘 / 外接屏完整菜单）、
双路 ADC 分压按键、串口无线调参协议、双档参数持久化（W25Q64 + CRC16）。

## 背景与依据

- 关联宪章：[mission.md](../../mission.md)「车载人机交互」「参数持久化」条目；
  [techstack.md](../../techstack.md) 硬性约束 1（App 只经 `bsp.h` 接触硬件）、约束 4（占空比 0~10000 万分比语义）。
- 前置条件：Phase 01 运行时架构（主循环停车态调度、TIM15/TIM6/TIM7 契约）、
  Phase 02 BSP 平台层（`bsp_lcd` / `bsp_adc` / `bsp_key` / `bsp_uart` / `bsp_flash`）。
- 算法出处：菜单/按键/串口调参移植自源工程 `menu.c` / `config.c(uart_adjust)`；
  参数存取替代源 `eeprom.c`；仪表盘为本工程新增（板载小屏简化交互）。

## 任务分组（Task Groups）

### Group 1: 显示方案编译期二选一
- [x] `app_config.h` 提供 `LCD_TARGET_EXTERNAL` 开关：`=0` 板载 0.96 寸屏（默认，单页仪表盘），`=1` 外接屏（完整按键菜单）；两屏共用 SPI4，互斥不可同时接。
- [x] `App/dashboard.c`：板载屏单页仪表盘。`Dashboard_Init()` 绘制固定标签（5 行布局）；
      `Dashboard_Update()` 以 `HAL_GetTick()` 做 100ms 节流刷新数值区
      （四路电感 / 偏差+方向输出 / 电池+DL1B / 陀螺 xyz / 轮速+里程+kernel 状态）。
- [x] `App/menu.c`：外接屏完整菜单。HOME(0) → 监视页 21/22/23 + 调参页 ADJUST1(11)/ADJUST2(12)；
      页面切换时清屏、光标变化时局部重绘，避免每帧闪烁。
- [x] `App/app.c`：停车态（`key_flag==0`）主循环按开关分发 `menu()` / `Dashboard_Update()`。

### Group 2: 按键输入
- [x] `App/menu.c::key_scan()`：双路 ADC 分压键盘扫描。KEY_ADC1(PA2)=UP/DOWN/OK/LEFT/RIGHT，
      KEY_ADC2(PA3)=RST/ADJUST1/ADJUST2/FOUR/BACK；每路 3 次平均后按 `/100` 区间判定键值，
      返回值经 `key != last_key` 沿检测消抖；判定后原始值再经 α=0.7 低通供 HOME 页显示。
- [x] `App/menu.c::key_action()`：键值→页面导航/调参动作分发；ADJUST1/2 键一键进入调参页
      并 `Param_SelectGear` 挂对应档位；LEFT/RIGHT 按 ±0.001（PID）/ ±100（速度/风扇）步进；第 5 项 OK 切换运行/空闲风扇值。
- [x] `BSP/bsp_key.c`：启动键 K1(PC13，下拉输入) 按下沿检测 + 30ms 消抖，
      供主循环起跑判定（与 ADC 键盘独立）。

### Group 3: 参数持久化
- [x] `App/param.c`：`param_store_t` 落盘镜像（magic `0xA55A3C3C` + version 2 +
      CRC16-CCITT（poly 0x1021、初值 0xFFFF，覆盖范围自 `page` 起、不含头部）+
      菜单 page/arrow + KP_v/KI_v + 慢/快双档各 {KP_x, K2P_x, KD_x, base_speed, fan_run, fan_idle}；Phase 08 从 v1 迁移旧 fan_duty 为 fan_run。
- [x] 存储位置：W25Q64 末尾 4KB 扇区（0x7FF000），经 `BSP_Flash_Read/Write`（扇区读-改-擦-写）。
- [x] `Param_Load()` 上电即加载：magic/version/CRC 任一校验失败 → 载入默认值并立即回写；
      默认挂慢速档（**与源工程差异**：源工程上电不读 Flash、参数固定默认值，本工程上电即恢复已存参数）。
- [x] float 按 4 字节完整存取（**与源工程差异**：修复源 `eeprom.c` 只写 float 前 2 字节的 bug）。
- [x] 保存时机：调参页按键松手（`key==0` 且 `param_dirty`）或 BACK 退出调参页 → `Param_Save()`；串口参数静默 3 秒保存、起跑前刷新
      （**与源工程差异**：源工程 ADJUST 页每帧写 Flash，本工程改为空闲单次写入，减少擦除损耗）。

### Group 4: 串口无线调参
- [x] `App/wireless.c::wireless_adjust()`：USART1 115200，仅停车态（主循环 `key_flag==0` 分支）解析。
- [x] 单字母命令：`d` = 阻塞导出日志 CSV（联动 Phase 07）；`c` = 复位日志区。
- [x] 参数帧：双字母命令 + 1~4 位 ASCII 数字 + 1 终止字节（总长 4~7 字节），数值 = 数字 × 0.01；
      支持 `vp/vi`（KP_v/KI_v）、`xp/xd`（KP_x/KD_x），命中回显 `xx=NN`（NN=数值×100 取整）。
- [x] 删除源工程 STC 专有逻辑（0x7F 串口自动下载）。

## 实现顺序与依赖

Group 2（按键）→ Group 1（显示）：按键扫描值是菜单页面内容的输入；
Group 3（参数）依赖 Group 1/2 的全局状态（`page`/`arrow`/PID 全局量）；
Group 4（串口）独立，但 `d`/`c` 命令依赖 Phase 07 的 datalog。
全部模块由 Phase 01 的主循环在停车态统一调度。

## 风险与取舍（已知缺陷，如实记录）

| 风险/缺陷 | 现状 | 整改指向 |
|---|---|---|
| **`BSP_Flash_Init()` 全工程从未被调用** | `bsp_flash.c::s_ready` 恒 0 → `BSP_Flash_Read/Write/Log*` 全部静默返回失败 → **参数保存/加载与数据记录在现状代码中实际全部不生效**（掉电后参数回默认值） | **Phase 08** |
| 串口调参不触发 `Param_Save()` | `wireless_adjust` 只改 RAM 全局量，掉电即失（菜单保存路径可间接带走无线改的 KP_v/KI_v/KP_x/KD_x，但无线单独使用时不落盘） | **Phase 08** |
| 风扇参数无输出链路 | 菜单/参数区保存运行/空闲风扇值；Phase 08 接通两者到 PWM，并迁移旧镜像 | Phase 08 |
| 串口协议与菜单能力不对齐 | 无线仅支持 vp/vi/xp/xd，不支持 K2P_x / base_speed / fan_duty / 档位切换 | 如实记录（本规约 FR-11），扩展另行排期 |
| 按键先判定后滤波 | `key_scan` 用未滤波的 3 次均值做区间判定，低通只影响显示值；分压网络抖动时可能误判键值 | Phase 11 |
| 键值裸宏撞名风险 | `OK/UP/DOWN/LEFT/RIGHT/BACK/RST` 等为 `menu.h` 裸宏，通用名易与其他库冲突 | Phase 11 |
| `key_scan` 区间判定魔数堆 | `/100` 区间上下限为一组散置魔数，无集中表驱动 | Phase 11 |
| App 层轻微越层 | `dashboard.c` 直接调用 `HAL_GetTick()`（app.c 亦有 HAL_GPIO/HAL_TIM 直用），与技术栈硬性约束 1 的精神有出入（时基服务，非外设操作） | Phase 11 评估 |

**被排除的方案**：片内 Flash 存参数（已明确全部走 W25Q64）；菜单每帧写 Flash（已改空闲单次写）；
运行中刷屏幕（屏幕与 IMU 共用 SPI4，运行中刷屏会与 IMU 采样交错，架构上禁止）。
