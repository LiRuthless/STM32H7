/**
 * @file bsp_lcd.c
 * @brief BSP LCD 字符与背光接口；停车主循环使用 SPI4。
 * @details 封装 LCD 组件初始化、清屏、文本和数值绘制，为应用层隔离厂商 ST7735 接口。
 */
/* BSP 层 LCD 接口实现，底层为 BSP/lcd/ 下移植的 ST7735 驱动
 * 屏幕：0.96 寸 ST7735，横屏 160x80，8x16 ASCII 字库 */

#include "bsp_lcd.h"
#include "lcd/lcd.h"
#include "lcd/font.h"
#include "tim.h"
#include "app_config.h"

/* 横屏实际分辨率 */
#define BSP_LCD_FONT_WIDTH 8 /* 8x16 字库 */
#define BSP_LCD_FONT_HEIGHT 16

/* 横屏像素尺寸常量，仅供绘制边界计算读取，初始化后不存在写入上下文。 */
const uint16_t g_bsp_lcd_width = 160;
const uint16_t g_bsp_lcd_height = 80;


/**
 * @brief 初始化显示、清黑屏并打开背光。
 * @details 按现有调用顺序使用 BSP_LCDIO_Init、BSP_LCD_Clear、BSP_LCD_SetBacklight 完成本函数处理。
 * @note 启动主循环调用；保持原有底层初始化结果忽略行为。
 */
void BSP_LCD_Init(void)
{
    (void)BSP_LCDIO_Init();       /* 初始化序列 + 横屏设置，内含背光 PWM 启动 */
    BSP_LCD_Clear(BSP_LCD_BLACK); /* 清屏黑 */
    BSP_LCD_SetBacklight(100);    /* 默认 100% 亮度 */
}

/**
 * @brief 按厂商上下文宽高填充整屏。
 * @param[in] color RGB565 颜色，0～65535。
 * @note 停车主循环阻塞使用 SPI4；不与 TIM15 IMU 访问交错。
 */
void BSP_LCD_Clear(uint16_t color)
{
    ST7735_LCD_Driver.FillRect(&g_bsp_lcdio_object, 0, 0, ST7735Ctx.Width, ST7735Ctx.Height, color);
}

/* 背光：板载屏 PE10=TIM1_CH2N，ARR=999，OCNPolarity=LOW，PMOS 驱动（低=亮），
 * CCR 越大越亮（对齐 SDK 语义）；外接屏背光 PD10=GPIO（高电平点亮） */
/**
 * @brief 设置当前 LCD 配置的背光。
 * @details 按现有调用顺序使用 HAL_GPIO_WritePin、__HAL_TIM_SET_COMPARE 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @param[in] percent 板载屏百分比 0～100，超限截为 100；外接屏零灭非零亮。
 * @note 停车主循环调用，保持两种屏幕不同的硬件控制方式。
 */
void BSP_LCD_SetBacklight(uint8_t percent)
{
#if LCD_TARGET_EXTERNAL
    HAL_GPIO_WritePin(LCD2_BLK_GPIO_Port, LCD2_BLK_Pin, percent ? GPIO_PIN_SET : GPIO_PIN_RESET);
#else
    uint32_t ccr;
    if (percent > 100)
    {
        percent = 100;
    }
    ccr = (uint32_t)percent * 999 / 100;
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, ccr);
#endif
}

/**
 * @brief 显示一个 8×16 字符，越界时不绘制。
 * @details 按现有调用顺序使用 FillRGBRect 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @param[in] x 起始横坐标，像素；可见范围 0～159。
 * @param[in] y 起始纵坐标，像素；可见范围 0～79。
 * @param[in] ch ASCII 字符；不在空格至波浪号范围时显示为空格。
 * @param[in] fc 前景 RGB565 颜色。
 * @param[in] bc 背景 RGB565 颜色。
 * @note 停车主循环阻塞使用 SPI4，不自动换行；保持原有截断与边界行为。
 */
void BSP_LCD_ShowChar(uint16_t x, uint16_t y, char ch, uint16_t fc, uint16_t bc)
{
    uint8_t temp;
    uint8_t t1;
    uint8_t t;
    uint16_t y0 = y;
    uint16_t x0 = x;
    uint16_t write[16][8]; /* 与 SDK 一致的转置缓冲（该面板方向要求） */
    uint16_t count = 0;
    uint8_t num;

    if (ch < ' ' || ch > '~')
    {
        ch = ' ';
    }
    if ((x + BSP_LCD_FONT_WIDTH) > g_bsp_lcd_width || (y + BSP_LCD_FONT_HEIGHT) > g_bsp_lcd_height)
    {
        return;
    }
    num = (uint8_t)ch - ' ';

    for (t = 0; t < BSP_LCD_FONT_HEIGHT; t++)
    {
        temp = asc2_1608[num][t];
        for (t1 = 0; t1 < 8; t1++)
        {
            uint16_t c = (temp & 0x80) ? fc : bc;
            write[count][t / 2] = (uint16_t)((c & 0xFF) << 8 | c >> 8); /* 字节序同 SDK */
            count++;
            if (count >= BSP_LCD_FONT_HEIGHT)
            {
                count = 0;
            }
            temp <<= 1;
            y++;
            if ((y - y0) == BSP_LCD_FONT_HEIGHT)
            { /* 一列画完换下一列 */
                y = y0;
                x++;
                break;
            }
        }
    }
    ST7735_LCD_Driver.FillRGBRect(
        &g_bsp_lcdio_object, x0, y0, (uint8_t *)write, BSP_LCD_FONT_WIDTH, BSP_LCD_FONT_HEIGHT);
}

/**
 * @brief 顺序显示可打印 ASCII 字符，非法字符或行末停止。
 * @details 按现有调用顺序使用 BSP_LCD_ShowChar 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @param[in] x 起始横坐标，像素；可见范围 0～159。
 * @param[in] y 起始纵坐标，像素；可见范围 0～79。
 * @param[in] str 必须非空且以零结尾，逐字符读取。
 * @param[in] fc 前景 RGB565 颜色。
 * @param[in] bc 背景 RGB565 颜色。
 * @note 停车主循环阻塞使用 SPI4，不自动换行；保持原有截断与边界行为。
 */
void BSP_LCD_ShowString(uint16_t x, uint16_t y, const char *str, uint16_t fc, uint16_t bc)
{
    while (*str)
    {
        if (*str < ' ' || *str > '~')
        {
            break;
        }
        if ((x + BSP_LCD_FONT_WIDTH) > g_bsp_lcd_width)
        {
            break; /* 超出右边界停止 */
        }
        BSP_LCD_ShowChar(x, y, *str, fc, bc);
        x += BSP_LCD_FONT_WIDTH;
        str++;
    }
}

/* 十进制右对齐显示 int32，width 为字符数（含符号位） */
/**
 * @brief 按原整数拆分和右对齐方式绘制数值。
 * @details 按现有调用顺序使用 BSP_LCD_ShowChar 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @param[in] x 起始横坐标，像素；可见范围 0～159。
 * @param[in] y 起始纵坐标，像素；可见范围 0～79。
 * @param[in] val int32_t 数值，保持符号与整数拆分顺序。
 * @param[in] width 显示字符宽度；零或超过缓冲宽度时按 12 处理。
 * @param[in] fc 前景 RGB565 颜色。
 * @param[in] bc 背景 RGB565 颜色。
 * @note 停车主循环阻塞使用 SPI4，不自动换行；保持原有截断与边界行为。
 */
void BSP_LCD_ShowInt(uint16_t x, uint16_t y, int32_t val, uint8_t width, uint16_t fc, uint16_t bc)
{
    char buf[12]; /* 符号 + 最多10位数字 */
    uint8_t len = 0;
    uint8_t i;
    uint32_t mag;

    if (width == 0 || width > sizeof(buf))
    {
        width = sizeof(buf);
    }
    mag = (val < 0) ? (uint32_t)(-(int64_t)val) : (uint32_t)val;

    do
    { /* 先取数字（逆序） */
        buf[len++] = (char)('0' + mag % 10);
        mag /= 10;
    } while (mag && len < sizeof(buf));
    if (val < 0 && len < sizeof(buf))
    {
        buf[len++] = '-';
    }

    while (len < width)
    {
        buf[len++] = ' '; /* 左侧补空格右对齐 */
    }

    for (i = 0; i < width; i++) /* 正序输出 */
    {
        BSP_LCD_ShowChar((uint16_t)(x + i * BSP_LCD_FONT_WIDTH), y, buf[width - 1 - i], fc, bc);
    }
}

/* 浮点显示：先四舍五入到 dec_w 位小数，再拆整数/小数部分打印
 * （整数拆分法，避免 MicroLIB 不支持 %f） */
/**
 * @brief 按原定点拆分和舍入顺序绘制浮点数。
 * @details 按现有调用顺序使用 BSP_LCD_ShowChar、BSP_LCD_ShowInt 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @param[in] x 起始横坐标，像素；可见范围 0～159。
 * @param[in] y 起始纵坐标，像素；可见范围 0～79。
 * @param[in] val 浮点值，须满足现有缩放后 int32_t 转换的可表示范围。
 * @param[in] int_w 整数最小显示宽度；不足时按实际位数扩展。
 * @param[in] dec_w 小数位数，0～6；超过 6 时截为 6。
 * @param[in] fc 前景 RGB565 颜色。
 * @param[in] bc 背景 RGB565 颜色。
 * @note 停车主循环阻塞使用 SPI4，不自动换行；保持原有截断与边界行为。
 */
void BSP_LCD_ShowFloat(
    uint16_t x, uint16_t y, float val, uint8_t int_w, uint8_t dec_w, uint16_t fc, uint16_t bc)
{
    int32_t scale = 1;
    int32_t scaled;
    int32_t ipart;
    int32_t fpart;
    uint8_t i;
    uint8_t neg;
    uint8_t digits = 1;
    uint16_t cx;

    if (dec_w > 6)
    {
        dec_w = 6; /* float 精度有限，最多6位小数 */
    }
    for (i = 0; i < dec_w; i++)
    {
        scale *= 10;
    }

    /* 四舍五入到 dec_w 位小数；符号单独处理，避免 (-1,0) 丢负号 */
    scaled = (int32_t)(val * (float)scale + ((val >= 0) ? 0.5f : -0.5f));
    neg = (scaled < 0);
    if (neg)
    {
        scaled = -scaled;
    }
    ipart = scaled / scale;
    fpart = scaled % scale;

    /* 整数位不够时自动扩位（含符号），不丢高位 */
    for (i = ipart; i >= 10; i /= 10)
    {
        digits++;
    }
    if (digits + neg > int_w)
    {
        int_w = digits + neg;
    }
    if (int_w == 0)
    {
        int_w = 1;
    }

    if (neg)
    {
        BSP_LCD_ShowChar(x, y, '-', fc, bc);
        BSP_LCD_ShowInt((uint16_t)(x + BSP_LCD_FONT_WIDTH), y, ipart, (uint8_t)(int_w - 1), fc, bc);
    }
    else
    {
        BSP_LCD_ShowInt(x, y, ipart, int_w, fc, bc);
    }
    cx = (uint16_t)(x + int_w * BSP_LCD_FONT_WIDTH);
    if (dec_w == 0)
    {
        return;
    }

    BSP_LCD_ShowChar(cx, y, '.', fc, bc);
    cx += BSP_LCD_FONT_WIDTH;
    for (i = 0; i < dec_w; i++)
    { /* 小数部分左对齐零填充 */
        scale /= 10;
        BSP_LCD_ShowChar(cx, y, (char)('0' + (fpart / scale) % 10), fc, bc);
        cx += BSP_LCD_FONT_WIDTH;
    }
}
