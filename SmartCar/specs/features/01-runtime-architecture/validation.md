# Phase 01: 运行时架构与调度 — 验证

2026-10-31文档迁移：**Phase → docs → 代码**。本页保留需求/任务/验收历史，不新增阶段、不修改实现或重新验收。

当前需求导航：[P01功能索引](../../../Docs/traceability.md#p01)。具体输入输出、状态/错误/时序、代码符号与验证方法由下列docs条目唯一维护。

## 当前验收方法入口

阶段完成标准：本阶段适用需求的docs契约可定位实现且其验证判据有实际证据；仍未实施/待决定/待上机的项目继续保持该状态。本轮只检查文档迁移，不新增固件通过结论。原V编号、通过标准、结果、提交号、日志和未执行项完整保留在下方。

| 需求 | 当前方法/判据/实现 | 结果读取方式 |
|---|---|---|
| FR-1 | [F01 初始化与三线调度](../../../Docs/system/runtime-and-safety.md#f01) | 历史实际结果见下方；限制/当前方法见docs |
| FR-2 | [F29 时钟DMA采样与驱动](../../../Docs/platform/board-and-bsp.md#f29) | 历史实际结果见下方；限制/当前方法见docs |
| FR-3 | [F07 IMU校准滤波与积分](../../../Docs/sensing/sensors-and-calibration.md#f07) | 历史实际结果见下方；限制/当前方法见docs |
| FR-4 | [F01 初始化与三线调度](../../../Docs/system/runtime-and-safety.md#f01) | 历史实际结果见下方；限制/当前方法见docs |
| FR-5 | [F01 初始化与三线调度](../../../Docs/system/runtime-and-safety.md#f01)；[F14 菜单按键与显示](../../../Docs/hmi/operation.md#f14)；[F15 多入口参数应用](../../../Docs/services/parameters-and-tuning.md#f15)；[F02 起停、模式与首因](../../../Docs/system/runtime-and-safety.md#f02) | 历史实际结果见下方；限制/当前方法见docs |
| FR-6 | [F01 初始化与三线调度](../../../Docs/system/runtime-and-safety.md#f01) | 历史实际结果见下方；限制/当前方法见docs |
| FR-7 | [F02 起停、模式与首因](../../../Docs/system/runtime-and-safety.md#f02) | 历史实际结果见下方；限制/当前方法见docs |
| FR-8 | [F20 队列前台写入与故障](../../../Docs/services/recording-and-export.md#f20)；[F29 时钟DMA采样与驱动](../../../Docs/platform/board-and-bsp.md#f29)；[F19 日志格式容量](../../../Docs/services/recording-and-export.md#f19) | 历史实际结果见下方；限制/当前方法见docs |
| FR-9 | [F01 初始化与三线调度](../../../Docs/system/runtime-and-safety.md#f01)；[F09 活动电磁元素](../../../Docs/control/elements-and-dormant-code.md#f09)；[F08 方向角度速度与执行](../../../Docs/control/tracking-and-controllers.md#f08)；[F11 双FSM与唯一执行](../../../Docs/control/tracking-and-controllers.md#f11) | 历史实际结果见下方；限制/当前方法见docs |
| FR-10 | [F03 低压持续判定](../../../Docs/system/runtime-and-safety.md#f03) | 历史实际结果见下方；限制/当前方法见docs |
| FR-11 | [F01 初始化与三线调度](../../../Docs/system/runtime-and-safety.md#f01) | 历史实际结果见下方；限制/当前方法见docs |
| FR-12 | [F03 低压持续判定](../../../Docs/system/runtime-and-safety.md#f03) | 历史实际结果见下方；限制/当前方法见docs |
| FR-13 | [F01 初始化与三线调度](../../../Docs/system/runtime-and-safety.md#f01) | 历史实际结果见下方；限制/当前方法见docs |
| FR-14 | [F01 初始化与三线调度](../../../Docs/system/runtime-and-safety.md#f01) | 历史实际结果见下方；限制/当前方法见docs |















## 冻结历史基线（68cb508，非现行契约）

以下原文用于保留阶段决定、组顺序、需求历史与真实验证；不继续独立维护算法/布局/源码映射。当前契约先查上方docs条目。旧“本轮”“待执行”“当前”只指原记录时点，不覆盖后续决定。历史任务的全量软件门禁不自动成为以后每次修改的要求。

<details>
<summary>展开原始阶段记录（原文保留）</summary>

# Phase 01: 运行时架构与调度 — 验证

> as-built 追认规约。计划见 [plan.md](plan.md)，需求见 [requirements.md](requirements.md)。
> 本清单用于确认现状行为与规约一致，并作为后续整改的回归基准。

## 验证清单（Scorecard）

| # | 检查项 | 方法 | 通过标准 | 关联需求 |
|---|---|---|---|---|
| V-1 | 构建基线 | `MDK-GCC/build.sh` | 构建通过，0 警告 0 错误 | 全部 |
| V-2 | 时钟树正确性 | 烧录后串口 115200 收发 | 串口输出无乱码（480MHz 时钟树错误的直接表现是波特率漂移乱码） | FR-1 |
| V-3 | 初始化链完整 | 上电观察 | 屏幕点亮（清屏白底→仪表盘），背光 100%，串口无错误提示 | FR-1, FR-4 |
| V-4 | IMU 失败不卡死 | 拔掉/悬空 IMU（或断电按住 IMU 的 CS）后上电 | 串口输出 `IMU660RB init fail`，屏幕照常显示，主循环不卡死 | FR-2 |
| V-5 | 陀螺零偏校准 | 上电保持车静止约 1s，后观察仪表盘 GYRO 显示 | 静止时角速度显示在 0 附近（无持续漂移趋势） | FR-3 |
| V-6 | 停车态慢闪 | 观察 PE3 蓝灯 | 约 2Hz 闪烁（250ms 半周期） | FR-6, FR-14 |
| V-7 | 停车态显示与调参 | 移动电感/遮挡，操作 ADC 按键或串口发调参命令 | 屏幕电感/偏差值实时变化；调参命令有响应 | FR-5 |
| V-8 | 启动序列 | 按 K1（PC13） | LED 立即转常亮；约 2s 后电机起跑（起跑延时）；风扇在按键后即转 | FR-7, FR-11, FR-14 |
| V-9 | 运行态屏幕冻结 | 起跑后观察屏幕 | 屏幕内容停止刷新（SPI4 让位 IMU），LED 保持常亮 | TC-3, FR-10 |
| V-10 | 低压保护 | 可调电源供电，缓慢降至 < 11.5V 持续 > 1s | 车强制停车（回停车态），LED 恢复慢闪；回升电压后不自动重启 | FR-12 |
| V-11 | 看门狗在喂 | 运行/停车状态各保持 > 10s | 不发生看门狗复位；运行态喂狗点在 TIM7，安全停车后喂狗点转到主循环 | FR-10, FR-13 |
| V-12 | 采样器节拍 | 串口发 `d` 导出 CSV（跑一次后） | `tick` 列连续递增且步进 = DATALOG_DIV（1ms 节拍），记录条数与跑动秒数×1000 吻合 | FR-8 |
| V-13 | 中断优先级不变量 | 代码审查 `tim.c`/`bsp_sampler.c` | TIM15=(0,0)、TIM6=(0,0)、TIM7=(1,0)，与 `.ioc` 一致 | TC-2 |

## 自动化验证

```bash
cd SmartCar/MDK-GCC && ./build.sh
```

- 通过标准：脚本输出 `*** 完成：0 警告 0 错误 ***`，产出 `MDK-GCC/SmartCar.{elf,hex,bin}`。
- 现状实测（2026-10，arm-none-eabi-gcc -O2 -std=c99 -Wall）：0 警告 0 错误，`text=59452 data=236 bss=10192`（约 68KB，超 Keil 评估版 32KB 限制，主构建需正式授权 AC5）。
- 本阶段无单元测试框架（见 [techstack.md](../../techstack.md) 测试节），自动化基线 = 编译零警告。

## 人工验证步骤

1. **烧录**：`MDK-GCC/flash.sh`（OpenOCD + ST-Link），或 Keil 下载。
2. **上电自检**：接串口（115200 8N1）→ 上电 → 屏幕点亮显示仪表盘、LED 慢闪、串口无异常输出（对应 V-2/V-3/V-6）。
3. **校准确认**：上电 1s 内保持车静止（陀螺零偏校准窗口），看 GYRO 页静止值≈0（V-5）。
4. **空转检查**：抬起车轮按 K1 → 2s 后车轮转动、风扇已先转；观察 LED 常亮、屏幕冻结（V-8/V-9）。
5. **停车确认**：断/降电触发低压保护或跑完出赛道，确认回停车态后 LED 恢复慢闪、屏幕恢复刷新（V-10）。
6. **数据链路抽查**：跑一段 → 停车 → 串口发 `d` 导出 CSV，检查 tick 连续性（V-12）。

## 回归检查

后续任何 Phase 整改后，本阶段必须保持的既有行为：

- `MDK-GCC/build.sh` 零警告通过（V-1）。
- 时钟树参数不变：SYSCLK 480MHz / HCLK 240MHz / APB 120MHz / SPI4 10MHz；`.ioc` 再生后 `main.c` 两处 USER CODE 补丁（SysInit SPI45SEL、`App_Init` 调用）仍在（V-2，techstack 约束 5）。
- 中断优先级不变量 TIM15=(0,0)、TIM6=(0,0)、TIM7=(1,0) 不被修改（V-13，techstack 约束 3）。
- TIM6 = 2ms 控制拍不变；若必须改，全套控制系数重标定并同步修改本文件与 techstack（techstack 约束 2）。
- 启动序列行为不变：按键 → 常亮 → 2s 起跑；低压保护与喂狗行为不回归（V-8/V-10/V-11）。

</details>
