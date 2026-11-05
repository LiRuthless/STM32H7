/**
 * @file esp_random.h
 * @brief 声明SPI夹具的可重复随机候选入口。
 * @details 仅为tests/host自有夹具提供esp_random.h所需的声明、固定常量或内联替身。
 *     实现由宿主夹具或本头内联提供，不链接真实SDK；不证明DMA、调度或硬件能力。
 */
#pragma once
#include <stdint.h>
uint32_t esp_random(void);
