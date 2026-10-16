/* PC13 高电平按下；双沿30ms消抖，短按松开启动，长按1秒切换。 */
#include "bsp_key.h"
static uint8_t s_raw,s_stable,s_armed,s_pressed,s_long;
static uint32_t s_changed,s_pressed_at;
void BSP_Key_RequireRelease(void)
{
    s_raw=(HAL_GPIO_ReadPin(KEY_GPIO_Port,KEY_Pin)==GPIO_PIN_SET);
    s_stable=1; s_changed=HAL_GetTick();
    s_armed=s_pressed=s_long=0;
}
void BSP_Key_Init(void) { BSP_Key_RequireRelease(); }
bsp_key_event_t BSP_Key_PollEvent(void)
{
    uint32_t now=HAL_GetTick();
    uint8_t raw=(HAL_GPIO_ReadPin(KEY_GPIO_Port,KEY_Pin)==GPIO_PIN_SET);
    if(raw!=s_raw) {s_raw=raw;s_changed=now;}
    if((uint32_t)(now-s_changed)>=30u && s_stable!=s_raw)
    {
        s_stable=s_raw;
        if(!s_stable)
        {
            uint8_t start=s_armed && s_pressed && !s_long;
            s_armed=1;s_pressed=0;
            if(start) return BSP_KEY_START;
        }
        else if(s_armed) { s_pressed=1;s_long=0;s_pressed_at=now; }
    }
    if(s_armed && s_pressed && s_stable && s_raw && !s_long && (uint32_t)(now-s_pressed_at)>=1000u)
    {s_long=1;return BSP_KEY_TOGGLE;}
    return BSP_KEY_NONE;
}
uint8_t BSP_Key_StartPressed(void) { return BSP_Key_PollEvent()==BSP_KEY_START; }
