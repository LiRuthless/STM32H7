/**
 * @file esp_event.h
 * @brief 从共同边界导出默认事件循环初始化声明。
 * @details 仅为tests/host自有夹具提供esp_event.h所需的声明、固定常量或内联替身。
 *     实现由宿主夹具或本头内联提供，不链接真实SDK；不证明DMA、调度或硬件能力。
 */
#pragma once
#include "test_sdk.h"
