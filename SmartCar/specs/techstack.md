# 技术栈（Tech Stack）

本文记录当前选型、技术约束与实现导航。阶段进度和历史验证结果见 [roadmap](roadmap.md) 与所属功能 `validation.md`；开发验证的执行规则见 [AGENTS](../AGENTS.md#验证要求2026-10-31-起)。

## 语言、芯片与运行时

- **C99**：Keil 开启 C99，GCC 使用 `-std=c99`。
- **STM32H743VIT6**：Cortex-M7，目标 SYSCLK 480 MHz，双精度 FPU，LQFP100。
- 不使用 RTOS；裸机前台与三线中断调度：TIM15 采样 1ms、TIM6 控制 2ms、TIM7 辅助 5ms。
- I-Cache 开启，D-Cache 关闭；App 使用静态分配，不引入动态堆分配。

## 框架与来源

| 类别 | 当前选型/版本 | 边界与依据 |
|---|---|---|
| Cube 固件包 | STM32Cube FW_H7 **V1.11.2，锁定** | `SmartCar.ioc` 的 FirmwarePackage 声明；仓内 HAL 组件版本宏为 1.11.3，包版本与组件版本分别记录，不据此改动或升级驱动 |
| 配置工具 | STM32CubeMX 6.11.1；MDK-ARM V5.27 工程目标 | `.ioc` 记录 CubeMX 管理的配置；不覆盖 BSP 代码接管外设或 USER CODE 补丁 |
| MCU 基础库 | 仓内 `Drivers/` 的 HAL/CMSIS | 不擅自替换仓内库；生成区和第三方边界见代码标准 |
| LCD 组件 | ST MCD ST7735，来自 WeAct 核心板 SDK | 当前适配在 BSP/lcd 与 bsp_lcd；第三方组件不纳入自维护代码风格全量迁移 |
| 电磁算法来源 | [STC AI8051U 省赛快照](../../Material/Sirius20260718/)（2026-07-18） | 一次性算法出处对照，不按 SmartCar 功能阶段持续维护 |
| 灰度采集来源 | 用户提供 LQ_Track.c，TC387，V2.0.0（2026-03-24） | 来源摘要与适配差异见 [Phase14](features/14-dual-mode-tracking/requirements.md)；原样本缺失与实机待验见 roadmap |

`.ioc`、生成的 Core 配置、USER CODE 与 BSP 接管代码共同决定当前外设行为。TIM15、SPI1/W25Q64、IWDG 等由 BSP 代码配置；SPI4/5 时钟在 main 的 PLL3 配置后，还由生命周期初始化调用 BSP 恢复选择。不能把 `.ioc` 单独视为全部外设的唯一事实源。

## 存储与 DMA

- 业务持久化使用板载 **W25Q64 8MB SPI Flash**（SPI1，目标 30MHz）：末尾 4KB 扇区保存参数，其余为日志区。
- 参数为 88B v3 镜像，保留有效 v1/v2 迁移；日志为 256B 会话头与 96B 记录。布局、CRC、迁移与失败语义见 [Phase14](features/14-dual-mode-tracking/requirements.md)。
- 参数和日志不写片内 Flash；固件程序本身仍由链接脚本放在片内 Flash，不能将“不使用片内 Flash”理解为程序不占片内 Flash。
- ADC DMA 缓冲固定到 D2 SRAM1 `0x30000000`；默认 RAM 在 AXI SRAM。DMA1 不能访问 DTCM，不得将 DMA 缓冲放入 DTCM。
- SPI2 DMA 缓冲固定在 AXI `0x2407F000/0x2407F400`，GCC 链接划出独立区域；双工程栈预留 8KB。静态资源预算不替代实测栈峰值。

## 构建与工具链

| 用途 | 工具 | 入口/限制 |
|---|---|---|
| Keil 目标 | **ARMCC V5.06 update 5（AC5，锁定）** | `MDK-ARM/SmartCar.uvprojx`；实际工程超出评估版 32KB 限制，须正式授权；GCC 结果不能代替 AC5 验收 |
| 日常构建 | EIDE 内置 arm-none-eabi-gcc，`-O2 -std=c99 -Wall` | `MDK-GCC/build.sh`；VS Code 默认构建任务 |
| 烧录 | OpenOCD＋ST-Link | `MDK-GCC/flash.sh`；按任务授权执行 |
| 外设再配置 | CubeMX | 同步 `.ioc`/生成配置，保留 USER CODE 和 BSP 接管；见硬性约束 |

## 验证方式（2026-10-31 起）

- 强制项：格式/命名、注释准确性、上下文逻辑审查、固件修改后的当前配置零警告编译、R08 人工证据和实现提交前检查。
- `tests/` 已有 Python 驱动真实 C 的宿主验证、结构/迁移检查和资源工具；电脑端工具还有独立 unittest。它们的存在不表示后续每次修改必须全量运行。
- 双 LCD 构建、宿主行为、架构/迁移及资源/历史对照按本次验收目标选取并记录原因；优先复用，尽可能减少新增软件测试脚本。真实硬件模块测试与相关板上验收保留。
- 纯文档变更只做文档交叉核对，不要求固件编译或软件行为回归。详细范围、优先关系与记录要求以 [AGENTS](../AGENTS.md#验证要求2026-10-31-起) 为准；既有结果及硬件待验状态不改写。

## 硬性约束

1. **分层边界**：App 仅经公开 BSP 接口接触硬件，禁止在 App 直接包含 HAL/CubeMX 外设头或使用句柄、寄存器。常用聚合入口为 `bsp.h`。
2. **控制拍标定**：控制系数按 **TIM6=2ms** 标定；改变周期须重标定相关系数并先更新规约。
3. **中断优先级**：TIM15 与 TIM6 同为 NVIC 优先级 0、互不抢占；采样累计量和 SPI4 使用依赖此契约，不得擅自改动。
4. **占空比语义**：对外统一 0～10000 万分比，CCR 换算只在 BSP 内完成。
5. **CubeMX 再生保护**：保留 `Core/Src/main.c` USER CODE 的 PLL3 配置和 SPI45SEL→PLL3Q 补丁，以及 `Lifecycle_Init → BSP_Runtime_ConfigureSpiClock` 在全部 MX 初始化后恢复选择的调用。该 BSP 位于 `BSP/bsp_runtime.c`，不能只保留 main 中的写寄存器语句。
6. **D-Cache 保持关闭**：ADC DMA 当前免 Cache 维护依赖此条件；开启前须先完成全量 DMA 缓冲维护设计。
7. **版本锁定**：不擅自升级 Cube 固件包、仓内驱动、AC5 或 EIDE GCC；升级须独立治理并更新选型与验证要求。
8. **规约先行**：代码及工程配置（含纯机械修改）先完成并独立提交规约；未决设计未清空前不得编码。只读调查与构建取证可不改规约。
9. **未决设计**：影响硬件、行为、控制参数、状态机、协议、存储、实时性、安全、兼容性或休眠代码处置时，按 mission 提供事实与选项，由用户决定，不自行采用默认方案。
10. **追溯与真实记录**：每条Phase实现需求须引用docs准确功能条目，再由docs定位实现与验证项，不在Phase复制实现表，验证可为具体逻辑审查、构建或选定测试；未执行不标通过，硬件待验不由宿主结果替代。
11. **写作标准**：自维护 C/H 持续执行 [code-style](code-style.md) CS-01～15，第三方/生成区和固定名称遵守明确边界。
12. **注释验收**：采用“C 完整、H 摘要”，覆盖/格式/人工内容/行为保持分别验收；R08 证据及提交前检查继续强制，不能用历史构建或模板数量替代内容审查。

## 当前架构与功能导航

当前详细说明由[docs入口](../Docs/index.md)统一组织；[Phase/FR索引](../Docs/traceability.md)将阶段需求指向具体功能，[架构与模块反查](../Docs/architecture.md)再进入功能页定位代码。

- 双传感、手工标定与姿态：[感知契约](../Docs/sensing/sensors-and-calibration.md)。
- 起停、保护及前台/ISR：[运行安全](../Docs/system/runtime-and-safety.md)。
- 模式、控制器和配置：[控制](../Docs/control/tracking-and-controllers.md)。
- 参数及调参：[参数契约](../Docs/services/parameters-and-tuning.md)；记录/导出：[日志契约](../Docs/services/recording-and-export.md)。
- SCB/1、SPI2与上传：[通信契约](../Docs/services/communication.md)；硬件时钟/DMA：[BSP](../Docs/platform/board-and-bsp.md)。
- 构建与CubeMX：[工程指南](../Docs/guides/build-and-flash.md)；验证与R08：[验证维护](../Docs/guides/validation.md)。

Phase17设计/迁移/评审保留为阶段基线，实际阶段状态见roadmap，历史算法和镜像对照保留原提交路径。本文只维护技术选型和硬性约束，不再另列一套功能接口/状态表。
