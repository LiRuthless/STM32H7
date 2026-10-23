# App 五大类目标架构

| 类别 | 责任 | 禁止的反向依赖 |
|---|---|---|
| system | 初始化/前台/ISR编排、运行生命周期、安全协调 | 算法和服务核心不可依赖具体调度实现 |
| sensing | 采样数据处理、标定、滤波与传感器状态 | 不依赖HMI、通信、日志 |
| control | 双模式/FSM、控制器、电机执行、调参事务 | 控制核心不依赖SCB格式或菜单 |
| services | 参数持久化、记录、日志、协议、上传与命令适配 | 参数存储不依赖参数使用者 |
| hmi | 按键、菜单状态、绘制及仪表盘 | 不直接写业务内部状态 |

根 app.c 保留五个App外部入口及总装；app_config.h为编译配置；app_types.h仅放确有跨业务使用的类型。
目标模块：system的scheduler/lifecycle/safety；sensing的track_sensor/gray_sensor/imu_proc/battery/filter/legacy_imu；
control的tracking/control_profile/optical_fsm/electromagnetic_fsm/direction_controller/angle_controller/speed_controller/motor/tuning和legacy_control/element/roundabout/pid；
services的parameter_bank/parameter_store/run_record/logger/log_store/log_export/scb_frame/scb_json/scb_command_codec/command_dispatch/scb_link/telemetry/wireless；
hmi的menu/menu_keys/menu_render/dashboard。所有模块均直接位于其大类中，不建子目录。

TIM15消费采样结果并生产记录；TIM6更新传感器、双FSM、选中控制输出和完整拍快照；TIM7处理辅助保护。
TIM15/6同级不抢占前提保持。立即停车先切执行口，日志刷写和保存留前台。上传队列独立于Flash队列。
前台命令由codec转换业务请求，dispatch路由，tuning在完整控制拍提交，前台返回结果。

各组迁移必须先定位唯一状态所有者和每个写入上下文；不能复制状态解决跨文件可见性。
G1只是物理归类，原control/pid/param/datalog等文件名保留到后续所属组拆分，不能宣布目标架构完成。
