/**
 * @file reliable_link.h
 * @brief 独立可靠窗口与重试状态。
 * @details 声明独立可靠窗口与重试状态接口；实现及完整参数/错误语义位于reliable_link.c。
 *     调用方按接口约束串行访问owner状态，头文件不定义线图像或运行期服务。
 *
 * @note Phase02 G4：validated入口的视图与原始字节/队列负载必须匹配且不变。
 */
#ifndef BRIDGE_RELIABLE_LINK_H
#define BRIDGE_RELIABLE_LINK_H
#include "protocol.h"

#define SCB_RETRY_MS 200u
#define SCB_MAX_RETRIES 3u

typedef enum
{
    SCB_LINK_OK, /**< 无动作或新图像准备成功。 */
    SCB_LINK_DELIVER, /**< 新帧可交付，可靠帧已经预留。 */
    SCB_LINK_DUPLICATE, /**< 完整可靠身份相同，不重复交付。 */
    SCB_LINK_BACKPRESSURE, /**< 可靠帧无真实预留，不推进接管ACK。 */
    SCB_LINK_STALE, /**< 旧身份/旧序号，不交付。 */
    SCB_LINK_INVALID, /**< 参数、格式或身份非法。 */
    SCB_LINK_BUSY, /**< 可靠发送窗口仍被占用。 */
    SCB_LINK_RESEND, /**< 借出原图像，等待完整发送报告。 */
    SCB_LINK_REBUILD /**< 会话失效、近回绕或重试耗尽。 */
} scb_link_result_t;

/*
 * 各传输任务独占实例，无DMA/socket指针；tx在精确ACK或reset前不可改写，
 * last_rx保存完整可靠身份，不被非可靠高序号遮蔽。
 */
typedef struct
{
    scb_transport_t transport; /**< 实例固定传输类型SPI或TCP。 */
    uint32_t session_id; /**< 本可靠窗口的会话标识，零时dead。 */
    uint32_t next_seq; /**< 下一个普通发送序号，近回绕要求重建。 */
    uint32_t rx_high; /**< 已交付帧的最高普通序号。 */
    uint32_t rx_reliable_seq; /**< 最近可靠入帧的独立身份序号。 */
    uint32_t ack_seq; /**< 已可靠接管的序号，供新出帧携带。 */
    uint32_t tx_seq; /**< 当前在途可靠图像序号。 */
    uint64_t last_send_ms; /**< 上次完整发送完成时的单调毫秒数。 */
    size_t tx_length; /**< 不可变在途TX图像有效字节数。 */
    size_t rx_length; /**< 最近可靠身份缓存字节数，不含SPI填充。 */
    unsigned retries; /**< 已经完整发送的重发次数，首次发送不计。 */
    bool pending; /**< 仍等待精确ACK的可靠图像存在。 */
    bool sending; /**< 当前图像待发送/发送中，poll不重复发起。 */
    bool sent; /**< 当前可靠图像至少已完整发送一次。 */
    bool dead; /**< 会话无效或需重建，停止普通接管。 */
    uint8_t tx[SCB_SPI_BYTES]; /**< 原可靠发送字节，精确ACK/reset前保持内容。 */
    uint8_t last_rx[SCB_SPI_BYTES]; /**< 最近可靠入帧完整身份缓存，与非可靠高序号分离。 */
} scb_link_t;

/**
 * @brief 重置一个传输独占的可靠窗口。
 * @note reliable_link.c::scb_link_reset；所属传输任务串行调用；接管视图先提交再发ACK。
 */
void scb_link_reset(scb_link_t *link, scb_transport_t transport, uint32_t session);
/**
 * @brief 校验入帧并推进可靠接管与精确ACK。
 * @note reliable_link.c::scb_link_receive；所属传输任务串行调用；接管视图先提交再发ACK。
 */
scb_link_result_t scb_link_receive(scb_link_t *link, const uint8_t *bytes, size_t length,
    bool reserved, bridge_json_workspace_t *workspace, scb_frame_view_t *view);
/**
 * @brief 编码新出帧并在需要时保留不可变重发图像。
 * @note reliable_link.c::scb_link_prepare；所属传输任务串行调用；接管视图先提交再发ACK。
 */
scb_link_result_t scb_link_prepare(scb_link_t *link, const scb_header_t *header,
    const uint8_t *payload, uint8_t *output, size_t capacity, bridge_json_workspace_t *workspace,
    size_t *used);
/**
 * @brief 确认可靠图像已经全部发送完成。
 * @note reliable_link.c::scb_link_sent；所属传输任务串行调用；接管视图先提交再发ACK。
 */
bool scb_link_sent(scb_link_t *link, uint32_t seq, uint64_t now_ms);
/**
 * @brief 轮询重试截止并借出原可靠图像。
 * @note reliable_link.c::scb_link_poll；所属传输任务串行调用；接管视图先提交再发ACK。
 */
scb_link_result_t scb_link_poll(
    scb_link_t *link, uint64_t now_ms, const uint8_t **bytes, size_t *length);
/**
 * @brief 接管同一原始帧的已验证视图；不重复解码。
 * @note reliable_link.c::scb_link_receive_validated；原始字节与视图必须匹配且不变。
 */
scb_link_result_t scb_link_receive_validated(scb_link_t *link, const uint8_t *bytes,
    const scb_frame_view_t *validated, bool reserved, scb_frame_view_t *view);

/**
 * @brief 为已验证且不变的内部负载准备可靠发送，不重复负载校验。
 * @note reliable_link.c::scb_link_prepare_validated；本地新负载仍走完整prepare。
 */
scb_link_result_t scb_link_prepare_validated(scb_link_t *link, const scb_frame_view_t *frame,
    uint8_t *output, size_t capacity, size_t *used);

#endif
