/**
 * @file task.h
 * @brief 提供静态任务签名和宿主存储，不实现RTOS调度。
 * @details 仅为tests/host自有夹具提供freertos/task.h所需的声明、固定常量或内联替身。
 *     实现由宿主夹具或本头内联提供，不链接真实SDK；不证明DMA、调度或硬件能力。
 */
#pragma once
#include "test_sdk.h"
typedef int BaseType_t;
typedef unsigned UBaseType_t;
typedef uint32_t StackType_t;
typedef void *TaskHandle_t;

typedef struct
{
    unsigned char host_only_storage[128]; /**< 任务控制块占位字节，不实现SDK内部布局。 */
} StaticTask_t;

typedef void (*TaskFunction_t)(void *);
#define pdTRUE 1
#define portMAX_DELAY UINT32_MAX
#define pdMS_TO_TICKS(ms) ((uint32_t)(ms))
TaskHandle_t xTaskCreateStaticPinnedToCore(TaskFunction_t task, const char *name,
    uint32_t stack_bytes, void *arg, UBaseType_t priority, StackType_t *stack,
    StaticTask_t *storage, BaseType_t core);
int xPortGetCoreID(void);
void vTaskDelete(TaskHandle_t task);
uint32_t ulTaskNotifyTake(BaseType_t clear, uint32_t ticks);
UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t task);
