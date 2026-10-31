/**
 * @file scb_link.c
 * @brief SCB会话握手、停止等待重发和业务结果预留。
 * @details 前台从BSP接收完整事务，先校验帧/JSON，再预留32项缓存后分发命令。
 *     每方向单可靠在途，IDLE独立确认；200ms重发三次，800ms无合法帧重新握手。
 *     本模块不触发车辆停车；会话钩子仅取消未应用请求和导出，保留已生效值。
 */
#include "scb_link.h"
#include "telemetry.h"
#include "bsp.h"
#include <string.h>
#include <stdio.h>

typedef struct
{
    uint32_t request_id; /**< 会话内业务编号，0为空。 */
    uint32_t age; /**< 分配顺序，只有可淘汰槽参与比较。 */
    uint16_t original_length; /**< 原请求字节数。 */
    uint16_t result_length; /**< 当前结果字节数。 */
    uint8_t result_type; /**< RESULT或LOG_DATA。 */
    uint8_t pending; /**< 业务未终结，不允许淘汰。 */
    uint8_t ready; /**< 当前结果等待发送。 */
    uint8_t original[SCB_PAYLOAD_MAX]; /**< 去重保留完整原文。 */
    uint8_t result[SCB_PAYLOAD_MAX]; /**< 接管前已经预留的最终结果空间。 */
} scb_request_entry_t;

/* 全部传输状态由前台独占；业务ISR不能直接访问缓存或格式化JSON。 */
static scb_link_status_t s_link;
static scb_request_entry_t s_requests[SCB_REQUEST_CAPACITY];
static uint8_t s_reliable[SCB_SPI_SIZE];
static uint32_t s_reliable_id;
static uint32_t s_reliable_seq;
static uint32_t s_sent_at;
static uint32_t s_last_rx;
static uint32_t s_last_poll;
static uint32_t s_last_hello;
static uint32_t s_age;
static uint8_t s_confirm;
static uint8_t s_prepared;
static uint8_t s_reject_ready;
static uint32_t s_reject_id;
static uint16_t s_reject_length;
static uint8_t s_reject[128];
/* 缓存内错误结果与独立拒绝槽分离，避免覆盖尚未发送的BUSY。 */
static uint8_t s_error_result[128];
static scb_json_t s_hello_json;
static scb_command_handler_t s_command_handler;
static scb_session_handler_t s_session_handler;
/* 多上下文生产固定描述，短临界区交接；格式化和可靠发送仅前台执行。 */
static scb_event_t s_events[32];
static uint32_t s_event_head;
static uint32_t s_event_tail;
static scb_event_t s_gap_event;
static uint32_t s_gap_count;
static uint8_t s_payload[SCB_PAYLOAD_MAX];

/**
 * @brief 按业务编号查找缓存。
 * @details 最多扫描32项，不以CRC代替原文身份比较。
 * @param[in] request_id 非零请求编号。
 * @return 命中模块私有槽，缺失NULL。
 * @note 仅前台使用，返回值不交给其他模块。
 */
static scb_request_entry_t *SCB_FindRequest(uint32_t request_id)
{
    uint8_t i;
    for (i = 0; i < SCB_REQUEST_CAPACITY; ++i)
    {
        if (s_requests[i].request_id == request_id)
        {
            return &s_requests[i];
        }
    }
    return NULL;
}

/**
 * @brief 分配空槽或最旧可淘汰槽。
 * @details 未完成、待发送或正在可靠发送的结果不能淘汰，确保先预留再执行。
 * @return 可复用槽或NULL；不修改槽内容。
 * @note 仅前台调用，容量耗尽必须返回BUSY且不执行业务。
 */
static scb_request_entry_t *SCB_AllocateRequest(void)
{
    uint8_t i;
    scb_request_entry_t *oldest = NULL;
    for (i = 0; i < SCB_REQUEST_CAPACITY; ++i)
    {
        scb_request_entry_t *entry = &s_requests[i];
        if (!entry->request_id)
        {
            return entry;
        }
        if (!entry->pending && !entry->ready && entry->request_id != s_reliable_id
            && (!oldest || entry->age < oldest->age))
        {
            oldest = entry;
        }
    }
    return oldest;
}

/**
 * @brief 注册业务和会话钩子。
 * @param[in] command 可空命令处理器；空时合法请求返回INVALID_ARG。
 * @param[in] session 可空会话取消钩子。
 * @note 仅前台初始化时设置，钩子不在IRQ调用。
 */
void SCB_SetHandlers(scb_command_handler_t command, scb_session_handler_t session)
{
    s_command_handler = command;
    s_session_handler = session;
}

/**
 * @brief 丢弃旧会话传输状态并重新握手。
 * @details 先通知业务取消未应用请求和导出，随后清请求/可靠缓存；不接触车辆起停及有效参数。
 * @param[in] now 当前毫秒，用无符号差处理回绕。
 * @note 前台调用，ResetSession不重新初始化DMA或复用在途缓冲。
 */
void SCB_ResetSession(uint32_t now)
{
    uint32_t resets = s_link.resets + 1;
    uint32_t mask;
    if (s_session_handler)
    {
        s_session_handler();
    }
    memset(&s_link, 0, sizeof(s_link));
    memset(s_requests, 0, sizeof(s_requests));
    s_link.resets = resets;
    s_link.next_seq = 1;
    s_reliable_id = s_reliable_seq = s_age = 0;
    s_confirm = s_prepared = s_reject_ready = 0;
    s_last_rx = now;
    s_last_hello = now - 200u;
    s_last_poll = now - 5u;
    Telemetry_ResetSession();
    mask = BSP_Time_EnterCritical();
    s_event_head = 0;
    s_event_tail = 0;
    s_gap_count = 0;
    BSP_Time_ExitCritical(mask);
}

/**
 * @brief 初始化物理链和协议缓存。
 * @details 先初始化BSP静态事务，再建立session为零的HELLO状态。
 * @note 上电前台调用一次；命令钩子通过SetHandlers另行设置。
 */
void SCB_Init(void)
{
    Telemetry_Init();
    BSP_WiFi_Init();
    SCB_ResetSession(BSP_Time_GetTick());
}

/**
 * @brief 发布一个固定事件快照。
 * @details 满队列后锁存首个遗漏位置并累计gap；gap发送前不混入更新事件，保证遗漏显式可见。
 * @param[in] event 非空发生上下文固定的描述，kind须为已定义事件。
 * @return 入队1，非法类型或已累计入gap返回0；不影响请求结果预留。
 * @note ISR或前台可调用，只做固定大小复制，不格式化、不等待、不访问Flash。
 */
uint8_t SCB_QueueEvent(const scb_event_t *event)
{
    uint32_t mask;
    if (event->kind < SCB_EVENT_RUN_STARTED || event->kind > SCB_EVENT_GAP)
    {
        return 0;
    }
    mask = BSP_Time_EnterCritical();
    if (s_event_head - s_event_tail == 32u || s_gap_count)
    {
        if (!s_gap_count)
        {
            s_gap_event = *event;
            s_gap_event.kind = SCB_EVENT_GAP;
            s_gap_event.has_values = 0;
        }
        if (s_gap_count != 0xffffffffu)
        {
            ++s_gap_count;
        }
        BSP_Time_ExitCritical(mask);
        return 0;
    }
    s_events[s_event_head & 31u] = *event;
    ++s_event_head;
    BSP_Time_ExitCritical(mask);
    return 1;
}

/**
 * @brief 依序取出旧事件或其后gap。
 * @details 单次短临界区内复制描述并释放槽；gap直到旧队列排空才输出。
 * @param[out] event 非空输出，有事件时写入。
 * @return 取到事件1，空队列0。
 * @note 仅前台调用，返回副本可在中断开放后格式化。
 */
static uint8_t SCB_TakeEvent(scb_event_t *event)
{
    uint32_t mask = BSP_Time_EnterCritical();
    if (s_event_tail != s_event_head)
    {
        *event = s_events[s_event_tail & 31u];
        ++s_event_tail;
    }
    else if (s_gap_count)
    {
        *event = s_gap_event;
        event->dropped_events = s_gap_count;
        s_gap_count = 0;
    }
    else
    {
        BSP_Time_ExitCritical(mask);
        return 0;
    }
    BSP_Time_ExitCritical(mask);
    return 1;
}

/**
 * @brief 将事件副本编码为公共JSON。
 * @details 固定事件名、状态与数字全部经有界writer；附加values时仍预检容量，不截断发送。
 * @param[in] event 非空前台固定副本。
 * @return 非零负载长度，类型或容量错误为0。
 * @note 只写前台静态s_payload，不在IRQ调用。
 */
static uint16_t SCB_FormatEvent(const scb_event_t *event)
{
    /* 事件名与公共协议逐项对应，无自定义事件名。 */
    static const char *const s_names[] =
    {
        "RUN_STARTED", "RUN_STOPPED", "TUNE_APPLIED", "TUNE_SUSPENDED", "TUNE_RESUMED",
        "PARAM_SAVED", "PARAM_SAVE_FAILED", "EVENT_GAP"
    };
    scb_writer_t writer;
    SCB_JsonBegin(&writer, s_payload, sizeof(s_payload));
    SCB_JsonAppend(&writer, "{\"event\":\"");
    SCB_JsonAppend(&writer, s_names[event->kind]);
    SCB_JsonAppend(&writer, "\",\"status\":\"");
    SCB_JsonAppend(&writer, SCB_StatusName(event->status));
    SCB_JsonAppend(&writer, "\",\"run_id\":");
    SCB_JsonU32(&writer, event->run_id);
    SCB_JsonAppend(&writer, ",\"tick\":");
    SCB_JsonU32(&writer, event->tick);
    SCB_JsonAppend(&writer, ",\"control_sequence\":");
    SCB_JsonU32(&writer, event->control_sequence);
    SCB_JsonAppend(&writer, ",\"revision\":");
    SCB_JsonU32(&writer, event->revision);
    SCB_JsonAppend(&writer, ",\"stop_reason\":");
    SCB_JsonU32(&writer, event->stop_reason);
    if (event->kind == SCB_EVENT_GAP)
    {
        SCB_JsonAppend(&writer, ",\"dropped_events\":");
        SCB_JsonU32(&writer, event->dropped_events);
    }
    if (event->has_values)
    {
        SCB_JsonAppend(&writer, ",\"values\":");
        SCB_JsonValues(&writer, &event->values);
    }
    SCB_JsonAppend(&writer, "}");
    return writer.failed ? 0 : writer.length;
}

/**
 * @brief 将业务结果写到已预留槽。
 * @details 更新结果与发送标记；即使RECEIVED仍在可靠发送，最终结果也有独立缓存位置。
 * @param[in] request_id 已接管的非零编号。
 * @param[in] type 仅RESULT或LOG_DATA。
 * @param[in] payload 非空有效负载，不得指向待覆盖槽。
 * @param[in] length 不超过992B，不截断。
 * @param[in] final 非零终结请求，零保留pending。
 * @return 成功1，缺失预留槽或参数错误0，失败不更新。
 * @note 仅前台调用，ISR结果通过业务固定快照交给前台。
 */
uint8_t SCB_QueueResult(uint32_t request_id, uint8_t type,
    const uint8_t *payload, uint16_t length, uint8_t final)
{
    scb_request_entry_t *entry = SCB_FindRequest(request_id);
    if (!request_id || !entry || !payload || length > SCB_PAYLOAD_MAX
        || (type != SCB_RESULT && type != SCB_LOG_DATA))
    {
        return 0;
    }
    memcpy(entry->result, payload, length);
    entry->result_length = length;
    entry->result_type = type;
    entry->pending = !final;
    entry->ready = 1;
    return 1;
}

/**
 * @brief 复制缓存当前结果。
 * @param[in] request_id 非零查询目标。
 * @param[out] type 非空结果消息类型。
 * @param[out] payload 非空至少992B目标，命中时复制实际内容。
 * @param[out] length 非空实际结果长度。
 * @return 有当前结果1，缺失0；失败不写输出。
 * @note 前台调用，不暴露内部指针，不改变淘汰或发送顺序。
 */
uint8_t SCB_GetRequestResult(uint32_t request_id, uint8_t *type,
    uint8_t *payload, uint16_t *length)
{
    scb_request_entry_t *entry = SCB_FindRequest(request_id);
    if (!request_id || !entry || !entry->result_length)
    {
        return 0;
    }
    *type = entry->result_type;
    *length = entry->result_length;
    memcpy(payload, entry->result, entry->result_length);
    return 1;
}

/**
 * @brief 接管请求、去重并调用业务处理器。
 * @details 语法完整之后才调用；编号同原文返回当前结果，异原文单独返回INVALID_ARG。
 *     结果槽不足用固定拒绝槽返回BUSY，拒绝槽也满则不ACK，让对端可靠重试。
 * @param[in] frame 非空已校验命令帧。
 * @param[in] command 非空已解析快照。
 * @param[in] status 业务形状校验结果。
 * @return 已接管1，暂时无法保留回复0。
 */
static uint8_t SCB_AcceptRequest(const scb_frame_t *frame,
    const scb_command_t *command, scb_status_t status)
{
    scb_request_entry_t *entry = SCB_FindRequest(frame->request_id);
    if (entry)
    {
        if (entry->original_length == frame->payload_len
            && !memcmp(entry->original, frame->payload, frame->payload_len))
        {
            entry->ready = entry->result_length != 0;
            return 1;
        }
        status = SCB_INVALID_ARG;
    }
    else
    {
        entry = SCB_AllocateRequest();
        if (!entry)
        {
            status = SCB_BUSY;
        }
        else
        {
            entry->request_id = frame->request_id;
            entry->age = ++s_age;
            entry->original_length = frame->payload_len;
            memcpy(entry->original, frame->payload, frame->payload_len);
            entry->pending = 1;
            entry->result_length = 0;
            entry->ready = 0;
            if (status == SCB_OK && s_command_handler)
            {
                s_command_handler(frame->request_id, command);
                return 1;
            }
            if (status == SCB_OK)
            {
                status = SCB_INVALID_ARG;
            }
            uint16_t length = (uint16_t)snprintf((char *)s_error_result, sizeof(s_error_result),
                "{\"op\":\"%s\",\"status\":\"%s\"}",
                command->op[0] ? command->op : "INVALID", SCB_StatusName(status));
            return SCB_QueueResult(frame->request_id, SCB_RESULT, s_error_result, length, 1);
        }
    }
    if (s_reject_ready)
    {
        return 0;
    }
    s_reject_id = frame->request_id;
    s_reject_length = (uint16_t)snprintf((char *)s_reject, sizeof(s_reject),
        "{\"op\":\"%s\",\"status\":\"%s\"}",
        command->op[0] ? command->op : "INVALID", SCB_StatusName(status));
    s_reject_ready = 1;
    return 1;
}

/**
 * @brief 只按精确ACK释放单可靠在途。
 * @details 释放传输副本，不改变业务pending或把RECEIVED解释为生效。
 * @param[in] ack 对端头部ACK编号。
 */
static void SCB_HandleAck(uint32_t ack)
{
    if (s_link.reliable && ack == s_reliable_seq)
    {
        s_link.reliable = 0;
        s_link.retries = 0;
        s_reliable_id = 0;
    }
}

/**
 * @brief 接收完整事务并验证会话及命令。
 * @details HELLO_ACK仅在握手状态接受，验证JSON中的协议/会话/ready。
 *     正常命令先完整解析再更新ACK；可靠去重只比较可靠序号，不受更大IDLE序号干扰。
 * @param[in] input 非空1024B事务。
 * @param[in] now 接收完成毫秒。
 * @return 合法并接管1，坏帧/旧会话/容量未接管0。
 * @note 前台调用；旧会话、非法JSON不续租800ms计时，也不执行部分业务。
 */
uint8_t SCB_ReceiveFrame(const uint8_t *input, uint32_t now)
{
    scb_frame_t frame;
    scb_command_t command;
    scb_status_t status;
    if (!SCB_DecodeFrame(input, SCB_SPI_SIZE, &frame))
    {
        return 0;
    }
    if (frame.type == SCB_HELLO_ACK && !s_link.ready)
    {
        double protocol;
        double session;
        int16_t ready;
        if (!frame.session_id || frame.seq || !SCB_ParseJson(frame.payload,
            frame.payload_len, &s_hello_json)
            || !SCB_GetNumber(&s_hello_json,
                SCB_FindMember(&s_hello_json, 0, "protocol"), &protocol)
            || protocol != 1
            || !SCB_GetNumber(&s_hello_json,
                SCB_FindMember(&s_hello_json, 0, "session_id"), &session)
            || session != frame.session_id)
        {
            return 0;
        }
        ready = SCB_FindMember(&s_hello_json, 0, "link_ready");
        if (ready < 0 || (s_hello_json.tokens[ready].type != SCB_JSON_TRUE
            && s_hello_json.tokens[ready].type != SCB_JSON_FALSE))
        {
            return 0;
        }
        s_link.session_id = frame.session_id;
        s_confirm = 1;
        s_last_rx = now;
        return 1;
    }
    if (!s_link.ready || frame.session_id != s_link.session_id || !frame.seq
        || (frame.type != SCB_COMMAND && frame.type != SCB_IDLE))
    {
        return 0;
    }
    if (frame.type == SCB_COMMAND)
    {
        if (!frame.request_id
            || !SCB_ParseCommand(frame.payload, frame.payload_len, &command, &status))
        {
            return 0;
        }
        if (frame.seq > s_link.ack_seq)
        {
            if (!SCB_AcceptRequest(&frame, &command, status))
            {
                return 0;
            }
            s_link.ack_seq = frame.seq;
        }
    }
    SCB_HandleAck(frame.ack_seq);
    s_last_rx = now;
    ++s_link.received;
    return 1;
}

/**
 * @brief 保存并启动一个可靠输出。
 * @details 编码副本保留原seq/ACK/负载，重发时不重新编码，保证逐字节相同。
 * @param[out] output 非空1024B发送目标。
 * @param[in] type 消息种类。
 * @param[in] id 业务编号。
 * @param[in] data 非空实际负载。
 * @param[in] length 负载长度，已验证不超过992。
 * @param[in] now 当前毫秒。
 */
static void SCB_StartReliable(uint8_t *output, uint8_t type, uint32_t id,
    const uint8_t *data, uint16_t length, uint32_t now)
{
    scb_frame_t frame =
    {
        type, SCB_RELIABLE, length, 0, s_link.session_id,
        s_link.next_seq++, s_link.ack_seq, id, data
    };
    (void)SCB_EncodeFrame(s_reliable, &frame);
    memcpy(output, s_reliable, SCB_SPI_SIZE);
    s_reliable_id = id;
    s_reliable_seq = frame.seq;
    s_link.reliable = 1;
    s_link.retries = 0;
    s_sent_at = now;
}

/**
 * @brief 选择握手、重发、结果或IDLE。
 * @details 先检查800ms失联/序号边界，再优先重发和结果；等待ACK仍每5ms发送IDLE。
 *     可靠输出失败三次后重新HELLO，业务状态由会话钩子处理而非停车。
 * @param[out] output 非空1024B目标。
 * @param[in] now 当前毫秒。
 * @return 有待发送帧1，未到轮询时刻0。
 * @note 前台仅在有READY许可且空闲时调用；生成后必须保留直到实际启动，不重复生成。
 */
uint8_t SCB_PrepareFrame(uint8_t *output, uint32_t now)
{
    uint8_t i;
    uint16_t length;
    uint16_t count;
    scb_event_t event;
    scb_frame_t frame = { SCB_IDLE, 0, 0, 0, 0, 0, 0, 0, NULL };
    if (s_link.ready && ((uint32_t)(now - s_last_rx) >= 800u
        || s_link.next_seq >= 0xffffff00u))
    {
        SCB_ResetSession(now);
    }
    if (s_confirm)
    {
        frame.session_id = s_link.session_id;
        frame.seq = s_link.next_seq++;
        s_link.ready = 1;
        s_confirm = 0;
        s_last_poll = now;
        return SCB_EncodeFrame(output, &frame);
    }
    if (!s_link.ready)
    {
        /* 握手固定负载，无运行状态或动态字符串。 */
        static const uint8_t s_hello[] = "{\"role\":\"stm32\",\"protocol\":1}";
        if ((uint32_t)(now - s_last_hello) < 200u)
        {
            return 0;
        }
        s_last_hello = now;
        frame.type = SCB_HELLO;
        frame.payload = s_hello;
        frame.payload_len = sizeof(s_hello) - 1;
        return SCB_EncodeFrame(output, &frame);
    }
    if (s_link.reliable && (uint32_t)(now - s_sent_at) >= 200u)
    {
        if (s_link.retries == 3)
        {
            SCB_ResetSession(now);
            return SCB_PrepareFrame(output, now);
        }
        ++s_link.retries;
        s_sent_at = now;
        memcpy(output, s_reliable, SCB_SPI_SIZE);
        return 1;
    }
    if (!s_link.reliable)
    {
        if (s_reject_ready)
        {
            SCB_StartReliable(output, SCB_RESULT, s_reject_id, s_reject, s_reject_length, now);
            s_reject_ready = 0;
            return 1;
        }
        for (i = 0; i < SCB_REQUEST_CAPACITY; ++i)
        {
            scb_request_entry_t *entry = &s_requests[i];
            if (entry->ready)
            {
                SCB_StartReliable(output, entry->result_type, entry->request_id,
                    entry->result, entry->result_length, now);
                entry->ready = 0;
                return 1;
            }
        }
        if (SCB_TakeEvent(&event))
        {
            length = SCB_FormatEvent(&event);
            if (length)
            {
                SCB_StartReliable(output, SCB_EVENT, 0, s_payload, length, now);
                return 1;
            }
            /* 非有限内部快照或编码容量失败也必须显式报告遗漏，不能静默消费事件。 */
            event.kind = SCB_EVENT_GAP;
            event.has_values = 0;
            event.dropped_events = 1;
            event.status = SCB_IO_ERROR;
            (void)SCB_QueueEvent(&event);
        }
    }
    if (Telemetry_Service(s_payload, &length, &count, now))
    {
        frame.type = SCB_TELEMETRY;
        frame.payload = s_payload;
        frame.payload_len = length;
        frame.record_count = count;
        frame.session_id = s_link.session_id;
        frame.seq = s_link.next_seq++;
        frame.ack_seq = s_link.ack_seq;
        s_last_poll = now;
        return SCB_EncodeFrame(output, &frame);
    }
    if ((uint32_t)(now - s_last_poll) < 5u)
    {
        return 0;
    }
    frame.session_id = s_link.session_id;
    frame.seq = s_link.next_seq++;
    frame.ack_seq = s_link.ack_seq;
    s_last_poll = now;
    return SCB_EncodeFrame(output, &frame);
}

/**
 * @brief 前台服务一次物理交换。
 * @details 先消费完成RX再尝试下一事务；HAL_BUSY时保留已生成帧，不增加序号或重发次数。
 * @note 无等待READY，不解析在途缓冲；Flash服务先于本调用。
 */
void SCB_Service(void)
{
    const uint8_t *rx;
    uint8_t *tx;
    bsp_wifi_status_t wifi;
    uint32_t now = BSP_Time_GetTick();
    BSP_WiFi_Service();
    rx = BSP_WiFi_GetRx();
    if (rx)
    {
        (void)SCB_ReceiveFrame(rx, now);
        BSP_WiFi_ReleaseRx();
    }
    /* 即使没有READY许可也执行失联取消；不依赖PrepareFrame被硬件调用。 */
    if (s_link.ready && (uint32_t)(now - s_last_rx) >= 800u)
    {
        SCB_ResetSession(now);
    }
    BSP_WiFi_GetStatus(&wifi);
    tx = BSP_WiFi_GetTx();
    if (!tx || wifi.permits == wifi.consumed)
    {
        return;
    }
    if (!s_prepared)
    {
        s_prepared = SCB_PrepareFrame(tx, now);
    }
    if (s_prepared)
    {
        uint8_t started = BSP_WiFi_TryExchange();
        if (started == 1 && s_link.reliable && SCB_ReadU32(tx + 16) == s_reliable_seq)
        {
            /* HAL_BUSY可能推迟真正启动，200ms从实际启动计，不从首次准备缓冲计。 */
            s_sent_at = now;
        }
        if (started)
        {
            s_prepared = 0;
        }
    }
}

/**
 * @brief 复制前台拥有的链路状态。
 * @param[out] status 非空调用方对象。
 * @note 前台串行调用，无内部可写指针逃逸。
 */
void SCB_GetStatus(scb_link_status_t *status)
{
    *status = s_link;
}
