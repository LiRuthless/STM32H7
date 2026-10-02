# Phase 13: 控制链结构重构 — 需求

> 2026-10-03 用户决定：仅修改 `SmartCar`；采用模块静态结构体与只读状态指针；包含状态机重复赋值，但保持每拍覆写及全部现有运行行为。无实车条件，实车验收保持待执行。

## 行为需求

- **FR-1 电感状态**：`track_sensor` 用唯一静态 `track_state_t` 保存原始值、滤波值、偏差、对称度和四个权重。`Track_GetState()` 返回生命周期覆盖整个程序的非空 `const track_state_t *`，`Track_SetWeights(const track_weights_t *)` 按横差、横和、竖差、竖差绝对值顺序写入权重。`read_adc`、`get_track_error`、`symmetry_adc` 的采样次数、算式、整数截断与分母为零返回值不变；`get_track_error` 同时保存返回的偏差。
- **FR-2 电机状态**：`motor` 用唯一静态 `motor_state_t` 保存左右轮目标/实测速度与里程、平均里程、基础速度、运行/空闲风扇值及滤波系数。`Motor_GetState()` 返回非空只读指针；`Motor_SetTargets`、`Motor_SetBaseSpeed`、`Motor_SetFanDuty`、`Motor_SetFanDutyIdle`、`Motor_ResetDistance` 是跨模块写入口，赋值时不新增限幅或校验。差速、编码器低通、方向极性、PWM、停车清零及再次起跑复位保持现状。
- **FR-3 IMU 状态**：`imu_proc` 用唯一静态 `imu_state_t` 保存三轴原始值、换算值、速度与角度；零偏、梯形积分历史、高通滤波器均为模块私有。`IMU_GetState()` 返回非空只读指针，`IMU_ZeroAngleX()` 执行原有 `angle_x=0` 的单字段清零。校准、读数、积分、复位的计算顺序和系数不变；休眠的 `read_accel_velocity` 保留且不接入。
- **FR-4 PID 状态**：`pid` 用唯一静态 `pid_state_t` 保存原有增益和可观测输出；`PID_GetState()` 返回非空只读指针，`PID_SetGain` 按指定增益单字段赋值，`PID_SetTrackGains`/`PID_SetAngleGains` 按旧代码的字段顺序连续赋值，不增加校验。方向、角度公式及现有输出类型转换与限幅顺序不变。
- **FR-5 双轮 PI 去重**：活动的 `PID_L_pos` 与 `PID_R_pos` 保留无参入口，各自调用同一个接收独立 `speed_pi_state_t *`、目标、反馈、死区及输出位置的内部函数。左右积分、上次目标及输出互不共享；换向、零目标、积分 ±7000、输出 ±7500、死区 ±500、`PID_ResetSpeed`/`PID_ResetAll` 的逐拍结果不变。休眠的增量式 `PID_L/R` 保留且不接入或删除。
- **FR-6 状态配置表**：`control.c` 七个主状态和 `roundabout.c` 四个现有分支使用静态常量配置表。每条记录含字段存在标志，仅在原分支原位置写入旧代码实际赋值的权重、基础速度、方向或角度增益；未赋值字段保持前一拍值。主状态先应用本态配置，再进入环岛子状态配置，随后执行原有计算与判断。数值、判断条件、状态转移、十字限幅和每拍覆写调参值的语义不变。
- **FR-7 调用方与持久化兼容**：`app/control/roundabout/element/menu/wireless/param/dashboard/datalog` 通过新接口访问上述状态，不保留对应散落的 `extern` 定义。菜单步进、串口协议与回显、参数镜像 v1/v2 的 52/56 字节布局和 CRC 范围、32 字节日志记录与 CSV 字段顺序不变。

## 约束与边界

- **TC-1**：C99，静态分配；状态指针仅作只读访问，不返回可写内部指针，不缓存指向临时对象的指针。
- **TC-2**：TIM6 2 ms、TIM15/TIM6 优先级及同级不抢占、TIM7 5 ms、BSP 接口与硬件配置不变；App 不新增直接 HAL 依赖。
- **TC-3**：Phase 10 再决定状态机与双档调参的生效优先级；Phase 09/11 再决定未启用元素和休眠代码去留。本轮不借结构重构修复其他已排期行为缺陷。
- **TC-4**：自动化验证可判定代码与数值等价；本轮无实车，所有需实车验证项在 `validation.md` 标为待上机，Phase 13 不勾选完成。
