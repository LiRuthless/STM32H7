/**
 * @file bridge_state.c
 * @brief 生产桥接固定信用、接管与连接隔离。
 * @details SPI与NET各持串行租借，短锁保护信用及代际；解析在owner工作区锁外执行。
 *     COMMAND复制到SPI可靠缓存后保留信用，最终RESULT仅在TCP精确ACK后释放。
 *     本模块不访问驱动DMA、socket或车辆状态，不进行运行期分配。
 */
#include "bridge_state.h"
#include "buffer_pool.h"
#include <string.h>
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"

enum
{
    BRIDGE_CLASS_COUNT = 3, /**< 普通、查询、导出信用数。 */
    BRIDGE_QUERY_CLASS = 1, /**< 只读查询类别。 */
    BRIDGE_EXPORT_CLASS = 2 /**< 导出类别。 */
};

typedef struct
{
    scb_header_t header; /**< 原业务头；编码时重建传输身份。 */
    bool occupied; /**< 数据尚未归还。 */
    bool borrowed; /**< 锁外编码期间不得复用。 */
    bool revoked; /**< 清理已撤销，归还时释放。 */
    uint64_t epoch; /**< 建立租借时的bridge代际。 */
} bridge_frame_t;

typedef struct
{
    uint32_t request; /**< 当前信用请求ID。 */
    uint32_t tombstone; /**< 非零则该类隔离到SPI会话改变。 */
    bool active; /**< 正在等待结果或最终精确ACK。 */
    bool sent; /**< COMMAND已交SPI编码路径，不能撤回。 */
    unsigned operation; /**< 导出操作：0其他、1BEGIN、2READ、3END。 */
    uint32_t transfer; /**< READ/END请求的transfer_id。 */
    uint32_t offset; /**< READ请求字节偏移。 */
    uint32_t length; /**< READ请求字节数，1～960。 */
    bool read_valid; /**< READ线格式字段均存在且无范围溢出。 */
} bridge_credit_t;

/* 数据额度由R-05批准；所有字节和元数据由s_mux保护，借出期间清理只置revoked。 */
static uint8_t s_commands[BRIDGE_CLASS_COUNT][SCB_SPI_BYTES];
static uint8_t s_results[BRIDGE_CLASS_COUNT * 2][SCB_SPI_BYTES];
static bridge_frame_t s_command[BRIDGE_CLASS_COUNT];
static bridge_frame_t s_result[BRIDGE_CLASS_COUNT * 2];
static bridge_credit_t s_credit[BRIDGE_CLASS_COUNT];
static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
static bridge_identity_t s_identity;
static bridge_state_snapshot_t s_stats;
static uint64_t s_epoch = 1;
static int s_net_reservation = -1;
static int s_spi_reservation = -1;
static int s_spi_loan = -1;
static int s_net_loan = -1;
static int s_net_pending = -1;
static uint32_t s_net_seq;
/* 仅SPI任务写入；reserve成功至commit/cancel期间保存RESULT状态判定。 */
static bool s_spi_result_ok;
/* 导出活动状态由同一bridge锁保护，时间由NET轮询及SPI提交采样。 */
static bool s_export_active;
static uint64_t s_export_progress;
static uint32_t s_export_transfer;
static uint32_t s_export_total;
static uint32_t s_export_begin;
static uint32_t s_export_read;
static bridge_credit_t s_net_metadata;
static uint64_t s_gap_epoch;

typedef struct
{
    bridge_pool_slot_t slot; /**< 已取得的固定池凭据。 */
    scb_header_t header; /**< 独立源帧头。 */
    uint64_t epoch; /**< 入队归属代际。 */
} bridge_queue_item_t;

typedef struct
{
    uint32_t values[4]; /**< 首源run_id/tick/control_sequence/revision。 */
    bool valid; /**< 四字段均为精确u32。 */
} bridge_anchor_t;

/* bridge锁保护FIFO及锚点；池调用固定顺序为bridge→pool，无反向获取。 */
static bridge_queue_item_t s_events[BRIDGE_EVENT_SLOTS];
static bridge_queue_item_t s_telemetry[BRIDGE_UDP_SLOTS];
static unsigned s_event_count;
static unsigned s_udp_count;
static bridge_queue_item_t s_event_reserved;
static bool s_event_has_reservation;
static bridge_anchor_t s_event_anchor_reserved;
static bridge_anchor_t s_event_anchors[BRIDGE_EVENT_SLOTS];
static bridge_anchor_t s_pending_anchor;
/* 已知历史缺口与当前在途源事件分别保存，不能由新事件覆盖旧缺口来源。 */
static bridge_anchor_t s_gap_anchor;
static bool s_gap_needed;
static bool s_gap_loan;
static bool s_event_loan;
static bool s_event_pending;
static bool s_gap_pending;
static uint32_t s_event_seq;
static uint8_t s_anchor_data[SCB_SPI_BYTES];
_Static_assert(sizeof(s_commands) + sizeof(s_results) + sizeof(s_anchor_data) == 10240u,
    "R-05 additional frame data budget");
static size_t s_anchor_length;
static bool s_udp_loan;
static bridge_queue_item_t s_udp_borrowed;
/* 导出期限服务在帧清理定义前使用同一锁内撤销规则。 */
static void revoke_frame(bridge_frame_t *frame);

/**
 * @brief 锁内推进导出期限，不改变驱动持有的传输缓存。
 * @details 期限到达只撤销本端导出，已送SPI请求转墓碑；借出帧仍等待归还。
 * @param[in] now 当前单调毫秒时间。
 */
static void expire_export(uint64_t now)
{
    if (!s_export_active || now < s_export_progress || now - s_export_progress < 10000u)
    {
        return;
    }
    s_export_active = false;
    ++s_stats.export_timeouts;
    bridge_credit_t *credit = &s_credit[BRIDGE_EXPORT_CLASS];
    revoke_frame(&s_command[BRIDGE_EXPORT_CLASS]);
    if (credit->active)
    {
        if (credit->sent)
        {
            credit->tombstone = credit->request;
        }
        credit->active = false;
        credit->request = 0;
        revoke_frame(&s_result[BRIDGE_EXPORT_CLASS * 2]);
        revoke_frame(&s_result[BRIDGE_EXPORT_CLASS * 2 + 1]);
        if (s_net_pending / 2 == BRIDGE_EXPORT_CLASS)
        {
            s_net_pending = -1;
        }
    }
}

/**
 * @brief 锁外提取首源事件的可信设备字段。
 * @param[in] view 完整已验证源事件。
 * @return 返回锚点，任一字段缺失或非精确u32则valid=false。
 */
static bridge_anchor_t event_anchor(const scb_frame_view_t *view)
{
    static const char *const keys[] = {"run_id", "tick", "control_sequence", "revision"};
    bridge_anchor_t anchor = {.valid = true};
    for (unsigned i = 0; i < 4; ++i)
    {
        bridge_json_view_t field;
        if (!bridge_json_object_get_validated(
                (bridge_json_view_t){view->payload, view->header.payload_len}, keys[i], &field)
            || !bridge_json_u32(field, &anchor.values[i]))
        {
            anchor.valid = false;
        }
    }
    return anchor;
}

/**
 * @brief 锁内保存首个已知无法交付事件，缺源字段时要求关闭查询。
 * @details 已有缺口不被后续源覆盖；缺设备字段只发关闭待办，不生成猜值报告。
 * @param[in] anchor 首个缺失源的设备字段副本。
 */
static void remember_gap(bridge_anchor_t anchor)
{
    if (!s_gap_needed)
    {
        s_gap_anchor = anchor;
        s_gap_needed = true;
        if (!anchor.valid)
        {
            s_stats.close_requested = true;
        }
    }
}

/**
 * @brief 锁内清理未借出的遥测FIFO并累计生命周期丢弃包数。
 * @details 按原FIFO顺序归还池槽；不触碰NET独占借用，保持bridge到pool的锁顺序。
 * @pre 调用方已持有s_mux，不在本函数重新取得bridge锁。
 */
static void clear_udp_queue(void)
{
    for (unsigned i = 0; i < s_udp_count; ++i)
    {
        (void)bridge_pool_release(&s_telemetry[i].slot);
        ++s_stats.telemetry_dropped;
        ++s_stats.telemetry_cleared;
    }
    s_udp_count = 0;
}

/**
 * @brief 锁内清FIFO，借出的数据归还前不能复用。
 * @details 先保留旧会话首缺口，再释放未借出的池槽；新SPI会话清旧缺口身份。
 * @param[in] new_session SPI换会话时清旧锚点。
 */
static void clear_queues(bool new_session)
{
    if (!new_session)
    {
        if (s_event_pending)
        {
            remember_gap(s_pending_anchor);
        }
        if (s_event_count > 0)
        {
            remember_gap(s_event_anchors[0]);
        }
    }
    unsigned keep = s_event_loan && s_event_count > 0 ? 1u : 0u;
    for (unsigned i = keep; i < s_event_count; ++i)
    {
        (void)bridge_pool_release(&s_events[i].slot);
    }
    s_event_count = keep;
    clear_udp_queue();
    s_event_pending = false;
    s_gap_pending = false;
    if (new_session)
    {
        s_gap_needed = false;
        s_pending_anchor = (bridge_anchor_t){0};
        s_gap_anchor = (bridge_anchor_t){0};
        s_stats.close_requested = false;
    }
}


/**
 * @brief 锁外读取对象的精确u32字段，不解释车辆值。
 * @param[in] object 已验证对象字节视图。
 * @param[in] key 顶层字段名。
 * @param[out] value 成功写入字段，失败置零。
 * @return 存在且可精确表示返回true。
 */
static bool object_u32(bridge_json_view_t object, const char *key, uint32_t *value)
{
    bridge_json_view_t field;
    *value = 0;
    return bridge_json_object_get_validated(object, key, &field)
        && bridge_json_u32(field, value);
}

/**
 * @brief 锁外读取COMMAND导出匹配元数据，不据此生成业务结果。
 * @param[in] view 已验证COMMAND。
 * @param[in] op 本帧已提取的操作名，借用原始已验证JSON。
 * @return 仅供块匹配的线字段，非法业务仍透明交STM32判断。
 */
static bridge_credit_t command_metadata(const scb_frame_view_t *view, bridge_json_view_t op)
{
    bridge_credit_t metadata = {0};
    if (bridge_json_string_equal(op, "LOG_BEGIN"))
    {
        metadata.operation = 1;
    }
    else if (bridge_json_string_equal(op, "LOG_READ"))
    {
        metadata.operation = 2;
    }
    else if (bridge_json_string_equal(op, "LOG_END"))
    {
        metadata.operation = 3;
    }
    bridge_json_view_t args;
    if (metadata.operation >= 2 && bridge_json_object_get_validated(
            (bridge_json_view_t){view->payload, view->header.payload_len}, "args", &args))
    {
        const bool transfer = object_u32(args, "transfer_id", &metadata.transfer);
        metadata.read_valid = transfer
            && object_u32(args, "offset", &metadata.offset)
            && object_u32(args, "length", &metadata.length)
            && metadata.length >= 1 && metadata.length <= 960
            && metadata.offset <= UINT32_MAX - metadata.length;
    }
    return metadata;
}

/**
 * @brief 仅按操作名分配传输信用类别。
 * @param[in] op 本帧已提取的操作名，借用原始已验证JSON。
 * @return 返回0普通、1查询或2导出，不判车辆许可。
 */
static int command_class(bridge_json_view_t op)
{
    static const char *const queries[] =
    {
        "GET_STATUS", "GET_PARAMS", "GET_REQUEST_RESULT", "GET_LOG_META"
    };
    static const char *const exports[] = {"LOG_BEGIN", "LOG_READ", "LOG_END"};
    for (unsigned i = 0; i < sizeof(queries) / sizeof(queries[0]); ++i)
    {
        if (bridge_json_string_equal(op, queries[i]))
        {
            return BRIDGE_QUERY_CLASS;
        }
    }
    for (unsigned i = 0; i < sizeof(exports) / sizeof(exports[0]); ++i)
    {
        if (bridge_json_string_equal(op, exports[i]))
        {
            return BRIDGE_EXPORT_CLASS;
        }
    }
    return 0;
}

/**
 * @brief 锁内撤销固定帧而保留尚在锁外使用的数据。
 * @param[in,out] frame 待撤销帧元数据。
 */
static void revoke_frame(bridge_frame_t *frame)
{
    if (frame->borrowed)
    {
        frame->revoked = true;
    }
    else
    {
        *frame = (bridge_frame_t){0};
    }
}

/**
 * @brief 锁内清理连接信用，必要时保留迟答墓碑。
 * @details 新SPI会话可清墓碑，普通断线必须隔离已经交给SPI的请求。
 * @param[in] new_session true表示旧SPI身份永久失效。
 */
static void clear_connection(bool new_session)
{
    clear_queues(new_session);
    s_export_active = false;
    s_export_read = 0;
    s_export_begin = 0;
    ++s_epoch;
    for (int i = 0; i < BRIDGE_CLASS_COUNT; ++i)
    {
        if (new_session)
        {
            s_credit[i] = (bridge_credit_t){0};
        }
        else
        {
            if (s_credit[i].active && s_credit[i].sent)
            {
                s_credit[i].tombstone = s_credit[i].request;
            }
            s_credit[i].active = false;
            s_credit[i].request = 0;
        }
        revoke_frame(&s_command[i]);
        revoke_frame(&s_result[i * 2]);
        revoke_frame(&s_result[i * 2 + 1]);
    }
    s_net_pending = -1;
    s_net_seq = 0;
}

/**
 * @brief NET发布连接身份并同步撤销旧归属。
 * @details 同一连接更新订阅不撤销信用；代际/会话改变或关闭执行清理。
 * @param[in] identity NET独占状态生成的一致副本。
 */
void bridge_state_identity(bridge_identity_t identity)
{
    portENTER_CRITICAL(&s_mux);
    if (identity.generation != s_identity.generation || identity.session != s_identity.session
        || (!identity.open && s_identity.open))
    {
        clear_connection(false);
    }
    else if (identity.subscription != s_identity.subscription
        || (!identity.subscribed && s_identity.subscribed))
    {
        clear_udp_queue();
    }
    s_identity = identity;
    portEXIT_CRITICAL(&s_mux);
}

/**
 * @brief NET为可靠COMMAND实际锁定信用及固定帧。
 * @details 锁外分类；锁内验证SPI和连接身份，重复ID不能占用另一类信用。
 * @param[in] view 本次已完整验证COMMAND，不能为空。
 * @param[in] op NET已从本帧提取的操作名视图，不存在时为空。
 * @return 取得真实预留返回true，否则背压。
 */
bool bridge_state_net_reserve(const scb_frame_view_t *view, bridge_json_view_t op)
{
    const int kind = command_class(op);
    const bridge_credit_t metadata = command_metadata(view, op);
    portENTER_CRITICAL(&s_mux);
    bool allowed = s_net_reservation < 0 && s_stats.ready && s_identity.open
        && s_identity.session == s_stats.session && view->header.session_id == s_stats.session
        && view->header.request_id != 0 && !s_credit[kind].active
        && !s_credit[kind].tombstone && !s_command[kind].occupied;
    for (int i = 0; i < BRIDGE_CLASS_COUNT; ++i)
    {
        if ((s_credit[i].active && s_credit[i].request == view->header.request_id)
            || s_credit[i].tombstone == view->header.request_id)
        {
            allowed = false;
        }
    }
    if (allowed)
    {
        s_net_reservation = kind;
        s_net_metadata = metadata;
        s_command[kind] = (bridge_frame_t){.occupied = true, .borrowed = true, .epoch = s_epoch};
    }
    else
    {
        ++s_stats.command_backpressure;
    }
    portEXIT_CRITICAL(&s_mux);
    return allowed;
}

/**
 * @brief NET提交已预留COMMAND或显式取消失去归属的提交。
 * @details 清理可能在reserve与commit间发生；保留槽在本次归还前不复用。
 * @param[in] view DELIVER帧，仅本次调用借用。
 */
void bridge_state_net_commit(const scb_frame_view_t *view)
{
    portENTER_CRITICAL(&s_mux);
    const int i = s_net_reservation;
    if (i >= 0)
    {
        bridge_frame_t *frame = &s_command[i];
        if (!frame->revoked && frame->epoch == s_epoch)
        {
            frame->header = view->header;
            memcpy(s_commands[i], view->payload, view->header.payload_len);
            frame->borrowed = false;
            s_credit[i] = s_net_metadata;
            s_credit[i].active = true;
            s_credit[i].request = view->header.request_id;
            s_credit[i].sent = false;
            ++s_stats.commands;
        }
        else
        {
            *frame = (bridge_frame_t){0};
        }
        s_net_reservation = -1;
    }
    portEXIT_CRITICAL(&s_mux);
}

/** @brief NET归还可靠层未交付的预留，不改变业务信用。 */
void bridge_state_net_cancel(void)
{
    portENTER_CRITICAL(&s_mux);
    if (s_net_reservation >= 0)
    {
        s_command[s_net_reservation] = (bridge_frame_t){0};
        s_net_reservation = -1;
    }
    portEXIT_CRITICAL(&s_mux);
}

/**
 * @brief SPI预留RESULT的真实槽或有归因的丢弃接管。
 * @details 锁外区分RECEIVED；无归属/墓碑结果用负令牌标记，commit显式计丢弃。
 * @param[in] context 启动上下文，不使用。
 * @param[in] view 已验证可靠帧，本次回调借用。
 * @return 有槽或可明确归因丢弃返回true，否则false。
 */
static bool spi_reserve(void *context, const scb_frame_view_t *view)
{
    (void)context;
    if (view->header.type == SCB_EVENT)
    {
        const bridge_anchor_t anchor = event_anchor(view);
        portENTER_CRITICAL(&s_mux);
        bool allowed = s_spi_reservation == -1;
        s_event_has_reservation = false;
        if (allowed && s_identity.open && s_identity.session == s_stats.session)
        {
            allowed = s_event_count < BRIDGE_EVENT_SLOTS
                && bridge_pool_claim(BRIDGE_POOL_EVENT, &s_event_reserved.slot) == ESP_OK;
            s_event_has_reservation = allowed;
        }
        if (allowed)
        {
            s_spi_reservation = -4;
            s_event_reserved.header = view->header;
            s_event_reserved.epoch = s_epoch;
            s_event_anchor_reserved = anchor;
        }
        else
        {
            ++s_stats.event_backpressure;
        }
        portEXIT_CRITICAL(&s_mux);
        return allowed;
    }
    if (view->header.type != SCB_RESULT && view->header.type != SCB_LOG_DATA)
    {
        return false;
    }
    bridge_json_view_t status = {0};
    if (view->header.type == SCB_RESULT)
    {
        (void)bridge_json_object_get_validated(
            (bridge_json_view_t){view->payload, view->header.payload_len}, "status", &status);
    }
    const bool intermediate = bridge_json_string_equal(status, "RECEIVED");
    s_spi_result_ok = bridge_json_string_equal(status, "OK");
    portENTER_CRITICAL(&s_mux);
    int found = -1;
    int discard = -2;
    for (int i = 0; i < BRIDGE_CLASS_COUNT; ++i)
    {
        if (s_credit[i].tombstone == view->header.request_id)
        {
            discard = -3;
        }
        if (s_credit[i].active && s_credit[i].request == view->header.request_id)
        {
            found = i * 2 + (intermediate ? 0 : 1);
        }
    }
    bool allowed = s_spi_reservation == -1;
    if (found >= 0 && view->header.type == SCB_LOG_DATA)
    {
        const bridge_credit_t *credit = &s_credit[found / 2];
        uint32_t offset = 0;
        for (unsigned j = 0; j < 4; ++j)
        {
            offset |= (uint32_t)view->payload[j] << (j * 8);
        }
        const uint32_t length = (uint32_t)view->payload[4]
            | ((uint32_t)view->payload[5] << 8);
        allowed = allowed && credit->operation == 2 && credit->read_valid
            && offset == credit->offset && length == credit->length;
    }
    if (found >= 0 && s_identity.open && discard != -3)
    {
        allowed = allowed && !s_result[found].occupied;
        if (allowed)
        {
            s_result[found] = (bridge_frame_t)
            {
                .occupied = true, .borrowed = true, .epoch = s_epoch
            };
            s_spi_reservation = found;
        }
    }
    else if (allowed)
    {
        s_spi_reservation = discard;
    }
    if (!allowed)
    {
        ++s_stats.result_backpressure;
    }
    portEXIT_CRITICAL(&s_mux);
    return allowed;
}

/**
 * @brief SPI同步复制已接管结果，或记录明确归因的迟答丢弃。
 * @details 锁外取时间及解析结果字段，锁内核对预留代际、复制并更新导出进度。
 *     若清理已撤销租借，只记录取消丢弃；不向新连接提交旧结果。
 * @param[in] context 启动上下文，不使用。
 * @param[in] view DELIVER帧，本次调用后不得保留指针。
 */
static void spi_commit(void *context, const scb_frame_view_t *view)
{
    (void)context;
    const uint64_t now = (uint64_t)esp_timer_get_time() / 1000u;
    const bool okay = view->header.type == SCB_RESULT && s_spi_result_ok;
    portENTER_CRITICAL(&s_mux);
    const int reservation = s_spi_reservation;
    const bool is_begin = reservation >= 0
        && s_credit[reservation / 2].operation == 1;
    portEXIT_CRITICAL(&s_mux);
    uint32_t transfer = 0;
    uint32_t total = 0;
    const bridge_json_view_t object = {view->payload, view->header.payload_len};
    const bool begin_valid = okay && is_begin
        && object_u32(object, "transfer_id", &transfer)
        && object_u32(object, "total_bytes", &total);
    portENTER_CRITICAL(&s_mux);
    expire_export(now);
    const int i = s_spi_reservation;
    if (i == -4)
    {
        if (s_event_has_reservation && s_event_reserved.epoch == s_epoch)
        {
            s_event_reserved.header = view->header;
            memcpy(s_event_reserved.slot.data, view->payload, view->header.payload_len);
            s_events[s_event_count] = s_event_reserved;
            s_event_anchors[s_event_count++] = s_event_anchor_reserved;
            if (s_event_count > s_stats.event_peak)
            {
                s_stats.event_peak = s_event_count;
            }
        }
        else
        {
            if (s_event_has_reservation)
            {
                (void)bridge_pool_release(&s_event_reserved.slot);
            }
            if (view->header.session_id == s_stats.session)
            {
                remember_gap(s_event_anchor_reserved);
            }
        }
        s_event_has_reservation = false;
    }
    else if (i >= 0)
    {
        bridge_frame_t *frame = &s_result[i];
        if (!frame->revoked && frame->epoch == s_epoch)
        {
            frame->header = view->header;
            memcpy(s_results[i], view->payload, view->header.payload_len);
            frame->borrowed = false;
            ++s_stats.results;
            bridge_credit_t *credit = &s_credit[i / 2];
            if (i / 2 == BRIDGE_EXPORT_CLASS)
            {
                if (credit->operation == 1 && begin_valid
                    && s_export_begin != credit->request)
                {
                    s_export_active = true;
                    s_export_progress = now;
                    s_export_transfer = transfer;
                    s_export_total = total;
                    s_export_begin = credit->request;
                    s_export_read = 0;
                }
                else if (credit->operation == 2 && view->header.type == SCB_LOG_DATA
                    && s_export_active && credit->transfer == s_export_transfer
                    && credit->offset <= s_export_total
                    && credit->length <= s_export_total - credit->offset
                    && credit->request > s_export_read)
                {
                    s_export_progress = now;
                    s_export_read = credit->request;
                }
                else if (credit->operation == 3 && okay)
                {
                    s_export_active = false;
                }
            }
        }
        else
        {
            *frame = (bridge_frame_t){0};
            ++s_stats.canceled_results;
        }
    }
    else if (i == -3)
    {
        ++s_stats.canceled_results;
    }
    else if (i == -2)
    {
        ++s_stats.orphan_results;
    }
    s_spi_reservation = -1;
    portEXIT_CRITICAL(&s_mux);
}

/**
 * @brief SPI归还重复帧或可靠拒绝的预留。
 * @details EVENT归还真实池凭据，结果归还固定槽，丢弃令牌只清预留不计交付。
 * @param[in] context 启动上下文，不使用。
 */
static void spi_cancel(void *context)
{
    (void)context;
    portENTER_CRITICAL(&s_mux);
    if (s_spi_reservation == -4 && s_event_has_reservation)
    {
        (void)bridge_pool_release(&s_event_reserved.slot);
        s_event_has_reservation = false;
    }
    if (s_spi_reservation >= 0)
    {
        s_result[s_spi_reservation] = (bridge_frame_t){0};
    }
    s_spi_reservation = -1;
    portEXIT_CRITICAL(&s_mux);
}

/**
 * @brief SPI借出一个待发COMMAND。
 * @details 锁外编码期间保留槽，先标记可能已进入SPI，断线不得假定能够撤回。
 * @param[in] context 启动上下文，不使用。
 * @param[out] view 成功时借用至spi_consumed，失败不发布。
 * @return 有可发送COMMAND返回true。
 */
static bool spi_peek(void *context, scb_frame_view_t *view)
{
    (void)context;
    portENTER_CRITICAL(&s_mux);
    bool found = false;
    for (int i = 0; i < BRIDGE_CLASS_COUNT && s_spi_loan < 0; ++i)
    {
        if (s_command[i].occupied && !s_command[i].borrowed && !s_command[i].revoked)
        {
            s_spi_loan = i;
            s_command[i].borrowed = true;
            s_credit[i].sent = true;
            *view = (scb_frame_view_t){s_command[i].header, s_commands[i]};
            found = true;
        }
    }
    portEXIT_CRITICAL(&s_mux);
    return found;
}

/**
 * @brief SPI归还编码借用，成功后释放数据但不释放请求信用。
 * @details 撤销租借无论编码结果均归还；未撤销的编码失败保留数据供后续重试。
 * @param[in] context 启动上下文，不使用。
 * @param[in] encoded true表示已复制进SPI可靠缓存。
 */
static void spi_consumed(void *context, bool encoded)
{
    (void)context;
    portENTER_CRITICAL(&s_mux);
    if (s_spi_loan >= 0)
    {
        bridge_frame_t *frame = &s_command[s_spi_loan];
        if (encoded || frame->revoked)
        {
            *frame = (bridge_frame_t){0};
        }
        else
        {
            frame->borrowed = false;
            s_credit[s_spi_loan].sent = false;
        }
        s_spi_loan = -1;
    }
    portEXIT_CRITICAL(&s_mux);
}

/**
 * @brief SPI发布逻辑会话变化，不调用NET或socket。
 * @details 会话变化先清旧队列、信用和墓碑，再发布新session/ready；不访问DMA。
 * @param[in] context 启动上下文，不使用。
 * @param[in] session SPI逻辑会话，零表示失效。
 * @param[in] ready SPI已完成IDLE握手。
 */
static void spi_session(void *context, uint32_t session, bool ready)
{
    (void)context;
    portENTER_CRITICAL(&s_mux);
    if (session != s_stats.session)
    {
        clear_connection(true);
        s_identity.open = false;
        s_identity.subscribed = false;
    }
    s_stats.session = session;
    s_stats.ready = ready;
    portEXIT_CRITICAL(&s_mux);
}

/**
 * @brief 锁内弹出最旧遥测并归还池额度。
 * @details 当前网络借用已经从FIFO分离，因此不会在此复用借出数据。
 */
static void drop_oldest_udp(void)
{
    (void)bridge_pool_release(&s_telemetry[0].slot);
    --s_udp_count;
    for (unsigned i = 0; i < s_udp_count; ++i)
    {
        s_telemetry[i] = s_telemetry[i + 1];
    }
    ++s_stats.telemetry_dropped;
    ++s_stats.telemetry_overflow;
}

/**
 * @brief SPI将遥测同步复制入有界生产FIFO，满时丢旧保新。
 * @param[in] context 启动上下文，不使用。
 * @param[in] view 已验证源遥测，本次调用借用。
 * @return 成功入队返回true，离线或无池返回false并计包。
 */
static bool spi_telemetry(void *context, const scb_frame_view_t *view)
{
    (void)context;
    portENTER_CRITICAL(&s_mux);
    if (!s_identity.open || !s_identity.subscribed || !s_stats.ready
        || s_identity.session != s_stats.session)
    {
        ++s_stats.telemetry_dropped;
        ++s_stats.telemetry_offline;
        portEXIT_CRITICAL(&s_mux);
        return false;
    }
    bridge_pool_slot_t slot;
    if (s_udp_count == BRIDGE_UDP_SLOTS)
    {
        drop_oldest_udp();
    }
    esp_err_t error = bridge_pool_claim(BRIDGE_POOL_UDP, &slot);
    if (error != ESP_OK && s_udp_count > 0)
    {
        drop_oldest_udp();
        error = bridge_pool_claim(BRIDGE_POOL_UDP, &slot);
    }
    if (error != ESP_OK)
    {
        ++s_stats.telemetry_dropped;
        ++s_stats.telemetry_failed;
        portEXIT_CRITICAL(&s_mux);
        return false;
    }
    memcpy(slot.data, view->payload, view->header.payload_len);
    s_telemetry[s_udp_count++] = (bridge_queue_item_t){slot, view->header, s_epoch};
    const unsigned used = s_udp_count + (s_udp_loan ? 1u : 0u);
    if (used > s_stats.telemetry_peak)
    {
        s_stats.telemetry_peak = used;
    }
    portEXIT_CRITICAL(&s_mux);
    return true;
}

/**
 * @brief NET取出独占遥测并写入发送时累计丢弃数。
 * @details 借出槽先从FIFO分离，直到consumed才归还；只改元数据偏移12至15。
 * @param[out] view 成功借用至udp_consumed，不能保留到归还后。
 * @return 有当前身份报文返回true，否则false。
 */
bool bridge_state_udp_peek(scb_frame_view_t *view)
{
    portENTER_CRITICAL(&s_mux);
    if (s_udp_loan || s_udp_count == 0 || !s_identity.open || !s_identity.subscribed
        || s_identity.session != s_stats.session)
    {
        portEXIT_CRITICAL(&s_mux);
        return false;
    }
    s_udp_borrowed = s_telemetry[0];
    --s_udp_count;
    for (unsigned i = 0; i < s_udp_count; ++i)
    {
        s_telemetry[i] = s_telemetry[i + 1];
    }
    s_udp_loan = true;
    uint8_t *bytes = s_udp_borrowed.slot.data;
    for (unsigned i = 0; i < 4; ++i)
    {
        bytes[12 + i] = (uint8_t)(s_stats.telemetry_dropped >> (i * 8));
    }
    *view = (scb_frame_view_t){s_udp_borrowed.header, bytes};
    portEXIT_CRITICAL(&s_mux);
    return true;
}

/**
 * @brief NET结束一次独占UDP借用，不把sendto成功当作空口接收。
 * @details 失败或租借身份已撤销计一次丢弃，再归还固定池槽，不重试socket。
 * @param[in] sent 是否完整交给当前连接的UDP出口。
 */
void bridge_state_udp_consumed(bool sent)
{
    portENTER_CRITICAL(&s_mux);
    if (s_udp_loan)
    {
        if (!sent || s_udp_borrowed.epoch != s_epoch)
        {
            ++s_stats.telemetry_dropped;
            ++s_stats.telemetry_failed;
        }
        (void)bridge_pool_release(&s_udp_borrowed.slot);
        s_udp_loan = false;
    }
    portEXIT_CRITICAL(&s_mux);
}

/**
 * @brief NET在socket调用前复核共享会话撤销事实。
 * @details 校验后释放短锁才允许socket；已交socket的字节不可撤回。
 * @return 当前借用仍属于有效连接/订阅且SPI就绪返回true。
 */
bool bridge_state_udp_valid(void)
{
    portENTER_CRITICAL(&s_mux);
    const bool valid = s_udp_loan && s_udp_borrowed.epoch == s_epoch
        && s_identity.open && s_identity.subscribed && s_stats.ready
        && s_identity.session == s_stats.session;
    portEXIT_CRITICAL(&s_mux);
    return valid;
}

/**
 * @brief NET在独占锚点数据中生成G-03报告，不解析JSON也不取设备时钟。
 * @param[in] anchor 四个可信源字段副本。
 * @return 完整JSON负载长度。
 */
static size_t format_gap(bridge_anchor_t anchor)
{
    static const char *const parts[] =
    {
        "{\"event\":\"EVENT_GAP\",\"run_id\":", ",\"tick\":",
        ",\"control_sequence\":", ",\"revision\":"
    };
    size_t used = 0;
    for (unsigned i = 0; i < 4; ++i)
    {
        for (unsigned j = 0; parts[i][j] != 0; ++j)
        {
            s_anchor_data[used++] = (uint8_t)parts[i][j];
        }
        uint8_t digits[10];
        unsigned count = 0;
        uint32_t value = anchor.values[i];
        do
        {
            digits[count++] = (uint8_t)('0' + value % 10u);
            value /= 10u;
        } while (value != 0);
        while (count > 0)
        {
            s_anchor_data[used++] = digits[--count];
        }
    }
    s_anchor_data[used++] = '}';
    return used;
}

/**
 * @brief NET借出待发结果，信用中间结果先于最终结果。
 * @details 缺口报告优先于新结果和EVENT，已有TCP在途不再借出；数据借用至consumed。
 * @param[out] view 成功借用至bridge_state_net_consumed，失败不发布。
 * @param[in,out] workspace NET独占工作区，仅用于本地生成EVENT_GAP的首次验证。
 * @return 存在可发送结果返回true。
 */
bool bridge_state_net_peek(scb_frame_view_t *view, bridge_json_workspace_t *workspace)
{
    portENTER_CRITICAL(&s_mux);
    bool found = false;
    if (!s_identity.open || s_identity.session != s_stats.session || !s_stats.ready
        || s_event_loan || s_event_pending || s_gap_loan || s_gap_pending)
    {
        portEXIT_CRITICAL(&s_mux);
        return false;
    }
    if (!found && s_net_pending < 0 && s_gap_needed && s_gap_anchor.valid)
    {
        const bridge_anchor_t anchor = s_gap_anchor;
        const uint32_t session = s_stats.session;
        s_gap_loan = true;
        s_gap_epoch = s_epoch;
        portEXIT_CRITICAL(&s_mux);
        s_anchor_length = format_gap(anchor);
        if (bridge_json_validate(s_anchor_data, s_anchor_length, workspace) != BRIDGE_JSON_OK)
        {
            portENTER_CRITICAL(&s_mux);
            s_gap_loan = false;
            portEXIT_CRITICAL(&s_mux);
            return false;
        }
        *view = (scb_frame_view_t)
        {
            .header =
            {
                .type = SCB_EVENT, .flags = SCB_RELIABLE,
                .session_id = session, .payload_len = (uint16_t)s_anchor_length
            },
            .payload = s_anchor_data
        };
        return true;
    }
    for (int i = 0; i < BRIDGE_CLASS_COUNT * 2 && s_net_loan < 0 && s_net_pending < 0; ++i)
    {
        if (s_result[i].occupied && !s_result[i].borrowed && !s_result[i].revoked)
        {
            s_net_loan = i;
            s_result[i].borrowed = true;
            *view = (scb_frame_view_t){s_result[i].header, s_results[i]};
            found = true;
        }
    }
    if (!found && s_net_pending < 0 && s_event_count > 0)
    {
        s_event_loan = true;
        *view = (scb_frame_view_t){s_events[0].header, s_events[0].slot.data};
        found = true;
    }
    portEXIT_CRITICAL(&s_mux);
    return found;
}

/**
 * @brief NET归还编码租借并绑定精确可靠序号。
 * @details EVENT转入TCP缓存后归还池槽但保留源锚点，结果数据保留到ACK；撤销租借不发布新归属。
 * @param[in] encoded true表示复制进TCP可靠缓存。
 * @param[in] seq 成功时的TCP可靠序号。
 */
void bridge_state_net_consumed(bool encoded, uint32_t seq)
{
    portENTER_CRITICAL(&s_mux);
    if (s_gap_loan)
    {
        if (encoded && s_identity.open && s_gap_epoch == s_epoch)
        {
            s_gap_pending = true;
            s_event_seq = seq;
        }
        s_gap_loan = false;
    }
    if (s_event_loan)
    {
        const bool current = s_events[0].epoch == s_epoch;
        if (encoded && current)
        {
            s_pending_anchor = s_event_anchors[0];
            s_event_pending = true;
            s_event_seq = seq;
        }
        if (encoded || !current)
        {
            (void)bridge_pool_release(&s_events[0].slot);
            --s_event_count;
            for (unsigned i = 0; i < s_event_count; ++i)
            {
                s_events[i] = s_events[i + 1];
                s_event_anchors[i] = s_event_anchors[i + 1];
            }
        }
        s_event_loan = false;
    }
    if (s_net_loan >= 0)
    {
        bridge_frame_t *frame = &s_result[s_net_loan];
        frame->borrowed = false;
        if (frame->revoked)
        {
            *frame = (bridge_frame_t){0};
        }
        else if (encoded)
        {
            s_net_pending = s_net_loan;
            s_net_seq = seq;
        }
        s_net_loan = -1;
    }
    portEXIT_CRITICAL(&s_mux);
}

/**
 * @brief NET在可靠层确认精确ACK后归还帧及最终信用。
 * @details 仅匹配本端绑定seq；中间结果ACK不释放信用，EVENT/GAP ACK只清对应锚点状态。
 * @param[in] seq 已实际确认的可靠序号，不接受猜测或累计ACK。
 */
void bridge_state_net_acked(uint32_t seq)
{
    portENTER_CRITICAL(&s_mux);
    if ((s_event_pending || s_gap_pending) && seq == s_event_seq)
    {
        if (s_gap_pending)
        {
            s_gap_needed = false;
            s_gap_anchor = (bridge_anchor_t){0};
        }
        s_event_pending = false;
        s_gap_pending = false;
        s_pending_anchor = (bridge_anchor_t){0};
    }
    if (s_net_pending >= 0 && seq == s_net_seq)
    {
        const int i = s_net_pending;
        s_result[i] = (bridge_frame_t){0};
        if (i % 2 != 0)
        {
            s_credit[i / 2].active = false;
            s_credit[i / 2].request = 0;
        }
        s_net_pending = -1;
        s_net_seq = 0;
    }
    portEXIT_CRITICAL(&s_mux);
}

/**
 * @brief 启动前生成完整可靠SPI端口绑定。
 * @return 供bridge_spi_configure复制的静态实现入口。
 */
bridge_spi_port_t bridge_state_spi_port(void)
{
    return (bridge_spi_port_t)
    {
        .reserve = spi_reserve,
        .commit = spi_commit,
        .cancel = spi_cancel,
        .telemetry = spi_telemetry,
        .peek_tx = spi_peek,
        .tx_consumed = spi_consumed,
        .session_changed = spi_session
    };
}

/**
 * @brief 复制一致bridge快照与信用位图。
 * @details 计数和三类位图在同一短锁内读取；不暴露负载或可写状态指针。
 * @param[out] snapshot 非空输出，不包含业务负载或密码。
 */
void bridge_state_snapshot(bridge_state_snapshot_t *snapshot)
{
    portENTER_CRITICAL(&s_mux);
    *snapshot = s_stats;
    snapshot->credit_mask = 0;
    snapshot->tombstone_mask = 0;
    for (int i = 0; i < BRIDGE_CLASS_COUNT; ++i)
    {
        if (s_credit[i].active)
        {
            snapshot->credit_mask |= 1u << i;
        }
        if (s_credit[i].tombstone)
        {
            snapshot->tombstone_mask |= 1u << i;
        }
    }
    portEXIT_CRITICAL(&s_mux);
}

/**
 * @brief NET每轮独立推进导出计时，不依赖任何端点流量。
 * @details 仅短锁内处理过期元数据，不向驱动或socket发起调用。
 * @param[in] now_ms 单调毫秒时间，回退不触发取消。
 */
void bridge_state_poll(uint64_t now_ms)
{
    portENTER_CRITICAL(&s_mux);
    expire_export(now_ms);
    portEXIT_CRITICAL(&s_mux);
}

/**
 * @brief NET取走一次缺锚点关闭待办，允许后续重连查询。
 * @details 取走即清待办，不把后续源字段冒充已丢失事件的历史。
 * @return 存在待办返回true并清待办，不清未知历史本身。
 */
bool bridge_state_take_close(void)
{
    portENTER_CRITICAL(&s_mux);
    const bool requested = s_stats.close_requested;
    s_stats.close_requested = false;
    portEXIT_CRITICAL(&s_mux);
    return requested;
}
