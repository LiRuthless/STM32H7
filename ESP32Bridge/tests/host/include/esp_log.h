#pragma once
#include "test_sdk.h"
#define ESP_LOGI(tag, ...) test_log(tag, __VA_ARGS__)
#define ESP_LOGW(tag, ...) test_log(tag, __VA_ARGS__)
#define ESP_LOGE(tag, ...) test_log(tag, __VA_ARGS__)
