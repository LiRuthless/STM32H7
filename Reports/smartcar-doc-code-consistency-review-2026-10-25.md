# SmartCar 文档体系与实际代码一致性审查报告

- 审查日期：2026-10-25
- 仓库分支：`feature/17-app-architecture`
- 审查基线：`7961f2da3e31acd0cc59457606c8d7c2348af06f`
- 审查范围：`SmartCar/` 现行文档体系、App/BSP/Core 源码、CubeMX 配置、GCC/Keil 工程配置、迁移映射及自动验证入口
- 审查性质：只读调查与构建验证；未修改规约、源码、标定或工程配置
- 审查结论：确认 **6 项需要用户决定基准的不一致**，另有 **1 项历史文档导航问题**

## 1. 判定口径

按项目文档自身记录的演进关系解释各阶段：

- Phase 01～07 是存量功能的历史 as-built 规约化追认。
- Phase 08～16 是后续整改与演进增量。
- Phase 17 是当前 App 结构及行为保持契约。

因此，旧路径、旧日志格式、旧验证结果及已经被后续阶段明确替代的策略，不会仅因与当前代码不同就被判为文档漂移。只有现行文档仍保留相反承诺，或文档之间对当前行为给出不可同时成立的约定时，才列为需要决策的不一致。

已知代码缺陷也不自动等于文档漂移。Phase 17 FR-12 要求保持既有数值、错误路径、失败副作用及已知缺陷；只有文档承诺与实际行为冲突时，才在本报告中单独立项。

## 2. 决策总表

| 编号 | 主题 | 文档基准 | 代码基准 | 用户决定 |
|---|---|---|---|---|
| D01 | UART 发送失败返回值 | 返回未发送字节数 | 失败按已发送数量折算 | 待决定 |
| D02 | DL1B 更新方式 | 非阻塞轮询 | TIM6 控制链内阻塞 I²C | 待决定 |
| D03 | 停车风扇 PWM 更新时机 | 停车始终使用当前档空闲值 | 停车后改值/切档只更新状态，不直写 PWM | 待决定 |
| D04 | SPI2 当前速率 | README 写 15 MHz | 默认 1.875 MHz，可显式切换 7.5 MHz | 待决定 |
| D05 | 栈配置同步 | 双工程均由 0x400 增至 0x2000 | 实际工程为 8 KB，但 `.ioc` 仍为 0x400 | 待决定 |
| D06 | HOME 页按键显示值 | README 称“原始值” | 实际显示低通滤波值 | 待决定 |
| N01 | 历史报告源码链接 | 保留历史证据定位 | 改用现行源码导航 | 待决定 |

## 3. 确认不一致

### D01：UART 发送失败时，返回值含义相反

**文档承诺**

Phase 02 FR-5 规定：发送为阻塞式，超时 100 ms，返回未发送字节数，0 表示全部发完。

证据：`SmartCar/specs/features/02-bsp-platform/requirements.md:25`

**当前实现**

`BSP_UART_Write` 在 HAL 成功时返回 0；失败时返回：

```c
return (uint32_t)(len - (uint16_t)huart1.TxXferCount);
```

该计算按已发送数量折算，而不是未发送数量。如果发送开始前失败且 `TxXferCount` 仍为 `len`，错误路径也可能返回 0。

证据：`SmartCar/BSP/bsp_uart.c:38-50`

**影响**

调用方若按 Phase 02 解释返回值，会在错误路径得到相反语义；零值还可能掩盖发送前失败。

**待用户选择**

- 以文档为准：修改返回值实现，并核对所有调用方和测试。
- 以代码为准：修订 Phase 02，明确失败返回值及零值歧义。

### D02：DL1B“非阻塞轮询”与实际阻塞 I²C 冲突

**文档承诺**

Phase 02 将 `BSP_DL1B_Update` 描述为“周期轮询，非阻塞”；FR-17 再次规定“Update 非阻塞轮询”。

证据：

- `SmartCar/specs/features/02-bsp-platform/requirements.md:89`
- `SmartCar/specs/features/02-bsp-platform/requirements.md:92`
- `SmartCar/specs/features/02-bsp-platform/plan.md:31`

**当前实现**

寄存器读写使用阻塞 `HAL_I2C_Mem_Read/Write`，`BSP_DL1B_Update` 在 TIM6 控制链中调用。当前 C/H 注释已经如实写明“阻塞 I²C”，并指出 HAL 时基不推进时不能保证超时退出。

证据：

- `SmartCar/BSP/bsp_dl1b.c:42-62`
- `SmartCar/BSP/bsp_dl1b.c:122-128`
- `SmartCar/BSP/bsp_dl1b.h:21-25`
- `SmartCar/App/system/scheduler.c:172`

**影响**

异常等待可能进入 TIM6 控制链，10 ms 参数不是严格的退出时间上界。本项不扩大解释为“任何传感器故障都必然卡死”。

**待用户选择**

- 以文档为准：重新设计有界采集和错误传播，满足非阻塞契约。
- 以代码为准：删除“非阻塞”承诺，保留阻塞风险及实机验证要求。

### D03：停车后修改风扇参数或切档，物理 PWM 更新时机不一致

**文档承诺**

Phase 08 FR-7 表述为：当前档运行时风扇 PWM 使用该档运行值，停车/安全停车使用该档空闲值。

证据：`SmartCar/specs/features/08-storage-chain-fix/requirements.md:22`

**当前实现**

正常停车路径确实写一次当前空闲 PWM：

```c
BSP_PWM_SetDuty(BSP_PWM_FAN, (uint32_t)Motor_GetState()->fan_duty_idle);
```

但停车后修改当前档空闲值或切档时，`Param_SetFan` / `Param_SelectGear` 只分发状态；`Motor_SetFanDutyIdle` 只更新 `s_motor.fan_duty_idle`，不直接写 PWM。因此物理输出可保持旧值，直到后续运行或停车写出路径。

证据：

- `SmartCar/App/system/safety.c:93-99`
- `SmartCar/App/system/parameter_application.c:61-71`
- `SmartCar/App/system/parameter_application.c:137-158`
- `SmartCar/App/control/motor.c:53-73`
- `SmartCar/App/system/lifecycle.c:75-77`
- `SmartCar/App/system/scheduler.c:177`

Phase 17 又明确要求保持现状，不在参数应用中新增 PWM 直写。

证据：`SmartCar/specs/features/17-app-architecture/g4-parameters-hmi.md:83-86`

**影响**

正常进入停车时的输出符合预期，但“停车状态持续使用当前档空闲值”与“停车后改值/切档不立即应用”不能同时按字面成立。

**待用户选择**

- 以 Phase 08 为准：明确停车提交、切档时的物理输出同步，并覆盖菜单、SCB 等入口。
- 以当前实现/Phase 17 为准：收窄 Phase 08 的承诺，明确只有哪些生命周期节点实际写 PWM。

### D04：README 的 SPI2 当前速率过时

**文档承诺**

README 外设表写为：SPI2 主机 15 MHz，PLL1Q 480 MHz ÷ 32。

证据：`SmartCar/README.md:96`

**当前实现**

`.ioc` 与生成代码均为 `SPI_BAUDRATEPRESCALER_256`，默认源码核算速率为 1.875 MHz。Wi-Fi BSP 仅提供显式切换 64 分频、7.5 MHz 的接口，默认初始化仍保持低速。

证据：

- `SmartCar/SmartCar.ioc:568-569`
- `SmartCar/Core/Src/spi.c:43-51`
- `SmartCar/BSP/bsp_wifi.c:171-186`
- `SmartCar/specs/features/16-spi-wifi-link/requirements.md:34-41`
- `SmartCar/specs/features/16-spi-wifi-link/validation.md:390-393`

**验证边界**

本项只确认配置和源码核算，没有实测 SCK 波形。

**待用户选择**

- 以 README 的 15 MHz 为准：修改时钟配置，同步 Phase 16，并重新进行波形验收。
- 以当前实现/Phase 16 为准：README 改为“默认 1.875 MHz，可显式切换 7.5 MHz”。

### D05：CubeMX 栈元数据未同步到双工程 8 KB 决定

**文档承诺**

Phase 16 要求 GCC 最小栈预留与 Keil 启动栈均由 `0x400` 同步增加至 `0x2000`，验证文档称“两工程配置一致”。

证据：

- `SmartCar/specs/features/16-spi-wifi-link/requirements.md:141-146`
- `SmartCar/specs/features/16-spi-wifi-link/validation.md:63-64`
- `SmartCar/specs/features/16-spi-wifi-link/validation.md:119-124`

**当前实现**

实际 Keil 启动文件和 GCC 链接脚本均为 8 KB：

```text
Stack_Size      EQU     0x2000
_Min_Stack_Size = 0x2000;
```

但 `.ioc` 仍为：

```text
ProjectManager.StackSize=0x400
```

证据：

- `SmartCar/MDK-ARM/startup_stm32h743xx.s:26-35`
- `SmartCar/MDK-GCC/STM32H743VITx.ld:8-10`
- `SmartCar/SmartCar.ioc:439`

**影响**

当前实际编译使用的启动文件和链接脚本已经是 8 KB；问题仅在 CubeMX 元数据及重新生成时的回退风险。不能据此声称当前运行栈只有 1 KB。

**待用户选择**

- 以双工程同步契约为准：将 `.ioc` 同步为 `0x2000`。
- 以当前工程文件为准：明确 `.ioc` 是否负责栈配置，并修订“唯一事实源/两工程一致”及重新生成保护约定。

### D06：README 把 HOME 页按键值称为“原始值”，实际是低通显示值

**文档承诺**

README 两处要求通过 HOME 页 KEY1/KEY2“原始值”核对键盘接线。

证据：

- `SmartCar/README.md:111-112`
- `SmartCar/README.md:124-126`

**当前实现**

按键判定使用未滤波值，但显示状态经过 α=0.7 的低通滤波；HOME 页读取并绘制的是该滤波显示值。

证据：

- `SmartCar/App/hmi/menu_keys.c:39-58`
- `SmartCar/App/hmi/menu_keys.c:102-119`
- `SmartCar/App/hmi/menu_render.c:39-61`

Phase 06 已正确写明：“显示值为低通滤波后值，稳态读数与原始值一致”。

证据：`SmartCar/specs/features/06-hmi-tuning/requirements.md:39-40`

**影响**

主要影响诊断措辞和瞬态读数；稳态接线核对通常不受本质影响。

**待用户选择**

- 以 README 的“原始值”为准：改变显示行为，并同步 Phase 06。
- 以代码/Phase 06 为准：只修订 README 措辞。

## 4. 历史导航问题

### N01：历史研究文档中的 `App/pid.c` 链接失效

历史审查报告在 R01 位置链接 `../../App/pid.c`，该路径已不存在。

证据：`SmartCar/specs/research/2026-10-22-program-review.md:37`

Phase 17 当前映射为：

- 文件级：`App/pid.c` → `App/control/pid.c`
- 函数级：`pid.c::PID_DirectionStep` → `App/control/direction_controller.c::PID_DirectionStep`

证据：

- `SmartCar/specs/features/17-app-architecture/migration-map.md:32`
- `SmartCar/specs/features/17-app-architecture/migration-map.md:392`

这是历史证据定位失效，不是当前算法契约冲突。

**待用户选择**

- 改成历史提交中的文件定位，保持报告的时代语境。
- 改成当前源码位置，并注明这是现行迁移导航。

## 5. 不计为新增不一致的项目

### 5.1 已被后续阶段明确替代的旧策略

Phase 08 FR-4/FR-5 的“写失败继续起跑”等旧策略，已被 Phase 14 的“保存/日志不可用禁止起跑、运行日志故障停车”替代。

Phase 06 关于版本变化不兼容的旧表述，与后续 v1/v2→v3 迁移之间属于已声明的增量覆盖。

### 5.2 已记录并被行为保持契约保留的缺陷

以下项目属于已记录代码缺陷或风险，没有相反现行承诺时不另算文档漂移：

- R01：方向输出窄化后再限幅。
- R02：高优先级中断中的 HAL 超时风险。
- R03：IMU 读取失败后的数据发布问题。
- R04：UART 将一次读缓冲当作完整命令。
- R06：旧 UART `vp/vi` 立即修改活动 PI，而镜像及 SCB `GET_PARAMS.values` 在 flush 前保留旧值。

其中 R06 的“镜像读值/活动读值差异”由 Phase 17 明确要求保持。

### 5.3 尚未验证不等于不一致

以下项目仍是验证门禁或可复现性限制，不计为文档与代码漂移：

- AC5 编译、链接和 map 验证。
- 实车功能、安全停车及风扇验证。
- 灰度 700 μs、完整 TIM6 900 μs 实测。
- SPI SCK 波形和持续吞吐测试。
- 真实运行栈高水位。
- Phase 09/10 待确认设计及 Phase 11 未实施项目。
- R07 完整 `dual_mode_tracking.py` 所依赖的外部原始样本缺失；这是可复现性阻塞，不是灰度代码失败。

## 6. 本次实际验证结果

| 检查 | 实际结果 |
|---|---|
| `python tests/app_layout_check.py` | PASS；95 个 App C/H，45 个 Keil 源各一次，5 个 GCC 分组 |
| `python tests/app_architecture_check.py` | PASS；95 个 C/H，365 个 App 函数定义，结构边界和 Keil 清单通过 |
| `python tests/app_migration_check.py` | PASS；1395 个旧项、1772 个最终项均绑定现有源码 |
| `python tests/test_comment_review_check.py` | 22 项全部通过 |
| `bash MDK-GCC/build.sh` | LCD0 构建成功；0 警告、0 错误；text=91432、data=308、bss=231688 |
| `python tests/scb_protocol.py` | 当前 Windows 环境找不到原生 `gcc`，编译阶段中止，未进入断言 |
| `python tests/scb_link.py` | 当前 Windows 环境找不到原生 `gcc`，编译阶段中止，未进入断言 |
| 受跟踪 Markdown 链接扫描 | 78 份文档、351 个链接目标；本地相对链接仅 N01 失效 |
| Git 工作区复核 | 构建 ELF 改动已恢复；`SmartCar` 无本次审查留下的受跟踪文件改动 |

两个 SCB 宿主测试的结果属于**环境阻塞**，不能记作代码通过，也不能记作协议断言失败。本次没有声称其余宿主入口全部重新通过。

## 7. 用户决策记录

待用户确认后回填：

```text
D01：以 ______ 为准；补充要求：______
D02：以 ______ 为准；补充要求：______
D03：以 ______ 为准；补充要求：______
D04：以 ______ 为准；补充要求：______
D05：以 ______ 为准；补充要求：______
D06：以 ______ 为准；补充要求：______
N01：采用 ______；补充要求：______
```

在用户逐项决定前，不修改任何相关规约或实现。
