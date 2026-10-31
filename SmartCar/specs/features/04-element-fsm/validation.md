# Phase 04: 赛道元素状态机（现状） — 验证

2026-10-31文档迁移：**Phase → docs → 代码**。本页保留需求/任务/验收历史，不新增阶段、不修改实现或重新验收。

当前需求导航：[P04功能索引](../../../Docs/traceability.md#p04)。具体输入输出、状态/错误/时序、代码符号与验证方法由下列docs条目唯一维护。

## 当前验收方法入口

阶段完成标准：本阶段适用需求的docs契约可定位实现且其验证判据有实际证据；仍未实施/待决定/待上机的项目继续保持该状态。本轮只检查文档迁移，不新增固件通过结论。原V编号、通过标准、结果、提交号、日志和未执行项完整保留在下方。

| 需求 | 当前方法/判据/实现 | 结果读取方式 |
|---|---|---|
| FR-1 | [F09 活动电磁元素](../../../Docs/control/elements-and-dormant-code.md#f09)；[F12 部分配置与全局调参规划](../../../Docs/control/tracking-and-controllers.md#f12)；[F11 双FSM与唯一执行](../../../Docs/control/tracking-and-controllers.md#f11) | 历史实际结果见下方；限制/当前方法见docs |
| FR-2 | [F02 起停、模式与首因](../../../Docs/system/runtime-and-safety.md#f02) | 历史实际结果见下方；限制/当前方法见docs |
| FR-3 | [F09 活动电磁元素](../../../Docs/control/elements-and-dormant-code.md#f09) | 历史实际结果见下方；限制/当前方法见docs |
| FR-4 | [F09 活动电磁元素](../../../Docs/control/elements-and-dormant-code.md#f09) | 历史实际结果见下方；限制/当前方法见docs |
| FR-5 | [F09 活动电磁元素](../../../Docs/control/elements-and-dormant-code.md#f09) | 历史实际结果见下方；限制/当前方法见docs |
| FR-6 | [F09 活动电磁元素](../../../Docs/control/elements-and-dormant-code.md#f09) | 历史实际结果见下方；限制/当前方法见docs |
| FR-7 | [F09 活动电磁元素](../../../Docs/control/elements-and-dormant-code.md#f09)；[F12 部分配置与全局调参规划](../../../Docs/control/tracking-and-controllers.md#f12) | 历史实际结果见下方；限制/当前方法见docs |
| FR-8 | [F09 活动电磁元素](../../../Docs/control/elements-and-dormant-code.md#f09) | 历史实际结果见下方；限制/当前方法见docs |
| FR-9 | [F13 legacy与未接入元素](../../../Docs/control/elements-and-dormant-code.md#f13) | 历史实际结果见下方；限制/当前方法见docs |
| FR-10 | [F09 活动电磁元素](../../../Docs/control/elements-and-dormant-code.md#f09) | 历史实际结果见下方；限制/当前方法见docs |
| FR-11 | [F13 legacy与未接入元素](../../../Docs/control/elements-and-dormant-code.md#f13) | 历史实际结果见下方；限制/当前方法见docs |

**实体前提**：选择电磁模式、填有效实测标定、同时提供灰度与电磁信号，再区分电磁观察与实际控制；默认光电时日志十字不等于执行十字限幅。












## 冻结历史基线（68cb508，非现行契约）

以下原文用于保留阶段决定、组顺序、需求历史与真实验证；不继续独立维护算法/布局/源码映射。当前契约先查上方docs条目。旧“本轮”“待执行”“当前”只指原记录时点，不覆盖后续决定。历史任务的全量软件门禁不自动成为以后每次修改的要求。

<details>
<summary>展开原始阶段记录（原文保留）</summary>

# Phase 04: 赛道元素状态机（现状） — 验证

## 验证清单（Scorecard）

| # | 检查项 | 方法 | 通过标准 | 关联需求 |
|---|---|---|---|---|
| V-1 | 编译零警告 | `MDK-GCC/build.sh` | 构建通过、无 warning | TC-1~TC-5 |
| V-2 | 出赛道保护 | 运行中将车抬离赛道 | 车辆立即停车，屏幕回菜单态，蓝灯由常亮转慢闪 | FR-2 |
| V-3 | 十字通过 | 赛道十字处实车跑行，串口导出走行数据 | 过十字时状态机进 CROSSROADS，方向输出被限在 ±10 内，车辆直行通过不转弯，出十字回 TRACKING | FR-3 |
| V-4 | 预环岛触发 | 环岛前实车跑行，导出数据 | 横两路和 >2800 时置 R_round_flag、里程与 angle_x 清零、状态转 REISLAND | FR-4 |
| V-5 | 右环岛完整通过 | 右环岛实车跑行 | 依次经过 LPREENTER→TURN_LEFT→IN→OUT→NORMAL，车辆完成入环、环内循迹、出环并回到正常循迹，无冲出赛道 | FR-5、FR-7 |
| V-6 | 预环岛超时 | 制造误触发（手遮横电感模拟） | 未进环行驶 >5000 计数后自动回 TRACKING，不卡死在 REISLAND | FR-6 |
| V-7 | 路障抑制 | 预环岛触发同时将障碍物置于 DL1B 前方 <100 mm | cask_flag 置位，REISLAND 态不进 ISLAND_R，超时后回 TRACKING | FR-8 |
| V-8 | 不可达三态确认 | 代码审查 | `teeterboard_judge`/`cask_judge`/`L_roundabout_judge`/`element_judge` 无任何调用方；ISLAND_L/TEETERBOARD/CASK 无入态路径 | FR-9、FR-10 |
| V-9 | 可达路径封闭性 | 代码审查 `kernel_state` 全部写点 | 全部转移仅落在 FR-10 列举的集合内 | FR-10 |
| V-10 | 环岛距离参数 | 实车观察入环/出环时机 | 预入环直行、打角、环内、出环四段时机与 8000/13000/32000/10000 计数量级一致，可在实车上调 `enter_distance1`/`out_distance1` 生效 | FR-7、TC-3 |

## 自动化验证

- 构建基线（全项目统一）：

  ```bash
  cd SmartCar/MDK-GCC && ./build.sh
  ```

  通过标准：编译链接成功、零警告。
- 不可达性与可达路径检查（V-8/V-9）以代码审查执行：检索 `kernel_state` 的写点与 `*_judge` 函数的调用方。

## 人工验证步骤

1. 烧录固件，车辆置于赛道起跑线，上电保持静止完成校准。
2. 按 K1 起跑，先跑**纯循迹段**：确认 TRACKING 态稳定，无误入元素状态。
3. **十字段**（V-3）：车辆直穿十字；停车后串口发 `d` 导出 CSV，检查状态机字段在十字处短暂为 CROSSROADS(4)、方向输出未超 ±10。
4. **右环岛段**（V-4/V-5）：观察入环打角方向（向右）、环内循迹、出环回正；CSV 中依次出现 REISLAND(6)→ISLAND_R(2)→TRACKING(0)，环岛子状态机距离阈值附近转移时机合理。
5. **出赛道保护**（V-2）：运行中抬车，确认立即停车。
6. **误触发恢复**（V-6/V-7）：按上表方法制造预环岛误触发与路障抑制，确认车辆回正常循迹。

## 回归检查

- 本阶段任何改动不得破坏：
  - [Phase 03 循迹链](../03-track-following/validation.md)：正常段循迹、差速、速度闭环行为（V-2~V-7 of Phase 03）。
  - 出赛道保护语义：电感和 ≤300 必须停车（这是安全兜底，禁止放宽）。
  - 起跑延时（`time > 1000` 拍 ≈2 s）与停车回菜单语义。
  - 里程清零接口与环岛子状态机的配合：任何对 `read_encoder`/`Distance` 的改动必须重跑 V-5。
  - [Phase 05](../05-imu-attitude/validation.md) `angle_x` 清零约定：预环岛与进环时的清零时机不得改变。

</details>
