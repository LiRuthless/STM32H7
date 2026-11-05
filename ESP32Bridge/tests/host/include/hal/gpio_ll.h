/**
 * @file gpio_ll.h
 * @brief 把READY低层写操作转发至夹具事件记录。
 * @details 仅为tests/host自有夹具提供hal/gpio_ll.h所需的声明、固定常量或内联替身。
 *     实现由宿主夹具或本头内联提供，不链接真实SDK；不证明DMA、调度或硬件能力。
 */
#pragma once
#include "soc/gpio_struct.h"
void test_ready_level(unsigned pin, unsigned value);

/**
 * @brief 记录生产READY回调的低层GPIO写入。
 * @param[in] device 此替身忽略的GPIO占位指针，可为空。
 * @param[in] pin 要记录的引脚号。
 * @param[in] value 要记录的电平。
 */
static inline void gpio_ll_set_level(gpio_dev_t *device, unsigned pin, unsigned value)
{
    (void)device;
    test_ready_level(pin, value);
}
