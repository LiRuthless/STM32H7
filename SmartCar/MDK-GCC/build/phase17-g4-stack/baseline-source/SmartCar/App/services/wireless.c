/**
 * @file wireless.c
 * @brief App 原串口调参和日志命令；仅停车前台解析，协议逐字保持。
 * @details 在停车前台解析单字符调参命令，修改参数、模式或日志请求，并通过 UART 返回结果。
 * @note 移植来源：Sirius20260718/project/code/config.c。
 * @note STC AI8051U 省赛快照（2026-07-18）；原算法未单列作者或独立许可证。
 * @note 保留源工程出处，关联 Phase 13/14/15 规约。
 */
#include "bsp.h"
#include "app.h"
#include "services/wireless.h"
#include "control/pid.h"
#include "services/datalog.h"
#include "services/param.h"
#include "control/tracking.h"
#include <stdio.h>
#include <string.h>

/* 停车前台串口命令的收发缓冲，各32字节，静态清零。 */
static uint8_t s_uart_tx_buffer[32];
static uint8_t s_uart_rx_buffer[32];

/**
 * @brief 处理原串口调参和日志命令。
 * @details 按现有调用顺序使用 BSP_UART_Read、BSP_UART_WriteString、Datalog_Dump、Datalog_Clear
 *     完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @note 仅停车前台调用；导出/擦除可能阻塞。保持 d/c、vp/vi/xp/xd、数字顺序、帧长及逐字回显。
 */
void Wireless_Adjust(void)
{
    float figure = 0.0f;

    uint16_t len = BSP_UART_Read(s_uart_rx_buffer, sizeof(s_uart_rx_buffer));

    if (len == 0)
    {
        return;
    }

    /* 单字母命令（2 字节：字母+终止符） */
    if (len == 2)
    {
        if (s_uart_rx_buffer[0] == 'd') /* 导出日志 */
        {
            BSP_UART_WriteString("dump start\r\n");
            Datalog_Dump();
        }
        else if (s_uart_rx_buffer[0] == 'c') /* 擦除首个 4KB 日志扇区（典型几十 ms） */
        {
            BSP_UART_WriteString("log erasing...\r\n");
            BSP_UART_WriteString(Datalog_Clear() == 0 ? "log erased\r\n" : "erase fail\r\n");
        }
        memset(s_uart_rx_buffer, 0, len);
        return;
    }

    /* 合法帧长 4~7（2 字节命令 + 1~4 位数字 + 1 终止字节），其余丢弃 */
    if (len < 4 || len > 7)
    {
        memset(s_uart_rx_buffer, 0, sizeof(s_uart_rx_buffer));
        return;
    }

    for (uint16_t i = 2; i < len - 1; i++)
    {
        if (s_uart_rx_buffer[i] < '0' || s_uart_rx_buffer[i] > '9')
        {
            memset(s_uart_rx_buffer, 0, len);
            return;
        }
    }

    /* 数字部分为 s_uart_rx_buffer[2..len-2]，末字节为终止符不参与运算（与源一致） */
    switch (len)
    {
        case 4:
            figure = 0.01f * (s_uart_rx_buffer[2] - '0');
            break;
        case 5:
            figure
                = 0.01f * (s_uart_rx_buffer[3] - '0') + 0.1f * (s_uart_rx_buffer[2] - '0');
            break;
        case 6:
            figure = 0.01f * (s_uart_rx_buffer[4] - '0')
                + 0.1f * (s_uart_rx_buffer[3] - '0') + 1.0f * (s_uart_rx_buffer[2] - '0');
            break;
        case 7:
            figure = 0.01f * (s_uart_rx_buffer[5] - '0')
                + 0.1f * (s_uart_rx_buffer[4] - '0') + 1.0f * (s_uart_rx_buffer[3] - '0')
                + 10.0f * (s_uart_rx_buffer[2] - '0');
            break;
        default:
            break;
    }

    char p0 = (char)s_uart_rx_buffer[0];
    char p1 = (char)s_uart_rx_buffer[1];
    uint8_t hit = 1;

    switch (s_uart_rx_buffer[0])
    {
        case 'v':
            switch (s_uart_rx_buffer[1])
            {
                case 'p':
                    PID_SetGain(PID_GAIN_KP_V, figure);
                    break;
                case 'i':
                    PID_SetGain(PID_GAIN_KI_V, figure);
                    break;
                default:
                    hit = 0;
                    break;
            }
            break;

        case 'x':
        {
            uint8_t mode = Tracking_GetState()->mode;
            uint8_t gear = Tracking_GetState()->gear;
            param_direction_t direction = *Param_GetDirection(mode, gear);
            switch (s_uart_rx_buffer[1])
            {
                case 'p':
                    direction.kp = figure;
                    break;
                case 'd':
                    direction.kd = figure;
                    break;
                default:
                    hit = 0;
                    break;
            }
            if (hit)
            {
                (void)Param_SetDirection(mode, gear, &direction);
            }
            break;
        }

        default:
            hit = 0;
            break;
    }

    memset(s_uart_rx_buffer, 0, len);

    /* 回显（数值×100 用整数打印，避免 %f） */
    if (hit)
    {
        int32_t c = (int32_t)(figure * 100.0f + (figure >= 0 ? 0.5f : -0.5f));
        sprintf((char *)s_uart_tx_buffer, "%c%c=%ld\r\n", p0, p1, (long)c);
        BSP_UART_WriteString((const char *)s_uart_tx_buffer);
        Param_MarkDirty();
    }
}
