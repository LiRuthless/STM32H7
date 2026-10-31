/**
 * @file scb_frame.h
 * @brief SCB帧格式、字段与字节接口。
 * @details scb_frame.c实现纯帧编解码，借用负载受调用方缓冲寿命约束。
 */
#ifndef SMARTCAR_APP_SERVICES_SCB_FRAME_H
#define SMARTCAR_APP_SERVICES_SCB_FRAME_H
#include <stdint.h>
#include <stddef.h>

#define SCB_HEADER_SIZE 32u
#define SCB_PAYLOAD_MAX 992u
#define SCB_SPI_SIZE 1024u
#define SCB_RELIABLE 1u
#define SCB_HELLO 0x01u
#define SCB_HELLO_ACK 0x02u
#define SCB_IDLE 0x03u
#define SCB_COMMAND 0x10u
#define SCB_RESULT 0x11u
#define SCB_EVENT 0x12u
#define SCB_TELEMETRY 0x20u
#define SCB_LOG_DATA 0x30u

typedef struct
{
    uint8_t type; /**< SCB 消息种类。 */
    uint16_t flags; /**< 仅 bit0 可靠标志。 */
    uint16_t payload_len; /**< 实际负载字节数。 */
    uint16_t record_count; /**< 上传条数，其他消息为零。 */
    uint32_t session_id; /**< 链路会话。 */
    uint32_t seq; /**< 本方向帧编号。 */
    uint32_t ack_seq; /**< 最近接管的可靠帧编号。 */
    uint32_t request_id; /**< 业务身份，异步为零。 */
    const uint8_t *payload; /**< 借用调用方负载，只在缓冲有效期内读取。 */
} scb_frame_t;

/**
 * @brief 从两字节读取小端整数。
 * @note 完整契约见 scb_frame.c::SCB_ReadU16。
 */
uint16_t SCB_ReadU16(const uint8_t *data);

/**
 * @brief 从四字节读取小端整数。
 * @note 完整契约见 scb_frame.c::SCB_ReadU32。
 */
uint32_t SCB_ReadU32(const uint8_t *data);

/**
 * @brief 显式写入小端两字节。
 * @note 完整契约见 scb_frame.c::SCB_WriteU16。
 */
void SCB_WriteU16(uint8_t *data, uint16_t value);

/**
 * @brief 显式写入小端四字节。
 * @note 完整契约见 scb_frame.c::SCB_WriteU32。
 */
void SCB_WriteU32(uint8_t *data, uint32_t value);

/**
 * @brief 转发通用CRC32续算。
 * @note 完整契约见 scb_frame.c::SCB_CalculateCrc。
 */
uint32_t SCB_CalculateCrc(uint32_t previous, const uint8_t *data, size_t length);

/**
 * @brief 编码完整SPI事务并清零全部填充。
 * @note 完整契约见 scb_frame.c::SCB_EncodeFrame。
 */
uint8_t SCB_EncodeFrame(uint8_t *output, const scb_frame_t *frame);

/**
 * @brief 验证帧字段和CRC后发布描述。
 * @note 完整契约见 scb_frame.c::SCB_DecodeFrame。
 */
uint8_t SCB_DecodeFrame(const uint8_t *input, size_t length, scb_frame_t *frame);

#endif /* SMARTCAR_APP_SERVICES_SCB_FRAME_H */
