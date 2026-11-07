# 编码器原始读清

现行源码核对基线：8d5ab2faf68432ae03b2c1f91bc4593c7dc7cc3e（2026-11-07）。本文为实现事实说明，未重新批准算法或硬件参数；未执行固件/实机验收。

<a id="f26-encoder"></a>
## F26.ENCODER 硬件累计与方向

左右轮分别为TIM5 PA0/PA1与TIM3 PC6/PC7，TI12正交四倍频，两路输入分频DIV1、IC1/IC2Filter=6、PSC=0、ARR=65535。TIM5虽是32bit定时器，当前ARR同样65535，不能据硬件宽度宣称接口发布32bit里程。BSP_Encoder_Init依次清左右计数并启动全通道，HAL失败被忽略。

GetLeft/GetRight调用私有ReadClear：先把硬件CNT窄化为int16，再清CNT，最后按invert取反并转int16。读取有清零副作用，不能前台与采样器同时读同一轮。TIM15每1ms独占读清，存最近1ms值并累计；TIM6每2ms消费累计，详见[采样器](sampler.md#f29-sampler)。脉冲至业务速度、滤波及里程换算主说明归[电机执行](../control/motor-execution.md#f08-motor)。

<a id="parameters"></a>
## 参数及限制

| 参数 | 真实值/标度 | 来源、覆盖、生效、持久化 |
|---|---|---|
| BSP_ENCODER_L_INVERT / R_INVERT | 编译宏1 / 0 | bsp_encoder.h；左取反右保持，构建后生效，无运行/Flash覆盖；前进正需实测接线确认 |
| CNT发布类型 / ARR | int16，-32768～32767脉冲；ARR=65535 | Core tim.c+bsp_encoder.c；先窄化再取反，溢出不饱和，无错误状态 |
| 读取周期 / 消费周期 | 1ms / 2ms | BSP TIM15 / TIM6，同优先级0保证累计读清不被采样抢占 |

模块不保存业务速度，也不检出断线或反馈过大；读CNT后清零之间到达的硬件边沿不能由软件文档保证无损。异常大计数及int16最小值取反仍受窄化边界限制，不能把“正常速度不溢出”说成全输入证明。

来源：[Phase02 FR10](../../specs/features/02-bsp-platform/requirements.md)、[Phase03](../../specs/features/03-track-following/requirements.md)。实现：[bsp_encoder.c/h](../../BSP/bsp_encoder.c)、[Core tim.c](../../Core/Src/tim.c)。本次核对调用副作用、顺序和两个方向宏；实际方向、每转脉冲及距离标定未由本次确认。[F33编码器](isolated-board-tests.md#f33-encoder)保留真实板测待验。
