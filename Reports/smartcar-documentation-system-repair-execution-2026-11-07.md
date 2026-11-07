# 文档体系修复执行记录

## G0 基线与保护边界

开始分支 feature/02-isolated-board-tests；HEAD `8d5ab2faf68432ae03b2c1f91bc4593c7dc7cc3e` 与调查基线一致，无后续已提交差异。工作区仅两个未跟踪方案。本方案纳入执行依据；sensor-isolated-board-test-plan-2026-11-06.md 只读、不暂存。治理分支 chore/constitution-phase-docs-alignment。

保护：所有非本次明确 Markdown 文档以 Git 相对基线最终差异比较；源码、工程、测试、证据JSON、日志、构建产物、硬件不修改。未跟踪板测方案另核对摘要。

## 逐文件恢复登记

| 文件 | 68cb508正文 / 10月31日操作 | 后续增量 / 恢复位置与锚点 |
|---|---|---|
| `SmartCar/specs/features/01-runtime-architecture/plan.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/01-runtime-architecture/requirements.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/01-runtime-architecture/validation.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/02-bsp-platform/plan.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保护11月FR27～30、任务、验收和人工记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/02-bsp-platform/requirements.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保护11月FR27～30、任务、验收和人工记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/02-bsp-platform/validation.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保护11月FR27～30、任务、验收和人工记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/03-track-following/plan.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/03-track-following/requirements.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/03-track-following/validation.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/04-element-fsm/plan.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/04-element-fsm/requirements.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/04-element-fsm/validation.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/05-imu-attitude/plan.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/05-imu-attitude/requirements.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/05-imu-attitude/validation.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/06-hmi-tuning/plan.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/06-hmi-tuning/requirements.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/06-hmi-tuning/validation.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/07-datalog/plan.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/07-datalog/requirements.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/07-datalog/validation.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/08-storage-chain-fix/plan.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/08-storage-chain-fix/requirements.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/08-storage-chain-fix/validation.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/09-element-completion/plan.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/09-element-completion/requirements.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/09-element-completion/validation.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/10-control-consistency/plan.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/10-control-consistency/requirements.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/10-control-consistency/validation.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/11-hardening-cleanup/plan.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/11-hardening-cleanup/requirements.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/11-hardening-cleanup/validation.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/12-control-safety-closure/plan.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/12-control-safety-closure/requirements.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/12-control-safety-closure/validation.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/13-control-structure/plan.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/13-control-structure/requirements.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/13-control-structure/validation.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/14-dual-mode-tracking/plan.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/14-dual-mode-tracking/requirements.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/14-dual-mode-tracking/validation.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/15-code-style-consistency/comment-review-workflow.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/15-code-style-consistency/comment-review.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/15-code-style-consistency/plan.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/15-code-style-consistency/r08-review-contract.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/15-code-style-consistency/requirements.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/15-code-style-consistency/symbol-map.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/15-code-style-consistency/validation.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/16-spi-wifi-link/plan.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/16-spi-wifi-link/protocol.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/16-spi-wifi-link/requirements.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/16-spi-wifi-link/validation.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/17-app-architecture/architecture.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/17-app-architecture/g2-review.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/17-app-architecture/g2-system.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/17-app-architecture/g3-control.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/17-app-architecture/g3-review.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/17-app-architecture/g4-parameters-hmi.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/17-app-architecture/g4-review.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/17-app-architecture/g5-recording-protocol.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/17-app-architecture/g5-review.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/17-app-architecture/g6-acceptance.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/17-app-architecture/g6-review.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/17-app-architecture/plan.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/17-app-architecture/requirements.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |
| `SmartCar/specs/features/17-app-architecture/validation.md` | 完整阶段正文；迁移时分流/折叠，协议为跳转替代 | 保留当前导航及后续日期记录；原文件正文直接展开，现存显式锚点保留 |

## 分类迁移登记

#### 4.6.3 具体拆分与保留清单

下表路径均相对于`SmartCar/Docs/`，是后续实施的明确落点，不表示这些新页已经存在。旧文件继续保留为功能总览及旧锚点兼容入口；详细实现只在新主题页维护。未列出的现有主题页保持路径并补全正文，不为追求目录对称而拆分。

| 当前入口/功能 | 目标主题页或子条目 | 具体分工 |
|---|---|---|
| `platform/board-and-bsp.md` F26 | `platform/adc.md#f26-adc`、`platform/encoders.md#f26-encoder`、`platform/pwm-and-direction.md#f26-pwm` | ADC通道/rank/DMA/新转换；编码器读清/方向；PWM频率/CCR/方向电平分别完整说明 |
| 同页F27 | `platform/uart.md#f27` | 独立说明收发、缓冲、错误重武装、真实返回语义及调用限制 |
| 同页F28 | `platform/dl1b.md#f28` | 独立说明初始化、轮询状态、阻塞调用/时基、无效值和失败传播 |
| 同页F29 | `platform/clocks-and-dma.md#f29-clock-dma`、`platform/sampler.md#f29-sampler` | 时钟/存储区域/中断资源；TIM15窗口、采样钩子和数据发布分别展开 |
| 同页F29器件驱动 | `platform/imu660rb.md#f29-imu`、`platform/w25q64-and-flash.md#f29-flash`、`platform/lcd.md#f29-lcd`、`platform/spi-wifi.md#f29-wifi` | 展开器件级初始化、原码/存储/显示/传输接口、共享资源和错误路径；F29保留资源与驱动总览 |
| `sensing/sensors-and-calibration.md` F05/F06/F07 | `sensing/electromagnetic.md#f05`、`sensing/grayscale.md#f06`、`sensing/attitude.md#f07` | 分开电感、灰度和姿态业务算法；共享滤波在原总览内设稳定子锚点`#filters`，由相关页引用 |
| `control/tracking-and-controllers.md` F08 | `control/controllers.md#f08`，内部`#f08-direction`、`#f08-angle`、`#f08-speed`；`control/motor-execution.md#f08-motor` | 三种控制器用统一算法/状态/参数模板分章；差速、反馈、里程与执行协调独立成页；暂不为每个控制器建立一篇短页 |
| 同页F10/F11 | `control/tracking-modes.md#f10`及`#f11` | 光电FSM与双模式观察/唯一执行保留在同一完整协调主题中，分别有状态和时序说明 |
| 同页F12 | `control/profiles.md#f12` | 只说明当前部分配置机制和完整值表；未实现全局调参目标回链Phase10，不把“规划”当作现行分类 |
| `control/elements-and-dormant-code.md` F09/F13 | 保留原页；细分`#f09-cross`、`#f09-roundabout`和`#f13` | 十字、右环分别有转移/参数/调用说明；休眠清单与活动链清楚隔开，不以页数决定是否启用 |
| `services/parameters-and-tuning.md` F15～18 | `services/parameters.md#f15/#f16/#f17`；`services/runtime-tuning.md#f18` | 参数应用/镜像/迁移保存作为持久参数主题；完整拍临时覆盖事务单独说明；旧页保留两者关系图和兼容导航 |
| `services/recording-and-export.md` F19～22 | 保留原页，按格式、采样/队列/写入、CSV、二进制导出分章 | 它们共享日志会话及故障收尾，可在完整生命周期页内阅读；若后续新增独立导出协议再按拆分条件评估 |
| `services/communication.md` F23～25 | 保留原页，强化帧/命令、会话链路、上传三章和实际调用图 | 物理SPI及DMA细节移到platform主题；当前STM32协议仍可在一页完整查阅 |
| `platform/isolated-board-tests.md` F33 | 保留原页；增加选择总表和`#f33-lcd/#f33-adc/#f33-gray/#f33-imu/#f33-dl1b/#f33-encoder/#f33-battery/#f33-keys`子锚点 | F33是“独立板测”这一完整运行场景，设备多不等于分类错误；公共隔离/快照/异常规则一处维护，各对象分别展开 |
| `system/runtime-and-safety.md` F01～04、`hmi/operation.md` F14、`guides/` F30～32 | 保留原页并补全分章 | 先将完整启动/运行/停车链、操作链和工程操作写清，不同时进行无必要的全目录重命名 |

上表中的`#f15/#f16/#f17`等记法表示同一文件内多个独立锚点，不是包含斜线的单个URL。实施时索引须分别生成准确链接。系统电池处理等跨类源码按实际数据与状态职责映射，不要求Docs物理目录与C文件目录完全同构。

#### 4.6.4 功能身份、主归属与旧链接兼容

1. 保留F01～F33身份；F26、F29这类宽功能保留父级总览，细节用上述稳定子锚点表达。拆页本身不新增F34，也不重编号旧功能。
2. F父条目负责功能范围、子功能和协作总览；子主题负责自己的算法、状态、参数、错误及源码定位。父页引用子页，不复制成另一套完整说明。
3. `traceability.md`增加“父功能/具体子条目/主说明/源码主归属”字段。一个FR指向其实际涉及的子条目，确实跨域时列多个，不把父级总览当作所有需求的唯一终点。
4. `architecture.md`逐模块登记主要职责及协作条目；每个关键符号/状态/参数确定一处详细说明归属。跨页出现只作必要上下文及链接，不重复维护值表。
5. 旧文件保留现存显式锚点和已有被引用的章节入口，注明详细内容的新位置。主要导航切换到新页；旧链接仍能到达明确的对应内容，不仅跳到新的总首页。
6. Phase历史正文和原始证据中的路径不批量改写；现行导航、可复制模板及必要承接说明更新到新主条目，保留当时基线含义。
7. 文件移动不改变F身份；以后新增实际功能才建立新的F身份。一个新Phase可以修改多个旧F，一个F也可由多个Phase演进，不建立Phase号与F号一一对应关系。


## 执行状态

G0已建立基线、逐文件恢复与分类清单。G1～G5结果按实际推进回填；不预先宣称验收通过。D05按本组Docs先行规则执行，本次仅纠正文档无实现同步目标。

## G1～G4实际交付与提交

| 组 | 提交 | 实际交付 |
|---|---|---|
| G0/G1 | eb489a1 | 固定方案/恢复分类清单，纠正治理、SDD正文/模板/流程/提示词与维护契约 |
| G2 | 6d93801 | 62份阶段正文与逐文件审计；保留11月增量及协议13兼容锚点 |
| G3样板 | 7f49ede | 控制器/电机/完整配置矩阵/双模式主题，元素展开及F33八子项 |
| G3感知/系统/HMI/指南 | 0b56db9 | 感知三链拆分、算法参数、起停及完整拍、操作与R08接口校核 |
| G3服务 | 25f08b8 | 持久参数/临时调参拆分、四bank默认覆盖、日志格式生命周期及SCB机制 |
| G3平台 | 7146550 | 11个指定主题+旧父入口，ADC/编码器/PWM、资源/驱动及错误路径 |
| G4 | 70ce96d | 能力概览、架构、参数、需求/父子追溯、已知问题及三份旧迁移规则替代说明 |

无源码、配置或测试提交；未推送。恢复清单中的原文保持方法、摘要和协议锚点逐项结果见[恢复审计](smartcar-phase-restoration-audit-2026-11-07.md)。未恢复项：无；当前Docs不覆盖历史决定。

## G5：V01～V11实际验收

验收日期2026-11-07；方法为Git正文比较、一次性只读链接/锚点/模块枚举及逐域源码语义审查。没有执行固件构建、软件行为回归、R08实现检查、烧录或实机验收，没有新增永久检查器。下列“通过”只指本次文档验收。

| 验收 | 结果 | 实际检查与结论 |
|---|---|---|
| V01 前层约束 | 通过 | 根/固件AGENTS、mission/techstack/roadmap/code-style、SDD正文和全部可复制提示词逐项核对；完整Phase和D05顺序一致，10月31日验证分类仍有效 |
| V02 Phase完整性 | 通过 | 主agent独立Git比较62份原文完整前缀（协议去新锚点后比较）；61原折叠正文逐字与68cb508一致，Phase02三后缀全文保持；13协议锚点准确插章，未改JSON/人工表/原始结果 |
| V03 Docs→代码 | 通过 | 全F01～33按功能逐域核对算法/顺序、状态owner、并发、错误及参数；控制样板和服务域另由独立agent复核，主agent核验修正；无遗留实现同步中条目（维护模板中的示例除外） |
| V04 代码→Docs | 通过 | 枚举App、自维护BSP和Core共156个C/H（ST7735/font按第三方排除），architecture零未归属；正文核对主要函数/宏/表/条件路径，Core生成配置与USER CODE边界明确。计数只验证映射，语义结论依下方审查范围 |
| V05 参数完整性 | 通过 | 全局25类参数索引指具体主表；核对采集、滤波、控制/配置、保护、镜像CRC/迁移、日志布局容量、SCB/DMA/队列、HMI和板测；默认/恢复/活动/查询/Flash区分，物理未测项不编造 |
| V06 来源闭环 | 通过 | F01～33保持；阶段FR/TC导航、现行父子表、源码与参数反查互通；Phase08→14存储策略显式承接，fatal现状无来源不虚构批准 |
| V07 状态真实性 | 通过 | 旧复选框、11月板测软件证据及AC5/R07/标定/栈/时序/吞吐/实车待验保持；桥接README明确生产链/最终验证未完成，纠正“无桥接”过期说法 |
| V08 文件/链接 | 通过 | 最终正文检查49份现行/治理/执行文档1815处本地路径和锚点零缺失；基线Docs全部显式锚点零丢失；diff --check通过。阶段历史源码路径按原基线解释，不批量改造历史 |
| V09 范围保持 | 通过，工作区有外部增量 | 本任务累计提交全部为明确Markdown路径；源码/工程/测试/JSON/日志/硬件无本任务提交。进行中出现board_test_config.h外部新增两空行，保留且不暂存；因此不声称整个工作区源码与初始基线逐字相同。另一个未跟踪板测方案始终排除 |
| V10 分类粒度 | 通过 | 七领域保持；11平台新页、3感知新页、4控制新页、2服务新页落实4.6；F26/F29父子、原码/业务算法/安全编排、日志/通信/物理SPI、持久参数/临时事务主归属明确 |
| V11 导航兼容 | 通过 | 旧board f26～29、控制f08/f10/f11/f12、感知f05～07、参数f15～18保持并精确指向新主题；F33八对象子项、参数和源码反查可双向遍历；无F编号重排 |

### 语义审查范围与已修正发现

- system：main正常/板测分流，lifecycle初始化/重入起跑/切模式、scheduler三线/complete失败出口、safety首因/累计位/前台收尾、battery滤波与201拍低压。主agent对照实际调用顺序及失败副作用。
- sensing：track_sensor八读去极值/整数偏差、gray标定归一化/质心及失败保留、imu_proc与legacy_imu/filter状态及换算/积分/reset；原码机制分别由platform说明。
- control：三控制器、pid、motor、profiles七主态四子态、tracking/optical/electromagnetic全部Select/Calculate/Execute与转移，legacy调用/复位及未接入边界；样板由主agent对照，复核不以历史测试替代。
- services：parameter_store/bank/image/application、wireless、tuning/values/application、logger/run_record/log_store/log_export、scb_frame/json/codec/link/dispatch/checksum/telemetry；独立复核默认、CRC、容量、回绕、禁用/会话区别、解析/接管顺序。
- hmi/guides：menu/keys/render/dashboard及BSP按键、build/flash/linker/工程和R08检查器接口。显示值与bank/活动/镜像区分；纯文档未执行这些工具。
- platform：所有自维护驱动函数体及Core ADC/TIM/SPI/DMA/GPIO/I2C/UART、main时钟/IRQ；核对NDTR新转换、原码/CCR整数顺序、共享SPI、时基依赖、HAL忽略/失败返回和完整发布。

修正了电感权重误连持久参数、gray percent类型、R08新增文件cleanup、旧调查基线、HMI显示与bank关系、验证表模板残留、SCB解析/接管顺序和运行禁用恢复时点。复核中“APP_WDG_ENABLE未消费”的初步意见仅查App漏看BSP；主agent读取bsp_wdt.c确认条件编译后撤销，最终说明与BSP一致。没有把调查意见直接当成事实。

## 十二个读者场景走查

| 场景 | 实际路径与复述结论 | 结果 |
|---|---|---|
| 1 治理到三线 | mission/techstack→P01/P02→system调度表/architecture图→main/runtime/sampler；裸机、静态分配、1/2/5ms、TIM15/TIM6同级0 | 通过 |
| 2 P14双保护 | P14 D03～06/FR→grayscale参数→Lifecycle_TryStart；32项全-1，任一路无效使两模式门禁失败；有效后仍需双信号 | 通过 |
| 3 F08速度PI | controllers#f08-speed→speed_controller；严格异号清积分，保存目标，误差/累加，±7000，目标非0补500，±7500，float→int32→int16 | 通过 |
| 4 配置矩阵 | control_profile.c→architecture→profiles；7主态4子态W/B/T/A、沿用字段、主态后子态和临时覆盖均可查 | 通过 |
| 5 KP/KI层次 | parameters全局→服务主表→store/PID；默认20/.75、Flash恢复、活动值与旧UART尚未flush镜像差异保持R06 | 通过 |
| 6 存储策略演进 | P08 FR4/5允许继续→P14存储/日志门禁→services参数承接及system当前起跑；两阶段正文未互相覆盖 | 通过 |
| 7 板测选择 | F33选择0～15→对象子项→最小BSP/IRQ/快照/发送；NONE正常、其他隔离，真实板测未执行 | 通过 |
| 8 协议到实现 | P16完整快照→communication公共格式及STM32适配→frame/link/codec/dispatch；32B头/CRC明确，D08及TCP断开责任边界保留 | 通过 |
| 9 R08来源 | P15完整契约/专项→guides#r08-schema→检查器；人工内容、四check、摘要失效/依赖/cleanup与历史22项不同 | 通过 |
| 10 F26拆分 | board旧f26→ADC rank表/编码器读清/PWM CCR三页→同源码，反向architecture也到同一主说明 | 通过 |
| 11 灰度跨域 | platform ADC#gray-raw新转换完整帧→sensing标定/质心→system失线/采集停车；公式/阈值与原码机制各有主归属 | 通过 |
| 12 旧入口及IMU | board#f29进入具体器件/资源；tracking-and-controllers#f08进入三控制器/执行；F33#f33-imu到JustFloat队列/参数，与正常姿态积分分开 | 通过 |

## 保留、未知与本次完成边界

Phase15四专项及R08阶段批准依据、Phase16完整协议、Phase17六设计主体均已恢复；Phase02 FR27～30、五组任务、BT-V01～05、board-test-review人工表、F33及11月6日原始证据完整保留。没有取消AC5、R07原LQ样本、真实标定、栈峰值、700/900us、SPI波形/吞吐或真实板测待验。

未知物理值继续登记于known-issues及所属主表：灰度左右/极性/串扰，编码器方向/每转计数/物理距离，电池ADC→V阈值，元素门限实测依据。R01/D01/R02/R03/R04/R05/R06/D07/D08/D09等行为争议仍按原台账待决定；本次没有提出或批准新的固件策略，文档纠偏不以解决它们为前提。后续改变行为须逐项提供事实、选项、影响并由用户决定。

范围例外：board_test_config.h在执行中被其他来源增加两处空行，本任务未写入、未还原、未暂存；最终工作区仍有此改动和既有未跟踪板测方案。板测方案交付时SHA256为c9b0934ed4cc28d0b693725e17ef66452d5e55c72a8add8dac731d50d50fa5c3，仅记录交付状态，未冒称开始时已经计算摘要。

G0～G5文档修复与V01～V11文档验收已执行；固件功能/硬件验收没有在本次完成。最后验收回填以单独纯文档提交保存，其提交号从本文件Git历史查询，避免自引用提交号。
