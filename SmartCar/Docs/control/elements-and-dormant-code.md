# 活动元素与休眠边界

现行功能说明，核对基线 `8d5ab2f`；2026-11-07源码对照。需求经[功能索引](../traceability.md)进入本页，再由实现表定位源码。
当前行为不等于全部已批准/实机验收，冲突见[已知问题](../known-issues.md)。

<a id="f09"></a>
## F09 活动电磁元素
<a id="f09-cross"></a>
### 十字与活动入口

仅普通TRACKING、CROSSROADS、REISLAND和ISLAND_R构成活动电磁主链；7态枚举/宏存在不等于7态均可达。

| 当前态/条件 | 后果 |
|---|---|
| TRACKING竖和>2800且symmetry_y<25 | 进入十字；同拍预环判定可覆盖此转移 |
| TRACKING横和>2800 | 按左右大小置标志，清私有角度/距离，进入预环 |
| TRACKING中DL1B<100mm | 置cask_flag抑制右环，不进入CASK |
| CROSSROADS | 方向限±10；竖和<2800回普通 |
| REISLAND竖和<300且right_flag且无抑制 | ISLAND_R；阶段LPREENTER，零输出/私有角度距离 |
| REISLAND距离>5000 | 回普通并清双标志/输出/角度；入环清距离后再判超时 |
| ISLAND_R阶段 | ≥8000 → >13000 → >32000 → >10000，最后回普通 |

<a id="f09-roundabout"></a>
### 右环阶段与比较顺序

右环进入前必须在普通态横和>2800时设置左右标志；左右相等走right_flag分支。该分支先清cask_flag、私有角度和距离，随后DL1B<100mm可以在同拍再次置抑制。预环只有竖和<300、right_flag且!cask_flag才入右环；没有进入左环的活动分支。预环成功后先清距离再重新读取距离判>5000，因此不能缓存入环前距离造成同拍超时覆盖。

| 当前右环子态 | 计算/执行意图 | 转移、复位与下一拍 |
|---|---|---|
| LPREENTER=1 | Measure，正常方向计算 | 平均滤波脉冲距离≥8000→TURN_LEFT=3，并清私有距离/发共享距离复位意图 |
| TURN_LEFT=3 | 直接steering=−30；无新方向Calculate，output保留先前值 | 距离>13000→IN=5并清距离 |
| IN=5 | Measure与方向Calculate | 距离>32000→OUT=11并清距离 |
| OUT=11 | Measure与方向Calculate | 距离>10000→NORMAL=0、主态TRACKING并清左右标志；本分支不额外清距离 |
| 其他子态 | 不产生新方向计算 | 无自动恢复或超时，保留既有状态，不能假定安全退出 |

主态表先合并，再按入分支时子态合并部分配置；转移后的配置在下拍读取。完整值及fields归 [F12](profiles.md#parameters)。十字进入拍仍以普通态分支计算；进入CROSSROADS分支后才把方向output限制±10，再赋给steering。预环入环/超时拍的s_zero_output在赋steering之后才将output置0，因此执行差速使用steering，日志监视使用output，两者不能混同。LPREENTER/TURN_LEFT是历史名称，活动主态是ISLAND_R。

<a id="parameters"></a>
### 元素参数、标度与状态来源

| 参数 | 当前值/单位/比较 | 来源、覆盖、生效与持久化 |
|---|---|---|
| 十字入口/出口竖和 | >2800进入，<2800退出；等于2800不转移 | filtered VL+VR，整数ADC算法标度，FSM编译常量；无Flash/菜单setter |
| 十字对称条件 | symmetry_y<25，百分比 | abs(VL−VR)×100/(VL+VR)，整数除，和0返回0；与竖和条件同时成立才进入 |
| 预环横和 | >2800 | HL+HR；同拍后于十字判断，可覆盖十字转移；无持久化 |
| 路障抑制 | DL1B<100mm | 普通态置cask_flag，不进入CASK；未加独立有效性判断，不宣称识别完整路障 |
| 右环入口竖和 | <300 | 与right_flag/!cask_flag共同判断；值为编译常量 |
| 预环超时距离 | >5000滤波累计脉冲 | 左右私有累计和/2，整数除；不等于mm/墙钟超时 |
| 入/出环阈值 | ≥8000 / >10000滤波累计脉冲 | control_profile.c的uint16只读变量；其他中段>13000/>32000由FSM常量定义；无运行保存 |
| 固定转向/十字限幅 | −30 / ±10，方向算法标度 | 当前分支当拍生效；不由方向增益或Flash项替代 |

FSM的s_em持有配置、主/子态、左右标志、cask抑制、左右相对脉冲、angle累计及reset意图；每拍Select先清本拍许可和reset标志，再累加angle_delta。Reset清整个s_em与方向历史，再仅恢复默认权重15/20/22/10。未enabled时只Measure，不推进主态；Calculate每次消费许可一次，重复调用不再次推进方向历史。以上状态全为RAM、不自动落盘。阈值的实车依据及脉冲→物理距离标定尚未验证，本次不重标定。

固定入环方向-30。LPREENTER/TURN_LEFT历史名称实际服务右环岛，不能据此认定左环已启用。
FSM持有相对左右距离、角度增量累计、配置和方向历史；未选中时只观察，不改PWM/实际PID/共享里程或角度。

### 验证前提

当前默认光电，验证“十字限幅/右环实际控制”须停车切电磁、有效32项标定和白线黑底/电磁双信号。光电模式记录电磁十字只证明观察，不证明输出执行。门限单位是原编码器计数，不自行换成mm；8000/10000 getter不是菜单调参入口。
既有独立FSM、配置等价、runtime_tuning刺激活动右环各阶段；实车十字/环岛及超时仍未验。等待期保护优先于元素，故障拍不继续电机执行。

**需求来源：** [P04 FR-01, FR-03, FR-04, FR-05, FR-06, FR-07, FR-08, FR-10](../../specs/features/04-element-fsm/requirements.md)；[P09 FR-05](../../specs/features/09-element-completion/requirements.md)；[P14 FR-10](../../specs/features/14-dual-mode-tracking/requirements.md)。实现入口：[App/control/electromagnetic_fsm.c](../../App/control/electromagnetic_fsm.c)、[App/control/control_profile.c](../../App/control/control_profile.c)。

<a id="f13"></a>
## F13 legacy与未接入元素
### 当前保留边界

legacy_control旧整体控制入口未接入活动调度；legacy_pid仍有停车方向预览，legacy_roundabout仍有起跑复位和旧内部调用，legacy_imu通过同owner包装保留。不能按整个legacy文件目录删代码。
左环岛/跷跷板/路障没有完整活动进入—处理—退出链。旧Element_Judge内部调用左入环和跷跷板判断，但不等于活动入口可达；不是“所有旧函数都词法零调用”。旧路障判断未被该汇总调用，Roundabout_JudgeEnteredAgain有条件转移，不是空函数。
旧增量PI、加速度积分与旧角度包装的历史/副作用按原边界保留；ResetAll不清休眠增量历史。启用/删除每元素须先明确输入、阈值、进入/退出、配置、超时、保护及日志，再用户决定。

### 规划和验证

P09三个元素与P10语义改造未实施，P11处置不等于全部已关闭。当前不把草案800/1000/60/20等当批准运行阈值。检查实际调用边、条件编译及复位；g3_control/g4_legacy_boundary的历史比较只证保留行为。未来设计确认、实车和最终逐项处置仍待完成。

**需求来源：** [P04 FR-09, FR-11](../../specs/features/04-element-fsm/requirements.md)；[P09 FR-01, FR-02, FR-03, FR-04](../../specs/features/09-element-completion/requirements.md)；[P11 FR-04, FR-11](../../specs/features/11-hardening-cleanup/requirements.md)；[P15 FR-09](../../specs/features/15-code-style-consistency/requirements.md)；[P17 FR-10](../../specs/features/17-app-architecture/requirements.md)。实现入口：[App/control/legacy_control.c](../../App/control/legacy_control.c)、[App/control/legacy_element.c](../../App/control/legacy_element.c)、[App/control/legacy_roundabout.c](../../App/control/legacy_roundabout.c)、[App/control/legacy_pid.c](../../App/control/legacy_pid.c)、[App/sensing/legacy_imu.c](../../App/sensing/legacy_imu.c)。

## 本页实现与模块反查

下表是当前源码定位；完整符号表仅在模块主功能页维护，协作行转向主条目。公共/私有辅助函数归所属功能，不把声明视为另一实现。Core生成部分只作边界，用户区按原标记维护。

| 文件 | 主功能 | 定义/配置定位 |
|---|---|---|
| [App/control/control_profile.c](../../App/control/control_profile.c) | F12 | 协作模块；完整符号表见[F12主条目](profiles.md#f12) |
| [App/control/electromagnetic_fsm.c](../../App/control/electromagnetic_fsm.c) | F09 | `ElectromagneticFSM_GetState`、`ElectromagneticFSM_Reset`、`ElectromagneticFSM_Accumulate`、`ElectromagneticFSM_GetDistance`、`ElectromagneticFSM_ClearDistance`、`ElectromagneticFSM_ClearAngle`、`ElectromagneticFSM_Measure`、`ElectromagneticFSM_Select`、`ElectromagneticFSM_Calculate`、`ElectromagneticFSM_Update` |
| [App/control/electromagnetic_fsm.h](../../App/control/electromagnetic_fsm.h) | F09 | 类型/布局/配置；关联同模块C定义 |
| [App/control/legacy_control.c](../../App/control/legacy_control.c) | F13 | `Control_UpdateLegacy`、`PID_UpdateAngle` |
| [App/control/legacy_control.h](../../App/control/legacy_control.h) | F13 | 类型/布局/配置；关联同模块C定义 |
| [App/control/legacy_element.c](../../App/control/legacy_element.c) | F13 | `Element_Judge`、`Element_JudgeStraight`、`Element_JudgeCrossroads`、`Element_JudgeCrossroadsExit`、`Element_JudgeTeeterboard`、`Element_JudgeTeeterboardExit`、`Element_JudgeCask`、`Element_JudgeCaskExit` |
| [App/control/legacy_element.h](../../App/control/legacy_element.h) | F13 | 类型/布局/配置；关联同模块C定义 |
| [App/control/legacy_pid.c](../../App/control/legacy_pid.c) | F13 | `PID_UpdateTrack`、`PID_UpdateIncrementalLeft`、`PID_UpdateIncrementalRight`、`LegacyPID_ResetTrackHistory` |
| [App/control/legacy_pid.h](../../App/control/legacy_pid.h) | F13 | 类型/布局/配置；关联同模块C定义 |
| [App/control/legacy_roundabout.c](../../App/control/legacy_roundabout.c) | F13 | `Roundabout_AbsFloat`、`Roundabout_Update`、`Roundabout_JudgePreentryLeft`、`Roundabout_JudgePreentryRight`、`Roundabout_JudgePreentryExit`、`Roundabout_JudgeEntryLeft`、`Roundabout_JudgeEntryRight`、`Roundabout_JudgeAhead`、`Roundabout_JudgeEntered`、`Roundabout_JudgeEnteredAgain`、`Roundabout_JudgePreExit`、`Roundabout_JudgeExit`、`Roundabout_JudgeExited`、`Roundabout_Judge`、`Roundabout_ResetRunState` |
| [App/control/legacy_roundabout.h](../../App/control/legacy_roundabout.h) | F13 | 类型/布局/配置；关联同模块C定义 |
| [App/sensing/legacy_imu.c](../../App/sensing/legacy_imu.c) | F07 | 协作模块；完整符号表见[F07主条目](../sensing/sensors-and-calibration.md#f07) |
