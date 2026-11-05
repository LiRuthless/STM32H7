/**
 * @file spi_bridge.h
 * @brief SPI事务驱动与READY发布。
 * @details 声明SPI事务驱动与READY发布接口；实现及完整参数/错误语义位于spi_bridge.c。
 *     调用方按接口约束串行访问owner状态，头文件不定义线图像或运行期服务。
 */
#ifndef BRIDGE_SPI_BRIDGE_H
#define BRIDGE_SPI_BRIDGE_H
#include "esp_err.h"
#include "spi_protocol.h"

enum
{
    BRIDGE_SPI_CORE = 1, /**< SPI任务及驱动初始化固定核号。 */
    BRIDGE_SPI_PRIORITY = 12, /**< SPI任务优先级。 */
    BRIDGE_SPI_STACK_BYTES = 6144 /**< SPI任务静态栈字节数。 */
};

/**
 * @brief 在启动前复制SPI端口表。
 * @note spi_bridge.c::bridge_spi_configure；配置/启动仅启动阶段；快照供任务读取。
 */
esp_err_t bridge_spi_configure(const bridge_spi_port_t *port);
/**
 * @brief 单次创建SPI静态任务并等待初始化结果。
 * @note spi_bridge.c::bridge_spi_start；配置/启动仅启动阶段；快照供任务读取。
 */
esp_err_t bridge_spi_start(void *context);

typedef struct
{
    esp_err_t init_error; /**< 实际驱动初始化结果。 */
    esp_err_t service_error; /**< 事务循环返回错误，未返回时为初值。 */
    uint32_t queued; /**< 驱动成功接受的排队事务次数。 */
    uint32_t completed; /**< 取回原描述且成功reclaim的事务次数。 */
    uint32_t truncated; /**< 回收长度小于8192bit的次数。 */
    uint32_t overlength; /**< 回收报告长度大于8192bit的次数；不覆盖硬件饱和盲区。 */
    uint32_t wait_timeouts; /**< get_trans_result等待超时次数，不等于回收。 */
    uint32_t session_id; /**< 发布时逻辑SPI会话，零表示无会话。 */
    uint32_t stack_free_min; /**< 当前SDK报告的任务历史最小剩余栈值，保留原单位。 */
    bool link_ready; /**< 发布时SPI握手已进入READY。 */
    bool driver_owned; /**< 发布时应用尚未取得驱动回收证据。 */
    bridge_spi_protocol_stats_t protocol; /**< 同一发布点的SPI协议累计计数副本。 */
} bridge_spi_snapshot_t;

/**
 * @brief 读取SPI服务发布的快照副本。
 * @note spi_bridge.c::bridge_spi_snapshot；配置/启动仅启动阶段；快照供任务读取。
 */
esp_err_t bridge_spi_snapshot(bridge_spi_snapshot_t *snapshot);
#endif
