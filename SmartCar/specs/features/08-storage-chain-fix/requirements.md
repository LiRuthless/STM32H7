# Phase 08: 存储链修复与风扇参数接通 — 需求

## 决策记录

- 日期：2026-10-03。
- 用户确认先规划并实施 Phase 08，Phase 12 的 V-9/V-10 实车验证仍待执行；路线图不得将 Phase 12 标为完成。
- 用户确认串口参数在最后一次修改后静默 3 秒合并保存；若按启动键时仍有待保存修改，先写参数再开始日志/运行。
- 用户确认每个速度档分别设置运行和空闲风扇占空比；默认运行值 1600、空闲值 1100。
- 用户确认参数镜像升版并迁移 v1：旧 `fan_duty_low/high` 作为运行值，新增空闲值初始化 1100。
- 用户确认外接菜单保留 5 项，第 5 项以 OK 在运行/空闲风扇值间切换，LEFT/RIGHT 以 ±100 调值。

## 功能需求

- FR-1：`App_Init()` 在 `BSP_UART_Init()` 后、`Param_Load()` 前调用 `BSP_Flash_Init()`。初始化/读取不可用时 `Param_Load()` 载入 RAM 默认值并串口提示；Flash 不可用时参数存储和日志功能降级关闭，不阻止车辆控制初始化。
- FR-2：`Param_Load()` 检查 Flash 读取、镜像版本、magic 和 CRC16。读失败、未知版本或校验失败时载入默认值；默认镜像写回失败必须串口报告。有效 v1 镜像按 FR-6 迁移；迁移写回失败时保留已读参数在 RAM 使用、报告保存失败并关闭日志/后续存储写入。
- FR-3：`Param_Save()` 检查 `BSP_Flash_Write()` 结果；写失败串口输出 `parameter save fail\r\n`，成功不增加额外协议回显。菜单松键保存和 BACK 保存保持现有时机。
- FR-4：合法 `vp/vi/xp/xd` 串口命令仅在停车主循环解析并立即生效；命中后标记参数待保存。最后一次命令后 3000ms 无新命令时由停车主循环调用 `Param_Save()`。启动键先调用 `Param_FlushPending()`；Flash 写失败可观测且继续进入起跑流程。
- FR-5：启动时若 Flash 不可用，日志保持关闭并输出 `log unavailable\r\n`；若 Flash 可用但 `Datalog_Start()` 失败，输出相同提示并继续循迹启动。无线 `c` 命令失败继续使用既有 `erase fail` 回显。
- FR-6：版本 2 镜像在 v1 原字段顺序及偏移之后追加慢速档与快速档空闲风扇 `int16_t` 字段；参数区仍为 W25Q64 末尾 4KB，magic 不变，CRC16 覆盖自 `page` 至 v2 镜像末尾。有效 v1 镜像的所有原字段原值保留，旧低/高速 `fan_duty` 转为对应运行值，空闲值分别设 1100，然后写入 v2。未知版本不尝试迁移。v2 新建默认值为两档运行 1600、空闲 1100；方向、速度参数等既有默认保持原值。
- FR-7：当前档运行时风扇 PWM 使用该档运行值，停车/安全停车使用该档空闲值；`fan_duty` 与 `fan_duty_idle` 均为 0～10000 万分比。加载镜像时超范围值钳位到 0 或 10000；菜单每次 ±100 后同样钳位。
- FR-8：`PAGE_ADJUST1/2` 保持 5 行。光标第 5 项显示 `FAN-R` 或 `FAN-I`；OK 在两者间切换；LEFT/RIGHT 分别调整所选值 ±100。切换速度档时分发该档两个风扇值。页面、按键步进、两档隔离和菜单光标合法范围与实现一致。

## 技术约束与兼容性

- 保持 BSP Flash 函数签名、Flash 区域地址、串口命令帧/回显格式、CSV 格式、TIM 周期与中断优先级不变；只扩展参数镜像版本。
- Flash 参数保存和日志启动/收尾写入仅在停车主循环执行；不在 TIM6/TIM7/TIM15 新增参数擦写。
- App 层硬件访问仍经 `bsp.h`；不重构 Flash 读改擦写机制。

## 实现映射

| 需求 | 实现位置 | 验证 |
|---|---|---|
| FR-1/FR-5 | `App/app.c::App_Init`, `App_Loop` | V-1～V-3 |
| FR-2/FR-3/FR-6/FR-7 | `App/param.c::Param_Load`, `Param_Save`, `Param_SelectGear` | V-4～V-6 |
| FR-4 | `App/wireless.c::wireless_adjust`; `App/param.c::Param_MarkDirty`, `Param_ServiceSave`, `Param_FlushPending` | V-7～V-8 |
| FR-7/FR-8 | `App/menu.c::key_action`, `menu_draw_content`, `menu_draw_cursor`; `App/motor.c` 风扇运行态全局值 | V-9～V-10 |

## 非目标

- 不改变控制算法/控制参数、日志记录格式、串口命令集合或 Flash 底层擦写机制。
- 不处理 W25Q64 超时、4KB 栈缓冲及其他 Phase 11 项目。
