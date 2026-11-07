# PWM 与方向物理输出

现行源码核对基线：8d5ab2faf68432ae03b2c1f91bc4593c7dc7cc3e（2026-11-07）。本文为实现事实说明，未重新批准算法或硬件参数；未执行固件/实机验收。

<a id="f26-pwm"></a>
## F26.PWM 无符号占空比与电平

BSP不保存电机闭环状态。BSP_PWM_Init依次启动TIM13_CH1、TIM4_CH3、TIM14_CH1，忽略HAL返回，保持已有CCR，不设方向也不启动控制环。SetDuty先拒绝ch>=CH_NUM，再把uint32 duty截到10000，读ARR，以uint32计算CCR=duty×(ARR+1)/10000（整数除法），最后写目标比较寄存器。0为零占空；10000得到ARR+1。索引和占空均为非负类型，不提供负值方向编码。

SetDirection仅左右轮有效，high非0写SET、0写RESET；风扇/其他枚举无动作，不顺带清PWM。App决定正反和方向电平，再调用BSP。带符号控制、左右不同极性和停车顺序见[电机执行](../control/motor-execution.md#f08-motor)，不能把BSP的high直接定义成所有轮的前进。

<a id="parameters"></a>
## 通道与参数

| 枚举 | 定时器 / 引脚 | 频率与范围 |
|---|---|---|
| BSP_PWM_MOTOR_L=0 | TIM13_CH1 / PA6；DIR PE7 | PSC=1，ARR=7058，约16999.58Hz（240MHz/(2×7059)推算） |
| BSP_PWM_MOTOR_R=1 | TIM4_CH3 / PD14；DIR PE15 | 同左；实际频率与极性待示波/空转确认 |
| BSP_PWM_FAN=2 | TIM14_CH1 / PA7；无DIR写入 | 同左 |

BSP_PWM_DUTY_MAX=10000为编译期万分比上限，CCR只在BSP换算。Core tim.c决定初始PSC/ARR/Pulse，运行每次SetDuty立即覆盖CCR；BSP不落盘，风扇/速度参数的Flash默认和运行覆盖归[参数服务](../services/parameters.md#f15)。TIM16/17备用PWM虽初始化，当前BSP三通道表不暴露它们，不能宣称已接入执行。LED背光TIM1另见[LCD](lcd.md#f29-lcd)。

来源：[Phase02 FR8/9](../../specs/features/02-bsp-platform/requirements.md)。实现：[bsp_pwm.c](../../BSP/bsp_pwm.c)、[bsp_pwm.h](../../BSP/bsp_pwm.h)、[Core tim.c](../../Core/Src/tim.c)、[main.h](../../Core/Inc/main.h)。本次核对表下标、截断/CCR顺序和无效通道副作用；PWM启动失败无状态、物理频率/极性/停车电平仍需实机验证，不新增通过结论。
