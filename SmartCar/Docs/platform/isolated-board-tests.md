# 独立板测契约

<a id="f33"></a>
## F33 传感器、按键和板载屏（2026-11-06，待实现）

来源：Phase02 FR-27～30；任务方案为Reports/sensor-isolated-board-test-plan-2026-11-06.md。报告第1～8节已确认需求全部适用，下列明确原未决项。正常固件故障策略、公共UART语义、IMU错误处理、起跑门禁、未填灰度标定及休眠代码保持。

### 文件、符号与调用链

| 文件 | 职责与符号 |
|---|---|
| App/system/board_test_config.h | BOARD_TEST_SELECT，BOARD_TEST_NONE=0；LCD=1、IND_HL/VL/VR/HR=2～5、GRAY=6、GYRO=7、ACCEL=8、DL1B=9、ENCODER_LEFT/RIGHT=10/11、BATTERY=12、ADC_KEY1/2=13/14、PC13=15；非法值#error |
| App/system/board_test.c/.h | BoardTest_Run分派；BoardTest_RunLcd/InductanceHL/InductanceVL/InductanceVR/InductanceHR/Gray/Gyro/Accel/Dl1b/EncoderLeft/EncoderRight/Battery/AdcKey1/AdcKey2/Pc13独立入口；对应对象ISR处理，允许内部复用；快照、窗口和队列owner |
| BSP/bsp_board_test.c/.h | BSP_BoardTest_Init/StartTimer/StopTimer/SendFrame；最小外设、单轮启用、测试UART结果；公开给App |
| Core/Src/main.c | SysInit的PLL3补丁后、整体MX及App_Init前调用BoardTest_Run；意外返回永久停留 |
| BSP/bsp_runtime.c | 唯一HAL_TIM_PeriodElapsedCallback编译期分派TIM6/7测试 |
| BSP/bsp_sampler.c | 唯一TIM15_IRQHandler测试分支先确认/清UIF，再采集所选对象；NONE原路径 |
| MDK-ARM/SmartCar.uvprojx | 登记新增两个C，不宣称AC5已验收 |

不调用MX_GPIO_Init整体入口，测试BSP只配置必要GPIO。SPI4共用CS先置高，LCD仅初始化SPI4/TIM1背光，IMU无LCD；ADC测试保留十rank扫描/DMA D2 0x30000000及D-Cache关闭。按需初始化TIM6/7、单轮TIM3或TIM5、I2C2、USART1；所有MX后恢复SPI45选择。状态先初始化再使能采集IRQ，清挂起后仅启动所选定时器。
不进入Lifecycle/Scheduler正常链，不启动电机/风扇PWM、参数、日志、Flash、SCB、调参、看门狗及Wi-Fi EXTI。SysTick保留；USART1仅IMU测试460800/8N1，不开启接收业务。App仅用公开BSP接口。

### 采集、状态和显示

TIM6优先级0/2ms：电感四路分别读指定ADC最近值；灰度BSP完整帧（五个新转换丢前三均后二，700μs）；DL1B先Update后Get。TIM15优先级0/1ms：陀螺/加速度分别读三轴原码，左/右轮分别唯一读清。TIM7优先级1/5ms：Battery_Update及原0.65低通。ADC键/PC13仅前台轮询；PC13唯一事件消费入口。初始化失败不启用采集，显示模式显示错误，IMU静默留存错误状态。
所选ISR拥有采集状态，前台短临界区复制一致快照，LCD绘图和UART发送均在临界区外，不重复读取被测硬件（前台按键除外）。编码器int64累计来自每个1ms读清，不从显示快照累加。
每100ms刷新五行20字符，不翻页；统计为1秒非重叠窗口，保留上一完整窗口极值，首次完成前显示----，ADC键按实际前台读取次数统计。

| 测试 | 五行固定字段顺序/单位 |
|---|---|
| 电感、ADC键 | 当前ADC码、上一窗口min、max、峰峰值、总读取数 |
| 电池 | 原始ADC码、原滤波ADC码、上一窗口原始min、max、更新数 |
| 灰度 | 前四行ch0～15，4×4布局，x=0/40/80/120；第五行状态、成功帧号、成功帧耗时μs；失败旧完整帧红色，首帧前---- |
| 编码器 | 最近1ms脉冲、累计脉冲、采集数、入口间隔min/max μs、最长执行μs |
| DL1B | 当前mm、当前有效标志、最近有效mm、距有效值ms、无效次数；8192为无效，未就绪计无效，不表示500个新距离 |
| PC13 | 最近事件码、短按数、长按数，其余行空白 |
| LCD | 五行边缘、正负数、变化计数和数字缩短后残留检查 |

### IMU上传和证据

静态队列1024可用槽，每槽int16三轴（6字节，共6144字节）；32bit单调head/tail，无符号差判满，下标&1023。ISR写槽→DMB→发布head；前台观察head→DMB→复制至独立静态发送缓冲，发送成功后DMB发布tail。发送结束前不释放槽；缓冲全程序寿命，无DMA复用，不阻塞ISR。
三轴原码分别转32bit float，显式打包三个小端float与00 00 80 7F，共16字节；CH0=X、CH1=Y、CH2=Z，每1ms完整上传，不抽样、不混入printf或文本。
SendFrame返回完整/部分/失败及已提交字节数；只有HAL_OK且TC才完整。失败的字节数仅表示已写入UART，不能证明线路完成或PC收到；调用前忙则失败且0，不读陈旧TxXferCount。前台HAL阻塞发送超时10ms，部分帧不重试；采集ISR阻塞时不保证墙钟10ms内返回。公共BSP_UART_Write保持原样。
分别记录采集、入队、完整发送、溢出、发送异常、队列余量及峰值。首次满：计本样本溢出并停TIM15，前台排空后冻结；发送异常：停TIM15及后续发送，保留当前帧及队列，失败槽不消费。异常均需重新上电重测。
60秒连续观察后停止IMU采集、排空冻结计数，其他测试可继续观察；验收区分余量与丢样。IMU初始化成功后坏读数仍不可识别，不宣称有效样本检测通过。

### 验收与限制

BT-V01：审查NONE原行为、非法值报错、返回隔离、最小初始化、唯一IRQ、周期/优先级、ADC/D-Cache、SPI45；调用顺序、并发发布及错误路径逐函数审查。
BT-V02：所有选择及NONE用MDK-GCC/build.sh零错误零警告；CS-01～15及R08真实人工证据/暂存检查；不例行双LCD或历史全量脚本，不增加模拟硬件。
BT-V03：每项板上连续60秒，IRQ平均频率误差≤1%，每次入口间隔偏差≤周期10%，最长执行严格小于周期，灰度<700μs。保存IRQ次数、累计间隔（逐拍无符号DWT差，避免跨60秒回绕）、min/max及最长耗时，观察刷屏/上传影响。
BT-V04：IMU零溢出/异常/丢样，停止排空后采集=入队=完整发送且余量0；串口抓帧及PC计数，不用VOFA曲线替代证据。USART1 PA9/TX接转换器RX、共地、核实电平；VOFA实际COM、460800、8N1、JustFloat三通道XYZ，按1kHz设置时间轴。
BT-V05：逐对象按报告第5/6/9节施加单变量刺激，记录实际值/接线/轴方向/通道顺序；不提供伪标定、不换算未经核实的V/RPM。
所有板测保持待验。IMU和DL1B阻塞HAL调用位于优先级0，SysTick同级不能抢占，故障超时可能不退出；本次不修复，不把10ms解释为ISR严格退出上限。IMU无DRDY，不保证每拍独立新样本；理论串口带宽不替代吞吐实测。
