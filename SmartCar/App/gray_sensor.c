/**
 * @file gray_sensor.c
 * @brief App 标定、归一化与质心；采集仍由 BSP 完整帧接口提供。
 */
/* 采集来源 LQ_Track.c V2.0.0，chiusir/北京龙邱科技有限公司 (C)2025-2026，
 * GPL-3.0；版权、原许可证声明及 SHA256 见 BSP/bsp_gray.c。
 * 本模块增加 Phase14 手工标定和质心，百分比采用均值*100/4095。 */
#include "gray_sensor.h"
#include "gray_calibration.h"
#include "bsp.h"
#include <stdio.h>
#include <string.h>
/* TIM6 或停车采样更新；失败不发布半帧，前台显示/日志遵守既有快照契约。 */
static gray_state_t s_gray;

const gray_state_t *Gray_GetState(void)
{
    return &s_gray;
}

const int16_t *Gray_GetBlack(void)
{
    return s_gray_black;
}

const int16_t *Gray_GetWhite(void)
{
    return s_gray_white;
}

uint16_t Gray_ValidateCalibration(const int16_t *black, const int16_t *white)
{
    uint16_t mask = 0;
    uint8_t i;
    for (i = 0; i < 16; ++i)
    {
        int32_t delta = (int32_t)white[i] - black[i];
        if (black[i] < 0 || black[i] > 4095 || white[i] < 0 || white[i] > 4095
            || (delta > -200 && delta < 200))
        {
            mask |= (uint16_t)(1u << i);
        }
    }
    return mask;
}

uint8_t Gray_CheckCalibration(uint8_t report)
{
    uint16_t mask = Gray_ValidateCalibration(s_gray_black, s_gray_white);
    uint8_t i;
    s_gray.calibration_valid = (mask == 0);
    if (report)
    {
        for (i = 0; i < 16; ++i)
        {
            if (mask & (1u << i))
            {
                char text[48];
                (void)snprintf(text, sizeof(text), "gray calibration invalid: ch%u\r\n", i);
                BSP_UART_WriteString(text);
            }
        }
    }
    return s_gray.calibration_valid;
}

uint16_t Gray_Normalize(uint16_t raw, int16_t black, int16_t white)
{
    int32_t delta = (int32_t)white - black;
    int32_t q;
    if (black < 0 || black > 4095 || white < 0 || white > 4095 || (delta > -200 && delta < 200))
    {
        return 0;
    }
    q = 1000 * ((int32_t)raw - black) / delta;
    if (q < 50)
    {
        return 0;
    }
    if (q > 1000)
    {
        return 1000;
    }
    return (uint16_t)q;
}

int16_t Gray_ComputeError(const uint16_t *strength, uint8_t *line_valid)
{
    int32_t sum = 0;
    int32_t weighted = 0;
    uint16_t maximum = 0;
    uint8_t i;
    for (i = 0; i < 16; ++i)
    {
        sum += strength[i];
        weighted += (15 - 2 * (int32_t)i) * strength[i];
        if (strength[i] > maximum)
        {
            maximum = strength[i];
        }
    }
    *line_valid = (maximum >= 200 && sum != 0);
    return sum ? (int16_t)(100 * weighted / (15 * sum)) : 0;
}

void Gray_ResetSequence(void)
{
    BSP_Gray_ResetSequence();
    s_gray.sequence = 0;
}

void Gray_Init(void)
{
    memset(&s_gray, 0, sizeof(s_gray));
    BSP_Gray_Init();
    Gray_CheckCalibration(1);
}

void Gray_Update(void)
{
    bsp_gray_frame_t frame;
    uint8_t i;
    s_gray.acquisition_status = (uint8_t)BSP_Gray_ReadFrame(&frame);
    Gray_CheckCalibration(0);
    if (s_gray.acquisition_status != BSP_GRAY_OK)
    {
        return;
    }
    memcpy(s_gray.raw, frame.raw, sizeof(frame.raw));
    s_gray.sequence = frame.sequence;
    s_gray.scan_cycles = frame.scan_cycles;
    for (i = 0; i < 16; ++i)
    {
        s_gray.percent[i] = (uint16_t)((uint32_t)frame.raw[i] * 100u / 4095u);
        s_gray.strength[i] = Gray_Normalize(frame.raw[i], s_gray_black[i], s_gray_white[i]);
    }
    s_gray.error = Gray_ComputeError(s_gray.strength, &s_gray.line_valid);
    if (!s_gray.calibration_valid)
    {
        s_gray.line_valid = 0;
    }
}
