# Phase 13: 控制链结构重构 — 实施计划

2026-10-31文档迁移：**Phase → docs → 代码**。本页保留需求/任务/验收历史，不新增阶段、不修改实现或重新验收。

当前需求导航：[P13功能索引](../../../docs/traceability.md#p13)。具体输入输出、状态/错误/时序、代码符号与验证方法由下列docs条目唯一维护。

## 任务分组与详细设计入口

历史分组/依赖/风险/回退和用户决定保留在下方原记录；本次未重开原实现任务。以后执行本阶段增量时，先确定对应需求和docs目标，按原前置及已确认决定分组；实现文件与符号不在本页复制维护。

| 原需求 | 详细契约/实现/验证入口 | 执行边界 |
|---|---|---|
| FR-1 电感owner | [F05 电感偏差与对称度](../../../docs/sensing/sensors-and-calibration.md#f05) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-2 电机owner | [F08 方向角度速度与执行](../../../docs/control/tracking-and-controllers.md#f08)；[F15 多入口参数应用](../../../docs/services/parameters-and-tuning.md#f15) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-3 IMU owner | [F07 IMU校准滤波与积分](../../../docs/sensing/sensors-and-calibration.md#f07) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-4 PID owner | [F08 方向角度速度与执行](../../../docs/control/tracking-and-controllers.md#f08)；[F12 部分配置与全局调参规划](../../../docs/control/tracking-and-controllers.md#f12) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-5 独立速度PI | [F08 方向角度速度与执行](../../../docs/control/tracking-and-controllers.md#f08)；[F13 legacy与未接入元素](../../../docs/control/elements-and-dormant-code.md#f13) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-6 部分配置 | [F12 部分配置与全局调参规划](../../../docs/control/tracking-and-controllers.md#f12)；[F09 活动电磁元素](../../../docs/control/elements-and-dormant-code.md#f09) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-7 调用与布局兼容 | [F16 参数v3格式](../../../docs/services/parameters-and-tuning.md#f16)；[F14 菜单按键与显示](../../../docs/hmi/operation.md#f14)；[F15 多入口参数应用](../../../docs/services/parameters-and-tuning.md#f15)；[F19 日志格式容量](../../../docs/services/recording-and-export.md#f19)；[F21 CSV与会话边界](../../../docs/services/recording-and-export.md#f21) | 当前契约及历史替代见docs；不改变原验收状态 |
| FR-8 旧内部命名迁移 | [F08 方向角度速度与执行](../../../docs/control/tracking-and-controllers.md#f08)；[F32 标准R08与维护](../../../docs/guides/validation.md#f32) | 当前契约及历史替代见docs；不改变原验收状态 |

执行顺序：先读需求→对应docs→检查未决/依赖→纯规约提交→实现→所选验证及记录。回退以该次完整组为单位，同步回退接口/调用/文档，历史兼容和保护边界不借回退改变。纯文档检查不触发旧组软件门禁。









## 冻结历史基线（68cb508，非现行契约）

以下原文用于保留阶段决定、组顺序、需求历史与真实验证；不继续独立维护算法/布局/源码映射。当前契约先查上方docs条目。旧“本轮”“待执行”“当前”只指原记录时点，不覆盖后续决定。历史任务的全量软件门禁不自动成为以后每次修改的要求。

<details>
<summary>展开原始阶段记录（原文保留）</summary>

# Phase 13: 控制链结构重构 — 实施计划

## 顺序与定位

1. 从已提交的 Phase 11 非元素整改 HEAD 派生独立宪章分支，只提交 `roadmap.md` 的 Phase 13 排期；随后派生 `feature/13-control-structure`。先提交本规约三件套及 Phase 03/10/11 重叠说明，再修改代码。
2. **FR-1/2/3/4**：分别在 `track_sensor.c/.h`、`motor.c/.h`、`imu_proc.c/.h`、`pid.c/.h` 定义模块静态状态及只读 getter；在 `filter.c/.h` 将仅由 IMU 使用的高通实例移入 IMU 模块。getter 地址固定、无分配；写接口只改被指定字段，保留旧赋值的即时生效与类型。
3. **FR-5**：`pid.c` 中以两个私有 `pid_speed_pi_state_t` 实例取代活动 PI 的左右历史变量。内部公共计算函数接受状态指针与输入值，左右旧函数作为薄入口；`Motor_EmergencyStop` 与再次起跑复位仍调用现有 PID 复位入口。
4. **FR-6**：`control.h/.c` 声明配置结构及 `Control_ApplyProfile`，字段存在位分别控制权重、基础速度、方向增益、角度增益。`control.c`、`roundabout.c` 各有静态常量表，按旧分支位置调用；不把未写字段补成默认值，也不提前到状态判断之前执行。
5. **FR-7**：迁移 `app.c`、`control.c`、`roundabout.c`、`element.c`、`menu.c`、`wireless.c`、`param.c`、`dashboard.c`、`datalog.c` 的相关读取/写入。`param_store_v1_t`/`param_store_t` 与 `datalog_record_t` 只改取值来源，不改定义或序列化。最后移除旧全局量及其 `extern` 声明，同步 CHANGELOG 和各规约接口描述。
6. **FR-8**：仅在 `App/pid.c` 将 `speed_pi_step` 的定义和 `PID_UpdateSpeedLeft`/`PID_UpdateSpeedRight` 的两处调用机械改为 `PID_speed`。先完成本规约修改，再改代码；以全局搜索旧名、GCC 零警告构建和 Phase 13 控制等价性脚本验证。若输出或构建变化，回退本次命名改动并定位原因。

## 数据流与风险

- 2 ms 控制链仍为采样/IMU → 状态配置 → 偏差/方向 → 差速目标 → 编码器 → 双轮 PI → PWM。TIM15 日志只读模块状态；TIM7 与停车菜单保留各自调用时机。
- 表配置采用字段存在位，是因为 `CONTROL_KERNEL_TEETERBOARD` 不写权重、`CONTROL_KERNEL_ISLAND_L` 不写基础速度、`ROUNDABOUT_ISLAND_IN` 不写权重；完整结构体覆盖会改变沿用上拍值的行为。
- 参数镜像仍是独立磁盘格式，不以运行态结构体直接写 Flash。调用方不持有可写指针，避免意外跨模块修改；对照测试覆盖结构迁移后的数值和复位行为。
- 每组变更后运行 GCC 构建并记录结果；验证失败时只回退该组，先恢复已确认的逐拍值与执行顺序。

> 2026-10-16 Phase 15 G2：现行传感器/控制符号按[完整映射](../15-code-style-consistency/symbol-map.md)迁移，
> 配置、计算顺序及休眠调用边保持；历史 validation 原文及本阶段实车门禁不变。

> 2026-10-16 Phase 15 G3：现行调度、HMI、参数和日志名称已按[符号表](../15-code-style-consistency/symbol-map.md)统一。
> 本文原阶段行为记录与未完成待办不因此重新验收；现行运行链以 Phase 14 和当前头文件契约为准，历史 validation 保留原文。

## Phase 17 当前实现定位（2026-10-24）

control/pid成为增益/可见输出协调，左右PI历史归speed_controller，角度历史归angle_controller，方向纯计算归direction_controller；旧方向预览与休眠增量归legacy_pid，旧角度到电机动作在legacy_control。原PID_UpdateSpeed私有核心现为SpeedController_Update；部分配置归control_profile，参数与双模式历史不复制，窄化/限幅/复位顺序保持。

本节是现行归属导航；上文历史计划、验证、提交路径及未完成实机项目保持。实际重构验证见
[Phase17验收](../17-app-architecture/validation.md)，逐符号迁移见[迁移导航](../17-app-architecture/migration-map.md)。

</details>
