/**
 * @file bsp_key.h
 * @brief BSP PC13 按键事件；主循环完成消抖、短按和长按识别。
 */
#ifndef SMARTCAR_BSP_BSP_KEY_H
#define SMARTCAR_BSP_BSP_KEY_H

#include "main.h"

/* 启动按键：板载 K1 = PC13（下拉输入，按下为高），主循环轮询 + 消抖。
 * ADC 分压键盘（PA2/PA3）由 App 层 menu 直接经 BSP_ADC_Read 读取，不在此模块。 */

typedef enum
{
    BSP_KEY_NONE = 0,
    BSP_KEY_START,
    BSP_KEY_TOGGLE
} bsp_key_event_t;

/**
 * @brief 初始化 PC13 事件识别并要求先释放按键。
 * @note 停车主循环调用，复位消抖与长按状态。
 */
void BSP_Key_Init(void);
/**
 * @brief 消费一次按键轮询并判断短按松开事件。
 * @return 本次为 BSP_KEY_START 返回 1，其他事件返回 0。
 * @note 主循环调用；会推进事件状态并消费长按事件，不能与 PollEvent 重复轮询。
 */
uint8_t BSP_Key_StartPressed(void);


/**
 * @brief 推进双沿 30 ms 消抖、短按松开和一秒长按识别。
 * @return NONE 无事件；START 短按松开；TOGGLE 长按首次达到一秒。
 * @note 主循环串行调用；状态保存在模块内，每次长按仅报告一次。
 */
bsp_key_event_t BSP_Key_PollEvent(void);
/**
 * @brief 清空按键事件并要求释放后才能再次响应。
 * @note 前台起停流程调用；保留当前电平读取和标志写入次序。
 */
void BSP_Key_RequireRelease(void);

#endif /* __BSP_KEY_H */
