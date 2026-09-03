/* BSP 层 LCD 接口实现，底层为 BSP/lcd/ 下移植的 ST7735 驱动
 * 屏幕：0.96 寸 ST7735，横屏 160x80，8x16 ASCII 字库 */

#include "bsp_lcd.h"
#include "lcd/lcd.h"
#include "lcd/font.h"
#include "tim.h"

/* 横屏实际分辨率 */
const uint16_t BSP_LCD_W = 160;
const uint16_t BSP_LCD_H = 80;

#define FONT_W   8    /* 8x16 字库 */
#define FONT_H   16

void BSP_LCD_Init(void)
{
	(void)LCD_Init();            /* 初始化序列 + 横屏设置，内含背光 PWM 启动 */
	BSP_LCD_Clear(LCD_BLACK);    /* 清屏黑 */
	BSP_LCD_SetBacklight(100);   /* 默认 100% 亮度 */
}

void BSP_LCD_Clear(uint16_t color)
{
	ST7735_LCD_Driver.FillRect(&st7735_pObj, 0, 0, ST7735Ctx.Width, ST7735Ctx.Height, color);
}

/* 背光：PE10=TIM1_CH2N，ARR=999，OCNPolarity=LOW（CCR 越小越亮），
 * 换算为 percent 越大越亮。若实测反了只改此处的换算 */
void BSP_LCD_SetBacklight(uint8_t percent)
{
	uint32_t ccr;
	if (percent > 100) percent = 100;
	ccr = (uint32_t)(100 - percent) * 999 / 100;
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, ccr);
}

void BSP_LCD_ShowChar(uint16_t x, uint16_t y, char ch, uint16_t fc, uint16_t bc)
{
	uint8_t buf[FONT_W * FONT_H * 2];
	uint16_t row, col;
	const uint8_t *glyph;
	uint32_t i = 0;
	uint16_t color;

	if (ch < ' ' || ch > '~') ch = ' ';
	if ((x + FONT_W) > BSP_LCD_W || (y + FONT_H) > BSP_LCD_H) return;
	glyph = asc2_1608[(uint8_t)ch - ' '];

	for (row = 0; row < FONT_H; row++) {          /* 按行（GRAM 写入顺序）填充 */
		for (col = 0; col < FONT_W; col++) {
			color = (glyph[row] & (0x80 >> col)) ? fc : bc;
			buf[i++] = (uint8_t)(color >> 8);     /* RGB565 大端 */
			buf[i++] = (uint8_t)(color);
		}
	}
	ST7735_LCD_Driver.FillRGBRect(&st7735_pObj, x, y, buf, FONT_W, FONT_H);
}

void BSP_LCD_ShowString(uint16_t x, uint16_t y, const char *str, uint16_t fc, uint16_t bc)
{
	while (*str) {
		if (*str < ' ' || *str > '~') break;
		if ((x + FONT_W) > BSP_LCD_W) break;      /* 超出右边界停止 */
		BSP_LCD_ShowChar(x, y, *str, fc, bc);
		x += FONT_W;
		str++;
	}
}

/* 十进制右对齐显示 int32，width 为字符数（含符号位） */
void BSP_LCD_ShowInt(uint16_t x, uint16_t y, int32_t val, uint8_t width, uint16_t fc, uint16_t bc)
{
	char buf[12];    /* 符号 + 最多10位数字 */
	uint8_t len = 0, i;
	uint32_t mag;

	if (width == 0 || width > sizeof(buf)) width = sizeof(buf);
	mag = (val < 0) ? (uint32_t)(-(int64_t)val) : (uint32_t)val;

	do {                                  /* 先取数字（逆序） */
		buf[len++] = (char)('0' + mag % 10);
		mag /= 10;
	} while (mag && len < sizeof(buf));
	if (val < 0 && len < sizeof(buf)) buf[len++] = '-';

	while (len < width) buf[len++] = ' '; /* 左侧补空格右对齐 */

	for (i = 0; i < width; i++)           /* 正序输出 */
		BSP_LCD_ShowChar((uint16_t)(x + i * FONT_W), y, buf[width - 1 - i], fc, bc);
}

/* 浮点显示：先四舍五入到 dec_w 位小数，再拆整数/小数部分打印
 * （整数拆分法，避免 MicroLIB 不支持 %f） */
void BSP_LCD_ShowFloat(uint16_t x, uint16_t y, float val, uint8_t int_w, uint8_t dec_w, uint16_t fc, uint16_t bc)
{
	int32_t scale = 1;
	int32_t scaled, ipart, fpart;
	uint8_t i, neg, digits = 1;
	uint16_t cx;

	if (dec_w > 6) dec_w = 6;             /* float 精度有限，最多6位小数 */
	for (i = 0; i < dec_w; i++) scale *= 10;

	/* 四舍五入到 dec_w 位小数；符号单独处理，避免 (-1,0) 丢负号 */
	scaled = (int32_t)(val * (float)scale + ((val >= 0) ? 0.5f : -0.5f));
	neg = (scaled < 0);
	if (neg) scaled = -scaled;
	ipart = scaled / scale;
	fpart = scaled % scale;

	/* 整数位不够时自动扩位（含符号），不丢高位 */
	for (i = ipart; i >= 10; i /= 10) digits++;
	if (digits + neg > int_w) int_w = digits + neg;
	if (int_w == 0) int_w = 1;

	if (neg) {
		BSP_LCD_ShowChar(x, y, '-', fc, bc);
		BSP_LCD_ShowInt((uint16_t)(x + FONT_W), y, ipart, (uint8_t)(int_w - 1), fc, bc);
	} else {
		BSP_LCD_ShowInt(x, y, ipart, int_w, fc, bc);
	}
	cx = (uint16_t)(x + int_w * FONT_W);
	if (dec_w == 0) return;

	BSP_LCD_ShowChar(cx, y, '.', fc, bc);
	cx += FONT_W;
	for (i = 0; i < dec_w; i++) {         /* 小数部分左对齐零填充 */
		scale /= 10;
		BSP_LCD_ShowChar(cx, y, (char)('0' + (fpart / scale) % 10), fc, bc);
		cx += FONT_W;
	}
}
