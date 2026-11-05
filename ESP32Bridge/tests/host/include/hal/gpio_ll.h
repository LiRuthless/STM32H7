#pragma once
#include "soc/gpio_struct.h"
void test_ready_level(unsigned pin, unsigned value);
static inline void gpio_ll_set_level(gpio_dev_t *device, unsigned pin, unsigned value)
{ (void)device; test_ready_level(pin, value); }
