/**
 * @file lcd.c
 * @brief BSP ST7735 总线胶水；停车主循环注册并访问 SPI4。
 * @details 实现 ST7735 组件要求的板级 LCD IO 胶水，按配置选择 GPIO 或 SPI 传输分支。
 */
/* ST7735 移植胶水层：SPI4 阻塞收发 + 片选/命令脚 + 背光
 * 移植自核心板 SDK 03-LCD_Test，已裁剪图片显示、渐变调光、按键测试等
 * 板载屏/外接屏由 app_config.h 的 LCD_TARGET_EXTERNAL 编译期选择（互斥使用） */

#include "lcd.h"
#include "spi.h"
#include "tim.h"
#include "app_config.h"

#if LCD_TARGET_EXTERNAL
/* 外接屏：CS=PE9，RST=PD9（软件复位） */

#define BSP_LCDIO_CS_SET HAL_GPIO_WritePin(LCD2_CS_GPIO_Port, LCD2_CS_Pin, GPIO_PIN_SET)
#define BSP_LCDIO_CS_RESET HAL_GPIO_WritePin(LCD2_CS_GPIO_Port, LCD2_CS_Pin, GPIO_PIN_RESET)
#else
/* 板载屏：RST 硬接 NRST，软件不操作 */

#define BSP_LCDIO_CS_SET HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET)
#define BSP_LCDIO_CS_RESET HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET)
#endif

/* DC（数据/命令），两屏共用 PE13 */
#define BSP_LCDIO_RS_SET HAL_GPIO_WritePin(LCD_WR_RS_GPIO_Port, LCD_WR_RS_Pin, GPIO_PIN_SET)
#define BSP_LCDIO_RS_RESET HAL_GPIO_WritePin(LCD_WR_RS_GPIO_Port, LCD_WR_RS_Pin, GPIO_PIN_RESET)

#define BSP_LCDIO_SPI (&hspi4)

static void BSP_LCDIO_SetReset(void);
static void BSP_LCDIO_ClearReset(void);
static int32_t BSP_LCDIO_InitBus(void);
static int32_t BSP_LCDIO_GetTick(void);
static int32_t BSP_LCDIO_WriteRegister(uint8_t reg, uint8_t *pdata, uint32_t length);
static int32_t BSP_LCDIO_ReadRegister(uint8_t reg, uint8_t *pdata);
static int32_t BSP_LCDIO_SendData(uint8_t *pdata, uint32_t length);
static int32_t BSP_LCDIO_ReceiveData(uint8_t *pdata, uint32_t length);

/* ST7735 组件注册时只读的 SPI4 回调表，地址与函数项在运行期保持不变。 */
static ST7735_IO_t s_st7735_io =
{
    BSP_LCDIO_InitBus,
    NULL,
    0,
    BSP_LCDIO_WriteRegister,
    BSP_LCDIO_ReadRegister,
    BSP_LCDIO_SendData,
    BSP_LCDIO_ReceiveData,
    BSP_LCDIO_GetTick
};

/* LCD 组件上下文，由停车主循环初始化和绘制路径读写；ISR 不访问。 */
ST7735_Object_t g_bsp_lcdio_object;

/* 横屏初始化（对应 SDK 的 03-LCD_Test_0_96 目标；外接屏若为 1.8 寸，
 * 改 Type 为 ST7735_1_8_inch_screen、Panel 为 BOE_Panel 即可） */
/**
 * @brief 注册 ST7735 总线并按原方向和屏型初始化。
 * @details 按现有调用顺序使用 BSP_LCDIO_ClearReset、HAL_Delay、BSP_LCDIO_SetReset、ST7735_RegisterBusIO
 *     完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @return ST7735_ERROR 注册失败；否则原样返回厂商 Init 的结果。
 * @note 启动或停车主循环调用，SPI4 阻塞且含复位延时，不与采样并发。
 */
int32_t BSP_LCDIO_Init(void)
{
    ST7735Ctx.Orientation = ST7735_ORIENTATION_LANDSCAPE_ROT180;
    ST7735Ctx.Panel = HannStar_Panel;
    ST7735Ctx.Type = ST7735_0_9_inch_screen;

    BSP_LCDIO_CS_SET; /* 空闲时 CS 拉高，让出 SPI4 总线 */

    /* 外接屏需要软件复位（板载屏 RST 硬接 NRST 无需操作） */
    BSP_LCDIO_ClearReset();
    HAL_Delay(20);
    BSP_LCDIO_SetReset();
    HAL_Delay(120);

    if (ST7735_RegisterBusIO(&g_bsp_lcdio_object, &s_st7735_io) != ST7735_OK)
    {
        return ST7735_ERROR;
    }
    return ST7735_LCD_Driver.Init(&g_bsp_lcdio_object, ST7735_FORMAT_RBG565, &ST7735Ctx);
}

/* 总线初始化钩子：启动背光 */
#if LCD_TARGET_EXTERNAL
/**
 * @brief 设置外接 LCD 复位高电平；板载配置保留空钩子。
 * @details 外接屏配置写 RST GPIO 高电平；该条件分支没有返回值或软件状态发布。
 * @note 停车主循环使用 SPI4，不与 IMU 采样并发。
 */
static void BSP_LCDIO_SetReset(void)
{
    HAL_GPIO_WritePin(LCD2_RST_GPIO_Port, LCD2_RST_Pin, GPIO_PIN_SET);
}

/**
 * @brief 设置外接 LCD 复位低电平；板载配置保留空钩子。
 * @details 外接屏配置写 RST GPIO 低电平；该条件分支没有返回值或软件状态发布。
 * @note 停车主循环使用 SPI4，不与 IMU 采样并发。
 */
static void BSP_LCDIO_ClearReset(void)
{
    HAL_GPIO_WritePin(LCD2_RST_GPIO_Port, LCD2_RST_Pin, GPIO_PIN_RESET);
}
#else
/**
 * @brief 设置外接 LCD 复位高电平；板载配置保留空钩子。
 * @details 板载屏 RST 硬接 NRST，函数保持空操作以维持两种配置一致的初始化调用序列。
 * @note 停车主循环使用 SPI4，不与 IMU 采样并发。
 */
static void BSP_LCDIO_SetReset(void)
{
    /* 板载屏复位硬接 NRST，保留空钩子以维持原调用时序。 */
}

/**
 * @brief 设置外接 LCD 复位低电平；板载配置保留空钩子。
 * @details 板载屏 RST 硬接 NRST，函数保持空操作以维持两种配置一致的初始化调用序列。
 * @note 停车主循环使用 SPI4，不与 IMU 采样并发。
 */
static void BSP_LCDIO_ClearReset(void)
{
    /* 板载屏复位硬接 NRST，保留空钩子以维持原调用时序。 */
}
#endif

/**
 * @brief 按当前 LCD 配置启动背光。
 * @details 按现有调用顺序使用 HAL_GPIO_WritePin、HAL_TIMEx_PWMN_Start 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @return ST7735_OK；保持原 HAL 结果忽略行为。
 * @note 停车主循环使用 SPI4，不与 IMU 采样并发。
 */
static int32_t BSP_LCDIO_InitBus(void)
{
#if LCD_TARGET_EXTERNAL
    HAL_GPIO_WritePin(LCD2_BLK_GPIO_Port, LCD2_BLK_Pin, GPIO_PIN_SET); /* 外接屏背光 GPIO 点亮 */
#else
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2); /* 板载屏背光 TIM1_CH2N PWM */
#endif
    return ST7735_OK;
}

/**
 * @brief 提供厂商总线使用的毫秒计数。
 * @return HAL_GetTick 转为 int32_t 的原值。
 * @note 停车主循环使用 SPI4，不与 IMU 采样并发。
 */
static int32_t BSP_LCDIO_GetTick(void)
{
    return (int32_t)HAL_GetTick();
}

/**
 * @brief 按片选、命令与数据次序写寄存器。
 * @details 按现有调用顺序使用 HAL_SPI_Transmit 完成本函数处理；条件分支和状态写入均保持函数体所示顺序。
 * @param[in] reg 寄存器地址，使用调用方指定的原始字节或 16 位地址。
 * @param[in,out] pdata 数据缓冲，必须非空且具有传输所需容量；HAL 调用期间读取或写入。
 * @param[in] length 传输字节数，保持原 uint32_t 到 HAL 长度的转换。
 * @return HAL 状态累计大于零返回 -1，否则返回 0。
 * @note 停车主循环使用 SPI4，不与 IMU 采样并发。
 */
static int32_t BSP_LCDIO_WriteRegister(uint8_t reg, uint8_t *pdata, uint32_t length)
{
    int32_t result;
    BSP_LCDIO_CS_RESET;
    BSP_LCDIO_RS_RESET;
    result = HAL_SPI_Transmit(BSP_LCDIO_SPI, &reg, 1, 100);
    BSP_LCDIO_RS_SET;
    if (length > 0)
    {
        result += HAL_SPI_Transmit(BSP_LCDIO_SPI, pdata, length, 500);
    }
    BSP_LCDIO_CS_SET;
    return (result > 0) ? -1 : 0;
}

/**
 * @brief 按原总线时序读取一个寄存器字节。
 * @details 按现有调用顺序使用 HAL_SPI_Transmit、HAL_SPI_Receive 完成本函数处理。
 * @param[in] reg 寄存器地址，使用调用方指定的原始字节或 16 位地址。
 * @param[in,out] pdata 数据缓冲，必须非空且具有传输所需容量；HAL 调用期间读取或写入。
 * @return HAL 状态累计大于零返回 -1，否则返回 0。
 * @note 停车主循环使用 SPI4，不与 IMU 采样并发。
 */
static int32_t BSP_LCDIO_ReadRegister(uint8_t reg, uint8_t *pdata)
{
    int32_t result;
    BSP_LCDIO_CS_RESET;
    BSP_LCDIO_RS_RESET;
    result = HAL_SPI_Transmit(BSP_LCDIO_SPI, &reg, 1, 100);
    BSP_LCDIO_RS_SET;
    result += HAL_SPI_Receive(BSP_LCDIO_SPI, pdata, 1, 500);
    BSP_LCDIO_CS_SET;
    return (result > 0) ? -1 : 0;
}

/**
 * @brief 阻塞发送 LCD 数据并释放片选。
 * @details 按现有调用顺序使用 HAL_SPI_Transmit 完成本函数处理。
 * @param[in,out] pdata 数据缓冲，必须非空且具有传输所需容量；HAL 调用期间读取或写入。
 * @param[in] length 传输字节数，保持原 uint32_t 到 HAL 长度的转换。
 * @return HAL 状态大于零返回 -1，否则返回 0。
 * @note 停车主循环使用 SPI4，不与 IMU 采样并发。
 */
static int32_t BSP_LCDIO_SendData(uint8_t *pdata, uint32_t length)
{
    int32_t result;
    BSP_LCDIO_CS_RESET;
    result = HAL_SPI_Transmit(BSP_LCDIO_SPI, pdata, length, 500);
    BSP_LCDIO_CS_SET;
    return (result > 0) ? -1 : 0;
}

/**
 * @brief 阻塞接收 LCD 数据并释放片选。
 * @details 按现有调用顺序使用 HAL_SPI_Receive 完成本函数处理。
 * @param[in,out] pdata 数据缓冲，必须非空且具有传输所需容量；HAL 调用期间读取或写入。
 * @param[in] length 传输字节数，保持原 uint32_t 到 HAL 长度的转换。
 * @return HAL 状态大于零返回 -1，否则返回 0。
 * @note 停车主循环使用 SPI4，不与 IMU 采样并发。
 */
static int32_t BSP_LCDIO_ReceiveData(uint8_t *pdata, uint32_t length)
{
    int32_t result;
    BSP_LCDIO_CS_RESET;
    result = HAL_SPI_Receive(BSP_LCDIO_SPI, pdata, length, 500);
    BSP_LCDIO_CS_SET;
    return (result > 0) ? -1 : 0;
}
