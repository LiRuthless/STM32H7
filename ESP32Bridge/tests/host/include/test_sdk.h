#pragma once
/* Host-only SDK boundary replacements. These values are never device evidence. */
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
enum { GPIO_NUM_14 = 14, GPIO_MODE_OUTPUT = 1, GPIO_PULLUP_DISABLE = 0,
       GPIO_PULLDOWN_DISABLE = 0, GPIO_INTR_DISABLE = 0 };
typedef struct { uint64_t pin_bit_mask; int mode, pull_up_en, pull_down_en, intr_type; } gpio_config_t;
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
