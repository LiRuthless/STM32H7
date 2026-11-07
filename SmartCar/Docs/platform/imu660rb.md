# IMU660RB 原码驱动

现行源码核对基线：8d5ab2faf68432ae03b2c1f91bc4593c7dc7cc3e（2026-11-07）。本文为实现事实说明，未重新批准算法或硬件参数；未执行固件/实机验收。

<a id="f29-imu"></a>
## F29.IMU 初始化和读取

SPI4 PE12 SCK/PE5 MISO/PE14 MOSI，CS PD3低有效，Mode0、8bit、MSB；恢复PLL3Q80MHz再/8得名义10MHz，和LCD共享SPI4。正常运行由TIM15读原码，TIM6只消费缓存；停车后才刷新LCD，避免共享总线交错，不存在驱动级锁。

Init先HAL_Delay20ms，再读WHO_AM_I(0x0F)直到0x6B，每轮Delay1ms，timeout_count++>0xFF时失败。成功后写INT1_CTRL=0x03、CTRL1_XL=0x8C、CTRL2_G=0x8C、CTRL3_C=0x44、CTRL4_C=0x02、CTRL5_C=0、CTRL6_C=0、CTRL7_G=0、CTRL9_XL=0x01。前两项对应当前注释名义±8g、±2000dps、1.66kHz；写寄存器HAL返回均未检查，Init=0只说明身份检查通过及发出了配置，不证明配置读回正确。

GetGyro从0x22、GetAcc从0x28，各发(reg|0x80)后收6字节，CS包住事务；按dat[1]<<8|dat[0]等低字节在前组合int16三轴。所有SPI发送/接收失败被忽略，局部dat[6]未初始化，失败可能发布未定义原码；没有有效位、保留旧值或安全零输出保证。R03保留，不能由“初始化失败继续”推定运行读取可靠。正常Init失败只打印后继续，板测Init失败单独中止见[F33 IMU](isolated-board-tests.md#f33-imu)。

<a id="parameters"></a>
## 参数与边界

| 参数 | 类型/真实值/单位 | 来源、覆盖、生效与持久化 |
|---|---|---|
| SPI超时 | 编译宏10ms/每次HAL调用 | bsp_imu660rb.c；依赖HAL时基，不是高优先级ISR墙钟上界 |
| 身份/轮询计数 | 0x6B / 0x00FF计数 | CheckIdentity；启动检查，无Flash覆盖 |
| ODR / 量程 | 名义1.66kHz，±2000dps / ±8g | 两个0x8C配置，启动写入；实际新样本率/量程未测 |
| 输出 | int16 x/y/z原码，-32768～32767 | GetAcc/GetGyro依次写指针，调用者须有效；不负责物理转换 |

业务换算使用的14.3与4098及滤波/积分顺序只在[姿态算法](../sensing/attitude.md#f07)维护；本页不把名义量程代替标定结果。来源：[Phase02 FR14/15](../../specs/features/02-bsp-platform/requirements.md)、[Phase05](../../specs/features/05-imu-attitude/requirements.md)。实现：[bsp_imu660rb.c](../../BSP/bsp_imu660rb.c)、[Core spi.c](../../Core/Src/spi.c)。本次核对寄存器常量、字节组合、失败副作用和SPI所有权；真实失败传播、零偏/姿态标定、吞吐与时序仍待验。
