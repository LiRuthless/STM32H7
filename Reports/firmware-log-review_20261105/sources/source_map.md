# 来源与审查边界

当前HEAD a0070b8848e262391e0b874c89231a2538946ff1。资料核对2026-11-05。
仅制作演示文稿，不更改固件，不重新宣称验收。

- P01 固件日志记录与上传：SmartCar/Docs/services/recording-and-export.md；SmartCar/Docs/services/communication.md
- P02 一份记录，走向两条独立通道：SmartCar/App/services/run_record.c::RunRecord_CaptureSample；SmartCar/App/services/logger.c::Datalog_Push
- P03 职责边界决定状态归谁所有：SmartCar/App/services/run_record.c；logger.c；log_store.c；log_export.c；telemetry.c；scb_link.c
- P04 1 ms 采样，不等于 1 ms 控制：SmartCar/App/services/run_record.c::Datalog_PublishControl；RunRecord_CaptureSample；SmartCar/App/system/scheduler.c
- P05 96 B 固定记录：把时间与状态放在一起：SmartCar/App/services/datalog.h::datalog_record_t；SmartCar/Docs/services/recording-and-export.md#f19
- P06 256 B 会话头：解释这一次运行：SmartCar/App/services/run_record.c::RunRecord_BuildHeader；datalog.h::datalog_header_t
- P07 先建立有效日志，再允许起跑：SmartCar/App/services/logger.c::Datalog_Start；SmartCar/App/services/log_store.c::LogStore_WriteHeader；SmartCar/App/system/lifecycle.c::Lifecycle_Start
- P08 Flash 队列：1024 槽，最多积压 1023 条：SmartCar/App/services/logger.c::Datalog_Push；SmartCar/App/services/datalog.h
- P09 发布与释放，各有一个不可提前的时点：SmartCar/App/services/logger.c::Datalog_Push；Datalog_Service
- P10 Flash 空间：日志与参数有明确边界：SmartCar/BSP/bsp_flash.h；SmartCar/App/services/datalog.h；SmartCar/App/services/logger.c::Datalog_Push
- P11 前台最多写 8 条，底层按页拆写：SmartCar/App/services/logger.c::Datalog_Service；SmartCar/BSP/bsp_flash.c::BSP_Flash_LogWrite
- P12 日志故障停车，网络拥塞不新增停车：SmartCar/App/services/logger.c::Datalog_RequestFaultStop；Datalog_Stop；SmartCar/App/system/safety.c
- P13 上传队列独立，丢弃不挪动 Flash 的 tail：SmartCar/App/services/telemetry.c::Telemetry_PushRecord；Telemetry_BeginRun；Telemetry_Service
- P14 封包有三条边界：运行、版本、连续索引：SmartCar/App/services/telemetry.c::Telemetry_Service；Telemetry_SetRevision
- P15 24 B 元数据，让缺口与归属可解释：SmartCar/App/services/telemetry.c::Telemetry_Service；SmartCar/Docs/services/communication.md#scb1
- P16 STM32 到电脑：实现边界必须可见：SmartCar/Docs/services/communication.md#scb1；SmartCar/Docs/services/communication.md#f25
- P17 SCB/1：同一帧，传输外壳不同：SmartCar/App/services/scb_frame.c；scb_frame.h；SmartCar/Docs/services/communication.md#scb1
- P18 可靠结果与实时数据，采用不同取舍：SmartCar/App/services/scb_link.c::SCB_PrepareFrame；SCB_ReceiveFrame；SCB_AcceptRequest
- P19 DMA 事务：中断只交接，前台做解析：SmartCar/BSP/bsp_wifi.c::BSP_WiFi_TryExchange；BSP_WiFi_Service；BSP_WiFi_ReleaseRx
- P20 停车导出：先冻结成功前缀，再顺序读取：SmartCar/App/services/log_export.c::Datalog_BeginExport；Datalog_ReadExport；Datalog_EndExport
- P21 重读最近块不推进，也不续租：SmartCar/App/services/log_export.c::Datalog_ReadExport；Datalog_ServiceExport；SmartCar/Docs/services/communication.md STM32适配说明
- P22 三层校验，各自回答不同问题：SmartCar/App/services/run_record.c；scb_frame.c；log_export.c；checksum.c
- P23 离线分析：CSV 与运行参数历史互补：SmartCar/App/services/log_export.c::Datalog_Dump；SmartCar/Docs/services/recording-and-export.md#f21
- P24 评审落点：机制已落地，实测证据仍需补齐：SmartCar/Docs/known-issues.md；SmartCar/Docs/services/communication.md#f24；SmartCar/specs/roadmap.md