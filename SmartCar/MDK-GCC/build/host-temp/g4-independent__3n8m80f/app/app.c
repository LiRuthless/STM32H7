/**
 * @file app.c
 * @brief App 总装与起停门禁；前台及 TIM15/6/7 保持 1/2/5 ms 契约。
 * @details 负责上电总装、停车前台、起跑事务和三线中断分派；读取各模块状态，控制定时链、看门狗与安全停车收尾。
 * @note 移植来源：Sirius20260718/project/user/main.c、user/isr.c、code/config.c。
 * @note STC AI8051U 省赛快照（2026-07-18）；原算法未单列作者或独立许可证。
 * @note 保留源工程出处，关联 Phase 13/14/15 规约。
 */
#include "app.h"
#include "lifecycle.h"
#include "scheduler.h"
#include "safety.h"

/**
 * @brief 转发生命周期初始化。
 * @note 调用约束与完整流程见lifecycle.c::Lifecycle_Init。
 */
void App_Init(void)
{
    Lifecycle_Init();
}

/**
 * @brief 转发前台调度。
 * @note 调用约束与完整流程见scheduler.c::Scheduler_Loop。
 */
void App_Loop(void)
{
    Scheduler_Loop();
}

/**
 * @brief 转发2ms控制拍。
 * @note 调用约束与完整流程见scheduler.c::Scheduler_ControlISR。
 */
void App_ControlISR(void)
{
    Scheduler_ControlISR();
}

/**
 * @brief 转发5ms辅助拍。
 * @note 调用约束与完整流程见scheduler.c::Scheduler_TaskISR。
 */
void App_TaskISR(void)
{
    Scheduler_TaskISR();
}

/**
 * @brief 转发1ms采样记录。
 * @note 调用约束与完整流程见scheduler.c::Scheduler_SampleISR。
 */
void App_SampleISR(void)
{
    Scheduler_SampleISR();
}

/**
 * @brief 请求安全停车。
 * @param[in] reason 停车原因；NONE无副作用返回。
 * @note ISR或前台调用；完整安全动作见safety.c::Safety_RequestStop。
 */
void App_RequestStop(app_stop_reason_t reason)
{
    Safety_RequestStop(reason);
}

/**
 * @brief 查询可进入停车事务的门禁。
 * @return 启动、按键和待收尾都清零返回1，否则0。
 * @note 完整读取顺序见lifecycle.c::Lifecycle_IsStopped。
 */
uint8_t App_IsStopped(void)
{
    return Lifecycle_IsStopped();
}
