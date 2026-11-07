# 光电状态机与双模式协调

现行源码对照基线：`8d5ab2f`，2026-11-07；当前行为不等于全部已批准或实机已验收。

<a id="f10"></a>
## F10 光电状态机
### 状态与输入输出

`OPTICAL_WAIT=0/TRACKING=1/LOST=2/STOPPED=3`。复位WAIT，控制计数>1000且双信号有效才TRACKING；运行灰度失线LOST并请求停车，LOST锁存到复位，其他停车转STOPPED。
仅普通方向PID，不识别光电十字/环岛。输入是归一化灰度误差与当前光电bank；输出控制意图及私有方向历史，选中模式由tracking执行。电磁处于特殊元素并不授权它控制光电输出。

### 验证

核对1000/1001、有效/失线/复位、未选中观察及输出隔离；dual独立FSM子项已有历史记录。当前标定-1下不能以正式固件直接跑该链；硬件循迹待验。

**需求来源：** [P14 FR-09](../../specs/features/14-dual-mode-tracking/requirements.md)。实现入口：[App/control/optical_fsm.c](../../App/control/optical_fsm.c)。

<a id="f11"></a>
## F11 双FSM与唯一执行
### 完整拍协调

tracking拥有完整控制拍与独立监视量；监视track_out不等于执行快照字段，不能合并。先完成双传感及保护，再同帧推进光电/电磁观察，分别用显式输入与独立方向历史计算，只选定模式提交Motor/PID及共享复位意图。
未选中电磁只能累加私有左右距离及angle_delta；选择后才可应用共享里程/角度复位。模式切换、跨模块reset和保存属于lifecycle，不属于纯setter。控制完成序号、调参提交/事件、完整日志发布按scheduler原时点顺序。

### 并发与验证

TIM6更新控制快照，TIM15同优先级不抢占后复制；const getter没有跨ISR事务保证。Tuning前台快照使用原短临界区，不扩大锁复制大缓冲。
审查唯一执行分支、观察路径无Motor/PID/IMU/PWM副作用、错误出口与记录一致性；既有真实C/人工审查组合证据不能把只比较Motor/PID内存扩大成“所有硬件调用均探测为零”。物理时序待验。

**需求来源：** [P14 FR-07, FR-11](../../specs/features/14-dual-mode-tracking/requirements.md)；[P17 FR-11, FR-12](../../specs/features/17-app-architecture/requirements.md)。实现入口：[App/control/tracking.c](../../App/control/tracking.c)、[App/system/scheduler.c](../../App/system/scheduler.c)。


<a id="configuration"></a>
## 输入与配置条件

| 条件/状态 | 来源及当前行为 |
|---|---|
| 默认模式/档位 | tracking.c静态mode=OPTICAL，gear零初值为低档；setter本身不执行跨模块复位或参数保存 |
| 档位有效性 | Tracking_SetGear仅gear<2写入，否则保持；当前低/高档分别取对应模式参数bank |
| 模式修改 | Tracking_SetMode直接写uint8，无内部合法性检查；外部入口负责停车门禁与验证，不能称setter安全切换事务 |
| 电磁观察许可 | running且control_ticks>APP_RUN_DELAY_COUNT（1000）；未许可仍Measure，不推进元素或方向历史 |
| 光电有效性 | gray.line_valid，且四路filtered总和>300；等待条件还需running和time>1000 |
| 光电LOST优先级 | 已LOST先返回，锁存直到reset；running或stop_reason非零且gray无效时置LOST并输出0 |
| 光电其他停车 | stop_reason非零，或未运行但此前TRACKING，置STOPPED并输出0；其他等待情形不强制改WAIT |
| 调试普通赛道判据 | 电磁kernel=TRACKING且roundabout=NORMAL；Tuning_SelectControl还检查模式、运行、保护等上下文 |

1000为2ms控制计数，不是毫秒。未开跑/等待状态和零输出不替代全局双保护；正常起跑仍需全部32项标定有效，保护判据与动作归 [system](../system/runtime-and-safety.md#f02-start)。

## 一拍内选择、计算、提交

Tracking_Observe先构造同拍输入：电磁取滤波四路、Lifecycle距离mm、IMU accel[0]和角度增量；光电取gray误差/有效性、选档光电bank与相同运行时刻。先ElectromagneticFSM_Select，再OpticalFSM_Select。基础方向增益按选中模式取opt.gains或e.configuration.track；电磁尚未enabled时取活动PID增益，不能把未初始化的观察配置当当前控制值。

随后构造tuning_context（运行、模式/档位、普通赛道、保护位、control_ticks、control_sequence），Tuning_SelectControl固定本拍覆盖；GetControl读取覆盖快照。两个FSM的Calculate各消费一次许可，仅选中模式能收到覆盖增益；光电基础速度覆盖也只传入选中光电。最后发布电磁权重/误差/对称度、主态/子态镜像和active_error。电磁观察配置不因调试事务被回写。

Tracking_Execute光电分支要求OPTICAL_TRACKING，再依次写base_speed、方向监视值、分配左右目标。电磁分支复制configuration到局部execution，有调试覆盖时改方向/速度并置fields；应用剖面→按e.steering差速→按标志重置共享距离/角度→写e.output监视值。e.steering和e.output在预环过渡拍可以不同，不能把日志监视输出当实际差速输入。最终写完整拍s_tracking.track_out；Motor_UpdateControl实际发PWM由scheduler的后续固定调用点执行。

两FSM分别持方向历史；ElectromagneticFSM_Select在所有路径先累加angle_delta，未选中也保留私有距离/角度。Tracking_AccumulateMotion把左右滤波速度积入电磁相对里程，未选中不得因此复位共享Motor/IMU。Control_ResetRunState清监视镜像、输出、完成序号、保护位并分别reset两个FSM；不清模式/档位。

## 所有权、失败与完成边界

s_tracking与各FSM静态状态在TIM6写；前台选择受Lifecycle约束。Tracking_AddProtection在原短临界区OR累加，完整序号uint32自然回绕；const getter不提供复合一致性。TIM15与TIM6同优先级、互不抢占是日志读取完整拍的前提，不能任意调整优先级。

Tracking_Stopped用当前灰度及累计灰度保护重建停止输入，只推进光电停止/失线并清两种输出镜像，不推进电磁执行。保护先退出的拍不应继续电机闭环；立即停车和前台收尾详见system。所有运行状态和完成序号都是RAM状态，无直接Flash持久化；bank参数恢复、模式切换保存见 [参数主题](../services/parameters.md#f15)。

主实现：[tracking.c](../../App/control/tracking.c)、[optical_fsm.c](../../App/control/optical_fsm.c)；协作：[scheduler.c](../../App/system/scheduler.c)、[参数bank](../../App/services/parameter_bank.c)、[临时调参](../services/runtime-tuning.md#f18)、[状态矩阵](profiles.md#f12)。本次核对所有Select/Calculate/Execute分支、reset与setter边界；不把历史双FSM软件证据扩大为当前实机时序或所有硬件副作用探测通过。
