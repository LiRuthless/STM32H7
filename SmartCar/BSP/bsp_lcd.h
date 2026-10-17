/**
 * @file bsp_lcd.h
 * @brief BSP LCD 字符与背光接口；停车主循环使用 SPI4。
 * @details 声明应用可用的 LCD 绘制接口；完整格式化和厂商调用边界位于 bsp_lcd.c。
 */
#ifndef SMARTCAR_BSP_BSP_LCD_H
#define SMARTCAR_BSP_BSP_LCD_H

#include "main.h"

/* 板载 ST7735（SPI4：SCK=PE12 MOSI=PE14，CS=PE11，DC=PE13，背光=PE10 TIM1_CH2N，RST 硬接 NRST）
 * 底层驱动移植自核心板 SDK 03-LCD_Test（BSP/lcd/ 目录）。
 * 浮点显示用整数拆分实现，避免 MicroLIB 不支持 %f。 */

/* RGB565 颜色 */
#define BSP_LCD_WHITE 0xFFFF
#define BSP_LCD_BLACK 0x0000
#define BSP_LCD_BLUE 0x001F
#define BSP_LCD_RED 0xF800
#define BSP_LCD_GREEN 0x07E0
#define BSP_LCD_YELLOW 0xFFE0
#define BSP_LCD_CYAN 0x07FF
#define BSP_LCD_MAGENTA 0xF81F
#define BSP_LCD_GRAY 0x8410

/* 实际像素宽（横屏） */
extern const uint16_t g_bsp_lcd_width;
/* 实际像素高（横屏） */
extern const uint16_t g_bsp_lcd_height;

/**
 * @brief 初始化显示、清黑屏并打开背光。
 * @note 启动主循环调用；保持原有底层初始化结果忽略行为。 完整契约见 bsp_lcd.c::BSP_LCD_Init。
 */
void BSP_LCD_Init(void);
/**
 * @brief 按厂商上下文宽高填充整屏。
 * @note color RGB565 颜色，0～65535。 停车主循环阻塞使用 SPI4；不与 TIM15 IMU 访问交错。 完整契约见 bsp_lcd.c::BSP_LCD_Clear。
 */
void BSP_LCD_Clear(uint16_t color);
/**
 * @brief 设置当前 LCD 配置的背光。
 * @note percent 板载屏百分比 0～100，超限截为 100；外接屏零灭非零亮。 停车主循环调用，保持两种屏幕不同的硬件控制方式。 完整契约见
 *     bsp_lcd.c::BSP_LCD_SetBacklight。
 */
void BSP_LCD_SetBacklight(uint8_t percent);
/**
 * @brief 显示一个 8×16 字符，越界时不绘制。
 * @note x 起始横坐标，像素；可见范围 0～159。 y 起始纵坐标，像素；可见范围 0～79。 停车主循环阻塞使用 SPI4，不自动换行；保持原有截断与边界行为。 完整契约见
 *     bsp_lcd.c::BSP_LCD_ShowChar。
 */
void BSP_LCD_ShowChar(uint16_t x, uint16_t y, char ch, uint16_t fc, uint16_t bc);
/**
 * @brief 顺序显示可打印 ASCII 字符，非法字符或行末停止。
 * @note x 起始横坐标，像素；可见范围 0～159。 y 起始纵坐标，像素；可见范围 0～79。 停车主循环阻塞使用 SPI4，不自动换行；保持原有截断与边界行为。 完整契约见
 *     bsp_lcd.c::BSP_LCD_ShowString。
 */
void BSP_LCD_ShowString(uint16_t x, uint16_t y, const char *str, uint16_t fc, uint16_t bc);
/**
 * @brief 按原整数拆分和右对齐方式绘制数值。
 * @note x 起始横坐标，像素；可见范围 0～159。 y 起始纵坐标，像素；可见范围 0～79。 停车主循环阻塞使用 SPI4，不自动换行；保持原有截断与边界行为。 完整契约见
 *     bsp_lcd.c::BSP_LCD_ShowInt。
 */
void BSP_LCD_ShowInt(uint16_t x, uint16_t y, int32_t val, uint8_t width, uint16_t fc, uint16_t bc);
/**
 * @brief 按原定点拆分和舍入顺序绘制浮点数。
 * @note x 起始横坐标，像素；可见范围 0～159。 y 起始纵坐标，像素；可见范围 0～79。 停车主循环阻塞使用 SPI4，不自动换行；保持原有截断与边界行为。 完整契约见
 *     bsp_lcd.c::BSP_LCD_ShowFloat。
 */
void BSP_LCD_ShowFloat(
    uint16_t x, uint16_t y, float val, uint8_t int_w, uint8_t dec_w, uint16_t fc, uint16_t bc);

#endif /* __BSP_LCD_H */
