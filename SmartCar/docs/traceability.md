# Phase → docs 功能追溯

Phase保留需求/任务/验收，当前详细契约先进入下表docs主条目，再由功能页定位源码。下表只维护第一段映射，不复制函数/字段清单。模块反查见[architecture](architecture.md)。
所有条目核对基线68cb508；存在实现不等于实机完成。P09/P10目标仍未定或未实施，FR/TC标识保持原文（前导0不改变编号身份）。

## 32功能主条目

| ID | 功能 | 唯一主说明 |
|---|---|---|
| F01 | 初始化与三线调度 | [system/runtime-and-safety.md#f01](system/runtime-and-safety.md#f01) |
| F02 | 起停、模式与首因 | [system/runtime-and-safety.md#f02](system/runtime-and-safety.md#f02) |
| F03 | 低压持续判定 | [system/runtime-and-safety.md#f03](system/runtime-and-safety.md#f03) |
| F04 | fatal与底层启停失败 | [system/runtime-and-safety.md#f04](system/runtime-and-safety.md#f04) |
| F05 | 电感偏差与对称度 | [sensing/sensors-and-calibration.md#f05](sensing/sensors-and-calibration.md#f05) |
| F06 | 灰度采集标定与质心 | [sensing/sensors-and-calibration.md#f06](sensing/sensors-and-calibration.md#f06) |
| F07 | IMU校准滤波与积分 | [sensing/sensors-and-calibration.md#f07](sensing/sensors-and-calibration.md#f07) |
| F08 | 方向角度速度与执行 | [control/tracking-and-controllers.md#f08](control/tracking-and-controllers.md#f08) |
| F09 | 活动电磁元素 | [control/elements-and-dormant-code.md#f09](control/elements-and-dormant-code.md#f09) |
| F10 | 光电状态机 | [control/tracking-and-controllers.md#f10](control/tracking-and-controllers.md#f10) |
| F11 | 双FSM与唯一执行 | [control/tracking-and-controllers.md#f11](control/tracking-and-controllers.md#f11) |
| F12 | 部分配置与全局调参规划 | [control/tracking-and-controllers.md#f12](control/tracking-and-controllers.md#f12) |
| F13 | legacy与未接入元素 | [control/elements-and-dormant-code.md#f13](control/elements-and-dormant-code.md#f13) |
| F14 | 菜单按键与显示 | [hmi/operation.md#f14](hmi/operation.md#f14) |
| F15 | 多入口参数应用 | [services/parameters-and-tuning.md#f15](services/parameters-and-tuning.md#f15) |
| F16 | 参数v3格式 | [services/parameters-and-tuning.md#f16](services/parameters-and-tuning.md#f16) |
| F17 | 迁移保存与失败 | [services/parameters-and-tuning.md#f17](services/parameters-and-tuning.md#f17) |
| F18 | 完整拍临时调参 | [services/parameters-and-tuning.md#f18](services/parameters-and-tuning.md#f18) |
| F19 | 日志格式容量 | [services/recording-and-export.md#f19](services/recording-and-export.md#f19) |
| F20 | 队列前台写入与故障 | [services/recording-and-export.md#f20](services/recording-and-export.md#f20) |
| F21 | CSV与会话边界 | [services/recording-and-export.md#f21](services/recording-and-export.md#f21) |
| F22 | 二进制流式导出 | [services/recording-and-export.md#f22](services/recording-and-export.md#f22) |
| F23 | SCB帧JSON与命令 | [services/communication.md#f23](services/communication.md#f23) |
| F24 | 会话可靠传输与回绕 | [services/communication.md#f24](services/communication.md#f24) |
| F25 | 独立上传 | [services/communication.md#f25](services/communication.md#f25) |
| F26 | ADC编码器PWM | [platform/board-and-bsp.md#f26](platform/board-and-bsp.md#f26) |
| F27 | UART失败返回 | [platform/board-and-bsp.md#f27](platform/board-and-bsp.md#f27) |
| F28 | DL1B阻塞边界 | [platform/board-and-bsp.md#f28](platform/board-and-bsp.md#f28) |
| F29 | 时钟DMA采样与驱动 | [platform/board-and-bsp.md#f29](platform/board-and-bsp.md#f29) |
| F30 | 构建烧录与再生 | [guides/build-and-flash.md#f30](guides/build-and-flash.md#f30) |
| F31 | 验证证据与复现 | [guides/validation.md#f31](guides/validation.md#f31) |
| F32 | 标准R08与维护 | [guides/validation.md#f32](guides/validation.md#f32) |

## 逐阶段需求与约束

### P01

[阶段需求](../specs/features/01-runtime-architecture/requirements.md) → 以下条目。当前实现和历史演进差异由功能页说明，验收状态以阶段记录为准。

| 需求/约束 | docs主条目 | 状态边界 |
|---|---|---|
| P01 FR-1 | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 已知演进/限制见docs |
| P01 FR-2 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P01 FR-3 | [F07 IMU校准滤波与积分](sensing/sensors-and-calibration.md#f07) | 已知演进/限制见docs |
| P01 FR-4 | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 已知演进/限制见docs |
| P01 FR-5 | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 已知演进/限制见docs |
| P01 FR-6 | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 已知演进/限制见docs |
| P01 FR-7 | [F02 起停、模式与首因](system/runtime-and-safety.md#f02) | 已知演进/限制见docs |
| P01 FR-8 | [F20 队列前台写入与故障](services/recording-and-export.md#f20) | 已知演进/限制见docs |
| P01 FR-9 | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 已知演进/限制见docs |
| P01 FR-10 | [F03 低压持续判定](system/runtime-and-safety.md#f03) | 已知演进/限制见docs |
| P01 FR-11 | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 已知演进/限制见docs |
| P01 FR-12 | [F03 低压持续判定](system/runtime-and-safety.md#f03) | 已知演进/限制见docs |
| P01 FR-13 | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 已知演进/限制见docs |
| P01 FR-14 | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 已知演进/限制见docs |
| P01 TC-1 | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 已知演进/限制见docs |
| P01 TC-2 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P01 TC-3 | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 已知演进/限制见docs |
| P01 TC-4 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P01 TC-5 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P01 TC-6 | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 已知演进/限制见docs |
| P01 TC-7 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |

### P02

[阶段需求](../specs/features/02-bsp-platform/requirements.md) → 以下条目。当前实现和历史演进差异由功能页说明，验收状态以阶段记录为准。

| 需求/约束 | docs主条目 | 状态边界 |
|---|---|---|
| P02 FR-1 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P02 FR-2 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P02 FR-3 | [F27 UART失败返回](platform/board-and-bsp.md#f27) | 已知演进/限制见docs |
| P02 FR-4 | [F27 UART失败返回](platform/board-and-bsp.md#f27) | 已知演进/限制见docs |
| P02 FR-5 | [F27 UART失败返回](platform/board-and-bsp.md#f27) | 冲突待决定 |
| P02 FR-6 | [F26 ADC编码器PWM](platform/board-and-bsp.md#f26) | 已知演进/限制见docs |
| P02 FR-7 | [F26 ADC编码器PWM](platform/board-and-bsp.md#f26) | 已知演进/限制见docs |
| P02 TC-1 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P02 FR-8 | [F26 ADC编码器PWM](platform/board-and-bsp.md#f26) | 已知演进/限制见docs |
| P02 FR-9 | [F26 ADC编码器PWM](platform/board-and-bsp.md#f26) | 已知演进/限制见docs |
| P02 FR-10 | [F26 ADC编码器PWM](platform/board-and-bsp.md#f26) | 已知演进/限制见docs |
| P02 FR-11 | [F14 菜单按键与显示](hmi/operation.md#f14) | 已知演进/限制见docs |
| P02 FR-12 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P02 FR-13 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P02 TC-2 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P02 TC-3 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P02 FR-14 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P02 FR-15 | [F07 IMU校准滤波与积分](sensing/sensors-and-calibration.md#f07) | 已知演进/限制见docs |
| P02 FR-16 | [F28 DL1B阻塞边界](platform/board-and-bsp.md#f28) | 已知演进/限制见docs |
| P02 FR-17 | [F28 DL1B阻塞边界](platform/board-and-bsp.md#f28) | 冲突待决定 |
| P02 FR-18 | [F14 菜单按键与显示](hmi/operation.md#f14) | 已知演进/限制见docs |
| P02 FR-19 | [F14 菜单按键与显示](hmi/operation.md#f14) | 已知演进/限制见docs |
| P02 FR-20 | [F14 菜单按键与显示](hmi/operation.md#f14) | 已知演进/限制见docs |
| P02 FR-21 | [F17 迁移保存与失败](services/parameters-and-tuning.md#f17) | 已知演进/限制见docs |
| P02 FR-22 | [F17 迁移保存与失败](services/parameters-and-tuning.md#f17) | 已知演进/限制见docs |
| P02 FR-23 | [F17 迁移保存与失败](services/parameters-and-tuning.md#f17) | 已知演进/限制见docs |
| P02 FR-24 | [F20 队列前台写入与故障](services/recording-and-export.md#f20) | 已知演进/限制见docs |
| P02 FR-25 | [F17 迁移保存与失败](services/parameters-and-tuning.md#f17) | 已知演进/限制见docs |
| P02 FR-26 | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 已知演进/限制见docs |
| P02 TC-4 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P02 TC-5 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P02 TC-6 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P02 TC-7 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |

### P03

[阶段需求](../specs/features/03-track-following/requirements.md) → 以下条目。当前实现和历史演进差异由功能页说明，验收状态以阶段记录为准。

| 需求/约束 | docs主条目 | 状态边界 |
|---|---|---|
| P03 FR-1 | [F05 电感偏差与对称度](sensing/sensors-and-calibration.md#f05) | 已知演进/限制见docs |
| P03 FR-2 | [F05 电感偏差与对称度](sensing/sensors-and-calibration.md#f05) | 已知演进/限制见docs |
| P03 FR-3 | [F05 电感偏差与对称度](sensing/sensors-and-calibration.md#f05) | 已知演进/限制见docs |
| P03 FR-4 | [F08 方向角度速度与执行](control/tracking-and-controllers.md#f08) | 已知演进/限制见docs |
| P03 FR-5 | [F08 方向角度速度与执行](control/tracking-and-controllers.md#f08) | 已知演进/限制见docs |
| P03 FR-6 | [F08 方向角度速度与执行](control/tracking-and-controllers.md#f08) | 已知演进/限制见docs |
| P03 FR-7 | [F26 ADC编码器PWM](platform/board-and-bsp.md#f26) | 已知演进/限制见docs |
| P03 FR-8 | [F08 方向角度速度与执行](control/tracking-and-controllers.md#f08) | 已知演进/限制见docs |
| P03 FR-9 | [F02 起停、模式与首因](system/runtime-and-safety.md#f02) | 已知演进/限制见docs |
| P03 FR-10 | [F08 方向角度速度与执行](control/tracking-and-controllers.md#f08) | 已知演进/限制见docs |
| P03 TC-1 | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 已知演进/限制见docs |
| P03 TC-2 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P03 TC-3 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P03 TC-4 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P03 TC-5 | [F05 电感偏差与对称度](sensing/sensors-and-calibration.md#f05) | 已知演进/限制见docs |
| P03 TC-6 | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 已知演进/限制见docs |
| P03 TC-7 | [F05 电感偏差与对称度](sensing/sensors-and-calibration.md#f05) | 已知演进/限制见docs |

### P04

[阶段需求](../specs/features/04-element-fsm/requirements.md) → 以下条目。当前实现和历史演进差异由功能页说明，验收状态以阶段记录为准。

| 需求/约束 | docs主条目 | 状态边界 |
|---|---|---|
| P04 FR-1 | [F09 活动电磁元素](control/elements-and-dormant-code.md#f09) | 已知演进/限制见docs |
| P04 FR-2 | [F02 起停、模式与首因](system/runtime-and-safety.md#f02) | 已知演进/限制见docs |
| P04 FR-3 | [F09 活动电磁元素](control/elements-and-dormant-code.md#f09) | 已知演进/限制见docs |
| P04 FR-4 | [F09 活动电磁元素](control/elements-and-dormant-code.md#f09) | 已知演进/限制见docs |
| P04 FR-5 | [F09 活动电磁元素](control/elements-and-dormant-code.md#f09) | 已知演进/限制见docs |
| P04 FR-6 | [F09 活动电磁元素](control/elements-and-dormant-code.md#f09) | 已知演进/限制见docs |
| P04 FR-7 | [F09 活动电磁元素](control/elements-and-dormant-code.md#f09) | 已知演进/限制见docs |
| P04 FR-8 | [F09 活动电磁元素](control/elements-and-dormant-code.md#f09) | 已知演进/限制见docs |
| P04 FR-9 | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 已知演进/限制见docs |
| P04 FR-10 | [F09 活动电磁元素](control/elements-and-dormant-code.md#f09) | 已知演进/限制见docs |
| P04 FR-11 | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 已知演进/限制见docs |
| P04 TC-1 | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 已知演进/限制见docs |
| P04 TC-2 | [F09 活动电磁元素](control/elements-and-dormant-code.md#f09) | 已知演进/限制见docs |
| P04 TC-3 | [F09 活动电磁元素](control/elements-and-dormant-code.md#f09) | 已知演进/限制见docs |
| P04 TC-4 | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 已知演进/限制见docs |
| P04 TC-5 | [F09 活动电磁元素](control/elements-and-dormant-code.md#f09) | 已知演进/限制见docs |

### P05

[阶段需求](../specs/features/05-imu-attitude/requirements.md) → 以下条目。当前实现和历史演进差异由功能页说明，验收状态以阶段记录为准。

| 需求/约束 | docs主条目 | 状态边界 |
|---|---|---|
| P05 FR-1 | [F07 IMU校准滤波与积分](sensing/sensors-and-calibration.md#f07) | 已知演进/限制见docs |
| P05 FR-2 | [F07 IMU校准滤波与积分](sensing/sensors-and-calibration.md#f07) | 已知演进/限制见docs |
| P05 FR-3 | [F07 IMU校准滤波与积分](sensing/sensors-and-calibration.md#f07) | 已知演进/限制见docs |
| P05 FR-4 | [F07 IMU校准滤波与积分](sensing/sensors-and-calibration.md#f07) | 已知演进/限制见docs |
| P05 FR-5 | [F07 IMU校准滤波与积分](sensing/sensors-and-calibration.md#f07) | 已知演进/限制见docs |
| P05 FR-6 | [F07 IMU校准滤波与积分](sensing/sensors-and-calibration.md#f07) | 已知演进/限制见docs |
| P05 FR-7 | [F07 IMU校准滤波与积分](sensing/sensors-and-calibration.md#f07) | 已知演进/限制见docs |
| P05 FR-8 | [F07 IMU校准滤波与积分](sensing/sensors-and-calibration.md#f07) | 已知演进/限制见docs |
| P05 TC-1 | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 已知演进/限制见docs |
| P05 TC-2 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P05 TC-3 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P05 TC-4 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P05 TC-5 | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 已知演进/限制见docs |

### P06

[阶段需求](../specs/features/06-hmi-tuning/requirements.md) → 以下条目。当前实现和历史演进差异由功能页说明，验收状态以阶段记录为准。

| 需求/约束 | docs主条目 | 状态边界 |
|---|---|---|
| P06 FR-1 | [F14 菜单按键与显示](hmi/operation.md#f14) | 已知演进/限制见docs |
| P06 FR-2 | [F14 菜单按键与显示](hmi/operation.md#f14) | 已知演进/限制见docs |
| P06 FR-3 | [F14 菜单按键与显示](hmi/operation.md#f14) | 已知演进/限制见docs |
| P06 FR-4 | [F14 菜单按键与显示](hmi/operation.md#f14) | 已知演进/限制见docs |
| P06 FR-5 | [F15 多入口参数应用](services/parameters-and-tuning.md#f15) | 已知演进/限制见docs |
| P06 FR-6 | [F14 菜单按键与显示](hmi/operation.md#f14) | 已知演进/限制见docs |
| P06 FR-7 | [F14 菜单按键与显示](hmi/operation.md#f14) | 已知演进/限制见docs |
| P06 FR-8 | [F14 菜单按键与显示](hmi/operation.md#f14) | 已知演进/限制见docs |
| P06 FR-9 | [F02 起停、模式与首因](system/runtime-and-safety.md#f02) | 已知演进/限制见docs |
| P06 FR-10 | [F14 菜单按键与显示](hmi/operation.md#f14) | 已知演进/限制见docs |
| P06 FR-11 | [F17 迁移保存与失败](services/parameters-and-tuning.md#f17) | 已知演进/限制见docs |
| P06 FR-12 | [F16 参数v3格式](services/parameters-and-tuning.md#f16) | 已知演进/限制见docs |
| P06 FR-13 | [F17 迁移保存与失败](services/parameters-and-tuning.md#f17) | 已知演进/限制见docs |
| P06 FR-14 | [F17 迁移保存与失败](services/parameters-and-tuning.md#f17) | 已知演进/限制见docs |
| P06 FR-15 | [F15 多入口参数应用](services/parameters-and-tuning.md#f15) | 已知演进/限制见docs |
| P06 FR-16 | [F15 多入口参数应用](services/parameters-and-tuning.md#f15) | 已知演进/限制见docs |
| P06 FR-17 | [F15 多入口参数应用](services/parameters-and-tuning.md#f15) | 已知演进/限制见docs |
| P06 FR-18 | [F17 迁移保存与失败](services/parameters-and-tuning.md#f17) | 已知演进/限制见docs |
| P06 TC-1 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P06 TC-2 | [F14 菜单按键与显示](hmi/operation.md#f14) | 已知演进/限制见docs |
| P06 TC-3 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P06 TC-4 | [F16 参数v3格式](services/parameters-and-tuning.md#f16) | 已知演进/限制见docs |
| P06 TC-5 | [F14 菜单按键与显示](hmi/operation.md#f14) | 已知演进/限制见docs |
| P06 TC-6 | [F14 菜单按键与显示](hmi/operation.md#f14) | 已知演进/限制见docs |

### P07

[阶段需求](../specs/features/07-datalog/requirements.md) → 以下条目。当前实现和历史演进差异由功能页说明，验收状态以阶段记录为准。

| 需求/约束 | docs主条目 | 状态边界 |
|---|---|---|
| P07 FR-1 | [F19 日志格式容量](services/recording-and-export.md#f19) | 已知演进/限制见docs |
| P07 FR-2 | [F19 日志格式容量](services/recording-and-export.md#f19) | 已知演进/限制见docs |
| P07 FR-3 | [F20 队列前台写入与故障](services/recording-and-export.md#f20) | 已知演进/限制见docs |
| P07 FR-4 | [F20 队列前台写入与故障](services/recording-and-export.md#f20) | 已知演进/限制见docs |
| P07 FR-5 | [F20 队列前台写入与故障](services/recording-and-export.md#f20) | 已知演进/限制见docs |
| P07 FR-6 | [F19 日志格式容量](services/recording-and-export.md#f19) | 已知演进/限制见docs |
| P07 FR-7 | [F21 CSV与会话边界](services/recording-and-export.md#f21) | 已知演进/限制见docs |
| P07 FR-8 | [F21 CSV与会话边界](services/recording-and-export.md#f21) | 已知演进/限制见docs |
| P07 FR-9 | [F20 队列前台写入与故障](services/recording-and-export.md#f20) | 已知演进/限制见docs |
| P07 TC-1 | [F16 参数v3格式](services/parameters-and-tuning.md#f16) | 已知演进/限制见docs |
| P07 TC-2 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P07 TC-3 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P07 TC-4 | [F19 日志格式容量](services/recording-and-export.md#f19) | 已知演进/限制见docs |
| P07 TC-5 | [F19 日志格式容量](services/recording-and-export.md#f19) | 已知演进/限制见docs |

### P08

[阶段需求](../specs/features/08-storage-chain-fix/requirements.md) → 以下条目。当前实现和历史演进差异由功能页说明，验收状态以阶段记录为准。

| 需求/约束 | docs主条目 | 状态边界 |
|---|---|---|
| P08 FR-1 | [F17 迁移保存与失败](services/parameters-and-tuning.md#f17) | 已知演进/限制见docs |
| P08 FR-2 | [F17 迁移保存与失败](services/parameters-and-tuning.md#f17) | 已知演进/限制见docs |
| P08 FR-3 | [F17 迁移保存与失败](services/parameters-and-tuning.md#f17) | 已知演进/限制见docs |
| P08 FR-4 | [F17 迁移保存与失败](services/parameters-and-tuning.md#f17) | 已知演进/限制见docs |
| P08 FR-5 | [F20 队列前台写入与故障](services/recording-and-export.md#f20) | 已知演进/限制见docs |
| P08 FR-6 | [F16 参数v3格式](services/parameters-and-tuning.md#f16) | 已知演进/限制见docs |
| P08 FR-7 | [F15 多入口参数应用](services/parameters-and-tuning.md#f15) | 冲突待决定 |
| P08 FR-8 | [F14 菜单按键与显示](hmi/operation.md#f14) | 已知演进/限制见docs |

### P09

[阶段需求](../specs/features/09-element-completion/requirements.md) → 以下条目。规划/待决定，不表示已有活动实现。

| 需求/约束 | docs主条目 | 状态边界 |
|---|---|---|
| P09 FR-1 | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 规划/待决定 |
| P09 FR-2 | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 规划/待决定 |
| P09 FR-3 | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 规划/待决定 |
| P09 FR-4 | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 规划/待决定 |
| P09 FR-5 | [F09 活动电磁元素](control/elements-and-dormant-code.md#f09) | 规划/待决定 |
| P09 FR-6 | [F19 日志格式容量](services/recording-and-export.md#f19) | 规划/待决定 |
| P09 TC-1 | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 规划/待决定 |
| P09 TC-2 | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 规划/待决定 |
| P09 TC-3 | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 规划/待决定 |
| P09 TC-4 | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 规划/待决定 |

### P10

[阶段需求](../specs/features/10-control-consistency/requirements.md) → 以下条目。规划/待决定，不表示已有活动实现。

| 需求/约束 | docs主条目 | 状态边界 |
|---|---|---|
| P10 FR-1 | [F12 部分配置与全局调参规划](control/tracking-and-controllers.md#f12) | 规划/待决定 |
| P10 FR-2 | [F12 部分配置与全局调参规划](control/tracking-and-controllers.md#f12) | 规划/待决定 |
| P10 FR-3 | [F12 部分配置与全局调参规划](control/tracking-and-controllers.md#f12) | 规划/待决定 |
| P10 FR-4 | [F12 部分配置与全局调参规划](control/tracking-and-controllers.md#f12) | 规划/待决定 |
| P10 FR-5 | [F12 部分配置与全局调参规划](control/tracking-and-controllers.md#f12) | 规划/待决定 |
| P10 TC-1 | [F12 部分配置与全局调参规划](control/tracking-and-controllers.md#f12) | 规划/待决定 |
| P10 TC-2 | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 规划/待决定 |
| P10 TC-3 | [F16 参数v3格式](services/parameters-and-tuning.md#f16) | 规划/待决定 |
| P10 TC-4 | [F12 部分配置与全局调参规划](control/tracking-and-controllers.md#f12) | 规划/待决定 |
| P10 TC-5 | [F12 部分配置与全局调参规划](control/tracking-and-controllers.md#f12) | 规划/待决定 |

### P11

[阶段需求](../specs/features/11-hardening-cleanup/requirements.md) → 以下条目。当前实现和历史演进差异由功能页说明，验收状态以阶段记录为准。

| 需求/约束 | docs主条目 | 状态边界 |
|---|---|---|
| P11 FR-1 | [F17 迁移保存与失败](services/parameters-and-tuning.md#f17) | 已知演进/限制见docs |
| P11 FR-2 | [F17 迁移保存与失败](services/parameters-and-tuning.md#f17) | 已知演进/限制见docs |
| P11 FR-3 | [F05 电感偏差与对称度](sensing/sensors-and-calibration.md#f05) | 已知演进/限制见docs |
| P11 FR-4 | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 已知演进/限制见docs |
| P11 FR-5 | [F15 多入口参数应用](services/parameters-and-tuning.md#f15) | 已知演进/限制见docs |
| P11 FR-6 | [F19 日志格式容量](services/recording-and-export.md#f19) | 已知演进/限制见docs |
| P11 FR-7 | [F32 标准R08与维护](guides/validation.md#f32) | 已知演进/限制见docs |
| P11 FR-8 | [F14 菜单按键与显示](hmi/operation.md#f14) | 已知演进/限制见docs |
| P11 FR-9 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P11 FR-10 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P11 FR-11 | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 已知演进/限制见docs |
| P11 TC-1 | [F17 迁移保存与失败](services/parameters-and-tuning.md#f17) | 已知演进/限制见docs |
| P11 TC-2 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P11 TC-3 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P11 TC-4 | [F32 标准R08与维护](guides/validation.md#f32) | 已知演进/限制见docs |

### P12

[阶段需求](../specs/features/12-control-safety-closure/requirements.md) → 以下条目。当前实现和历史演进差异由功能页说明，验收状态以阶段记录为准。

| 需求/约束 | docs主条目 | 状态边界 |
|---|---|---|
| P12 FR-1 | [F08 方向角度速度与执行](control/tracking-and-controllers.md#f08) | 已知演进/限制见docs |
| P12 FR-2 | [F02 起停、模式与首因](system/runtime-and-safety.md#f02) | 冲突待决定 |
| P12 FR-3 | [F02 起停、模式与首因](system/runtime-and-safety.md#f02) | 已知演进/限制见docs |
| P12 FR-4 | [F02 起停、模式与首因](system/runtime-and-safety.md#f02) | 冲突待决定 |
| P12 FR-5 | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 已知演进/限制见docs |
| P12 FR-6 | [F02 起停、模式与首因](system/runtime-and-safety.md#f02) | 已知演进/限制见docs |
| P12 FR-7 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P12 TC-1 | [F08 方向角度速度与执行](control/tracking-and-controllers.md#f08) | 已知演进/限制见docs |
| P12 TC-2 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P12 TC-3 | [F08 方向角度速度与执行](control/tracking-and-controllers.md#f08) | 已知演进/限制见docs |
| P12 TC-4 | [F16 参数v3格式](services/parameters-and-tuning.md#f16) | 已知演进/限制见docs |
| P12 TC-5 | [F32 标准R08与维护](guides/validation.md#f32) | 已知演进/限制见docs |

### P13

[阶段需求](../specs/features/13-control-structure/requirements.md) → 以下条目。当前实现和历史演进差异由功能页说明，验收状态以阶段记录为准。

| 需求/约束 | docs主条目 | 状态边界 |
|---|---|---|
| P13 FR-1 | [F05 电感偏差与对称度](sensing/sensors-and-calibration.md#f05) | 已知演进/限制见docs |
| P13 FR-2 | [F08 方向角度速度与执行](control/tracking-and-controllers.md#f08) | 已知演进/限制见docs |
| P13 FR-3 | [F07 IMU校准滤波与积分](sensing/sensors-and-calibration.md#f07) | 已知演进/限制见docs |
| P13 FR-4 | [F08 方向角度速度与执行](control/tracking-and-controllers.md#f08) | 已知演进/限制见docs |
| P13 FR-5 | [F08 方向角度速度与执行](control/tracking-and-controllers.md#f08) | 已知演进/限制见docs |
| P13 FR-6 | [F12 部分配置与全局调参规划](control/tracking-and-controllers.md#f12) | 已知演进/限制见docs |
| P13 FR-7 | [F16 参数v3格式](services/parameters-and-tuning.md#f16) | 已知演进/限制见docs |
| P13 FR-8 | [F08 方向角度速度与执行](control/tracking-and-controllers.md#f08) | 已知演进/限制见docs |
| P13 TC-1 | [F32 标准R08与维护](guides/validation.md#f32) | 已知演进/限制见docs |
| P13 TC-2 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P13 TC-3 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P13 TC-4 | [F05 电感偏差与对称度](sensing/sensors-and-calibration.md#f05) | 已知演进/限制见docs |
| P13 TC-5 | [F05 电感偏差与对称度](sensing/sensors-and-calibration.md#f05) | 已知演进/限制见docs |

### P14

[阶段需求](../specs/features/14-dual-mode-tracking/requirements.md) → 以下条目。当前实现和历史演进差异由功能页说明，验收状态以阶段记录为准。

| 需求/约束 | docs主条目 | 状态边界 |
|---|---|---|
| P14 FR-01 | [F06 灰度采集标定与质心](sensing/sensors-and-calibration.md#f06) | 已知演进/限制见docs |
| P14 FR-02 | [F06 灰度采集标定与质心](sensing/sensors-and-calibration.md#f06) | 已知演进/限制见docs |
| P14 FR-03 | [F06 灰度采集标定与质心](sensing/sensors-and-calibration.md#f06) | 已知演进/限制见docs |
| P14 FR-04 | [F06 灰度采集标定与质心](sensing/sensors-and-calibration.md#f06) | 已知演进/限制见docs |
| P14 FR-05 | [F06 灰度采集标定与质心](sensing/sensors-and-calibration.md#f06) | 已知演进/限制见docs |
| P14 FR-06 | [F06 灰度采集标定与质心](sensing/sensors-and-calibration.md#f06) | 已知演进/限制见docs |
| P14 FR-07 | [F11 双FSM与唯一执行](control/tracking-and-controllers.md#f11) | 已知演进/限制见docs |
| P14 FR-08 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P14 FR-09 | [F10 光电状态机](control/tracking-and-controllers.md#f10) | 已知演进/限制见docs |
| P14 FR-10 | [F09 活动电磁元素](control/elements-and-dormant-code.md#f09) | 已知演进/限制见docs |
| P14 FR-11 | [F11 双FSM与唯一执行](control/tracking-and-controllers.md#f11) | 已知演进/限制见docs |
| P14 FR-12 | [F16 参数v3格式](services/parameters-and-tuning.md#f16) | 已知演进/限制见docs |
| P14 FR-13 | [F02 起停、模式与首因](system/runtime-and-safety.md#f02) | 已知演进/限制见docs |
| P14 FR-14 | [F14 菜单按键与显示](hmi/operation.md#f14) | 已知演进/限制见docs |
| P14 FR-15 | [F16 参数v3格式](services/parameters-and-tuning.md#f16) | 已知演进/限制见docs |
| P14 FR-16 | [F02 起停、模式与首因](system/runtime-and-safety.md#f02) | 已知演进/限制见docs |
| P14 FR-17 | [F19 日志格式容量](services/recording-and-export.md#f19) | 已知演进/限制见docs |
| P14 FR-18 | [F20 队列前台写入与故障](services/recording-and-export.md#f20) | 已知演进/限制见docs |
| P14 FR-19 | [F20 队列前台写入与故障](services/recording-and-export.md#f20) | 已知演进/限制见docs |
| P14 FR-20 | [F20 队列前台写入与故障](services/recording-and-export.md#f20) | 已知演进/限制见docs |
| P14 FR-21 | [F21 CSV与会话边界](services/recording-and-export.md#f21) | 已知演进/限制见docs |
| P14 FR-22 | [F32 标准R08与维护](guides/validation.md#f32) | 已知演进/限制见docs |

### P15

[阶段需求](../specs/features/15-code-style-consistency/requirements.md) → 以下条目。当前实现和历史演进差异由功能页说明，验收状态以阶段记录为准。

| 需求/约束 | docs主条目 | 状态边界 |
|---|---|---|
| P15 FR-01 | [F32 标准R08与维护](guides/validation.md#f32) | 已知演进/限制见docs |
| P15 FR-02 | [F32 标准R08与维护](guides/validation.md#f32) | 已知演进/限制见docs |
| P15 FR-03 | [F32 标准R08与维护](guides/validation.md#f32) | 已知演进/限制见docs |
| P15 FR-04 | [F32 标准R08与维护](guides/validation.md#f32) | 已知演进/限制见docs |
| P15 FR-05 | [F32 标准R08与维护](guides/validation.md#f32) | 已知演进/限制见docs |
| P15 FR-06 | [F06 灰度采集标定与质心](sensing/sensors-and-calibration.md#f06) | 已知演进/限制见docs |
| P15 FR-07 | [F02 起停、模式与首因](system/runtime-and-safety.md#f02) | 已知演进/限制见docs |
| P15 FR-08 | [F16 参数v3格式](services/parameters-and-tuning.md#f16) | 已知演进/限制见docs |
| P15 FR-09 | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 已知演进/限制见docs |
| P15 FR-10 | [F32 标准R08与维护](guides/validation.md#f32) | 已知演进/限制见docs |
| P15 FR-11 | [F32 标准R08与维护](guides/validation.md#f32) | 已知演进/限制见docs |
| P15 FR-12 | [F32 标准R08与维护](guides/validation.md#f32) | 已知演进/限制见docs |
| P15 FR-13 | [F32 标准R08与维护](guides/validation.md#f32) | 已知演进/限制见docs |
| P15 FR-14 | [F32 标准R08与维护](guides/validation.md#f32) | 已知演进/限制见docs |
| P15 FR-15 | [F32 标准R08与维护](guides/validation.md#f32) | 已知演进/限制见docs |
| P15 FR-16 | [F32 标准R08与维护](guides/validation.md#f32) | 已知演进/限制见docs |
| P15 FR-17 | [F32 标准R08与维护](guides/validation.md#f32) | 已知演进/限制见docs |
| P15 FR-18 | [F32 标准R08与维护](guides/validation.md#f32) | 已知演进/限制见docs |
| P15 FR-19 | [F32 标准R08与维护](guides/validation.md#f32) | 已知演进/限制见docs |
| P15 FR-20 | [F32 标准R08与维护](guides/validation.md#f32) | 已知演进/限制见docs |
| P15 FR-21 | [F32 标准R08与维护](guides/validation.md#f32) | 已知演进/限制见docs |
| P15 FR-22 | [F32 标准R08与维护](guides/validation.md#f32) | 已知演进/限制见docs |
| P15 FR-23 | [F32 标准R08与维护](guides/validation.md#f32) | 已知演进/限制见docs |
| P15 FR-24 | [F32 标准R08与维护](guides/validation.md#f32) | 已知演进/限制见docs |
| P15 FR-25 | [F32 标准R08与维护](guides/validation.md#f32) | 已知演进/限制见docs |

### P16

[阶段需求](../specs/features/16-spi-wifi-link/requirements.md) → 以下条目。当前实现和历史演进差异由功能页说明，验收状态以阶段记录为准。

| 需求/约束 | docs主条目 | 状态边界 |
|---|---|---|
| P16 FR-01 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P16 FR-02 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P16 FR-03 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P16 FR-04 | [F29 时钟DMA采样与驱动](platform/board-and-bsp.md#f29) | 已知演进/限制见docs |
| P16 FR-05 | [F23 SCB帧JSON与命令](services/communication.md#f23) | 已知演进/限制见docs |
| P16 FR-06 | [F24 会话可靠传输与回绕](services/communication.md#f24) | 已知演进/限制见docs |
| P16 FR-07 | [F24 会话可靠传输与回绕](services/communication.md#f24) | 已知演进/限制见docs |
| P16 FR-08 | [F24 会话可靠传输与回绕](services/communication.md#f24) | 已知演进/限制见docs |
| P16 FR-09 | [F23 SCB帧JSON与命令](services/communication.md#f23) | 已知演进/限制见docs |
| P16 FR-10 | [F25 独立上传](services/communication.md#f25) | 已知演进/限制见docs |
| P16 FR-11 | [F25 独立上传](services/communication.md#f25) | 已知演进/限制见docs |
| P16 FR-12 | [F25 独立上传](services/communication.md#f25) | 已知演进/限制见docs |
| P16 FR-13 | [F18 完整拍临时调参](services/parameters-and-tuning.md#f18) | 已知演进/限制见docs |
| P16 FR-14 | [F18 完整拍临时调参](services/parameters-and-tuning.md#f18) | 已知演进/限制见docs |
| P16 FR-15 | [F18 完整拍临时调参](services/parameters-and-tuning.md#f18) | 已知演进/限制见docs |
| P16 FR-16 | [F18 完整拍临时调参](services/parameters-and-tuning.md#f18) | 已知演进/限制见docs |
| P16 FR-17 | [F18 完整拍临时调参](services/parameters-and-tuning.md#f18) | 已知演进/限制见docs |
| P16 FR-18 | [F18 完整拍临时调参](services/parameters-and-tuning.md#f18) | 已知演进/限制见docs |
| P16 FR-19 | [F22 二进制流式导出](services/recording-and-export.md#f22) | 已知演进/限制见docs |
| P16 FR-20 | [F22 二进制流式导出](services/recording-and-export.md#f22) | 已知演进/限制见docs |
| P16 FR-21 | [F22 二进制流式导出](services/recording-and-export.md#f22) | 已知演进/限制见docs |
| P16 FR-22 | [F24 会话可靠传输与回绕](services/communication.md#f24) | 已知演进/限制见docs |
| P16 FR-23 | [F30 构建烧录与再生](guides/build-and-flash.md#f30) | 已知演进/限制见docs |
| P16 FR-24 | [F31 验证证据与复现](guides/validation.md#f31) | 已知演进/限制见docs |

### P17

[阶段需求](../specs/features/17-app-architecture/requirements.md) → 以下条目。当前实现和历史演进差异由功能页说明，验收状态以阶段记录为准。

| 需求/约束 | docs主条目 | 状态边界 |
|---|---|---|
| P17 FR-01 | [F32 标准R08与维护](guides/validation.md#f32) | 已知演进/限制见docs |
| P17 FR-02 | [F32 标准R08与维护](guides/validation.md#f32) | 已知演进/限制见docs |
| P17 FR-03 | [F30 构建烧录与再生](guides/build-and-flash.md#f30) | 已知演进/限制见docs |
| P17 FR-04 | [F01 初始化与三线调度](system/runtime-and-safety.md#f01) | 已知演进/限制见docs |
| P17 FR-05 | [F05 电感偏差与对称度](sensing/sensors-and-calibration.md#f05) | 已知演进/限制见docs |
| P17 FR-06 | [F08 方向角度速度与执行](control/tracking-and-controllers.md#f08) | 已知演进/限制见docs |
| P17 FR-07 | [F18 完整拍临时调参](services/parameters-and-tuning.md#f18) | 已知演进/限制见docs |
| P17 FR-08 | [F20 队列前台写入与故障](services/recording-and-export.md#f20) | 已知演进/限制见docs |
| P17 FR-09 | [F23 SCB帧JSON与命令](services/communication.md#f23) | 已知演进/限制见docs |
| P17 FR-10 | [F13 legacy与未接入元素](control/elements-and-dormant-code.md#f13) | 已知演进/限制见docs |
| P17 FR-11 | [F11 双FSM与唯一执行](control/tracking-and-controllers.md#f11) | 已知演进/限制见docs |
| P17 FR-12 | [F11 双FSM与唯一执行](control/tracking-and-controllers.md#f11) | 已知演进/限制见docs |
| P17 FR-13 | [F32 标准R08与维护](guides/validation.md#f32) | 已知演进/限制见docs |

## 后续维护

新增/调整需求先增加Phase→docs绑定，功能页写明目标“待实现”和验证，然后纯规约提交；实现完成后更新docs事实和阶段记录。不能用索引登记完整推定软件或硬件全部通过。
