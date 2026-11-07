# ADC 与灰度原码采集

现行源码核对基线：8d5ab2faf68432ae03b2c1f91bc4593c7dc7cc3e（2026-11-07）。本文为实现事实说明，未重新批准算法或硬件参数；未执行固件/实机验收。

<a id="f26-adc"></a>
## F26.ADC 连续扫描与返回语义

ADC1由Core初始化为16bit、单端、连续十rank扫描，软件触发，无过采样，16.5周期采样时间，DMA循环保留溢出前数据。BSP_ADC_Init先偏移校准再启动DMA，均忽略HAL返回，不提供初始化失败状态。s_adc_buf为10个volatile uint16，固定D2 SRAM1 0x30000000；DMA1 Stream0半字传输、存储递增、高优先级。D-Cache关闭是当前可见性的前提。

BSP_ADC_Read(ch)在枚举达到CH_NUM时返回0，否则读对应半字并右移4位，输出0～4095。0既可能是合法输入也可能是索引越界，接口没有独立有效位。普通读取是最近DMA值，不保证新转换，也不保证十通道来自同一扫描周期。电感业务滤波/偏差见[F05](../sensing/electromagnetic.md#f05)，标定与质心见[F06](../sensing/grayscale.md#f06)，电池判断见[系统保护](../system/runtime-and-safety.md#f03)。

| DMA下标 / rank | BSP枚举 | ADC1通道 | 引脚 / 用途 |
|---|---|---|---|
| 0 / 1 | IND_H_L | INP10 | PC0 横电感左 |
| 1 / 2 | IND_V_L | INP11 | PC1 竖电感左 |
| 2 / 3 | IND_V_R | INP18 | PA4 竖电感右 |
| 3 / 4 | IND_H_R | INP8 | PC5 横电感右 |
| 4 / 5 | GRAY | INP4 | PC4 灰度复用输出 |
| 5 / 6 | KEY1 | INP14 | PA2 方向/确定分压键 |
| 6 / 7 | KEY2 | INP15 | PA3 其余分压键 |
| 7 / 8 | VBAT | INP9 | PB0 电池原码 |
| 8 / 9 | SPARE1 | INP5 | PB1 备用 |
| 9 / 10 | SPARE2 | INP19 | PA5 备用 |

<a id="gray-raw"></a>
## 灰度选择、新转换与完整帧发布

bsp_gray驱动PE4/PE6/PE0/PE1（IO1～4，对应地址位0～3），一次BSRR同时置位/复位，顺序选通0～15。BSP_Gray_Init启用自由运行DWT、选0并复位s_sequence，不清DWT计数。每通道只选址一次，再调用BSP_ADC_ReadGrayFresh五次：每次先观察NDTR>5（rank5尚未写），再等待NDTR<=5（rank5写完），以确保消费选址后新转换；不是重复读取同一DMA缓存。

整帧共享一个DWT起点。每次fresh先检查空输出/句柄，再在轮询中检查周期差达到预算、DMA使能、TE/DME/FE错误、NDTR是否大于10。失败分别返回ARG/TIMEOUT/DMA_ERROR，不写输出。每路丢前三个新转换，对后两个先整数求和除2，保留12bit原码；缩放与标定由sensing负责。16路全部完成且scan_cycles<336000，才递增sequence并把局部next结构复制给out。失败不发布半帧、不递增序号；GPIO仍停在失败通道，无自动回零。结构复制本身不是跨ISR原子快照，调用者负责串行上下文；正常采集在停车前台或TIM6，复位不得与扫描并发。成功序号从1开始，自然回绕。

<a id="parameters"></a>
## 参数、来源和生效

| 符号/配置 | 类型/单位与值 | 来源、覆盖、生效及持久化 |
|---|---|---|
| BSP_ADC_CH_NUM / 分辨率 | 枚举10；16bit右移4到12bit | bsp_adc.h/Core adc.c；编译配置、初始化生效，无Flash覆盖 |
| PLL2 / ADC prescaler | M6/N40/P2/Q1/R2；ASYNC_DIV2 | Core adc.c；按当前时钟源配置，实际采样率未测，不用名义值推定帧耗时 |
| BSP_GRAY_BUDGET_CYCLES | uint32，336000 cycle=700us@480MHz | bsp_time.h；整帧共同上限，达到即失败，无运行覆盖/持久化 |
| 通道/样本数 | uint8，16×5；丢3均2 | bsp_gray.c；80个新转换/帧，先均值后业务缩放，无Flash关系 |
| raw / sequence / scan_cycles | uint16[16] / uint32 / uint32 | 完整成功时发布；raw 0～4095，时间为周期数；非持久状态 |

来源：[Phase02 FR6/7](../../specs/features/02-bsp-platform/requirements.md)、[Phase14 FR2～4/8](../../specs/features/14-dual-mode-tracking/requirements.md)。实现：[bsp_adc.c](../../BSP/bsp_adc.c)、[bsp_adc.h](../../BSP/bsp_adc.h)、[bsp_gray.c](../../BSP/bsp_gray.c)、[Core adc.c](../../Core/Src/adc.c)、[main.h引脚](../../Core/Inc/main.h)。本次逐项核对枚举/rank/引脚、NDTR门限、平均及失败发布顺序；R07原LQ样本缺失、电气/新转换真实性、700us及标定实机仍待验，[F33灰度板测](isolated-board-tests.md#f33-gray)不等于已验通过。
