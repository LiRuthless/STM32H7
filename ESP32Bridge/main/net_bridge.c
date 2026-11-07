/**
 * @file net_bridge.c
 * @brief SoftAP与单控制TCP/订阅UDP服务。
 * @details 启动配置AP并创建Core0网络任务；任务装配帧、推进可靠收发/订阅并输出UDP。
 *     socket、游标、连接许可及TCP窗口由NET任务持有；快照和UDP交接分别用短临界区。
 *     其他任务仅offer独立遥测副本，不借用驱动DMA，ISR不参与网络解析。
 */
#include "net_bridge.h"
#include "bridge_state.h"
#include "bridge_stats.h"
#include "spi_bridge.h"
#include "link_session.h"
#include "reliable_link.h"
#include <stdint.h>
#include "lwip/sockets.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "esp_mac.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "sdkconfig.h"

/* 网络图像及socket归网络任务；不访问DMA。跨任务遥测入口通过s_udp_mux交接图像。 */
/* 网络任务静态栈，容量按字节定义。 */
static StackType_t s_stack[BRIDGE_NET_STACK_BYTES / sizeof(StackType_t)];
/* 静态网络任务控制块，交RTOS使用。 */
static StaticTask_t s_task_storage;
/* 网络初始化通知信号量存储。 */
static StaticSemaphore_t s_boot_storage;
/* NET任务通知、启动者等待的初始化句柄。 */
static SemaphoreHandle_t s_boot;
/* 启动时创建的NET任务句柄。 */
static TaskHandle_t s_task;
/* NET任务写、初始化通知后启动者读的结果。 */
static esp_err_t s_init_error = ESP_ERR_INVALID_STATE;
/* 启动上下文单次标志，启动后禁止更换端口表。 */
static bool s_started;
/* 启动时复制、NET任务调用的UDP出口表。 */
static bridge_net_port_t s_port;
/* 启动前绑定生产消费者，运行期只读；未绑定夹具保留原背压边界。 */
static bool s_bridge_bound;
/* NET任务发布的诊断副本，快照锁保护。 */
static bridge_net_snapshot_t s_health = {.init_error = ESP_ERR_INVALID_STATE};
/* 只保护已发布快照的复制。 */
static portMUX_TYPE s_snapshot_mux = portMUX_INITIALIZER_UNLOCKED;
/* 保护UDP图像的pending/in_use/overflow交接，不围住socket调用。 */
static portMUX_TYPE s_udp_mux = portMUX_INITIALIZER_UNLOCKED;
_Static_assert(sizeof(s_stack) == BRIDGE_NET_STACK_BYTES, "ESP-IDF static stack size in bytes");

/* NET任务持有的监听fd，-1为无效。 */
static int s_listen = -1;
/* NET任务持有的首控制连接fd，-1为空槽。 */
static int s_conn = -1;
/* NET任务持有的独立BUSY连接fd，-1为空槽。 */
static int s_busy = -1;
/* NET任务持有的非阻塞UDP fd，-1表示出口不可用。 */
static int s_udp = -1;
/* 跨任务图像仅在非pending且非in_use时可写；溢出由网络任务折叠到丢弃计数。 */
/* 跨任务复制的UDP图像，pending或in_use时禁止写者改写。 */
static uint8_t s_udp_image[BRIDGE_NET_UDP_BYTES];
/* 交接图像有效字节数，交接锁保护。 */
static size_t s_udp_length;
/* 图像接管时记录的连接代际，消费时与当前值比较。 */
static uint32_t s_udp_generation;
/* 交接锁内写；true表示已提交等待NET消费。 */
static bool s_udp_pending;
/* 交接锁内写；NET锁外发送期间保留图像所有权。 */
static bool s_udp_in_use;
/* 写者忙时置位，NET下一次消费折叠一次丢弃计数。 */
static bool s_udp_overflow;
/* 以下连接、流游标及可靠状态只由网络任务推进。 */
/* NET任务独占的TCP拼接图像。 */
static uint8_t s_rx[BRIDGE_NET_RX_BYTES];
/* 拼接图像有效字节数，NET任务维护。 */
static size_t s_rx_length;
/* NET任务独占的控制发送图像，与SPI DMA分离。 */
static uint8_t s_tx[BRIDGE_NET_TX_BYTES];
/* 控制图像待发送总字节数，NET任务维护。 */
static size_t s_tx_length;
/* 控制图像已正数send的字节数，NET任务维护。 */
static size_t s_tx_cursor;
/* 控制图像对应的可靠序号，NET任务维护。 */
static uint32_t s_tx_seq;
/* 当前控制图像最后正进展的单调毫秒时间。 */
static uint64_t s_tx_progress;
/* 当前控制图像是否需在全发完后报告可靠sent。 */
static bool s_tx_reliable;
/* NET任务的IDLE确认待办，不覆盖在途TX。 */
static bool s_want_idle;
/* NET任务独占的第二连接应答图像。 */
static uint8_t s_busy_image[BRIDGE_NET_BUSY_BYTES];
/* BUSY应答字节数。 */
static size_t s_busy_length;
/* BUSY已发送字节数。 */
static size_t s_busy_cursor;
/* BUSY绝对单调毫秒截止，NET任务检查。 */
static uint64_t s_busy_deadline;
/* NET任务独占TCP可靠实例，不共享SPI实例。 */
static scb_link_t s_link;
/* NET递增、跨任务offer读取；所有跨任务读写均由s_udp_mux保护。 */
static uint32_t s_generation;
/* NET独占的连接代际高位；公开身份使用64位并在耗尽前停止accept。 */
static uint32_t s_generation_high;
/* NET独占的生产UDP发送序号，当前连接从1开始，耗尽撤销订阅。 */
static uint32_t s_bridge_udp_seq = 1;
/* NET独占的订阅身份，合法重新订阅清旧遥测队列。 */
static uint32_t s_subscription_generation;
/* accept记录的最近对端IPv4，网络字节序，仅NET写。 */
static uint32_t s_peer_ip;
/* 当前订阅端口，主机序，仅NET任务维护。 */
static uint16_t s_sub_port;
/* 当前控制连接订阅许可，仅NET任务维护。 */
static bool s_subscribed;
/* 当前控制连接已完成HELLO，仅NET任务维护。 */
static bool s_hello_done;
/* NET最近读到的SPI会话快照，不是可写SPI对象。 */
static uint32_t s_spi_session;
/* NET是否已成功取得过SPI快照。 */
static bool s_spi_known;
/* NET最近快照的SPI READY值。 */
static bool s_spi_ready;

/**
 * @brief 读取网络任务的单调毫秒时间。
 * @details 将SDK微秒值除以1000，用于重试、BUSY及停滞截止。
 * @return 当前毫秒值。
 */
static uint64_t now_ms(void)
{
    return (uint64_t)esp_timer_get_time() / 1000u;
}

/**
 * @brief NET发布一致身份，socket及连接字段仍由NET独占。
 * @details bridge锁内提交副本；不持UDP锁进入bridge锁。
 */
static void publish_identity(void)
{
    if (s_bridge_bound)
    {
        const bridge_identity_t identity =
        {
            .generation = ((uint64_t)s_generation_high << 32) | s_generation,
            .session = s_link.session_id,
            .open = s_conn >= 0 && s_hello_done,
            .subscribed = s_subscribed,
            .subscription = s_subscription_generation
        };
        bridge_state_identity(identity);
    }
}

/**
 * @brief 启动前绑定真实bridge消费者。
 * @return 已启动返回INVALID_STATE，否则ESP_OK。
 */
esp_err_t bridge_net_bind_bridge(void)
{
    if (s_started)
    {
        return ESP_ERR_INVALID_STATE;
    }
    s_bridge_bound = true;
    return ESP_OK;
}

/**
 * @brief 读取流头中的小端16位字段。
 * @details 逐字节组合，不要求输入整数对齐。
 * @param[in] b 非空且至少两字节可读。
 * @return 解码后的字段值。
 */
static uint16_t load16(const uint8_t *b)
{
    return (uint16_t)((uint16_t)b[0] | ((uint16_t)b[1] << 8));
}

/**
 * @brief 发布网络任务状态副本。
 * @details 在任务内先采集连接/订阅、对端IP及栈余量，再短临界区复制累计诊断；不暴露密码或图像指针。
 * @param[in,out] health 非空网络任务局部状态，补充字段后发布。
 */
static void publish(bridge_net_snapshot_t *health)
{
    health->listening = s_listen >= 0;
    health->control_open = s_conn >= 0;
    health->subscribed = s_subscribed;
    health->udp_port = s_subscribed ? s_sub_port : 0;
    health->peer_ipv4 = s_peer_ip;
    health->stack_free_min = uxTaskGetStackHighWaterMark(NULL);
    portENTER_CRITICAL(&s_snapshot_mux);
    s_health = *health;
    portEXIT_CRITICAL(&s_snapshot_mux);
}

/**
 * @brief 读取已发布的网络状态副本。
 * @details 在快照锁内复制，不接触socket或修改连接状态。
 * @param[out] snapshot 非空；成功写入副本，空指针直接拒绝。
 * @return 成功ESP_OK，否则ESP_ERR_INVALID_ARG。
 * @note 供任务读取；发布值不保证读取瞬间仍对应活连接。
 */
esp_err_t bridge_net_snapshot(bridge_net_snapshot_t *snapshot)
{
    if (!snapshot)
    {
        return ESP_ERR_INVALID_ARG;
    }
    portENTER_CRITICAL(&s_snapshot_mux);
    *snapshot = s_health;
    portEXIT_CRITICAL(&s_snapshot_mux);
    return ESP_OK;
}

/**
 * @brief 在服务启动前绑定UDP出口。
 * @details NULL清表；非空必须带telemetry。复制表但不复制context指向对象，启动后禁止替换。
 * @param[in] port 可为空；表只借用到返回，context及回调依赖须存续到服务结束。
 * @return 成功ESP_OK，已启动或缺出口返回对应ESP错误。
 */
esp_err_t bridge_net_configure(const bridge_net_port_t *port)
{
    if (s_started)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (port && !port->telemetry)
    {
        return ESP_ERR_INVALID_ARG;
    }
    s_port = port ? *port : (bridge_net_port_t){0};
    return ESP_OK;
}

/**
 * @brief 复制一个完整遥测帧到跨任务UDP暂存图像。
 * @details 先按UDP格式完整校验，再在s_udp_mux内检查pending/in_use；忙时置溢出标志，空闲时复制并记录当前generation。此入口不调用socket，也不证明已
 *     订阅或已发送。
 * @param[in] frame 非空且length字节可读，借用到返回；成功后已有独立副本。
 * @param[in] length 帧字节数，32至1016且须为合法TELEMETRY。
 * @return 接管副本ESP_OK，坏帧ESP_ERR_INVALID_ARG，图像忙ESP_ERR_INVALID_STATE。
 * @note 可从任务调用；只保护图像交接，不承担Phase05生产队列。
 */
esp_err_t bridge_net_telemetry_offer(const uint8_t *frame, size_t length)
{
    if (s_bridge_bound)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (!frame || length < SCB_HEADER_BYTES || length > BRIDGE_NET_UDP_BYTES)
    {
        return ESP_ERR_INVALID_ARG;
    }
    scb_frame_view_t view;
    if (scb_decode(frame, length, SCB_UDP, NULL, &view) != SCB_OK)
    {
        return ESP_ERR_INVALID_ARG;
    }
    portENTER_CRITICAL(&s_udp_mux);
    if (s_udp_pending || s_udp_in_use)
    {
        s_udp_overflow = true;
        portEXIT_CRITICAL(&s_udp_mux);
        return ESP_ERR_INVALID_STATE;
    }
    for (size_t i = 0; i < length; ++i)
    {
        s_udp_image[i] = frame[i];
    }
    s_udp_length = length;
    s_udp_generation = s_generation;
    s_udp_pending = true;
    portEXIT_CRITICAL(&s_udp_mux);
    return ESP_OK;
}

/* 关闭控制连接同时撤订阅、待发图像及UDP pending；不重置SPI、不重放设置。 */
/**
 * @brief 关闭当前控制socket并清理其逻辑资源。
 * @details 清RX/TX游标、握手、订阅、IDLE待办和TCP窗口，再锁内撤销UDP pending；不重放旧设置、不重置SPI。
 * @note 只由网络任务调用，关闭归因由各调用点计数。
 */
static void close_control(void)
{
    if (s_conn >= 0)
    {
        close(s_conn);
    }
    s_conn = -1;
    s_rx_length = 0;
    s_tx_length = 0;
    s_tx_cursor = 0;
    s_tx_reliable = false;
    s_want_idle = false;
    s_hello_done = false;
    s_subscribed = false;
    scb_link_reset(&s_link, SCB_TCP, 0);
    portENTER_CRITICAL(&s_udp_mux);
    s_udp_pending = false;
    portEXIT_CRITICAL(&s_udp_mux);
    publish_identity();
}

/**
 * @brief 根据SPI快照更新网络侧许可状态。
 * @details 读取失败保留旧值；已知session变化且有控制连接时先关旧连接，再更新本地快照及计数；不操作SPI会话对象。
 * @param[in,out] health 非空网络任务局部累计状态。
 */
static void session_check(bridge_net_snapshot_t *health)
{
    bridge_spi_snapshot_t spi;
    if (bridge_spi_snapshot(&spi) != ESP_OK)
    {
        return;
    }
    if (s_bridge_bound)
    {
        bridge_state_snapshot_t bridge;
        bridge_state_snapshot(&bridge);
        spi.session_id = bridge.session;
        spi.link_ready = bridge.ready;
    }
    /* G-02：SPI会话变化即撤旧业务许可并关闭TCP，电脑需重连HELLO获知新状态。 */
    if (s_spi_known && spi.session_id != s_spi_session && s_conn >= 0)
    {
        close_control();
        ++health->session_closes;
    }
    s_spi_session = spi.session_id;
    s_spi_ready = spi.link_ready;
    s_spi_known = true;
}

/**
 * @brief 推进TCP可靠重发或逻辑故障关闭。
 * @details 只有连接已握手且TX图像空闲才poll。零SPI会话的dead状态保持状态连接，不记故障；非零dead关闭TCP。RESEND只复制原缓存并初始化独立游标。
 * @param[in,out] health 非空网络任务局部计数。
 * @param[in] now 当前单调毫秒时间。
 */
static void link_service(bridge_net_snapshot_t *health, uint64_t now)
{
    if (s_conn < 0 || !s_hello_done || s_tx_length != 0)
    {
        return;
    }
    const uint8_t *bytes;
    size_t length;
    const scb_link_result_t result = scb_link_poll(&s_link, now, &bytes, &length);
    if (result == SCB_LINK_REBUILD)
    {
        /*
         * 无SPI会话的dead实例是仅状态连接，不关闭、不计真实故障；
         * 只有非零会话的可靠故障关闭本TCP，不影响SPI。
         */
        if (s_link.session_id)
        {
            ++health->link_failures;
            close_control();
        }
        return;
    }
    if (result == SCB_LINK_RESEND && length <= sizeof(s_tx))
    {
        for (size_t i = 0; i < length; ++i)
        {
            s_tx[i] = bytes[i];
        }
        s_tx_length = length;
        s_tx_cursor = 0;
        s_tx_seq = s_link.tx_seq;
        s_tx_reliable = true;
        s_tx_progress = now;
    }
}

/**
 * @brief 在空闲TX中编码待发确认IDLE。
 * @details 要求连接有效、有确认待办且可靠层非dead；编码成功才清待办并发布TX长度，不覆盖在途图像。
 * @param[in,out] workspace 非空网络任务独占JSON工作区。
 * @param[in] now 当前单调毫秒时间，用于发送进度起点。
 */
static void idle_service(bridge_json_workspace_t *workspace, uint64_t now)
{
    if (s_conn < 0 || !s_want_idle || s_tx_length != 0 || s_link.dead)
    {
        return;
    }
    const scb_header_t idle = {.type = SCB_IDLE};
    size_t used;
    if (scb_link_prepare(&s_link, &idle, NULL, s_tx, sizeof(s_tx), workspace, &used) != SCB_LINK_OK)
    {
        return;
    }
    s_want_idle = false;
    s_tx_length = used;
    s_tx_cursor = 0;
    s_tx_reliable = false;
    s_tx_progress = now;
}

/**
 * @brief 处理本地订阅并准备可靠RESULT。
 * @details 检查非零request_id和args.udp_port精确整数1至65535；合法时先更新订阅，目的IP保持accept对端，随后编码RESULT。编码失败计错并关闭连接，清
 *     理刚更新的订阅。
 * @param[in,out] health 非空网络任务局部计数。
 * @param[in,out] workspace 非空独占JSON工作区。
 * @param[in] view 非空已接管的完整COMMAND借用视图，仅调用期间有效。
 * @param[in] now 当前单调毫秒时间。
 * @note 调用前须已预留TX图像与可靠窗口；不转发STM32。
 */
static void handle_subscribe(bridge_net_snapshot_t *health, bridge_json_workspace_t *workspace,
    const scb_frame_view_t *view, uint64_t now)
{
    uint32_t port_value = 0;
    bool legal = view->header.request_id != 0;
    if (legal)
    {
        bridge_json_view_t args;
        bridge_json_view_t port;
        legal = bridge_json_object_get(
                    view->payload, view->header.payload_len, "args", workspace, &args)
            && bridge_json_object_get(args.bytes, args.length, "udp_port", workspace, &port)
            && bridge_json_u32(port, &port_value) && port_value >= 1u && port_value <= 65535u;
    }
    uint8_t payload[48];
    size_t n = 0;
    static const char prefix[] = "{\"op\":\"SUBSCRIBE\",\"status\":\"";
    static const char okay[] = "OK";
    static const char invalid[] = "INVALID_ARG";
    const char *status = legal ? okay : invalid;
    while (prefix[n])
    {
        payload[n] = (uint8_t)prefix[n];
        ++n;
    }
    for (size_t i = 0; status[i]; ++i)
    {
        payload[n++] = (uint8_t)status[i];
    }
    payload[n++] = (uint8_t)'"';
    payload[n++] = (uint8_t)'}';
    if (legal)
    {
        /* 目的IP仅取accept记录的TCP对端，不采用帧内第三方地址。 */
        s_subscribed = true;
        ++s_subscription_generation;
        s_sub_port = (uint16_t)port_value;
        ++health->subscriptions;
    }
    const scb_header_t header =
    {
        .type = SCB_RESULT,
        .flags = SCB_RELIABLE,
        .payload_len = (uint16_t)n,
        .request_id = view->header.request_id
    };
    size_t used;
    if (scb_link_prepare(&s_link, &header, payload, s_tx, sizeof(s_tx), workspace, &used)
        != SCB_LINK_OK)
    {
        ++health->invalid_closes;
        close_control();
        return;
    }
    s_tx_length = used;
    s_tx_cursor = 0;
    s_tx_seq = s_link.tx_seq;
    s_tx_reliable = true;
    s_tx_progress = now;
}

/**
 * @brief 可靠层确认精确ACK后通知bridge，不从入帧ack值猜测交付。
 * @param[in] bytes 完整TCP帧。
 * @param[in] length 完整图像字节数。
 * @param[in] reserved 真实下游预留事实。
 * @param[in,out] workspace NET独占JSON工作区。
 * @param[out] view 可靠层交付视图，本次调用借用。
 * @return 返回实际可靠层判定。
 */
static scb_link_result_t net_receive(const uint8_t *bytes, size_t length, bool reserved,
    bridge_json_workspace_t *workspace, scb_frame_view_t *view)
{
    const bool pending = s_link.pending;
    const uint32_t seq = s_link.tx_seq;
    const scb_link_result_t result
        = scb_link_receive(&s_link, bytes, length, reserved, workspace, view);
    if (s_bridge_bound && pending && !s_link.pending)
    {
        bridge_state_net_acked(seq);
    }
    return result;
}

/**
 * @brief NET从真实bridge借出可靠上行并独立重封装。
 * @details 编码期间不持bridge锁，consumed结束借用；失败保留源槽供后续处理。
 * @param[in,out] workspace NET独占工作区。
 * @param[in] now 当前单调毫秒时间。
 */
static void bridge_tx_service(bridge_json_workspace_t *workspace, uint64_t now)
{
    if (s_conn < 0 || !s_hello_done || s_tx_length != 0 || s_link.pending || s_link.dead)
    {
        return;
    }
    scb_frame_view_t view;
    if (!bridge_state_net_peek(&view))
    {
        return;
    }
    size_t used;
    const bool encoded = scb_link_prepare(&s_link, &view.header, view.payload,
        s_tx, sizeof(s_tx), workspace, &used) == SCB_LINK_OK;
    bridge_state_net_consumed(encoded, s_link.tx_seq);
    if (encoded)
    {
        s_tx_length = used;
        s_tx_cursor = 0;
        s_tx_seq = s_link.tx_seq;
        s_tx_reliable = true;
        s_tx_progress = now;
    }
}

/**
 * @brief 分派完整TCP帧，预留后接管并在提交复制后安排ACK。
 * @details HELLO绑定当前SPI状态；SUBSCRIBE本地处理，其他COMMAND进入生产信用。
 *     背压帧不ACK，重复帧不提交；IDLE可独立推进反向精确ACK。
 * @param[in,out] health NET任务局部诊断。
 * @param[in,out] workspace NET独占JSON工作区。
 * @param[in] frame_length s_rx中完整图像字节数。
 * @param[out] consumed 可安全移出RX时置true，否则保留图像。
 * @param[in] now 当前单调毫秒时间。
 */
static void dispatch(bridge_net_snapshot_t *health, bridge_json_workspace_t *workspace,
    size_t frame_length, bool *consumed, uint64_t now)
{
    *consumed = false;
    scb_frame_view_t view;
    const scb_error_t error = scb_decode(s_rx, frame_length, SCB_TCP, workspace, &view);
    if (error != SCB_OK)
    {
        if (error == SCB_CRC)
        {
            ++health->crc_errors;
        }
        else
        {
            ++health->invalid_closes;
        }
        close_control();
        return;
    }
    if (!s_hello_done)
    {
        /* HELLO前的其他类型都关闭连接。 */
        if (view.header.type != SCB_HELLO)
        {
            ++health->invalid_closes;
            close_control();
            return;
        }
        size_t used;
        if (scb_session_tcp_hello_explicit(s_spi_known ? s_spi_session : 0,
                s_spi_known && s_spi_ready, s_rx, frame_length, workspace, s_tx, sizeof(s_tx),
                &used)
            != SCB_SESSION_REPLY)
        {
            ++health->invalid_closes;
            close_control();
            return;
        }
        ++health->received;
        s_tx_length = used;
        s_tx_cursor = 0;
        s_tx_reliable = false;
        s_tx_progress = now;
        s_hello_done = true;
        s_bridge_udp_seq = 1;
        scb_link_reset(&s_link, SCB_TCP, s_spi_known ? s_spi_session : 0);
        publish_identity();
        *consumed = true;
        return;
    }
    if (view.header.type == SCB_COMMAND)
    {
        bool ready = s_spi_ready && s_spi_session != 0
            && s_link.session_id == s_spi_session;
        if (s_bridge_bound)
        {
            bridge_state_snapshot_t bridge;
            bridge_state_snapshot(&bridge);
            ready = ready && bridge.ready && bridge.session == s_link.session_id;
        }
        if (!ready)
        {
            ++health->invalid_closes;
            close_control();
            return;
        }
        bridge_json_view_t op;
        const bool subscribe
            = bridge_json_object_get(view.payload, view.header.payload_len, "op", workspace, &op)
            && bridge_json_string_equal(op, "SUBSCRIBE");
        /*
         * SUBSCRIBE新帧须同时有TX图像和可靠窗口才接管，否则保留RX等待。
         * 重复帧可由IDLE确认；其他COMMAND必须从bridge取得真实额度。
         * 携带反向ACK由可靠层独立处理，不凭入帧ack值猜测最终交付。
         */
        const bool bridge_reserved = !subscribe && s_bridge_bound
            && bridge_state_net_reserve(&view, workspace);
        const bool reserve = bridge_reserved
            || (subscribe && s_tx_length == 0 && !s_link.pending);
        const scb_link_result_t result
            = net_receive(s_rx, frame_length, reserve, workspace, &view);
        if (bridge_reserved)
        {
            if (result == SCB_LINK_DELIVER)
            {
                bridge_state_net_commit(&view);
            }
            else
            {
                bridge_state_net_cancel();
            }
        }
        if (result == SCB_LINK_BACKPRESSURE && subscribe)
        {
            return; /* 延后到TX图像和可靠窗口可用。 */
        }
        ++health->received;
        if (result == SCB_LINK_DELIVER)
        {
            if (subscribe)
            {
                handle_subscribe(health, workspace, &view, now);
                publish_identity();
            }
            else
            {
                s_want_idle = true;
            }
            if (s_conn < 0)
            {
                return;
            }
        }
        else if (result == SCB_LINK_DUPLICATE)
        {
            s_want_idle = true;
        }
        else if (result == SCB_LINK_BACKPRESSURE)
        {
            ++health->backpressure;
        }
        else if (result == SCB_LINK_STALE)
        {
            /* 更早的可靠身份不再重复确认。 */
        }
        else
        {
            ++health->invalid_closes;
            close_control();
            return;
        }
        *consumed = true;
        return;
    }
    if (view.header.type == SCB_IDLE)
    {
        scb_frame_view_t ignored;
        const scb_link_result_t result
            = net_receive(s_rx, frame_length, false, workspace, &ignored);
        if (result == SCB_LINK_INVALID || result == SCB_LINK_REBUILD)
        {
            ++health->invalid_closes;
            close_control();
            return;
        }
        ++health->received;
        *consumed = true;
        return;
    }
    ++health->invalid_closes;
    close_control();
}

/**
 * @brief 向固定RX图像追加一段非阻塞输入。
 * @details 满缓冲直接返回，避免零长度recv与EOF混淆；正数累积长度，零关闭连接，EAGAIN保留进度，其他负错误关闭。
 * @note 只由网络任务在s_conn有效时调用。
 */
static void rx_recv(void)
{
    /* 完整但延后的帧仍占缓冲；满时不做零长度recv，避免把返回零误判为EOF。 */
    if (s_rx_length == sizeof(s_rx))
    {
        return;
    }
    const ssize_t n = recv(s_conn, s_rx + s_rx_length, sizeof(s_rx) - s_rx_length, 0);
    if (n == 0)
    {
        close_control();
        return;
    }
    if (n < 0)
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            close_control();
        }
        return;
    }
    s_rx_length += (size_t)n;
}

/* 每轮尝试消费完整帧，使被TX占用延后的帧在释放后继续分派。 */
/**
 * @brief 按完整帧边界消费拼接RX图像。
 * @details 先检查32字节头的magic/版本/类型/长度，再等完整负载；分派允许消费才搬移剩余字节。延后帧保留到下一循环，坏头关闭，无部分业务发布。
 * @param[in,out] health 非空任务局部诊断。
 * @param[in,out] workspace 非空网络任务独占工作区。
 * @param[in] now 当前单调毫秒时间。
 */
static void rx_consume(
    bridge_net_snapshot_t *health, bridge_json_workspace_t *workspace, uint64_t now)
{
    for (;;)
    {
        if (s_conn < 0 || s_rx_length < SCB_HEADER_BYTES)
        {
            return;
        }
        /* 等负载前先验证头，防止声明超长输入占用无界空间。 */
        if (s_rx[0] != 'S' || s_rx[1] != 'C' || s_rx[2] != 'B' || s_rx[3] != '1' || s_rx[4] != 1u
            || (s_rx[5] != SCB_HELLO && s_rx[5] != SCB_HELLO_ACK && s_rx[5] != SCB_IDLE
                && s_rx[5] != SCB_COMMAND && s_rx[5] != SCB_RESULT && s_rx[5] != SCB_EVENT
                && s_rx[5] != SCB_TELEMETRY && s_rx[5] != SCB_LOG_DATA)
            || load16(s_rx + 8) > SCB_PAYLOAD_BYTES)
        {
            ++health->invalid_closes;
            close_control();
            return;
        }
        const size_t frame_length = SCB_HEADER_BYTES + (size_t)load16(s_rx + 8);
        if (s_rx_length < frame_length)
        {
            return;
        }
        bool consumed;
        dispatch(health, workspace, frame_length, &consumed, now);
        if (s_conn < 0 || !consumed)
        {
            return;
        }
        for (size_t i = frame_length; i < s_rx_length; ++i)
        {
            s_rx[i - frame_length] = s_rx[i];
        }
        s_rx_length -= frame_length;
    }
}

/**
 * @brief 推进控制连接的部分发送游标。
 * @details 可写时发送剩余图像，只有全部字节完成才通知可靠sent；EAGAIN或send零保留进度。超过停滞阈值才关连接，不阻塞SPI。
 * @param[in,out] health 非空网络任务局部计数。
 * @param[in] writable 本次poll报告是否可写。
 * @param[in] now 当前单调毫秒时间；有正进展时刷新s_tx_progress。
 */
static void tx_service(bridge_net_snapshot_t *health, bool writable, uint64_t now)
{
    if (s_conn < 0 || s_tx_length == s_tx_cursor)
    {
        return;
    }
    if (writable)
    {
        const size_t remaining = s_tx_length - s_tx_cursor;
        const ssize_t n = send(s_conn, s_tx + s_tx_cursor, remaining, 0);
        if (n > 0)
        {
            s_tx_cursor += (size_t)n;
            health->sent_bytes += (uint32_t)n;
            s_tx_progress = now;
            if ((size_t)n < remaining)
            {
                ++health->short_writes;
            }
            if (s_tx_cursor == s_tx_length)
            {
                if (s_tx_reliable)
                {
                    scb_link_sent(&s_link, s_tx_seq, now);
                }
                s_tx_length = 0;
                s_tx_cursor = 0;
                s_tx_reliable = false;
            }
        }
        else if (n == 0)
        {
            ++health->short_writes;
        }
        else if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            close_control();
            return;
        }
    }
    /* 单帧无进展超过阈值才关闭并清理控制连接，不能阻塞SPI。 */
    if (s_tx_length > s_tx_cursor && now - s_tx_progress > BRIDGE_NET_STALL_LIMIT_MS)
    {
        ++health->stalls;
        close_control();
    }
}

/**
 * @brief 推进独立BUSY连接的短写与截止关闭。
 * @details 只操作第二连接图像/游标；完成、截止或非暂时错误时关闭第二连接，不挤掉控制槽。
 * @param[in,out] health 非空网络任务局部发送统计。
 * @param[in] writable 第二连接是否可写。
 * @param[in] now 当前单调毫秒数，截止比较使用大于等于。
 */
static void busy_service(bridge_net_snapshot_t *health, bool writable, uint64_t now)
{
    if (s_busy < 0)
    {
        return;
    }
    if (writable && s_busy_cursor < s_busy_length)
    {
        const size_t remaining = s_busy_length - s_busy_cursor;
        const ssize_t n = send(s_busy, s_busy_image + s_busy_cursor, remaining, 0);
        if (n > 0)
        {
            s_busy_cursor += (size_t)n;
            health->sent_bytes += (uint32_t)n;
            if ((size_t)n < remaining)
            {
                ++health->short_writes;
            }
        }
        else if (n == 0)
        {
            ++health->short_writes;
        }
        else if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            close(s_busy);
            s_busy = -1;
            return;
        }
    }
    if (s_busy_cursor >= s_busy_length || now >= s_busy_deadline)
    {
        close(s_busy);
        s_busy = -1;
    }
}

/**
 * @brief 接管首连接或安排独立BUSY应答。
 * @details 空控制槽记录对端IP并增加连接代际，清本地状态后等待HELLO。槽已占时按G-01编码BUSY，已有BUSY连接则直接关闭新fd；失败不影响首连接。
 * @param[in,out] health 非空网络任务局部连接统计。
 * @param[in,out] workspace 非空独占JSON工作区。
 * @param[in] now 当前单调毫秒数，用于500ms BUSY截止。
 */
static void accept_service(
    bridge_net_snapshot_t *health, bridge_json_workspace_t *workspace, uint64_t now)
{
    struct sockaddr_in peer = {0};
    socklen_t peer_length = sizeof(peer);
    const int fd = accept(s_listen, (struct sockaddr *)&peer, &peer_length);
    if (fd < 0)
    {
        return;
    }
    if (s_conn < 0)
    {
        if (s_generation == UINT32_MAX && s_generation_high == UINT32_MAX)
        {
            close(fd);
            return;
        }
        /* 控制槽空闲时接管，随后等待HELLO。 */
        s_conn = fd;
        portENTER_CRITICAL(&s_udp_mux);
        ++s_generation;
        if (s_generation == 0)
        {
            ++s_generation_high;
        }
        portEXIT_CRITICAL(&s_udp_mux);
        ++health->connected;
        s_peer_ip = peer.sin_addr.s_addr;
        s_rx_length = 0;
        s_tx_length = 0;
        s_tx_cursor = 0;
        s_tx_reliable = false;
        s_hello_done = false;
        s_subscribed = false;
        s_want_idle = false;
        scb_link_reset(&s_link, SCB_TCP, 0);
        return;
    }
    ++health->rejected;
    if (s_busy >= 0)
    {
        close(fd);
        return;
    }
    /*
     * G-01：已占槽用独立图像/游标立即回复HELLO_ACK/BUSY，
     * 限定截止只影响第二连接，首连接继续服务。
     */
    size_t used;
    if (scb_session_tcp_busy(s_spi_known ? s_spi_session : 0, s_spi_known && s_spi_ready, workspace,
            s_busy_image, sizeof(s_busy_image), &used)
        != SCB_SESSION_REPLY)
    {
        close(fd);
        return;
    }
    s_busy = fd;
    s_busy_length = used;
    s_busy_cursor = 0;
    s_busy_deadline = now + BRIDGE_NET_BUSY_LIMIT_MS;
}

/**
 * @brief 消费跨任务遥测图像并按连接身份发送。
 * @details 锁内折叠溢出计数并把pending转为in_use，锁外核对连接/订阅/代际及出口后发送；完成再锁内释放图像。无绑定、无订阅或旧身份只计丢弃，发送失败另计。
 * @param[in,out] health 非空网络任务局部UDP统计。
 * @note socket及端口回调仅由网络任务执行；in_use期间offer不能改写图像。
 */
static void udp_service(bridge_net_snapshot_t *health)
{
    portENTER_CRITICAL(&s_udp_mux);
    if (s_udp_overflow)
    {
        s_udp_overflow = false;
        ++health->udp_dropped;
    }
    if (!s_udp_pending)
    {
        portEXIT_CRITICAL(&s_udp_mux);
        return;
    }
    const size_t length = s_udp_length;
    const uint32_t generation = s_udp_generation;
    s_udp_pending = false;
    s_udp_in_use = true;
    portEXIT_CRITICAL(&s_udp_mux);
    /* 用连接代际拒绝延迟旧工作；无出口、无订阅、断线或身份过期都只计丢弃。 */
    const bool deliver
        = s_conn >= 0 && s_subscribed && generation == s_generation && s_port.telemetry != NULL;
    if (!deliver)
    {
        ++health->udp_dropped;
    }
    else if (s_port.telemetry(s_port.context, s_peer_ip, s_sub_port, s_udp_image, length))
    {
        ++health->udp_sent;
    }
    else
    {
        ++health->udp_failed;
    }
    portENTER_CRITICAL(&s_udp_mux);
    s_udp_in_use = false;
    portEXIT_CRITICAL(&s_udp_mux);
}

/**
 * @brief NET消费生产遥测FIFO，重新生成UDP序号与CRC。
 * @details 独占NET图像；SPI只写bridge槽，旧offer在生产绑定后拒绝，避免两条供给链。
 * @param[in,out] health NET累计统计。
 * @param[in,out] workspace NET独占JSON工作区。
 */
static void bridge_udp_service(bridge_net_snapshot_t *health,
    bridge_json_workspace_t *workspace)
{
    scb_frame_view_t view;
    if (!bridge_state_udp_peek(&view))
    {
        return;
    }
    bool sent = false;
    if (s_conn >= 0 && s_subscribed && s_hello_done && s_spi_ready
        && view.header.session_id == s_link.session_id && s_bridge_udp_seq != UINT32_MAX)
    {
        view.header.seq = s_bridge_udp_seq++;
        view.header.ack_seq = 0;
        size_t used;
        if (scb_encode(&view.header, view.payload, SCB_UDP, s_udp_image,
                sizeof(s_udp_image), workspace, &used) == SCB_OK && s_port.telemetry
            && bridge_state_udp_valid())
        {
            sent = s_port.telemetry(s_port.context, s_peer_ip, s_sub_port, s_udp_image, used);
        }
        if (sent)
        {
            ++health->udp_sent;
        }
        else
        {
            ++health->udp_failed;
        }
    }
    else
    {
        ++health->udp_dropped;
        if (s_bridge_udp_seq == UINT32_MAX)
        {
            s_subscribed = false;
            publish_identity();
        }
    }
    bridge_state_udp_consumed(sent);
}

/* NET每轮重建的最多三项poll表。 */
static struct pollfd s_fds[3];
/* 本轮监听项索引，-1表示未加入。 */
static int s_listen_index;
/* 本轮控制项索引，-1表示未加入。 */
static int s_conn_index;
/* 本轮BUSY项索引，-1表示未加入。 */
static int s_busy_index;

/* 每轮只做一次有界poll，等待不得越过本端下个可靠截止。 */
/**
 * @brief 有界等待监听、控制和BUSY事件。
 * @details 重建本轮poll表；以10ms为上限，再按可靠重试和BUSY截止缩短等待。轮询只等待事件，不处理DMA。
 * @param[in] now 当前单调毫秒数，计算本端最近截止所用基准。
 */
static void poll_wait(uint64_t now)
{
    nfds_t count = 0;
    s_listen_index = s_conn_index = s_busy_index = -1;
    if (s_listen >= 0)
    {
        s_listen_index = (int)count;
        s_fds[count++] = (struct pollfd){.fd = s_listen, .events = POLLIN, .revents = 0};
    }
    if (s_conn >= 0)
    {
        s_conn_index = (int)count;
        short events = POLLIN;
        if (s_tx_length > s_tx_cursor)
        {
            events |= POLLOUT;
        }
        s_fds[count++] = (struct pollfd){.fd = s_conn, .events = events, .revents = 0};
    }
    if (s_busy >= 0)
    {
        s_busy_index = (int)count;
        s_fds[count++] = (struct pollfd){.fd = s_busy, .events = POLLOUT, .revents = 0};
    }
    int timeout = BRIDGE_NET_POLL_LIMIT_MS;
    if (s_conn >= 0 && s_hello_done && !s_link.dead && s_link.pending && s_link.sent
        && !s_link.sending)
    {
        const uint64_t due = s_link.last_send_ms + SCB_RETRY_MS;
        const int remaining = due > now ? (int)(due - now) : 0;
        if (remaining < timeout)
        {
            timeout = remaining;
        }
    }
    if (s_busy >= 0)
    {
        const int remaining = s_busy_deadline > now ? (int)(s_busy_deadline - now) : 0;
        if (remaining < timeout)
        {
            timeout = remaining;
        }
    }
    (void)poll(s_fds, count, timeout);
}

/**
 * @brief 串行运行网络状态与socket服务。
 * @details 每轮先核对SPI会话并推进重试/IDLE，再poll；依次处理BUSY、控制TX/RX、accept、UDP并发布快照。保持这一顺序以免旧连接工作进入新控制槽。
 * @param[in,out] health 非空网络任务局部诊断。
 * @param[in,out] workspace 非空网络任务独占JSON工作区。
 * @return 正常无限循环不返回；保留末尾ESP_OK语句。
 */
static esp_err_t run_service(bridge_net_snapshot_t *health, bridge_json_workspace_t *workspace)
{
    for (;;)
    {
        uint64_t now = now_ms();
        session_check(health);
        if (s_bridge_bound)
        {
            if (s_conn >= 0 && bridge_state_take_close())
            {
                close_control();
            }
        }
        link_service(health, now);
        if (s_bridge_bound)
        {
            bridge_state_poll(now);
            bridge_stats_poll(now);
            bridge_tx_service(workspace, now);
        }
        idle_service(workspace, now);
        poll_wait(now);
        now = now_ms();
        /* poll期间SPI可换会话，先撤销旧连接，再解释其可读/可写事件。 */
        session_check(health);
        const short listen_events = s_listen_index >= 0 ? s_fds[s_listen_index].revents : 0;
        const short conn_events = s_conn_index >= 0 ? s_fds[s_conn_index].revents : 0;
        const short busy_events = s_busy_index >= 0 ? s_fds[s_busy_index].revents : 0;
        if (busy_events & (POLLERR | POLLHUP | POLLNVAL))
        {
            close(s_busy);
            s_busy = -1;
        }
        else
        {
            busy_service(health, (busy_events & POLLOUT) != 0, now);
        }
        if (conn_events & (POLLERR | POLLHUP | POLLNVAL))
        {
            close_control();
        }
        else if (s_conn >= 0)
        {
            tx_service(health, (conn_events & POLLOUT) != 0, now);
            if (s_conn >= 0 && (conn_events & POLLIN))
            {
                rx_recv();
            }
            if (s_conn >= 0)
            {
                rx_consume(health, workspace, now);
            }
        }
        if (s_listen >= 0 && (listen_events & POLLIN))
        {
            accept_service(health, workspace, now);
        }
        if (s_bridge_bound)
        {
            bridge_udp_service(health, workspace);
        }
        else
        {
            udp_service(health);
        }
        publish(health);
    }
    /* 上方循环不退出，此返回仅保留原函数结束路径。 */
    return ESP_OK;
}

/* 生产UDP出口仅用网络任务的非阻塞socket；短写或失败由调用方计数，不等待重发。 */
/**
 * @brief 用网络任务持有的非阻塞UDP socket发送一帧。
 * @details 只做一次sendto；完整长度才算成功，不重试短报文，不解释帧内容。目的IP直接采用accept记录的网络字节序值。
 * @param[in] context 此生产绑定不使用，可为空。
 * @param[in] ipv4 对端IPv4，网络字节序。
 * @param[in] port 订阅UDP端口，主机序，由调用方保证1至65535。
 * @param[in] frame 非空只读借用帧，length字节在调用期间有效。
 * @param[in] length 报文字节数，不得超过1016；合法TELEMETRY由调用方保证。
 * @return 完整报文已交socket返回true，否则false。
 * @note 只在网络任务调用，不能从SPI任务/ISR直接发送。
 */
bool bridge_net_socket_telemetry(
    void *context, uint32_t ipv4, uint16_t port, const uint8_t *frame, size_t length)
{
    (void)context;
    if (s_udp < 0 || !frame || length > BRIDGE_NET_UDP_BYTES)
    {
        return false;
    }
    struct sockaddr_in destination = {0};
    destination.sin_family = AF_INET;
    destination.sin_port = htons(port);
    destination.sin_addr.s_addr = ipv4;
    const ssize_t sent = sendto(
        s_udp, frame, length, 0, (const struct sockaddr *)&destination, sizeof(destination));
    return sent == (ssize_t)length;
}

/**
 * @brief 建立固定地址TCP监听及可选UDP出口。
 * @details TCP创建/bind/listen/非阻塞任一步失败关闭本fd并报错；成功发布监听后再尝试UDP。UDP不可用只计后续发送失败，不回滚TCP服务。
 * @return TCP监听就绪ESP_OK，否则ESP_FAIL。
 * @note 网络任务初始化阶段调用，不改变AP或SPI状态。
 */
static esp_err_t init_sockets(void)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        ESP_LOGE("net", "listen socket failed errno=%d", errno);
        return ESP_FAIL;
    }
    esp_err_t error = ESP_FAIL;
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons(BRIDGE_NET_TCP_PORT);
    address.sin_addr.s_addr = htonl(0xC0A80401u); /* 固定监听地址192.168.4.1。 */
    if (bind(fd, (const struct sockaddr *)&address, sizeof(address)) != 0)
    {
        ESP_LOGE("net", "bind %u failed errno=%d", (unsigned)BRIDGE_NET_TCP_PORT, errno);
    }
    else if (listen(fd, BRIDGE_NET_BACKLOG) != 0)
    {
        ESP_LOGE("net", "listen failed errno=%d", errno);
    }
    else if (fcntl(fd, F_SETFL, O_NONBLOCK) != 0)
    {
        ESP_LOGE("net", "nonblocking listen failed errno=%d", errno);
    }
    else
    {
        error = ESP_OK;
    }
    if (error != ESP_OK)
    {
        close(fd);
        return error;
    }
    s_listen = fd;
    /* UDP创建失败不终止TCP服务，之后报文出口返回失败。 */
    s_udp = socket(AF_INET, SOCK_DGRAM, 0);
    if (s_udp >= 0 && fcntl(s_udp, F_SETFL, O_NONBLOCK) != 0)
    {
        close(s_udp);
        s_udp = -1;
    }
    if (s_udp < 0)
    {
        ESP_LOGW("net", "UDP socket unavailable; telemetry will be counted failed");
    }
    ESP_LOGI("net", "Listening on 192.168.4.1:%u backlog=%u core=%d priority=%u stack_bytes=%u",
        (unsigned)BRIDGE_NET_TCP_PORT, (unsigned)BRIDGE_NET_BACKLOG, xPortGetCoreID(),
        (unsigned)BRIDGE_NET_PRIORITY, (unsigned)sizeof(s_stack));
    return ESP_OK;
}

/**
 * @brief 初始化并持有网络运行期服务。
 * @details 建立socket，发布初始化状态并通知启动者，再用独占工作区运行循环；若循环返回，关闭本模块socket、发布失败并删除任务，不复位设备。
 * @param[in] context 此任务不使用，可为空。
 */
static void net_task(void *context)
{
    (void)context;
    s_init_error = init_sockets();
    bridge_net_snapshot_t health = {.init_error = s_init_error};
    publish(&health);
    xSemaphoreGive(s_boot);
    if (s_init_error == ESP_OK)
    {
        bridge_json_workspace_t workspace;
        health.service_error = run_service(&health, &workspace);
        close_control();
        if (s_busy >= 0)
        {
            close(s_busy);
            s_busy = -1;
        }
        if (s_listen >= 0)
        {
            close(s_listen);
            s_listen = -1;
        }
        if (s_udp >= 0)
        {
            close(s_udp);
            s_udp = -1;
        }
        publish(&health);
        ESP_LOGE("net", "Network service stopped error=%s; no reset",
            esp_err_to_name(health.service_error));
    }
    vTaskDelete(NULL);
}

/**
 * @brief 单次创建静态网络任务并等待初始化。
 * @details 先设置started再创建信号量及Core0/优先级5/8192字节栈任务；创建失败不重试。仅等待任务初始化，不等待SPI握手。
 * @param[in] context 启动协调器参数，此适配忽略，可为空。
 * @return 成功ESP_OK，否则已启动、资源不足、通知失败或实际初始化错误。
 * @note 启动前完成端口绑定，运行期不换表。
 */
esp_err_t bridge_net_start(void *context)
{
    (void)context;
    if (s_started)
    {
        return ESP_ERR_INVALID_STATE;
    }
    s_started = true;
    s_boot = xSemaphoreCreateBinaryStatic(&s_boot_storage);
    if (!s_boot)
    {
        portENTER_CRITICAL(&s_snapshot_mux);
        s_health.init_error = ESP_ERR_NO_MEM;
        portEXIT_CRITICAL(&s_snapshot_mux);
        return ESP_ERR_NO_MEM;
    }
    s_task = xTaskCreateStaticPinnedToCore(net_task, "scb_net", sizeof(s_stack), NULL,
        BRIDGE_NET_PRIORITY, s_stack, &s_task_storage, BRIDGE_NET_CORE);
    if (!s_task)
    {
        portENTER_CRITICAL(&s_snapshot_mux);
        s_health.init_error = ESP_ERR_NO_MEM;
        portEXIT_CRITICAL(&s_snapshot_mux);
        return ESP_ERR_NO_MEM;
    }
    /* 这里只等网络任务初始化通知，不依赖SPI握手。 */
    if (xSemaphoreTake(s_boot, portMAX_DELAY) != pdTRUE)
    {
        return ESP_ERR_INVALID_STATE;
    }
    return s_init_error;
}

/**
 * @brief 配置WPA2热点及固定IPv4与DHCP。
 * @details 依次建立AP接口、停DHCP/设地址/启DHCP、初始化Wi-Fi、选AP模式、读取MAC并配置SSID/密码，最后启动。失败按已初始化标志反初始化Wi-Fi并销毁本接口，保留
 *     原错误。
 * @param[in] context 启动协调器参数，此适配不使用，可为空。
 * @return 全部步骤成功ESP_OK，否则返回实际SDK错误；接口创建失败ESP_ERR_NO_MEM。
 * @note 仅启动协调器在密码合法且netif/事件循环成功后调用；本函数不重复校验密码，也不回退开放热点。
 */
esp_err_t bridge_net_ap_start(void *context)
{
    (void)context;
    esp_err_t error;
    esp_netif_t *ap = esp_netif_create_default_wifi_ap();
    if (!ap)
    {
        ESP_LOGE("net", "AP netif creation failed");
        return ESP_ERR_NO_MEM;
    }
    error = esp_netif_dhcps_stop(ap);
    if (error == ESP_OK)
    {
        const esp_netif_ip_info_t info =
        {
            .ip = {.addr = ESP_IP4TOADDR(192, 168, 4, 1)},
            .gw = {.addr = ESP_IP4TOADDR(192, 168, 4, 1)},
            .netmask = {.addr = ESP_IP4TOADDR(255, 255, 255, 0)}
        };
        error = esp_netif_set_ip_info(ap, &info);
    }
    if (error == ESP_OK)
    {
        error = esp_netif_dhcps_start(ap);
    }
    bool wifi_initialized = false;
    if (error == ESP_OK)
    {
        wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
        error = esp_wifi_init(&init);
        wifi_initialized = error == ESP_OK;
    }
    if (error == ESP_OK)
    {
        error = esp_wifi_set_mode(WIFI_MODE_AP);
    }
    uint8_t mac[6] = {0};
    if (error == ESP_OK)
    {
        error = esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
    }
    if (error == ESP_OK)
    {
        static const char digits[] = "0123456789ABCDEF";
        char ssid[14] = "SmartCar-";
        ssid[9] = digits[mac[4] >> 4];
        ssid[10] = digits[mac[4] & 0xF];
        ssid[11] = digits[mac[5] >> 4];
        ssid[12] = digits[mac[5] & 0xF];
        ssid[13] = 0;
        wifi_config_t config =
        {
            .ap
            = {.ssid_len = 13, .channel = 1, .authmode = WIFI_AUTH_WPA2_PSK, .max_connection = 4}
        };
        for (size_t i = 0; i < 13u; ++i)
        {
            config.ap.ssid[i] = (uint8_t)ssid[i];
        }
        static const unsigned char password[] = CONFIG_BRIDGE_AP_PASSWORD;
        for (size_t i = 0; i < sizeof(password) - 1u; ++i)
        {
            config.ap.password[i] = password[i];
        }
        /* 密码合法性由启动协调器先检查；此路径仅配置WPA2-PSK，不回退开放AP。 */
        error = esp_wifi_set_config(WIFI_IF_AP, &config);
    }
    if (error == ESP_OK)
    {
        error = esp_wifi_start();
    }
    if (error != ESP_OK)
    {
        ESP_LOGE("net", "AP startup failed error=%s; resources released, no open fallback",
            esp_err_to_name(error));
        if (wifi_initialized)
        {
            esp_wifi_deinit();
        }
        esp_netif_destroy_default_wifi(ap);
    }
    return error;
}
