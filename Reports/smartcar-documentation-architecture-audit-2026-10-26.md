# SmartCar 文档体系与代码一致性审计报告

审计日期：2026-10-26  
代码基线：`7961f2da3e31acd0cc59457606c8d7c2348af06f`

本报告接续所提供会话中的逐文件审计，并对关键差异、未完成部分及机器证据重新核验。第三方库、生成代码和构建目录内的历史副本不纳入自维护功能文档覆盖率。

## 1. 执行摘要

**SmartCar 最大的问题是缺少稳定的“现行功能说明层”。**

代码已经按 system、sensing、control、services、hmi 组织；主要文档仍按开发 Phase 组织。维护者通常需要同时阅读早期功能规约、后续增量规约、架构拆分契约和迁移表，才能还原一个功能现在的完整行为。

现有体系有三项明显优势：

- 主要功能都有需求或设计依据，并非大量代码完全没有文档。
- Phase 17 已提供完整的符号迁移和状态归属索引。
- 最新验收材料明确保留 R07、AC5 和实机待验项，没有把软件通过直接当作整车通过。

本轮实际执行的两项只读检查均通过：

| 检查 | 实际结果 |
|---|---|
| App 架构边界检查 | 95 个 C/H、365 个函数定义、五类目录、私有对象及双 Keil 清单通过 |
| 迁移索引检查 | 1395 个历史项与 1772 个现行项完整对应 |

另核对：

- 迁移索引及验收清单中的 **101 个文件摘要**，统一换行后全部匹配。
- 资源报告中的 **156 个产品源码摘要**，全部匹配当前文件。

因此，不能把问题概括为“Phase 17 没有建立追溯”。主要缺口在于：**机器索引较完整，人阅读功能和执行验收的入口仍不够统一。**

## 2. 代码功能全景

| 功能域 | 当前实际职责 | 主要代码 |
|---|---|---|
| 应用入口与系统协调 | 初始化、起跑门禁、模式切换、前台与中断编排 | [app.c](../SmartCar/App/app.c)、[lifecycle.c](../SmartCar/App/system/lifecycle.c)、[scheduler.c](../SmartCar/App/system/scheduler.c) |
| 安全与电池 | 保护位累计、首因锁存、立即停车、前台收尾、低压持续判断 | [safety.c](../SmartCar/App/system/safety.c)、[battery.c](../SmartCar/App/sensing/battery.c) |
| 传感器处理 | 四电感、16 路灰度、标定、滤波、陀螺校准与积分 | [track_sensor.c](../SmartCar/App/sensing/track_sensor.c)、[gray_sensor.c](../SmartCar/App/sensing/gray_sensor.c)、[imu_proc.c](../SmartCar/App/sensing/imu_proc.c) |
| 循迹与控制 | 两模式观察、选中模式执行、方向计算、差速、速度 PI | [tracking.c](../SmartCar/App/control/tracking.c)、[direction_controller.c](../SmartCar/App/control/direction_controller.c)、[motor.c](../SmartCar/App/control/motor.c) |
| 元素状态机 | 活动电磁十字、预环岛与右环岛；光电普通循迹及失线状态 | [electromagnetic_fsm.c](../SmartCar/App/control/electromagnetic_fsm.c)、[optical_fsm.c](../SmartCar/App/control/optical_fsm.c) |
| 参数与调参 | 两模式两档 bank、v3 镜像、迁移、保存、完整控制拍事务 | [parameter_store.c](../SmartCar/App/services/parameter_store.c)、[tuning.c](../SmartCar/App/control/tuning.c)、[tuning_application.c](../SmartCar/App/system/tuning_application.c) |
| 日志与通信 | 完整拍快照、Flash 队列、CSV/二进制导出、SCB 会话和独立上传 | [run_record.c](../SmartCar/App/services/run_record.c)、[logger.c](../SmartCar/App/services/logger.c)、[scb_link.c](../SmartCar/App/services/scb_link.c) |
| 人机交互 | 页面与编辑、ADC 按键、绘制、板载仪表盘、旧串口调参 | [menu.c](../SmartCar/App/hmi/menu.c)、[menu_keys.c](../SmartCar/App/hmi/menu_keys.c)、[wireless.c](../SmartCar/App/services/wireless.c) |
| BSP 与工程 | 硬件访问、时间与临界区、定时分派、采样、SPI DMA、存储 | [bsp_runtime.c](../SmartCar/BSP/bsp_runtime.c)、[bsp_sampler.c](../SmartCar/BSP/bsp_sampler.c)、[bsp_wifi.c](../SmartCar/BSP/bsp_wifi.c) |

当前运行链有以下重要边界：

1. TIM15 每 1 ms 采样并记录，TIM6 每 2 ms 控制，TIM7 每 5 ms 执行辅助任务。
2. 两种模式均要求有效灰度标定和双传感器条件；当前 32 个标定值仍为 `-1`，两模式均不能起跑。
3. 两套状态机共享同拍输入、保持独立历史，只有选中模式执行控制。
4. Flash 日志写入在前台消费，不在 ISR 内刷写。
5. 紧急停车与阻塞式日志收尾分开执行。
6. 左环岛、跷跷板和路障尚未接入完整活动链，不能因存在函数或枚举就算作已实现能力。

这些边界集中见现有[架构说明](../SmartCar/specs/features/17-app-architecture/architecture.md)。

## 3. 现有文档结构

排除构建副本后，现行主文档集合为 **78 份 Markdown、11124 行**，其中 Phase 文档 69 份。

```text
SmartCar/
├── README.md
├── AGENTS.md
├── CHANGELOG.md
└── specs/
    ├── mission.md
    ├── techstack.md
    ├── roadmap.md
    ├── code-style.md
    ├── research/
    │   ├── 2026-10-02-dormant-code-inventory.md
    │   └── 2026-10-22-program-review.md
    └── features/
        ├── 01～14：各阶段 requirements / plan / validation
        ├── 15：另含符号表、注释审查和 R08 工作流
        ├── 16：另含 SCB 协议
        └── 17：另含架构、分组契约、评审及迁移索引
```

当前组织逻辑是“阶段变更记录为主，功能分类为辅”。

同一个功能存在多份文档本身合理。例如日志需要保留早期实现、v2 格式演进、二进制导出和架构拆分历史。问题在于这些文件承担的角色没有始终清楚区分：

- 当前必须遵守的契约；
- 当时的实现事实；
- 尚未实施的设计；
- 已执行的历史验证；
- 当前仍可执行的验收步骤。

部分文件在页尾增加了当前位置导航，但页首、接口表和验证清单仍使用旧的“当前”“待执行”措辞。

## 4. 代码 ↔ 文档双向映射表

### 4.1 代码到文档

以下用 P01～P17 表示对应 Phase。状态评价针对文档可用性，不表示硬件验收完成。

- **完整**：主要现行行为和边界有明确说明。
- **部分**：有文档，但存在导航、边界、错误描述或验证说明缺口。
- **过时**：仍有作为当前说明或待执行标准呈现的旧内容。
- **未覆盖**：未找到完整的行为契约。
- **待判定**：存在文档解释歧义，不能直接判定实现错误。

| ID | 功能组 | 实现定位 | 对应文档 | 状态与主要差异 |
|---|---|---|---|---|
| F01 | 初始化与三线调度 | scheduler、lifecycle | P01、P14、P16、P17 G2 | 部分：当前链需跨阶段拼接 |
| F02 | 起停、模式切换与首因 | lifecycle、safety | P12、P14、P17 G2/G4 | 部分：首因时序及停车文本存在契约差异 |
| F03 | 低压持续判定 | safety | P01、P12、P17 G2 | 过时：旧页写 200 拍，代码第 201 次触发 |
| F04 | fatal 异常及底层启停失败 | Core 异常处理、BSP runtime | 无完整功能契约 | 未覆盖：已有停留/忽略返回行为，缺少系统级说明 |
| F05 | 电感偏差与对称度 | track_sensor | P03、P14、P17 G3 | 部分：旧共享量与活动 FSM 私有测量易混淆 |
| F06 | 灰度采集、标定与质心 | gray_sensor、bsp_gray | P14、P17 | 完整 |
| F07 | IMU 校准、滤波和积分 | imu_proc、legacy_imu | P05、P14、P17 G3 | 部分：旧 CSV 观察步骤不适用当前字段 |
| F08 | 方向、角度、速度算法 | 三控制器、pid | P03、P13、P17 G3 | 完整；异常窄化风险已有现行记录 |
| F09 | 活动电磁元素 | electromagnetic_fsm | P04、P14、P17 G3 | 部分：旧实车步骤缺少模式和双信号前提 |
| F10 | 光电状态机 | optical_fsm | P14、P17 G3 | 完整 |
| F11 | 双 FSM 与选中执行 | tracking | P14、P17 G3 | 完整 |
| F12 | 部分配置表及调参合成边界 | control_profile | P10、P13、P16、P17 | 部分：已建表与未完成全局语义混排 |
| F13 | legacy 保留与活动边界 | legacy_* | P09、P11、P17、休眠清单 | 完整定位；未来接入/删除仍待专项决定 |
| F14 | 菜单、按键与显示 | hmi 四模块 | P06、P17 G4 | 部分：默认显示配置、滤波值和恢复编辑限制不够清楚 |
| F15 | 菜单、旧串口与风扇参数应用 | wireless、parameter_application、motor | P06、P08、P17 G4 | 部分：活动值、镜像值及停车 PWM 更新时点易混淆 |
| F16 | 参数镜像格式 | parameter_image | P06、P08、P14、P17 | 过时：P06 正文仍称当前 v2，现行为 88B v3 |
| F17 | 迁移、dirty 与保存失败 | parameter_store | P08、P14、P17 G4 | 部分：重启重试保证及 pending 描述不准确 |
| F18 | 完整拍临时调参事务 | tuning、tuning_application | P16、P17 G4 | 完整；不等于 P10 全局整改完成 |
| F19 | 日志头、记录与容量 | datalog、run_record | P07、P14、P17 G5 | 过时：旧验收仍使用旧格式和容量 |
| F20 | RAM 队列、前台写入与故障 | logger、log_store | P14、P16、P17 G5 | 完整 |
| F21 | CSV 与会话可导出范围 | log_export、logger | P07、P14、P16 | 过时：旧页要求 16 列和断电恢复导出 |
| F22 | 二进制流式导出 | log_export、command_dispatch | P16、P17 G5 | 完整 |
| F23 | SCB 帧、JSON 和命令解析 | scb_frame/json/command_codec | P16 协议、P17 G5 | 完整 |
| F24 | 序号回绕与重连责任 | scb_link | P16 协议 | 待判定：接收方是否也须主动处理对端临近回绕 |
| F25 | 独立上传与拥塞隔离 | telemetry、scb_link | P16、P17 G5 | 完整 |
| F26 | ADC、编码器、PWM 基础接口 | 对应 BSP 模块 | P02、后续增量 | 完整的主要映射 |
| F27 | UART 发送失败返回值 | bsp_uart | P02 FR-5 | 部分：文档与实现返回值含义冲突 |
| F28 | DL1B 更新及阻塞边界 | bsp_dl1b、scheduler | P02、P17 | 部分：“非阻塞”承诺与实际 HAL 调用冲突 |
| F29 | 时钟、DMA、采样和 BSP 回调边界 | BSP/Core | P02、P14、P16、P17 | 部分：旧 DMA 定位及唯一回调例外描述过宽 |
| F30 | 双工程、烧录与 CubeMX 再生 | 工程配置和脚本 | README、techstack、P16/17 | 部分：栈配置、SPI2 表格和环境前提需收口 |
| F31 | 自动验证与证据复现 | tests | 各 validation、P17 G6 | 部分：R07 阻塞、组合证据与单入口说明混排 |
| F32 | 写作标准、符号索引与维护门禁 | R08 工具及迁移 JSON | code-style、P15、P17 | 部分：索引完整，但历史授权和当前状态混排 |

### 4.2 文档到代码

| 文档组 | 实际对应 | 一致性判断及建议 |
|---|---|---|
| [P01 运行时](../SmartCar/specs/features/01-runtime-architecture/requirements.md) | system、BSP runtime/time | 历史主干可保留；增加当前起停、保护及通信导航 |
| [P02 BSP](../SmartCar/specs/features/02-bsp-platform/requirements.md) | BSP 驱动 | 修清 UART、DL1B、DMA 和固定回调例外 |
| [P03 循迹](../SmartCar/specs/features/03-track-following/requirements.md) | sensing、控制器、motor | 核心公式可对应；旧执行链及状态说明需分流 |
| [P04 元素](../SmartCar/specs/features/04-element-fsm/requirements.md) | electromagnetic_fsm、legacy_* | 活动/休眠主体区分正确；修验收前提及内部调用描述 |
| [P05 IMU](../SmartCar/specs/features/05-imu-attitude/requirements.md) | imu_proc、legacy_imu | 主要公式一致；调整当前观察方法 |
| [P06 HMI](../SmartCar/specs/features/06-hmi-tuning/requirements.md) | hmi、参数应用、wireless | 基础功能存在；不能独立承担当前全部调参契约 |
| [P07 日志](../SmartCar/specs/features/07-datalog/requirements.md) | 现已拆成记录、队列、存储、导出 | 保留历史格式；旧验证页退出当前验收入口 |
| [P08 存储](../SmartCar/specs/features/08-storage-chain-fix/requirements.md) | parameter_store、Flash | 核心实现存在；两项待上机标准已被后续安全策略替代 |
| [P09 元素补齐](../SmartCar/specs/features/09-element-completion/requirements.md) | 尚无三元素完整活动链 | 明确规划，不算已实现功能缺失；开工前需更新设计 |
| [P10 一致性](../SmartCar/specs/features/10-control-consistency/requirements.md) | 配置表部分已被后续阶段承接 | 全局调参语义仍未定稿，不能按 SCB 已实现宣告完成 |
| [P11 健壮性](../SmartCar/specs/features/11-hardening-cleanup/requirements.md) | BSP、传感器和 legacy | 已做、未做、被后续取代项需要逐 FR 区分 |
| [P12 安全停车](../SmartCar/specs/features/12-control-safety-closure/requirements.md) | safety、lifecycle、motor、logger | 主链存在；首因顺序和精确串口文本需明确演进关系 |
| [P13 控制结构](../SmartCar/specs/features/13-control-structure/requirements.md) | 三控制器、pid、profile | 基本对应；“无参入口”等签名措辞错误 |
| [P14 双模式](../SmartCar/specs/features/14-dual-mode-tracking/requirements.md) | 双传感器、双 FSM、v3 参数、v2 日志 | 主要现行行为依据；接口名和当前复验状态需更新导航 |
| [P15 写作标准](../SmartCar/specs/features/15-code-style-consistency/requirements.md) | 风格、注释、R08 | 历史成果存在；顶部“只发布规约”不能继续充当当前状态 |
| [P16 通信](../SmartCar/specs/features/16-spi-wifi-link/requirements.md) | SCB、SPI2、上传、调参、导出 | 主要契约对应；历史模板、当前入口和结果应分层 |
| [P17 架构](../SmartCar/specs/features/17-app-architecture/requirements.md) | 当前五类 App | 结构及迁移证据完整，是当前定位的重要基础 |

## 5. 问题清单

以下严重程度表示对维护和验收的影响，不表示本轮已经复现对应实车故障。

### 高优先级

| 编号 | 类型 | 证据、影响及建议 |
|---|---|---|
| H01 | 过时、关系断裂 | [P08 验证 V-6/V-9](../SmartCar/specs/features/08-storage-chain-fix/validation.md)仍要求 Flash/日志故障后可以起跑；[当前起跑门禁](../SmartCar/App/system/lifecycle.c)拒绝起跑。应明确替代关系，避免正确固件被判失败。 |
| H02 | 过时、描述错误 | [P07 验证](../SmartCar/specs/features/07-datalog/validation.md)仍要求关闭日志可跑、16 列 CSV、断电后导出旧会话和任意分频。现行为强制日志、57 列、RAM 会话身份及固定分频。旧清单不能用于当前验收。 |
| H03 | 描述错误、实时边界不足 | [P02 FR-17](../SmartCar/specs/features/02-bsp-platform/requirements.md)称 DL1B 非阻塞；[实际更新](../SmartCar/BSP/bsp_dl1b.c)在 TIM6 内调用阻塞 HAL I²C，超时退出还依赖时基。需先明确契约，实测时间仍待验证。 |
| H04 | 配置不一致、覆盖不足 | [IOC 栈配置](../SmartCar/SmartCar.ioc)仍为 `0x400`，实际 Keil 启动文件为 `0x2000`。再生成保护只强调时钟补丁，存在恢复小栈的风险。后续配置修正须走规约先行。 |
| H05 | 过时、重复文档 | [P06 参数表](../SmartCar/specs/features/06-hmi-tuning/requirements.md)仍称当前 v2；[实际镜像](../SmartCar/App/services/parameter_image.h)为 88B v3。离线工具若照旧表实现，会使用错误格式。 |
| H06 | 关系断裂 | [程序审查报告](../SmartCar/specs/research/2026-10-22-program-review.md)保留 R01～R07，但路线图没有完整索引 R01～R06。应建立已知问题入口，区分保留缺陷、待设计和已关闭项。 |

### 中优先级

| 编号 | 类型 | 证据、影响及建议 |
|---|---|---|
| M01 | 描述错误 | [UART FR-5](../SmartCar/specs/features/02-bsp-platform/requirements.md)约定返回未发送字节；[实现](../SmartCar/BSP/bsp_uart.c)失败返回 `len-TxXferCount`，含义相反。需单独决定保留哪项契约，不能只改措辞掩盖差异。 |
| M02 | 描述错误 | [README SPI2 表](../SmartCar/README.md)写 15 MHz；当前启动配置为 1.875 MHz，可显式切换 7.5 MHz。配置事实可确定，实际 SCK 波形仍待测。 |
| M03 | 覆盖不足、契约冲突 | [P12 首因顺序](../SmartCar/specs/features/12-control-safety-closure/requirements.md)要求先锁首因；[当前代码](../SmartCar/App/system/safety.c)先累计保护位，再进入首因临界区。跨优先级窗口中，最先进入请求者未必最先锁存。需明确诊断语义。 |
| M04 | 描述错误 | [P12 停车文本](../SmartCar/specs/features/12-control-safety-closure/requirements.md)规定精确字符串；[当前输出](../SmartCar/App/system/safety.c)改为包含原因、保护位和日志计数的摘要。应声明替代及兼容边界。 |
| M05 | 描述错误 | [P08 计划](../SmartCar/specs/features/08-storage-chain-fix/plan.md)暗示恢复后重启可重试迁移；实际擦写失败可能已破坏旧镜像。只能保证本次 RAM 保留，不能保证下次仍可迁移。 |
| M06 | 过时、无法直接验收 | [P05 V-7/V-8](../SmartCar/specs/features/05-imu-attitude/validation.md)要求通过 CSV 看 `angle_x` 和 `accel_x`；[当前记录结构](../SmartCar/App/services/datalog.h)不含这两个字段。应提供调试观察方法或重新设计验证，不应直接扩展日志格式。 |
| M07 | 过时、关系断裂 | [技术栈测试章节](../SmartCar/specs/techstack.md)仍主要描述编译和上机观测，遗漏真实 C 宿主测试、工具测试及 R07 阻塞。 |
| M08 | 结构错位、状态混杂 | roadmap、标准及部分 requirements 顶部保留“待启动”“本轮仅文档”，后文又记录完成。应将最新状态置顶，历史授权按日期保留。 |

### 低优先级及待判定项

- **当前位置导航不均匀**：很多 requirements/plan 已有 Phase 17 链接，validation 没有同等入口；历史验证中直接搜索旧符号会失效。
- **陈旧注释**：[Flash 擦除函数](../SmartCar/BSP/bsp_flash.c)的块内文字仍称全区块擦除，实际只擦首个扇区。
- **路径漂移**：部分引脚、码源引用仍指向旧位置，实际资料已在仓库 `Material/` 下。
- **协议歧义**：[回绕约定](../SmartCar/specs/features/16-spi-wifi-link/protocol.md)未清楚区分发送端与接收端责任。当前不能据此确认 STM32 缺少实现。
- **缺失文档**：fatal 异常、`Error_Handler`、底层定时器启停失败的系统行为缺少完整契约；不能把现有原地等待推断成已经设计并验证的安全策略。

没有发现应当直接归为“已完成能力却完全没有实现”的左环岛、跷跷板或路障条目。它们属于明确保留的规划。

## 6. 文档覆盖情况

### 6.1 本报告功能组统计

统计对象就是第 4.1 节列出的 **32 个功能组**，每组只计一次。它衡量文档可用性，不是代码行、分支或实机覆盖率。

| 分类 | 数量 | 比例 |
|---|---:|---:|
| 完整覆盖 | 11 | 34.4% |
| 部分覆盖或存在错误/导航缺口 | 15 | 46.9% |
| 存在影响当前使用的过时条目 | 4 | 12.5% |
| 未找到完整行为契约 | 1 | 3.1% |
| 契约解释待判定 | 1 | 3.1% |
| 合计 | 32 | 100% |

**31/32 个功能组能找到相关文档，不代表 31/32 已准确完整说明。**

### 6.2 可直接核实的索引和证据指标

| 指标 | 结果 | 解释 |
|---|---:|---|
| 当前 App 文件 | 45 C + 50 H | 与架构检查一致 |
| 当前 App 函数定义 | 365 | 不等于 365 个业务功能 |
| 历史迁移项 | 1395/1395 对应 | 本轮检查通过 |
| 现行索引项 | 1772/1772 登记 | owner、context、validation 均非空 |
| 索引源码摘要 | 101/101 匹配 | 按工具约定统一换行 |
| 资源源码摘要 | 156/156 匹配 | 本轮逐文件核对 |
| 已有 Phase 17 执行记录 | 15 项通过、1 项 R07 阻塞 | 属已有记录，本轮未重跑全部宿主测试 |

现有测试确实编译并执行真实 C，也包含历史对照、故障注入、字节比较和结构检查。不过：

- BSP 替身及受控注入不能替代真实中断、电气和 Flash 时序。
- 历史等价证明的是所测试输入下的行为保持，不能证明历史行为本身正确。
- `owner/context/validation` 非空只能证明登记完整，不能证明文字语义全部正确。
- 4176/8192B 是含预留的栈规划，不是实测峰值或形式化上界。
- R07 原始样本、AC5、700/900μs、波形、吞吐和实车仍待验证。

接续过程中纠正了两项原审计误判：

1. 右环岛使用带 `LEFT` 的历史阶段名；真实主态是 `CONTROL_KERNEL_ISLAND_R`，不能按枚举名字判成左环岛测试。
2. 双模式夹具包含起跑后立即注入灰度、电感和采集异常的等待期用例，不能笼统说等待期失线没有测试。

因此，本报告没有沿用原结果中依赖这些判断计算的测试完整度百分比。

## 7. 根因分析

1. **功能说明与变更记录使用同一套载体。**  
   Phase 很适合回答“这一轮改了什么”，不适合独立回答“这个功能现在如何工作”。

2. **增量说明持续追加，旧正文没有一致标明适用时代。**  
   新信息常在页尾，旧“当前接口”和待执行标准仍在前面。

3. **符号追溯与功能追溯没有完全衔接。**  
   Phase 17 能定位函数和对象，但完整业务规则仍需跨 Phase 拼接。

4. **验证步骤、执行记录和证据限制混排。**  
   计划模板、历史成功、当前阻塞和实机待验共处一页，容易误读。

5. **同一事实重复维护。**  
   参数布局、日志格式、阶段进度和构建条件在多个位置展开，已经出现冲突。

6. **已知问题没有统一状态入口。**  
   研究报告中的开放问题没有完整进入路线图，重构完成容易被误解为问题已解决。

## 8. 推荐的新文档体系

保留现有 SDD 阶段体系，增加按功能阅读的现行层。以下是建议结构，未创建任何目录或文件。

```text
SmartCar/
├── README.md                     # 简明入口和当前限制
├── AGENTS.md                     # 协作规则
├── CHANGELOG.md                  # 变更历史
├── specs/                       # 保留现有治理与阶段规约
│   ├── mission.md
│   ├── techstack.md
│   ├── roadmap.md
│   ├── code-style.md
│   ├── features/01～17/
│   └── research/
└── docs/
    ├── index.md                 # 按目的选择阅读入口
    ├── architecture.md          # 系统关系、状态归属、并发
    ├── traceability.yaml        # 功能—需求—实现—验证索引
    ├── known-issues.md
    ├── system/
    │   └── runtime-and-safety.md
    ├── sensing/
    │   └── sensors-and-calibration.md
    ├── control/
    │   ├── tracking-and-controllers.md
    │   └── elements-and-dormant-code.md
    ├── services/
    │   ├── parameters-and-tuning.md
    │   ├── recording-and-export.md
    │   └── communication.md
    ├── hmi/
    │   └── operation.md
    ├── platform/
    │   └── board-and-bsp.md
    └── guides/
        ├── build-and-flash.md
        └── validation.md
```

组织规则：

- 一个文档覆盖一组紧密协作的功能，不机械对应每个 C/H。
- 现行说明描述用途、输入输出、状态、时序、失败和兼容边界。
- Phase 保留需求决策、实施顺序和历史验收。
- 协议、参数布局、日志格式各有唯一详细权威来源；其他文档引用。
- 初期继续引用现有 P16 协议等权威文件，避免先复制一套。
- 代码引用以路径和符号为主，行号由工具辅助生成。
- 需求、实现、验证和硬件状态分别记录，不能只用一个“完成”字段。

建议功能索引至少包含：

| 字段 | 用途 |
|---|---|
| 功能 ID 与名称 | 稳定标识业务行为 |
| 当前权威文档 | 唯一阅读入口 |
| 需求来源 | Phase 与 FR |
| 实现 | 路径、符号、状态 owner |
| 验证 | 入口、用例、直接证明范围 |
| 证据 | 日期、源码摘要、结果 |
| 状态 | 实现、宿主、硬件、阻塞分别记录 |
| 替代关系 | 哪项旧契约被何时替代 |

## 9. 迁移与整改计划

| 顺序 | 动作 | 影响范围与完成标准 |
|---|---|---|
| 1 | 更新当前入口 | README、roadmap 顶部给出唯一现状摘要、标定限制、R07 和硬件待验 |
| 2 | 分流危险旧验收 | P07/P08 旧标准明确标为历史，链接当前起跑及日志验收 |
| 3 | 建立功能索引 | 先覆盖本报告 32 组，关联现有 FR、源码和验证，不重抄 1772 项 |
| 4 | 补写现行功能说明 | 优先运行安全、参数调参、日志导出，再补控制、传感器、HMI、BSP |
| 5 | 建立统一验证入口 | 区分静态检查、真实 C 宿主、ARM 构建、AC5 和实机；记录 R07 依赖 |
| 6 | 收口明确事实错误 | SPI2 表格、当前类型/接口名、低压拍数、失效路径 |
| 7 | 单独处理契约决策 | UART 返回值、DL1B 阻塞策略、首因定义、停车风扇应用时点等 |
| 8 | 后续实施工程修正 | 例如 IOC 栈同步；先独立提交规约，再修改配置 |
| 9 | 精简重复说明 | 新现行文档稳定后，合并重复正文，历史证据保留原基线 |

现有文档的具体处置建议：

- **保留**：使命、长期技术约束、代码标准、Phase 需求决策、原始验证证据、迁移 JSON。
- **补充**：当前功能入口、fatal 行为、构建环境、验证复现方法和已知问题索引。
- **更新**：当前状态、明确事实错误、当前符号导航。
- **拆分**：validation 中的当前执行方法与历史执行记录。
- **合并**：重复的现行参数、日志、操作和构建说明。
- **归档**：旧资源数值、旧格式说明和已被替代的验收步骤，保留提交号。
- **移动**：初期不批量移动；后续确需迁移时保留旧入口跳转。
- **删除**：本阶段不建议删除历史证据。只在稳定替代和链接迁移完成后，考虑删除纯重复的现行正文。

长期维护应采用以下门禁：

1. 功能变更先提交对应 Phase 规约，继续遵守现有规则。
2. 实现变更同时更新其现行功能文档和索引。
3. 自动检查路径、符号、需求 ID、验证入口、格式常量和源码摘要。
4. 测试结果必须在失败时也输出当前失败状态，避免旧成功文件继续被当作最新结果。
5. 人工检查边界、失败路径和状态语义；自动化不能替代内容审查。
6. 代码重构保持已知缺陷时，在问题索引中明确保留状态。
7. 不把硬件待验项目通过文档整理或宿主回归勾选完成。

## 10. 最终结论

**当前最需要调整的是阅读和维护方式：让功能说明成为稳定入口，让 Phase 继续承担变更与验收历史。**

问题最集中的区域是：

- 日志与存储的旧验收步骤；
- HMI、参数格式和多通道调参说明；
- 运行安全契约在多个阶段之间的演进关系；
- 构建、CubeMX 再生成及当前测试状态入口。

不需要推倒现有规约，也不需要为了文档整齐再次重构代码。

最低成本的做法是：**先修正容易误用的入口和验收标准，再建立功能索引，最后逐域汇总现行说明。** 已有 Phase 17 迁移清单可以直接作为基础，无须重建另一套逐符号大表。

本轮完成的是审计与方案设计。既有 PCB 改动、未跟踪报告和构建产物均未由本轮修改。
