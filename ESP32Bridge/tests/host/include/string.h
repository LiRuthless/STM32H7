/**
 * @file string.h
 * @brief 提供无CRT夹具的字节填充和复制声明。
 * @details 仅为tests/host自有夹具提供string.h所需的声明、固定常量或内联替身。
 *     实现由宿主夹具或本头内联提供，不链接真实SDK；不证明DMA、调度或硬件能力。
 */
#pragma once
#include <stddef.h>
void *memset(void *destination, int value, size_t count);
void *memcpy(void *destination, const void *source, size_t count);
