/* ST7735 移植胶水层：SPI4 阻塞收发 + 片选/命令脚 + 背光
 * 移植自核心板 SDK 03-LCD_Test，已裁剪图片显示、渐变调光、按键测试等
 * 板载屏/外接屏由 app_config.h 的 LCD_TARGET_EXTERNAL 编译期选择（互斥使用） */

#include "lcd.h"
#include "spi.h"
#include "tim.h"
#include "app_config.h"

#if LCD_TARGET_EXTERNAL
/* 外接屏：CS=PE9，RST=PD9（软件复位） */
static void lcd_rst_set(void)   { HAL_GPIO_WritePin(LCD2_RST_GPIO_Port, LCD2_RST_Pin, GPIO_PIN_SET); }
static void lcd_rst_reset(void) { HAL_GPIO_WritePin(LCD2_RST_GPIO_Port, LCD2_RST_Pin, GPIO_PIN_RESET); }
#define LCD_CS_SET      HAL_GPIO_WritePin(LCD2_CS_GPIO_Port, LCD2_CS_Pin, GPIO_PIN_SET)
#define LCD_CS_RESET    HAL_GPIO_WritePin(LCD2_CS_GPIO_Port, LCD2_CS_Pin, GPIO_PIN_RESET)
#else
/* 板载屏：RST 硬接 NRST，软件不操作 */
static void lcd_rst_set(void)   { }
static void lcd_rst_reset(void) { }
#define LCD_CS_SET      HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET)
#define LCD_CS_RESET    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET)
#endif

/* DC（数据/命令），两屏共用 PE13 */
#define LCD_RS_SET      HAL_GPIO_WritePin(LCD_WR_RS_GPIO_Port, LCD_WR_RS_Pin, GPIO_PIN_SET)
#define LCD_RS_RESET    HAL_GPIO_WritePin(LCD_WR_RS_GPIO_Port, LCD_WR_RS_Pin, GPIO_PIN_RESET)

#define LCD_SPI         (&hspi4)

static int32_t lcd_init(void);
static int32_t lcd_gettick(void);
static int32_t lcd_writereg(uint8_t reg, uint8_t *pdata, uint32_t length);
static int32_t lcd_readreg(uint8_t reg, uint8_t *pdata);
static int32_t lcd_senddata(uint8_t *pdata, uint32_t length);
static int32_t lcd_recvdata(uint8_t *pdata, uint32_t length);

static ST7735_IO_t st7735_pIO = {
	lcd_init,
	NULL,
	0,
	lcd_writereg,
	lcd_readreg,
	lcd_senddata,
	lcd_recvdata,
	lcd_gettick
};

ST7735_Object_t st7735_pObj;

/* 横屏初始化（对应 SDK 的 03-LCD_Test_0_96 目标；外接屏若为 1.8 寸，
 * 改 Type 为 ST7735_1_8_inch_screen、Panel 为 BOE_Panel 即可） */
int32_t LCD_Init(void)
{
	ST7735Ctx.Orientation = ST7735_ORIENTATION_LANDSCAPE_ROT180;
	ST7735Ctx.Panel       = HannStar_Panel;
	ST7735Ctx.Type        = ST7735_0_9_inch_screen;

	LCD_CS_SET; /* 空闲时 CS 拉高，让出 SPI4 总线 */

	/* 外接屏需要软件复位（板载屏 RST 硬接 NRST 无需操作） */
	lcd_rst_reset();
	HAL_Delay(20);
	lcd_rst_set();
	HAL_Delay(120);

	if (ST7735_RegisterBusIO(&st7735_pObj, &st7735_pIO) != ST7735_OK)
		return ST7735_ERROR;
	return ST7735_LCD_Driver.Init(&st7735_pObj, ST7735_FORMAT_RBG565, &ST7735Ctx);
}

/* 总线初始化钩子：启动背光 */
static int32_t lcd_init(void)
{
#if LCD_TARGET_EXTERNAL
	HAL_GPIO_WritePin(LCD2_BLK_GPIO_Port, LCD2_BLK_Pin, GPIO_PIN_SET);    /* 外接屏背光 GPIO 点亮 */
#else
	HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2);    /* 板载屏背光 TIM1_CH2N PWM */
#endif
	return ST7735_OK;
}

static int32_t lcd_gettick(void)
{
	return (int32_t)HAL_GetTick();
}

static int32_t lcd_writereg(uint8_t reg, uint8_t *pdata, uint32_t length)
{
	int32_t result;
	LCD_CS_RESET;
	LCD_RS_RESET;
	result = HAL_SPI_Transmit(LCD_SPI, &reg, 1, 100);
	LCD_RS_SET;
	if (length > 0)
		result += HAL_SPI_Transmit(LCD_SPI, pdata, length, 500);
	LCD_CS_SET;
	return (result > 0) ? -1 : 0;
}

static int32_t lcd_readreg(uint8_t reg, uint8_t *pdata)
{
	int32_t result;
	LCD_CS_RESET;
	LCD_RS_RESET;
	result = HAL_SPI_Transmit(LCD_SPI, &reg, 1, 100);
	LCD_RS_SET;
	result += HAL_SPI_Receive(LCD_SPI, pdata, 1, 500);
	LCD_CS_SET;
	return (result > 0) ? -1 : 0;
}

static int32_t lcd_senddata(uint8_t *pdata, uint32_t length)
{
	int32_t result;
	LCD_CS_RESET;
	result = HAL_SPI_Transmit(LCD_SPI, pdata, length, 500);
	LCD_CS_SET;
	return (result > 0) ? -1 : 0;
}

static int32_t lcd_recvdata(uint8_t *pdata, uint32_t length)
{
	int32_t result;
	LCD_CS_RESET;
	result = HAL_SPI_Receive(LCD_SPI, pdata, length, 500);
	LCD_CS_SET;
	return (result > 0) ? -1 : 0;
}
