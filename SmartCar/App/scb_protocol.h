/**
 * @file scb_protocol.h
 * @brief SCB/1 字节帧和有界 JSON 的公共类型。
 * @details 仅声明协议数据与纯编解码接口，算法及资源边界见 scb_protocol.c。
 */
#ifndef SMARTCAR_APP_SCB_PROTOCOL_H
#define SMARTCAR_APP_SCB_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

#define SCB_HEADER_SIZE 32u
#define SCB_PAYLOAD_MAX 992u
#define SCB_SPI_SIZE 1024u
#define SCB_JSON_TOKENS 128u
#define SCB_JSON_DEPTH 8u
#define SCB_RELIABLE 1u
#define SCB_HELLO 0x01u
#define SCB_HELLO_ACK 0x02u
#define SCB_IDLE 0x03u
#define SCB_COMMAND 0x10u
#define SCB_RESULT 0x11u
#define SCB_EVENT 0x12u
#define SCB_TELEMETRY 0x20u
#define SCB_LOG_DATA 0x30u

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

typedef enum
{
    SCB_JSON_OBJECT, /**< JSON 对象。 */
    SCB_JSON_ARRAY, /**< JSON 数组。 */
    SCB_JSON_STRING, /**< 解码后的 UTF-8 字符串。 */
    SCB_JSON_NUMBER, /**< 有限 double 数值。 */
    SCB_JSON_TRUE, /**< 真值。 */
    SCB_JSON_FALSE, /**< 假值。 */
    SCB_JSON_NULL /**< 空值。 */
} scb_json_type_t;

typedef struct
{
    scb_json_type_t type; /**< 令牌类型。 */
    int16_t parent; /**< 所属容器，根为-1。 */
    uint16_t text; /**< 字符串池起点。 */
    uint16_t length; /**< 解码字符串字节数。 */
    uint8_t key; /**< 本令牌是否为对象键。 */
    uint8_t integer; /**< 原文是否不含小数和指数。 */
    double number; /**< 数字转换结果。 */
} scb_token_t;

typedef struct
{
    scb_token_t tokens[SCB_JSON_TOKENS]; /**< 固定令牌区，前台独占。 */
    char strings[SCB_PAYLOAD_MAX + 1u]; /**< 解码文本池。 */
    uint16_t count; /**< 已分配令牌数。 */
    uint16_t used; /**< 字符串池占用。 */
    uint16_t position; /**< 解析字节位置。 */
    uint16_t length; /**< 输入总长。 */
    const uint8_t *input; /**< 仅解析期间借用的输入。 */
} scb_json_t;

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

typedef struct
{
    uint8_t *data; /**< 调用方静态目标。 */
    uint16_t capacity; /**< 可写字节数，不含隐式终止符。 */
    uint16_t length; /**< 当前完整输出长度。 */
    uint8_t failed; /**< 任何溢出后整条结果不可发送。 */
} scb_writer_t;

/**
 * @brief 读取无对齐要求的小端整数。
 * @note 完整契约见 scb_protocol.c::SCB_ReadU16。
 */
uint16_t SCB_ReadU16(const uint8_t *data);
/**
 * @brief 读取无对齐要求的小端整数。
 * @note 完整契约见 scb_protocol.c::SCB_ReadU32。
 */
uint32_t SCB_ReadU32(const uint8_t *data);
/**
 * @brief 写入小端整数。
 * @note 完整契约见 scb_protocol.c::SCB_WriteU16。
 */
void SCB_WriteU16(uint8_t *data, uint16_t value);
/**
 * @brief 写入小端整数。
 * @note 完整契约见 scb_protocol.c::SCB_WriteU32。
 */
void SCB_WriteU32(uint8_t *data, uint32_t value);
/**
 * @brief 续算 zlib 兼容 CRC32。
 * @note 完整契约见 scb_protocol.c::SCB_CalculateCrc。
 */
uint32_t SCB_CalculateCrc(uint32_t previous, const uint8_t *data, size_t length);
/**
 * @brief 编成1024B零填充帧。
 * @note 完整契约见 scb_protocol.c::SCB_EncodeFrame。
 */
uint8_t SCB_EncodeFrame(uint8_t *output, const scb_frame_t *frame);
/**
 * @brief 校验实际帧长、字段和CRC后借用负载。
 * @note 完整契约见 scb_protocol.c::SCB_DecodeFrame。
 */
uint8_t SCB_DecodeFrame(const uint8_t *input, size_t length, scb_frame_t *frame);
/**
 * @brief 解析完整UTF-8 JSON，拒绝重复键和资源越界。
 * @note 完整契约见 scb_protocol.c::SCB_ParseJson。
 */
uint8_t SCB_ParseJson(const uint8_t *input, uint16_t length, scb_json_t *json);
/**
 * @brief 在对象中查找解码后键名，缺失返回-1。
 * @note 完整契约见 scb_protocol.c::SCB_FindMember。
 */
int16_t SCB_FindMember(const scb_json_t *json, int16_t object, const char *key);
/**
 * @brief 返回字符串令牌或空指针。
 * @note 完整契约见 scb_protocol.c::SCB_GetString。
 */
const char *SCB_GetString(const scb_json_t *json, int16_t token);
/**
 * @brief 检查数字令牌后复制。
 * @note 完整契约见 scb_protocol.c::SCB_GetNumber。
 */
uint8_t SCB_GetNumber(const scb_json_t *json, int16_t token, double *value);
/**
 * @brief 解析业务命令，语法失败返回0，业务错误写status。
 * @note 完整契约见 scb_protocol.c::SCB_ParseCommand。
 */
uint8_t SCB_ParseCommand(const uint8_t *input, uint16_t length,
    scb_command_t *command, scb_status_t *status);
/**
 * @brief 返回公共状态拼写。
 * @note 完整契约见 scb_protocol.c::SCB_StatusName。
 */
const char *SCB_StatusName(scb_status_t status);


/**
 * @brief 初始化有界JSON写入器。
 * @note 完整契约见 scb_protocol.c::SCB_JsonBegin。
 */
void SCB_JsonBegin(scb_writer_t *writer, uint8_t *data, uint16_t capacity);
/**
 * @brief 追加可信JSON片段，溢出锁存失败。
 * @note 完整契约见 scb_protocol.c::SCB_JsonAppend。
 */
void SCB_JsonAppend(scb_writer_t *writer, const char *text);
/**
 * @brief 写十进制无符号整数。
 * @note 完整契约见 scb_protocol.c::SCB_JsonU32。
 */
void SCB_JsonU32(scb_writer_t *writer, uint32_t value);
/**
 * @brief 写十进制有符号整数。
 * @note 完整契约见 scb_protocol.c::SCB_JsonI32。
 */
void SCB_JsonI32(scb_writer_t *writer, int32_t value);
/**
 * @brief 写可往返的binary32数字，拒绝非有限值。
 * @note 完整契约见 scb_protocol.c::SCB_JsonFloat。
 */
void SCB_JsonFloat(scb_writer_t *writer, float value);
/**
 * @brief 写完整八项values对象。
 * @note 完整契约见 scb_protocol.c::SCB_JsonValues。
 */
void SCB_JsonValues(scb_writer_t *writer, const scb_values_t *values);

#endif
