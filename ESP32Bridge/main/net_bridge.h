/**
 * @file net_bridge.h
 * @brief SoftAP与单控制TCP/订阅UDP服务。
 * @details 声明SoftAP与单控制TCP/订阅UDP服务接口；实现及完整参数/错误语义位于net_bridge.c。
 *     调用方按接口约束串行访问owner状态，头文件不定义线图像或运行期服务。
 */
#ifndef BRIDGE_NET_BRIDGE_H
#define BRIDGE_NET_BRIDGE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "protocol.h"

enum
{
    BRIDGE_NET_CORE = 0, /**< 网络任务固定核号。 */
    BRIDGE_NET_PRIORITY = 5, /**< 网络任务优先级。 */
    BRIDGE_NET_STACK_BYTES = 8192, /**< 网络任务静态栈字节数。 */
    BRIDGE_NET_TCP_PORT = 5000, /**< TCP监听端口，主机序。 */
    BRIDGE_NET_BACKLOG = 2, /**< 监听backlog参数，不等于控制槽数。 */
    BRIDGE_NET_RX_BYTES = 1024, /**< TCP接收图像字节容量。 */
    BRIDGE_NET_TX_BYTES = 1024, /**< TCP发送图像字节容量。 */
    BRIDGE_NET_BUSY_BYTES = 256, /**< 独立BUSY图像字节容量。 */
    BRIDGE_NET_UDP_BYTES = 1016, /**< UDP图像最大字节数。 */
    BRIDGE_NET_BUSY_LIMIT_MS = 500, /**< 第二连接BUSY完成期限，毫秒。 */
    BRIDGE_NET_STALL_LIMIT_MS = 2000, /**< 单帧无发送进展阈值，毫秒。 */
    BRIDGE_NET_POLL_LIMIT_MS = 10 /**< 单次网络等待上限，毫秒。 */
};

/*
 * 启动前绑定的非阻塞UDP出口；收到完整TELEMETRY图像，
 * IPv4为网络字节序、端口为订阅值；false仅表示本报文未完整发送。
 */
typedef struct
{
    void *context; /**< 启动绑定的出口上下文，借用至服务结束。 */
    bool (*telemetry)(
        /**
         * @brief NET任务调用的非阻塞UDP出口；完整发送才返回true。
         */
        void *context, uint32_t ipv4, uint16_t port, const uint8_t *frame, size_t length);
} bridge_net_port_t;

/**
 * @brief 在服务启动前绑定UDP出口。
 * @note net_bridge.c::bridge_net_configure；配置/启动仅启动阶段；socket出口仅NET任务，offer供任务提交。
 */
esp_err_t bridge_net_configure(const bridge_net_port_t *port);
/** @brief 启动前启用生产桥接消费者；net_bridge.c::bridge_net_bind_bridge。 */
esp_err_t bridge_net_bind_bridge(void);
/**
 * @brief 用网络任务持有的非阻塞UDP socket发送一帧。
 * @note net_bridge.c::bridge_net_socket_telemetry；配置/启动仅启动阶段；socket出口仅NET任务，offer供任务提交。
 */
bool bridge_net_socket_telemetry(
    void *context, uint32_t ipv4, uint16_t port, const uint8_t *frame, size_t length);
/**
 * @brief 配置WPA2热点及固定IPv4与DHCP。
 * @note net_bridge.c::bridge_net_ap_start；配置/启动仅启动阶段；socket出口仅NET任务，offer供任务提交。
 */
esp_err_t bridge_net_ap_start(void *context);
/**
 * @brief 单次创建静态网络任务并等待初始化。
 * @note net_bridge.c::bridge_net_start；配置/启动仅启动阶段；socket出口仅NET任务，offer供任务提交。
 */
esp_err_t bridge_net_start(void *context);
/**
 * @brief 复制一个完整遥测帧到跨任务UDP暂存图像。
 * @note net_bridge.c::bridge_net_telemetry_offer；配置/启动仅启动阶段；socket出口仅NET任务，offer供任务提交。
 */
esp_err_t bridge_net_telemetry_offer(const uint8_t *frame, size_t length);

typedef struct
{
    esp_err_t init_error; /**< 网络任务socket初始化结果。 */
    esp_err_t service_error; /**< 服务循环返回的错误，未返回时为初值。 */
    uint32_t connected; /**< 成功占用首控制槽的连接次数。 */
    uint32_t rejected; /**< 控制槽已占时的新连接次数。 */
    uint32_t received; /**< HELLO成功及COMMAND/IDLE处理分支计数，不等于业务交付数。 */
    uint32_t crc_errors; /**< 完整入帧CRC错误次数。 */
    uint32_t invalid_closes; /**< 非法头/帧/许可或应答编码失败导致关闭的计数。 */
    uint32_t sent_bytes; /**< 控制及BUSY正数send累计字节数，不含UDP。 */
    uint32_t short_writes; /**< send短写或返回零次数。 */
    uint32_t stalls; /**< 控制帧无进展超过阈值而关闭次数。 */
    uint32_t link_failures; /**< 非零会话可靠重建导致的关闭次数。 */
    uint32_t subscriptions; /**< 合法本地SUBSCRIBE接受次数。 */
    uint32_t udp_sent; /**< 出口确认完整发送的遥测报文数。 */
    uint32_t udp_failed; /**< 调用出口但未完整发送的报文数。 */
    uint32_t udp_dropped; /**< 消费时无资格或折叠溢出的丢弃计数。 */
    uint32_t backpressure; /**< 无生产消费者的可靠COMMAND背压次数。 */
    uint32_t session_closes; /**< SPI会话变化而关闭控制连接次数。 */
    uint32_t stack_free_min; /**< SDK报告的网络任务历史最小剩余栈值，保留原单位。 */
    uint32_t peer_ipv4; /**< 最近记录的TCP对端IPv4，网络字节序；关闭后可保留旧值。 */
    uint16_t udp_port; /**< 发布时有效订阅端口，未订阅为零。 */
    bool listening; /**< 发布时监听fd有效。 */
    bool control_open; /**< 发布时首控制连接fd有效。 */
    bool subscribed; /**< 发布时本连接已本地订阅。 */
} bridge_net_snapshot_t;

/**
 * @brief 读取已发布的网络状态副本。
 * @note net_bridge.c::bridge_net_snapshot；配置/启动仅启动阶段；socket出口仅NET任务，offer供任务提交。
 */
esp_err_t bridge_net_snapshot(bridge_net_snapshot_t *snapshot);
#endif
