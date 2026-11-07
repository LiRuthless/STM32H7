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
