/**
 * @file esp_intr_alloc.h
 * @brief 提供夹具核对的中断属性常量。
 * @details 仅为tests/host自有夹具提供esp_intr_alloc.h所需的声明、固定常量或内联替身。
 *     实现由宿主夹具或本头内联提供，不链接真实SDK；不证明DMA、调度或硬件能力。
 */
#pragma once
#define ESP_INTR_FLAG_IRAM (1 << 10)
