# Phase 13: 控制链结构重构 — 验证

2026-10-31文档迁移：**Phase → docs → 代码**。本页保留需求/任务/验收历史，不新增阶段、不修改实现或重新验收。

当前需求导航：[P13功能索引](../../../Docs/traceability.md#p13)。具体输入输出、状态/错误/时序、代码符号与验证方法由下列docs条目唯一维护。

## 当前验收方法入口

阶段完成标准：本阶段适用需求的docs契约可定位实现且其验证判据有实际证据；仍未实施/待决定/待上机的项目继续保持该状态。本轮只检查文档迁移，不新增固件通过结论。原V编号、通过标准、结果、提交号、日志和未执行项完整保留在下方。

| 需求 | 当前方法/判据/实现 | 结果读取方式 |
|---|---|---|
| FR-1 | [F05 电感偏差与对称度](../../../Docs/sensing/sensors-and-calibration.md#f05) | 历史实际结果见下方；限制/当前方法见docs |
| FR-2 | [F08 方向角度速度与执行](../../../Docs/control/tracking-and-controllers.md#f08)；[F15 多入口参数应用](../../../Docs/services/parameters-and-tuning.md#f15) | 历史实际结果见下方；限制/当前方法见docs |
| FR-3 | [F07 IMU校准滤波与积分](../../../Docs/sensing/sensors-and-calibration.md#f07) | 历史实际结果见下方；限制/当前方法见docs |
| FR-4 | [F08 方向角度速度与执行](../../../Docs/control/tracking-and-controllers.md#f08)；[F12 部分配置与全局调参规划](../../../Docs/control/tracking-and-controllers.md#f12) | 历史实际结果见下方；限制/当前方法见docs |
| FR-5 | [F08 方向角度速度与执行](../../../Docs/control/tracking-and-controllers.md#f08)；[F13 legacy与未接入元素](../../../Docs/control/elements-and-dormant-code.md#f13) | 历史实际结果见下方；限制/当前方法见docs |
| FR-6 | [F12 部分配置与全局调参规划](../../../Docs/control/tracking-and-controllers.md#f12)；[F09 活动电磁元素](../../../Docs/control/elements-and-dormant-code.md#f09) | 历史实际结果见下方；限制/当前方法见docs |
| FR-7 | [F16 参数v3格式](../../../Docs/services/parameters-and-tuning.md#f16)；[F14 菜单按键与显示](../../../Docs/hmi/operation.md#f14)；[F15 多入口参数应用](../../../Docs/services/parameters-and-tuning.md#f15)；[F19 日志格式容量](../../../Docs/services/recording-and-export.md#f19)；[F21 CSV与会话边界](../../../Docs/services/recording-and-export.md#f21) | 历史实际结果见下方；限制/当前方法见docs |
| FR-8 | [F08 方向角度速度与执行](../../../Docs/control/tracking-and-controllers.md#f08)；[F32 标准R08与维护](../../../Docs/guides/validation.md#f32) | 历史实际结果见下方；限制/当前方法见docs |









## 冻结历史基线（68cb508，非现行契约）

以下原文用于保留阶段决定、组顺序、需求历史与真实验证；不继续独立维护算法/布局/源码映射。当前契约先查上方docs条目。旧“本轮”“待执行”“当前”只指原记录时点，不覆盖后续决定。历史任务的全量软件门禁不自动成为以后每次修改的要求。

<details>
<summary>展开原始阶段记录（原文保留）</summary>

# Phase 13: 控制链结构重构 — 验证

## 基线（修改前，2026-10-03）

- 在 `feature/11-hardening-cleanup` 的 `60b9f4a` 上运行 `MDK-GCC/build.sh`：成功，0 警告 0 错误；`text=62288`、`data=236`、`bss=10240` 字节。
- 基线参数镜像 `sizeof(v1)=52`、`sizeof(v2)=56`；日志记录定义 32 字节。控制周期 2 ms；状态机继续每拍覆写指定增益。

## 自动化与代码检查

| 项 | 需求 | 方法 | 通过标准 | 结果 |
|---|---|---|---|---|
| V-1 | FR-1～FR-7 | 每个任务组执行 `MDK-GCC/build.sh` | 编译链接成功，0 警告 | 通过；最终构建结果见下方记录 |
| V-2 | FR-1、FR-2、FR-3、FR-5 | 执行 `python SmartCar/tests/control_equivalence.py`，编译并运行 Phase 11 原始源码与当前源码 | 5 组电感、9 步双轮 PI、2 组差速和 6 步 IMU 输出逐项一致，含零分母、换向、限幅、零目标与复位 | 通过 |
| V-3 | FR-6 | 同一脚本从 Phase 11 原始分支提取赋值，逐态对照当前配置表字段掩码、值和调用位置 | 七个主状态和四个环岛分支写入字段、数值一致 | 通过；调用位置另经代码检查 |
| V-4 | FR-1～FR-4、FR-7 | 搜索旧全局定义/extern；检查所有写入点与 getter 的 `const` 返回类型 | 无遗漏的跨模块直写；只读指针不被强转为可写 | 通过；搜索无匹配 |
| V-5 | FR-7 | 编译期大小/偏移断言及同一脚本对照镜像结构、CRC 函数、日志定义和串口命令集合 | v1=52、v2=56、日志=32 字节；协议、CSV 字段顺序和 2 ms 调度路径不变 | 通过；Core、BSP、定时器和日志格式未改 |
| V-9 | FR-8 | 在 `App/` 搜索 `speed_pi_step`、`PID_speed`；运行 `MDK-GCC/build.sh` 和 `python SmartCar/tests/control_equivalence.py` | 旧名零匹配；新名仅有一个 `static` 定义和两个调用；构建 0 警告 0 错误，等价性脚本通过 | 通过；2026-10-03 执行结果见下方记录 |

## 实车回归（本轮无设备，保持待上机）

| 项 | 通过标准 | 结果 |
|---|---|---|
| V-6 架空轮与复位 | 左右方向、闭环、停车 PWM 立即为零；再次起跑无历史积分残留 | 待上机 |
| V-7 循迹与元素 | 直线/弯道、十字与右环岛状态转移及控制输出与基线一致 | 待上机 |
| V-8 调参与日志 | 双档菜单/串口保存、掉电恢复、32 字节日志导出正常；运行中覆写规则保持现状 | 待上机 |

## 实际执行记录（2026-10-03）

- 初次结构迁移后 GCC 构建：0 警告 0 错误，`text=61828`、`data=284`、`bss=10216`。
- 加入镜像布局断言及日志记录大小断言后 GCC 构建：0 警告 0 错误，`text=61780`、`data=284`、`bss=10216`。
- 最终热路径 getter 合并后 GCC 构建：0 警告 0 错误，`text=61736`、`data=284`、`bss=10216`；相对基线 text 少 552 字节，data 多 48 字节，bss 少 24 字节。此体积差异不代表已实测中断执行时间。
- 等价性脚本通过：5 组电感、9 步 PI、2 组差速、6 步 IMU，七个主状态和四个环岛配置，参数结构/CRC、日志结构及串口命令集合均与 Phase 11 基线一致。
- V-6～V-8 未执行前 Phase 13 不勾选完成，也不声明实车等价。
- V-9：`App/` 中 `speed_pi_step` 零匹配，`PID_speed` 为一个 `static` 定义和两个调用；GCC 构建 0 警告 0 错误（`text=61736`、`data=284`、`bss=10216`）；控制等价性脚本三项检查均通过。

## 流程偏差记录（2026-10-03）

- 提交 `b23aaff` 同时包含 `App/pid.c` 的 FR-8 函数重命名、Phase 13 规约增量及 CHANGELOG。虽然 V-9 的构建与等价性验证通过，该提交未满足“规约先作独立提交，再修改实现”的流程要求；单个提交也无法证明提交前后的文档门禁顺序。
- 保留 `b23aaff` 历史，不把本记录视为事前规约提交，也不据此改变 V-6～V-8 待上机状态。后续固件代码改动按更新后的 `AGENTS.md` 分次提交规约与实现。

</details>
