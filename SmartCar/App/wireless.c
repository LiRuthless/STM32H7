// ============================================================
// 文件名: wireless.c
// 功能说明: 无线/串口调参模块（源 config.c 的 uart_adjust）
// 协议与源工程一致：「字母字母+数字+终止字节」，数值 = 数字×0.01。
//   例：vp15\n → len=5 → KP_v = 0.1*1 + 0.01*5 = 0.15
//   首字母：v=速度环(p=KP_v/i=KI_v)，x=方向环(p=KP_x/d=KD_x)
// 已删除源 isr.c 的 0x7F 串口自动下载逻辑（STC ISP 特性）。
// ============================================================

#include "bsp.h"
#include "app.h"
#include "wireless.h"
#include "pid.h"
#include <stdio.h>
#include <string.h>

// 函数名: wireless_adjust
// 功能: 串口无线调参解析，主循环周期调用
void wireless_adjust(void)
{
    float figure = 0.0f;

    uint16_t len = BSP_UART_Read(dat, sizeof(dat));

    if(len == 0)
    {
        return;
    }

    /* 合法帧长 4~7（2 字节命令 + 1~4 位数字 + 1 终止字节），其余丢弃 */
    if(len < 4 || len > 7)
    {
        memset(dat, 0, sizeof(dat));
        return;
    }

    /* 数字部分为 dat[2..len-2]，末字节为终止符不参与运算（与源一致） */
    switch(len)
    {
        case 4: figure = 0.01f * (dat[2]-'0');                                                                  break;
        case 5: figure = 0.01f * (dat[3]-'0') + 0.1f * (dat[2]-'0');                                            break;
        case 6: figure = 0.01f * (dat[4]-'0') + 0.1f * (dat[3]-'0') + 1.0f * (dat[2]-'0');                      break;
        case 7: figure = 0.01f * (dat[5]-'0') + 0.1f * (dat[4]-'0') + 1.0f * (dat[3]-'0') + 10.0f * (dat[2]-'0'); break;
        default: break;
    }

    char p0 = (char)dat[0];
    char p1 = (char)dat[1];
    uint8_t hit = 1;

    switch(dat[0])
    {
        case 'v':
            switch(dat[1])
            {
                case 'p': KP_v = figure; break;
                case 'i': KI_v = figure; break;
                default:  hit = 0;       break;
            }
            break;

        case 'x':
            switch(dat[1])
            {
                case 'p': KP_x = figure; break;
                case 'd': KD_x = figure; break;
                default:  hit = 0;       break;
            }
            break;

        default: hit = 0; break;
    }

    memset(dat, 0, len);

    /* 回显（数值×100 用整数打印，避免 %f） */
    if(hit)
    {
        int32_t c = (int32_t)(figure * 100.0f + (figure >= 0 ? 0.5f : -0.5f));
        sprintf((char *)uart, "%c%c=%ld\r\n", p0, p1, (long)c);
        BSP_UART_WriteString((const char *)uart);
    }
}
