/**
 * @file app.h
 * @brief App 总装与起停门禁；前台及 TIM15/6/7 保持 1/2/5 ms 契约。
 * @note 移植来源：Sirius20260718/project/user/main.c、user/isr.c、code/config.c。
 * @note STC AI8051U 省赛快照（2026-07-18）；原算法未单列作者或独立许可证。
 * @note 保留源工程出处，关联 Phase 13/14/15 规约。
 */
#ifndef SMARTCAR_APP_APP_H
#define SMARTCAR_APP_APP_H

#include "main.h"

typedef enum
{
    APP_STOP_NONE = 0,
    APP_STOP_OFF_TRACK,
    APP_STOP_LOW_BATTERY,
    APP_STOP_GRAY_LOST,
    APP_STOP_GRAY_ACQUIRE,
    APP_STOP_CONTROL_OVERRUN,
    APP_STOP_LOG_OVERFLOW,
    APP_STOP_LOG_WRITE,
    APP_STOP_LOG_FULL
} app_stop_reason_t;

/* 源 config.c 的全局保留链接和类型；前台/ISR 按既有调度访问，名称按 Phase 15 统一。 */
extern uint8_t g_app_uart_tx_buffer[32];  /* 串口发送缓冲区 */
extern uint8_t g_app_uart_rx_buffer[32];  /* 串口接收缓冲区 */
extern int16_t g_app_battery_raw;         /* 电池电压ADC原始值 */
extern int16_t g_app_battery_filtered;    /* 电池电压滤波值 */
extern int32_t g_app_control_ticks;       /* 控制节拍计数（2ms） */
extern volatile uint8_t g_app_key_flag;   /* 启动键已按下 */
extern volatile uint8_t g_app_start_flag; /* 启动标志位 */
extern volatile uint8_t g_app_run_flag;   /* 运行标志位（赛道检测控制） */
extern uint16_t g_app_dl1b_distance_mm; /* 激光距离毫米，8192 无效；控制拍更新。 */

/**
 * @brief 按既有顺序初始化 BSP、参数和 App 模块。
 * @note 前台上电调用，可能阻塞；保持 SPI45SEL 补丁、参数失败路径、陀螺校准和定时器不启动。
 */
void App_Init(void);

/**
 * @brief 调度运行日志或停车采样、显示、调参和起跑。
 * @note 前台反复调用；顶部及分支内均保留停车收尾检查，避免停车 ISR 后访问 SPI4 的竞态。
 */
void App_Loop(void);

/**
 * @brief 执行 2 ms 控制拍及完整预算保护。
 * @note 仅 TIM6 调用；与 TIM15 同优先级不互抢，双保护先于控制，等待 1000/1001 拍及错误路径保持。
 */
void App_ControlISR(void);

/**
 * @brief 执行 5 ms 电池保护、看门狗和运行灯。
 * @note 仅 TIM7 调用；低压计数比较仍为 >200，保持首次停车和喂狗顺序。
 */
void App_TaskISR(void);

/**
 * @brief 运行时向日志环形队列推入 1 ms 记录。
 * @note 仅 TIM15 采样器钩子调用，不在 ISR 擦写 Flash；保留启动标志检查。
 */
void App_SampleISR(void);

/**
 * @brief 锁存首次原因并执行有界安全停车。
 * @param[in] reason APP_STOP_* 原因；NONE 不操作，其他值保留原 default 路径。
 * @note ISR/前台均可调用；先累计保护位，再在原临界区锁存首次原因，后续不覆盖；阻塞日志尾包交前台。
 */
void App_RequestStop(app_stop_reason_t reason);

/**
 * @brief 检查启动、按键和待收尾三个原标志。
 * @return 三个标志均为零返回 1，否则 0。
 * @note 无阻塞；模式切换与起跑门禁重复读取位置不缓存或合并。
 */
uint8_t App_IsStopped(void);

/**
 * @brief 按原分支求浮点绝对值。
 * @param[in] a 原浮点值，不新增 NaN/负零处理。
 * @return a<0 时取反，否则保持原值。
 * @note 保留旧环岛入口调用边。
 */
float App_AbsFloat(float a);

#endif /* SMARTCAR_APP_APP_H */
