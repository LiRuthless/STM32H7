/**
 * @file scb_command_codec.h
 * @brief SCB业务命令类型和格式接口。
 * @details 实现见scb_command_codec.c；只提供值转换/格式，不拥有业务事务状态。
 */
#ifndef SMARTCAR_APP_SERVICES_SCB_COMMAND_CODEC_H
#define SMARTCAR_APP_SERVICES_SCB_COMMAND_CODEC_H
#include "services/scb_frame.h"
#include "services/scb_json.h"
#include "control/tuning_values.h"

typedef enum
{
    SCB_OK, /**< 查询成功。 */
    SCB_RECEIVED, /**< 请求已预留，尚未生效。 */
    SCB_APPLIED, /**< 整组已生效。 */
    SCB_SAVED, /**< 已完成持久化。 */
    SCB_INVALID_ARG, /**< 类型、形状或目标不合法。 */
    SCB_OUT_OF_RANGE, /**< 数值越界。 */
    SCB_BUSY, /**< 状态或容量暂不允许。 */
    SCB_SPECIAL_ELEMENT, /**< 当前为特殊元素。 */
    SCB_NOT_FOUND, /**< 没有对应对象。 */
    SCB_IO_ERROR /**< 外部存储失败。 */
} scb_status_t;

typedef struct
{
    float kp; /**< 方向比例增益。 */
    float k2p; /**< 方向非线性增益。 */
    float kd; /**< 方向差分增益。 */
    float kp_v; /**< 速度比例增益。 */
    float ki_v; /**< 速度积分增益。 */
    int32_t base_speed; /**< 脉冲/2ms，保留校验前完整整数。 */
    int32_t fan_run; /**< 运行风扇万分比。 */
    int32_t fan_idle; /**< 空闲风扇万分比。 */
} scb_values_t;

typedef struct
{
    char op[24]; /**< 已解码命令名，不截断。 */
    uint8_t mode; /**< 目标模式。 */
    uint8_t gear; /**< 目标档位。 */
    scb_values_t values; /**< 完整待校验参数。 */
    uint32_t target_request_id; /**< 被查询请求编号。 */
    uint32_t transfer_id; /**< 导出会话编号。 */
    uint32_t offset; /**< 导出绝对字节偏移。 */
    uint32_t length; /**< 导出请求字节数。 */
} scb_command_t;

/**
 * @brief 返回公共业务状态名称。
 * @note 完整契约见 scb_command_codec.c::SCB_StatusName。
 */
const char *SCB_StatusName(scb_status_t status);

/**
 * @brief 输出完整八项参数对象。
 * @note 完整契约见 scb_command_codec.c::SCB_JsonValues。
 */
void SCB_JsonValues(scb_writer_t *writer, const scb_values_t *values);

/**
 * @brief 将完整JSON解析成命令快照。
 * @note 完整契约见 scb_command_codec.c::SCB_ParseCommand。
 */
uint8_t SCB_ParseCommand(const uint8_t *input, uint16_t length,
    scb_command_t *command, scb_status_t *status);

/**
 * @brief 将协议参数逐字段转换后调用唯一业务校验器。
 * @note 完整契约见 scb_command_codec.c::SCB_ValidateValues。
 */
scb_status_t SCB_ValidateValues(const scb_values_t *values);

/**
 * @brief 编码LOG_DATA固定八字节头及原始日志块。
 * @note 完整契约见 scb_command_codec.c::SCB_EncodeLogData。
 */
uint16_t SCB_EncodeLogData(uint8_t *payload, uint32_t offset,
    const uint8_t *data, uint16_t length);

#endif /* SMARTCAR_APP_SERVICES_SCB_COMMAND_CODEC_H */
