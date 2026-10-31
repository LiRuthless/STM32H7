/**
 * @file scb_frame.c
 * @brief SCB帧字段与字节编解码。
 * @details 仅验证/编码帧字段与CRC，失败保留调用方输出；负载借用输入缓冲。
 *     帧构造不依赖业务命令、JSON解析、会话状态或硬件。
 */
#include "services/scb_frame.h"
#include "services/checksum.h"
#include <string.h>

typedef char scb_header_assert_t[(SCB_HEADER_SIZE + SCB_PAYLOAD_MAX == SCB_SPI_SIZE) ? 1 : -1];

/**
 * @brief 从两字节读取小端整数。
 * @param[in] data 非空且至少两字节的缓冲。
 * @return 解码的无符号值。
 */
uint16_t SCB_ReadU16(const uint8_t *data)
{
    return (uint16_t)((uint16_t)data[0] | (uint16_t)data[1] << 8);
}

/**
 * @brief 从四字节读取小端整数。
 * @param[in] data 非空且至少四字节的缓冲。
 * @return 解码的无符号值。
 */
uint32_t SCB_ReadU32(const uint8_t *data)
{
    return (uint32_t)data[0] | (uint32_t)data[1] << 8
        | (uint32_t)data[2] << 16 | (uint32_t)data[3] << 24;
}

/**
 * @brief 显式写入小端两字节。
 * @param[out] data 非空且至少两字节的目标。
 * @param[in] value 待编码整数。
 */
void SCB_WriteU16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8);
}

/**
 * @brief 显式写入小端四字节。
 * @param[out] data 非空且至少四字节的目标。
 * @param[in] value 待编码整数。
 */
void SCB_WriteU32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8);
    data[2] = (uint8_t)(value >> 16);
    data[3] = (uint8_t)(value >> 24);
}

/**
 * @brief 转发通用CRC32续算。
 * @param[in] previous 上一次结果，初次传0。
 * @param[in] data 至少length字节，length为0时允许空指针。
 * @param[in] length 需要累计的字节数。
 * @return 与通用Checksum_UpdateCrc32相同，空长度返回previous。
 * @note 无共享状态；帧层保留原公共名称，公式仅在checksum.c定义。
 */
uint32_t SCB_CalculateCrc(uint32_t previous, const uint8_t *data, size_t length)
{
    return Checksum_UpdateCrc32(previous, data, length);
}

/**
 * @brief 校验消息种类与对应负载约束。
 * @details 公共消息表之外的类型全部拒绝；可靠位及握手/业务/异步身份须符合消息种类。
 * @param[in] frame 非空待检查帧描述。
 * @return 规则全部成立返回1，否则0。
 */
static uint8_t SCB_BuildFrame(const scb_frame_t *frame)
{
    uint8_t reliable;
    switch (frame->type)
    {
        case SCB_HELLO:
        case SCB_HELLO_ACK:
        case SCB_IDLE:
        case SCB_TELEMETRY:
            reliable = 0;
            break;
        case SCB_COMMAND:
        case SCB_RESULT:
        case SCB_EVENT:
        case SCB_LOG_DATA:
            reliable = 1;
            break;
        default:
            return 0;
    }
    if (frame->flags != reliable || frame->payload_len > SCB_PAYLOAD_MAX)
    {
        return 0;
    }
    /* 握手保留seq0；正常会话和业务请求非零，异步消息不得借用业务编号。 */
    if (frame->type == SCB_HELLO || frame->type == SCB_HELLO_ACK)
    {
        if (frame->seq || frame->request_id
            || (frame->type == SCB_HELLO ? frame->session_id != 0 : frame->session_id == 0))
        {
            return 0;
        }
    }
    else if (!frame->session_id || !frame->seq
        || ((frame->type == SCB_COMMAND || frame->type == SCB_RESULT
        || frame->type == SCB_LOG_DATA) ? !frame->request_id : frame->request_id != 0))
    {
        return 0;
    }
    if (frame->type == SCB_TELEMETRY)
    {
        return frame->record_count >= 1 && frame->record_count <= 10
            && frame->payload_len == 24u + frame->record_count * 96u;
    }
    if (frame->record_count || (frame->type == SCB_IDLE && frame->payload_len))
    {
        return 0;
    }
    return 1;
}

/**
 * @brief 编码完整SPI事务并清零全部填充。
 * @details 先验证描述再逐字段写入，CRC只覆盖头0～27及实际负载，拒绝无效输入。
 * @param[out] output 非空1024B目标，不能与payload重叠；失败时不写入。
 * @param[in] frame 非空描述，非零负载需有效payload。
 * @return 成功1，参数或字段无效0。
 */
uint8_t SCB_EncodeFrame(uint8_t *output, const scb_frame_t *frame)
{
    uint32_t crc;
    if (!output || !frame || (frame->payload_len && !frame->payload) || !SCB_BuildFrame(frame))
    {
        return 0;
    }
    memset(output, 0, SCB_SPI_SIZE);
    memcpy(output, "SCB1", 4);
    output[4] = 1;
    output[5] = frame->type;
    SCB_WriteU16(output + 6, frame->flags);
    SCB_WriteU16(output + 8, frame->payload_len);
    SCB_WriteU16(output + 10, frame->record_count);
    SCB_WriteU32(output + 12, frame->session_id);
    SCB_WriteU32(output + 16, frame->seq);
    SCB_WriteU32(output + 20, frame->ack_seq);
    SCB_WriteU32(output + 24, frame->request_id);
    if (frame->payload_len)
    {
        memcpy(output + SCB_HEADER_SIZE, frame->payload, frame->payload_len);
    }
    crc = SCB_CalculateCrc(0, output, 28);
    crc = SCB_CalculateCrc(crc, output + SCB_HEADER_SIZE, frame->payload_len);
    SCB_WriteU32(output + 28, crc);
    return 1;
}

/**
 * @brief 验证帧字段和CRC后发布描述。
 * @details 接受精确网络帧或1024B事务；先在局部构造描述，全部通过才写输出。
 * @param[in] input 非空输入，不要求对齐。
 * @param[in] length 实际缓冲长度，32～1024B。
 * @param[out] frame 非空输出，失败保留原值，payload借用input。
 * @return 合法1，否则0；不产生ACK或业务执行。
 */
uint8_t SCB_DecodeFrame(const uint8_t *input, size_t length, scb_frame_t *frame)
{
    scb_frame_t decoded;
    uint32_t crc;
    if (!input || !frame || length < SCB_HEADER_SIZE || length > SCB_SPI_SIZE
        || memcmp(input, "SCB1", 4) || input[4] != 1)
    {
        return 0;
    }
    decoded.type = input[5];
    decoded.flags = SCB_ReadU16(input + 6);
    decoded.payload_len = SCB_ReadU16(input + 8);
    decoded.record_count = SCB_ReadU16(input + 10);
    decoded.session_id = SCB_ReadU32(input + 12);
    decoded.seq = SCB_ReadU32(input + 16);
    decoded.ack_seq = SCB_ReadU32(input + 20);
    decoded.request_id = SCB_ReadU32(input + 24);
    decoded.payload = input + SCB_HEADER_SIZE;
    if (!SCB_BuildFrame(&decoded) || (length != SCB_SPI_SIZE
        && length != SCB_HEADER_SIZE + decoded.payload_len))
    {
        return 0;
    }
    crc = SCB_CalculateCrc(0, input, 28);
    crc = SCB_CalculateCrc(crc, decoded.payload, decoded.payload_len);
    if (crc != SCB_ReadU32(input + 28))
    {
        return 0;
    }
    *frame = decoded;
    return 1;
}
