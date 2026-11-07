/**
 * @file link_session.c
 * @brief SPI/TCP逻辑握手与会话许可。
 * @details 输入完整HELLO/IDLE和随机候选或显式SPI快照，输出握手帧和状态动作。
 *     对象由调用owner串行维护；网络生产路径仅使用显式快照，不共享可写SPI会话。
 *     本模块无socket、时钟、随机源或DMA访问，不执行ISR工作。
 *
 * @note Phase02 G4：生产接收使用validated入口复用边界解码结果；原始字节接口继续完整验证。
 */
#include "link_session.h"

/**
 * @brief 清空会话状态。
 * @details 零值对应等待HELLO；不访问传输或随机源。
 * @param[out] s 非空时整体清零，为空不操作。
 */
void scb_session_init(scb_session_t *s)
{
    if (s)
    {
        *s = (scb_session_t){0};
    }
}

/**
 * @brief 校验指定角色的零会话HELLO。
 * @details 检查头部握手字段及完整JSON中的role/protocol，不修改会话。
 * @param[in] v 非空且已解码的借用帧视图。
 * @param[in] role 非空ASCII角色名，调用期间借用。
 * @return 角色匹配且protocol精确为1返回true。
 */
static bool hello(const scb_frame_view_t *v, const char *role)
{
    const scb_header_t *h = &v->header;
    if (h->type != SCB_HELLO || h->session_id || h->seq || h->ack_seq || h->request_id)
    {
        return false;
    }
    bridge_json_view_t value;
    uint32_t protocol;
    return bridge_json_object_get_validated(
               (bridge_json_view_t){v->payload, h->payload_len}, "role", &value)
        && bridge_json_string_equal(value, role)
        && bridge_json_object_get_validated(
            (bridge_json_view_t){v->payload, h->payload_len}, "protocol", &value)
        && bridge_json_u32(value, &protocol) && protocol == 1;
}

/**
 * @brief 将固定字面量追加到本地握手负载。
 * @details 逐字节复制，不写尾NUL；内部调用方通过固定片段及128字节数组保证容量。
 * @param[out] b 非空可写缓冲，容量由调用者保证。
 * @param[in] n 起始写偏移，字节。
 * @param[in] literal 非空NUL结尾只读片段。
 * @return 追加后的末尾字节偏移。
 */
static size_t append(uint8_t *b, size_t n, const char *literal)
{
    while (*literal)
    {
        b[n++] = (uint8_t)*literal++;
    }
    return n;
}

/**
 * @brief 编码握手应答或BUSY应答。
 * @details 用固定局部缓冲构造session十进制数及JSON；status非空时附加状态。传输差异由codec决定，不修改会话对象。
 * @param[in] id 明确的SPI会话快照，零允许表示未建立。
 * @param[in] ready 该快照对应的链路就绪值。
 * @param[in] status 可为空；非空只由内部固定短字面量提供。
 * @param[in] transport SPI或TCP。
 * @param[in,out] w 非空独占JSON工作区。
 * @param[out] out 非空独立输出，检查失败时不写图像。
 * @param[in] capacity 输出容量，字节。
 * @param[out] used 非空；由codec清零，成功写帧长。
 * @return codec成功返回true，其他false。
 */
static bool reply(uint32_t id, bool ready, const char *status, scb_transport_t transport,
    bridge_json_workspace_t *w, uint8_t *out, size_t capacity, size_t *used)
{
    uint8_t payload[128];
    uint8_t digits[10];
    size_t n = append(payload, 0, "{\"protocol\":1,\"session_id\":");
    size_t count = 0;
    uint32_t value = id;
    do
    {
        digits[count++] = (uint8_t)('0' + value % 10u);
        value /= 10u;
    } while (value);
    while (count)
    {
        payload[n++] = digits[--count];
    }
    n = append(payload, n, ready ? ",\"link_ready\":true" : ",\"link_ready\":false");
    if (status)
    {
        n = append(payload, n, ",\"status\":\"");
        n = append(payload, n, status);
        n = append(payload, n, "\"");
    }
    n = append(payload, n, "}");
    const scb_header_t h = {.type = SCB_HELLO_ACK, .payload_len = (uint16_t)n, .session_id = id};
    return scb_encode(&h, payload, transport, out, capacity, w, used) == SCB_OK;
}

/**
 * @brief 撤销TCP业务许可并保留关闭请求。
 * @details 仅已打开TCP时置close_tcp；始终撤销tcp_hello，不调用socket。
 * @param[in,out] s 非空且由会话owner独占。
 */
static void invalidate_tcp(scb_session_t *s)
{
    if (s->tcp_open)
    {
        s->close_tcp = true;
    }
    s->tcp_hello = false;
}

/**
 * @brief 推进SPI握手并生成后续事务应答。
 * @details 合法零会话HELLO在WAIT_IDLE重用会话，其他阶段要求候选非零且不同于当前/上次值；编码成功才切状态。WAIT_IDLE仅接受新会话IDLE建立READY，换会话先撤
 *     TCP许可。
 * @param[in,out] s 非空独占会话；为空拒绝。
 * @param[in] b 完整SPI输入，调用期间借用。
 * @param[in] n 输入字节数，codec要求1024。
 * @param[in] candidate 新会话随机候选；生产由硬件随机源提供，零表示尚未获得合格值。
 * @param[in,out] w 非空独占JSON工作区。
 * @param[out] out 可写SPI应答；仅需要REPLY时使用，编码前检查由codec完成。
 * @param[in] capacity 应答可写字节数。
 * @param[out] used 非空；先清零，仅REPLY编码成功时非零。
 * @return REJECT拒绝、NEED_RANDOM需重试候选、REPLY已有应答、ESTABLISHED完成握手、PASS普通帧。
 * @note NEED_RANDOM不更新会话；REPLY用于后续事务，不代表本次MISO已发送。
 */
scb_session_result_t scb_session_spi(scb_session_t *s, const uint8_t *b, size_t n,
    uint32_t candidate, bridge_json_workspace_t *w, uint8_t *out, size_t capacity, size_t *used)
{
    if (!used)
    {
        return SCB_SESSION_REJECT;
    }
    *used = 0;
    if (!s)
    {
        return SCB_SESSION_REJECT;
    }
    scb_frame_view_t v;
    if (scb_decode(b, n, SCB_SPI, w, &v) != SCB_OK)
    {
        return SCB_SESSION_REJECT;
    }
    return scb_session_spi_validated(s, &v, candidate, w, out, capacity, used);
}

/**
 * @brief 使用已验证视图处理握手，不重复解码。
 * @details 与原始字节入口共用状态转换；仍校验角色、版本与握手身份。
 * @param[in,out] s SPI任务独占会话。
 * @param[in] validated 已完整验证的SPI视图，借用期间不变。
 * @param[in] candidate 随机会话候选值。
 * @param[in,out] w 当前任务独占JSON编码工作区。
 * @param[out] out 独立应答缓冲，不可与输入重叠。
 * @param[in] capacity 输出容量，字节。
 * @param[out] used 非空输出；先清零，成功后发布长度。
 * @return 应答、建立、需要随机数或拒绝结果。
 * @pre validated来自对应传输的scb_decode成功结果，原输入有效且不变。
 */
scb_session_result_t scb_session_spi_validated(scb_session_t *s,
    const scb_frame_view_t *validated, uint32_t candidate, bridge_json_workspace_t *w,
    uint8_t *out, size_t capacity, size_t *used)
{
    if (!used)
    {
        return SCB_SESSION_REJECT;
    }
    *used = 0;
    if (!s || !validated)
    {
        return SCB_SESSION_REJECT;
    }
    const scb_frame_view_t v = *validated;
    if (v.header.type == SCB_HELLO)
    {
        if (!hello(&v, "stm32"))
        {
            return SCB_SESSION_REJECT;
        }
        bool fresh = s->phase != SCB_WAIT_IDLE;
        uint32_t id = fresh ? candidate : s->session_id;
        if (fresh && (!id || id == s->session_id || id == s->previous_session))
        {
            return SCB_SESSION_NEED_RANDOM;
        }
        if (!reply(id, false, NULL, SCB_SPI, w, out, capacity, used))
        {
            return SCB_SESSION_REJECT;
        }
        if (fresh)
        {
            s->previous_session = s->session_id ? s->session_id : s->previous_session;
            s->session_id = id;
            invalidate_tcp(s);
        }
        s->phase = SCB_WAIT_IDLE;
        return SCB_SESSION_REPLY;
    }
    if (s->phase == SCB_WAIT_HELLO || v.header.session_id != s->session_id || !v.header.seq
        || v.header.seq == UINT32_MAX)
    {
        return SCB_SESSION_REJECT;
    }
    if (s->phase == SCB_WAIT_IDLE)
    {
        if (v.header.type != SCB_IDLE || v.header.ack_seq || v.header.request_id)
        {
            return SCB_SESSION_REJECT;
        }
        s->phase = SCB_SESSION_READY;
        return SCB_SESSION_ESTABLISHED;
    }
    return v.header.type == SCB_HELLO_ACK ? SCB_SESSION_REJECT : SCB_SESSION_PASS;
}

/**
 * @brief 逻辑失效SPI会话并撤销TCP许可。
 * @details 保存非零当前会话为previous，清session并进入WAIT_HELLO；不释放DMA或关闭socket。
 * @param[in,out] s 独占会话，可为空，空时无操作。
 */
void scb_session_spi_lost(scb_session_t *s)
{
    if (!s)
    {
        return;
    }
    if (s->session_id)
    {
        s->previous_session = s->session_id;
    }
    s->session_id = 0;
    s->phase = SCB_WAIT_HELLO;
    invalidate_tcp(s);
}

/**
 * @brief 重编码等待IDLE期间的握手应答。
 * @details 只读会话，不换session、不推进阶段；不在WAIT_IDLE时不生成图像。
 * @param[in] s 可为空的只读会话。
 * @param[in,out] w 非空独占JSON工作区，仅编码时使用。
 * @param[out] out 可写独立输出，仅实际编码时访问。
 * @param[in] capacity 输出容量，字节。
 * @param[out] used 非空；进入清零，成功写帧长。
 * @return 有待应答且编码成功返回true，否则false。
 */
bool scb_session_spi_pending_reply(
    const scb_session_t *s, bridge_json_workspace_t *w, uint8_t *out, size_t capacity, size_t *used)
{
    if (!used)
    {
        return false;
    }
    *used = 0;
    return s && s->phase == SCB_WAIT_IDLE
        && reply(s->session_id, false, NULL, SCB_SPI, w, out, capacity, used);
}

/**
 * @brief 登记一个新TCP逻辑连接。
 * @details 仅在没有已有连接时置open并清握手/关闭请求/绑定会话；不重置SPI。
 * @param[in,out] s 非空独占会话，空或已open拒绝。
 * @return 成功接管返回true，否则false。
 */
bool scb_session_tcp_open(scb_session_t *s)
{
    if (!s || s->tcp_open)
    {
        return false;
    }
    s->tcp_open = true;
    s->tcp_hello = false;
    s->close_tcp = false;
    s->tcp_session = 0;
    return true;
}

/**
 * @brief 清理TCP逻辑连接状态。
 * @details 清open、hello、close请求与TCP会话，不改变SPI会话。
 * @param[in,out] s 独占状态，可为空，空时无操作。
 */
void scb_session_tcp_close(scb_session_t *s)
{
    if (!s)
    {
        return;
    }
    s->tcp_open = false;
    s->tcp_hello = false;
    s->close_tcp = false;
    s->tcp_session = 0;
}

/**
 * @brief 为逻辑TCP连接处理一次HELLO。
 * @details 要求已open且未握手/未请求关闭，完整验证pc角色后用SPI状态编码；成功后才发布tcp_session和tcp_hello。
 * @param[in,out] s 非空独占会话状态。
 * @param[in] b 调用期间借用的完整TCP帧。
 * @param[in] n 输入字节数。
 * @param[in,out] w 非空独占JSON工作区。
 * @param[out] out 非空独立输出，失败不发布应答长度。
 * @param[in] capacity 可写输出容量，字节。
 * @param[out] used 非空；进入清零，成功写帧长。
 * @return 成功SESSION_REPLY，否则SESSION_REJECT。
 */
scb_session_result_t scb_session_tcp_hello(scb_session_t *s, const uint8_t *b, size_t n,
    bridge_json_workspace_t *w, uint8_t *out, size_t capacity, size_t *used)
{
    if (!used)
    {
        return SCB_SESSION_REJECT;
    }
    *used = 0;
    if (!s || !s->tcp_open || s->tcp_hello || s->close_tcp)
    {
        return SCB_SESSION_REJECT;
    }
    scb_frame_view_t v;
    if (scb_decode(b, n, SCB_TCP, w, &v) != SCB_OK || !hello(&v, "pc"))
    {
        return SCB_SESSION_REJECT;
    }
    if (!reply(s->session_id, s->phase == SCB_SESSION_READY, NULL, SCB_TCP, w, out, capacity, used))
    {
        return SCB_SESSION_REJECT;
    }
    s->tcp_session = s->session_id;
    s->tcp_hello = true;
    return SCB_SESSION_REPLY;
}

/**
 * @brief 查询TCP帧是否仍有当前会话业务许可。
 * @details 同时检查连接、握手、关闭请求、SPI READY和双侧绑定session，不操作socket。
 * @param[in] s 只读会话，可为空，空返回false。
 * @param[in] id 帧携带的非零session。
 * @return 全部许可条件成立返回true，否则false。
 */
bool scb_session_tcp_permitted(const scb_session_t *s, uint32_t id)
{
    return s && s->tcp_open && s->tcp_hello && !s->close_tcp && s->phase == SCB_SESSION_READY && id
        && id == s->session_id && id == s->tcp_session;
}

/**
 * @brief 根据显式SPI快照编码TCP握手应答。
 * @details 网络owner调用；验证PC HELLO后编码快照，不持有或修改SPI会话对象。
 * @param[in] id SPI会话快照，零表示无会话。
 * @param[in] ready 快照对应的link_ready。
 * @param[in] b 完整TCP HELLO，调用期间只读借用。
 * @param[in] n 输入字节数。
 * @param[in,out] w 非空独占JSON工作区。
 * @param[out] out 非空独立可写输出。
 * @param[in] capacity 输出容量，字节。
 * @param[out] used 非空；先清零，成功写帧长。
 * @return 编码成功SESSION_REPLY，否则SESSION_REJECT。
 */
scb_session_result_t scb_session_tcp_hello_explicit(uint32_t id, bool ready, const uint8_t *b,
    size_t n, bridge_json_workspace_t *w, uint8_t *out, size_t capacity, size_t *used)
{
    if (!used)
    {
        return SCB_SESSION_REJECT;
    }
    *used = 0;
    scb_frame_view_t v;
    if (scb_decode(b, n, SCB_TCP, w, &v) != SCB_OK)
    {
        return SCB_SESSION_REJECT;
    }
    return scb_session_tcp_hello_validated(id, ready, &v, w, out, capacity, used);
}

/**
 * @brief 使用已验证视图处理握手，不重复解码。
 * @details 与原始字节入口共用状态转换；仍校验角色、版本与握手身份。
 * @param[in] id 当前SPI会话快照。
 * @param[in] ready 当前SPI就绪事实。
 * @param[in] validated 已完整验证的TCP视图，借用期间不变。
 * @param[in,out] w 当前任务独占JSON编码工作区。
 * @param[out] out 独立应答缓冲，不可与输入重叠。
 * @param[in] capacity 输出容量，字节。
 * @param[out] used 非空输出；先清零，成功后发布长度。
 * @return 应答、建立、需要随机数或拒绝结果。
 * @pre validated来自对应传输的scb_decode成功结果，原输入有效且不变。
 */
scb_session_result_t scb_session_tcp_hello_validated(uint32_t id, bool ready,
    const scb_frame_view_t *validated, bridge_json_workspace_t *w,
    uint8_t *out, size_t capacity, size_t *used)
{
    if (!used)
    {
        return SCB_SESSION_REJECT;
    }
    *used = 0;
    if (!validated)
    {
        return SCB_SESSION_REJECT;
    }
    const scb_frame_view_t v = *validated;
    if (!hello(&v, "pc"))
    {
        return SCB_SESSION_REJECT;
    }
    if (!reply(id, ready, NULL, SCB_TCP, w, out, capacity, used))
    {
        return SCB_SESSION_REJECT;
    }
    return SCB_SESSION_REPLY;
}

/**
 * @brief 根据SPI快照编码独立BUSY拒绝帧。
 * @details 按G-01构造HELLO_ACK/BUSY，seq/ack/request/flags保持零；不影响首控制连接，不输出SPI填充。
 * @param[in] id SPI会话快照，允许零。
 * @param[in] ready 快照对应的link_ready。
 * @param[in,out] w 非空独占JSON工作区。
 * @param[out] out 非空独立BUSY输出缓冲。
 * @param[in] capacity 可写容量，字节。
 * @param[out] used 非空；先清零，成功写帧长。
 * @return 编码成功SESSION_REPLY，否则SESSION_REJECT。
 */
scb_session_result_t scb_session_tcp_busy(uint32_t id, bool ready, bridge_json_workspace_t *w,
    uint8_t *out, size_t capacity, size_t *used)
{
    if (!used)
    {
        return SCB_SESSION_REJECT;
    }
    *used = 0;
    if (!reply(id, ready, "BUSY", SCB_TCP, w, out, capacity, used))
    {
        return SCB_SESSION_REJECT;
    }
    return SCB_SESSION_REPLY;
}
