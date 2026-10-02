# 技术栈（Tech Stack）

## 语言与运行时

- **C99**（Keil 工程开启 C99 选项；GCC 构建使用 `-std=c99`）
- 目标芯片：**STM32H743VIT6**（Cortex-M7 @ 480 MHz，双精度 FPU，LQFP100）
- 无时钟外 RTOS：裸机 + 三线定时器中断调度（见 `specs/features/01-runtime-architecture/`）

## 框架与核心库

| 类别 | 选型 | 版本 | 理由 |
|---|---|---|---|
| MCU 固件库 | STM32Cube HAL（FW_H7） | **V1.11.2（锁定）** | CubeMX 生成与维护外设初始化 |
| 配置工具 | STM32CubeMX | 生成 MDK-ARM V5.27 目标 | `.ioc` 为外设配置唯一事实源 |
| LCD 组件驱动 | ST MCD ST7735 组件 | 随核心板 SDK 引入 | 厂商组件，只裁剪不修改 |
| 算法出处 | 逐飞 STC AI8051U 库工程 `Sirius20260718` | 省赛版（2026-07-18） | 循迹算法与参数体系来源（仓库外，仅作对照） |

## 数据存储

- **板载 W25Q64 8MB SPI Flash**（SPI1，30 MHz）：参数区（末尾 4 KB 扇区，magic+version+CRC16）+ 日志区（约 8 MB，边写边擦）。
- **片内 Flash：不使用**（参数与日志全部走 W25Q64）。
- ADC DMA 缓冲绝对定位到 D2 SRAM1（`0x30000000`）；默认 RAM = AXI SRAM（`0x24000000`），DTCM 不作 DMA 缓冲（DMA1 不可达）。

## 构建与工具链

| 用途 | 工具 | 备注 |
|---|---|---|
| 主构建 | Keil MDK-ARM，**ARMCC V5.06 update 5（AC5，锁定）** | `MDK-ARM/SmartCar.uvprojx`；代码量约 110 KB，需正式授权（评估版 32 KB 限制） |
| 备用/日常构建 | arm-none-eabi-gcc（EIDE 内置工具链）+ `-O2 -std=c99 -Wall` | `MDK-GCC/build.sh`，VS Code 任务 `Ctrl+Shift+B` |
| 烧录 | OpenOCD + ST-Link | `MDK-GCC/flash.sh` |
| 外设再配置 | CubeMX 重新生成 | 必须保留 `main.c` USER CODE 区补丁（见硬性约束 5） |

## 测试

- 无单元测试框架；验证方式为**编译零警告 + 上机观测清单**（各功能规约的 `validation.md`）。
- 自动化验证基线：`MDK-GCC/build.sh` 构建通过；人工验证按各 Phase 的验证清单执行。

## 硬性约束（不可协商）

1. **分层红线**：App 层只经 `bsp.h` 接触硬件，禁止在 `App/` 内直接包含 HAL/CubeMX 外设头文件。
2. **控制拍标定**：全部控制系数按 **TIM6 = 2 ms** 控制拍标定；改动 TIM6 周期必须重标定全套系数并同步规约。
3. **中断优先级不变量**：TIM15（采样）与 TIM6（控制环）同占 NVIC 优先级 0、互不抢占——这是采样器累计变量免锁、SPI4 总线（IMU 与 LCD）不交错的架构前提，禁止改动该优先级配置。
4. **占空比语义统一**：对外占空比一律 0~10000 万分比，CCR 换算只在 BSP 内发生。
5. **CubeMX 再生保护**：重新生成代码时不得覆盖 `main.c` USER CODE 区的 SPI45SEL→PLL3Q 补丁（`HAL_RCCEx_PeriphCLKConfig` 实测不生效，直接写 `RCC->D2CCIP1R`）。
6. **D-Cache 保持关闭**：ADC DMA 缓冲免 Cache 维护的前提；如需开启必须先完成全量 DMA 缓冲的 Cache 维护设计评审。
7. **版本锁定**：HAL 固件包 V1.11.2、AC5 编译器、EIDE GCC 工具链版本不得擅自升级；升级走 `chore/replan-*` 分支并更新本文件。

## 目录与代码规范

- 分层：`Core/`（CubeMX 生成，用户代码只在 USER CODE 区）→ `BSP/`（收拢全部 HAL 依赖）→ `App/`（控制算法，不碰 HAL）。
- 命名：BSP 接口 `BSP_模块_动作()`；模块文件小写下划线（`bsp_*.c` / App 模块同名 `.c/.h` 对）。
- 注释：中文；移植自源工程的算法在注释中标明出处（如"源 `adc.c`"），保持可追溯。
- 文档：全部纯 Markdown，文件名小写英文 + 连字符；规约与代码同仓库、同流程。
