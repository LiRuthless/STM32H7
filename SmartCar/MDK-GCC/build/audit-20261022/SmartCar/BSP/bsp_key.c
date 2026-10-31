/**
 * @file bsp_key.c
 * @brief BSP PC13 按键事件；主循环完成消抖、短按和长按识别。
 * @details 采样两路 ADC 分压按键，执行去抖、释放武装和单次事件发布；仅供停车前台轮询。
 */
/* PC13 高电平按下；双沿30ms消抖，短按松开启动，长按1秒切换。 */
#include "bsp_key.h"
/* 主循环独占事件状态；当前电平、稳定电平、释放门禁及长按历史分开保存。 */
static uint8_t s_raw;
static uint8_t s_stable;
static uint8_t s_armed;
static uint8_t s_pressed;
static uint8_t s_long;
static uint32_t s_changed;
static uint32_t s_pressed_at;

/**
 * @brief 清空按键事件并要求释放后才能再次响应。
 * @details 按现有调用顺序使用 HAL_GPIO_ReadPin、HAL_GetTick 完成本函数处理。
 * @note 前台起停流程调用；保留当前电平读取和标志写入次序。
 */
void BSP_Key_RequireRelease(void)
{
    s_raw = (HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin) == GPIO_PIN_SET);
    s_stable = 1;
    s_changed = HAL_GetTick();
    s_armed = s_pressed = s_long = 0;
}

/**
 * @brief 初始化 PC13 事件识别并要求先释放按键。
 * @note 停车主循环调用，复位消抖与长按状态。
 */
void BSP_Key_Init(void)
{
    BSP_Key_RequireRelease();
}

/**
 * @brief 推进双沿 30 ms 消抖、短按松开和一秒长按识别。
 * @details 按现有调用顺序使用 HAL_GetTick、HAL_GPIO_ReadPin 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @return NONE 无事件；START 短按松开；TOGGLE 长按首次达到一秒。
 * @note 主循环串行调用；状态保存在模块内，每次长按仅报告一次。
 */
bsp_key_event_t BSP_Key_PollEvent(void)
{
    /* 先根据释放门禁和当前 ADC 电平完成去抖；只有稳定按下且已武装时发布一次事件，持续按住不重复触发。 */
    uint32_t now = HAL_GetTick();
    uint8_t raw = (HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin) == GPIO_PIN_SET);
    if (raw != s_raw)
    {
        s_raw = raw;
        s_changed = now;
    }
    if ((uint32_t)(now - s_changed) >= 30u && s_stable != s_raw)
    {
        s_stable = s_raw;
        if (!s_stable)
        {
            uint8_t start = s_armed && s_pressed && !s_long;
            s_armed = 1;
            s_pressed = 0;
            if (start)
            {
                return BSP_KEY_START;
            }
        }
        else if (s_armed)
        {
            s_pressed = 1;
            s_long = 0;
            s_pressed_at = now;
        }
    }
    if (s_armed && s_pressed && s_stable && s_raw && !s_long
        && (uint32_t)(now - s_pressed_at) >= 1000u)
    {
        s_long = 1;
        return BSP_KEY_TOGGLE;
    }
    return BSP_KEY_NONE;
}

/**
 * @brief 消费一次按键轮询并判断短按松开事件。
 * @return 本次为 BSP_KEY_START 返回 1，其他事件返回 0。
 * @note 主循环调用；会推进事件状态并消费长按事件，不能与 PollEvent 重复轮询。
 */
uint8_t BSP_Key_StartPressed(void)
{
    return BSP_Key_PollEvent() == BSP_KEY_START;
}
