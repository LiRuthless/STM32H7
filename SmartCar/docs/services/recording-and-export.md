# 运行记录、存储与导出

现行功能说明，核对基线 `68cb508`；2026-10-31迁移。需求经[功能索引](../traceability.md)进入本页，再由实现表定位源码。
当前行为不等于全部已批准/实机验收，冲突见[已知问题](../known-issues.md)。

<a id="f19"></a>
## F19 日志格式容量
### 会话与记录

日志区`0..0x7FEFFF`，参数区从0x7FF000起。日志magic=0x324C4353/version2，头256B、记录96B，前256B为头；capacity=floor((8384512-256)/96)=87336条，1ms约87.336s。头CRC16从字节8覆盖248B，记录保留区14B写零。
头包含会话mode/gear、周期、16路黑白标定及权重、当前档两模式bank、共享PI/风扇、700/900us预算和容量；不是动态运行参数历史。记录含1ms编码器和原始陀螺换算整数，2ms控制/灰度序号、两模式误差/状态、保护与首因。
run_record唯一持96B控制快照和256B头；TIM6结尾发布，TIM15复制并补本拍采样。记录允许相邻灰度/控制序号相同，不压缩时间索引。当前头/记录布局在下表，SCB直接复用这些原字节，不另造上传记录格式。

**需求来源：** [P07 FR-01, FR-02, FR-06](../../specs/features/07-datalog/requirements.md)；[P09 FR-06](../../specs/features/09-element-completion/requirements.md)；[P11 FR-06](../../specs/features/11-hardening-cleanup/requirements.md)；[P14 FR-17](../../specs/features/14-dual-mode-tracking/requirements.md)。实现入口：[App/services/datalog.h](../../App/services/datalog.h)、[App/services/run_record.c](../../App/services/run_record.c)。

<a id="f20"></a>
## F20 队列前台写入与故障
### 单生产者/消费者

1024槽分配98304B，保留一个空槽，可积压1023条；TIM15生产、前台消费，内容写完DMB后发布head，Flash成功后才发布tail。不在TIM7刷Flash。
每次运行前台最多8条/768B，运行不足8条不写，停车允许短尾包，最多128次服务及喂狗。按256B页拆写，跨未擦扇区懒擦；不无限追赶。上传复制走独立队列，拥塞不丢Flash记录。

### 起跑门禁与故障

DATALOG_ENABLE必须1，DATALOG_DIV必须1；否则拒绝起跑。Flash不可用、无效标定、头写/回读/比较失败不建立有效会话。
accepted为生产权威总数，检查已接受容量避免并发叠加written重复统计；第87336条接受后请求满容量停车。队满该条计rejected并停车，写错首fault锁存、不重试，成功才计written。错误后不能保证所有尾包落盘；导出只用成功连续前缀。
核对1023/1024、末条、队满拒绝、写错不推进tail/不重试、停车短尾包；datalog_v2/g5_boundaries为历史软件证据。至少约96KB/s记录负载及擦写开销需实体Flash吞吐实测，不由RAM预算证明。

**需求来源：** [P01 FR-08](../../specs/features/01-runtime-architecture/requirements.md)；[P02 FR-24](../../specs/features/02-bsp-platform/requirements.md)；[P07 FR-03, FR-04, FR-05, FR-09](../../specs/features/07-datalog/requirements.md)；[P08 FR-05](../../specs/features/08-storage-chain-fix/requirements.md)；[P14 FR-18, FR-19, FR-20](../../specs/features/14-dual-mode-tracking/requirements.md)；[P17 FR-08](../../specs/features/17-app-architecture/requirements.md)。实现入口：[App/services/logger.c](../../App/services/logger.c)、[App/services/log_store.c](../../App/services/log_store.c)。

<a id="f21"></a>
## F21 CSV与会话边界
### CSV与会话身份

停车串口d导出当前有效头元信息/标定、固定57列、成功落盘记录、状态尾行和EOF；c只复位日志首区/前沿，不起跑、不伪造标定。每行含tick、电感、1ms轮速、活动误差/输出、陀螺整数、测距/电池/电磁状态、16路gray和16路q、双序号/误差/保护/模式/光电状态/有效/首因；q按会话标定重算。
无有效当前会话报告原因并EOF，不造数据。没有掉电恢复条数、扫描残留Flash、多会话或续录；上电RAM有效标志清除，即使Flash非易失也不能导出为上次有效会话。新起跑覆盖上次会话；旧32B日志须升级前导出，当前不解析。

### 观察与验证

CSV没有angle_x/accel_x，不据其验证姿态清零。运行调参revision不在Flash记录内，完整调参历史须结合电脑事件JSONL。
核对表头57列与所有字段顺序/单位、坏头/读取失败、无效会话和EOF；g5历史对照证明相同字节，不独立证明列定义正确。串口吞吐/Flash读错和实车日志仍待验。

**需求来源：** [P07 FR-07, FR-08](../../specs/features/07-datalog/requirements.md)；[P14 FR-21](../../specs/features/14-dual-mode-tracking/requirements.md)。实现入口：[App/services/log_export.c](../../App/services/log_export.c)。

<a id="f22"></a>
## F22 二进制流式导出
<a id="f22-export"></a>
### 顺序导出与借用缓存

GET_LOG_META只读内存缓存，不读写Flash。停车且收尾完成才LOG_BEGIN，冻结written成功前缀，total_bytes=256+96*record_count；一个时刻一个transfer_id，导出期间所有起跑/清日志入口拒绝。
LOG_READ要求身份和顺序offset，length1～960；LOG_DATA为offset(u32)、length(u16)、reserved0(u16)+原始字节。缓存968B，借出的bytes只在本次前台调用内有效，dispatch同调用复制。只重取最近块可复用，不重读Flash、不重复累计CRC、不续租；新的有效读取推进offset、CRC32及10s进展租期。
全部字节交付后LOG_END返回整体zlib兼容CRC32并释放；乱序/越界/不存在/忙分别返回业务枚举，由dispatch映射SCB状态。失败先长度清零，输出指针保持；部分读错误取消导出并保留原fault规则。
会话改变立即取消；无新有效读取10s超时释放。STM32不能直接感知TCP断开，桥接停止转发时依超时释放，不自行加LOG_CANCEL或重置SPI会话。

### 验证

核对总长/CRC、960极值、重复块/租期、全入口互斥及失败输出；spi_log_export/g5_boundaries有真实C历史证据。不是当前有完整三端实机闭环；网络/Flash持续导出仍待验。

**需求来源：** [P16 FR-19, FR-20, FR-21](../../specs/features/16-spi-wifi-link/requirements.md)。实现入口：[App/services/log_export.c](../../App/services/log_export.c)、[App/services/command_dispatch.c](../../App/services/command_dispatch.c)。

## 本页实现与模块反查

下表是当前源码定位；公共/私有辅助函数归所属功能，不把声明视为另一实现。Core生成部分只作边界，用户区按原标记维护。

| 文件 | 主功能 | 定义/配置定位 |
|---|---|---|
| [App/services/command_dispatch.c](../../App/services/command_dispatch.c) | F23 | `CommandDispatch_MapResult`、`CommandDispatch_ToWire`、`CommandDispatch_ToBusiness`、`CommandDispatch_Begin`、`CommandDispatch_Send`、`CommandDispatch_Position`、`CommandDispatch_ReadBank`、`CommandDispatch_QueryParams`、`CommandDispatch_QueryStatus`、`CommandDispatch_QueryResult`、`CommandDispatch_QueryLogMeta`、`CommandDispatch_MapLogResult`、`CommandDispatch_LogCommand`、`CommandDispatch_HandleCommand`、`CommandDispatch_Set`、`CommandDispatch_Switch`、`CommandDispatch_ResetSession`、`CommandDispatch_Init`、`CommandDispatch_InitLink`、`CommandDispatch_ServiceLink`、`CommandDispatch_Service`、`CommandDispatch_QueueTuningEvent`、`CommandDispatch_PublishRunStarted`、`CommandDispatch_PublishRunStopped` |
| [App/services/datalog.h](../../App/services/datalog.h) | F19 | 类型/布局/配置；关联同模块C定义 |
| [App/services/log_export.c](../../App/services/log_export.c) | F22 | `Datalog_CancelExport`、`Datalog_ServiceExport`、`Datalog_IsExporting`、`Datalog_GetExportStatus`、`Datalog_BeginExport`、`Datalog_ReadExport`、`Datalog_EndExport`、`Datalog_Dump` |
| [App/services/log_export.h](../../App/services/log_export.h) | F22 | 类型/布局/配置；关联同模块C定义 |
| [App/services/log_store.c](../../App/services/log_store.c) | F20 | `LogStore_Erase`、`LogStore_WriteHeader`、`LogStore_Write`、`LogStore_Read` |
| [App/services/log_store.h](../../App/services/log_store.h) | F20 | 类型/布局/配置；关联同模块C定义 |
| [App/services/logger.c](../../App/services/logger.c) | F20 | `Logger_LatchFault`、`Datalog_RequestFaultStop`、`Datalog_SetStopSummary`、`Datalog_GetStatus`、`Datalog_IsActive`、`Datalog_GetCount`、`Datalog_Init`、`Datalog_Start`、`Datalog_Clear`、`Datalog_GetMeta`、`Datalog_Push`、`Datalog_Service`、`Datalog_Flush`、`Datalog_Stop`、`Logger_GetStopReason`、`Logger_HasHeader` |
| [App/services/logger.h](../../App/services/logger.h) | F20 | 类型/布局/配置；关联同模块C定义 |
| [App/services/run_record.c](../../App/services/run_record.c) | F19 | `Datalog_CalculateCrc16`、`Datalog_SetProfile`、`RunRecord_ResetSnapshot`、`RunRecord_CheckCalibration`、`RunRecord_BuildHeader`、`RunRecord_GetHeader`、`Datalog_PublishControl`、`RunRecord_CaptureSample`、`RunRecord_IsHeaderValid` |
| [App/services/run_record.h](../../App/services/run_record.h) | F19 | 类型/布局/配置；关联同模块C定义 |

## datalog_header_t 当前布局

小端目标布局；源头为实际声明及偏移/尺寸断言。单位和语义随字段保留。

| 偏移 | 类型/数量 | 字段 | 说明 |
|---:|---|---|---|
| 0 | uint32_t | `magic` | 原日志魔数，0x324C4353。 |
| 4 | uint16_t | `version` | 固定日志版本 2，不随 C 符号改名升级。 |
| 6 | uint16_t | `crc16` | 从字节 8 开始覆盖 248 B 的原 CRC。 |
| 8 | uint16_t | `header_size` | 头部固定 256 B。 |
| 10 | uint16_t | `record_size` | 记录固定 96 B。 |
| 12 | uint16_t | `sample_period_us` | 采样周期，1000 微秒。 |
| 14 | uint16_t | `control_period_us` | 控制周期，2000 微秒。 |
| 16 | uint8_t | `mode` | 会话模式，0 电磁、1 光电。 |
| 17 | uint8_t | `gear` | 会话档位，0 低、1 高。 |
| 18 | uint16_t | `channel_count` | 灰度通道数，16。 |
| 20 | uint16_t | `noise` | 强度噪声阈值，50。 |
| 22 | uint16_t | `lost` | 有效线强度阈值，200。 |
| 24 | uint16_t | `min_contrast` | 黑白 ADC 最小绝对差，200。 |
| 26 | uint16_t | `reserved` | 原两字节保留区，构造时清零。 |
| 28 | int16_t×16 | `black` | 十六路黑底实测 ADC，原表快照。 |
| 60 | int16_t×16 | `white` | 十六路白线实测 ADC，原表快照。 |
| 92 | int8_t×16 | `weights` | 从 15 到 -15 的十六路质心整数权重。 |
| 108 | datalog_profile_t | `em` | 当前档显式电磁 bank 快照。 |
| 124 | datalog_profile_t | `optical` | 当前档显式光电 bank 快照。 |
| 140 | float | `kp_v` | 共用速度比例增益快照。 |
| 144 | float | `ki_v` | 共用速度积分增益快照。 |
| 148 | int16_t | `fan_run` | 当前档运行风扇万分比。 |
| 150 | int16_t | `fan_idle` | 当前档停车风扇万分比。 |
| 152 | uint32_t | `gray_budget_us` | 灰度预算，700 微秒。 |
| 156 | uint32_t | `control_budget_us` | 控制预算，900 微秒。 |
| 160 | uint32_t | `run_delay_count` | 起跑等待，1000 个 2 ms 控制拍。 |
| 164 | uint32_t | `capacity` | 固定最大记录条数，87336。 |
| 168 | uint32_t | `datalog_div` | 固定记录分频，1；其他配置拒绝起跑。 |
| 172 | uint8_t×84 | `reserved_tail` | 原八十四字节尾保留区，构造时清零。 |

## datalog_record_t 当前布局

小端目标布局；源头为实际声明及偏移/尺寸断言。单位和语义随字段保留。

| 偏移 | 类型/数量 | 字段 | 说明 |
|---:|---|---|---|
| 0 | uint32_t | `tick` | 1 ms 采样器 tick，uint32_t 自然回绕。 |
| 4 | int16_t×4 | `adc` | 四路滤波电感 ADC，横左/竖左/竖右/横右。 |
| 12 | int16_t | `speed_l` | 左轮原始 1 ms 编码器有符号脉冲。 |
| 14 | int16_t | `speed_r` | 右轮原始 1 ms 编码器有符号脉冲。 |
| 16 | int16_t | `track_error` | 所选模式本拍归一化偏差。 |
| 18 | int16_t | `track_out` | 唯一执行口方向输出，正常 ±1000。 |
| 20 | int16_t | `gyro_x` | x 轴原始角速度除以 14.3 后截断，度/秒。 |
| 22 | int16_t | `gyro_y` | y 轴原始角速度除以 14.3 后截断，度/秒。 |
| 24 | int16_t | `gyro_z` | z 轴原始角速度除以 14.3 后截断，度/秒。 |
| 26 | uint16_t | `dl1b_mm` | 激光距离毫米，8192 为无效。 |
| 28 | uint16_t | `battery` | 电池低通后 ADC 值，保留 uint16_t。 |
| 30 | uint8_t | `kernel_state` | 电磁影子主状态编号。 |
| 31 | uint8_t | `roundabout_state` | 电磁影子环岛子状态编号。 |
| 32 | uint16_t×16 | `gray_raw` | 十六路完整帧 ADC，ch0 最左。 |
| 64 | uint32_t | `gray_sequence` | 最近完整灰度帧序号。 |
| 68 | uint32_t | `control_sequence` | 最近完整控制拍序号。 |
| 72 | int16_t | `em_error` | 电磁观察偏差。 |
| 74 | int16_t | `optical_error` | 光电观察偏差。 |
| 76 | uint16_t | `protection_flags` | 累计 TRACKING_PROTECT_* 位图。 |
| 78 | uint8_t | `mode` | 当前模式，0 电磁、1 光电。 |
| 79 | uint8_t | `optical_state` | 光电影子 OPTICAL_* 状态。 |
| 80 | uint8_t | `gray_valid` | 标定/有效线/采集状态共同成立为 1。 |
| 81 | uint8_t | `stop_reason` | 首次 APP_STOP_* 原因字节。 |
| 82 | uint8_t×14 | `reserved` | 原十四字节保留区，快照清零，记录仍为 96 B。 |
