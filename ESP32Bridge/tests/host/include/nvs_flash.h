/**
 * @file nvs_flash.h
 * @brief 导出只记录启动顺序的NVS替身。
 * @details 仅为tests/host自有夹具提供nvs_flash.h所需的声明、固定常量或内联替身。
 *     实现由宿主夹具或本头内联提供，不链接真实SDK；不证明DMA、调度或硬件能力。
 */
#pragma once
#include "test_sdk.h"
