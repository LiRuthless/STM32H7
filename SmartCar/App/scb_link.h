/**
 * @file scb_link.h
 * @brief SCB会话、可靠传输及32项业务缓存接口。
 * @details 前台串行传输与命令分发，业务处理器在最终结果容量预留后调用；见 scb_link.c。
 */
#ifndef SMARTCAR_APP_SCB_LINK_H
#define SMARTCAR_APP_SCB_LINK_H
#include "scb_protocol.h"

#define SCB_REQUEST_CAPACITY 32u

typedef struct
{
    uint32_t session_id; /**< 当前会话，握手前0。 */
    uint32_t next_seq; /**< 下一正常发送编号。 */
    uint32_t ack_seq; /**< 最近接管可靠输入。 */
    uint32_t received; /**< 当前会话合法输入数。 */
    uint32_t resets; /**< 重握手累计数。 */
    uint8_t ready; /**< IDLE确认已生成。 */
    uint8_t retries; /**< 当前可靠输出重发次数。 */
    uint8_t reliable; /**< 当前是否有可靠输出在途。 */
} scb_link_status_t;

typedef void (*scb_command_handler_t)(uint32_t request_id, const scb_command_t *command);
typedef void (*scb_session_handler_t)(void);

/** @brief 初始化BSP与传输缓存；见 scb_link.c。 */
void SCB_Init(void);
/** @brief 设置前台命令/会话重建钩子；见 scb_link.c。 */
void SCB_SetHandlers(scb_command_handler_t command, scb_session_handler_t session);
/** @brief 取消旧传输和未完成缓存并重新HELLO；见 scb_link.c。 */
void SCB_ResetSession(uint32_t now);
/** @brief 处理已完成帧，坏帧不确认；见 scb_link.c。 */
uint8_t SCB_ReceiveFrame(const uint8_t *input, uint32_t now);
/** @brief 生成下一事务，包含超时重握手，返回1有帧；见 scb_link.c。 */
uint8_t SCB_PrepareFrame(uint8_t *output, uint32_t now);
/** @brief 服务一次非阻塞SPI通信；见 scb_link.c。 */
void SCB_Service(void);
/** @brief 更新已预留请求结果，final为0时保留未完成槽；见 scb_link.c。 */
uint8_t SCB_QueueResult(uint32_t request_id, uint8_t type,
    const uint8_t *payload, uint16_t length, uint8_t final);
/** @brief 复制缓存结果，返回0表示未找到；见 scb_link.c。 */
uint8_t SCB_GetRequestResult(uint32_t request_id, uint8_t *type,
    uint8_t *payload, uint16_t *length);
/** @brief 复制传输状态；见 scb_link.c。 */
void SCB_GetStatus(scb_link_status_t *status);

#endif
