# 传感处理、标定与姿态

现行功能说明，核对基线 `68cb508`；2026-10-31迁移。需求经[功能索引](../traceability.md)进入本页，再由实现表定位源码。
当前行为不等于全部已批准/实机验收，冲突见[已知问题](../known-issues.md)。

<a id="f05"></a>
## F05 电感偏差与对称度
### 采集、偏差和对称度

`Track_ReadAdc`四路横左/竖左/竖右/横右各读8次，去一个最大和最小，剩6次整数平均；来自12bit标度0～4095。不宣称8次均来自不同转换。
偏差为`100*(weight_x*横差 + weight_y*竖差)/(weight_xx*横和 + weight_abs*abs(竖差))`，分母零返回0；原权重15/20/22/10。对称度`abs(左右差)*100/左右和`，各对和零时独立返回0。
track_sensor唯一持有原始/滤波值、监视误差、对称度与权重。活动电磁FSM用自己的同拍测量/配置，不应通过修改共享监视量来改变活动元素判断。光电误差不写入电感误差。

### 上下文、失败与验证

停车前台和运行TIM6按既有互斥状态调用；滤波/差分整数顺序保持。权重输入由模块写接口设置，const状态地址寿命为全程序但不是并发快照。F08方向控制消费显式偏差。
对照8读/去极值、零分母、符号/截断、私有测量分离；control_equivalence既有数值基线可按需使用，不证明ADC接线、刷新速率或实车已通过。

**需求来源：** [P03 FR-01, FR-02, FR-03](../../specs/features/03-track-following/requirements.md)；[P11 FR-03](../../specs/features/11-hardening-cleanup/requirements.md)；[P13 FR-01](../../specs/features/13-control-structure/requirements.md)；[P17 FR-05](../../specs/features/17-app-architecture/requirements.md)。实现入口：[App/sensing/track_sensor.c](../../App/sensing/track_sensor.c)、[App/sensing/filter.c](../../App/sensing/filter.c)。

<a id="f06"></a>
## F06 灰度采集标定与质心
### 真实新转换与完整帧

BSP选择S0/S1/S2/S3=PE4/PE6/PE0/PE1，PC4模拟输出；ch0最左、ch15最右须实测。每通道只选址一次，再等5个不同DMA更新，丢前3、后2整数平均；不能在优先级0等待更低优先级DMA中断回调。以硬件推进/DWT识别新转换和预算，16路成功才发布整帧并增加sequence，失败保留上一整帧/序号并设状态。700us=336000周期，达到预算未完成即超时；uint32差处理回绕。

### 手工标定和质心

`s_gray_black/white[16]`为代码配置，0～4095且每通道`abs(white-black)>=200`才有效；0合法，-1未填。当前32项全-1，任一失败都禁止两种模式起跑。只能填用户实测值，不提供可直接运行的伪标定。
`q=clamp(1000*(raw-black)/(white-black),0,1000)`用至少int32及C99向零截断；q<50置0，等于50保留；max(q)<200失线，等于200有效。权重`15-2*ch`，`error=100*sum(w*q)/(15*sum(q))`，有效时约±100；无有效线不继续控制。显示percent=`raw*100/4095`。

### 边界与验证

BSP只负责选址/新转换/整帧，App负责标定/归一化/线有效/误差。TIM6从首拍采双传感，停车前台也刷新；1ms记录可以复用同一个2ms灰度序号，不表示同时采样。
核对逐通道范围、对比度199/200、强度49/50/199/200、负分母极性、timeout/回绕/失败不半帧发布；既有dual灰度夹具为可选软件证据。物理方向、极性、串扰、700us和整车900us预算待上机。

**需求来源：** [P14 FR-01, FR-02, FR-03, FR-04, FR-05, FR-06](../../specs/features/14-dual-mode-tracking/requirements.md)；[P15 FR-06](../../specs/features/15-code-style-consistency/requirements.md)。实现入口：[App/sensing/gray_sensor.c](../../App/sensing/gray_sensor.c)、[App/sensing/gray_calibration.h](../../App/sensing/gray_calibration.h)、[BSP/bsp_gray.c](../../BSP/bsp_gray.c)、[BSP/bsp_adc.c](../../BSP/bsp_adc.c)、[BSP/bsp_time.c](../../BSP/bsp_time.c)。

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

## 滤波共享组件

`Filter_InitLowpass`保存调用者alpha并清last_output；更新为`alpha*input+(1-alpha)*last_output`。高通为`alpha*(last_output+input-last_input)`，随后更新last_input。状态和单位由调用者持有，接口不作参数校验，不访问其他业务状态；当前IMU高通及编码器/电池低通不共用历史。

## 本页实现与模块反查

下表是当前源码定位；完整符号表仅在模块主功能页维护，协作行转向主条目。公共/私有辅助函数归所属功能，不把声明视为另一实现。Core生成部分只作边界，用户区按原标记维护。

| 文件 | 主功能 | 定义/配置定位 |
|---|---|---|
| [App/sensing/filter.c](../../App/sensing/filter.c) | F05 | `Filter_InitLowpass`、`Filter_UpdateLowpass`、`Filter_InitHighpass`、`Filter_UpdateHighpass` |
| [App/sensing/filter.h](../../App/sensing/filter.h) | F05 | 类型/布局/配置；关联同模块C定义 |
| [App/sensing/gray_calibration.h](../../App/sensing/gray_calibration.h) | F06 | 类型/布局/配置；关联同模块C定义 |
| [App/sensing/gray_sensor.c](../../App/sensing/gray_sensor.c) | F06 | `Gray_GetState`、`Gray_GetBlack`、`Gray_GetWhite`、`Gray_ValidateCalibration`、`Gray_CheckCalibration`、`Gray_Normalize`、`Gray_ComputeError`、`Gray_ResetSequence`、`Gray_Init`、`Gray_Update` |
| [App/sensing/gray_sensor.h](../../App/sensing/gray_sensor.h) | F06 | 类型/布局/配置；关联同模块C定义 |
| [App/sensing/imu_proc.c](../../App/sensing/imu_proc.c) | F07 | `IMU_GetAngleDeltaX`、`IMU_GetState`、`IMU_ZeroAngleX`、`IMU_Init`、`IMU_UpdateAccelVelocity`、`IMU_UpdateGyroAngle`、`IMU_CalibrateAccel`、`IMU_CalibrateGyro`、`IMU_ResetRunState` |
| [App/sensing/imu_proc.h](../../App/sensing/imu_proc.h) | F07 | 类型/布局/配置；关联同模块C定义 |
| [App/sensing/legacy_imu.c](../../App/sensing/legacy_imu.c) | F07 | `LegacyIMU_UpdateAccelVelocity`、`LegacyIMU_CalibrateAccel` |
| [App/sensing/legacy_imu.h](../../App/sensing/legacy_imu.h) | F07 | 类型/布局/配置；关联同模块C定义 |
| [App/sensing/track_sensor.c](../../App/sensing/track_sensor.c) | F05 | `Track_GetState`、`Track_CalculateError`、`Track_PublishObservation`、`Track_SetWeights`、`Track_UpdateError`、`Track_UpdateSymmetry`、`Track_ReadAdc`、`Track_InitAdc` |
| [App/sensing/track_sensor.h](../../App/sensing/track_sensor.h) | F05 | 类型/布局/配置；关联同模块C定义 |
| [BSP/bsp_adc.c](../../BSP/bsp_adc.c) | F26 | 协作模块；完整符号表见[F26主条目](../platform/board-and-bsp.md#f26) |
| [BSP/bsp_gray.c](../../BSP/bsp_gray.c) | F29 | 协作模块；完整符号表见[F29主条目](../platform/board-and-bsp.md#f29) |
| [BSP/bsp_imu660rb.c](../../BSP/bsp_imu660rb.c) | F29 | 协作模块；完整符号表见[F29主条目](../platform/board-and-bsp.md#f29) |
| [BSP/bsp_time.c](../../BSP/bsp_time.c) | F29 | 协作模块；完整符号表见[F29主条目](../platform/board-and-bsp.md#f29) |
