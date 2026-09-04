#ifndef __APP_CONFIG_H
#define __APP_CONFIG_H

/* 全局配置常量。模块级参数（PID、权重、环岛距离等）保留在各自模块内，与源工程一致。 */

/* 控制周期：源工程 pit_track 为 2ms，全部系数（陀螺积分 0.001、滤波 dt、
 * time>1000 起跑延时、环岛距离阈值单位）均按 2ms 标定，改周期必须同步调整。 */
#define CTRL_PERIOD_MS          2       /* TIM6：完整控制环（陀螺+循迹+差速+速度） */
#define TASK_PERIOD_MS          5       /* TIM7：辅助任务（DL1B/电池/按键/喂狗/状态灯） */
#define CTRL_DT_S               0.002f
#define RUN_DELAY_COUNT         1000    /* 起步延时：1000×2ms = 2s */

/* 独立看门狗开关（1=使能，约 1s 超时） */
#define WDG_ENABLE              1

/* ADC 归一化后满量程（16bit 原始 >>4 = 12bit），源工程全部阈值基于此标度 */
#define ADC_FULL_SCALE          4095

/* 电池电压保护（12bit ADC 值；1220≈11.5V，满电约 1308≈12.6V） */
#define BATTERY_LOW_THRESHOLD   1220
#define BATTERY_MIN_VALID       300
#define BATTERY_LOW_COUNT       200

/* IMU660RB 量程系数：陀螺仪 ±2000dps（/14.3 = °/s），加速度 ±8g（/4098 = g） */
#define GYRO_RAW_TO_DPS         14.3f
#define ACC_RAW_TO_G            4098.0f

/* 负压风扇占空比（0~10000） */
#define FAN_DUTY_IDLE           1100
#define FAN_DUTY_RUN            1600

/* 调试/调参串口 USART1 波特率 */
#define DEBUG_UART_BAUD         115200

/* 运行数据记录（详见 App/datalog.c）：按启动键即开始记录（边写边擦，几乎无延时） */
#define DATALOG_ENABLE          1       /* 1=按启动键即开始记录，0=关闭 */
#define DATALOG_DIV             1       /* 记录分频：1=每拍都记(1ms/条)，N=每 N 拍一条 */

/* 显示屏选择：0=板载 0.96 寸屏（单页仪表盘），1=外接屏（完整按键菜单）。
 * 两屏共用 SPI4，互斥使用，不可同时接 */
#define LCD_TARGET_EXTERNAL     0

#endif /* __APP_CONFIG_H */
