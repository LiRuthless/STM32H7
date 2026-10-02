# Phase 13: 控制链结构重构 — 实施计划

## 顺序与定位

1. 从已提交的 Phase 11 非元素整改 HEAD 派生独立宪章分支，只提交 `roadmap.md` 的 Phase 13 排期；随后派生 `feature/13-control-structure`。先提交本规约三件套及 Phase 03/10/11 重叠说明，再修改代码。
2. **FR-1/2/3/4**：分别在 `track_sensor.c/.h`、`motor.c/.h`、`imu_proc.c/.h`、`pid.c/.h` 定义模块静态状态及只读 getter；在 `filter.c/.h` 将仅由 IMU 使用的高通实例移入 IMU 模块。getter 地址固定、无分配；写接口只改被指定字段，保留旧赋值的即时生效与类型。
3. **FR-5**：`pid.c` 中以两个私有 `speed_pi_state_t` 实例取代活动 PI 的左右历史变量。内部公共计算函数接受状态指针与输入值，左右旧函数作为薄入口；`Motor_EmergencyStop` 与再次起跑复位仍调用现有 PID 复位入口。
4. **FR-6**：`control.h/.c` 声明配置结构及 `Control_ApplyProfile`，字段存在位分别控制权重、基础速度、方向增益、角度增益。`control.c`、`roundabout.c` 各有静态常量表，按旧分支位置调用；不把未写字段补成默认值，也不提前到状态判断之前执行。
5. **FR-7**：迁移 `app.c`、`control.c`、`roundabout.c`、`element.c`、`menu.c`、`wireless.c`、`param.c`、`dashboard.c`、`datalog.c` 的相关读取/写入。`param_store_v1_t`/`param_store_t` 与 `datalog_record_t` 只改取值来源，不改定义或序列化。最后移除旧全局量及其 `extern` 声明，同步 CHANGELOG 和各规约接口描述。

## 数据流与风险

- 2 ms 控制链仍为采样/IMU → 状态配置 → 偏差/方向 → 差速目标 → 编码器 → 双轮 PI → PWM。TIM15 日志只读模块状态；TIM7 与停车菜单保留各自调用时机。
- 表配置采用字段存在位，是因为 `KERNEL_TEETERBOARD` 不写权重、`KERNEL_ISLAND_L` 不写基础速度、`ISLAND_IN` 不写权重；完整结构体覆盖会改变沿用上拍值的行为。
- 参数镜像仍是独立磁盘格式，不以运行态结构体直接写 Flash。调用方不持有可写指针，避免意外跨模块修改；对照测试覆盖结构迁移后的数值和复位行为。
- 每组变更后运行 GCC 构建并记录结果；验证失败时只回退该组，先恢复已确认的逐拍值与执行顺序。
