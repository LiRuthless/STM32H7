/**
 * @file run_record.h
 * @brief 日志记录组装内部接口。
 * @details 声明run_record.c提供的布局校验、快照和头部接口；不暴露可写状态。
 */
#ifndef SMARTCAR_APP_SERVICES_RUN_RECORD_H
#define SMARTCAR_APP_SERVICES_RUN_RECORD_H
#include "services/datalog.h"

/**
 * @brief 清空完整控制快照。
 * @note 完整契约见 run_record.c::RunRecord_ResetSnapshot。
 */
void RunRecord_ResetSnapshot(void);

/**
 * @brief 检查当前灰度标定能否建立日志会话。
 * @note 完整契约见 run_record.c::RunRecord_CheckCalibration。
 */
uint8_t RunRecord_CheckCalibration(void);

/**
 * @brief 组装本次会话的固定256字节头。
 * @note 完整契约见 run_record.c::RunRecord_BuildHeader。
 */
const datalog_header_t *RunRecord_BuildHeader(void);

/**
 * @brief 借用当前内存会话头。
 * @note 完整契约见 run_record.c::RunRecord_GetHeader。
 */
const datalog_header_t *RunRecord_GetHeader(void);

/**
 * @brief 按原读取位置发布完整控制快照。
 * @note 完整契约见 run_record.c::Datalog_PublishControl。
 */
void Datalog_PublishControl(void);

/**
 * @brief 把完整控制快照与当前1ms采样合为记录。
 * @note 完整契约见 run_record.c::RunRecord_CaptureSample。
 */
void RunRecord_CaptureSample(datalog_record_t *record);

/**
 * @brief 逐项校验原 v2 头布局、配置、CRC 和标定。
 * @note 完整契约见 run_record.c::RunRecord_IsHeaderValid。
 */
uint8_t RunRecord_IsHeaderValid(const datalog_header_t *h);

#endif /* SMARTCAR_APP_SERVICES_RUN_RECORD_H */
