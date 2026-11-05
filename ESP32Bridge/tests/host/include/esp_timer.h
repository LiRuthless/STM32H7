/**
 * @file esp_timer.h
 * @brief 声明宿主虚拟时钟的微秒读取接口。
 * @details 仅为tests/host自有夹具提供esp_timer.h所需的声明、固定常量或内联替身。
 *     实现由宿主夹具或本头内联提供，不链接真实SDK；不证明DMA、调度或硬件能力。
 */
#pragma once
#include <stdint.h>
int64_t esp_timer_get_time(void);
