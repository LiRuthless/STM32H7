# ST7735 显示与共享总线

现行源码核对基线：8d5ab2faf68432ae03b2c1f91bc4593c7dc7cc3e（2026-11-07）。本文为实现事实说明，未重新批准算法或硬件参数；未执行固件/实机验收。

<a id="f29-lcd"></a>
## F29.LCD 两种屏幕配置

BSP封装ST MCD ST7735组件及胶水，不包含菜单业务状态。LCDIO固定用SPI4（PE12 SCK/PE14 MOSI，PE5 MISO），与IMU共享。LCD_TARGET_EXTERNAL=0默认板载屏CS=PE11，无可控复位脚，背光PE10 TIM1_CH2N低有效；=1外屏CS=PE9、RST=PD9、BLK=PD10，RS两者共用PE13，复位低20ms再高120ms，外屏背光只作0/非0开关。正常停车显示，运行不刷新以避免采样SPI4交错；无驱动级互斥。

LCDIO_Init设置LANDSCAPE_ROT180、HannStar、0.9inch，注册总线后以ST7735_FORMAT_RBG565初始化；BSP_LCD_Init忽略其错误，再清黑及背光100%，Lifecycle随后清白。胶水写寄存器时CS低→RS低发命令→RS高发数据→CS高；命令HAL超时100ms、数据500ms，错误折成-1。业务BSP_LCD_*多为void，不向HMI传播设备失败。

ShowChar接受ASCII空格到波浪号，非法字符改空格；8×16区域超160×80就无动作。逐位展开字模、按现有转置布局并交换颜色字节后FillRGBRect，不能按常规行优先假设重写字模。String遇非法字符或横向越界停止。Int用12字节局部缓冲，width=0或>12改12，逆序补空格，负值用int64求绝对值；宽度不足保留原截断显示行为。Float先把小数位限6，按正负±0.5加到缩放值再转int32，再拆整数/小数；整数宽会按现有uint8循环估算，输入过大/NaN没有安全格式化承诺，不能用显示值代替控制/存储真值。

<a id="parameters"></a>
## 参数与返回边界

| 参数/符号 | 值/单位/范围 | 来源、覆盖、生效、持久化 |
|---|---|---|
| LCD_TARGET_EXTERNAL | 默认0，编译选0/1 | app_config.h；构建决定业务和引脚，无运行/Flash覆盖 |
| width/height、font | 160×80px；8×16px | bsp_lcd.c常量，不随用户编辑改变 |
| 板载背光 | percent uint8限100；CCR=percent×999/100 | TIM1 PSC23/ARR999，名义10kHz；互补低有效，引脚行为待实测 |
| 外屏背光 | percent=0关，其余开 | PD10 GPIO，无连续调光实现 |
| 发送超时 | 命令100ms，数据500ms | LCDIO单次HAL调用参数；前台阻塞，无持久化 |

菜单/仪表盘刷新、编辑对象与单位归[HMI](../hmi/operation.md#f14)；板测固定显示布局归[F33 LCD](isolated-board-tests.md#f33-lcd)。来源：[Phase02 FR18～20](../../specs/features/02-bsp-platform/requirements.md)、[Phase06](../../specs/features/06-hmi-tuning/requirements.md)。实现：[bsp_lcd.c](../../BSP/bsp_lcd.c)、[LCD胶水](../../BSP/lcd/lcd.c)、[Core tim.c](../../Core/Src/tim.c)。本次核对条件编译、字符转换/剪裁、背光和错误吞没；屏方向、颜色、共享总线与真实显示仍待验。
