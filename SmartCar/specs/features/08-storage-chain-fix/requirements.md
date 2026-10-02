# Phase 08: 存储链修复 — 需求

## 功能需求

- FR-1: `App_Init` 必须在 `Param_Load` 之前完成 `BSP_Flash_Init()`，并检查其后的可用性状态；初始化失败时通过串口输出明确错误信息，系统降级运行（参数用默认值、日志关闭），不得卡死。
- FR-2: `Param_Load` 必须检查 Flash 读取结果：读失败或校验失败时串口提示并载入默认值；默认值回写失败时串口提示。
- FR-3: `Param_Save` 的写失败必须可观测（返回值被检查且串口提示），不得静默吞错。
- FR-4: 串口无线调参命令（`vp/vi/xp/xd` 系列）命中后，修改结果必须按 [plan](plan.md) 定稿的策略落盘；掉电重启后串口调参结果仍然存在。
- FR-5: `fan_duty` 参数链首尾一致：按与用户确认的取舍，或菜单/存储中的 `fan_duty` 实际驱动风扇 PWM 输出，或从调参项与参数表中移除；不得保留"可调可存但不生效"的断头状态。
- FR-6: 启动键按下时若日志初始化（`Datalog_Start`）失败，串口输出"日志不可用"提示，车辆循迹功能不受影响。

## 技术约束

- TC-1: 不改变 W25Q64 存储布局（参数区末尾 4KB 扇区、日志区其余空间）与参数镜像格式（magic/version/CRC16 及字段顺序），已存数据保持兼容。
- TC-2: 不重构 `BSP/bsp_flash.c` 的读-改-擦-写与边写边擦机制；本阶段只做调用链接通与错误处理。
- TC-3: Flash 擦写只允许发生在停车态（主循环上下文）；TIM6/TIM7/TIM15 中断路径不得新增任何阻塞擦写（[techstack](../../techstack.md) 硬性约束 2、3）。日志刷写维持现状（TIM7 页编程为既有行为，不在本阶段改动）。
- TC-4: 遵守分层红线：App 层只经 `bsp.h` 接触硬件（[techstack](../../techstack.md) 硬性约束 1）。
- TC-5: 串口错误提示复用 USART1 调试通道与既有文本协议风格，不新增协议命令。

## 接口约定

- `BSP_Flash_Init(void)`：既有接口不变，调用方为 `App_Init`（`App/app.c`），调用顺序位于 `BSP_UART_Init` 之后、`Param_Load` 之前。
- `Param_Load/Param_Save`：函数签名不变；内部补返回值检查与串口提示。
- 无线调参协议：命令格式（字母×2 + 数字×N + 终止字节）与回显格式不变，仅新增落盘副作用。
- `fan_duty`：若取舍为"接通"，占空比语义保持 0~10000 万分比（[techstack](../../techstack.md) 硬性约束 4），`FAN_DUTY_IDLE/RUN` 宏的去留由 Group 3 确认结论回填。

## 非目标（Non-goals）

- 不改变存储布局与 CRC 格式，不做旧数据迁移。
- 不重构 `bsp_flash`/`bsp_w25q64` 内部机制（含 `w25q_wait_idle` 超时、4KB 栈缓冲等问题，转 [Phase 11](../11-hardening-cleanup/plan.md)）。
- 不改动调参语义本身（"双档参数 vs 状态机每拍覆写"冲突转 [Phase 10](../10-control-consistency/plan.md)）。
- 不新增上位机、不改变 CSV 导出格式。
