/**
 * @file test_sdk.h
 * @brief 集中声明宿主分配、GPIO、初始化、锁与日志边界。
 * @details 仅为tests/host自有夹具提供test_sdk.h所需的声明、固定常量或内联替身。
 *     实现由宿主夹具或本头内联提供，不链接真实SDK；不证明DMA、调度或硬件能力。
 */
#pragma once
/* 仅宿主SDK边界替身，常量与结果不构成实机证据。 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
typedef int portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED 0
void test_lock(void);
void test_unlock(void);
#define portENTER_CRITICAL(mux) ((void)(mux), test_lock())
#define portEXIT_CRITICAL(mux) ((void)(mux), test_unlock())

enum
{
    GPIO_NUM_14 = 14,
    GPIO_MODE_OUTPUT = 1,
    GPIO_PULLUP_DISABLE = 0,
    GPIO_PULLDOWN_DISABLE = 0,
    GPIO_INTR_DISABLE = 0
};

typedef struct
{
    uint64_t pin_bit_mask; /**< GPIO配置位掩码，夹具核对READY引脚。 */
    int mode; /**< GPIO方向配置字段。 */
    int pull_up_en; /**< 上拉配置字段，保留SDK签名。 */
    int pull_down_en; /**< 下拉配置字段，保留SDK签名。 */
    int intr_type; /**< GPIO中断配置字段。 */
} gpio_config_t;

esp_err_t gpio_set_level(int gpio, unsigned level);
esp_err_t gpio_config(const gpio_config_t *config);
esp_err_t nvs_flash_init(void);
esp_err_t esp_netif_init(void);
esp_err_t esp_event_loop_create_default(void);
#define MALLOC_CAP_INTERNAL 1U
#define MALLOC_CAP_8BIT 2U
#define MALLOC_CAP_DMA 4U
void *heap_caps_aligned_alloc(size_t alignment, size_t bytes, uint32_t caps);
void heap_caps_free(void *ptr);
size_t heap_caps_get_free_size(uint32_t caps);
size_t heap_caps_get_minimum_free_size(uint32_t caps);
bool esp_ptr_internal(const void *ptr);
bool esp_ptr_dma_capable(const void *ptr);
void test_log(const char *tag, const char *format, ...);
