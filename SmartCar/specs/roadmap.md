# 路线图（Roadmap）

> 活文档。每完成一个阶段勾选对应复选框；阶段合并、拆分、重排都在这里记录。
> Phase 01–07 为**存量功能的规约化追认**（as-built：代码已存在，规约描述现状并作为后续整改的对照基准）；Phase 08–11 为**整改与演进**（spec-first：先规约后代码）。

## 阶段总览

### Phase 1: 运行时架构与调度 `01-runtime-architecture`
- [x] 应用总装（App_Init/App_Loop）、三线中断契约（TIM15 采样 1ms / TIM6 控制环 2ms / TIM7 辅助 5ms）、480 MHz 时钟树、低压保护与看门狗、启动流程
- 规模：medium
- 功能规约：`specs/features/01-runtime-architecture/`

### Phase 2: BSP 平台层 `02-bsp-platform`
- [x] 全部板级驱动（ADC/PWM/编码器/UART/按键/看门狗/W25Q64/Flash 服务/DL1B/IMU660RB/LCD/采样器）与三种外设接管风格
- 规模：medium
- 功能规约：`specs/features/02-bsp-platform/`

### Phase 3: 电磁循迹控制链 `03-track-following`
- [x] 4 路电感采集与滤波、加权差分归一化偏差、方向非线性 PD、差速分配、速度位置式 PI
- 规模：medium
- 功能规约：`specs/features/03-track-following/`

### Phase 4: 赛道元素状态机 `04-element-fsm`
- [x] kernel 主状态机（7 态）与元素判断的现状规约（当前实际启用：十字 + 右环岛）
- 规模：medium
- 功能规约：`specs/features/04-element-fsm/`

### Phase 5: IMU 姿态处理 `05-imu-attitude`
- [x] IMU660RB 读取、零偏校准、高通滤波、角度梯形积分
- 规模：small
- 功能规约：`specs/features/05-imu-attitude/`

### Phase 6: 人机交互与调参 `06-hmi-tuning`
- [x] LCD 菜单/仪表盘、双路 ADC 分压按键、串口无线调参协议、双档参数持久化（param + CRC16）
- 规模：medium
- 功能规约：`specs/features/06-hmi-tuning/`

### Phase 7: 运行数据记录 `07-datalog`
- [x] 1 ms 采样记录 → RAM 环形缓冲 → W25Q64 边写边擦 → 串口 CSV 导出
- 规模：small
- 功能规约：`specs/features/07-datalog/`

### Phase 8: 存储链修复 `08-storage-chain-fix`
- [ ] 修复 `BSP_Flash_Init()` 从未被调用导致的参数持久化与数据记录整体失效；串口调参结果落盘；`fan_duty` 参数链接通或裁剪（实现时与用户定）
- 规模：small
- 优先级：**最高**（现有功能实际不生效）
- 功能规约：`specs/features/08-storage-chain-fix/`

### Phase 9: 元素功能补齐 `09-element-completion`
- [ ] 逐个接入未启用元素（左环岛 / 跷跷板 / 路障），每个元素一个任务组；**开工前逐个与用户确认设计**
- 规模：medium
- 功能规约：`specs/features/09-element-completion/`

### Phase 10: 调参语义与状态机一致性 `10-control-consistency`
- [ ] 解决"双档参数 vs 状态机每拍覆写"的语义冲突、增益表驱动化、不可达状态处置
- 规模：medium
- 依赖：Phase 8（参数链先可用，语义统一才有意义）；与 Phase 9 同触状态机，须在 9 之后实施
- 功能规约：`specs/features/10-control-consistency/`

### Phase 11: 健壮性与休眠代码清理 `11-hardening-cleanup`
- [ ] W25Q64 等待超时、4 KB 栈缓冲、除零保护、注释漂移修正、高危命名治理、休眠代码处置（联动 `specs/research/2026-10-02-dormant-code-inventory.md`）
- 规模：medium
- 功能规约：`specs/features/11-hardening-cleanup/`

## 当前状态

- 已完成：Phase 01–07（代码层面已落地，2026-10-02 完成规约化追认）
- 进行中：文档体系建立（本文件所属批次）
- 下一步：Phase 8 存储链修复——参数保存与数据记录在现状代码中**实际不生效**，先于一切演进修复
- 阶段门禁：从 2026-10-02 起，任何 Phase/Task Group 必须先完成详细规约并清空未决事项，再进入代码实现；存在不确定项时必须取得用户明确决定，不允许实施者自行选择。

## 已合并/已取消的计划

- **2026-09-03 移植决策（随初始移植落地）**：
  - 源工程 TIM4 速度环 / `pit_speed` 为半废弃死代码，不予移植；
  - 蜂鸣器与 DL1B 引脚冲突且从未启用，不移植，状态指示改用板载蓝灯（PE3）；
  - STC 专有逻辑（0x7F 串口自动下载、寄存器看门狗、`bit` 类型）不移植，看门狗改 IWDG1；
  - 源工程"上电不读 Flash（参数固定默认值）"改为上电即恢复已存参数；
  - 修复源工程 EEPROM 只写 float 前 2 字节的 bug（4 字节完整存取 + CRC16）。
- **2026-10-02 文档体系建立**：按 SDD 规范逆向生成宪章与全量功能规约；已发现缺陷（存储链失效、三态不可达、调参覆写、注释漂移等）不就地修复，全部转入 Phase 08–11 排期。
- **2026-10-02 治理规则强化**：用户要求所有变更严格执行“先文档、后代码”；规约须与文件、符号、行为和验证项一一对应，表述不得模棱两可；遇到任何会影响设计或行为的不确定事项，必须暂停并询问用户，禁止擅自决定。该规则写入 `mission.md`、`techstack.md` 与 `AGENTS.md`，对后续全部 Phase 生效。
