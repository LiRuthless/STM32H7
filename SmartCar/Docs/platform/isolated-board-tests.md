# 独立板测契约

<a id="f33"></a>
## F33 传感器、按键和板载屏（2026-11-06，软件已实现，板测待验）

来源：Phase02 FR-27～30；任务方案为Reports/sensor-isolated-board-test-plan-2026-11-06.md。报告第1～8节已确认技术需求适用，下列明确原未决项；报告中历史“仅生成报告”授权由本次规约及固件实现授权替代。正常固件故障策略、公共UART语义、IMU错误处理、起跑门禁、未填灰度标定及休眠代码保持。

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

不调用MX_GPIO_Init或MX_DMA_Init整体入口，测试BSP只配置必要GPIO及ADC的DMA Stream0中断（原优先级2），不启用Wi-Fi的DMA Stream1/2。SPI4共用CS先置高，LCD仅初始化SPI4/TIM1背光，IMU无LCD；ADC测试保留十rank扫描/DMA D2 0x30000000及D-Cache关闭。按需初始化TIM6/7、单轮TIM3或TIM5、I2C2、USART1；所有MX后恢复SPI45选择。状态先初始化再使能采集IRQ，清挂起后仅启动所选定时器。
不进入Lifecycle/Scheduler正常链，不启动电机/风扇PWM、参数、日志、Flash、SCB、调参、看门狗及Wi-Fi EXTI。SysTick保留；USART1仅IMU测试460800/8N1，不开启接收业务。App仅用公开BSP接口。

<a id="f33-display"></a>
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
| DL1B | 当前mm、当前有效标志、最近有效mm、距有效值ms、无效次数；首个有效值前年龄为----，8192为无效，未就绪计无效，不表示500个新距离 |
| PC13 | 最近事件码、短按数、长按数，其余行空白 |
| LCD | 五行边缘、正负数、变化计数和数字缩短后残留检查 |

<a id="f33-imu"></a>
### IMU上传和证据

静态队列1024可用槽，每槽int16三轴（6字节，共6144字节）；32bit单调head/tail，无符号差判满，下标&1023。ISR写槽→DMB→发布head；前台观察head→DMB→复制至独立静态发送缓冲，发送成功后DMB发布tail。发送结束前不释放槽；缓冲全程序寿命，无DMA复用，不阻塞ISR。
三轴原码分别转32bit float，显式打包三个小端float与00 00 80 7F，共16字节；CH0=X、CH1=Y、CH2=Z，每1ms完整上传，不抽样、不混入printf或文本。
SendFrame返回status=0完整、1部分或线路未确认、2失败，以及已提交字节数；只有HAL_OK且TC才完整。失败的字节数仅表示已写入UART，不能证明线路完成或PC收到；调用前忙则失败且0，不读陈旧TxXferCount。前台HAL阻塞发送超时10ms，部分帧不重试；HAL tick量化及采集ISR阻塞时不保证墙钟10ms内返回。公共BSP_UART_Write保持原样。
分别记录采集、入队、完整发送、溢出、发送异常、队列余量及峰值。测试停TIM15时先禁用其NVIC、停止计数，再清UIE/UIF及挂起，防止停止后补采；不改正常Sampler_Stop接口。首次满：计本样本溢出并停TIM15，前台排空后冻结；发送异常：停TIM15及后续发送，保留当前帧及队列，失败槽不消费。异常均需重新上电重测。
60秒连续观察后停止IMU采集、排空冻结计数，其他测试可继续观察；验收区分余量与丢样。IMU初始化成功后坏读数仍不可识别，不宣称有效样本检测通过。

### 验收与限制

BT-V01：审查NONE原行为、非法值报错、返回隔离、最小初始化、唯一IRQ、周期/优先级、ADC/D-Cache、SPI45；调用顺序、并发发布及错误路径逐函数审查。
BT-V02：所有选择及NONE用MDK-GCC/build.sh零错误零警告；CS-01～15及R08真实人工证据/暂存检查；不例行双LCD或历史全量脚本，不增加模拟硬件。
BT-V03：每项板上连续60秒，IRQ平均频率误差≤1%，每次入口间隔偏差≤周期10%，最长执行严格小于周期，灰度<700μs。保存IRQ次数、累计间隔（逐拍无符号DWT差，避免跨60秒回绕）、min/max及最长耗时，观察刷屏/上传影响。
BT-V04：IMU零溢出/异常/丢样，停止排空后采集=入队=完整发送且余量0；串口抓帧及PC计数，不用VOFA曲线替代证据。USART1 PA9/TX接转换器RX、共地、核实电平；VOFA实际COM、460800、8N1、JustFloat三通道XYZ，按1kHz设置时间轴。
BT-V05：逐对象按报告第5/6/9节施加单变量刺激，记录实际值/接线/轴方向/通道顺序；不提供伪标定、不换算未经核实的V/RPM。
所有板测保持待验。IMU和DL1B阻塞HAL调用位于优先级0，SysTick同级不能抢占，故障超时可能不退出；本次不修复，不把10ms解释为ISR严格退出上限。IMU无DRDY，不保证每拍独立新样本；理论串口带宽不替代吞吐实测。

## 编译、切换和操作

本轮支持板载LCD配置LCD_TARGET_EXTERNAL=0；不宣称外接屏验收。
在App/system/board_test_config.h修改默认BOARD_TEST_SELECT为下表一个宏，用Git Bash执行：

```bash
cd SmartCar/MDK-GCC
./build.sh
# 确认0错误0警告和所选宏后，用户自行授权并执行烧录：
./flash.sh
```

切换测试必须重新编译、烧录、重新上电；无菜单。恢复BOARD_TEST_NONE后重新编译烧录才回到正常业务。本次不自动烧录。

| 宏（BOARD_TEST_前缀） | 独立入口（BoardTest_前缀） | 采集处理/周期 |
|---|---|---|
| NONE | 正常App_Init/App_Loop | 原正常链 |
| LCD | RunLcd | 前台显示 |
| IND_HL | RunInductanceHL | ControlISR/TIM6，2ms |
| IND_VL | RunInductanceVL | ControlISR/TIM6，2ms |
| IND_VR | RunInductanceVR | ControlISR/TIM6，2ms |
| IND_HR | RunInductanceHR | ControlISR/TIM6，2ms |
| GRAY | RunGray | ControlISR/TIM6，2ms |
| GYRO | RunGyro | SampleISR/TIM15，1ms |
| ACCEL | RunAccel | SampleISR/TIM15，1ms |
| DL1B | RunDl1b | ControlISR/TIM6，2ms |
| ENCODER_LEFT | RunEncoderLeft | SampleISR/TIM15，1ms |
| ENCODER_RIGHT | RunEncoderRight | SampleISR/TIM15，1ms |
| BATTERY | RunBattery | TaskISR/TIM7，5ms |
| ADC_KEY1 | RunAdcKey1 | 前台轮询，无固定频率 |
| ADC_KEY2 | RunAdcKey2 | 前台轮询，无固定频率 |
| PC13 | RunPc13 | 前台轮询，无固定频率 |

TIM6/7使用唯一HAL回调，TIM15使用唯一IRQ；同类采集复用公共处理函数。各入口仅用于对应编译选择上电调用，不作为运行时切换API。

屏上RAW/FILTER/MIN/MAX/PP为12bit ADC码，N为读取/更新次数；PULSE/SUM为脉冲，DT/MAX为μs。灰度S=0成功、1参数错误、2超时、3DMA错误，F是成功帧号；失败保留旧数据并红色显示。PC13 EVENT=0无事件、1短按松开、2长按；事件字段保留最近非零事件。INIT/START ERROR表示已可检测到初始化或定时启动失败，采集不继续。未完成首个统计窗口显示----。

IMU无LCD也无串口文本。调试器查看board_test.c的s_state.count（采集）、s_head（入队）、s_sent（完整提交线路）、s_overflow、s_send_errors、s_peak、s_head-s_tail（余量）、s_tx（发送异常状态/提交字节）和s_frame（当前帧）。调试暂停会改变实时条件，不作为不中断吞吐的唯一证据；应结合串口抓取和外部计时。
IMU以逐拍累积DWT入口间隔达到60×480000000周期为结束条件，停止采集并排空；实际频率和间隔须由s_state.interval_cycles、count、min/max和外部时钟核验。不能凭定时配置或拍数声明墙钟60秒通过。s_stopped=1表示停采；发送异常时队列保留，余量不等于丢样。

### 待执行板测清单

- LCD五行边缘、符号、数值缩短无残留。
- 电感逐路接线及赛道响应；灰度16路顺序、遮挡、700μs预算、失败旧帧标识。
- 电池原码/滤波与万用表对照；两组ADC键每键稳定区间；PC13短按、长按及释放重武装。
- 左右轮独立读清、方向、静止噪声及累计计数。
- IMU轴响应、USB串口460800能力、16字节抓帧、PC帧数、队列/发送计数和零丢样。
- DL1B尺距、有效/无效/未就绪及最近有效值年龄。
- 各IRQ连续60秒频率、间隔、完整IRQ耗时及刷屏/发送干扰；DWT执行统计不包含向量入口/分派开销，外部测量须覆盖完整IRQ。
- 记录板卡、供电、接线、固件提交、宏、刺激、读数和证据；硬件异常阻塞限制单列，不以编译替代通过。

软件构建与人工审查结果见[交付验证](../../../Reports/isolated-board-test-validation-2026-11-06.md)，板上项目继续待验。

## 对象子条目与主说明

以下子条目是同一F33运行场景的具体入口；所有对象共享上述隔离、快照、初始化失败停留和板测待验约束。数值是源码配置，不是实测结论。

<a id="f33-lcd"></a>
### LCD（选择1）

最小SPI4/TIM1背光与LCD，前台每100ms绘五行×20字符；LCD用计数、正负和长短字符串验证残留。每格8×16像素，末尾补空格；无采集IRQ。默认板载LCD_TARGET_EXTERNAL=0，外屏未验。

<a id="f33-adc"></a>
### 四路电感ADC（选择2～5）

最小ADC十rank DMA与LCD/TIM6；每2ms读取枚举select−2的最近ADC值。未添加正常电感低通或归一化。1秒非重叠窗口保存上一窗口min/max，pp=max−min；读取次数不代表独立新转换数。物理通道/rank见 [平台F26](board-and-bsp.md#f26)。

<a id="f33-gray"></a>
### 16路灰度（选择6）

在ADC基础上配置四路选通GPIO，TIM6每2ms同步请求完整帧。五个新转换丢前三、均后二；成功才替换s_state.gray，失败保留旧完整帧并用红字显示；首帧前----。帧号只计成功，耗时除480显示μs；帧预算700μs，不能由显示刷新周期推定满足。

<a id="f33-dl1b"></a>
### DL1B（选择9）

最小I2C2/XS、LCD/TIM6；每2ms先Update再Get。8192无效，更新最近有效距离/时刻只在非8192时；无效次数是轮询无效次数，非独立量测次数。初始化失败不启动采集；无首个有效值时AGE----。未就绪、总线故障与ISR时基限制均按本页验收限制保留。

<a id="f33-encoder"></a>
### 左右编码器（选择10/11）

左只初始化/启动TIM5，右只TIM3，TIM15每1ms唯一读清对应计数器。int64 pulses直接累计本次原始脉冲；不走正常Motor的0.88滤波和2ms里程。前台只复制快照显示；统计IRQ入口间隔和执行耗时，不再次读硬件。

<a id="f33-battery"></a>
### 电池（选择12）

ADC与LCD/TIM7；每5ms调用Battery_Update，然后记录原码、原0.65低通码。窗口统计原码min/max；本页不换算未经核实的电池V值，也不启动正常Lifecycle低压停车。

<a id="f33-keys"></a>
### ADC键与PC13（选择13～15）

ADC键只前台轮询原始ADC，并按实际读取次数记N/窗口，无固定采集频率；PC13只初始化GPIOC输入下拉与BSP_Key，不启动ADC或采集定时器。唯一前台事件消费入口统计短按/长按，保留最近非零事件0/1/2语义；不借板测操作改变正常起跑/切模式策略。

<a id="parameters"></a>
## 板测参数、来源与生效

| 参数/类型 | 固定值或默认、单位/范围 | 覆盖、生效与持久化 |
|---|---|---|
| BOARD_TEST_SELECT / 宏 | 默认NONE=0，合法0～15，详见选择表 | 编译命令可通过宏覆盖#ifndef默认；重编译/烧录/上电生效，无运行菜单、无Flash项 |
| BOARD_TEST_QUEUE_SIZE / uint宏 | 1024可用槽，每槽3×int16=6B，6144B | 编译固定，head/tail uint32自然回绕，以差值判满；无持久化 |
| s_frame | 16B，三个float32小端+00 00 80 7F | 前台逐帧复制；发送完成才释放槽，不与正常SCB共享封装 |
| USART1 | 460800 baud、8N1、仅TX | 板测IMU初始化覆盖生成默认值；无接收业务，硬件吞吐未验 |
| UART发送超时 | HAL参数10ms | 单帧阻塞发送；不是含ISR/时基阻塞的严格墙钟上限；部分帧不重试 |
| BOARD_TEST_OBSERVATION_CYCLES / uint64 | 480000000×60周期 | 逐次uint32 DWT差累加，达到后停IMU采集、排空冻结；无Flash项 |
| 统计窗口 / 显示周期 | 1000ms / 100ms | tick差无符号比较；窗口保留上一完整段，显示在临界区外；重上电清状态 |
| TIM15/TIM6/TIM7 | 1/2/5ms，优先级0/0/1 | 只启动选择所需IRQ，ADC DMA Stream0优先级2；无Wi-Fi DMA1/2 |
| 灰度扫描预算 | 700μs | 成功帧发布/失败保留旧帧；实际耗时待板测 |

## 本次源码对照（2026-11-07）

已逐分支核对 [board_test.c](../../App/system/board_test.c)、[选择宏](../../App/system/board_test_config.h)、[最小BSP](../../BSP/bsp_board_test.c)、[main分流](../../Core/Src/main.c)、[TIM6/7回调](../../BSP/bsp_runtime.c) 与 [TIM15 IRQ](../../BSP/bsp_sampler.c)。确认16个编译选择、ISR/前台owner、队列发布和三类停止结果与本页一致；未执行构建、烧录或实机测试，原11月6日软件证据和所有板测待验状态不变。
