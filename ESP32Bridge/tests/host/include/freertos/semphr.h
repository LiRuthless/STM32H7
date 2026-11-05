/**
 * @file semphr.h
 * @brief 用token存储模拟初始化通知，不实现真实阻塞。
 * @details 仅为tests/host自有夹具提供freertos/semphr.h所需的声明、固定常量或内联替身。
 *     实现由宿主夹具或本头内联提供，不链接真实SDK；不证明DMA、调度或硬件能力。
 */
#pragma once
#include "freertos/task.h"

typedef struct
{
    int token; /**< 初始化通知计数，由夹具同步增减。 */
} StaticSemaphore_t;

typedef StaticSemaphore_t *SemaphoreHandle_t;
SemaphoreHandle_t xSemaphoreCreateBinaryStatic(StaticSemaphore_t *storage);
BaseType_t xSemaphoreGive(SemaphoreHandle_t semaphore);
BaseType_t xSemaphoreTake(SemaphoreHandle_t semaphore, uint32_t ticks);
