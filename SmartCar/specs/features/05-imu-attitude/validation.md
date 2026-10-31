# Phase 05: IMU 姿态处理 — 验证

2026-10-31文档迁移：**Phase → docs → 代码**。本页保留需求/任务/验收历史，不新增阶段、不修改实现或重新验收。

当前需求导航：[P05功能索引](../../../docs/traceability.md#p05)。具体输入输出、状态/错误/时序、代码符号与验证方法由下列docs条目唯一维护。

## 当前验收方法入口

阶段完成标准：本阶段适用需求的docs契约可定位实现且其验证判据有实际证据；仍未实施/待决定/待上机的项目继续保持该状态。本轮只检查文档迁移，不新增固件通过结论。原V编号、通过标准、结果、提交号、日志和未执行项完整保留在下方。

| 需求 | 当前方法/判据/实现 | 结果读取方式 |
|---|---|---|
| FR-1 | [F07 IMU校准滤波与积分](../../../docs/sensing/sensors-and-calibration.md#f07) | 历史实际结果见下方；限制/当前方法见docs |
| FR-2 | [F07 IMU校准滤波与积分](../../../docs/sensing/sensors-and-calibration.md#f07) | 历史实际结果见下方；限制/当前方法见docs |
| FR-3 | [F07 IMU校准滤波与积分](../../../docs/sensing/sensors-and-calibration.md#f07) | 历史实际结果见下方；限制/当前方法见docs |
| FR-4 | [F07 IMU校准滤波与积分](../../../docs/sensing/sensors-and-calibration.md#f07) | 历史实际结果见下方；限制/当前方法见docs |
| FR-5 | [F07 IMU校准滤波与积分](../../../docs/sensing/sensors-and-calibration.md#f07) | 历史实际结果见下方；限制/当前方法见docs |
| FR-6 | [F07 IMU校准滤波与积分](../../../docs/sensing/sensors-and-calibration.md#f07) | 历史实际结果见下方；限制/当前方法见docs |
| FR-7 | [F07 IMU校准滤波与积分](../../../docs/sensing/sensors-and-calibration.md#f07) | 历史实际结果见下方；限制/当前方法见docs |
| FR-8 | [F07 IMU校准滤波与积分](../../../docs/sensing/sensors-and-calibration.md#f07) | 历史实际结果见下方；限制/当前方法见docs |

**V-7/V-8观测替代**：当前CSV没有angle_x/accel_x，按docs的调试器与上下文方法观察；本轮未增加日志字段或实测。









## 冻结历史基线（68cb508，非现行契约）

以下原文用于保留阶段决定、组顺序、需求历史与真实验证；不继续独立维护算法/布局/源码映射。当前契约先查上方docs条目。旧“本轮”“待执行”“当前”只指原记录时点，不覆盖后续决定。历史任务的全量软件门禁不自动成为以后每次修改的要求。

<details>
<summary>展开原始阶段记录（原文保留）</summary>

# Phase 05: IMU 姿态处理 — 验证

## 验证清单（Scorecard）

| # | 检查项 | 方法 | 通过标准 | 关联需求 |
|---|---|---|---|---|
| V-1 | 编译零警告 | `MDK-GCC/build.sh` | 构建通过、无 warning | TC-1~TC-5 |
| V-2 | IMU 上电自检 | 上电后看串口 | 无 "IMU660RB init fail" 输出；有则说明 SPI4/接线异常 | FR-1 |
| V-3 | 零偏校准 | 上电保持车辆静止，随后停车状态看 GYRO 监视页（或串口数据） | 静止时 `gyro_x/y/z` 均在 0 附近小幅波动（零偏已扣除） | FR-2、FR-3 |
| V-4 | 角速度换算 | 手动车架绕各轴转动，看 GYRO 页 | `gyro_*` 示数随转动方向变号，量级合理（°/s） | FR-3 |
| V-5 | 高通抗漂移 | 静止放置 ≥60 s，观察 `angle_y` | `angle_y` 不随时间单向漂移（0.2 Hz 高通抑制残余零偏）；`angle_x/z`（无高通）允许缓慢漂移——现状如此 | FR-4、FR-5 |
| V-6 | 角度积分 | 将车绕对应轴转动约 90° 后回正 | `angle_*` 随之变化并在回正后回到 0 附近（梯形积分方向正确） | FR-5 |
| V-7 | `angle_x` 清零约定 | 环岛实车跑行并导出 CSV | 预环岛触发与进右环岛时刻 `angle_x` 归零，环岛段从 0 重新积分 | FR-6 |
| V-8 | accel_x 现状确认 | 导出 CSV 或调试观察 | `accel_x` 全程为 0（休眠接口未调用的如实表现，非故障） | FR-7 |
| V-9 | SPI4 互斥 | 停车刷屏期间与运行期间分别观察 | 停车时屏幕刷新正常；运行时无花屏、IMU 数据无错帧 | TC-3 |

## 自动化验证

- 构建基线（全项目统一）：

  ```bash
  cd SmartCar/MDK-GCC && ./build.sh
  ```

  通过标准：编译链接成功、零警告。

## 人工验证步骤

1. 烧录固件，车辆静置于水平地面，上电——上电瞬间不得触碰车辆（校准无静止检测，晃动会污染零偏）。
2. 看屏幕 GYRO 页：静止时三轴角速度近 0（V-3）。
3. 手动车架绕 z 轴（偏航）快速转动，确认 `gyro_z` 示数变号、量级合理（V-4）。
4. 静置 1 分钟以上，确认 `angle_y` 不单向漂移（V-5）。
5. 按 K1 起跑跑行含右环岛的赛道，停车后串口发 `d` 导出 CSV：检查 `angle_x` 在预环岛/进环时刻清零（V-7）、`accel_x` 恒 0（V-8）。
6. 停车回菜单反复刷屏，确认屏幕无花屏（V-9）。

## 回归检查

- 本阶段任何改动不得破坏：
  - TIM15/TIM6 优先级 0 互不抢占与采样器免锁前提（硬性约束 3）。
  - SPI4 互斥语义：运行期禁 LCD、停车期禁 IMU（[Phase 01](../01-runtime-architecture/validation.md) 运行时结构）。
  - [Phase 03](../03-track-following/validation.md) 对 `accel_x`/`gyro_x` 的引用语义（即便 accel_x 恒 0，也不得在未确认设计前改为有值）。
  - [Phase 04](../04-element-fsm/validation.md) 的 `angle_x` 清零时机与环岛序列（V-5 of Phase 04）。
  - 校准在 `App_Init` 中的位置：必须在 TIM15 采样器启动前完成。

</details>
