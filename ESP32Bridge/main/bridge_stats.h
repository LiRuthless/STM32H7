/**
 * @file bridge_stats.h
 * @brief 本地桥接诊断调度。
 * @details NET任务串行调用，具体快照及日志实现在bridge_stats.c。
 */
#ifndef BRIDGE_STATS_H
#define BRIDGE_STATS_H
#include <stdint.h>
/** @brief NET每轮调用，独立5秒串口诊断；bridge_stats.c::bridge_stats_poll。 */
void bridge_stats_poll(uint64_t now_ms);
#endif
