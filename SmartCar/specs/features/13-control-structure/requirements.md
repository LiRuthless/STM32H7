# Phase 13: 控制链结构重构 — 需求

2026-10-31文档迁移：**Phase → docs → 代码**。本页保留需求/任务/验收历史，不新增阶段、不修改实现或重新验收。

当前需求导航：[P13功能索引](../../../Docs/traceability.md#p13)。具体输入输出、状态/错误/时序、代码符号与验证方法由下列docs条目唯一维护。

## 需求意图与现行功能条目

编号身份及原用户决定保留；摘要不替代详细契约。涉及冲突时，以docs中并列的当前事实、既有要求和待决定边界阅读，不能自动选代码或旧文档为新设计。

<a id="fr-01"></a>
### FR-1 电感owner

目标与实现依据：[F05 电感偏差与对称度](../../../Docs/sensing/sensors-and-calibration.md#f05)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-02"></a>
### FR-2 电机owner

目标与实现依据：[F08 方向角度速度与执行](../../../Docs/control/tracking-and-controllers.md#f08)；[F15 多入口参数应用](../../../Docs/services/parameters-and-tuning.md#f15)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-03"></a>
### FR-3 IMU owner

目标与实现依据：[F07 IMU校准滤波与积分](../../../Docs/sensing/sensors-and-calibration.md#f07)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-04"></a>
### FR-4 PID owner

目标与实现依据：[F08 方向角度速度与执行](../../../Docs/control/tracking-and-controllers.md#f08)；[F12 部分配置与全局调参规划](../../../Docs/control/tracking-and-controllers.md#f12)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-05"></a>
### FR-5 独立速度PI

目标与实现依据：[F08 方向角度速度与执行](../../../Docs/control/tracking-and-controllers.md#f08)；[F13 legacy与未接入元素](../../../Docs/control/elements-and-dormant-code.md#f13)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-06"></a>
### FR-6 部分配置

目标与实现依据：[F12 部分配置与全局调参规划](../../../Docs/control/tracking-and-controllers.md#f12)；[F09 活动电磁元素](../../../Docs/control/elements-and-dormant-code.md#f09)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-07"></a>
### FR-7 调用与布局兼容

目标与实现依据：[F16 参数v3格式](../../../Docs/services/parameters-and-tuning.md#f16)；[F14 菜单按键与显示](../../../Docs/hmi/operation.md#f14)；[F15 多入口参数应用](../../../Docs/services/parameters-and-tuning.md#f15)；[F19 日志格式容量](../../../Docs/services/recording-and-export.md#f19)；[F21 CSV与会话边界](../../../Docs/services/recording-and-export.md#f21)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="fr-08"></a>
### FR-8 旧内部命名迁移

目标与实现依据：[F08 方向角度速度与执行](../../../Docs/control/tracking-and-controllers.md#f08)；[F32 标准R08与维护](../../../Docs/guides/validation.md#f32)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="tc-01"></a>
### TC-1 控制结构兼容边界（第1项）

目标与实现依据：[F08 方向角度速度与执行](../../../Docs/control/tracking-and-controllers.md#f08)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="tc-02"></a>
### TC-2 控制结构兼容边界（第2项）

目标与实现依据：[F01 初始化与三线调度](../../../Docs/system/runtime-and-safety.md#f01)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="tc-03"></a>
### TC-3 控制结构兼容边界（第3项）

目标与实现依据：[F13 legacy与未接入元素](../../../Docs/control/elements-and-dormant-code.md#f13)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="tc-04"></a>
### TC-4 控制结构兼容边界（第4项）

目标与实现依据：[F31 验证证据与复现](../../../Docs/guides/validation.md#f31)。状态：当前契约及历史替代见docs；不改变原验收状态。

<a id="tc-05"></a>
### TC-5 控制结构兼容边界（第5项）

目标与实现依据：[F08 方向角度速度与执行](../../../Docs/control/tracking-and-controllers.md#f08)。状态：当前契约及历史替代见docs；不改变原验收状态。

## 授权与完成边界

本轮仅迁移说明。历史用户决定、动机、来源、兼容性及未决事项完整保留在下方基线；新的行为决定须明确日期、理由和适用范围，更新本需求及docs目标后独立提交，再进入实现。不得用本次文档完成替代AC5、R07原样本或任何硬件待验。

## 冻结历史基线（68cb508，非现行契约）

以下原文用于保留阶段决定、组顺序、需求历史与真实验证；不继续独立维护算法/布局/源码映射。当前契约先查上方docs条目。旧“本轮”“待执行”“当前”只指原记录时点，不覆盖后续决定。历史任务的全量软件门禁不自动成为以后每次修改的要求。

<details>
<summary>展开原始阶段记录（原文保留）</summary>

# Phase 13: 控制链结构重构 — 需求

> **2026-10-16 命名决定更新**：用户确认以新长期标准覆盖 FR-8 对 `PID_speed` 的历史命名。
> [Phase 15 符号表](../15-code-style-consistency/symbol-map.md) 现行目标为
> `PID_UpdateSpeed`，静态属性、签名及计算保持。当前代码已在 Phase 15 G2 统一为 `PID_UpdateSpeed`；
> 下文 FR-8 和历史验证作为已完成工作的证据保留，实车待验状态不变。

> **2026-10-16 增量状态**：[Phase 14](../14-dual-mode-tracking/requirements.md) FR-09～FR-15/17～FR-21 已在静态私有上下文/只读接口原则下，扩展两套状态和方向历史、88B v3参数及日志v2。下文52/56B、32B日志和接口保持不变是Phase13历史验收边界；Phase14明确替代这些目标，不追溯修改Phase13验证结果。电磁公式、速度PI及每拍配置顺序仍须回归，见 [Phase 14 计划](../14-dual-mode-tracking/plan.md) / [验证](../14-dual-mode-tracking/validation.md)。 2026-10-16实现状态及V-01～15通过证据见上述Phase14验证页，V-16～20仍待上机。

> 2026-10-03 用户决定：仅修改 `SmartCar`；采用模块静态结构体与只读状态指针；包含状态机重复赋值，但保持每拍覆写及全部现有运行行为。无实车条件，实车验收保持待执行。

## 行为需求

- **FR-1 电感状态**：`track_sensor` 用唯一静态 `track_state_t` 保存原始值、滤波值、偏差、对称度和四个权重。`Track_GetState()` 返回生命周期覆盖整个程序的非空 `const track_state_t *`，`Track_SetWeights(const track_weights_t *)` 按横差、横和、竖差、竖差绝对值顺序写入权重。`Track_ReadAdc`、`Track_UpdateError`、`Track_UpdateSymmetry` 的采样次数、算式、整数截断与分母为零返回值不变；`Track_UpdateError` 同时保存返回的偏差。
- **FR-2 电机状态**：`motor` 用唯一静态 `motor_state_t` 保存左右轮目标/实测速度与里程、平均里程、基础速度、运行/空闲风扇值及滤波系数。`Motor_GetState()` 返回非空只读指针；`Motor_SetTargets`、`Motor_SetBaseSpeed`、`Motor_SetFanDuty`、`Motor_SetFanDutyIdle`、`Motor_ResetDistance` 是跨模块写入口，赋值时不新增限幅或校验。差速、编码器低通、方向极性、PWM、停车清零及再次起跑复位保持现状。
- **FR-3 IMU 状态**：`imu_proc` 用唯一静态 `imu_state_t` 保存三轴原始值、换算值、速度与角度；零偏、梯形积分历史、高通滤波器均为模块私有。`IMU_GetState()` 返回非空只读指针，`IMU_ZeroAngleX()` 执行原有 `angle_x=0` 的单字段清零。校准、读数、积分、复位的计算顺序和系数不变；休眠的 `IMU_UpdateAccelVelocity` 保留且不接入。
- **FR-4 PID 状态**：`pid` 用唯一静态 `pid_state_t` 保存原有增益和可观测输出；`PID_GetState()` 返回非空只读指针，`PID_SetGain` 按指定增益单字段赋值，`PID_SetTrackGains`/`PID_SetAngleGains` 按旧代码的字段顺序连续赋值，不增加校验。方向、角度公式及现有输出类型转换与限幅顺序不变。
- **FR-5 双轮 PI 去重**：活动的 `PID_UpdateSpeedLeft` 与 `PID_UpdateSpeedRight` 保留无参入口，各自调用同一个接收独立 `pid_speed_pi_state_t *`、目标、反馈、死区及输出位置的内部函数。左右积分、上次目标及输出互不共享；换向、零目标、积分 ±7000、输出 ±7500、死区 ±500、`PID_ResetSpeed`/`PID_ResetAll` 的逐拍结果不变。休眠的增量式 `PID_UpdateIncrementalLeft/R` 保留且不接入或删除。
- **FR-6 状态配置表**：`control.c` 七个主状态和 `roundabout.c` 四个现有分支使用静态常量配置表。每条记录含字段存在标志，仅在原分支原位置写入旧代码实际赋值的权重、基础速度、方向或角度增益；未赋值字段保持前一拍值。主状态先应用本态配置，再进入环岛子状态配置，随后执行原有计算与判断。数值、判断条件、状态转移、十字限幅和每拍覆写调参值的语义不变。
- **FR-7 调用方与持久化兼容**：`app/control/Roundabout_Update/element/Menu_Update/wireless/param/dashboard/datalog` 通过新接口访问上述状态，不保留对应散落的 `extern` 定义。菜单步进、串口协议与回显、参数镜像 v1/v2 的 52/56 字节布局和 CRC 范围、32 字节日志记录与 CSV 字段顺序不变。
- **FR-8 速度 PI 内部函数命名**：将 `App/pid.c` 中的私有函数 `speed_pi_step` 改名为 `PID_speed`，同步其在 `PID_UpdateSpeedLeft`、`PID_UpdateSpeedRight` 中的两个调用。函数保持 `static`，参数、返回类型、计算顺序、状态读写、错误路径及逐拍控制输出不变；不增加公共声明。其他函数和结构体的命名方案由用户另行选择，本项不改动它们。

## 约束与边界

- **TC-1**：C99，静态分配；状态指针仅作只读访问，不返回可写内部指针，不缓存指向临时对象的指针。
- **TC-2**：TIM6 2 ms、TIM15/TIM6 优先级及同级不抢占、TIM7 5 ms、BSP 接口与硬件配置不变；App 不新增直接 HAL 依赖。
- **TC-3**：Phase 10 再决定状态机与双档调参的生效优先级；Phase 09/11 再决定未启用元素和休眠代码去留。本轮不借结构重构修复其他已排期行为缺陷。
- **TC-4**：自动化验证可判定代码与数值等价；本轮无实车，所有需实车验证项在 `validation.md` 标为待上机，Phase 13 不勾选完成。
- **TC-5**：getter 均无参数、返回地址固定且非空；接受指针的写接口仅由静态配置对象调用，前置条件为指针非空。写接口返回 `void` 且不新增拒绝路径；`PID_SetGain` 的未知枚举值不写任何增益。速度与里程仍为编码器计数/2 ms、风扇值为 0～10000 万分比、IMU 角速度为 °/s、角度为 °，各字段沿用旧类型与默认值。

> 2026-10-16 Phase 15 G2：现行传感器/控制符号按[完整映射](../15-code-style-consistency/symbol-map.md)迁移，
> 配置、计算顺序及休眠调用边保持；历史 validation 原文及本阶段实车门禁不变。

> 2026-10-16 Phase 15 G3：现行调度、HMI、参数和日志名称已按[符号表](../15-code-style-consistency/symbol-map.md)统一。
> 本文原阶段行为记录与未完成待办不因此重新验收；现行运行链以 Phase 14 和当前头文件契约为准，历史 validation 保留原文。

## Phase 17 当前位置（2026-10-23）

App按system/sensing/control/services/hmi归类。现行文件及逐函数位置见[迁移清单](../17-app-architecture/migration-map.md)。
历史路径、符号与验证结果保留；本说明不表示后续职责拆分或硬件验收完成。

</details>
