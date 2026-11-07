/**
 * @file bridge_stats.c
 * @brief 汇总owner快照并输出串口统计。
 * @details NET每轮输入单调毫秒，5秒采集bridge/SPI/NET/池快照；各快照分别一致，
 *     不承诺跨owner同一时刻。所有日志在锁外，不向业务socket输出文本。
 */
#include "bridge_stats.h"
#include "bridge_state.h"
#include "buffer_pool.h"
#include "spi_bridge.h"
#include "net_bridge.h"
#include "esp_log.h"

/* NET独占的日志节拍，不按控制连接断开清零。 */
static uint64_t s_last_log;

/**
 * @brief 以独立5秒节拍采集快照并输出本地诊断。
 * @details 包、字节、背压尝试分别输出；快照读取返回后才调用日志系统。
 * @param[in] now_ms NET循环单调毫秒，回退时不输出。
 */
void bridge_stats_poll(uint64_t now_ms)
{
    if (now_ms < s_last_log || now_ms - s_last_log < 5000u)
    {
        return;
    }
    s_last_log = now_ms;
    bridge_state_snapshot_t bridge;
    bridge_spi_snapshot_t spi;
    bridge_net_snapshot_t net;
    bridge_pool_snapshot_t pool;
    bridge_state_snapshot(&bridge);
    (void)bridge_spi_snapshot(&spi);
    (void)bridge_net_snapshot(&net);
    (void)bridge_pool_snapshot(&pool);
    ESP_LOGI("bridge", "session=%lu ready=%d cmd=%lu result=%lu orphan=%lu canceled=%lu "
        "bp_cmd=%lu bp_result=%lu bp_event=%lu credit=%lu tombstone=%lu export_timeout=%lu",
        (unsigned long)bridge.session, bridge.ready, (unsigned long)bridge.commands,
        (unsigned long)bridge.results, (unsigned long)bridge.orphan_results,
        (unsigned long)bridge.canceled_results, (unsigned long)bridge.command_backpressure,
        (unsigned long)bridge.result_backpressure, (unsigned long)bridge.event_backpressure,
        (unsigned long)bridge.credit_mask, (unsigned long)bridge.tombstone_mask,
        (unsigned long)bridge.export_timeouts);
    ESP_LOGI("bridge", "udp_drop=%lu offline=%lu overflow=%lu cleared=%lu failed=%lu "
        "udp_peak=%lu event_peak=%lu pool_udp=%lu pool_event=%lu dma=%d",
        (unsigned long)bridge.telemetry_dropped, (unsigned long)bridge.telemetry_offline,
        (unsigned long)bridge.telemetry_overflow, (unsigned long)bridge.telemetry_cleared,
        (unsigned long)bridge.telemetry_failed, (unsigned long)bridge.telemetry_peak,
        (unsigned long)bridge.event_peak, (unsigned long)pool.used[BRIDGE_POOL_UDP],
        (unsigned long)pool.used[BRIDGE_POOL_EVENT], (int)pool.dma_owner);
    ESP_LOGI("bridge", "spi_rx=%lu spi_crc=%lu spi_short=%lu spi_retry=%lu "
        "tcp_rx=%lu tcp_bytes=%lu tcp_crc=%lu udp_sent=%lu udp_failed=%lu",
        (unsigned long)spi.protocol.received, (unsigned long)spi.protocol.crc_errors,
        (unsigned long)spi.truncated, (unsigned long)spi.protocol.retries,
        (unsigned long)net.received, (unsigned long)net.sent_bytes,
        (unsigned long)net.crc_errors, (unsigned long)net.udp_sent,
        (unsigned long)net.udp_failed);
}
