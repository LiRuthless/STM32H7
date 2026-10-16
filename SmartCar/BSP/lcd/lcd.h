/**
 * @file lcd.h
 * @brief BSP ST7735 总线胶水；停车主循环注册并访问 SPI4。
 */
#ifndef SMARTCAR_BSP_LCD_LCD_H
#define SMARTCAR_BSP_LCD_LCD_H

/* ST7735 移植胶水层头文件
 * 底层组件驱动见 st7735.h / st7735_reg.h
 * 板载屏：CS=PE11，RST 硬接 NRST，背光 PE10=TIM1_CH2N
 * 外接屏：CS=PE9，RST=PD9（软件复位），背光 PD10=GPIO（共用 SPI4/DC=PE13，
 *         与板载屏互斥使用，编译期经 app_config.h 的 LCD_TARGET_EXTERNAL 选择） */

#include "main.h"
#include "st7735.h"

extern ST7735_Object_t g_bsp_lcdio_object; /* 组件对象 */
extern ST7735_Ctx_t ST7735Ctx;             /* 屏幕参数（宽高/方向） */

/**
 * @brief 注册 ST7735 总线并按原方向和屏型初始化。
 * @return ST7735_ERROR 注册失败；否则原样返回厂商 Init 的结果。
 * @note 启动或停车主循环调用，SPI4 阻塞且含复位延时，不与采样并发。
 */
int32_t BSP_LCDIO_Init(void);

#endif /* __LCD_H */
