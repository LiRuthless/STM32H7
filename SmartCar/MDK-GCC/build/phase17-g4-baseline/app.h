/**
 * @file app.h
 * @brief App 总装与起停门禁；前台及 TIM15/6/7 保持 1/2/5 ms 契约。
 * @details 声明应用总装、前台循环、三线 ISR 钩子和停车接口；完整实现与共享状态约束位于 app.c。
 * @note 移植来源：Sirius20260718/project/user/main.c、user/isr.c、code/config.c。
 * @note STC AI8051U 省赛快照（2026-07-18）；原算法未单列作者或独立许可证。
 * @note 保留源工程出处，关联 Phase 13/14/15 规约。
 */
#ifndef SMARTCAR_APP_APP_H
#define SMARTCAR_APP_APP_H

#include <stdint.h>
#include "app_types.h"

/**
 * @brief 按既有顺序初始化 BSP、参数和 App 模块。
 * @note 前台上电调用，可能阻塞；保持 SPI45SEL 补丁、参数失败路径、陀螺校准和定时器不启动。 完整契约见 app.c::App_Init。
 */
void App_Init(void);

/**
 * @brief 调度运行日志或停车采样、显示、调参和起跑。
 * @note 前台反复调用；顶部及分支内均保留停车收尾检查，避免停车 ISR 后访问 SPI4 的竞态。 完整契约见 app.c::App_Loop。
 */
void App_Loop(void);

/**
 * @brief 执行 2 ms 控制拍及完整预算保护。
 * @note 仅 TIM6 调用；与 TIM15 同优先级不互抢，双保护先于控制，等待 1000/1001 拍及错误路径保持。 完整契约见 app.c::App_ControlISR。
 */
void App_ControlISR(void);

/**
 * @brief 执行 5 ms 电池保护、看门狗和运行灯。
 * @note 仅 TIM7 调用；低压计数比较仍为 >200，保持首次停车和喂狗顺序。 完整契约见 app.c::App_TaskISR。
 */
void App_TaskISR(void);

/**
 * @brief 运行时向日志环形队列推入 1 ms 记录。
 * @note 仅 TIM15 采样器钩子调用，不在 ISR 擦写 Flash；保留启动标志检查。 完整契约见 app.c::App_SampleISR。
 */
void App_SampleISR(void);

/**
 * @brief 锁存首次原因并执行有界安全停车。
 * @note ISR/前台均可调用；先累计保护位，再在原临界区锁存首次原因，后续不覆盖；阻塞日志尾包交前台。 完整契约见 app.c::App_RequestStop。
 */
void App_RequestStop(app_stop_reason_t reason);

/**
 * @brief 检查启动、按键和待收尾三个原标志。
 * @note 无阻塞；模式切换与起跑门禁重复读取位置不缓存或合并。 完整契约见 app.c::App_IsStopped。
 */
uint8_t App_IsStopped(void);

#endif /* SMARTCAR_APP_APP_H */
