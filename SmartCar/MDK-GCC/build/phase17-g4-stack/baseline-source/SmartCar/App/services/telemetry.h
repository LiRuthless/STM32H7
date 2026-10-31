/**
 * @file telemetry.h
 * @brief 独立1kHz上传队列及运行元数据。
 * @details 接口不共享Flash消费位置；并发发布与封包见 telemetry.c。
 */
#ifndef SMARTCAR_APP_TELEMETRY_H
#define SMARTCAR_APP_TELEMETRY_H
#include "services/datalog.h"

#define TELEMETRY_CAPACITY 256u

typedef struct
{
    uint32_t run_id; /**< 当前实际运行编号，启动前0。 */
    uint32_t revision; /**< 本拍有效参数版本。 */
    uint32_t next_index; /**< 下一应产生记录的索引，包含丢弃缺口。 */
    uint32_t dropped; /**< 上传生产/消费丢弃总数。 */
    uint32_t queued; /**< 已发布且尚未消费条数。 */
} telemetry_status_t;

/**
 * @brief 上电初始化队列。
 * @note 完整契约见 telemetry.c::Telemetry_Init。
 */
void Telemetry_Init(void);
/**
 * @brief 实际起跑前分配非零run_id；定时器未启动，见 telemetry.c。
 * @note 完整契约见 telemetry.c::Telemetry_BeginRun。
 */
void Telemetry_BeginRun(void);
/**
 * @brief TIM15固定复制一条完整记录。
 * @note 完整契约见 telemetry.c::Telemetry_PushRecord。
 */
void Telemetry_PushRecord(const datalog_record_t *record);
/**
 * @brief 前台每10ms取最多10条连续同版本记录。
 * @note 完整契约见 telemetry.c::Telemetry_Service。
 */
uint8_t Telemetry_Service(uint8_t *payload, uint16_t *length, uint16_t *count, uint32_t now);
/**
 * @brief 复制当前上传状态。
 * @note 完整契约见 telemetry.c::Telemetry_GetStatus。
 */
void Telemetry_GetStatus(telemetry_status_t *status);
/**
 * @brief 同步有效版本，不改已入队元数据。
 * @note 完整契约见 telemetry.c::Telemetry_SetRevision。
 */
void Telemetry_SetRevision(uint32_t revision);
/**
 * @brief 只推进消费者丢弃旧会话积压。
 * @note 完整契约见 telemetry.c::Telemetry_ResetSession。
 */
void Telemetry_ResetSession(void);

#endif
