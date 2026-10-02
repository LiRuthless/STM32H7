# Phase 09: 元素功能补齐 — 需求

> 标注"待确认"的条目以 [plan](plan.md) 各 Group 开工前的用户确认结论为准，确认后回填本条目的具体数值。

## 功能需求

- FR-1: kernel 主状态机 7 个状态均可达：`KERNEL_ISLAND_L`、`KERNEL_TEETERBOARD`、`KERNEL_CASK` 存在明确的进入与离开路径，不再有死态。
- FR-2: 左环岛：检测到左环特征后进入 `KERNEL_ISLAND_L` 分支，完成预入环→打角入环→环内循迹→出环的完整流程，随后返回正常循迹；打角方向与距离门限与右环岛对称（具体值待确认）。
- FR-3: 跷跷板：进入判断（草拟：4 路电感和 < 800）与离开判断（草拟：和 > 1000，或进入后超时 300 拍）接入主状态机；`in_time` 类型修正为 `int32_t`，超时判断在任意运行时长下正确。
- FR-4: 路障：进入判断（草拟：`angle_y > 60`）与离开判断（草拟：`angle_y < 20`）接入主状态机；`cask_flag` 职责拆分——"环岛入环抑制"与"路障元素检测"为两个独立标志，各自命名与语义单一。
- FR-5: 元素接入期间，已验证的十字与右环岛行为不发生变化（回归基准见 [validation](validation.md)）。
- FR-6: 每个元素接入后，数据记录的 `kernel_state` / `roundabout_state` 字段能反映新状态（依赖 Phase 08 存储链可用）。

## 技术约束

- TC-1: 不引入新传感器；判断仅使用现有 4 路电感、IMU 姿态角、DL1B 距离、编码器距离。
- TC-2: 判断函数仍挂接在 2ms 控制拍路径上，单拍开销不得显著增加（无阻塞、无浮点除零风险）。
- TC-3: 新分支的增益取值遵循现有"每态一组增益"的结构（表驱动化留给 [Phase 10](../10-control-consistency/requirements.md)），数值初值沿用源码草拟值并上机标定。
- TC-4: 遵守 [techstack](../../techstack.md) 全部硬性约束；状态值变化需同步 [Phase 04](../04-element-fsm/requirements.md) 规约。

## 接口约定

- 状态机接口：`kernel_state`（`control.h`）取值集合不变（7 态），本阶段只补转移路径，不改状态编号。
- 环岛子状态机：`roundabout_state` 取值集合（`roundabout.h`）允许裁剪闲置宏或按左环需要扩展；若裁剪，`datalog` 导出字段含义同步说明。
- `cask_flag` 拆分后的新标志命名与语义在 Group 3 确认后回填本节。
- 判断阈值参数（电感和门限、角度门限、距离门限、超时拍数）以模块内常量形式集中，便于后续接入调参体系。

## 非目标（Non-goals)

- 不改变已验证的十字/右环岛行为与参数。
- 不做"状态机增益表 vs 用户调参"的语义统一（[Phase 10](../10-control-consistency/requirements.md)）。
- 不清理与元素无关的休眠代码（[Phase 11](../11-hardening-cleanup/plan.md) Group 4）；仅 `cask_flag` 拆分与 `ISLAND_*` 闲置宏的元素侧处置属本阶段。
- 不引入摄像头、超声等新传感器，不改动硬件。
