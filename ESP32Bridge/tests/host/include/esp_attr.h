/**
 * @file esp_attr.h
 * @brief 保留宿主空IRAM属性，不能证明目标段放置。
 * @details 仅为tests/host自有夹具提供esp_attr.h所需的声明、固定常量或内联替身。
 *     实现由宿主夹具或本头内联提供，不链接真实SDK；不证明DMA、调度或硬件能力。
 */
#pragma once
/* 宿主执行不能证明IRAM放置，必须核对对应目标ELF。 */
#define IRAM_ATTR
