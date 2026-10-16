# Phase 11: 健壮性与休眠代码清理 — 需求

> **2026-10-16 命名治理转接**：用户确认全量自维护名称统一，FR-7 由
> [Phase 15](../15-code-style-consistency/requirements.md) 承接，目标名及引用见其
> [符号表](../15-code-style-consistency/symbol-map.md)，不再等待 Phase 09/10 完成。
> 本轮仅提交文档，FR-7 尚未实施或验收；FR-4/8/9/10/11、休眠处置和旧验证记录保持原边界。

> **2026-10-16 关联边界**：[Phase 14](../14-dual-mode-tracking/requirements.md) FR-08/16/18/19/20 增加独立于SysTick的采集/控制预算及日志故障锁存，将Flash刷写移到主循环以保留TIM7保护响应；不是本阶段全部健壮性或休眠清理已经完成。不得借双模式启用休眠元素/PID；具体范围见 [Phase 14 计划](../14-dual-mode-tracking/plan.md) / [验证](../14-dual-mode-tracking/validation.md)。 2026-10-16实现状态及V-01～15通过证据见上述Phase14验证页，V-16～20仍待上机。

## 功能需求

### 健壮性（对应 plan Group 1）

- FR-1: `BSP_W25Q64_WaitIdle(timeout_ms)` 从调用时的 `HAL_GetTick()` 起按无符号差值计时；页编程 10 ms、4 KB 擦除 500 ms、64 KB 擦除 2500 ms、整片擦除 110000 ms。每轮读取状态寄存器，WIP 清零返回 0；持续忙至各自上限或任一 SPI 传输失败立即返回 1。`BSP_W25Q64_ReadStatus1`、`BSP_W25Q64_EnableWrite`、初始化、读、页编程和各级擦除均检查 HAL SPI 返回状态；片选在成功或失败后均拉高，失败向现有 `BSP_W25Q64_*` 的 0/1 接口及 Flash 服务层传播，不继续发送该操作的后续命令。地址、布局、正常数据内容和函数签名不变。整片擦除是未被运行链调用的维护 API，仅在看门狗启动前且非 ISR 上下文调用；已启动的 IWDG 约 6 秒超时，无法等待最长 110 秒整片擦除。
- FR-2: `BSP_Flash_Write(offset, buf, len)` 在已有 `s_param_shadow[4096]` 读取成功后，先比较影子缓冲的 `[offset, offset+len)` 与输入；完全相同时立即返回 0，不擦写。存在差异时才将输入复制进影子缓冲并擦除、回写原参数扇区全部 16 页。读取失败返回 1，不擦写；禁止再分配 4KB 栈缓冲或为比较重复读取整扇区。输入边界与现有返回约定保持不变。
- FR-3: `track_sensor.c` 的 `Track_UpdateError()` 和 `Track_UpdateSymmetry()` 分别计算横向、纵向电感和；某一对和为 0 时对应 `symmetry_x` 或 `symmetry_y` 置 0（表示该对无差值），另一对仍按原式独立计算。两对都为 0 时两个对称度均为 0，`Track_UpdateError()` 的主分母为 0 时仍返回偏差 0。非零输入的权重、取整、返回类型及状态机判定条件不变。
- FR-4: `g_element_enter_tick` 与 `g_app_control_ticks` 类型一致（int32_t 语义），任意运行时长下跷跷板超时判断正确（若 Phase 09 已处理，本项为核对关闭）。
- FR-5: `wireless_adjust()` 对现有 4～7 字节调参帧，在计算数值或写入 `KP_v/KI_v/KP_x/KD_x` 前，逐一确认 `dat[2..len-2]` 都在 ASCII `'0'`～`'9'`。任一字节不合法时清除已读取帧并直接返回，不改参数、不调用 `Param_MarkDirty()`、不发送正常回显。合法帧仍按现有 0.01 缩放及 `vp/vi/xp/xd` 命令格式处理；单字母 `d/c` 和其他长度的既有处理不变。

### 注释漂移修正（对应 plan Group 2）

- FR-6: [plan](plan.md) Group 2 表中所列全部注释条目逐条核对并改至与代码实际一致（容量 262016 条、TIM15 生产、几十 ms 擦除、看门狗约 6s、DL1B 2ms、编码器 1ms、TIM7 职责等）。

### 命名与结构（对应 plan Group 3）

- FR-7: `OK/UP/DOWN/LEFT/RIGHT/BACK/RST/ADJUST1/ADJUST2/FOUR` 等裸宏与 `uart/dat/time/max/min` 等高危全局名加模块前缀，全工程无同名歧义。
- FR-8: `key_scan` 区间判定表驱动化，判定结果与整改前逐点一致。
- FR-9: `font.h` 不再含非 static 的数组定义（改 static 或拆 .c），多 TU 包含无多重定义风险。
- FR-10: `bsp_adc.c` 绝对地址写法的工具链耦合完成评估，结论（维持/收敛）回填规约；链接行为不变。

### 休眠代码（对应 plan Group 4）

- FR-11: [休眠代码清单](../../research/2026-10-02-dormant-code-inventory.md) 中每一项有明确的处置结论（保留 / 接入 / 删除）并已落实；保留项须有注释说明保留理由。

## 技术约束

- TC-1: **行为等价硬约束**：除 FR-1/FR-3/FR-4/FR-5 列明的故障路径改善外，不改变任何正常运行时行为与控制参数数值。
- TC-2: 不修改 ST vendor 组件内部逻辑（`BSP/lcd/st7735*`、`Drivers/`）；`font.h` 属移植字库，允许 FR-9 的存储类调整。
- TC-3: 超时取值覆盖仓库内《W25Q64JV-DTR Revision J》第 90 页（PDF 第 91 页）交流特性表最大值：tPP=3 ms、tSE=400 ms、tBE2=2000 ms、tCE=100 s；FR-1 的分项上限分别加余量。当前 SysTick 优先级 0、TIM7 优先级 1，TIM7 日志写入时 `HAL_GetTick()` 可推进；不得改中断优先级。
- TC-4: 遵守 [techstack](../../techstack.md) 全部硬性约束；命名治理后构建零警告。

## 接口约定

- 2026-10-03 用户确认 FR-1 使用 10/500/2500/110000 ms 分项超时；依据仓库内 W25Q64JV 数据手册最大值及余量，保持 0/1 接口。
- 2026-10-03 用户确认 FR-3 某对电感和为 0 时其对称度取 0，表示无差值；适用于 `Track_UpdateError` 与 `Track_UpdateSymmetry` 两处。
- 重命名后的标识符映射表在 Group 3 实施时回填本节（供 CHANGELOG 与规约同步引用）。
- 休眠代码每项处置结论回填清单文档并同步本节链接。

## 非目标（Non-goals）

- 不改变任何控制参数数值与正常运行时行为（纯结构与健壮性整改）。
- 不动 ST vendor 组件内部逻辑；不升级工具链与 HAL 版本。
- 不在本阶段"接入"休眠功能（接入结论转独立 Phase）。
- 不重排存储布局、不改 CSV 格式、不改菜单交互。

> 2026-10-16 Phase 15 G2：现行传感器/控制符号按[完整映射](../15-code-style-consistency/symbol-map.md)迁移，
> 配置、计算顺序及休眠调用边保持；历史 validation 原文及本阶段实车门禁不变。
