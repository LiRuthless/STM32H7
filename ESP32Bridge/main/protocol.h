/**
 * @file protocol.h
 * @brief SCB显式帧编解码。
 * @details 声明SCB显式帧编解码接口；实现及完整参数/错误语义位于protocol.c。
 *     调用方按接口约束串行访问owner状态，头文件不定义线图像或运行期服务。
 *
 * @note Phase02 G4：内部validated入口仅供已验证且类型/负载结构未变的帧使用。
 */
#ifndef BRIDGE_PROTOCOL_H
#define BRIDGE_PROTOCOL_H

#include "json_guard.h"

#define SCB_HEADER_BYTES 32u
#define SCB_PAYLOAD_BYTES 992u
#define SCB_SPI_BYTES 1024u
#define SCB_RECORD_BYTES 96u
#define SCB_TELEMETRY_PREFIX 24u
#define SCB_RELIABLE 1u

enum
{
    SCB_HELLO = 0x01, /**< 握手请求类型。 */
    SCB_HELLO_ACK = 0x02, /**< 握手/状态/BUSY应答类型。 */
    SCB_IDLE = 0x03, /**< 空闲及可靠ACK载体。 */
    SCB_COMMAND = 0x10, /**< 可靠业务命令类型。 */
    SCB_RESULT = 0x11, /**< 可靠业务结果类型。 */
    SCB_EVENT = 0x12, /**< 可靠事件类型。 */
    SCB_TELEMETRY = 0x20, /**< 非可靠遥测类型。 */
    SCB_LOG_DATA = 0x30 /**< 可靠日志块类型。 */
};

typedef enum
{
    SCB_SPI, /**< 固定1024字节SPI传输。 */
    SCB_TCP, /**< 无填充完整TCP帧。 */
    SCB_UDP /**< 单遥测报文传输。 */
} scb_transport_t;

typedef enum
{
    SCB_OK = 0, /**< 编解码成功。 */
    SCB_ARGUMENT, /**< 接口参数无效。 */
    SCB_LENGTH, /**< 输入/输出或类型负载长度不符。 */
    SCB_MAGIC, /**< 帧magic不匹配。 */
    SCB_VERSION, /**< 协议版本不匹配。 */
    SCB_TYPE, /**< 消息类型不支持。 */
    SCB_FLAGS, /**< flags与消息类型不符。 */
    SCB_RECORD_COUNT, /**< 记录数不符合类型约束。 */
    SCB_CRC, /**< CRC校验失败。 */
    SCB_PAYLOAD, /**< 类型负载字段不符。 */
    SCB_JSON, /**< JSON完整校验失败。 */
    SCB_TRANSPORT, /**< 传输类别或允许类型不符。 */
    SCB_ALIAS /**< 输入/输出地址范围溢出或重叠。 */
} scb_error_t;

/* 仅表示字段值，普通C布局不是线格式。 */
typedef struct
{
    uint8_t type; /**< SCB消息类型字节值。 */
    uint16_t flags; /**< 线格式标志值，按类型验证。 */
    uint16_t payload_len; /**< 实际负载字节数，不含头和SPI填充。 */
    uint16_t record_count; /**< 遥测记录条数，其他类型必须零。 */
    uint32_t session_id; /**< 线格式会话标识，零用于握手/无会话状态。 */
    uint32_t seq; /**< 发送方向序号，零用于握手。 */
    uint32_t ack_seq; /**< 对向可靠接管确认序号，不代表业务生效。 */
    uint32_t request_id; /**< 业务请求标识，由上层解释幂等。 */
} scb_header_t;

typedef struct
{
    scb_header_t header; /**< 解码后的字段值副本，不是线布局。 */
    const uint8_t *payload; /**< 借用输入的负载起点，输入有效期间可读。 */
} scb_frame_view_t;

/**
 * @brief 验证完整帧并发布借用负载视图。
 * @note protocol.c::scb_decode；输入在视图借用期间不变，JSON工作区由调用任务独占。
 */
scb_error_t scb_decode(const uint8_t *bytes, size_t length, scb_transport_t transport,
    bridge_json_workspace_t *json_workspace, scb_frame_view_t *view);
/**
 * @brief 验证字段并显式编码到独立输出。
 * @note protocol.c::scb_encode；输入在视图借用期间不变，JSON工作区由调用任务独占。
 */
scb_error_t scb_encode(const scb_header_t *header, const uint8_t *payload,
    scb_transport_t transport, uint8_t *output, size_t capacity,
    bridge_json_workspace_t *json_workspace, size_t *used);

/**
 * @brief 重封装已验证且类型/负载结构未变的内部帧。
 * @note protocol.c::scb_encode_validated；仅身份可变，容量与别名保护保留。
 */
scb_error_t scb_encode_validated(const scb_frame_view_t *frame, scb_transport_t transport,
    uint8_t *output, size_t capacity, size_t *used);

#endif
