# Phase 02: BSP 平台层 — 验证

## 2026-10-20 工作区烧录验证

| 编号 | 关联需求 | 操作与通过标准 | 实际结果 |
|---|---|---|---|
| FL-V01 | FL-01/03 | 解析任务 JSON、PowerShell 和 Bash；flash 的默认路径与 Keil 工程输出一致，build/clean 保持 | 通过：JSON/PowerShell/Bash 语法检查及 Keil 输出比对通过，build/clean 未改 |
| FL-V02 | FL-02/05 | 原 ZIP 中全部 OpenOCD 文件逐个 SHA256 对照；运行包内 openocd.exe --version 返回 0 | 已对照 945 文件一致；沙箱外版本 0.12.0-rc2-g7a1adfbec 返回 0 |
| FL-V03 | FL-01/03 | 从仓库外目录用默认参数执行 -CheckOnly，及显式含中文/空格 HEX；返回 0 且计划包含 verify reset exit | 通过：Windows PowerShell 5.1 从 TEMP 默认调用及中文空格 HEX 均退出 0；未访问设备 |
| FL-V04 | FL-02 | 缺失 HEX、非 HEX、含 Tcl 特殊字符路径执行 -CheckOnly 均非零退出，无 OpenOCD 调用 | 通过：三项均退出 1，报错与输入对应，未进入 OpenOCD 调用 |
| FL-V05 | FL-04/05 | 检查兼容入口和现行文档统一使用新入口；Git 暂存不含第三方运行目录及其他在途工作 | 通过：旧 Bash 入口 -CheckOnly 成功转发至 Keil HEX；根/固件 AGENTS 与 README 已同步；运行目录被忽略，仅按本任务路径暂存 |
| FL-V06 | FL-04 | 执行既有 MDK-GCC/build.sh，记录实际构建结果；本次不改固件，不进行实际烧录 | 基线失败：旧脚本指向 /c/Users/Li/.eide/tools/gcc_arm/bin/arm-none-eabi-gcc，本机不存在。未改编译器或构建脚本，恢复构建产生的单个日志差异；不影响独立的 Keil HEX 烧录配置 |

本次不修改自维护 C/H，CS-01～CS-15 不适用；不改变既有阶段实机验收状态。

> as-built 追认规约。计划见 [plan.md](plan.md)，需求见 [requirements.md](requirements.md)。
> 本清单用于确认 BSP 现状行为与规约一致。已知失效项（存储链）如实标注预期，修复后转为必过项。

## 验证清单（Scorecard）

| # | 检查项 | 方法 | 通过标准 | 关联需求 |
|---|---|---|---|---|
| V-1 | 构建基线 | `MDK-GCC/build.sh` | 0 警告 0 错误，产出 elf/hex/bin | 全部 |
| V-2 | 分层红线 | 代码审查：`grep -rn "stm32h7xx_hal\|\"adc.h\"\|\"tim.h\"" App/` | App/ 内无 HAL/CubeMX 外设头引用（仅经 bsp.h） | FR-1, FR-2 |
| V-3 | UART 收发 | 串口助手 115200 发调参命令（如 `vp15`），并观察 printf 输出 | 命令有响应（参数变化/回显），输出无乱码无丢行 | FR-3, FR-5 |
| V-4 | UART 错误恢复 | 串口以错误波特率突发灌数据后恢复 115200 | 接收不永久停摆，后续命令仍响应 | FR-4 |
| V-5 | ADC 十通道 | 停车态 HOME/仪表盘页观察 | 电感×4 随遮挡变化、电池值≈实际电压对应码值（满电约 1308）、按键组原始值随按键变化 | FR-6, FR-7 |
| V-6 | ADC 枚举≡rank 不变量 | 代码审查 bsp_adc.h 与 .ioc rank 表 | 枚举序与 rank 序逐项一致 | FR-6 |
| V-7 | PWM 输出 | 菜单/调参设风扇占空比；起跑空转 | 风扇转速随占空比变化；电机 PWM 频率 17kHz（示波器可选；人耳无尖锐啸叫） | FR-8 |
| V-8 | 占空比语义 | 代码审查 BSP_PWM_SetDuty | 输入 0~10000、超限截断、CCR 换算仅在 BSP 内 | FR-8, FR-9 |
| V-9 | 编码器方向 | 空转车轮观察 `real_speed_L/R`（菜单/CSV） | 前进方向为正；反则翻转 `BSP_ENCODER_*_INVERT` 宏后复测 | FR-10 |
| V-10 | 启动键消抖 | 连续快按/抖动按 K1 | 一次按下只触发一次启动，无误触发 | FR-11 |
| V-11 | 采样器双窗口 | 跑一次导出 CSV | `spdL/spdR`（1ms 窗口）与 tick 连续；2ms 累计由 motor 消费（车速闭环正常即间接证明） | FR-12, FR-13 |
| V-12 | IMU 自检与量程 | 上电串口无 `IMU660RB init fail`；静止时陀螺≈0，水平放置加速度 z≈±1g 量级 | 自检通过；原始值换算符合 ÷14.3 / ÷4098 | FR-14, FR-15 |
| V-13 | DL1B 测距 | 障碍物由远及近，观察菜单/CSV 距离值 | 距离 mm 连续变化；超量程/无目标 = 8192 | FR-16, FR-17 |
| V-14 | LCD 显示 | 上电观察板载屏 | 背光亮（100%）、字符方向正确（ROT180 横屏）、无乱码（乱码=排线或字库写法被改） | FR-18, FR-19, FR-20 |
| V-15 | W25Q64 自检 | 代码审查 +（Phase 08 修复后）上机 | `BSP_W25Q64_Init` JEDEC=0xEF4017 返回 0 | FR-21, FR-22 |
| V-16 | 参数持久化 | 调参后断电重上电 | **现状预期：不生效**（`BSP_Flash_Init` 未调用，已知缺陷）；Phase 08 修复后：参数掉电不丢，改为必过 | FR-23, FR-25 |
| V-17 | 日志边写边擦 | 按 K1 起跑→停车→串口发 `d` | **现状预期：不生效**（同 V-16）；Phase 08 后：CSV 导出 tick 连续、条数≈跑动时长×1000/DATALOG_DIV | FR-24, FR-25 |
| V-18 | 看门狗超时 | 代码审查 RLR=2999/PR=64 + 上机 >10s 无复位 | 超时 ≈6s；正常运行/停车均无误复位 | FR-26 |

## 自动化验证

```bash
cd SmartCar/MDK-GCC && ./build.sh
```

- 通过标准：输出 `*** 完成：0 警告 0 错误 ***`（现状实测 2026-10：0 警告 0 错误，text=59452 data=236 bss=10192）。
- 分层红线快速检查（V-2 自动化形式）：

```bash
grep -rn '#include "stm32h7xx_hal' SmartCar/App/ ; grep -rn '#include "\(adc\|tim\|spi\|i2c\|usart\|gpio\|dma\)\.h"' SmartCar/App/
```

两条命令均应无输出。

## 人工验证步骤

1. **烧录**：`MDK-GCC/flash.sh`。
2. **串口链路**（V-3/V-4）：接 USART1 115200，发 `vp15` 等命令确认调参响应。
3. **采集链路**（V-5/V-12/V-13）：停车态看屏幕——遮挡电感看数值、看电池码值、按 ADC 键盘看原始值（先核对两组按键是否接反）；GYRO 页静止值≈0；DL1B 前移动障碍物看距离变化与 8192 无效值。
4. **执行链路**（V-7/V-9/V-10）：抬起车轮按 K1，2s 后车轮转动；核对 `real_speed_L/R` 前进为正；风扇按键后即转。
5. **显示链路**（V-14）：确认屏幕方向、背光、无乱码。
6. **存储链路**（V-16/V-17）：现状只验证"不生效"这一基准事实（调参重电后回默认；`d` 导出无数据）——作为 Phase 08 修复的对照。

## 回归检查

后续整改不得破坏：

- `MDK-GCC/build.sh` 零警告（V-1）；App/ 不出现 HAL 直接引用（V-2）。
- ADC 通道枚举 ≡ DMA 下标 ≡ .ioc rank 序（V-6）；`.ioc` 变更必须三处同步。
- 占空比对外 0~10000 万分比语义不变（V-8）。
- TIM15/TIM6 同优先级 0（采样器免锁与 SPI4 不交错前提，techstack 约束 3）。
- W25Q64/LCD/IMU 的纯代码或 vendor 组件风格不回退进 .ioc（三种接管风格决策，见 [plan.md](plan.md)）。
- `BSP_LCD_ShowChar` 转置块写不被改回常规行序（V-14）。
- 编码器 `读即清零` 语义不变（采样器双窗口与 motor 累计消费依赖它）。
