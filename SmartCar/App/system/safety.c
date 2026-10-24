/**
 * @file safety.c
 * @brief 首因锁存、立即停车与前台收尾。
 * @details ISR与前台累计保护位；只在原临界区锁存首因和清标志，阻塞日志尾包留前台。
 */
/**
 * @note 移植来源：Sirius20260718/project/user/main.c、user/isr.c、code/config.c。
 * @note STC AI8051U 省赛快照（2026-07-18）；原算法未单列作者或独立许可证。
 * @note 保留源工程出处，关联 Phase 13/14/15 规约。
 */
#include "bsp.h"
#include "app_config.h"
#include "system/safety.h"
#include "system/lifecycle.h"
#include "sensing/battery.h"
#include "control/tracking.h"
#include "control/motor.h"
#include "services/datalog.h"
#include "services/command_dispatch.h"
#include "system/tuning_application.h"
#include "services/telemetry.h"
#include "control/tuning.h"
#include <stdio.h>

/* 前台/ISR共享停车事务，首因仅由成功起跑复位。 */
static volatile uint8_t s_stop_pending = 0;
static volatile app_stop_reason_t s_stop_reason = APP_STOP_NONE;
/* TIM7累计区间内低压次数，成功起跑前清零。 */
static int16_t s_low_count = 0;

/**
 * @brief 锁存首次原因并执行有界安全停车。
 * @details NONE立即返回；其他原因先映射保护位并累计，再在PRIMASK临界区内锁存首因、
 *     设置待收尾并清运行标志；退出后发布日志摘要，仅首次请求切输出、停调度并排队停车事件。
 * @param[in] reason 本次停车原因枚举；零值直接返回，不累计保护位或操作硬件。
 * @note ISR/前台均可调用；先累计保护位，再在原临界区锁存首次原因，后续不覆盖；阻塞日志尾包交前台。
 */
void Safety_RequestStop(app_stop_reason_t reason)
{
    uint16_t flag = 0;
    uint8_t first;
    uint32_t mask;
    if (reason == APP_STOP_NONE)
    {
        return;
    }
    switch (reason)
    {
        case APP_STOP_OFF_TRACK:
            flag = TRACKING_PROTECT_EM_LOST;
            break;
        case APP_STOP_LOW_BATTERY:
            flag = TRACKING_PROTECT_LOW_BATTERY;
            break;
        case APP_STOP_GRAY_LOST:
            flag = TRACKING_PROTECT_GRAY_LOST;
            break;
        case APP_STOP_GRAY_ACQUIRE:
            flag = TRACKING_PROTECT_ACQUIRE;
            break;
        case APP_STOP_CONTROL_OVERRUN:
            flag = TRACKING_PROTECT_OVERRUN;
            break;
        case APP_STOP_LOG_OVERFLOW:
            flag = TRACKING_PROTECT_LOG_OVERFLOW;
            break;
        case APP_STOP_LOG_WRITE:
            flag = TRACKING_PROTECT_LOG_WRITE;
            break;
        case APP_STOP_LOG_FULL:
            flag = TRACKING_PROTECT_LOG_FULL;
            break;
        default:
            break;
    }
    Tracking_AddProtection(flag);
    /* 保护位先累加；原临界区只锁存第一次原因和标志，不吞掉后续保护位。 */
    mask = BSP_Time_EnterCritical();
    first = (s_stop_reason == APP_STOP_NONE);
    if (first)
    {
        s_stop_reason = reason;
        s_stop_pending = 1;
        Lifecycle_ClearRunFlags();
    }
    BSP_Time_ExitCritical(mask);
    Datalog_SetStopSummary((uint8_t)s_stop_reason, Tracking_GetState()->protection_flags);
    if (!first)
    {
        return;
    }

    /* 只对首次请求切断输出和三条调度线；阻塞式日志收尾留给前台。 */
    Tuning_Stop();
    Motor_EmergencyStop();
    BSP_PWM_SetDuty(BSP_PWM_FAN, (uint32_t)Motor_GetState()->fan_duty_idle);
    BSP_Sampler_Stop();
    BSP_Runtime_StopTimers();
    Tracking_Stopped((uint8_t)s_stop_reason);
    /* 首次停车事件在停定时链后取固定快照，重复保护不重复入事件队列。 */
    CommandDispatch_PublishRunStopped((uint8_t)s_stop_reason);

}

/**
 * @brief 在前台完成日志尾包并报告首次停车。
 * @details 先复制停车首因，有条件喂狗后在前台刷写日志尾包，再完成调参停车收尾和再次喂狗；
 *     按当前日志计数输出一次摘要，最后清待收尾标志并要求按键释放。
 * @note 可能阻塞，禁止 ISR 调用；保留前后喂狗、一次报告、清 pending 和按键释放次序。
 */
void Safety_FinalizeStop(void)
{
    /* 仅在待收尾时进入：先复制首因，刷写尾包并完成调参收尾，再输出摘要、
     * 清pending并要求按键释放；运行标志已在立即停车阶段清除。 */
    app_stop_reason_t reason = s_stop_reason;

    if (Lifecycle_IsWatchdogStarted())
    {
        BSP_WDT_Feed();
    }
#if DATALOG_ENABLE
    Datalog_Stop();
#endif
    TuningApplication_FinalizeStop();
    if (Lifecycle_IsWatchdogStarted())
    {
        BSP_WDT_Feed();
    }

    {
        const datalog_status_t *log = Datalog_GetStatus();
        char message[192];
        snprintf(message, sizeof(message),
            "stop: reason=%u protect=%u fault=%u accepted=%lu written=%lu rejected=%lu pending=%lu\r\n",
            (unsigned)reason, (unsigned)Tracking_GetState()->protection_flags, (unsigned)log->fault,
            (unsigned long)log->accepted, (unsigned long)log->written, (unsigned long)log->rejected,
            (unsigned long)log->pending);
        BSP_UART_WriteString(message);
    }
    s_stop_pending = 0;
    BSP_Key_RequireRelease();
}

/**
 * @brief 读取待前台收尾标志。
 * @return 待收尾为1，否则0。
 * @note 单次volatile读取，前台/ISR可读。
 */
uint8_t Safety_IsStopPending(void)
{
    return s_stop_pending;
}

/**
 * @brief 读取已锁存的首次停车原因。
 * @return 首个非零原因；成功起跑复位后为APP_STOP_NONE。
 * @note 单次volatile读取，不清除原因。
 */
app_stop_reason_t Safety_GetStopReason(void)
{
    return s_stop_reason;
}

/**
 * @brief 在成功起跑前清除安全历史。
 * @details 依次清低压持续计数、首因和待收尾标志，保留原跨对象写入顺序。
 * @pre 定时链尚未启动，门禁已通过。
 */
void Safety_Reset(void)
{
    s_low_count = 0;
    s_stop_reason = APP_STOP_NONE;
    s_stop_pending = 0;
}

/**
 * @brief 检查本次滤波值的低压持续条件。
 * @details 先读取电池值比较低压上限，仅比较成立时再读取一次检查有效值下限；
 *     区间内先累加再比较持续次数，否则清计数。
 * @return 连续低压超过APP_BATTERY_LOW_COUNT返回1，否则0。
 * @note 仅TIM7采样后调用；本函数不采样、不请求停车。
 */
uint8_t Safety_CheckLowVoltage(void)
{
    if (Battery_GetFiltered() < APP_BATTERY_LOW_THRESHOLD
        && Battery_GetFiltered() > APP_BATTERY_MIN_VALID)
    {
        if (++s_low_count > APP_BATTERY_LOW_COUNT)
        {
            return 1;
        }
    }
    else
    {
        s_low_count = 0;
    }
    return 0;
}
