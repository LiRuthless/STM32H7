/**
 * @file gpio_struct.h
 * @brief 提供GPIO对象的宿主占位布局。
 * @details 仅为tests/host自有夹具提供soc/gpio_struct.h所需的声明、固定常量或内联替身。
 *     实现由宿主夹具或本头内联提供，不链接真实SDK；不证明DMA、调度或硬件能力。
 */
#pragma once

typedef struct
{
    unsigned host_only; /**< GPIO寄存器对象占位，不代表真实SDK寄存器布局。 */
} gpio_dev_t;

extern gpio_dev_t GPIO;
