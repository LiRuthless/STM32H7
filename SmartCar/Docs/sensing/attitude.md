# 姿态处理

核对代码基线 `8d5ab2f`，2026-11-07；不代表物理验收。

<a id="f07"></a>
## F07 IMU校准滤波与积分
### 当前姿态链

TIM15经SPI4采原始陀螺/加速度，TIM6只消费采样缓存。上电静止`IMU_CalibrateGyro`连续100次读取求均值，不在样本间显式等DRDY/延时，不保证100个独立新样本或固定1s耗时。
角速度`(raw-offset)/14.3`为度/秒，仅Y轴0.2Hz一阶高通，RC=1/(2πf)、α=RC/(RC+dt)，dt=0.002s；X/Z不高通。角度`angle += (gyro+last)*0.001`，发布本拍X增量供电磁独立累计。
`IMU_ZeroAngleX`只清共享X角，保留历史和本拍增量；选中电磁路径才可提交共享复位意图，观察器不能清共享基准。运行reset清积分/滤波历史但不重校准，保留零偏。

### 休眠与失败

IMU状态只有imu_proc持有，legacy_imu接受同一对象/偏置/历史地址，没有第二套历史。加速度/速度积分和加速度校准未接入，accel运行为0；旧积分中读取源、换算系数和积分输入错位仍保留，不能直接启用。
BSP传输失败处理存在R03，不能把初始化能继续理解为全部坏读数受保护。F29记录驱动边界。

### 当前观察方法

当前96B记录/CSV没有angle_x和accel_x，不能用旧P05 V7/V8步骤证明其清零/恒零。停车调试观察`IMU_GetState()->angle/accel/gyro`，控制拍可更新时使用断点或明确上下文读取；运行环岛需另安排不破坏时序的观测方案，不在本轮增加字段。
既有g3_control比较校准/积分/reset及同owner历史；实机漂移、量程、SPI异常与环岛观察仍待验。

**需求来源：** [P01 FR-03](../../specs/features/01-runtime-architecture/requirements.md)；[P02 FR-15](../../specs/features/02-bsp-platform/requirements.md)；[P05 FR-01, FR-02, FR-03, FR-04, FR-05, FR-06, FR-07, FR-08](../../specs/features/05-imu-attitude/requirements.md)；[P13 FR-03](../../specs/features/13-control-structure/requirements.md)。实现入口：[App/sensing/imu_proc.c](../../App/sensing/imu_proc.c)、[App/sensing/legacy_imu.c](../../App/sensing/legacy_imu.c)、[BSP/bsp_imu660rb.c](../../BSP/bsp_imu660rb.c)。


<a id="parameters"></a>
## 参数与更新链

| 参数/状态 | 数值/单位/范围 | 来源、覆盖、生效/保存 |
|---|---|---|
| 校准样本数 | int8常量100；int32三轴累加后float平均 | 上电直接读SPI，无DRDY等待；零偏仅RAM，重启重校准 |
| IMU_GYRO_RAW_TO_DPS | 14.3 原码/(度/秒) | app_config.h常量，三轴先去偏再除；非运行调参/Flash |
| 高通 | 0.2Hz，dt=0.002s；仅Y活动 | IMU_Init/ResetRunState初始化，私有状态RAM；X实例保留未用于活动滤波 |
| 梯形系数 | 0.001s；(当前+前拍)×系数 | 2ms控制拍固定；角度为度；reset清积分/历史保留上电偏置 |
| accel/velocity | 活动值保持0 | 休眠转换与积分未接入；不得把板测原码上传当正常姿态加速度处理 |

共享滤波公式见[filters](sensors-and-calibration.md#filters)；保护动作见[起停](../system/runtime-and-safety.md#f02-stop)。
