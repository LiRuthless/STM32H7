#include "main.h"
/**
 * @file scheduler.c
 * @brief 前台与1/2/5ms调度编排。
 * @details 保持前台二次停车收尾与三线工作次序；业务状态由生命周期、安全和感知模块持有。
 */
/**
 * @note 移植来源：Sirius20260718/project/user/main.c、user/isr.c、code/config.c。
 * @note STC AI8051U 省赛快照（2026-07-18）；原算法未单列作者或独立许可证。
 * @note 保留源工程出处，关联 Phase 13/14/15 规约。
 */
#include "bsp.h"
#include "app_config.h"
#include "app.h"
#include "scheduler.h"
#include "lifecycle.h"
#include "safety.h"
#include "battery.h"
#include "parameter_application.h"
#include "menu.h"
#include "wireless.h"
#include "track_sensor.h"
#include "imu_proc.h"
#include "pid.h"
#include "legacy_pid.h"
#include "tracking.h"
#include "motor.h"
#include "datalog.h"
#include "dashboard.h"
#include "gray_sensor.h"
#include "command_dispatch.h"
#include "tuning_application.h"
#include "tuning.h"

/**
 * @brief 调度运行日志或停车采样、显示、调参和起跑。
 * @details 先完成既有停车收尾；运行分支先服务Flash，再非阻塞服务SCB并返回。
 *     停车分支完成二次收尾检查后先服务通信，再采样与刷新界面；Flash/LCD既有阻塞结束后
 *     由下一次前台服务检查5ms到期，不把通信等待带入高优先级控制。
 * @note 前台反复调用；顶部及分支内均保留停车收尾检查，避免停车 ISR 后访问 SPI4 的竞态。
 */
void Scheduler_Loop(void)
{
    if (Lifecycle_IsWatchdogStarted() && Lifecycle_IsKeyActive() == 0)
    {
        BSP_WDT_Feed(); /* TIM7 停止后的停车态喂狗路径 */
    }

    if (Safety_IsStopPending())
    {
        Safety_FinalizeStop(); /* 日志尾包刷写只在主循环执行 */
    }

    Test_InjectStop();
    if (Lifecycle_IsKeyActive())
    {
        Datalog_Service();
        CommandDispatch_Service();
        CommandDispatch_ServiceLink();
        if (Safety_IsStopPending())
        {
            Safety_FinalizeStop();
        }
        return;
    }

    if (Lifecycle_IsKeyActive() == 0)
    {
        /* 顶部检查后可能被停车ISR打断；界面/采样前再次完成收尾。 */
        if (Safety_IsStopPending())
        {
            Safety_FinalizeStop();
        }
        CommandDispatch_Service();
        CommandDispatch_ServiceLink();
        Track_ReadAdc(); /* 刷新电感值，让菜单页能看实时偏差 */
        Gray_Update();
        (void)Track_UpdateError();
        Tracking_SetTrackOutput(PID_UpdateTrack(Track_GetState()->error, IMU_GetState()->accel[0]));

        /* 电池电压监测（启动前 TIM7 未运行，在此采样滤波，
         * 供菜单显示与低压保护；保护动作在 App_TaskISR 中执行） */
        Battery_Update();

#if LCD_TARGET_EXTERNAL
        Menu_Update(); /* 外接屏：完整按键菜单 */
#else
        Dashboard_Update(); /* 板载屏：单页仪表盘刷新 */
#endif
        Wireless_Adjust();   /* 无线串口调参 */
        Param_ServiceSave(); /* 串口改参静默3秒后保存 */

        /* 状态灯：停车时慢闪（主循环软件定时，TIM7 启动前也生效）。
         * PE3 蓝灯经 NPN 驱动，高电平点亮 */
        {
            /* 停车前台LED软件时基，跨起跑保留，单位ms。 */
            static uint32_t s_led_last = 0;
            uint32_t now_ms = BSP_Time_GetTick();
            if ((uint32_t)(now_ms - s_led_last) >= 250u)
            {
                s_led_last = now_ms;
                BSP_Runtime_ToggleLed();
            }
        }

        bsp_key_event_t event = BSP_Key_PollEvent();
        if (event == BSP_KEY_TOGGLE)
        {
            (void)Lifecycle_TrySetMode((uint8_t)(1u - Tracking_GetState()->mode));
        }
        if (event == BSP_KEY_START)
        {
            (void)Lifecycle_TryStart();
        }
    }
}

/**
 * @brief 执行 2 ms 控制拍及完整预算保护。
 * @details 启动门禁后推进2ms拍并刷新双传感器，先累计采集、灰度和电感保护位，
 *     再按首因优先级请求停车；无故障时更新测距、IMU和双状态机，等待结束后执行电机控制。
 *     所有路径汇合后提交调参和日志快照，最后用无符号周期差检查完整900us预算。
 * @note 仅 TIM6 调用；与 TIM15 同优先级不互抢，双保护先于控制，等待 1000/1001 拍及错误路径保持。
 */
void Scheduler_ControlISR(void)
{
    uint32_t start;
    uint16_t flags = 0;
    if (!Lifecycle_IsStarted())
    {
        return;
    }
    start = BSP_Time_GetCycles();
    Lifecycle_AdvanceControlTick();
    Track_ReadAdc();
    Gray_Update();
    if (Gray_GetState()->acquisition_status)
    {
        flags |= TRACKING_PROTECT_ACQUIRE;
    }
    if (!Gray_GetState()->calibration_valid)
    {
        flags |= TRACKING_PROTECT_CALIBRATION;
    }
    if (!Gray_GetState()->acquisition_status && Gray_GetState()->calibration_valid
        && !Gray_GetState()->line_valid)
    {
        flags |= TRACKING_PROTECT_GRAY_LOST;
    }
    if (Track_GetState()->filtered[0] + Track_GetState()->filtered[1]
            + Track_GetState()->filtered[2] + Track_GetState()->filtered[3]
        <= 300)
    {
        flags |= TRACKING_PROTECT_EM_LOST;
    }
    Tracking_AddProtection(flags);
    /* 全部条件先记位；同拍首次原因严格按采集→灰度→电感排序。 */
    if (flags & (TRACKING_PROTECT_ACQUIRE | TRACKING_PROTECT_CALIBRATION))
    {
        App_RequestStop(APP_STOP_GRAY_ACQUIRE);
    }
    if (flags & TRACKING_PROTECT_GRAY_LOST)
    {
        App_RequestStop(APP_STOP_GRAY_LOST);
    }
    if (flags & TRACKING_PROTECT_EM_LOST)
    {
        App_RequestStop(APP_STOP_OFF_TRACK);
    }
    if (flags)
    {
        goto complete;
    }
    BSP_DL1B_Update();
    Lifecycle_SetDistanceMm(BSP_DL1B_GetDistanceMm());
    IMU_UpdateGyroAngle();
    Tracking_Observe(1, 0);
    TuningApplication_Synchronize();
    BSP_PWM_SetDuty(BSP_PWM_FAN, (uint32_t)Motor_GetState()->fan_duty);
    if (Lifecycle_GetControlTicks() > APP_RUN_DELAY_COUNT)
    {
        Lifecycle_MarkRunning();
        Tracking_Execute();
    }
    else
    {
        Motor_EmergencyStop();
    }
    if (!Lifecycle_IsStarted())
    {
        goto complete;
    }
    Motor_ReadEncoder();
    Tracking_AccumulateMotion();
    if (Lifecycle_GetControlTicks() > APP_RUN_DELAY_COUNT)
    {
        Motor_UpdateControl();
    }
complete:
    Tracking_CompleteControl();
    tuning_context_t tuning_context;
    TuningApplication_CaptureContext(&tuning_context);
    Tuning_CompleteControl(&tuning_context);
    TuningApplication_Synchronize();
    Datalog_PublishControl();
    /* 包含快照发布及错误路径；故障后再发布停车摘要但绝不继续控制。 */
    if ((uint32_t)(BSP_Time_GetCycles() - start) >= BSP_CONTROL_BUDGET_CYCLES)
    {
        App_RequestStop(APP_STOP_CONTROL_OVERRUN);
        Datalog_PublishControl();
    }
}

/**
 * @brief 执行 5 ms 电池保护、看门狗和运行灯。
 * @details 先刷新电池采样和滤波值，再检查安全模块的持续低压计数；超过200次时请求停车，
 *     随后无条件喂狗，最后只在启动标志仍有效时保持运行灯高电平。
 * @note 仅 TIM7 调用；低压计数比较仍为 >200，保持首次停车和喂狗顺序。
 */
void Scheduler_TaskISR(void)
{
    /* 电池电压监测（源 key_start 的低压保护，5ms×200≈1s 后强制停车） */
    Battery_Update();
    if (Safety_CheckLowVoltage())
    {
        App_RequestStop(APP_STOP_LOW_BATTERY);
    }

    BSP_WDT_Feed(); /* 喂狗 */

    /* 状态灯由主循环负责（停车慢闪），此处仅在运行时保持常亮 */
    if (Lifecycle_IsStarted())
    {
        BSP_Runtime_SetRunLed();
    }
}

/**
 * @brief 运行时向日志环形队列推入 1 ms 记录。
 * @details 每个1ms采样钩子先重读启动标志，仅运行时向RAM日志队列写入本拍记录。
 * @note 仅 TIM15 采样器钩子调用，不在 ISR 擦写 Flash；保留启动标志检查。
 */
void Scheduler_SampleISR(void)
{
    if (Lifecycle_IsStarted())
    {
        Datalog_Push();
    }
}
