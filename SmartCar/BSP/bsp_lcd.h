#ifndef __BSP_LCD_H
#define __BSP_LCD_H

#include "main.h"

/* 板载 ST7735（SPI4：SCK=PE12 MOSI=PE14，CS=PE11，DC=PE13，背光=PE10 TIM1_CH2N，RST 硬接 NRST）
 * 底层驱动移植自核心板 SDK 03-LCD_Test（BSP/lcd/ 目录）。
 * 浮点显示用整数拆分实现，避免 MicroLIB 不支持 %f。 */

/* RGB565 颜色 */
#define LCD_WHITE       0xFFFF
#define LCD_BLACK       0x0000
#define LCD_BLUE        0x001F
#define LCD_RED         0xF800
#define LCD_GREEN       0x07E0
#define LCD_YELLOW      0xFFE0
#define LCD_CYAN        0x07FF
#define LCD_MAGENTA     0xF81F
#define LCD_GRAY        0x8410

extern const uint16_t BSP_LCD_W;    /* 实际像素宽（横屏） */
extern const uint16_t BSP_LCD_H;    /* 实际像素高（横屏） */

void BSP_LCD_Init(void);
void BSP_LCD_Clear(uint16_t color);
void BSP_LCD_SetBacklight(uint8_t percent);                         /* 0~100 */
void BSP_LCD_ShowChar(uint16_t x, uint16_t y, char ch, uint16_t fc, uint16_t bc);
void BSP_LCD_ShowString(uint16_t x, uint16_t y, const char *str, uint16_t fc, uint16_t bc);
void BSP_LCD_ShowInt(uint16_t x, uint16_t y, int32_t val, uint8_t width, uint16_t fc, uint16_t bc);
void BSP_LCD_ShowFloat(uint16_t x, uint16_t y, float val, uint8_t int_w, uint8_t dec_w, uint16_t fc, uint16_t bc);

#endif /* __BSP_LCD_H */
