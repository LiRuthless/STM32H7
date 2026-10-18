#pragma once
#include "test_sdk.h"
typedef int BaseType_t;
typedef unsigned UBaseType_t;
typedef uint32_t StackType_t;
typedef void *TaskHandle_t;
typedef struct { unsigned char host_only_storage[128]; } StaticTask_t;
typedef void (*TaskFunction_t)(void *);
#define pdTRUE 1
#define portMAX_DELAY UINT32_MAX
TaskHandle_t xTaskCreateStaticPinnedToCore(TaskFunction_t task, const char *name, uint32_t stack_bytes,
    void *arg, UBaseType_t priority, StackType_t *stack, StaticTask_t *storage, BaseType_t core);
int xPortGetCoreID(void);
void vTaskDelete(TaskHandle_t task);
uint32_t ulTaskNotifyTake(BaseType_t clear, uint32_t ticks);
