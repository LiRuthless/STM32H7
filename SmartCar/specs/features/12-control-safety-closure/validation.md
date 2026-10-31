# Phase 12: 控制闭环与安全停车修复 — 验证

2026-10-31文档迁移：**Phase → docs → 代码**。本页保留需求/任务/验收历史，不新增阶段、不修改实现或重新验收。

当前需求导航：[P12功能索引](../../../docs/traceability.md#p12)。具体输入输出、状态/错误/时序、代码符号与验证方法由下列docs条目唯一维护。

## 当前验收方法入口

阶段完成标准：本阶段适用需求的docs契约可定位实现且其验证判据有实际证据；仍未实施/待决定/待上机的项目继续保持该状态。本轮只检查文档迁移，不新增固件通过结论。原V编号、通过标准、结果、提交号、日志和未执行项完整保留在下方。

| 需求 | 当前方法/判据/实现 | 结果读取方式 |
|---|---|---|
| FR-1 | [F08 方向角度速度与执行](../../../docs/control/tracking-and-controllers.md#f08) | 历史实际结果见下方；限制/当前方法见docs |
| FR-2 | [F02 起停、模式与首因](../../../docs/system/runtime-and-safety.md#f02-stop) | 历史实际结果见下方；限制/当前方法见docs |
| FR-3 | [F02 起停、模式与首因](../../../docs/system/runtime-and-safety.md#f02-stop) | 历史实际结果见下方；限制/当前方法见docs |
| FR-4 | [F02 起停、模式与首因](../../../docs/system/runtime-and-safety.md#f02-stop)；[F20 队列前台写入与故障](../../../docs/services/recording-and-export.md#f20)；[F18 完整拍临时调参](../../../docs/services/parameters-and-tuning.md#f18) | 历史实际结果见下方；限制/当前方法见docs |
| FR-5 | [F01 初始化与三线调度](../../../docs/system/runtime-and-safety.md#f01) | 历史实际结果见下方；限制/当前方法见docs |
| FR-6 | [F02 起停、模式与首因](../../../docs/system/runtime-and-safety.md#f02-start)；[F07 IMU校准滤波与积分](../../../docs/sensing/sensors-and-calibration.md#f07)；[F08 方向角度速度与执行](../../../docs/control/tracking-and-controllers.md#f08)；[F11 双FSM与唯一执行](../../../docs/control/tracking-and-controllers.md#f11) | 历史实际结果见下方；限制/当前方法见docs |
| FR-7 | [F29 时钟DMA采样与驱动](../../../docs/platform/board-and-bsp.md#f29)；[F14 菜单按键与显示](../../../docs/hmi/operation.md#f14)；[F07 IMU校准滤波与积分](../../../docs/sensing/sensors-and-calibration.md#f07) | 历史实际结果见下方；限制/当前方法见docs |








## 冻结历史基线（68cb508，非现行契约）

以下原文用于保留阶段决定、组顺序、需求历史与真实验证；不继续独立维护算法/布局/源码映射。当前契约先查上方docs条目。旧“本轮”“待执行”“当前”只指原记录时点，不覆盖后续决定。历史任务的全量软件门禁不自动成为以后每次修改的要求。

<details>
<summary>展开原始阶段记录（原文保留）</summary>

# Phase 12: 控制闭环与安全停车修复 — 验证

## 需求追溯矩阵

| 验证项 | 验证内容 | 关联需求 |
|---|---|---|
| V-1 | GCC 全量构建 0 警告 0 错误 | 全部 |
| V-2 | 编码器滤波初始化调用链与系数 | FR-1 |
| V-3 | 出赛道立即停车调用链 | FR-2、FR-3 |
| V-4 | 低压立即停车调用链 | FR-2、FR-3 |
| V-5 | 停车 ISR 无阻塞收尾 | FR-2、FR-4 |
| V-6 | 停车后 SPI4 分时 | FR-4、FR-7 |
| V-7 | 停车态 IWDG 喂狗 | FR-5 |
| V-8 | 再次起跑状态确定性 | FR-6 |
| V-9 | 架空轮速度反馈实测 | FR-1 |
| V-10 | 架空轮安全停车实测 | FR-2～FR-7 |

## 自动化验证

### V-1 全量构建

执行：

```bash
cd SmartCar/MDK-GCC
./build.sh
```

通过标准：退出码为 0，输出 `0 警告 0 错误`，链接成功。

### V-2 编码器初始化链

代码检查：

- `App_Init()` 调用 `encoder_init()`；
- `encoder_init()` 同时调用 `BSP_Encoder_Init()` 和两次 `lowpass_init(..., alpha)`；
- `alpha` 仍为 `0.88f`；
- `App_Init()` 不再绕过 App 层直接调用 `BSP_Encoder_Init()`。

通过标准：四项全部成立。

### V-3/V-4 保护入口

代码检查：

- `control.c` 出赛道分支只通过 `App_RequestStop(APP_STOP_OFF_TRACK)` 发起停车；
- `App_TaskISR` 低压计数超限只通过 `App_RequestStop(APP_STOP_LOW_BATTERY)` 发起停车；
- 两处不再各自散落修改运行标志。

通过标准：两条保护路径均进入统一接口，原因枚举不混用。

### V-5 ISR 有界性

检查 `App_RequestStop()` 调用图。

通过标准：路径内不存在 `Datalog_Stop/Flush`、`BSP_Flash_*`、`BSP_UART_*`、`BSP_LCD_*`、`HAL_Delay`、动态内存或无超时循环；PWM 清零发生在停止定时器之前；`App_ControlISR` 停车后不会调用 `motor_control()`。

### V-6 SPI4 分时

代码检查：

- 停车请求调用 `BSP_Sampler_Stop()`；
- `App_FinalizeStop()` 在主循环执行；
- 主循环仅在停车收尾完成后执行 `Dashboard_Update()` 或 `menu()`；
- 再次起跑前主循环内没有并发 LCD 任务。

通过标准：停车态刷新 LCD 时 TIM15 已停止。

### V-7 IWDG 生命周期

代码检查：首次启动调用一次 `BSP_WDT_Init()` 并记录已启动；运行态 TIM7 喂狗；停车态主循环在已启动条件下喂狗。

通过标准：首次上电未起跑不写 IWDG；首次起跑后无论运行或停车均存在喂狗路径。

### V-8 再次起跑复位覆盖

逐项核对 FR-6 列出的状态均有明确复位语句和唯一调用点，且发生在启动三路定时器之前。

通过标准：无遗漏；参数、零偏与控制常数未被复位。

## 人工上机验证

> 未连接实车执行前，以下项目状态必须保持“待上机”，不得根据代码检查推定通过。

### V-9 编码器反馈

1. 架空车轮，上电完成 IMU 校准。
2. 手动转动左右轮，观察仪表盘 `real_speed_L/R`。
3. 按启动键，在限位保护条件下短时运行并记录速度。

通过标准：手动转轮产生方向正确的非零反馈；电机运行时反馈不恒为 0；停止后反馈回到 0。若方向相反，只记录现象并按既有编码器方向确认流程处理，不在本阶段擅自改极性。

### V-10 停车与再次起跑

1. 架空车轮起跑，等待 2 s 延时结束。
2. 人工移开电磁传感器使总和不大于 300。
3. 观察电机 PWM、风扇、LED、LCD、串口和 10 s 内系统状态。
4. 再次按启动键，确认重新经历 2 s 延时并正常运行。
5. 使用可调电源重复低压持续超过 1 s 的停车验证。

通过标准：

- 两种停车原因下电机 PWM 均立即归零且无积分导致的持续转动；
- 风扇回 `FAN_DUTY_IDLE`；
- 串口分别只输出一次对应原因；
- 停车后 LCD 正常刷新且无花屏，保持 10 s 不发生看门狗复位；
- 再次起跑的距离、状态机、PID 历史和 IMU 积分均从规约定义值开始；
- 两种停车后均不会自动重启。

## 验证结果记录

| 日期 | 项目 | 结果 | 证据/备注 |
|---|---|---|---|
| 2026-10-02 | V-1 | 通过 | `MDK-GCC/build.sh`：0 警告、0 错误；text=60548、data=236、bss=10232 |
| 2026-10-02 | V-2～V-8 | 通过（代码检查） | 初始化、统一停车、ISR 有界性、SPI4 分时、IWDG 生命周期及复位覆盖逐项核对通过 |
| 待上机 | V-9～V-10 | 待上机 | 必须由真实硬件验证 |

</details>
