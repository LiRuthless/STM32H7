# 状态部分配置

现行说明核对基线：`8d5ab2faf68432ae03b2c1f91bc4593c7dc7cc3e`（2026-11-07）。本次仅源码对照，不新增固件或实机验收。

<a id="f12"></a>
## F12 持有者与应用顺序

control_profile.c 持有静态只读主态/环岛表和距离阈值，电磁FSM的 s_em.configuration 保存合并结果。fields 位 W=WEIGHTS=1、B=BASE=2、T=TRACK=4、A=ANGLE=8。Control_MergeProfile依W/B/T/A顺序只复制置位组，最后dst.fields |= src.fields，未选组保留此前值。Control_ApplyProfile同顺序依次写 Track 权重、Motor基础速度、PID方向增益、PID角度增益，不清算法历史。

每个启用观察拍先合并当前主态，再在右环分支合并当前子态，之后计算方向；转移发生在分支内部，因此新主态表通常在下一观察拍合并。Select生成观察结果，Tracking_Execute只应用选中电磁配置；光电不把电磁特殊状态配置写入实际执行。

<a id="parameters"></a>
## 完整主态值矩阵

权重顺序为结构字段(x,xx,y,abs)，方向顺序(kp,k2p,kd,k2d)，角度顺序(kp,kd,kg)；“沿用”表示该字段组未置位，即使C初始化器为0也不写0。基础速度为int16脉冲/2ms，增益float为既有每拍算法标度，无已证明的稳定范围。

| 主态 / 编号 | fields | 权重 | 基础速度 | 方向 | 角度 |
|---|---|---|---|---|---|
| TRACKING / 0 | W+B+T=7 | 15,20,22,10 | 245 | 2,0.008,15,1 | 沿用 |
| ISLAND_L / 1 | W+A=9 | 15,20,22,10 | 沿用 | 沿用 | 2,1.2,0 |
| ISLAND_R / 2 | W+B+A=11 | 15,20,22,10 | 150 | 沿用 | 2,1.2,0 |
| TEETERBOARD / 3 | B+T=6 | 沿用 | 100 | 4.5,0,6.2,0.9 | 沿用 |
| CROSSROADS / 4 | W+B+T=7 | 15,20,2,10 | 240 | 2,0.008,15,1 | 沿用 |
| CASK / 5 | W+B+T=7 | 15,20,2,5 | 210 | 2,0.008,15,1 | 沿用 |
| REISLAND / 6 | W+B+T=7 | 20,10,1,10 | 180 | 2,0.008,15,1 | 沿用 |

Control_GetKernelProfile 对编号≥7回退TRACKING。表项存在不表示左环、跷跷板、路障已接入。

## 完整环岛子态值矩阵

| 子态 / 编号 | fields | 权重 | 基础速度 | 方向 | 角度 |
|---|---|---|---|---|---|
| LPREENTER / 1 | W+B+T=7 | 20,10,1,10 | 150 | 2,0.008,15,1 | 沿用 |
| TURN_LEFT / 3 | 0 | 沿用 | 沿用 | 沿用 | 沿用 |
| IN / 5 | B+T=6 | 沿用 | 150 | 2,0.008,15,1 | 沿用 |
| OUT / 11 | W+B+T=7 | 20,10,1,10 | 150 | 2,0.008,15,1 | 沿用 |
| 其他状态 | 0（TURN空配置） | 沿用 | 沿用 | 沿用 | 沿用 |

这些值是编译期表值，无菜单setter、无Flash持久化。uint16 s_enter_distance=8000、s_out_distance=10000 是滤波编码器累计脉冲阈值，getter只读；入环用≥8000，出环用>10000。右环中间>13000/>32000及固定转向−30由FSM持有，见 [元素主说明](elements-and-dormant-code.md#f09-roundabout)。重跑FSM清配置后权重恢复15/20/22/10，其他组从零状态逐拍合并。

## 与持久参数、临时覆盖的关系

上电/档位模式应用先设置活动值；电磁状态表随后按fields覆写，未选组沿用。菜单修改不保证永久压过运行表。Tuning_SelectControl先判断本拍普通赛道/模式/保护等上下文；事务有效时Calculate用覆盖方向值，Execute复制观察配置到局部execution，再覆写T与B并置对应位。临时值不回写只读表或电磁观察配置，也不自动持久化。光电使用当前bank与其临时覆盖，不套用电磁主态速度245。详见 [临时调参](../services/parameters-and-tuning.md#f18)。

[Phase10](../../specs/features/10-control-consistency/requirements.md)的全局“基准+用户调参”合成尚未实施；[Phase13](../../specs/features/13-control-structure/requirements.md) FR-06建表和Phase16普通赛道显式覆盖不等于该目标完成。非法主态回退与未知子态空配置是当前行为，不代表新增策略批准。

主实现：[control_profile.c](../../App/control/control_profile.c) / [.h](../../App/control/control_profile.h)；合并调用：[electromagnetic_fsm.c](../../App/control/electromagnetic_fsm.c)，执行：[tracking.c](../../App/control/tracking.c)。本次逐行对照7主态、4子态、fields宏、应用顺序和回退；无构建/行为测试，实车门限与覆盖策略仍按阶段待验。
