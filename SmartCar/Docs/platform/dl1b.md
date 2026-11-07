# DL1B 距离原语

现行源码核对基线：8d5ab2faf68432ae03b2c1f91bc4593c7dc7cc3e（2026-11-07）。本文为实现事实说明，未重新批准算法或硬件参数；未执行固件/实机验收。

<a id="f28"></a>
## F28 初始化、轮询与无效传播

I2C2 PB10 SCL/PB11 SDA，7bit地址0x29，HAL传入左移地址0x52；XS为PE8。Init按高50ms→低10ms→高50ms复位，读0x00E5确认bit0=1；从0x0001起写135B固定配置，再循环读0x0031，bit0=0才结束，期间1ms延迟及1000计数超时。所有显式失败返回1，不主动清除既有初始化标志；重新Init失败时不能据返回值推断标志已复位。成功置s_dl1b_init_flag=1、distance=8192，返回0；正常Lifecycle没有消费Init返回值。

Update在正常TIM6调用，未初始化直接返回、保留现值。先读0x0031，失败置8192；此处实际判断是整个dat!=0（与Init判断bit0不同），然后写0x0086=1清中断且忽略写结果；读0x0089必须等于0x89，再从0x0096读两字节，高字节在前，合成int16。0～4000mm发布为距离，其余及任一失败发布8192。不保留上个有效距离；GetDistanceMm只读最近值，不做总线事务。右环用distance<100抑制入口，不等于障碍物功能已启用。

读写实际为HAL_I2C_Mem_Read/Write阻塞操作，16bit寄存器地址，每次10ms超时依赖HAL时基前进。Phase02 FR17的非阻塞要求与实现不符；高优先级ISR中10ms不能视为严格退出上界。900us控制预算是在返回后检查，不能保证阻塞时能够触发保护。此为D02/R02待决定项，本文不改采集策略。

<a id="parameters"></a>
## 参数表

| 参数 | 值/单位/范围 | 来源、覆盖、生效及持久化 |
|---|---|---|
| I2C2 Timing | 0x50921113，模拟滤波开启/数字0 | Core i2c.c；初始化生效，实际SCL未测，无Flash覆盖 |
| 地址 / HAL超时 | 0x29<<1 / 10ms | bsp_dl1b.c编译常量，每笔事务生效 |
| Init轮询计数 / 延迟 | 1000计数；1ms循环延迟 | 判断为1000<counter++，不是严格1000ms总墙钟上限 |
| 有效距离 / INVALID | uint16，0～4000mm / 8192 | bsp_dl1b.c/h；本次读失败立即覆盖，不落盘 |
| 默认配置 | 135B寄存器连续写 | s_dl1b_default_configuration，启动时写，无运行配置接口；测距阈值依据未新增实测批准 |

来源：[Phase02 FR16/17](../../specs/features/02-bsp-platform/requirements.md)、[Phase12](../../specs/features/12-control-safety-closure/requirements.md)。实现：[bsp_dl1b.c](../../BSP/bsp_dl1b.c)、[i2c.c](../../Core/Src/i2c.c)、[scheduler](../../App/system/scheduler.c)。本次核对初始化与Update不同判据、清中断返回、大小端与无效传播；I2C异常/耗时/物理距离和[F33 DL1B](isolated-board-tests.md#f33-dl1b)仍待真实板测。
