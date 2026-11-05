/**
 * @file link_session.h
 * @brief SPI/TCP逻辑握手与会话许可。
 * @details 声明SPI/TCP逻辑握手与会话许可接口；实现及完整参数/错误语义位于link_session.c。
 *     调用方按接口约束串行访问owner状态，头文件不定义线图像或运行期服务。
 */
#ifndef BRIDGE_LINK_SESSION_H
#define BRIDGE_LINK_SESSION_H
#include "protocol.h"

typedef enum
{
    SCB_WAIT_HELLO, /**< 等待零会话HELLO。 */
    SCB_WAIT_IDLE, /**< 已生成会话，等待新会话IDLE确认。 */
    SCB_SESSION_READY /**< SPI握手已建立。 */
} scb_spi_phase_t;

typedef enum
{
    SCB_SESSION_REJECT, /**< 拒绝输入，不交付。 */
    SCB_SESSION_REPLY, /**< 输出已编码的握手应答。 */
    SCB_SESSION_ESTABLISHED, /**< 本次IDLE完成SPI握手。 */
    SCB_SESSION_PASS, /**< 已建立会话的普通帧可交可靠层。 */
    SCB_SESSION_NEED_RANDOM /**< 未取得合格候选，需重试随机值。 */
} scb_session_result_t;

/* 同一会话owner串行访问；close_tcp请求先撤业务许可，实际socket由网络owner关闭。 */
typedef struct
{
    scb_spi_phase_t phase; /**< SPI握手阶段，owner串行写。 */
    uint32_t session_id; /**< 当前SPI会话，零为未建立。 */
    uint32_t previous_session; /**< 上次非零会话，拒绝随机候选复用。 */
    uint32_t tcp_session; /**< 逻辑TCP握手绑定的SPI会话。 */
    bool tcp_open; /**< 逻辑TCP连接已占用；不代表握手完成。 */
    bool tcp_hello; /**< 逻辑TCP已完成HELLO。 */
    bool close_tcp; /**< 粘性关闭请求，先撤业务许可，由socket owner执行。 */
} scb_session_t;

/**
 * @brief 清空会话状态。
 * @note link_session.c::scb_session_init；状态接口由owner串行调用；显式快照接口不访问SPI对象。
 */
void scb_session_init(scb_session_t *session);
/**
 * @brief 推进SPI握手并生成后续事务应答。
 * @note link_session.c::scb_session_spi；状态接口由owner串行调用；显式快照接口不访问SPI对象。
 */
scb_session_result_t scb_session_spi(scb_session_t *session, const uint8_t *bytes, size_t length,
    uint32_t candidate, bridge_json_workspace_t *workspace, uint8_t *reply, size_t capacity,
    size_t *used);
/**
 * @brief 逻辑失效SPI会话并撤销TCP许可。
 * @note link_session.c::scb_session_spi_lost；状态接口由owner串行调用；显式快照接口不访问SPI对象。
 */
void scb_session_spi_lost(scb_session_t *session);
/**
 * @brief 重编码等待IDLE期间的握手应答。
 * @note link_session.c::scb_session_spi_pending_reply；状态接口由owner串行调用；显式快照接口不访问SPI对象。
 */
bool scb_session_spi_pending_reply(const scb_session_t *session, bridge_json_workspace_t *workspace,
    uint8_t *reply, size_t capacity, size_t *used);
/**
 * @brief 登记一个新TCP逻辑连接。
 * @note link_session.c::scb_session_tcp_open；状态接口由owner串行调用；显式快照接口不访问SPI对象。
 */
bool scb_session_tcp_open(scb_session_t *session);
/**
 * @brief 清理TCP逻辑连接状态。
 * @note link_session.c::scb_session_tcp_close；状态接口由owner串行调用；显式快照接口不访问SPI对象。
 */
void scb_session_tcp_close(scb_session_t *session);
/**
 * @brief 为逻辑TCP连接处理一次HELLO。
 * @note link_session.c::scb_session_tcp_hello；状态接口由owner串行调用；显式快照接口不访问SPI对象。
 */
scb_session_result_t scb_session_tcp_hello(scb_session_t *session, const uint8_t *bytes,
    size_t length, bridge_json_workspace_t *workspace, uint8_t *reply, size_t capacity,
    size_t *used);
/**
 * @brief 查询TCP帧是否仍有当前会话业务许可。
 * @note link_session.c::scb_session_tcp_permitted；状态接口由owner串行调用；显式快照接口不访问SPI对象。
 */
bool scb_session_tcp_permitted(const scb_session_t *session, uint32_t frame_session);
/**
 * @brief 根据显式SPI快照编码TCP握手应答。
 * @note link_session.c::scb_session_tcp_hello_explicit；状态接口由owner串行调用；显式快照接口不访问SPI对象。
 */
scb_session_result_t scb_session_tcp_hello_explicit(uint32_t session_id, bool link_ready,
    const uint8_t *bytes, size_t length, bridge_json_workspace_t *workspace, uint8_t *reply,
    size_t capacity, size_t *used);
/**
 * @brief 根据SPI快照编码独立BUSY拒绝帧。
 * @note link_session.c::scb_session_tcp_busy；状态接口由owner串行调用；显式快照接口不访问SPI对象。
 */
scb_session_result_t scb_session_tcp_busy(uint32_t session_id, bool link_ready,
    bridge_json_workspace_t *workspace, uint8_t *reply, size_t capacity, size_t *used);
#endif
