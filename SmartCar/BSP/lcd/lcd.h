#ifndef __LCD_H
#define __LCD_H

/* ST7735 移植胶水层头文件
 * 底层组件驱动见 st7735.h / st7735_reg.h
 * 板载屏：CS=PE11，RST 硬接 NRST，背光 PE10=TIM1_CH2N
 * 外接屏：CS=PE9，RST=PD9（软件复位），背光 PD10=GPIO（共用 SPI4/DC=PE13，
 *         与板载屏互斥使用，编译期经 app_config.h 的 LCD_TARGET_EXTERNAL 选择） */

#include "main.h"
#include "st7735.h"

extern ST7735_Object_t st7735_pObj;     /* 组件对象 */
extern ST7735_Ctx_t    ST7735Ctx;       /* 屏幕参数（宽高/方向） */

int32_t LCD_Init(void);                 /* 注册总线并初始化屏幕（横屏） */

#endif /* __LCD_H */
