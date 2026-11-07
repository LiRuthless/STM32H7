# TIM15 原码采样与发布

现行源码核对基线：8d5ab2faf68432ae03b2c1f91bc4593c7dc7cc3e（2026-11-07）。本文为实现事实说明，未重新批准算法或硬件参数；未执行固件/实机验收。

<a id="f29-sampler"></a>
## F29.SAMPLER 初始化、启动和消费

BSP_Sampler_Init直接配置TIM15：CR1停计数、PSC=239、ARR=999、CNT=0、UG装载、清UIF、开UIE并配置NVIC(0,0)，尚未启动。240MHz/240/1000得到1kHz。Start先清s_tick和左右累计，再清CNT/UIF并开CEN；不清最近1ms值或IMU缓存。Stop只清CEN，保留已采数据。

正常NONE分支的TIM15_IRQHandler先清UIF、s_tick++，顺序GetLeft→GetRight读清硬件，更新最近1ms轮值并加到int16累计；再GetGyro、发布三轴、GetAcc、发布三轴，最后App_SampleISR记录钩子。缓存发布先于日志。IMU没有DRDY同步，1ms读取并不保证每次独立新样本；错误发布风险见[IMU驱动](imu660rb.md#f29-imu)。BSP不做业务滤波、单位转换或积分，归[姿态](../sensing/attitude.md#f07)。

TIM6同优先级0消费左右累计：复制sum到局部→sum=0→返回，依赖TIM15不能抢占这段读清。GetEnc*_1ms只读最近脉冲，GetGyroRaw/GetAccRaw逐轴写调用方指针且无空检查；前台不能据volatile宣称三轴/多字段一致快照，应停车后读或另用已批准快照路径。

独立板测编译分支只在UIF和UIE都置位时清标志并调用BoardTest_SampleISR，不推进正常s_tick/编码器/IMU/日志。测试只采所选对象、所选IRQ见[F33](isolated-board-tests.md#f33)。停止测试还屏蔽IRQ、清挂起等，由bsp_board_test处理，不能等同普通Sampler_Stop。

<a id="parameters"></a>
## 状态与参数

| 对象 | 类型/值/单位 | owner、复位、覆盖与持久化 |
|---|---|---|
| s_tick | volatile uint32，1ms拍，回绕 | TIM15写，Start清0，getter只读，无落盘 |
| s_enc_*_1ms / s_enc_*_sum | volatile int16，脉冲/1ms与消费间隔累计 | TIM15写、TIM6读清sum；Start只清sum，无饱和保护 |
| s_gyro / s_acc | volatile int16[3]原码 | TIM15顺序发布；上层换算；Stop保留，无Flash恢复 |
| PSC/ARR/NVIC | 239/999/(0,0) | bsp_sampler.c编译配置，Init生效；周期改动须先规约与重标定 |

来源：[Phase02 FR12/13](../../specs/features/02-bsp-platform/requirements.md)、[Phase17 G2](../../specs/features/17-app-architecture/g2-system.md)。实现：[bsp_sampler.c](../../BSP/bsp_sampler.c)、[bsp_runtime.c](../../BSP/bsp_runtime.c)、[App采样入口](../../App/app.c)。本次核对所有写入顺序、Start/Stop非对称清理与条件编译；真实采样间隔、ISR耗时、累计溢出边界和SPI共享仍待实机验证。
