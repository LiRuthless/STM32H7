# Phase → docs 功能追溯

Phase保留完整背景、需求、设计、用户决定、文件/符号映射、任务和验收，当前详细契约先进入下表docs主条目，再由功能页定位源码。下表只维护第一段映射，不复制函数/字段清单。模块反查见[architecture](architecture.md)。
现行说明核对代码基线8d5ab2f；历史阶段正文按各自基线解释；存在实现不等于实机完成。P09/P10目标仍未定或未实施，FR/TC标识保持原文（前导0不改变编号身份）。

## 33功能主条目

| ID | 功能 | 唯一主说明 |
|---|---|---|
| F01 | 初始化与三线调度 | [system/runtime-and-safety.md#f01](system/runtime-and-safety.md#f01) |
| F02 | 起停、模式与首因 | [system/runtime-and-safety.md#f02](system/runtime-and-safety.md#f02) |
| F03 | 低压持续判定 | [system/runtime-and-safety.md#f03](system/runtime-and-safety.md#f03) |
| F04 | fatal与底层启停失败 | [system/runtime-and-safety.md#f04](system/runtime-and-safety.md#f04) |
| F05 | 电感偏差与对称度 | [sensing/electromagnetic.md#f05](sensing/electromagnetic.md#f05) |
| F06 | 灰度采集标定与质心 | [sensing/grayscale.md#f06](sensing/grayscale.md#f06) |
| F07 | IMU校准滤波与积分 | [sensing/attitude.md#f07](sensing/attitude.md#f07) |
| F08 | 方向角度速度与执行 | [control/controllers.md#f08](control/controllers.md#f08) |
| F09 | 活动电磁元素 | [control/elements-and-dormant-code.md#f09](control/elements-and-dormant-code.md#f09) |
| F10 | 光电状态机 | [control/tracking-modes.md#f10](control/tracking-modes.md#f10) |
| F11 | 双FSM与唯一执行 | [control/tracking-modes.md#f11](control/tracking-modes.md#f11) |
| F12 | 部分配置与全局调参规划 | [control/profiles.md#f12](control/profiles.md#f12) |
| F13 | legacy与未接入元素 | [control/elements-and-dormant-code.md#f13](control/elements-and-dormant-code.md#f13) |
| F14 | 菜单按键与显示 | [hmi/operation.md#f14](hmi/operation.md#f14) |
| F15 | 多入口参数应用 | [services/parameters.md#f15](services/parameters.md#f15) |
| F16 | 参数v3格式 | [services/parameters.md#f16](services/parameters.md#f16) |
| F17 | 迁移保存与失败 | [services/parameters.md#f17](services/parameters.md#f17) |
| F18 | 完整拍临时调参 | [services/runtime-tuning.md#f18](services/runtime-tuning.md#f18) |
| F19 | 日志格式容量 | [services/recording-and-export.md#f19](services/recording-and-export.md#f19) |
| F20 | 队列前台写入与故障 | [services/recording-and-export.md#f20](services/recording-and-export.md#f20) |
| F21 | CSV与会话边界 | [services/recording-and-export.md#f21](services/recording-and-export.md#f21) |
| F22 | 二进制流式导出 | [services/recording-and-export.md#f22](services/recording-and-export.md#f22) |
| F23 | SCB帧JSON与命令 | [services/communication.md#f23](services/communication.md#f23) |
| F24 | 会话可靠传输与回绕 | [services/communication.md#f24](services/communication.md#f24) |
| F25 | 独立上传 | [services/communication.md#f25](services/communication.md#f25) |
| F26 | ADC编码器PWM | [platform/board-and-bsp.md#f26](platform/board-and-bsp.md#f26) |
| F27 | UART失败返回 | [platform/uart.md#f27](platform/uart.md#f27) |
| F28 | DL1B阻塞边界 | [platform/dl1b.md#f28](platform/dl1b.md#f28) |
| F29 | 时钟DMA采样与驱动 | [platform/board-and-bsp.md#f29](platform/board-and-bsp.md#f29) |
| F30 | 构建烧录与再生 | [guides/build-and-flash.md#f30](guides/build-and-flash.md#f30) |
| F31 | 验证证据与复现 | [guides/validation.md#f31](guides/validation.md#f31) |
| F32 | 标准R08与维护 | [guides/validation.md#f32](guides/validation.md#f32) |
| F33 | 传感器独立板测 | [platform/isolated-board-tests.md#f33](platform/isolated-board-tests.md#f33) |

## 逐阶段需求与约束

### P01

[阶段需求](../specs/features/01-runtime-architecture/requirements.md)。主条目唯一，跨域协作同时列出所需功能；规划/待决定不计已实现，硬件状态仍见阶段原记录。

| 需求/约束 | docs主条目及协作条目 | 状态边界 |
|---|---|---|
| [P01 FR-1](../specs/features/01-runtime-architecture/requirements.md#fr-01) | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 具体实现、演进/冲突与限制见docs |
| [P01 FR-2](../specs/features/01-runtime-architecture/requirements.md#fr-02) | [F29 时钟DMA采样与驱动](platform/clocks-and-dma.md#f29-clock-dma) | 具体实现、演进/冲突与限制见docs |
| [P01 FR-3](../specs/features/01-runtime-architecture/requirements.md#fr-03) | [F07 IMU校准滤波与积分](sensing/attitude.md#f07) | 具体实现、演进/冲突与限制见docs |
| [P01 FR-4](../specs/features/01-runtime-architecture/requirements.md#fr-04) | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 具体实现、演进/冲突与限制见docs |
| [P01 FR-5](../specs/features/01-runtime-architecture/requirements.md#fr-05) | [F01 初始化与三线调度](system/runtime-and-safety.md#f01)；[F14 菜单按键与显示](hmi/operation.md#f14)；[F15 多入口参数应用](services/parameters.md#f15)；[F02 起停、模式与首因](system/runtime-and-safety.md#f02) | 具体实现、演进/冲突与限制见docs |
| [P01 FR-6](../specs/features/01-runtime-architecture/requirements.md#fr-06) | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 具体实现、演进/冲突与限制见docs |
| [P01 FR-7](../specs/features/01-runtime-architecture/requirements.md#fr-07) | [F02 起停、模式与首因](system/runtime-and-safety.md#f02) | 具体实现、演进/冲突与限制见docs |
| [P01 FR-8](../specs/features/01-runtime-architecture/requirements.md#fr-08) | [F20 队列前台写入与故障](services/recording-and-export.md#f20)；[F29 时钟DMA采样与驱动](platform/w25q64-and-flash.md#f29-flash)；[F19 日志格式容量](services/recording-and-export.md#f19) | 具体实现、演进/冲突与限制见docs |
| [P01 FR-9](../specs/features/01-runtime-architecture/requirements.md#fr-09) | [F01 初始化与三线调度](system/runtime-and-safety.md#f01)；[F09 活动电磁元素](control/elements-and-dormant-code.md#f09)；[F08 方向角度速度与执行](control/controllers.md#f08)；[F11 双FSM与唯一执行](control/tracking-modes.md#f11) | 具体实现、演进/冲突与限制见docs |
| [P01 FR-10](../specs/features/01-runtime-architecture/requirements.md#fr-10) | [F03 低压持续判定](system/runtime-and-safety.md#f03) | 具体实现、演进/冲突与限制见docs |
| [P01 FR-11](../specs/features/01-runtime-architecture/requirements.md#fr-11) | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 具体实现、演进/冲突与限制见docs |
| [P01 FR-12](../specs/features/01-runtime-architecture/requirements.md#fr-12) | [F03 低压持续判定](system/runtime-and-safety.md#f03) | 具体实现、演进/冲突与限制见docs |
| [P01 FR-13](../specs/features/01-runtime-architecture/requirements.md#fr-13) | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 具体实现、演进/冲突与限制见docs |
| [P01 FR-14](../specs/features/01-runtime-architecture/requirements.md#fr-14) | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 具体实现、演进/冲突与限制见docs |
| [P01 TC-1](../specs/features/01-runtime-architecture/requirements.md#tc-01) | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 具体实现、演进/冲突与限制见docs |
| [P01 TC-2](../specs/features/01-runtime-architecture/requirements.md#tc-02) | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 具体实现、演进/冲突与限制见docs |
| [P01 TC-3](../specs/features/01-runtime-architecture/requirements.md#tc-03) | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 具体实现、演进/冲突与限制见docs |
| [P01 TC-4](../specs/features/01-runtime-architecture/requirements.md#tc-04) | [F29 时钟DMA采样与驱动](platform/clocks-and-dma.md#f29-clock-dma) | 具体实现、演进/冲突与限制见docs |
| [P01 TC-5](../specs/features/01-runtime-architecture/requirements.md#tc-05) | [F29 时钟DMA采样与驱动](platform/clocks-and-dma.md#f29-clock-dma) | 具体实现、演进/冲突与限制见docs |
| [P01 TC-6](../specs/features/01-runtime-architecture/requirements.md#tc-06) | [F20 队列前台写入与故障](services/recording-and-export.md#f20) | 具体实现、演进/冲突与限制见docs |
| [P01 TC-7](../specs/features/01-runtime-architecture/requirements.md#tc-07) | [F29 时钟DMA采样与驱动](platform/clocks-and-dma.md#f29-clock-dma) | 具体实现、演进/冲突与限制见docs |

### P02

[阶段需求](../specs/features/02-bsp-platform/requirements.md)。主条目唯一，跨域协作同时列出所需功能；规划/待决定不计已实现，硬件状态仍见阶段原记录。

| 需求/约束 | docs主条目及协作条目 | 状态边界 |
|---|---|---|
| [P02 FR-1](../specs/features/02-bsp-platform/requirements.md#fr-01) | [F29 时钟DMA采样与驱动](platform/clocks-and-dma.md#f29-clock-dma) | 具体实现、演进/冲突与限制见docs |
| [P02 FR-2](../specs/features/02-bsp-platform/requirements.md#fr-02) | [F29 时钟DMA采样与驱动](platform/clocks-and-dma.md#f29-clock-dma) | 具体实现、演进/冲突与限制见docs |
| [P02 FR-3](../specs/features/02-bsp-platform/requirements.md#fr-03) | [F27 UART失败返回](platform/uart.md#f27) | 具体实现、演进/冲突与限制见docs |
| [P02 FR-4](../specs/features/02-bsp-platform/requirements.md#fr-04) | [F27 UART失败返回](platform/uart.md#f27) | 具体实现、演进/冲突与限制见docs |
| [P02 FR-5](../specs/features/02-bsp-platform/requirements.md#fr-05) | [F27 UART失败返回](platform/uart.md#f27) | 具体实现、演进/冲突与限制见docs |
| [P02 FR-6](../specs/features/02-bsp-platform/requirements.md#fr-06) | [具体机制](platform/adc.md#f26-adc) | 具体实现、演进/冲突与限制见docs |
| [P02 FR-7](../specs/features/02-bsp-platform/requirements.md#fr-07) | [具体机制](platform/adc.md#f26-adc) | 具体实现、演进/冲突与限制见docs |
| [P02 TC-1](../specs/features/02-bsp-platform/requirements.md#tc-01) | [F29 时钟DMA采样与驱动](platform/clocks-and-dma.md#f29-clock-dma) | 具体实现、演进/冲突与限制见docs |
| [P02 FR-8](../specs/features/02-bsp-platform/requirements.md#fr-08) | [具体机制](platform/pwm-and-direction.md#f26-pwm) | 具体实现、演进/冲突与限制见docs |
| [P02 FR-9](../specs/features/02-bsp-platform/requirements.md#fr-09) | [具体机制](platform/pwm-and-direction.md#f26-pwm) | 具体实现、演进/冲突与限制见docs |
| [P02 FR-10](../specs/features/02-bsp-platform/requirements.md#fr-10) | [具体机制](platform/encoders.md#f26-encoder) | 具体实现、演进/冲突与限制见docs |
| [P02 FR-11](../specs/features/02-bsp-platform/requirements.md#fr-11) | [F02 起停、模式与首因](system/runtime-and-safety.md#f02) | 具体实现、演进/冲突与限制见docs |
| [P02 FR-12](../specs/features/02-bsp-platform/requirements.md#fr-12) | [具体机制](platform/sampler.md#f29-sampler) | 具体实现、演进/冲突与限制见docs |
| [P02 FR-13](../specs/features/02-bsp-platform/requirements.md#fr-13) | [具体机制](platform/sampler.md#f29-sampler) | 具体实现、演进/冲突与限制见docs |
| [P02 TC-2](../specs/features/02-bsp-platform/requirements.md#tc-02) | [F29 时钟DMA采样与驱动](platform/clocks-and-dma.md#f29-clock-dma) | 具体实现、演进/冲突与限制见docs |
| [P02 TC-3](../specs/features/02-bsp-platform/requirements.md#tc-03) | [F29 时钟DMA采样与驱动](platform/clocks-and-dma.md#f29-clock-dma) | 具体实现、演进/冲突与限制见docs |
| [P02 FR-14](../specs/features/02-bsp-platform/requirements.md#fr-14) | [具体机制](platform/imu660rb.md#f29-imu) | 具体实现、演进/冲突与限制见docs |
| [P02 FR-15](../specs/features/02-bsp-platform/requirements.md#fr-15) | [具体机制](sensing/attitude.md#f07) | 具体实现、演进/冲突与限制见docs |
| [P02 FR-16](../specs/features/02-bsp-platform/requirements.md#fr-16) | [F28 DL1B阻塞边界](platform/dl1b.md#f28) | 具体实现、演进/冲突与限制见docs |
| [P02 FR-17](../specs/features/02-bsp-platform/requirements.md#fr-17) | [F28 DL1B阻塞边界](platform/dl1b.md#f28) | 具体实现、演进/冲突与限制见docs |
| [P02 FR-18](../specs/features/02-bsp-platform/requirements.md#fr-18) | [具体机制](platform/lcd.md#f29-lcd) | 具体实现、演进/冲突与限制见docs |
| [P02 FR-19](../specs/features/02-bsp-platform/requirements.md#fr-19) | [具体机制](platform/lcd.md#f29-lcd) | 具体实现、演进/冲突与限制见docs |
| [P02 FR-20](../specs/features/02-bsp-platform/requirements.md#fr-20) | [具体机制](platform/lcd.md#f29-lcd) | 具体实现、演进/冲突与限制见docs |
| [P02 FR-21](../specs/features/02-bsp-platform/requirements.md#fr-21) | [具体机制](platform/w25q64-and-flash.md#f29-flash) | 具体实现、演进/冲突与限制见docs |
| [P02 FR-22](../specs/features/02-bsp-platform/requirements.md#fr-22) | [具体机制](platform/w25q64-and-flash.md#f29-flash) | 具体实现、演进/冲突与限制见docs |
| [P02 FR-23](../specs/features/02-bsp-platform/requirements.md#fr-23) | [具体机制](platform/w25q64-and-flash.md#f29-flash) | 具体实现、演进/冲突与限制见docs |
| [P02 FR-24](../specs/features/02-bsp-platform/requirements.md#fr-24) | [F20 队列前台写入与故障](services/recording-and-export.md#f20) | 具体实现、演进/冲突与限制见docs |
| [P02 FR-25](../specs/features/02-bsp-platform/requirements.md#fr-25) | [具体机制](platform/w25q64-and-flash.md#f29-flash) | 具体实现、演进/冲突与限制见docs |
| [P02 FR-26](../specs/features/02-bsp-platform/requirements.md#fr-26) | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 具体实现、演进/冲突与限制见docs |
| [P02 TC-4](../specs/features/02-bsp-platform/requirements.md#tc-04) | [F29 时钟DMA采样与驱动](platform/clocks-and-dma.md#f29-clock-dma) | 具体实现、演进/冲突与限制见docs |
| [P02 TC-5](../specs/features/02-bsp-platform/requirements.md#tc-05) | [F30 构建烧录与再生](guides/build-and-flash.md#f30) | 具体实现、演进/冲突与限制见docs |
| [P02 TC-6](../specs/features/02-bsp-platform/requirements.md#tc-06) | [F32 标准R08与维护](guides/validation.md#f32) | 具体实现、演进/冲突与限制见docs |
| [P02 TC-7](../specs/features/02-bsp-platform/requirements.md#tc-07) | [F29 时钟DMA采样与驱动](platform/clocks-and-dma.md#f29-clock-dma) | 具体实现、演进/冲突与限制见docs |

### P03

[阶段需求](../specs/features/03-track-following/requirements.md)。主条目唯一，跨域协作同时列出所需功能；规划/待决定不计已实现，硬件状态仍见阶段原记录。

| 需求/约束 | docs主条目及协作条目 | 状态边界 |
|---|---|---|
| [P03 FR-1](../specs/features/03-track-following/requirements.md#fr-01) | [F05 电感偏差与对称度](sensing/electromagnetic.md#f05) | 具体实现、演进/冲突与限制见docs |
| [P03 FR-2](../specs/features/03-track-following/requirements.md#fr-02) | [F05 电感偏差与对称度](sensing/electromagnetic.md#f05) | 具体实现、演进/冲突与限制见docs |
| [P03 FR-3](../specs/features/03-track-following/requirements.md#fr-03) | [F05 电感偏差与对称度](sensing/electromagnetic.md#f05) | 具体实现、演进/冲突与限制见docs |
| [P03 FR-4](../specs/features/03-track-following/requirements.md#fr-04) | [F08 方向角度速度与执行](control/controllers.md#f08) | 具体实现、演进/冲突与限制见docs |
| [P03 FR-5](../specs/features/03-track-following/requirements.md#fr-05) | [F08 方向角度速度与执行](control/controllers.md#f08) | 具体实现、演进/冲突与限制见docs |
| [P03 FR-6](../specs/features/03-track-following/requirements.md#fr-06) | [F08 方向角度速度与执行](control/controllers.md#f08) | 具体实现、演进/冲突与限制见docs |
| [P03 FR-7](../specs/features/03-track-following/requirements.md#fr-07) | [F26 ADC编码器PWM](platform/encoders.md#f26-encoder) | 具体实现、演进/冲突与限制见docs |
| [P03 FR-8](../specs/features/03-track-following/requirements.md#fr-08) | [F08 方向角度速度与执行](control/controllers.md#f08) | 具体实现、演进/冲突与限制见docs |
| [P03 FR-9](../specs/features/03-track-following/requirements.md#fr-09) | [F02 起停、模式与首因](system/runtime-and-safety.md#f02) | 具体实现、演进/冲突与限制见docs |
| [P03 FR-10](../specs/features/03-track-following/requirements.md#fr-10) | [F05 电感偏差与对称度](sensing/electromagnetic.md#f05)；[F07 IMU校准滤波与积分](sensing/attitude.md#f07) | 具体实现、演进/冲突与限制见docs |
| [P03 TC-1](../specs/features/03-track-following/requirements.md#tc-01) | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 具体实现、演进/冲突与限制见docs |
| [P03 TC-2](../specs/features/03-track-following/requirements.md#tc-02) | [F29 时钟DMA采样与驱动](platform/clocks-and-dma.md#f29-clock-dma) | 具体实现、演进/冲突与限制见docs |
| [P03 TC-3](../specs/features/03-track-following/requirements.md#tc-03) | [F26 ADC编码器PWM](platform/encoders.md#f26-encoder) | 具体实现、演进/冲突与限制见docs |
| [P03 TC-4](../specs/features/03-track-following/requirements.md#tc-04) | [F29 时钟DMA采样与驱动](platform/clocks-and-dma.md#f29-clock-dma) | 具体实现、演进/冲突与限制见docs |
| [P03 TC-5](../specs/features/03-track-following/requirements.md#tc-05) | [F08 方向角度速度与执行](control/controllers.md#f08) | 具体实现、演进/冲突与限制见docs |
| [P03 TC-6](../specs/features/03-track-following/requirements.md#tc-06) | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 具体实现、演进/冲突与限制见docs |
| [P03 TC-7](../specs/features/03-track-following/requirements.md#tc-07) | [F08 方向角度速度与执行](control/controllers.md#f08) | 具体实现、演进/冲突与限制见docs |

### P04

[阶段需求](../specs/features/04-element-fsm/requirements.md)。主条目唯一，跨域协作同时列出所需功能；规划/待决定不计已实现，硬件状态仍见阶段原记录。

| 需求/约束 | docs主条目及协作条目 | 状态边界 |
|---|---|---|
| [P04 FR-1](../specs/features/04-element-fsm/requirements.md#fr-01) | [F09 活动电磁元素](control/elements-and-dormant-code.md#f09)；[F12 部分配置与全局调参规划](control/profiles.md#f12)；[F11 双FSM与唯一执行](control/tracking-modes.md#f11) | 具体实现、演进/冲突与限制见docs |
| [P04 FR-2](../specs/features/04-element-fsm/requirements.md#fr-02) | [F02 起停、模式与首因](system/runtime-and-safety.md#f02) | 具体实现、演进/冲突与限制见docs |
| [P04 FR-3](../specs/features/04-element-fsm/requirements.md#fr-03) | [F09 活动电磁元素](control/elements-and-dormant-code.md#f09) | 具体实现、演进/冲突与限制见docs |
| [P04 FR-4](../specs/features/04-element-fsm/requirements.md#fr-04) | [F09 活动电磁元素](control/elements-and-dormant-code.md#f09) | 具体实现、演进/冲突与限制见docs |
| [P04 FR-5](../specs/features/04-element-fsm/requirements.md#fr-05) | [F09 活动电磁元素](control/elements-and-dormant-code.md#f09) | 具体实现、演进/冲突与限制见docs |
| [P04 FR-6](../specs/features/04-element-fsm/requirements.md#fr-06) | [F09 活动电磁元素](control/elements-and-dormant-code.md#f09) | 具体实现、演进/冲突与限制见docs |
| [P04 FR-7](../specs/features/04-element-fsm/requirements.md#fr-07) | [F09 活动电磁元素](control/elements-and-dormant-code.md#f09)；[F12 部分配置与全局调参规划](control/profiles.md#f12) | 具体实现、演进/冲突与限制见docs |
| [P04 FR-8](../specs/features/04-element-fsm/requirements.md#fr-08) | [F09 活动电磁元素](control/elements-and-dormant-code.md#f09) | 具体实现、演进/冲突与限制见docs |
| [P04 FR-9](../specs/features/04-element-fsm/requirements.md#fr-09) | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 具体实现、演进/冲突与限制见docs |
| [P04 FR-10](../specs/features/04-element-fsm/requirements.md#fr-10) | [F09 活动电磁元素](control/elements-and-dormant-code.md#f09) | 具体实现、演进/冲突与限制见docs |
| [P04 FR-11](../specs/features/04-element-fsm/requirements.md#fr-11) | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 具体实现、演进/冲突与限制见docs |
| [P04 TC-1](../specs/features/04-element-fsm/requirements.md#tc-01) | [F09 活动电磁元素](control/elements-and-dormant-code.md#f09) | 具体实现、演进/冲突与限制见docs |
| [P04 TC-2](../specs/features/04-element-fsm/requirements.md#tc-02) | [F09 活动电磁元素](control/elements-and-dormant-code.md#f09) | 具体实现、演进/冲突与限制见docs |
| [P04 TC-3](../specs/features/04-element-fsm/requirements.md#tc-03) | [F09 活动电磁元素](control/elements-and-dormant-code.md#f09) | 具体实现、演进/冲突与限制见docs |
| [P04 TC-4](../specs/features/04-element-fsm/requirements.md#tc-04) | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 具体实现、演进/冲突与限制见docs |
| [P04 TC-5](../specs/features/04-element-fsm/requirements.md#tc-05) | [F08 方向角度速度与执行](control/controllers.md#f08) | 具体实现、演进/冲突与限制见docs |

### P05

[阶段需求](../specs/features/05-imu-attitude/requirements.md)。主条目唯一，跨域协作同时列出所需功能；规划/待决定不计已实现，硬件状态仍见阶段原记录。

| 需求/约束 | docs主条目及协作条目 | 状态边界 |
|---|---|---|
| [P05 FR-1](../specs/features/05-imu-attitude/requirements.md#fr-01) | [F07 IMU校准滤波与积分](sensing/attitude.md#f07) | 具体实现、演进/冲突与限制见docs |
| [P05 FR-2](../specs/features/05-imu-attitude/requirements.md#fr-02) | [F07 IMU校准滤波与积分](sensing/attitude.md#f07) | 具体实现、演进/冲突与限制见docs |
| [P05 FR-3](../specs/features/05-imu-attitude/requirements.md#fr-03) | [F07 IMU校准滤波与积分](sensing/attitude.md#f07) | 具体实现、演进/冲突与限制见docs |
| [P05 FR-4](../specs/features/05-imu-attitude/requirements.md#fr-04) | [F07 IMU校准滤波与积分](sensing/attitude.md#f07) | 具体实现、演进/冲突与限制见docs |
| [P05 FR-5](../specs/features/05-imu-attitude/requirements.md#fr-05) | [F07 IMU校准滤波与积分](sensing/attitude.md#f07) | 具体实现、演进/冲突与限制见docs |
| [P05 FR-6](../specs/features/05-imu-attitude/requirements.md#fr-06) | [F07 IMU校准滤波与积分](sensing/attitude.md#f07) | 具体实现、演进/冲突与限制见docs |
| [P05 FR-7](../specs/features/05-imu-attitude/requirements.md#fr-07) | [F07 IMU校准滤波与积分](sensing/attitude.md#f07) | 具体实现、演进/冲突与限制见docs |
| [P05 FR-8](../specs/features/05-imu-attitude/requirements.md#fr-08) | [F07 IMU校准滤波与积分](sensing/attitude.md#f07) | 具体实现、演进/冲突与限制见docs |
| [P05 TC-1](../specs/features/05-imu-attitude/requirements.md#tc-01) | [F07 IMU校准滤波与积分](sensing/attitude.md#f07) | 具体实现、演进/冲突与限制见docs |
| [P05 TC-2](../specs/features/05-imu-attitude/requirements.md#tc-02) | [F29 时钟DMA采样与驱动](platform/imu660rb.md#f29-imu) | 具体实现、演进/冲突与限制见docs |
| [P05 TC-3](../specs/features/05-imu-attitude/requirements.md#tc-03) | [F29 时钟DMA采样与驱动](platform/imu660rb.md#f29-imu) | 具体实现、演进/冲突与限制见docs |
| [P05 TC-4](../specs/features/05-imu-attitude/requirements.md#tc-04) | [F29 时钟DMA采样与驱动](platform/imu660rb.md#f29-imu) | 具体实现、演进/冲突与限制见docs |
| [P05 TC-5](../specs/features/05-imu-attitude/requirements.md#tc-05) | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 具体实现、演进/冲突与限制见docs |

### P06

[阶段需求](../specs/features/06-hmi-tuning/requirements.md)。主条目唯一，跨域协作同时列出所需功能；规划/待决定不计已实现，硬件状态仍见阶段原记录。

| 需求/约束 | docs主条目及协作条目 | 状态边界 |
|---|---|---|
| [P06 FR-1](../specs/features/06-hmi-tuning/requirements.md#fr-01) | [F14 菜单按键与显示](hmi/operation.md#f14) | 具体实现、演进/冲突与限制见docs |
| [P06 FR-2](../specs/features/06-hmi-tuning/requirements.md#fr-02) | [F14 菜单按键与显示](hmi/operation.md#f14) | 具体实现、演进/冲突与限制见docs |
| [P06 FR-3](../specs/features/06-hmi-tuning/requirements.md#fr-03) | [F14 菜单按键与显示](hmi/operation.md#f14) | 具体实现、演进/冲突与限制见docs |
| [P06 FR-4](../specs/features/06-hmi-tuning/requirements.md#fr-04) | [F14 菜单按键与显示](hmi/operation.md#f14) | 具体实现、演进/冲突与限制见docs |
| [P06 FR-5](../specs/features/06-hmi-tuning/requirements.md#fr-05) | [F15 多入口参数应用](services/parameters.md#f15)；[F14 菜单按键与显示](hmi/operation.md#f14) | 具体实现、演进/冲突与限制见docs |
| [P06 FR-6](../specs/features/06-hmi-tuning/requirements.md#fr-06) | [F14 菜单按键与显示](hmi/operation.md#f14) | 具体实现、演进/冲突与限制见docs |
| [P06 FR-7](../specs/features/06-hmi-tuning/requirements.md#fr-07) | [F14 菜单按键与显示](hmi/operation.md#f14) | 具体实现、演进/冲突与限制见docs |
| [P06 FR-8](../specs/features/06-hmi-tuning/requirements.md#fr-08) | [F14 菜单按键与显示](hmi/operation.md#f14) | 具体实现、演进/冲突与限制见docs |
| [P06 FR-9](../specs/features/06-hmi-tuning/requirements.md#fr-09) | [F02 起停、模式与首因](system/runtime-and-safety.md#f02) | 具体实现、演进/冲突与限制见docs |
| [P06 FR-10](../specs/features/06-hmi-tuning/requirements.md#fr-10) | [F14 菜单按键与显示](hmi/operation.md#f14) | 具体实现、演进/冲突与限制见docs |
| [P06 FR-11](../specs/features/06-hmi-tuning/requirements.md#fr-11) | [F17 迁移保存与失败](services/parameters.md#f17) | 具体实现、演进/冲突与限制见docs |
| [P06 FR-12](../specs/features/06-hmi-tuning/requirements.md#fr-12) | [F16 参数v3格式](services/parameters.md#f16) | 具体实现、演进/冲突与限制见docs |
| [P06 FR-13](../specs/features/06-hmi-tuning/requirements.md#fr-13) | [F17 迁移保存与失败](services/parameters.md#f17) | 具体实现、演进/冲突与限制见docs |
| [P06 FR-14](../specs/features/06-hmi-tuning/requirements.md#fr-14) | [F17 迁移保存与失败](services/parameters.md#f17) | 具体实现、演进/冲突与限制见docs |
| [P06 FR-15](../specs/features/06-hmi-tuning/requirements.md#fr-15) | [F15 多入口参数应用](services/parameters.md#f15) | 具体实现、演进/冲突与限制见docs |
| [P06 FR-16](../specs/features/06-hmi-tuning/requirements.md#fr-16) | [F15 多入口参数应用](services/parameters.md#f15) | 具体实现、演进/冲突与限制见docs |
| [P06 FR-17](../specs/features/06-hmi-tuning/requirements.md#fr-17) | [F15 多入口参数应用](services/parameters.md#f15) | 具体实现、演进/冲突与限制见docs |
| [P06 FR-18](../specs/features/06-hmi-tuning/requirements.md#fr-18) | [F17 迁移保存与失败](services/parameters.md#f17)；[F15 多入口参数应用](services/parameters.md#f15) | 具体实现、演进/冲突与限制见docs |
| [P06 TC-1](../specs/features/06-hmi-tuning/requirements.md#tc-01) | [F29 时钟DMA采样与驱动](platform/clocks-and-dma.md#f29-clock-dma) | 具体实现、演进/冲突与限制见docs |
| [P06 TC-2](../specs/features/06-hmi-tuning/requirements.md#tc-02) | [F30 构建烧录与再生](guides/build-and-flash.md#f30) | 具体实现、演进/冲突与限制见docs |
| [P06 TC-3](../specs/features/06-hmi-tuning/requirements.md#tc-03) | [F29 时钟DMA采样与驱动](platform/clocks-and-dma.md#f29-clock-dma) | 具体实现、演进/冲突与限制见docs |
| [P06 TC-4](../specs/features/06-hmi-tuning/requirements.md#tc-04) | [F16 参数v3格式](services/parameters.md#f16) | 具体实现、演进/冲突与限制见docs |
| [P06 TC-5](../specs/features/06-hmi-tuning/requirements.md#tc-05) | [F16 参数v3格式](services/parameters.md#f16) | 具体实现、演进/冲突与限制见docs |
| [P06 TC-6](../specs/features/06-hmi-tuning/requirements.md#tc-06) | [F26 ADC编码器PWM](platform/adc.md#f26-adc) | 具体实现、演进/冲突与限制见docs |

### P07

[阶段需求](../specs/features/07-datalog/requirements.md)。主条目唯一，跨域协作同时列出所需功能；规划/待决定不计已实现，硬件状态仍见阶段原记录。

| 需求/约束 | docs主条目及协作条目 | 状态边界 |
|---|---|---|
| [P07 FR-1](../specs/features/07-datalog/requirements.md#fr-01) | [F19 日志格式容量](services/recording-and-export.md#f19) | 具体实现、演进/冲突与限制见docs |
| [P07 FR-2](../specs/features/07-datalog/requirements.md#fr-02) | [F19 日志格式容量](services/recording-and-export.md#f19) | 具体实现、演进/冲突与限制见docs |
| [P07 FR-3](../specs/features/07-datalog/requirements.md#fr-03) | [F20 队列前台写入与故障](services/recording-and-export.md#f20) | 具体实现、演进/冲突与限制见docs |
| [P07 FR-4](../specs/features/07-datalog/requirements.md#fr-04) | [F20 队列前台写入与故障](services/recording-and-export.md#f20) | 具体实现、演进/冲突与限制见docs |
| [P07 FR-5](../specs/features/07-datalog/requirements.md#fr-05) | [F20 队列前台写入与故障](services/recording-and-export.md#f20)；[F02 起停、模式与首因](system/runtime-and-safety.md#f02) | 具体实现、演进/冲突与限制见docs |
| [P07 FR-6](../specs/features/07-datalog/requirements.md#fr-06) | [F19 日志格式容量](services/recording-and-export.md#f19) | 具体实现、演进/冲突与限制见docs |
| [P07 FR-7](../specs/features/07-datalog/requirements.md#fr-07) | [F21 CSV与会话边界](services/recording-and-export.md#f21) | 具体实现、演进/冲突与限制见docs |
| [P07 FR-8](../specs/features/07-datalog/requirements.md#fr-08) | [F21 CSV与会话边界](services/recording-and-export.md#f21) | 具体实现、演进/冲突与限制见docs |
| [P07 FR-9](../specs/features/07-datalog/requirements.md#fr-09) | [F20 队列前台写入与故障](services/recording-and-export.md#f20) | 具体实现、演进/冲突与限制见docs |
| [P07 TC-1](../specs/features/07-datalog/requirements.md#tc-01) | [F19 日志格式容量](services/recording-and-export.md#f19) | 具体实现、演进/冲突与限制见docs |
| [P07 TC-2](../specs/features/07-datalog/requirements.md#tc-02) | [F17 迁移保存与失败](services/parameters.md#f17) | 具体实现、演进/冲突与限制见docs |
| [P07 TC-3](../specs/features/07-datalog/requirements.md#tc-03) | [F20 队列前台写入与故障](services/recording-and-export.md#f20) | 具体实现、演进/冲突与限制见docs |
| [P07 TC-4](../specs/features/07-datalog/requirements.md#tc-04) | [F30 构建烧录与再生](guides/build-and-flash.md#f30) | 具体实现、演进/冲突与限制见docs |
| [P07 TC-5](../specs/features/07-datalog/requirements.md#tc-05) | [F21 CSV与会话边界](services/recording-and-export.md#f21) | 具体实现、演进/冲突与限制见docs |

### P08

[阶段需求](../specs/features/08-storage-chain-fix/requirements.md)。主条目唯一，跨域协作同时列出所需功能；规划/待决定不计已实现，硬件状态仍见阶段原记录。

| 需求/约束 | docs主条目及协作条目 | 状态边界 |
|---|---|---|
| [P08 FR-1](../specs/features/08-storage-chain-fix/requirements.md#fr-01) | [F17 迁移保存与失败](services/parameters.md#f17)；[F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 具体实现、演进/冲突与限制见docs |
| [P08 FR-2](../specs/features/08-storage-chain-fix/requirements.md#fr-02) | [F17 迁移保存与失败](services/parameters.md#f17) | 具体实现、演进/冲突与限制见docs |
| [P08 FR-3](../specs/features/08-storage-chain-fix/requirements.md#fr-03) | [F17 迁移保存与失败](services/parameters.md#f17) | 具体实现、演进/冲突与限制见docs |
| [P08 FR-4](../specs/features/08-storage-chain-fix/requirements.md#fr-04) | [F17 迁移保存与失败](services/parameters.md#f17)；[F15 多入口参数应用](services/parameters.md#f15)；[F02 起停、模式与首因](system/runtime-and-safety.md#f02) | 具体实现、演进/冲突与限制见docs |
| [P08 FR-5](../specs/features/08-storage-chain-fix/requirements.md#fr-05) | [F20 队列前台写入与故障](services/recording-and-export.md#f20)；[F02 起停、模式与首因](system/runtime-and-safety.md#f02) | 具体实现、演进/冲突与限制见docs |
| [P08 FR-6](../specs/features/08-storage-chain-fix/requirements.md#fr-06) | [F16 参数v3格式](services/parameters.md#f16)；[F17 迁移保存与失败](services/parameters.md#f17) | 具体实现、演进/冲突与限制见docs |
| [P08 FR-7](../specs/features/08-storage-chain-fix/requirements.md#fr-07) | [F15 多入口参数应用](services/parameters.md#f15)；[F02 起停、模式与首因](system/runtime-and-safety.md#f02) | 具体实现、演进/冲突与限制见docs |
| [P08 FR-8](../specs/features/08-storage-chain-fix/requirements.md#fr-08) | [F14 菜单按键与显示](hmi/operation.md#f14)；[F15 多入口参数应用](services/parameters.md#f15) | 具体实现、演进/冲突与限制见docs |

### P09

[阶段需求](../specs/features/09-element-completion/requirements.md)。主条目唯一，跨域协作同时列出所需功能；规划/待决定不计已实现，硬件状态仍见阶段原记录。

| 需求/约束 | docs主条目及协作条目 | 状态边界 |
|---|---|---|
| [P09 FR-1](../specs/features/09-element-completion/requirements.md#fr-01) | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13)；[F09 活动电磁元素](control/elements-and-dormant-code.md#f09) | 规划/待决定 |
| [P09 FR-2](../specs/features/09-element-completion/requirements.md#fr-02) | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 规划/待决定 |
| [P09 FR-3](../specs/features/09-element-completion/requirements.md#fr-03) | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 规划/待决定 |
| [P09 FR-4](../specs/features/09-element-completion/requirements.md#fr-04) | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 规划/待决定 |
| [P09 FR-5](../specs/features/09-element-completion/requirements.md#fr-05) | [F09 活动电磁元素](control/elements-and-dormant-code.md#f09) | 规划/待决定 |
| [P09 FR-6](../specs/features/09-element-completion/requirements.md#fr-06) | [F19 日志格式容量](services/recording-and-export.md#f19)；[F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 规划/待决定 |
| [P09 TC-1](../specs/features/09-element-completion/requirements.md#tc-01) | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 规划/待决定 |
| [P09 TC-2](../specs/features/09-element-completion/requirements.md#tc-02) | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 规划/待决定 |
| [P09 TC-3](../specs/features/09-element-completion/requirements.md#tc-03) | [F12 部分配置与全局调参规划](control/profiles.md#f12) | 规划/待决定 |
| [P09 TC-4](../specs/features/09-element-completion/requirements.md#tc-04) | [F32 标准R08与维护](guides/validation.md#f32) | 规划/待决定 |

### P10

[阶段需求](../specs/features/10-control-consistency/requirements.md)。主条目唯一，跨域协作同时列出所需功能；规划/待决定不计已实现，硬件状态仍见阶段原记录。

| 需求/约束 | docs主条目及协作条目 | 状态边界 |
|---|---|---|
| [P10 FR-1](../specs/features/10-control-consistency/requirements.md#fr-01) | [F12 部分配置与全局调参规划](control/profiles.md#f12)；[F18 完整拍临时调参](services/runtime-tuning.md#f18) | 规划/待决定 |
| [P10 FR-2](../specs/features/10-control-consistency/requirements.md#fr-02) | [F12 部分配置与全局调参规划](control/profiles.md#f12)；[F15 多入口参数应用](services/parameters.md#f15)；[F18 完整拍临时调参](services/runtime-tuning.md#f18) | 规划/待决定 |
| [P10 FR-3](../specs/features/10-control-consistency/requirements.md#fr-03) | [F12 部分配置与全局调参规划](control/profiles.md#f12)；[F08 方向角度速度与执行](control/controllers.md#f08) | 规划/待决定 |
| [P10 FR-4](../specs/features/10-control-consistency/requirements.md#fr-04) | [F12 部分配置与全局调参规划](control/profiles.md#f12) | 规划/待决定 |
| [P10 FR-5](../specs/features/10-control-consistency/requirements.md#fr-05) | [F12 部分配置与全局调参规划](control/profiles.md#f12) | 规划/待决定 |
| [P10 TC-1](../specs/features/10-control-consistency/requirements.md#tc-01) | [F12 部分配置与全局调参规划](control/profiles.md#f12) | 规划/待决定 |
| [P10 TC-2](../specs/features/10-control-consistency/requirements.md#tc-02) | [F12 部分配置与全局调参规划](control/profiles.md#f12) | 规划/待决定 |
| [P10 TC-3](../specs/features/10-control-consistency/requirements.md#tc-03) | [F16 参数v3格式](services/parameters.md#f16) | 规划/待决定 |
| [P10 TC-4](../specs/features/10-control-consistency/requirements.md#tc-04) | [F19 日志格式容量](services/recording-and-export.md#f19) | 规划/待决定 |
| [P10 TC-5](../specs/features/10-control-consistency/requirements.md#tc-05) | [F32 标准R08与维护](guides/validation.md#f32) | 规划/待决定 |

### P11

[阶段需求](../specs/features/11-hardening-cleanup/requirements.md)。主条目唯一，跨域协作同时列出所需功能；规划/待决定不计已实现，硬件状态仍见阶段原记录。

| 需求/约束 | docs主条目及协作条目 | 状态边界 |
|---|---|---|
| [P11 FR-1](../specs/features/11-hardening-cleanup/requirements.md#fr-01) | [F17 迁移保存与失败](services/parameters.md#f17) | 具体实现、演进/冲突与限制见docs |
| [P11 FR-2](../specs/features/11-hardening-cleanup/requirements.md#fr-02) | [F17 迁移保存与失败](services/parameters.md#f17) | 具体实现、演进/冲突与限制见docs |
| [P11 FR-3](../specs/features/11-hardening-cleanup/requirements.md#fr-03) | [F05 电感偏差与对称度](sensing/electromagnetic.md#f05) | 具体实现、演进/冲突与限制见docs |
| [P11 FR-4](../specs/features/11-hardening-cleanup/requirements.md#fr-04) | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 具体实现、演进/冲突与限制见docs |
| [P11 FR-5](../specs/features/11-hardening-cleanup/requirements.md#fr-05) | [F15 多入口参数应用](services/parameters.md#f15) | 具体实现、演进/冲突与限制见docs |
| [P11 FR-6](../specs/features/11-hardening-cleanup/requirements.md#fr-06) | [F19 日志格式容量](services/recording-and-export.md#f19)；[F01 初始化与三线调度](system/runtime-and-safety.md#f01)；[F20 队列前台写入与故障](services/recording-and-export.md#f20)；[F28 DL1B阻塞边界](platform/dl1b.md#f28) | 具体实现、演进/冲突与限制见docs |
| [P11 FR-7](../specs/features/11-hardening-cleanup/requirements.md#fr-07) | [F32 标准R08与维护](guides/validation.md#f32) | 具体实现、演进/冲突与限制见docs |
| [P11 FR-8](../specs/features/11-hardening-cleanup/requirements.md#fr-08) | [F14 菜单按键与显示](hmi/operation.md#f14) | 具体实现、演进/冲突与限制见docs |
| [P11 FR-9](../specs/features/11-hardening-cleanup/requirements.md#fr-09) | [F29 时钟DMA采样与驱动](platform/adc.md#f26-adc) | 具体实现、演进/冲突与限制见docs |
| [P11 FR-10](../specs/features/11-hardening-cleanup/requirements.md#fr-10) | [F29 时钟DMA采样与驱动](platform/clocks-and-dma.md#f29-clock-dma) | 具体实现、演进/冲突与限制见docs |
| [P11 FR-11](../specs/features/11-hardening-cleanup/requirements.md#fr-11) | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 具体实现、演进/冲突与限制见docs |
| [P11 TC-1](../specs/features/11-hardening-cleanup/requirements.md#tc-01) | [F08 方向角度速度与执行](control/controllers.md#f08) | 具体实现、演进/冲突与限制见docs |
| [P11 TC-2](../specs/features/11-hardening-cleanup/requirements.md#tc-02) | [F29 时钟DMA采样与驱动](platform/clocks-and-dma.md#f29-clock-dma) | 具体实现、演进/冲突与限制见docs |
| [P11 TC-3](../specs/features/11-hardening-cleanup/requirements.md#tc-03) | [F17 迁移保存与失败](services/parameters.md#f17) | 具体实现、演进/冲突与限制见docs |
| [P11 TC-4](../specs/features/11-hardening-cleanup/requirements.md#tc-04) | [F32 标准R08与维护](guides/validation.md#f32) | 具体实现、演进/冲突与限制见docs |

### P12

[阶段需求](../specs/features/12-control-safety-closure/requirements.md)。主条目唯一，跨域协作同时列出所需功能；规划/待决定不计已实现，硬件状态仍见阶段原记录。

| 需求/约束 | docs主条目及协作条目 | 状态边界 |
|---|---|---|
| [P12 FR-1](../specs/features/12-control-safety-closure/requirements.md#fr-01) | [F08 方向角度速度与执行](control/controllers.md#f08) | 具体实现、演进/冲突与限制见docs |
| [P12 FR-2](../specs/features/12-control-safety-closure/requirements.md#fr-02) | [F02 起停、模式与首因](system/runtime-and-safety.md#f02-stop) | 具体实现、演进/冲突与限制见docs |
| [P12 FR-3](../specs/features/12-control-safety-closure/requirements.md#fr-03) | [F02 起停、模式与首因](system/runtime-and-safety.md#f02-stop) | 具体实现、演进/冲突与限制见docs |
| [P12 FR-4](../specs/features/12-control-safety-closure/requirements.md#fr-04) | [F02 起停、模式与首因](system/runtime-and-safety.md#f02-stop)；[F20 队列前台写入与故障](services/recording-and-export.md#f20)；[F18 完整拍临时调参](services/runtime-tuning.md#f18) | 具体实现、演进/冲突与限制见docs |
| [P12 FR-5](../specs/features/12-control-safety-closure/requirements.md#fr-05) | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 具体实现、演进/冲突与限制见docs |
| [P12 FR-6](../specs/features/12-control-safety-closure/requirements.md#fr-06) | [F02 起停、模式与首因](system/runtime-and-safety.md#f02-start)；[F07 IMU校准滤波与积分](sensing/attitude.md#f07)；[F08 方向角度速度与执行](control/controllers.md#f08)；[F11 双FSM与唯一执行](control/tracking-modes.md#f11) | 具体实现、演进/冲突与限制见docs |
| [P12 FR-7](../specs/features/12-control-safety-closure/requirements.md#fr-07) | [F29 时钟DMA采样与驱动](platform/sampler.md#f29-sampler)；[F14 菜单按键与显示](hmi/operation.md#f14)；[F07 IMU校准滤波与积分](sensing/attitude.md#f07) | 具体实现、演进/冲突与限制见docs |
| [P12 TC-1](../specs/features/12-control-safety-closure/requirements.md#tc-01) | [F02 起停、模式与首因](system/runtime-and-safety.md#f02) | 具体实现、演进/冲突与限制见docs |
| [P12 TC-2](../specs/features/12-control-safety-closure/requirements.md#tc-02) | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 具体实现、演进/冲突与限制见docs |
| [P12 TC-3](../specs/features/12-control-safety-closure/requirements.md#tc-03) | [F08 方向角度速度与执行](control/controllers.md#f08) | 具体实现、演进/冲突与限制见docs |
| [P12 TC-4](../specs/features/12-control-safety-closure/requirements.md#tc-04) | [F02 起停、模式与首因](system/runtime-and-safety.md#f02-stop) | 具体实现、演进/冲突与限制见docs |
| [P12 TC-5](../specs/features/12-control-safety-closure/requirements.md#tc-05) | [F30 构建烧录与再生](guides/build-and-flash.md#f30) | 具体实现、演进/冲突与限制见docs |

### P13

[阶段需求](../specs/features/13-control-structure/requirements.md)。主条目唯一，跨域协作同时列出所需功能；规划/待决定不计已实现，硬件状态仍见阶段原记录。

| 需求/约束 | docs主条目及协作条目 | 状态边界 |
|---|---|---|
| [P13 FR-1](../specs/features/13-control-structure/requirements.md#fr-01) | [F05 电感偏差与对称度](sensing/electromagnetic.md#f05) | 具体实现、演进/冲突与限制见docs |
| [P13 FR-2](../specs/features/13-control-structure/requirements.md#fr-02) | [F08 方向角度速度与执行](control/controllers.md#f08)；[F15 多入口参数应用](services/parameters.md#f15) | 具体实现、演进/冲突与限制见docs |
| [P13 FR-3](../specs/features/13-control-structure/requirements.md#fr-03) | [F07 IMU校准滤波与积分](sensing/attitude.md#f07) | 具体实现、演进/冲突与限制见docs |
| [P13 FR-4](../specs/features/13-control-structure/requirements.md#fr-04) | [F08 方向角度速度与执行](control/controllers.md#f08)；[F12 部分配置与全局调参规划](control/profiles.md#f12) | 具体实现、演进/冲突与限制见docs |
| [P13 FR-5](../specs/features/13-control-structure/requirements.md#fr-05) | [F08 方向角度速度与执行](control/controllers.md#f08)；[F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 具体实现、演进/冲突与限制见docs |
| [P13 FR-6](../specs/features/13-control-structure/requirements.md#fr-06) | [F12 部分配置与全局调参规划](control/profiles.md#f12)；[F09 活动电磁元素](control/elements-and-dormant-code.md#f09) | 具体实现、演进/冲突与限制见docs |
| [P13 FR-7](../specs/features/13-control-structure/requirements.md#fr-07) | [F16 参数v3格式](services/parameters.md#f16)；[F14 菜单按键与显示](hmi/operation.md#f14)；[F15 多入口参数应用](services/parameters.md#f15)；[F19 日志格式容量](services/recording-and-export.md#f19)；[F21 CSV与会话边界](services/recording-and-export.md#f21) | 具体实现、演进/冲突与限制见docs |
| [P13 FR-8](../specs/features/13-control-structure/requirements.md#fr-08) | [F08 方向角度速度与执行](control/controllers.md#f08)；[F32 标准R08与维护](guides/validation.md#f32) | 具体实现、演进/冲突与限制见docs |
| [P13 TC-1](../specs/features/13-control-structure/requirements.md#tc-01) | [F08 方向角度速度与执行](control/controllers.md#f08) | 具体实现、演进/冲突与限制见docs |
| [P13 TC-2](../specs/features/13-control-structure/requirements.md#tc-02) | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 具体实现、演进/冲突与限制见docs |
| [P13 TC-3](../specs/features/13-control-structure/requirements.md#tc-03) | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 具体实现、演进/冲突与限制见docs |
| [P13 TC-4](../specs/features/13-control-structure/requirements.md#tc-04) | [F31 验证证据与复现](guides/validation.md#f31) | 具体实现、演进/冲突与限制见docs |
| [P13 TC-5](../specs/features/13-control-structure/requirements.md#tc-05) | [F08 方向角度速度与执行](control/controllers.md#f08) | 具体实现、演进/冲突与限制见docs |

### P14

[阶段需求](../specs/features/14-dual-mode-tracking/requirements.md)。主条目唯一，跨域协作同时列出所需功能；规划/待决定不计已实现，硬件状态仍见阶段原记录。

| 需求/约束 | docs主条目及协作条目 | 状态边界 |
|---|---|---|
| [P14 FR-01](../specs/features/14-dual-mode-tracking/requirements.md#fr-01) | [F06 灰度采集标定与质心](sensing/grayscale.md#f06) | 具体实现、演进/冲突与限制见docs |
| [P14 FR-02](../specs/features/14-dual-mode-tracking/requirements.md#fr-02) | [F06 灰度采集标定与质心](sensing/grayscale.md#f06) | 具体实现、演进/冲突与限制见docs |
| [P14 FR-03](../specs/features/14-dual-mode-tracking/requirements.md#fr-03) | [F06 灰度采集标定与质心](sensing/grayscale.md#f06) | 具体实现、演进/冲突与限制见docs |
| [P14 FR-04](../specs/features/14-dual-mode-tracking/requirements.md#fr-04) | [F06 灰度采集标定与质心](sensing/grayscale.md#f06) | 具体实现、演进/冲突与限制见docs |
| [P14 FR-05](../specs/features/14-dual-mode-tracking/requirements.md#fr-05) | [F06 灰度采集标定与质心](sensing/grayscale.md#f06) | 具体实现、演进/冲突与限制见docs |
| [P14 FR-06](../specs/features/14-dual-mode-tracking/requirements.md#fr-06) | [F06 灰度采集标定与质心](sensing/grayscale.md#f06) | 具体实现、演进/冲突与限制见docs |
| [P14 FR-07](../specs/features/14-dual-mode-tracking/requirements.md#fr-07) | [F11 双FSM与唯一执行](control/tracking-modes.md#f11) | 具体实现、演进/冲突与限制见docs |
| [P14 FR-08](../specs/features/14-dual-mode-tracking/requirements.md#fr-08) | [F29 时钟DMA采样与驱动](platform/adc.md#gray-raw)；[F06 灰度采集标定与质心](sensing/grayscale.md#f06) | 具体实现、演进/冲突与限制见docs |
| [P14 FR-09](../specs/features/14-dual-mode-tracking/requirements.md#fr-09) | [F10 光电状态机](control/tracking-modes.md#f10) | 具体实现、演进/冲突与限制见docs |
| [P14 FR-10](../specs/features/14-dual-mode-tracking/requirements.md#fr-10) | [F09 活动电磁元素](control/elements-and-dormant-code.md#f09) | 具体实现、演进/冲突与限制见docs |
| [P14 FR-11](../specs/features/14-dual-mode-tracking/requirements.md#fr-11) | [F11 双FSM与唯一执行](control/tracking-modes.md#f11) | 具体实现、演进/冲突与限制见docs |
| [P14 FR-12](../specs/features/14-dual-mode-tracking/requirements.md#fr-12) | [F16 参数v3格式](services/parameters.md#f16)；[F12 部分配置与全局调参规划](control/profiles.md#f12)；[F15 多入口参数应用](services/parameters.md#f15) | 具体实现、演进/冲突与限制见docs |
| [P14 FR-13](../specs/features/14-dual-mode-tracking/requirements.md#fr-13) | [F02 起停、模式与首因](system/runtime-and-safety.md#f02-start)；[F14 菜单按键与显示](hmi/operation.md#f14) | 具体实现、演进/冲突与限制见docs |
| [P14 FR-14](../specs/features/14-dual-mode-tracking/requirements.md#fr-14) | [F14 菜单按键与显示](hmi/operation.md#f14)；[F15 多入口参数应用](services/parameters.md#f15) | 具体实现、演进/冲突与限制见docs |
| [P14 FR-15](../specs/features/14-dual-mode-tracking/requirements.md#fr-15) | [F16 参数v3格式](services/parameters.md#f16)；[F17 迁移保存与失败](services/parameters.md#f17) | 具体实现、演进/冲突与限制见docs |
| [P14 FR-16](../specs/features/14-dual-mode-tracking/requirements.md#fr-16) | [F02 起停、模式与首因](system/runtime-and-safety.md#f02-start)；[F03 低压持续判定](system/runtime-and-safety.md#f03)；[F04 fatal与底层启停失败](system/runtime-and-safety.md#f04)；[F20 队列前台写入与故障](services/recording-and-export.md#f20) | 具体实现、演进/冲突与限制见docs |
| [P14 FR-17](../specs/features/14-dual-mode-tracking/requirements.md#fr-17) | [F19 日志格式容量](services/recording-and-export.md#f19) | 具体实现、演进/冲突与限制见docs |
| [P14 FR-18](../specs/features/14-dual-mode-tracking/requirements.md#fr-18) | [F20 队列前台写入与故障](services/recording-and-export.md#f20)；[F11 双FSM与唯一执行](control/tracking-modes.md#f11)；[F25 独立上传](services/communication.md#f25) | 具体实现、演进/冲突与限制见docs |
| [P14 FR-19](../specs/features/14-dual-mode-tracking/requirements.md#fr-19) | [F20 队列前台写入与故障](services/recording-and-export.md#f20)；[F17 迁移保存与失败](services/parameters.md#f17) | 具体实现、演进/冲突与限制见docs |
| [P14 FR-20](../specs/features/14-dual-mode-tracking/requirements.md#fr-20) | [F20 队列前台写入与故障](services/recording-and-export.md#f20)；[F02 起停、模式与首因](system/runtime-and-safety.md#f02-start) | 具体实现、演进/冲突与限制见docs |
| [P14 FR-21](../specs/features/14-dual-mode-tracking/requirements.md#fr-21) | [F21 CSV与会话边界](services/recording-and-export.md#f21)；[F22 二进制流式导出](services/recording-and-export.md#f22-export) | 具体实现、演进/冲突与限制见docs |
| [P14 FR-22](../specs/features/14-dual-mode-tracking/requirements.md#fr-22) | [F32 标准R08与维护](guides/validation.md#f32)；[F29 时钟DMA采样与驱动](platform/clocks-and-dma.md#f29-clock-dma)；[F30 构建烧录与再生](guides/build-and-flash.md#f30)；[F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 具体实现、演进/冲突与限制见docs |

### P15

[阶段需求](../specs/features/15-code-style-consistency/requirements.md)。主条目唯一，跨域协作同时列出所需功能；规划/待决定不计已实现，硬件状态仍见阶段原记录。

| 需求/约束 | docs主条目及协作条目 | 状态边界 |
|---|---|---|
| [P15 FR-01](../specs/features/15-code-style-consistency/requirements.md#fr-01) | [F32 标准R08与维护](guides/validation.md#f32) | 具体实现、演进/冲突与限制见docs |
| [P15 FR-02](../specs/features/15-code-style-consistency/requirements.md#fr-02) | [F32 标准R08与维护](guides/validation.md#f32) | 具体实现、演进/冲突与限制见docs |
| [P15 FR-03](../specs/features/15-code-style-consistency/requirements.md#fr-03) | [F32 标准R08与维护](guides/validation.md#f32) | 具体实现、演进/冲突与限制见docs |
| [P15 FR-04](../specs/features/15-code-style-consistency/requirements.md#fr-04) | [F32 标准R08与维护](guides/validation.md#f32) | 具体实现、演进/冲突与限制见docs |
| [P15 FR-05](../specs/features/15-code-style-consistency/requirements.md#fr-05) | [F32 标准R08与维护](guides/validation.md#f32) | 具体实现、演进/冲突与限制见docs |
| [P15 FR-06](../specs/features/15-code-style-consistency/requirements.md#fr-06) | [F06 灰度采集标定与质心](sensing/grayscale.md#f06) | 具体实现、演进/冲突与限制见docs |
| [P15 FR-07](../specs/features/15-code-style-consistency/requirements.md#fr-07) | [F02 起停、模式与首因](system/runtime-and-safety.md#f02) | 具体实现、演进/冲突与限制见docs |
| [P15 FR-08](../specs/features/15-code-style-consistency/requirements.md#fr-08) | [F16 参数v3格式](services/parameters.md#f16) | 具体实现、演进/冲突与限制见docs |
| [P15 FR-09](../specs/features/15-code-style-consistency/requirements.md#fr-09) | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 具体实现、演进/冲突与限制见docs |
| [P15 FR-10](../specs/features/15-code-style-consistency/requirements.md#fr-10) | [F32 标准R08与维护](guides/validation.md#f32) | 具体实现、演进/冲突与限制见docs |
| [P15 FR-11](../specs/features/15-code-style-consistency/requirements.md#fr-11) | [F32 标准R08与维护](guides/validation.md#f32) | 具体实现、演进/冲突与限制见docs |
| [P15 FR-12](../specs/features/15-code-style-consistency/requirements.md#fr-12) | [F32 标准R08与维护](guides/validation.md#f32) | 具体实现、演进/冲突与限制见docs |
| [P15 FR-13](../specs/features/15-code-style-consistency/requirements.md#fr-13) | [F32 标准R08与维护](guides/validation.md#f32) | 具体实现、演进/冲突与限制见docs |
| [P15 FR-14](../specs/features/15-code-style-consistency/requirements.md#fr-14) | [F32 标准R08与维护](guides/validation.md#f32) | 具体实现、演进/冲突与限制见docs |
| [P15 FR-15](../specs/features/15-code-style-consistency/requirements.md#fr-15) | [F32 标准R08与维护](guides/validation.md#f32) | 具体实现、演进/冲突与限制见docs |
| [P15 FR-16](../specs/features/15-code-style-consistency/requirements.md#fr-16) | [F32 标准R08与维护](guides/validation.md#f32) | 具体实现、演进/冲突与限制见docs |
| [P15 FR-17](../specs/features/15-code-style-consistency/requirements.md#fr-17) | [F32 标准R08与维护](guides/validation.md#f32) | 具体实现、演进/冲突与限制见docs |
| [P15 FR-18](../specs/features/15-code-style-consistency/requirements.md#fr-18) | [F32 标准R08与维护](guides/validation.md#f32) | 具体实现、演进/冲突与限制见docs |
| [P15 FR-19](../specs/features/15-code-style-consistency/requirements.md#fr-19) | [F32 标准R08与维护](guides/validation.md#f32) | 具体实现、演进/冲突与限制见docs |
| [P15 FR-20](../specs/features/15-code-style-consistency/requirements.md#fr-20) | [F32 标准R08与维护](guides/validation.md#f32) | 具体实现、演进/冲突与限制见docs |
| [P15 FR-21](../specs/features/15-code-style-consistency/requirements.md#fr-21) | [F32 标准R08与维护](guides/validation.md#f32) | 具体实现、演进/冲突与限制见docs |
| [P15 FR-22](../specs/features/15-code-style-consistency/requirements.md#fr-22) | [F32 标准R08与维护](guides/validation.md#f32) | 具体实现、演进/冲突与限制见docs |
| [P15 FR-23](../specs/features/15-code-style-consistency/requirements.md#fr-23) | [F32 标准R08与维护](guides/validation.md#f32) | 具体实现、演进/冲突与限制见docs |
| [P15 FR-24](../specs/features/15-code-style-consistency/requirements.md#fr-24) | [F32 标准R08与维护](guides/validation.md#f32) | 具体实现、演进/冲突与限制见docs |
| [P15 FR-25](../specs/features/15-code-style-consistency/requirements.md#fr-25) | [F32 标准R08与维护](guides/validation.md#f32) | 具体实现、演进/冲突与限制见docs |

### P16

[阶段需求](../specs/features/16-spi-wifi-link/requirements.md)。主条目唯一，跨域协作同时列出所需功能；规划/待决定不计已实现，硬件状态仍见阶段原记录。

| 需求/约束 | docs主条目及协作条目 | 状态边界 |
|---|---|---|
| [P16 FR-01](../specs/features/16-spi-wifi-link/requirements.md#fr-01) | [F29 时钟DMA采样与驱动](platform/spi-wifi.md#f29-wifi)；[F01 初始化与三线调度](system/runtime-and-safety.md#f01)；[F08 方向角度速度与执行](control/controllers.md#f08) | 具体实现、演进/冲突与限制见docs |
| [P16 FR-02](../specs/features/16-spi-wifi-link/requirements.md#fr-02) | [F29 时钟DMA采样与驱动](platform/spi-wifi.md#f29-wifi)；[F23 SCB帧JSON与命令](services/communication.md#f23) | 具体实现、演进/冲突与限制见docs |
| [P16 FR-03](../specs/features/16-spi-wifi-link/requirements.md#fr-03) | [F29 时钟DMA采样与驱动](platform/spi-wifi.md#f29-wifi)；[F30 构建烧录与再生](guides/build-and-flash.md#f30) | 具体实现、演进/冲突与限制见docs |
| [P16 FR-04](../specs/features/16-spi-wifi-link/requirements.md#fr-04) | [F29 时钟DMA采样与驱动](platform/spi-wifi.md#f29-wifi)；[F24 会话可靠传输与回绕](services/communication.md#f24) | 具体实现、演进/冲突与限制见docs |
| [P16 FR-05](../specs/features/16-spi-wifi-link/requirements.md#fr-05) | [F23 SCB帧JSON与命令](services/communication.md#f23) | 具体实现、演进/冲突与限制见docs |
| [P16 FR-06](../specs/features/16-spi-wifi-link/requirements.md#fr-06) | [F24 会话可靠传输与回绕](services/communication.md#f24) | 具体实现、演进/冲突与限制见docs |
| [P16 FR-07](../specs/features/16-spi-wifi-link/requirements.md#fr-07) | [F24 会话可靠传输与回绕](services/communication.md#f24) | 具体实现、演进/冲突与限制见docs |
| [P16 FR-08](../specs/features/16-spi-wifi-link/requirements.md#fr-08) | [F24 会话可靠传输与回绕](services/communication.md#f24)；[F18 完整拍临时调参](services/runtime-tuning.md#f18) | 具体实现、演进/冲突与限制见docs |
| [P16 FR-09](../specs/features/16-spi-wifi-link/requirements.md#fr-09) | [F23 SCB帧JSON与命令](services/communication.md#f23)；[F18 完整拍临时调参](services/runtime-tuning.md#f18)；[F22 二进制流式导出](services/recording-and-export.md#f22-export) | 具体实现、演进/冲突与限制见docs |
| [P16 FR-10](../specs/features/16-spi-wifi-link/requirements.md#fr-10) | [F25 独立上传](services/communication.md#f25) | 具体实现、演进/冲突与限制见docs |
| [P16 FR-11](../specs/features/16-spi-wifi-link/requirements.md#fr-11) | [F25 独立上传](services/communication.md#f25) | 具体实现、演进/冲突与限制见docs |
| [P16 FR-12](../specs/features/16-spi-wifi-link/requirements.md#fr-12) | [F25 独立上传](services/communication.md#f25)；[F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 具体实现、演进/冲突与限制见docs |
| [P16 FR-13](../specs/features/16-spi-wifi-link/requirements.md#fr-13) | [F18 完整拍临时调参](services/runtime-tuning.md#f18)；[F23 SCB帧JSON与命令](services/communication.md#f23) | 具体实现、演进/冲突与限制见docs |
| [P16 FR-14](../specs/features/16-spi-wifi-link/requirements.md#fr-14) | [F18 完整拍临时调参](services/runtime-tuning.md#f18) | 具体实现、演进/冲突与限制见docs |
| [P16 FR-15](../specs/features/16-spi-wifi-link/requirements.md#fr-15) | [F18 完整拍临时调参](services/runtime-tuning.md#f18) | 具体实现、演进/冲突与限制见docs |
| [P16 FR-16](../specs/features/16-spi-wifi-link/requirements.md#fr-16) | [F18 完整拍临时调参](services/runtime-tuning.md#f18) | 具体实现、演进/冲突与限制见docs |
| [P16 FR-17](../specs/features/16-spi-wifi-link/requirements.md#fr-17) | [F18 完整拍临时调参](services/runtime-tuning.md#f18) | 具体实现、演进/冲突与限制见docs |
| [P16 FR-18](../specs/features/16-spi-wifi-link/requirements.md#fr-18) | [F18 完整拍临时调参](services/runtime-tuning.md#f18)；[F17 迁移保存与失败](services/parameters.md#f17) | 具体实现、演进/冲突与限制见docs |
| [P16 FR-19](../specs/features/16-spi-wifi-link/requirements.md#fr-19) | [F22 二进制流式导出](services/recording-and-export.md#f22-export)；[F19 日志格式容量](services/recording-and-export.md#f19) | 具体实现、演进/冲突与限制见docs |
| [P16 FR-20](../specs/features/16-spi-wifi-link/requirements.md#fr-20) | [F22 二进制流式导出](services/recording-and-export.md#f22-export) | 具体实现、演进/冲突与限制见docs |
| [P16 FR-21](../specs/features/16-spi-wifi-link/requirements.md#fr-21) | [F22 二进制流式导出](services/recording-and-export.md#f22-export)；[F02 起停、模式与首因](system/runtime-and-safety.md#f02) | 具体实现、演进/冲突与限制见docs |
| [P16 FR-22](../specs/features/16-spi-wifi-link/requirements.md#fr-22) | [F24 会话可靠传输与回绕](services/communication.md#f24)；[F25 独立上传](services/communication.md#f25)；[F18 完整拍临时调参](services/runtime-tuning.md#f18) | 具体实现、演进/冲突与限制见docs |
| [P16 FR-23](../specs/features/16-spi-wifi-link/requirements.md#fr-23) | [F30 构建烧录与再生](guides/build-and-flash.md#f30)；[F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 具体实现、演进/冲突与限制见docs |
| [P16 FR-24](../specs/features/16-spi-wifi-link/requirements.md#fr-24) | [F31 验证证据与复现](guides/validation.md#f31)；[F32 标准R08与维护](guides/validation.md#f32) | 具体实现、演进/冲突与限制见docs |

### P17

[阶段需求](../specs/features/17-app-architecture/requirements.md)。主条目唯一，跨域协作同时列出所需功能；规划/待决定不计已实现，硬件状态仍见阶段原记录。

| 需求/约束 | docs主条目及协作条目 | 状态边界 |
|---|---|---|
| [P17 FR-01](../specs/features/17-app-architecture/requirements.md#fr-01) | [F32 标准R08与维护](guides/validation.md#f32) | 具体实现、演进/冲突与限制见docs |
| [P17 FR-02](../specs/features/17-app-architecture/requirements.md#fr-02) | [F32 标准R08与维护](guides/validation.md#f32) | 具体实现、演进/冲突与限制见docs |
| [P17 FR-03](../specs/features/17-app-architecture/requirements.md#fr-03) | [F30 构建烧录与再生](guides/build-and-flash.md#f30) | 具体实现、演进/冲突与限制见docs |
| [P17 FR-04](../specs/features/17-app-architecture/requirements.md#fr-04) | [F01 初始化与三线调度](system/runtime-and-safety.md#f01)；[F02 起停、模式与首因](system/runtime-and-safety.md#f02)；[F29 时钟DMA采样与驱动](platform/clocks-and-dma.md#f29-clock-dma) | 具体实现、演进/冲突与限制见docs |
| [P17 FR-05](../specs/features/17-app-architecture/requirements.md#fr-05) | [F05 电感偏差与对称度](sensing/electromagnetic.md#f05)；[F06 灰度采集标定与质心](sensing/grayscale.md#f06)；[F07 IMU校准滤波与积分](sensing/attitude.md#f07)；[F03 低压持续判定](system/runtime-and-safety.md#f03) | 具体实现、演进/冲突与限制见docs |
| [P17 FR-06](../specs/features/17-app-architecture/requirements.md#fr-06) | [F08 方向角度速度与执行](control/controllers.md#f08)；[F09 活动电磁元素](control/elements-and-dormant-code.md#f09)；[F10 光电状态机](control/tracking-modes.md#f10)；[F11 双FSM与唯一执行](control/tracking-modes.md#f11)；[F12 部分配置与全局调参规划](control/profiles.md#f12) | 具体实现、演进/冲突与限制见docs |
| [P17 FR-07](../specs/features/17-app-architecture/requirements.md#fr-07) | [F18 完整拍临时调参](services/runtime-tuning.md#f18)；[F15 多入口参数应用](services/parameters.md#f15)；[F16 参数v3格式](services/parameters.md#f16)；[F17 迁移保存与失败](services/parameters.md#f17) | 具体实现、演进/冲突与限制见docs |
| [P17 FR-08](../specs/features/17-app-architecture/requirements.md#fr-08) | [F20 队列前台写入与故障](services/recording-and-export.md#f20)；[F19 日志格式容量](services/recording-and-export.md#f19)；[F21 CSV与会话边界](services/recording-and-export.md#f21)；[F22 二进制流式导出](services/recording-and-export.md#f22-export)；[F25 独立上传](services/communication.md#f25) | 具体实现、演进/冲突与限制见docs |
| [P17 FR-09](../specs/features/17-app-architecture/requirements.md#fr-09) | [F23 SCB帧JSON与命令](services/communication.md#f23)；[F14 菜单按键与显示](hmi/operation.md#f14)；[F24 会话可靠传输与回绕](services/communication.md#f24) | 具体实现、演进/冲突与限制见docs |
| [P17 FR-10](../specs/features/17-app-architecture/requirements.md#fr-10) | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 具体实现、演进/冲突与限制见docs |
| [P17 FR-11](../specs/features/17-app-architecture/requirements.md#fr-11) | [F11 双FSM与唯一执行](control/tracking-modes.md#f11)；[F01 初始化与三线调度](system/runtime-and-safety.md#f01)；[F20 队列前台写入与故障](services/recording-and-export.md#f20) | 具体实现、演进/冲突与限制见docs |
| [P17 FR-12](../specs/features/17-app-architecture/requirements.md#fr-12) | [F11 双FSM与唯一执行](control/tracking-modes.md#f11)；[F01 初始化与三线调度](system/runtime-and-safety.md#f01)；[F08 方向角度速度与执行](control/controllers.md#f08)；[F19 日志格式容量](services/recording-and-export.md#f19)；[F23 SCB帧JSON与命令](services/communication.md#f23) | 具体实现、演进/冲突与限制见docs |
| [P17 FR-13](../specs/features/17-app-architecture/requirements.md#fr-13) | [F32 标准R08与维护](guides/validation.md#f32) | 具体实现、演进/冲突与限制见docs |

## 后续维护

Phase保留完整设计/映射/标准与结果并引用本表；先确认并独立提交Phase，再更新开工组Docs、紧接实现、双向核对并同批交付匹配Docs与代码。未开工目标留Phase，历史正文按当时基线解释。主条目登记齐全不等于语义或硬件已验。

## 2026-11-06 独立板测增量

| 需求 | 功能 | 验证 |
|---|---|---|
| [P02 FR-27](../specs/features/02-bsp-platform/requirements.md#fr-27) | [F33](platform/isolated-board-tests.md#f33) | BT-V01～05及Phase02 validation |
| [P02 FR-28](../specs/features/02-bsp-platform/requirements.md#fr-28) | [采集/窗口](platform/isolated-board-tests.md#f33-adc)、[灰度](platform/isolated-board-tests.md#f33-gray)、[编码器](platform/isolated-board-tests.md#f33-encoder)、[DL1B](platform/isolated-board-tests.md#f33-dl1b)、[电池](platform/isolated-board-tests.md#f33-battery)、[按键](platform/isolated-board-tests.md#f33-keys) | BT-V01～05及Phase02 validation |
| [P02 FR-29](../specs/features/02-bsp-platform/requirements.md#fr-29) | [F33](platform/isolated-board-tests.md#f33-imu) | BT-V01～05及Phase02 validation |
| [P02 FR-30](../specs/features/02-bsp-platform/requirements.md#fr-30) | [F33](platform/isolated-board-tests.md#f33) | BT-V01～05及Phase02 validation |

## 父功能与具体子条目主归属

身份F01～F33保持，父页负责总览；详细机制只维护于下列主主题，协作不代表重复所有权。

| 父功能 | 具体子条目 | 主说明 | 源码主归属 |
|---|---|---|---|
| F08 | f08-direction | [主说明](control/controllers.md#f08-direction) | direction_controller/PID |
| F08 | f08-angle | [主说明](control/controllers.md#f08-angle) | angle_controller/PID |
| F08 | f08-speed | [主说明](control/controllers.md#f08-speed) | speed_controller/PID |
| F08 | f08-motor | [主说明](control/motor-execution.md#f08-motor) | motor |
| F26 | f26-adc | [主说明](platform/adc.md#f26-adc) | bsp_adc/Core adc |
| F26 | f26-encoder | [主说明](platform/encoders.md#f26-encoder) | bsp_encoder/Core tim |
| F26 | f26-pwm | [主说明](platform/pwm-and-direction.md#f26-pwm) | bsp_pwm/Core tim |
| F29 | f29-clock-dma | [主说明](platform/clocks-and-dma.md#f29-clock-dma) | bsp_runtime/time/Core clock/dma |
| F29 | f29-sampler | [主说明](platform/sampler.md#f29-sampler) | bsp_sampler |
| F29 | f29-imu | [主说明](platform/imu660rb.md#f29-imu) | bsp_imu660rb |
| F29 | f29-flash | [主说明](platform/w25q64-and-flash.md#f29-flash) | bsp_flash/w25q64 |
| F29 | f29-lcd | [主说明](platform/lcd.md#f29-lcd) | bsp_lcd/lcd |
| F29 | f29-wifi | [主说明](platform/spi-wifi.md#f29-wifi) | bsp_wifi |
| F33 | f33-lcd | [主说明](platform/isolated-board-tests.md#f33-lcd) | board_test/bsp_board_test |
| F33 | f33-adc | [主说明](platform/isolated-board-tests.md#f33-adc) | board_test/bsp_board_test |
| F33 | f33-gray | [主说明](platform/isolated-board-tests.md#f33-gray) | board_test/bsp_board_test |
| F33 | f33-imu | [主说明](platform/isolated-board-tests.md#f33-imu) | board_test/bsp_board_test |
| F33 | f33-dl1b | [主说明](platform/isolated-board-tests.md#f33-dl1b) | board_test/bsp_board_test |
| F33 | f33-encoder | [主说明](platform/isolated-board-tests.md#f33-encoder) | board_test/bsp_board_test |
| F33 | f33-battery | [主说明](platform/isolated-board-tests.md#f33-battery) | board_test/bsp_board_test |
| F33 | f33-keys | [主说明](platform/isolated-board-tests.md#f33-keys) | board_test/bsp_board_test |
